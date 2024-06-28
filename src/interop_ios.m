// Silence deprecation warnings on modern iOS
#define GLES_SILENCE_DEPRECATION
#include "Core.h"

#if defined CC_BUILD_IOS
#include "_WindowBase.h"
#include "Bitmap.h"
#include "Input.h"
#include "Platform.h"
#include "String.h"
#include "Errors.h"
#include "Drawer2D.h"
#include "Launcher.h"
#include "Funcs.h"
#include "Gui.h"
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <UIKit/UIKit.h>
#include <UIKit/UIPasteboard.h>
#include <CoreText/CoreText.h>

#ifdef TARGET_OS_TV
	// NSFontAttributeName etc - iOS 6.0
	#define TEXT_ATTRIBUTE_FONT  NSFontAttributeName
	#define TEXT_ATTRIBUTE_COLOR NSForegroundColorAttributeName
#else
	// UITextAttributeFont etc - iOS 5.0
	#define TEXT_ATTRIBUTE_FONT  UITextAttributeFont
	#define TEXT_ATTRIBUTE_COLOR UITextAttributeTextColor
#endif

// shared state with LBackend_ios.m
UITextField* kb_widget;
CGContextRef win_ctx;
UIView* view_handle;

UIColor* ToUIColor(BitmapCol color, float A);
NSString* ToNSString(const cc_string* text);
void LInput_SetKeyboardType(UITextField* fld, int flags);
void LInput_SetPlaceholder(UITextField* fld, const char* placeholder);


@interface CCWindow : UIWindow
@end

@interface CCViewController : UIViewController<UIDocumentPickerDelegate, UIAlertViewDelegate>
@end

@interface CCAppDelegate : UIResponder<UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

static CCViewController* cc_controller;
static UIWindow* win_handle;
static cc_bool launcherMode;

static void AddTouch(UITouch* t) {
    CGPoint loc = [t locationInView:view_handle];
    int x = loc.x, y = loc.y; long ui_id = (long)t;
    Platform_Log3("POINTER %x - DOWN %i,%i", &ui_id, &x, &y);
    Input_AddTouch((long)t, loc.x, loc.y);
}

static void UpdateTouch(UITouch* t) {
    CGPoint loc = [t locationInView:view_handle];
    int x = loc.x, y = loc.y; long ui_id = (long)t;
    Platform_Log3("POINTER %x - MOVE %i,%i", &ui_id, &x, &y);
    Input_UpdateTouch((long)t, loc.x, loc.y);
}

static void RemoveTouch(UITouch* t) {
    CGPoint loc = [t locationInView:view_handle];
    int x = loc.x, y = loc.y; long ui_id = (long)t;
    Platform_Log3("POINTER %x - UP %i,%i", &ui_id, &x, &y);
    Input_RemoveTouch((long)t, loc.x, loc.y);
}

static cc_bool landscape_locked;
static UIInterfaceOrientationMask SupportedOrientations(void) {
    if (landscape_locked)
        return UIInterfaceOrientationMaskLandscape;
    return UIInterfaceOrientationMaskAll;
}

static cc_bool fullscreen = true;
static void UpdateStatusBar(void) {
    if ([cc_controller respondsToSelector:@selector(setNeedsStatusBarAppearanceUpdate)]) {
        // setNeedsStatusBarAppearanceUpdate - iOS 7.0
        [cc_controller setNeedsStatusBarAppearanceUpdate];
    } else {
        [[UIApplication sharedApplication] setStatusBarHidden:fullscreen withAnimation:UIStatusBarAnimationNone];
    }
}

static CGRect GetViewFrame(void) {
    UIScreen* screen = UIScreen.mainScreen;
    return fullscreen ? screen.bounds : screen.applicationFrame;
}

@implementation CCWindow

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent *)event {
    // touchesBegan:withEvent - iOS 2.0
    for (UITouch* t in touches) AddTouch(t);
    
    // clicking on the background should dismiss onscren keyboard
    if (launcherMode) { [view_handle endEditing:NO]; }
}

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent *)event {
    // touchesMoved:withEvent - iOS 2.0
    for (UITouch* t in touches) UpdateTouch(t);
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent *)event {
    // touchesEnded:withEvent - iOS 2.0
    for (UITouch* t in touches) RemoveTouch(t);
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent *)event {
    // touchesCancelled:withEvent - iOS 2.0
    for (UITouch* t in touches) RemoveTouch(t);
}

- (BOOL)isOpaque { return YES; }
@end


@implementation CCViewController
- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    // supportedInterfaceOrientations - iOS 6.0
    return SupportedOrientations();
}

- (BOOL)shouldAutorotate {
    // shouldAutorotate - iOS 6.0
    return YES;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)ori {
    // shouldAutorotateToInterfaceOrientation - iOS 2.0
    if (landscape_locked && !(ori == UIInterfaceOrientationLandscapeLeft || ori == UIInterfaceOrientationLandscapeRight))
        return NO;
    return YES;
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id)coordinator {
    // viewWillTransitionToSize:withTransitionCoordinator - iOS 8.0
    Window_Main.Width  = size.width;
    Window_Main.Height = size.height;
    
    Event_RaiseVoid(&WindowEvents.Resized);
    [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}

// ==== UIDocumentPickerDelegate ====
static FileDialogCallback open_dlg_callback;
static char save_buffer[FILENAME_SIZE];
static cc_string save_path = String_FromArray(save_buffer);

static void DeleteExportTempFile(void) {
    if (!save_path.length) return;
    
    char path[NATIVE_STR_LEN];
    String_EncodeUtf8(path, &save_path);
    unlink(path);
    save_path.length = 0;
}

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentAtURL:(NSURL *)url {
    // documentPicker:didPickDocumentAtURL - iOS 8.0
    NSString* str    = url.path;
    const char* utf8 = str.UTF8String;
    
    char tmpBuffer[NATIVE_STR_LEN];
    cc_string tmp = String_FromArray(tmpBuffer);
    String_AppendUtf8(&tmp, utf8, String_Length(utf8));
    
    DeleteExportTempFile();
    if (!open_dlg_callback) return;
    open_dlg_callback(&tmp);
    open_dlg_callback = NULL;
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller {
    // documentPickerWasCancelled - iOS 8.0
    DeleteExportTempFile();
}

static cc_bool kb_active;
- (void)keyboardDidShow:(NSNotification*)notification {
    NSDictionary* info = notification.userInfo;
    if (kb_active) return;
    // TODO this is wrong
    // TODO this doesn't actually trigger view resize???
    kb_active = true;
    
    double interval   = [[info objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
    NSInteger curve   = [[info objectForKey:UIKeyboardAnimationCurveUserInfoKey] integerValue];
    CGRect kbFrame    = [[info objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
    CGRect winFrame   = view_handle.frame;
    
    cc_bool can_shift = true;
    // would the active input widget be pushed offscreen?
    if (kb_widget) {
        can_shift = kb_widget.frame.origin.y > kbFrame.size.height;
    }
    if (can_shift) winFrame.origin.y = -kbFrame.size.height;
    kb_widget = nil;
    
    Platform_LogConst("APPEAR");
    [UIView animateWithDuration:interval delay: 0.0 options:curve animations:^{
        view_handle.frame = winFrame;
    } completion:nil];
}

- (void)keyboardDidHide:(NSNotification*)notification {
    NSDictionary* info = notification.userInfo;
    if (!kb_active) return;
    kb_active = false;
    kb_widget = nil;
    
    double interval   = [[info objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
    NSInteger curve   = [[info objectForKey:UIKeyboardAnimationCurveUserInfoKey] integerValue];
    CGRect winFrame   = view_handle.frame;
    winFrame.origin.y = 0;
    
    Platform_LogConst("VANISH");
    [UIView animateWithDuration:interval delay: 0.0 options:curve animations:^{
       view_handle.frame = winFrame;
    } completion:nil];
}

- (BOOL)prefersStatusBarHidden {
    // prefersStatusBarHidden - iOS 7.0
    return fullscreen;
}

- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures {
    // preferredScreenEdgesDeferringSystemGestures - iOS 11.0
    // recent iOS versions have a 'bottom home bar', which when swiped up,
    //  switches out of ClassiCube and to the app list menu
    // overriding this forces the user to swipe up twice, which should
    //  significantly the chance of accidentally triggering this gesture
    return UIRectEdgeBottom;
}

// == UIAlertViewDelegate ==
static int alert_completed;
- (void)alertView:(UIAlertView *)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex {
    alert_completed = true;
}
@end

@implementation CCAppDelegate

- (void)runMainLoop {
    extern int ios_main(int argc, char** argv);
    ios_main(1, NULL);
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Override point for customization after application launch.
    // schedule the actual main loop to run in next CFRunLoop iteration
    //  (as calling ios_main here doesn't work properly)
    [self performSelector:@selector(runMainLoop) withObject:nil afterDelay:0.0];
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {
    // applicationWillResignActive - iOS 2.0
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and invalidate graphics rendering callbacks. Games should use this method to pause the game.
    Platform_LogConst("INACTIVE");
    Window_Main.Focused = false;
    Event_RaiseVoid(&WindowEvents.FocusChanged);
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    // applicationDidEnterBackground - iOS 4.0
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
    Platform_LogConst("BACKGROUND");
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    // applicationWillEnterForeground - iOS 4.0
    // Called as part of the transition from the background to the active state; here you can undo many of the changes made on entering the background.
    Platform_LogConst("FOREGROUND");
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    // applicationDidBecomeActive - iOS 2.0
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
    Platform_LogConst("ACTIVE");
    Window_Main.Focused = true;
    Event_RaiseVoid(&WindowEvents.FocusChanged);
}

- (void)applicationWillTerminate:(UIApplication *)application {
    // applicationWillTerminate - iOS 2.0
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
    // TODO implement somehow, prob need a variable in Program.c
}

- (UIInterfaceOrientationMask)application:(UIApplication *)application supportedInterfaceOrientationsForWindow:(UIWindow *)window {
    // supportedInterfaceOrientationsForWindow - iOS 6.0
    return SupportedOrientations();
}
@end


static void LogUnhandled(NSString* str) {
    if (!str) return;
    const char* src = [str UTF8String];
    if (!src) return;
    
    cc_string msg = String_FromReadonly(src);
    Platform_Log(msg.buffer, msg.length);
    Logger_Log(&msg);
}

// TODO: Should really be handled elsewhere, in Logger or ErrorHandler
static void LogUnhandledNSErrors(NSException* ex) {
    // last chance to log exception details before process dies
    LogUnhandled(@"About to die from unhandled NSException..");
    LogUnhandled([ex name]);
    LogUnhandled([ex reason]);
}

int main(int argc, char * argv[]) {
    NSSetUncaughtExceptionHandler(LogUnhandledNSErrors);
    
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([CCAppDelegate class]));
    }
}

// iOS textfields manage ctrl+c/v
void Clipboard_GetText(cc_string* value) { }
void Clipboard_SetText(const cc_string* value) { }


/*########################################################################################################################*
 *------------------------------------------------------Common helpers--------------------------------------------------------*
 *#########################################################################################################################*/
UIColor* ToUIColor(BitmapCol color, float A) {
    // colorWithRed:green:blue:alpha - iOS 2.0
    float R = BitmapCol_R(color) / 255.0f;
    float G = BitmapCol_G(color) / 255.0f;
    float B = BitmapCol_B(color) / 255.0f;
    return [UIColor colorWithRed:R green:G blue:B alpha:A];
}

NSString* ToNSString(const cc_string* text) {
    char raw[NATIVE_STR_LEN];
    String_EncodeUtf8(raw, text);
    return [NSString stringWithUTF8String:raw];
}


/*########################################################################################################################*
 *------------------------------------------------------Logging/Time-------------------------------------------------------*
 *#########################################################################################################################*/
void Platform_Log(const char* msg, int len) {
    char tmp[2048 + 1];
    len = min(len, 2048);
    
    Mem_Copy(tmp, msg, len); tmp[len] = '\0';
    NSLog(@"%s", tmp);
}


/*########################################################################################################################*
*---------------------------------------------------------Window----------------------------------------------------------*
*#########################################################################################################################*/
// no cursor on iOS
void Cursor_GetRawPos(int* x, int* y) { *x = 0; *y = 0; }
void Cursor_SetPosition(int x, int y) { }
void Cursor_DoSetVisible(cc_bool visible) { }

void Window_SetTitle(const cc_string* title) {
    // TODO: Implement this somehow
}

void Window_PreInit(void) { }
void Window_Init(void) {
    //Window_Main.SoftKeyboard = SOFT_KEYBOARD_RESIZE;
    // keyboard now shifts up
    Window_Main.SoftKeyboard = SOFT_KEYBOARD_SHIFT;
    Input_SetTouchMode(true);
    Input.Sources = INPUT_SOURCE_NORMAL;
    Gui_SetTouchUI(true);
    
    DisplayInfo.Depth  = 32;
    DisplayInfo.ScaleX = 1; // TODO dpi scale
    DisplayInfo.ScaleY = 1; // TODO dpi scale
    NSSetUncaughtExceptionHandler(LogUnhandledNSErrors);
}

void Window_Free(void) { }

static UIColor* CalcBackgroundColor(void) {
    // default to purple if no themed background color yet
    if (!Launcher_Theme.BackgroundColor)
        return UIColor.purpleColor;
    return ToUIColor(Launcher_Theme.BackgroundColor, 1.0f);
}

static CGRect DoCreateWindow(void) {
    // UIKeyboardWillShowNotification - iOS 2.0
    cc_controller = [CCViewController alloc];
    UpdateStatusBar();
    
    CGRect bounds = GetViewFrame();
    win_handle    = [[CCWindow alloc] initWithFrame:bounds];
    
    win_handle.rootViewController = cc_controller;
    win_handle.backgroundColor = CalcBackgroundColor();
    Window_Main.Exists   = true;
    Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
    Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;

    Window_Main.Width  = bounds.size.width;
    Window_Main.Height = bounds.size.height;
	Window_Main.SoftKeyboardInstant = true;
    
    NSNotificationCenter* notifications = NSNotificationCenter.defaultCenter;
    [notifications addObserver:cc_controller selector:@selector(keyboardDidShow:) name:UIKeyboardWillShowNotification object:nil];
    [notifications addObserver:cc_controller selector:@selector(keyboardDidHide:) name:UIKeyboardWillHideNotification object:nil];
    return bounds;
}
void Window_SetSize(int width, int height) { }

void Window_Show(void) {
    [win_handle makeKeyAndVisible];
}

void Window_RequestClose(void) {
    Window_Main.Exists = false;
    Event_RaiseVoid(&WindowEvents.Closing);
}

void Window_ProcessEvents(float delta) {
    SInt32 res;
    // manually tick event queue
    do {
        res = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, TRUE);
    } while (res == kCFRunLoopRunHandledSource);
}

void Gamepads_Init(void) {

}

void Gamepads_Process(float delta) { }

void ShowDialogCore(const char* title, const char* msg) {
    // UIAlertController - iOS 8.0
    // UIAlertAction - iOS 8.0
    // UIAlertView - iOS 2.0
    Platform_LogConst(title);
    Platform_LogConst(msg);
    NSString* _title = [NSString stringWithCString:title encoding:NSASCIIStringEncoding];
    NSString* _msg   = [NSString stringWithCString:msg encoding:NSASCIIStringEncoding];
    alert_completed  = false;
    
#ifdef TARGET_OS_TV
    UIAlertController* alert = [UIAlertController alertControllerWithTitle:_title message:_msg preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction* okBtn     = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction* act) { alert_completed = true; }];
    [alert addAction:okBtn];
    [cc_controller presentViewController:alert animated:YES completion: Nil];
#else
    UIAlertView* alert = [UIAlertView alloc];
    alert = [alert initWithTitle:_title message:_msg delegate:cc_controller cancelButtonTitle:@"OK" otherButtonTitles:nil];
    [alert show];
#endif
    
    // TODO clicking outside message box crashes launcher
    // loop until alert is closed TODO avoid sleeping
    while (!alert_completed) {
        Window_ProcessEvents(0.0);
        Thread_Sleep(16);
    }
}


@interface CCKBController : NSObject<UITextFieldDelegate>
@end

@implementation CCKBController
- (void)handleTextChanged:(id)sender {
    UITextField* src = (UITextField*)sender;
    const char* str  = src.text.UTF8String;
    
    char tmpBuffer[NATIVE_STR_LEN];
    cc_string tmp = String_FromArray(tmpBuffer);
    String_AppendUtf8(&tmp, str, String_Length(str));
    
    Event_RaiseString(&InputEvents.TextChanged, &tmp);
}

// === UITextFieldDelegate ===
- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    // textFieldShouldReturn - iOS 2.0
    Input_SetPressed(CCKEY_ENTER);
    Input_SetReleased(CCKEY_ENTER);
    return YES;
}
@end

static UITextField* text_input;
static CCKBController* kb_controller;

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) {
    if (!kb_controller) {
        kb_controller = [[CCKBController alloc] init];
        CFBridgingRetain(kb_controller); // prevent GC TODO even needed?
    }
	DisplayInfo.ShowingSoftKeyboard = true;
    
    text_input = [[UITextField alloc] initWithFrame:CGRectZero];
    text_input.hidden   = YES;
    text_input.delegate = kb_controller;
    [text_input addTarget:kb_controller action:@selector(handleTextChanged:) forControlEvents:UIControlEventEditingChanged];
    
    LInput_SetKeyboardType(text_input, args->type);
    LInput_SetPlaceholder(text_input,  args->placeholder);
    
    [view_handle addSubview:text_input];
    [text_input becomeFirstResponder];
}

void OnscreenKeyboard_SetText(const cc_string* text) {
    NSString* str = ToNSString(text);
    NSString* cur = text_input.text;
    
    // otherwise on iOS 5, this causes an infinite loop
    if (cur && [str isEqualToString:cur]) return;
    text_input.text = str;
}

void OnscreenKeyboard_Draw2D(Rect2D* r, struct Bitmap* bmp) { }
void OnscreenKeyboard_Draw3D(void) { }

void OnscreenKeyboard_Close(void) {
	DisplayInfo.ShowingSoftKeyboard = false;
    [text_input resignFirstResponder];
}

int Window_GetWindowState(void) {
    return fullscreen ? WINDOW_STATE_FULLSCREEN : WINDOW_STATE_NORMAL;
}

static void ToggleFullscreen(cc_bool isFullscreen) {
    fullscreen = isFullscreen;
    UpdateStatusBar();
    view_handle.frame = GetViewFrame();
}

cc_result Window_EnterFullscreen(void) {
    ToggleFullscreen(true); return 0;
}
cc_result Window_ExitFullscreen(void) {
    ToggleFullscreen(false); return 0;
}
int Window_IsObscured(void) { return 0; }

void Window_EnableRawMouse(void)  { DefaultEnableRawMouse(); }
void Window_UpdateRawMouse(void)  { }
void Window_DisableRawMouse(void) { DefaultDisableRawMouse(); }

void Window_LockLandscapeOrientation(cc_bool lock) {
    // attemptRotationToDeviceOrientation - iOS 5.0
    // TODO doesn't work properly.. setting 'UIInterfaceOrientationUnknown' apparently
    //  restores orientation, but doesn't actually do that when I tried it
    if (lock) {
        //NSInteger ori    = lock ? UIInterfaceOrientationLandscapeRight : UIInterfaceOrientationUnknown;
        NSInteger ori    = UIInterfaceOrientationLandscapeRight;
        UIDevice* device = UIDevice.currentDevice;
        NSNumber* value  = [NSNumber numberWithInteger:ori];
        [device setValue:value forKey:@"orientation"];
    }
    
    landscape_locked = lock;
    [UIViewController attemptRotationToDeviceOrientation];
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
    // UIDocumentPickerViewController - iOS 8.0
    // see the custom UTITypes declared in Info.plist 
    NSDictionary* fileExt_map =
    @{
      @".cw"  : @"com.classicube.client.ios-cw",
      @".dat" : @"com.classicube.client.ios-dat",
      @".lvl" : @"com.classicube.client.ios-lvl",
      @".fcm" : @"com.classicube.client.ios-fcm",
      @".zip" : @"public.zip-archive"
    };
    NSMutableArray* types = [NSMutableArray array];
    const char* const* filters = args->filters;

    for (int i = 0; filters[i]; i++) 
    {
        NSString* fileExt = [NSString stringWithUTF8String:filters[i]];
        NSString* utType  = [fileExt_map objectForKey:fileExt];
        if (utType) [types addObject:utType];
    }
    
    UIDocumentPickerViewController* dlg;
    dlg = [UIDocumentPickerViewController alloc];
    dlg = [dlg initWithDocumentTypes:types inMode:UIDocumentPickerModeOpen];
    //dlg = [dlg initWithDocumentTypes:types inMode:UIDocumentPickerModeImport];
    
    open_dlg_callback = args->Callback;
    dlg.delegate = cc_controller;
    [cc_controller presentViewController:dlg animated:YES completion: Nil];
    return 0; // TODO still unfinished
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
    if (!args->defaultName.length) return SFD_ERR_NEED_DEFAULT_NAME;
    // UIDocumentPickerViewController - iOS 8.0
    
    // save the item to a temp file, which is then (usually) later deleted by picker callbacks
    Directory_Create(FILEPATH_RAW("Exported"));
    
    save_path.length = 0;
    String_Format2(&save_path, "Exported/%s%c", &args->defaultName, args->filters[0]);
    args->Callback(&save_path);
    
    NSString* str = ToNSString(&save_path);
    NSURL* url    = [NSURL fileURLWithPath:str isDirectory:NO];
    
    UIDocumentPickerViewController* dlg;
    dlg = [UIDocumentPickerViewController alloc];
    dlg = [dlg initWithURL:url inMode:UIDocumentPickerModeExportToService];
    
    dlg.delegate = cc_controller;
    [cc_controller presentViewController:dlg animated:YES completion: Nil];
    return 0;
}


/*#########################################################################################################################*
 *-----------------------------------------------------Window creation-----------------------------------------------------*
 *#########################################################################################################################*/
@interface CC3DView : UIView
@end
static void Init3DLayer(void);

void Window_Create2D(int width, int height) {
    launcherMode  = true;
    CGRect bounds = DoCreateWindow();
    
    view_handle = [[UIView alloc] initWithFrame:bounds];
    view_handle.multipleTouchEnabled = true;
    cc_controller.view = view_handle;
}

void Window_Create3D(int width, int height) {
    launcherMode  = false;
    CGRect bounds = DoCreateWindow();
    
    view_handle = [[CC3DView alloc] initWithFrame:bounds];
    view_handle.multipleTouchEnabled = true;
    cc_controller.view = view_handle;

    Init3DLayer();
}

void Window_Destroy(void) { }


/*########################################################################################################################*
*--------------------------------------------------------GLContext--------------------------------------------------------*
*#########################################################################################################################*/
#if (CC_GFX_BACKEND & CC_GFX_BACKEND_GL_MASK)
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

static EAGLContext* ctx_handle;
static GLuint framebuffer;
static GLuint color_renderbuffer, depth_renderbuffer;
static int fb_width, fb_height;

static void UpdateColorbuffer(void) {
    CAEAGLLayer* layer = (CAEAGLLayer*)view_handle.layer;
    glBindRenderbuffer(GL_RENDERBUFFER, color_renderbuffer);
    
    if (![ctx_handle renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer])
        Logger_Abort("Failed to link renderbuffer to window");
}

static void UpdateDepthbuffer(void) {
    int backingW = 0, backingH = 0;
    
    // In case layer dimensions are different
    glBindRenderbuffer(GL_RENDERBUFFER, color_renderbuffer);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH,  &backingW);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingH);
    
    // Shouldn't happen but just in case
    if (backingW <= 0) backingW = Window_Main.Width;
    if (backingH <= 0) backingH = Window_Main.Height;
    
    glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24_OES, backingW, backingH);
}

static void CreateFramebuffer(void) {
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    
    glGenRenderbuffers(1, &color_renderbuffer);
    UpdateColorbuffer();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_renderbuffer);

    glGenRenderbuffers(1, &depth_renderbuffer);
    UpdateDepthbuffer();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_RENDERBUFFER, depth_renderbuffer);
    
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        Logger_Abort2(status, "Failed to create renderbuffer");
    
    fb_width  = Window_Main.Width;
    fb_height = Window_Main.Height;
}

void GLContext_Create(void) {
    ctx_handle = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    [EAGLContext setCurrentContext:ctx_handle];
    
    // unlike other platforms, have to manually setup render framebuffer
    CreateFramebuffer();
}
                  
void GLContext_Update(void) {
    // trying to update renderbuffer here results in garbage output,
    //  so do instead when layoutSubviews method is called
}

static void GLContext_OnLayout(void) {
    // only resize buffers when absolutely have to
    if (fb_width == Window_Main.Width && fb_height == Window_Main.Height) return;
    fb_width  = Window_Main.Width;
    fb_height = Window_Main.Height;
    
    UpdateColorbuffer();
    UpdateDepthbuffer();
}

void GLContext_Free(void) {
    glDeleteRenderbuffers(1, &color_renderbuffer); color_renderbuffer = 0;
    glDeleteRenderbuffers(1, &depth_renderbuffer); depth_renderbuffer = 0;
    glDeleteFramebuffers(1, &framebuffer);         framebuffer        = 0;
    
    [EAGLContext setCurrentContext:Nil];
}

cc_bool GLContext_TryRestore(void) { return false; }
void* GLContext_GetAddress(const char* function) { return NULL; }

cc_bool GLContext_SwapBuffers(void) {
    static GLenum discards[] = { GL_DEPTH_ATTACHMENT };
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glDiscardFramebufferEXT(GL_FRAMEBUFFER, 1, discards);
    glBindRenderbuffer(GL_RENDERBUFFER, color_renderbuffer);
    [ctx_handle presentRenderbuffer:GL_RENDERBUFFER];
    return true;
}
void GLContext_SetVSync(cc_bool vsync) { }
void GLContext_GetApiInfo(cc_string* info) { }


@implementation CC3DView

+ (Class)layerClass {
    return [CAEAGLLayer class];
}

- (void)layoutSubviews {
    [super layoutSubviews];
    GLContext_OnLayout();
}
@end

static void Init3DLayer(void) {
    // CAEAGLLayer - iOS 2.0
    CAEAGLLayer* layer = (CAEAGLLayer*)view_handle.layer;

    layer.opaque = YES;
    layer.drawableProperties =
   @{
        kEAGLDrawablePropertyRetainedBacking : [NSNumber numberWithBool:NO],
        kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8
    };
}
#endif


/*########################################################################################################################*
 *--------------------------------------------------------Updater----------------------------------------------------------*
 *#########################################################################################################################*/
const struct UpdaterInfo Updater_Info = {
	"&eRedownload and reinstall to update", 0
};
cc_bool Updater_Clean(void) { return true; }

cc_result Updater_GetBuildTime(cc_uint64* t) {
    char path[NATIVE_STR_LEN + 1] = { 0 };
    uint32_t size = NATIVE_STR_LEN;
    if (_NSGetExecutablePath(path, &size)) return ERR_INVALID_ARGUMENT;
    
    struct stat sb;
    if (stat(path, &sb) == -1) return errno;
    *t = (cc_uint64)sb.st_mtime;
    return 0;
}

cc_result Updater_Start(const char** action)   { *action = "Updating game"; return ERR_NOT_SUPPORTED; }
cc_result Updater_MarkExecutable(void)         { return 0; }
cc_result Updater_SetNewBuildTime(cc_uint64 t) { return ERR_NOT_SUPPORTED; }


/*########################################################################################################################*
 *--------------------------------------------------------Platform--------------------------------------------------------*
 *#########################################################################################################################*/
cc_result Process_StartOpen(const cc_string* args) {
    // openURL - iOS 2.0 (deprecated)
    NSString* str = ToNSString(args);
    NSURL* url    = [[NSURL alloc] initWithString:str];

    [UIApplication.sharedApplication openURL:url];
    return 0;
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
    // NSSearchPathForDirectoriesInDomains - iOS 2.0
    // https://developer.apple.com/library/archive/documentation/FileManagement/Conceptual/FileSystemProgrammingGuide/FileSystemOverview/FileSystemOverview.html
    NSArray* array = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    if (array.count <= 0) return ERR_NOT_SUPPORTED;
    
    NSString* str    = [array objectAtIndex:0];
    const char* path = [str fileSystemRepresentation];
    
    mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    return chdir(path) == -1 ? errno : 0;
}

void Platform_ShareScreenshot(const cc_string* filename) {
    // UIActivityViewController - iOS 6.0
    cc_string path; char pathBuffer[FILENAME_SIZE];
    String_InitArray(path, pathBuffer);
    String_Format1(&path, "screenshots/%s", filename);
    
    NSString* pathStr = ToNSString(&path);
    UIImage* img = [UIImage imageWithContentsOfFile:pathStr];
    
    // https://stackoverflow.com/questions/31955140/sharing-image-using-uiactivityviewcontroller
    UIActivityViewController* act;
    act = [UIActivityViewController alloc];
    act = [act initWithActivityItems:@[ @"Share screenshot via", img] applicationActivities:Nil];
    [cc_controller presentViewController:act animated:true completion:Nil];
}

void GetDeviceUUID(cc_string* str) {
    // identifierForVendor - iOS 6.0
    UIDevice* device = UIDevice.currentDevice;
    
    if ([device respondsToSelector:@selector(identifierForVendor)]) {
        NSString* string = [[device identifierForVendor] UUIDString];
        // TODO avoid code duplication
        const char* src = string.UTF8String;
        String_AppendUtf8(str, src, String_Length(src));
    }
    // TODO find a pre iOS 6 solution
}

void Directory_GetCachePath(cc_string* path) {
    // NSSearchPathForDirectoriesInDomains - iOS 2.0
    NSArray* array = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    if (array.count <= 0) return;
    
    // try to use iOS app cache folder if possible
    // https://developer.apple.com/library/archive/documentation/FileManagement/Conceptual/FileSystemProgrammingGuide/FileSystemOverview/FileSystemOverview.html
    NSString* str    = [array objectAtIndex:0];
    const char* utf8 = [str UTF8String];
        
    String_AppendUtf8(path, utf8, String_Length(utf8));
}


/*########################################################################################################################*
 *-----------------------------------------------------Font handling-------------------------------------------------------*
 *#########################################################################################################################*/
#ifndef CC_BUILD_FREETYPE
void interop_GetFontNames(struct StringsBuffer* buffer) {
    NSArray* families = UIFont.familyNames;
    NSLog(@"Families: %@", families);
    char tmpBuffer[NATIVE_STR_LEN];
    cc_string tmp = String_FromArray(tmpBuffer);
    
    for (NSString* family in families)
    {
        const char* str = family.UTF8String;
        String_AppendUtf8(&tmp, str, String_Length(str));
        StringsBuffer_Add(buffer, &tmp);
        tmp.length = 0;
    }
    StringsBuffer_Sort(buffer);
}

#include "ExtMath.h"
/*static void InitFont(struct FontDesc* desc, CTFontRef font) {
    CGFloat ascender  = CTFontGetAscent(font);
    CGFloat descender = CTFontGetDescent(font);
    
    desc->handle = font;
    desc->height = Math_Ceil(Math_AbsF(ascender) + Math_AbsF(descender));
}
 
static CTFontRef TryCreateBoldFont(NSString* name, CGFloat uiSize) {
    NSArray* fontNames = [UIFont fontNamesForFamilyName:name];
    for (NSString* fontName in fontNames)
    {
        if ([fontName rangeOfString:@"Bold" options:NSCaseInsensitiveSearch].location != NSNotFound)
            return CTFontCreateWithName((__bridge CFStringRef)name, uiSize, NULL);
    }
    return NULL;
}
 
cc_result interop_SysFontMake(struct FontDesc* desc, const cc_string* fontName, int size, int flags) {
    CGFloat uiSize = size * 96.0f / 72.0f; // convert from point size
    NSString* name = ToNSString(fontName);
    CTFontRef font = (flags & FONT_FLAGS_BOLD) ? TryCreateBoldFont(name, uiSize) : NULL;
    
    if (!font) font = CTFontCreateWithName((__bridge CFStringRef)name, uiSize, NULL);
    if (!font) return ERR_NOT_SUPPORTED;
    
    InitFont(desc, font);
    return 0;
}
 
void interop_SysMakeDefault(struct FontDesc* desc, int size, int flags) {
    char nameBuffer[256];
    cc_string name = String_FromArray(nameBuffer);
    
    NSString* defaultName = [UIFont systemFontOfSize:1.0f].fontName;
    const char* str = defaultName.UTF8String;
                             
    String_AppendUtf8(&name, str, String_Length(str));
    interop_SysFontMake(desc, &name, size, flags);
}
 
void interop_SysFontFree(void* handle) {
    CFBridgingRelease(handle);
}
 
int interop_SysTextWidth(struct DrawTextArgs* args) {
    CTFontRef font = (CTFontRef)args->font->handle;
    cc_string left = args->text, part;
    char colorCode = 'f';
    
    CFMutableAttributedStringRef str = CFAttributedStringCreateMutable(kCFAllocatorDefault, 0);
    
    while (Drawer2D_UNSAFE_NextPart(&left, &part, &colorCode))
    {
        BitmapCol color = Drawer2D_GetColor(colorCode);
        NSString* bit = ToNSString(&part);
        CFRange range = CFRangeMake(CFAttributedStringGetLength(str), bit.length);
        
        CFAttributedStringReplaceString(str, range, (__bridge CFStringRef)bit);
        
        CFAttributedStringSetAttribute(str, range, kCTFontAttributeName, font);
        CFAttributedStringSetAttribute(str, range, kCTForegroundColorAttributeName, ToUIColor(color, 1.0f));
        
        NSAttributedString* attr_bit = [[NSAttributedString alloc] initWithString:bit attributes:attrs];
        [str appendAttributedString:attr_bit];
    }
    
    CTLineRef line = CTLineCreateWithAttributedString((CFAttributedStringRef)str);
    CGRect bounds  = CTLineGetImageBounds(line, NULL);
    
    CGFloat ascent, descent, leading;
    double width = CTLineGetTypographicBounds(line, &ascent, &descent, &leading);
    
    CFRelease(line);
    CFRelease(str);
    return Math_Ceil(width);
}

void interop_SysTextDraw(struct DrawTextArgs* args, struct Context2D* ctx, int x, int y, cc_bool shadow) {
    CTFontRef font  = (CTFontRef)args->font->handle;
    cc_string left  = args->text, part;
    BitmapCol color = Drawer2D.Colors['f'];
    NSMutableAttributedString* str = [[NSMutableAttributedString alloc] init];
    
    float X = x, Y = y;
    if (shadow) { X += 1.3f; Y -= 1.3f; }
    
    while (Drawer2D_UNSAFE_NextPart(&left, &part, &color))
    {
        if (shadow) color = GetShadowColor(color);
        NSString* bit = ToNSString(&part);
        NSDictionary* attrs =
        @{
          TEXT_ATTRIBUTE_FONT  : font,
          TEXT_ATTRIBUTE_COLOR : ToUIColor(color, 1.0f)
        };
        
        if (args->font->flags & FONT_FLAGS_UNDERLINE) {
            NSNumber* value = [NSNumber numberWithInt:kCTUnderlineStyleSingle];
            [attrs setValue:value forKey:NSUnderlineStyleAttributeName];
        }
        
        NSAttributedString* attr_bit = [[NSAttributedString alloc] initWithString:bit attributes:attrs];
        [str appendAttributedString:attr_bit];
    }
    
    CTLineRef line = CTLineCreateWithAttributedString((CFAttributedStringRef)str);
    struct Bitmap* bmp = &ctx->bmp;
    
    CGContextRef cg_ctx = CGBitmapContextCreate(bmp->scan0, bmp->width, ctx->height, 8, bmp->width * 4,
                                                CGColorSpaceCreateDeviceRGB(), kCGBitmapByteOrder32Host | kCGImageAlphaNoneSkipFirst);
    CGContextSetTextPosition(cg_ctx, X, Y - font.descender);
    CTLineDraw(line, cg_ctx);
    CGContextRelease(cg_ctx);
    
    CFRelease(line);
}*/
 
 static void InitFont(struct FontDesc* desc, UIFont* font) {
    desc->handle = CFBridgingRetain(font);
    desc->height = Math_Ceil(Math_AbsF(font.ascender) + Math_AbsF(font.descender));
}

static UIFont* TryCreateBoldFont(NSString* name, CGFloat uiSize) {
    NSArray* fontNames = [UIFont fontNamesForFamilyName:name];
    for (NSString* fontName in fontNames)
    {
        if ([fontName rangeOfString:@"Bold" options:NSCaseInsensitiveSearch].location != NSNotFound)
            return [UIFont fontWithName:fontName size:uiSize];
    }
    return nil;
}

cc_result interop_SysFontMake(struct FontDesc* desc, const cc_string* fontName, int size, int flags) {
    CGFloat uiSize = size * 96.0f / 72.0f; // convert from point size
    NSString* name = ToNSString(fontName);
    UIFont* font   = (flags & FONT_FLAGS_BOLD) ? TryCreateBoldFont(name, uiSize) : nil;
    
    if (!font) font = [UIFont fontWithName:name size:uiSize];
    if (!font) return ERR_NOT_SUPPORTED;
    
    InitFont(desc, font);
    return 0;
}

void interop_SysMakeDefault(struct FontDesc* desc, int size, int flags) {
    CGFloat uiSize = size * 96.0f / 72.0f; // convert from point size
    UIFont* font;
    
    if (flags & FONT_FLAGS_BOLD) {
        font = [UIFont boldSystemFontOfSize:uiSize];
    } else {
        font = [UIFont systemFontOfSize:uiSize];
    }
    InitFont(desc, font);
}

void interop_SysFontFree(void* handle) {
    CFBridgingRelease(handle);
}

static NSMutableAttributedString* GetAttributedString(struct DrawTextArgs* args, cc_bool shadow) {
    UIFont* font   = (__bridge UIFont*)args->font->handle;
    cc_string left = args->text, part;
    char colorCode = 'f';
    NSMutableAttributedString* str = [[NSMutableAttributedString alloc] init];
    
    while (Drawer2D_UNSAFE_NextPart(&left, &part, &colorCode))
    {
        BitmapCol color   = Drawer2D_GetColor(colorCode);
        if (shadow) color = GetShadowColor(color);
        
        NSString* bit = ToNSString(&part);
        NSRange range = NSMakeRange(str.length, bit.length);
        [str.mutableString appendString:bit];
        
        [str addAttribute:TEXT_ATTRIBUTE_FONT  value:font                   range:range];
        [str addAttribute:TEXT_ATTRIBUTE_COLOR value:ToUIColor(color, 1.0f) range:range];
        
        if (args->font->flags & FONT_FLAGS_UNDERLINE) {
            NSNumber* style = [NSNumber numberWithInt:kCTUnderlineStyleSingle];
            [str addAttribute:NSUnderlineStyleAttributeName value:style range:range];
        }
    }
    return str;
}

int interop_SysTextWidth(struct DrawTextArgs* args) {
    NSMutableAttributedString* str = GetAttributedString(args, false);
    
    CTLineRef line = CTLineCreateWithAttributedString((CFAttributedStringRef)str);
    CGRect bounds  = CTLineGetImageBounds(line, NULL);
    
    CGFloat ascent, descent, leading;
    double width = CTLineGetTypographicBounds(line, &ascent, &descent, &leading);
    
    CFRelease(line);
    return Math_Ceil(width);
}

void interop_SysTextDraw(struct DrawTextArgs* args, struct Context2D* ctx, int x, int y, cc_bool shadow) {
    UIFont* font = (__bridge UIFont*)args->font->handle;
    NSMutableAttributedString* str = GetAttributedString(args, shadow);
    
    float X = x, Y = y;
    if (shadow) { X += 1.3f; Y -= 1.3f; }
    
    CTLineRef line = CTLineCreateWithAttributedString((CFAttributedStringRef)str);
    struct Bitmap* bmp = &ctx->bmp;
    
    CGContextRef cg_ctx = CGBitmapContextCreate(bmp->scan0, bmp->width, ctx->height, 8, bmp->width * 4,
                                                CGColorSpaceCreateDeviceRGB(), kCGBitmapByteOrder32Host | kCGImageAlphaNoneSkipFirst);
    CGContextSetTextPosition(cg_ctx, X, Y - font.descender);
    CTLineDraw(line, cg_ctx);
    CGContextRelease(cg_ctx);
    
    CFRelease(line);
}

/*void interop_SysTextDraw(struct DrawTextArgs* args, struct Context2D* ctx, int x, int y, cc_bool shadow) {
    UIFont* font    = (__bridge UIFont*)args->font->handle;
    cc_string left  = args->text, part;
    BitmapCol color = Drawer2D.Colors['f'];
    NSMutableAttributedString* str = [[NSMutableAttributedString alloc] init];
    
    float X = x, Y = y;
    if (shadow) { X += 1.3f; Y -= 1.3f; }
    
    while (Drawer2D_UNSAFE_NextPart(&left, &part, &color))
    {
        if (shadow) color = GetShadowColor(color);
        NSString* bit = ToNSString(&part);
        NSDictionary* attrs =
        @{
          TEXT_ATTRIBUTE_FONT : font,
          NSForegroundColorAttributeName : ToUIColor(color, 1.0f)
          };
        NSAttributedString* attr_bit = [[NSAttributedString alloc] initWithString:bit attributes:attrs];
        [str appendAttributedString:attr_bit];
    }
    
    CTLineRef line = CTLineCreateWithAttributedString((CFAttributedStringRef)str);
    struct Bitmap* bmp = &ctx->bmp;
    
    CGContextRef cg_ctx = CGBitmapContextCreate(bmp->scan0, bmp->width, ctx->height, 8, bmp->width * 4,
                          CGColorSpaceCreateDeviceRGB(), kCGBitmapByteOrder32Host | kCGImageAlphaNoneSkipFirst);
    CGContextSetTextPosition(cg_ctx, X, Y - font.descender);
    //CGContextSetTextPosition(cg_ctx, x, y + font.ascender + font.descender);
    CTLineDraw(line, cg_ctx);
    CGContextRelease(cg_ctx);
}*/
#endif


void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->width  = width;
	bmp->height = height;
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	
	win_ctx = CGBitmapContextCreate(bmp->scan0, width, height, 8, width * 4,
									CGColorSpaceCreateDeviceRGB(), kCGBitmapByteOrder32Host | kCGImageAlphaNoneSkipFirst);
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	CGImageRef image = CGBitmapContextCreateImage(win_ctx);
	view_handle.layer.contents = CFBridgingRelease(image);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
	CGContextRelease(win_ctx);
}
#endif
