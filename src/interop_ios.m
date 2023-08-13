#define GLES_SILENCE_DEPRECATION
#include "_WindowBase.h"
#include "Bitmap.h"
#include "Input.h"
#include "Platform.h"
#include "String.h"
#include "Errors.h"
#include "Drawer2D.h"
#include "Launcher.h"
#include "LBackend.h"
#include "LWidgets.h"
#include "LScreens.h"
#include "Gui.h"
#include "LWeb.h"
#include "Funcs.h"
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <UIKit/UIKit.h>
#include <UIKit/UIPasteboard.h>
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include <CoreText/CoreText.h>

@interface CCWindow : UIWindow
@end

@interface CCViewController : UIViewController<UIDocumentPickerDelegate>
@end

@interface CCAppDelegate : UIResponder<UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

static CCViewController* cc_controller;
static UIWindow* win_handle;
static UIView* view_handle;

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
static CGRect GetViewFrame(void) {
    UIScreen* screen = UIScreen.mainScreen;
    return fullscreen ? screen.bounds : screen.applicationFrame;
}

@implementation CCWindow

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch* t in touches) AddTouch(t);
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch* t in touches) UpdateTouch(t);
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch* t in touches) RemoveTouch(t);
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch* t in touches) RemoveTouch(t);
}

- (BOOL)isOpaque { return YES; }
@end


@implementation CCViewController
- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    return SupportedOrientations();
}

- (BOOL)shouldAutorotate {
    return YES;
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id)coordinator {
    WindowInfo.Width  = size.width;
    WindowInfo.Height = size.height;
    
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
    DeleteExportTempFile();
}

static cc_bool kb_active;
static UITextField* kb_widget;
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
    return fullscreen;
}

- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures {
    // recent iOS versions have a 'bottom home bar', which when swiped up,
    //  switches out of ClassiCube and to the app list menu
    // overriding this forces the user to swipe up twice, which should
    //  significantly the chance of accidentally triggering this gesture
    return UIRectEdgeBottom;
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
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and invalidate graphics rendering callbacks. Games should use this method to pause the game.
    Platform_LogConst("INACTIVE");
    WindowInfo.Focused = false;
    Event_RaiseVoid(&WindowEvents.FocusChanged);
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
    Platform_LogConst("BACKGROUND");
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    // Called as part of the transition from the background to the active state; here you can undo many of the changes made on entering the background.
    Platform_LogConst("FOREGROUND");
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
    Platform_LogConst("ACTIVE");
    WindowInfo.Focused = true;
    Event_RaiseVoid(&WindowEvents.FocusChanged);
}

- (void)applicationWillTerminate:(UIApplication *)application {
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
    // TODO implement somehow, prob need a variable in Program.c
}

- (UIInterfaceOrientationMask)application:(UIApplication *)application supportedInterfaceOrientationsForWindow:(UIWindow *)window {
    return SupportedOrientations();
}
@end

int main(int argc, char * argv[]) {
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
static UIColor* ToUIColor(BitmapCol color, float A) {
    float R = BitmapCol_R(color) / 255.0f;
    float G = BitmapCol_G(color) / 255.0f;
    float B = BitmapCol_B(color) / 255.0f;
    return [UIColor colorWithRed:R green:G blue:B alpha:A];
}

static NSString* ToNSString(const cc_string* text) {
    char raw[NATIVE_STR_LEN];
    String_EncodeUtf8(raw, text);
    return [NSString stringWithUTF8String:raw];
}

static NSMutableAttributedString* ToAttributedString(const cc_string* text) {
    cc_string left = *text, part;
    char colorCode = 'f';
    NSMutableAttributedString* str = [[NSMutableAttributedString alloc] init];
    
    while (Drawer2D_UNSAFE_NextPart(&left, &part, &colorCode))
    {
        BitmapCol color = Drawer2D_GetColor(colorCode);
        NSString* bit   = ToNSString(&part);
        NSDictionary* attrs =
        @{
          //NSFontAttributeName : font,
          NSForegroundColorAttributeName : ToUIColor(color, 1.0f)
        };
        NSAttributedString* attr_bit = [[NSAttributedString alloc] initWithString:bit attributes:attrs];
        [str appendAttributedString:attr_bit];
    }
    return str;
}

static void FreeContents(void* info, const void* data, size_t size) { Mem_Free(data); }
// TODO probably a better way..
static UIImage* ToUIImage(struct Bitmap* bmp) {
    CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
    CGDataProviderRef provider;
    CGImageRef image;

    provider = CGDataProviderCreateWithData(NULL, bmp->scan0,
                                            Bitmap_DataSize(bmp->width, bmp->height), FreeContents);
    image    = CGImageCreate(bmp->width, bmp->height, 8, 32, bmp->width * 4, colorspace,
                             kCGBitmapByteOrder32Host | kCGImageAlphaNoneSkipFirst, provider, NULL, 0, 0);
    
    UIImage* img = [UIImage imageWithCGImage:image];
    
    CGImageRelease(image);
    CGDataProviderRelease(provider);
    CGColorSpaceRelease(colorspace);
    return img;
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

void Window_Init(void) {
    //WindowInfo.SoftKeyboard = SOFT_KEYBOARD_RESIZE;
    // keyboard now shifts up
    WindowInfo.SoftKeyboard = SOFT_KEYBOARD_SHIFT;
    Input_SetTouchMode(true);
    
    DisplayInfo.Depth  = 32;
    DisplayInfo.ScaleX = 1; // TODO dpi scale
    DisplayInfo.ScaleY = 1; // TODO dpi scale
}

static UIColor* CalcBackgroundColor(void) {
    // default to purple if no themed background color yet
    if (!Launcher_Theme.BackgroundColor)
        return UIColor.purpleColor;
    return ToUIColor(Launcher_Theme.BackgroundColor, 1.0f);
}

static CGRect DoCreateWindow(void) {
    CGRect bounds = GetViewFrame();
    cc_controller = [CCViewController alloc];
    win_handle    = [[CCWindow alloc] initWithFrame:bounds];
    
    win_handle.rootViewController = cc_controller;
    win_handle.backgroundColor = CalcBackgroundColor();
    WindowInfo.Exists = true;
    WindowInfo.Width  = bounds.size.width;
    WindowInfo.Height = bounds.size.height;
    
    NSNotificationCenter* notifications = NSNotificationCenter.defaultCenter;
    [notifications addObserver:cc_controller selector:@selector(keyboardDidShow:) name:UIKeyboardWillShowNotification object:nil];
    [notifications addObserver:cc_controller selector:@selector(keyboardDidHide:) name:UIKeyboardWillHideNotification object:nil];
    return bounds;
}
void Window_SetSize(int width, int height) { }

void Window_Show(void) {
    [win_handle makeKeyAndVisible];
}

void Window_Close(void) {
    WindowInfo.Exists = false;
    Event_RaiseVoid(&WindowEvents.Closing);
}

void Window_ProcessEvents(double delta) {
    SInt32 res;
    // manually tick event queue
    do {
        res = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, TRUE);
    } while (res == kCFRunLoopRunHandledSource);
}
void ShowDialogCore(const char* title, const char* msg) {
    Platform_LogConst(title);
    Platform_LogConst(msg);
    NSString* _title = [NSString stringWithCString:title encoding:NSASCIIStringEncoding];
    NSString* _msg   = [NSString stringWithCString:msg encoding:NSASCIIStringEncoding];
    __block int completed = false;
    
    UIAlertController* alert = [UIAlertController alertControllerWithTitle:_title message:_msg preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction* okBtn     = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction* act) { completed = true; }];
    [alert addAction:okBtn];
    [cc_controller presentViewController:alert animated:YES completion: Nil];
    
    // TODO clicking outside message box crashes launcher
    // loop until alert is closed TODO avoid sleeping
    while (!completed) {
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
    Input_SetPressed(CCKEY_ENTER);
    Input_SetReleased(CCKEY_ENTER);
    return YES;
}
@end

static void LInput_SetKeyboardType(UITextField* fld, int flags);
static void LInput_SetPlaceholder(UITextField* fld, const char* placeholder);
static UITextField* text_input;
static CCKBController* kb_controller;

void Window_OpenKeyboard(struct OpenKeyboardArgs* args) {
    if (!kb_controller) {
        kb_controller = [[CCKBController alloc] init];
        CFBridgingRetain(kb_controller); // prevent GC TODO even needed?
    }
    
    text_input = [[UITextField alloc] initWithFrame:CGRectZero];
    text_input.hidden   = YES;
    text_input.delegate = kb_controller;
    [text_input addTarget:kb_controller action:@selector(handleTextChanged:) forControlEvents:UIControlEventEditingChanged];
    
    LInput_SetKeyboardType(text_input, args->type);
    LInput_SetPlaceholder(text_input,  args->placeholder);
    
    [view_handle addSubview:text_input];
    [text_input becomeFirstResponder];
}

void Window_SetKeyboardText(const cc_string* text) {
    text_input.text = ToNSString(text);
}

void Window_CloseKeyboard(void) {
    [text_input resignFirstResponder];
}

int Window_GetWindowState(void) {
    return fullscreen ? WINDOW_STATE_FULLSCREEN : WINDOW_STATE_NORMAL;
}

static void ToggleFullscreen(cc_bool isFullscreen) {
    fullscreen = isFullscreen;
    [cc_controller setNeedsStatusBarAppearanceUpdate];
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
    // see the custom UTITypes declared in Info.plist 
    NSDictionary<NSString*, NSString*>* fileExt_map =
    @{
      @".cw"  : @"com.classicube.client.ios-cw",
      @".dat" : @"com.classicube.client.ios-dat",
      @".lvl" : @"com.classicube.client.ios-lvl",
      @".fcm" : @"com.classicube.client.ios-fcm",
      @".zip" : @"public.zip-archive"
    };
    NSMutableArray<NSString*>* types = [NSMutableArray array];
    const char* const* filters = args->filters;

    for (int i = 0; filters[i]; i++) {
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
    
    // save the item to a temp file, which is then (usually) later deleted by picker callbacks
    cc_string tmpDir = String_FromConst("Exported");
    Directory_Create(&tmpDir);
    
    save_path.length = 0;
    String_Format3(&save_path, "%s/%s%c", &tmpDir, &args->defaultName, args->filters[0]);
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
 *--------------------------------------------------------2D window--------------------------------------------------------*
 *#########################################################################################################################*/
void Window_Create2D(int width, int height) {
    CGRect bounds = DoCreateWindow();
    
    view_handle = [[UIView alloc] initWithFrame:bounds];
    view_handle.multipleTouchEnabled = true;
    cc_controller.view = view_handle;
}


/*#########################################################################################################################*
 *--------------------------------------------------------3D window--------------------------------------------------------*
 *#########################################################################################################################*/
static void GLContext_OnLayout(void);

@interface CCGLView : UIView
@end

@implementation CCGLView

+ (Class)layerClass {
    return [CAEAGLLayer class];
}

- (void)layoutSubviews {
    [super layoutSubviews];
    GLContext_OnLayout();
}
@end

void Window_Create3D(int width, int height) {
    CGRect bounds = DoCreateWindow();
    view_handle   = [[CCGLView alloc] initWithFrame:bounds];
    view_handle.multipleTouchEnabled = true;
    cc_controller.view = view_handle;
    
    CAEAGLLayer* layer = (CAEAGLLayer*)view_handle.layer;
    layer.opaque = YES;
    layer.drawableProperties =
   @{
        kEAGLDrawablePropertyRetainedBacking : [NSNumber numberWithBool:NO],
        kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8
    };
}


/*########################################################################################################################*
*--------------------------------------------------------GLContext--------------------------------------------------------*
*#########################################################################################################################*/
static EAGLContext* ctx_handle;
static GLuint framebuffer;
static GLuint color_renderbuffer, depth_renderbuffer;
static int fb_width, fb_height;

static void CreateFramebuffer(void) {
    CAEAGLLayer* layer = (CAEAGLLayer*)view_handle.layer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    
    glGenRenderbuffers(1, &color_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, color_renderbuffer);
    [ctx_handle renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer];
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_renderbuffer);
    
    glGenRenderbuffers(1, &depth_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24_OES, WindowInfo.Width, WindowInfo.Height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_renderbuffer);
    
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        Logger_Abort2(status, "Failed to create renderbuffer");
    
    fb_width  = WindowInfo.Width;
    fb_height = WindowInfo.Height;
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
    CAEAGLLayer* layer = (CAEAGLLayer*)view_handle.layer;
    
    // only resize buffers when absolutely have to
    if (fb_width == WindowInfo.Width && fb_height == WindowInfo.Height) return;
    fb_width  = WindowInfo.Width;
    fb_height = WindowInfo.Height;
    
    glBindRenderbuffer(GL_RENDERBUFFER, color_renderbuffer);
    [ctx_handle renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer];
    
    glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24_OES, WindowInfo.Width, WindowInfo.Height);
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
void GLContext_SetFpsLimit(cc_bool vsync, float minFrameMs) { }
void GLContext_GetApiInfo(cc_string* info) { }
const struct UpdaterInfo Updater_Info = { "&eCompile latest source code to update", 0 };


/*########################################################################################################################*
 *--------------------------------------------------------Updater----------------------------------------------------------*
 *#########################################################################################################################*/
const char* const Updater_OGL  = NULL;
const char* const Updater_D3D9 = NULL;
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
static char gameArgs[GAME_MAX_CMDARGS][STRING_SIZE];
static int gameNumArgs;

cc_result Process_StartOpen(const cc_string* args) {
    NSString* str = ToNSString(args);
    NSURL* url    = [[NSURL alloc] initWithString:str];
    [UIApplication.sharedApplication openURL:url];
    return 0;
}

cc_result Process_StartGame2(const cc_string* args, int numArgs) {
    for (int i = 0; i < numArgs; i++)
    {
        String_CopyToRawArray(gameArgs[i], &args[i]);
    }

    gameNumArgs = numArgs;
    return 0;
}

int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
    int count = gameNumArgs;
    for (int i = 0; i < count; i++)
    {
        args[i] = String_FromRawArray(gameArgs[i]);
    }

    // clear arguments so after game is closed, launcher is started
    gameNumArgs = 0;
    return count;
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
    // https://developer.apple.com/library/archive/documentation/FileManagement/Conceptual/FileSystemProgrammingGuide/FileSystemOverview/FileSystemOverview.html
    NSArray* array = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    if (array.count <= 0) return ERR_NOT_SUPPORTED;
    
    NSString* str    = [array objectAtIndex:0];
    const char* path = [str fileSystemRepresentation];
    
    mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    return chdir(path) == -1 ? errno : 0;
}

void Platform_ShareScreenshot(const cc_string* filename) {
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
    UIDevice* device = UIDevice.currentDevice;
    NSString* string = [[device identifierForVendor] UUIDString];
    
    // TODO avoid code duplication
    const char* src = string.UTF8String;
    String_AppendUtf8(str, src, String_Length(src));
}

void Directory_GetCachePath(cc_string* path) {
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
    NSArray<NSString*>* families = UIFont.familyNames;
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
    NSArray<NSString*>* fontNames = [UIFont fontNamesForFamilyName:name];
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
          NSFontAttributeName : font,
          NSForegroundColorAttributeName : ToUIColor(color, 1.0f)
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
    NSArray<NSString*>* fontNames = [UIFont fontNamesForFamilyName:name];
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
        
        [str addAttribute:NSFontAttributeName            value:font                   range:range];
        [str addAttribute:NSForegroundColorAttributeName value:ToUIColor(color, 1.0f) range:range];
        
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
          NSFontAttributeName : font,
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


/*########################################################################################################################*
 *------------------------------------------------------UI Backend--------------------------------------------------------*
 *#########################################################################################################################*/
static struct LWidget* FindWidgetForView(id obj) {
    struct LScreen* s = Launcher_Active;
    for (int i = 0; i < s->numWidgets; i++)
    {
        void* meta = s->widgets[i]->meta;
        if (meta != (__bridge void*)obj) continue;
        
        return s->widgets[i];
    }
    return NULL;
}

static void LTable_UpdateCellColor(UIView* view, struct ServerInfo* server, int row, cc_bool selected);
static void LTable_UpdateCell(UITableView* table, UITableViewCell* cell, int row);

static NSString* cellID = @"CC_Cell";
@interface CCUIController : NSObject<UITableViewDataSource, UITableViewDelegate, UITextFieldDelegate>
@end

@implementation CCUIController

- (void)handleButtonPress:(id)sender {
    struct LWidget* w = FindWidgetForView(sender);
    if (w == NULL) return;
        
    struct LButton* btn = (struct LButton*)w;
    btn->OnClick(btn);
}

- (void)handleTextChanged:(id)sender {
    struct LWidget* w = FindWidgetForView(sender);
    if (w == NULL) return;
    
    UITextField* src   = (UITextField*)sender;
    const char* str    = src.text.UTF8String;
    struct LInput* ipt = (struct LInput*)w;
    
    ipt->text.length = 0;
    String_AppendUtf8(&ipt->text, str, String_Length(str));
    if (ipt->TextChanged) ipt->TextChanged(ipt);
}

- (void)handleValueChanged:(id)sender {
    UISwitch* swt     = (UISwitch*)sender;
    UIView* parent    = swt.superview;
    struct LWidget* w = FindWidgetForView(parent);
    if (w == NULL) return;

    struct LCheckbox* cb = (struct LCheckbox*)w;
    cb->value = [swt isOn];
    if (!cb->ValueChanged) return;
    cb->ValueChanged(cb);
}

// === UITableViewDataSource ===
- (nonnull UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    //UITableViewCell* cell = [tableView dequeueReusableCellWithIdentifier:cellID forIndexPath:indexPath];
    UITableViewCell* cell = [tableView dequeueReusableCellWithIdentifier:cellID];
    if (cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:cellID];
    }
    
    LTable_UpdateCell(tableView, cell, (int)indexPath.row);
    return cell;
}

- (NSInteger)tableView:(nonnull UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    struct LTable* w = (struct LTable*)FindWidgetForView(tableView);
    return w ? w->rowsCount : 0;
}

// === UITableViewDelegate ===
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    int row = (int)indexPath.row;
    struct ServerInfo* server = LTable_Get(row);
    LTable_UpdateCellColor([tableView cellForRowAtIndexPath:indexPath], server, row, true);
    
    struct LTable* w = (struct LTable*)FindWidgetForView(tableView);
    if (w == NULL) return;
    LTable_RowClick(w, row);
}

- (void)tableView:(UITableView *)tableView didDeselectRowAtIndexPath:(NSIndexPath *)indexPath {
    int row = (int)indexPath.row;
    struct ServerInfo* server = LTable_Get(row);
    LTable_UpdateCellColor([tableView cellForRowAtIndexPath:indexPath], server, row, false);
}

// === UITextFieldDelegate ===
- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    struct LWidget* w   = FindWidgetForView(textField);
    if (w == NULL) return YES;
    struct LWidget* sel = Launcher_Active->onEnterWidget;
    
    if (sel && !w->skipsEnter) {
        sel->OnClick(sel);
    } else {
        [textField resignFirstResponder];
    }
    return YES;
}

- (void)textFieldDidBeginEditing:(UITextField *)textField {
    kb_widget = textField;
}

@end

static CCUIController* ui_controller;
void LBackend_Init(void) {
    ui_controller = [[CCUIController alloc] init];
    CFBridgingRetain(ui_controller); // prevent GC TODO even needed?
}
static CGContextRef win_ctx;

void LBackend_MarkDirty(void* widget) { }
void LBackend_Tick(void) { }
void LBackend_Free(void) { }
void LBackend_UpdateTitleFont(void) { }

static void DrawText(NSAttributedString* str, struct Context2D* ctx, int x, int y) {
    CTLineRef line = CTLineCreateWithAttributedString((CFAttributedStringRef)str);
    CGRect bounds  = CTLineGetImageBounds(line, win_ctx);
    int centreX    = (int)(ctx->width / 2.0f - bounds.size.width / 2.0f);
    
    CGContextSetTextPosition(win_ctx, centreX + x, ctx->height - y);
    CTLineDraw(line, win_ctx);
}

void LBackend_DrawTitle(struct Context2D* ctx, const char* title) {
    if (Launcher_BitmappedText()) {
        struct FontDesc font;
        Launcher_MakeTitleFont(&font);
        Launcher_DrawTitle(&font, title, ctx);
        // bitmapped fonts don't need to be freed
        return;
    }
    UIFont* font   = [UIFont systemFontOfSize:42 weight:0.2f]; //UIFontWeightSemibold
    NSString* text = [NSString stringWithCString:title encoding:NSASCIIStringEncoding];
        
    NSDictionary* attrs_bg =
    @{
      NSFontAttributeName : font,
      NSForegroundColorAttributeName : UIColor.blackColor
    };
    NSAttributedString* str_bg = [[NSAttributedString alloc] initWithString:text attributes:attrs_bg];
    DrawText(str_bg, ctx, 4, 42);
        
    NSDictionary* attrs_fg =
    @{
      NSFontAttributeName : font,
      NSForegroundColorAttributeName : UIColor.whiteColor
    };
    NSAttributedString* str_fg = [[NSAttributedString alloc] initWithString:text attributes:attrs_fg];
    DrawText(str_fg, ctx, 0, 38);
}

void LBackend_InitFramebuffer(void) { }
void LBackend_FreeFramebuffer(void) { }

void LBackend_Redraw(void) {
    struct Context2D ctx;
    struct Bitmap bmp;
    bmp.width  = max(WindowInfo.Width,  1);
    bmp.height = max(WindowInfo.Height, 1);
    bmp.scan0  = (BitmapCol*)Mem_Alloc(bmp.width * bmp.height, 4, "window pixels");
    
    Context2D_Wrap(&ctx, &bmp);
    win_ctx = CGBitmapContextCreate(bmp.scan0, bmp.width, bmp.height, 8, bmp.width * 4,
                                    CGColorSpaceCreateDeviceRGB(), kCGBitmapByteOrder32Host | kCGImageAlphaNoneSkipFirst);
    Launcher_Active->DrawBackground(Launcher_Active, &ctx);
    
    view_handle.layer.contents = CFBridgingRelease(CGBitmapContextCreateImage(win_ctx));
    Mem_Free(bmp.scan0); // TODO Context2D_Free
    CGContextRelease(win_ctx);
}

static void LBackend_ButtonUpdateBackground(struct LButton* w);
void LBackend_ThemeChanged(void) {
    struct LScreen* s = Launcher_Active;
    LBackend_Redraw();
    
    for (int i = 0; i < s->numWidgets; i++)
    {
        struct LWidget* w = s->widgets[i];
        if (w->type != LWIDGET_BUTTON) continue;
        LBackend_ButtonUpdateBackground((struct LButton*)w);
    }
}

/*########################################################################################################################*
 *------------------------------------------------------ButtonWidget-------------------------------------------------------*
 *#########################################################################################################################*/
static void LBackend_ButtonUpdateBackground(struct LButton* w) {
    UIButton* btn = (__bridge UIButton*)w->meta;
    CGRect rect   = [btn frame];
    int width     = (int)rect.size.width;
    int height    = (int)rect.size.height;
    // memory freeing deferred until UIImage is freed (see FreeContents)
    struct Bitmap bmp1, bmp2;
    struct Context2D ctx1, ctx2;
    
    Bitmap_Allocate(&bmp1, width, height);
    Context2D_Wrap(&ctx1, &bmp1);
    LButton_DrawBackground(&ctx1, 0, 0, width, height, false);
    [btn setBackgroundImage:ToUIImage(&bmp1) forState:UIControlStateNormal];
    
    Bitmap_Allocate(&bmp2, width, height);
    Context2D_Wrap(&ctx2, &bmp2);
    LButton_DrawBackground(&ctx2, 0, 0, width, height, true);
    [btn setBackgroundImage:ToUIImage(&bmp2) forState:UIControlStateHighlighted];
}

void LBackend_ButtonInit(struct LButton* w, int width, int height) {
    w->_textWidth  = width;
    w->_textHeight = height;
}

static UIView* LBackend_ButtonShow(struct LButton* w) {
    UIButton* btn = [[UIButton alloc] init];
    btn.frame = CGRectMake(0, 0, w->_textWidth, w->_textHeight);
    [btn addTarget:ui_controller action:@selector(handleButtonPress:) forControlEvents:UIControlEventTouchUpInside];
    
    w->meta = (__bridge void*)btn;
    LBackend_ButtonUpdateBackground(w);
    LBackend_ButtonUpdate(w);
    return btn;
}

void LBackend_ButtonUpdate(struct LButton* w) {
    UIButton* btn = (__bridge UIButton*)w->meta;
    
    NSString* str = ToNSString(&w->text);
    [btn setTitle:str forState:UIControlStateNormal];
}
void LBackend_ButtonDraw(struct LButton* w) { }


/*########################################################################################################################*
 *-----------------------------------------------------CheckboxWidget------------------------------------------------------*
 *#########################################################################################################################*/
void LBackend_CheckboxInit(struct LCheckbox* w) { }

static UIView* LBackend_CheckboxShow(struct LCheckbox* w) {
    UIView* root  = [[UIView alloc] init];
    UISwitch* swt = [[UISwitch alloc] init];
    [swt addTarget:ui_controller action:@selector(handleValueChanged:) forControlEvents:UIControlEventValueChanged];
    
    UILabel* lbl  = [[UILabel alloc] init];
    lbl.textColor = UIColor.whiteColor;
    lbl.text      = ToNSString(&w->text);
    lbl.translatesAutoresizingMaskIntoConstraints = false;
    [lbl sizeToFit]; // adjust label to fit text
                     
    [root addSubview:swt];
    [root addSubview:lbl];
    
    // label should be slightly to right of switch
    [root addConstraint:[NSLayoutConstraint constraintWithItem:lbl
                                            attribute: NSLayoutAttributeLeft
                                            relatedBy: NSLayoutRelationEqual
                                            toItem:swt
                                            attribute:NSLayoutAttributeRight
                                            multiplier:1.0f
                                            constant:10.0f]];
    // label should be vertically aligned
    [root addConstraint:[NSLayoutConstraint constraintWithItem:lbl
                                            attribute: NSLayoutAttributeCenterY
                                            relatedBy: NSLayoutRelationEqual
                                            toItem:root
                                            attribute:NSLayoutAttributeCenterY
                                            multiplier:1.0f
                                            constant:0.0f]];
    
    // adjust root view height to enclose children
    CGRect frame = root.frame;
    frame.size.width  = lbl.frame.origin.x + lbl.frame.size.width;
    frame.size.height = swt.frame.size.height;
    root.frame   = frame;
    
    //root.userInteractionEnabled = YES;
    w->meta = (__bridge void*)root;
    LBackend_CheckboxUpdate(w);
    return root;
}

void LBackend_CheckboxUpdate(struct LCheckbox* w) {
    UIView* root  = (__bridge UIView*)w->meta;
    UISwitch* swt = (UISwitch*)root.subviews[0];
    
    swt.on = w->value;
}
void LBackend_CheckboxDraw(struct LCheckbox* w) { }


/*########################################################################################################################*
 *------------------------------------------------------InputWidget--------------------------------------------------------*
 *#########################################################################################################################*/
static void LInput_SetKeyboardType(UITextField* fld, int flags) {
    int type = flags & 0xFF;
    if (type == KEYBOARD_TYPE_INTEGER) {
        fld.keyboardType = UIKeyboardTypeNumberPad;
    } else if (type == KEYBOARD_TYPE_PASSWORD) {
        fld.secureTextEntry = YES;
    }
    
    if (flags & KEYBOARD_FLAG_SEND) {
        fld.returnKeyType = UIReturnKeySend;
    } else {
        fld.returnKeyType = UIReturnKeyDone;
    }
}

static void LInput_SetPlaceholder(UITextField* fld, const char* placeholder) {
    if (!placeholder) return;
    
    cc_string hint  = String_FromReadonly(placeholder);
    fld.placeholder = ToNSString(&hint);
}

void LBackend_InputInit(struct LInput* w, int width) {
    w->_textHeight = width;
}

static UIView* LBackend_InputShow(struct LInput* w) {
    UITextField* fld = [[UITextField alloc] init];
    fld.frame           = CGRectMake(0, 0, w->_textHeight, LINPUT_HEIGHT);
    fld.borderStyle     = UITextBorderStyleBezel;
    fld.backgroundColor = UIColor.whiteColor;
    fld.textColor       = UIColor.blackColor;
    fld.delegate        = ui_controller;
    [fld addTarget:ui_controller action:@selector(handleTextChanged:) forControlEvents:UIControlEventEditingChanged];
    
    LInput_SetKeyboardType(fld, w->inputType);
    LInput_SetPlaceholder(fld,  w->hintText);
    
    w->meta = (__bridge void*)fld;
    LBackend_InputUpdate(w);
    return fld;
}

void LBackend_InputUpdate(struct LInput* w) {
    UITextField* fld = (__bridge UITextField*)w->meta;
    fld.text         = ToNSString(&w->text);
}

void LBackend_InputDraw(struct LInput* w) { }
void LBackend_InputTick(struct LInput* w) { }
void LBackend_InputSelect(struct LInput* w, int idx, cc_bool wasSelected) { }
void LBackend_InputUnselect(struct LInput* w) { }


/*########################################################################################################################*
 *------------------------------------------------------LabelWidget--------------------------------------------------------*
 *#########################################################################################################################*/
void LBackend_LabelInit(struct LLabel* w) { }

static UIView* LBackend_LabelShow(struct LLabel* w) {
    UILabel* lbl  = [[UILabel alloc] init];
    w->meta       = (__bridge void*)lbl;
    
    if (w->small) lbl.font = [UIFont systemFontOfSize:14.0f];
    LBackend_LabelUpdate(w);
    return lbl;
}

void LBackend_LabelUpdate(struct LLabel* w) {
    UILabel* lbl = (__bridge UILabel*)w->meta;
    if (!lbl) return;
    
    lbl.attributedText = ToAttributedString(&w->text);
    [lbl sizeToFit]; // adjust label to fit text
}
void LBackend_LabelDraw(struct LLabel* w) { }


/*########################################################################################################################*
 *-------------------------------------------------------LineWidget--------------------------------------------------------*
 *#########################################################################################################################*/
void LBackend_LineInit(struct LLine* w, int width) {
    w->_width = width;
}

static UIView* LBackend_LineShow(struct LLine* w) {
    UIView* view = [[UIView alloc] init];
    view.frame   = CGRectMake(0, 0, w->_width, LLINE_HEIGHT);
    w->meta      = (__bridge void*)view;
    
    BitmapCol color      = LLine_GetColor();
    view.backgroundColor = ToUIColor(color, 0.5f);
    return view;
}
void LBackend_LineDraw(struct LLine* w) { }


/*########################################################################################################################*
 *------------------------------------------------------SliderWidget-------------------------------------------------------*
 *#########################################################################################################################*/
void LBackend_SliderInit(struct LSlider* w, int width, int height) {
    w->_width  = width;
    w->_height = height;
}

static UIView* LBackend_SliderShow(struct LSlider* w) {
    UIProgressView* prg = [[UIProgressView alloc] init];
    prg.frame           = CGRectMake(0, 0, w->_width, w->_height);
    prg.progressTintColor = ToUIColor(w->color, 1.0f);
    
    w->meta = (__bridge void*)prg;
    return prg;
}

void LBackend_SliderUpdate(struct LSlider* w) {
    UIProgressView* prg = (__bridge UIProgressView*)w->meta;
    
    prg.progress = w->value / 100.0f;
}
void LBackend_SliderDraw(struct LSlider* w) { }


/*########################################################################################################################*
 *------------------------------------------------------TableWidget-------------------------------------------------------*
 *#########################################################################################################################*/
void LBackend_TableInit(struct LTable* w) { }

static UIView* LBackend_TableShow(struct LTable* w) {
    UITableView* tbl = [[UITableView alloc] init];
    tbl.delegate     = ui_controller;
    tbl.dataSource   = ui_controller;
    LTable_UpdateCellColor(tbl, NULL, 1, false);
    
    //[tbl registerClass:UITableViewCell.class forCellReuseIdentifier:cellID];
    w->meta = (__bridge void*)tbl;
    return tbl;
}

void LBackend_TableUpdate(struct LTable* w) {
    UITableView* tbl = (__bridge UITableView*)w->meta;
    [tbl reloadData];
}

// TODO only redraw flags
void LBackend_TableFlagAdded(struct LTable* w) {
    UITableView* tbl = (__bridge UITableView*)w->meta;
    
    // trying to update cell.imageView.image doesn't seem to work,
    // so pointlessly reload entire table data instead
    NSIndexPath* selected = [tbl indexPathForSelectedRow];
    [tbl reloadData];
    [tbl selectRowAtIndexPath:selected animated:NO scrollPosition:UITableViewScrollPositionNone];
}

void LBackend_TableDraw(struct LTable* w) { }
void LBackend_TableReposition(struct LTable* w) { }
void LBackend_TableMouseDown(struct LTable* w, int idx) { }
void LBackend_TableMouseUp(struct   LTable* w, int idx) { }
void LBackend_TableMouseMove(struct LTable* w, int idx) { }

static void LTable_UpdateCellColor(UIView* view, struct ServerInfo* server, int row, cc_bool selected) {
    BitmapCol color = LTable_RowColor(server, row, selected);
    if (color) {
        view.backgroundColor = ToUIColor(color, 1.0f);
        view.opaque          = YES;
    } else {
        view.backgroundColor = UIColor.clearColor;
        view.opaque          = NO;
    }
}

static void LTable_UpdateCell(UITableView* table, UITableViewCell* cell, int row) {
    struct ServerInfo* server = LTable_Get(row);
    struct Flag* flag = Flags_Get(server);
    
    char descBuffer[128];
    cc_string desc = String_FromArray(descBuffer);
    String_Format2(&desc, "%i/%i players, up for ", &server->players, &server->maxPlayers);
    LTable_FormatUptime(&desc, server->uptime);
    if (server->software.length) String_Format1(&desc, " | %s", &server->software);
    
    if (flag && !flag->meta && flag->bmp.scan0) {
        UIImage* img = ToUIImage(&flag->bmp);
        flag->meta   = CFBridgingRetain(img);
    }
    if (flag && flag->meta)
        cell.imageView.image = (__bridge UIImage*)flag->meta;
        
    cell.textLabel.text       = ToNSString(&server->name);
    cell.detailTextLabel.text = ToNSString(&desc);//[ToNSString(&desc) stringByAppendingString:@"\nLine2"];
    cell.textLabel.textColor  = UIColor.whiteColor;
    cell.detailTextLabel.textColor = UIColor.whiteColor;
    cell.selectionStyle       = UITableViewCellSelectionStyleNone;
    
    NSIndexPath* sel = table.indexPathForSelectedRow;
    cc_bool selected = sel && sel.row == row;
    LTable_UpdateCellColor(cell, server, row, selected);
}

/*########################################################################################################################*
 *------------------------------------------------------UI Backend--------------------------------------------------------*
 *#########################################################################################################################*/

static void LBackend_LayoutDimensions(struct LWidget* w, CGRect* r) {
    const struct LLayout* l = w->layouts + 2;
    while (l->type)
    {
        switch (l->type)
        {
            case LLAYOUT_WIDTH:
                r->size.width  = WindowInfo.Width  - (int)r->origin.x - Display_ScaleX(l->offset);
                break;
            case LLAYOUT_HEIGHT:
                r->size.height = WindowInfo.Height - (int)r->origin.y - Display_ScaleY(l->offset);
                break;
        }
        l++;
    }
}

void LBackend_LayoutWidget(struct LWidget* w) {
    const struct LLayout* l = w->layouts;
    UIView* view = (__bridge UIView*)w->meta;
    CGRect r     = [view frame];
    int width    = (int)r.size.width;
    int height   = (int)r.size.height;
    
    r.origin.x = Gui_CalcPos(l[0].type & 0xFF, Display_ScaleX(l[0].offset), width,  WindowInfo.Width);
    r.origin.y = Gui_CalcPos(l[1].type & 0xFF, Display_ScaleY(l[1].offset), height, WindowInfo.Height);
    
    // e.g. Table widget needs adjusts width/height based on window
    if (l[1].type & LLAYOUT_EXTRA)
        LBackend_LayoutDimensions(w, &r);
    view.frame = r;
}

static UIView* ShowWidget(struct LWidget* w) {
    switch (w->type)
    {
        case LWIDGET_BUTTON:
            return LBackend_ButtonShow((struct LButton*)w);
        case LWIDGET_CHECKBOX:
            return LBackend_CheckboxShow((struct LCheckbox*)w);
        case LWIDGET_INPUT:
            return LBackend_InputShow((struct LInput*)w);
        case LWIDGET_LABEL:
            return LBackend_LabelShow((struct LLabel*)w);
        case LWIDGET_LINE:
            return LBackend_LineShow((struct LLine*)w);
        case LWIDGET_SLIDER:
            return LBackend_SliderShow((struct LSlider*)w);
        case LWIDGET_TABLE:
            return LBackend_TableShow((struct LTable*)w);
    }
    return NULL;
}

void LBackend_SetScreen(struct LScreen* s) {
    for (int i = 0; i < s->numWidgets; i++)
    {
        struct LWidget* w = s->widgets[i];
        UIView* view      = ShowWidget(w);
        
        [view_handle addSubview:view];
    }
    // TODO replace with native constraints some day, maybe
    s->Layout(s);
}

void LBackend_CloseScreen(struct LScreen* s) {
    if (!s) return;
    
    // remove reference to soon to be garbage collected views
    for (int i = 0; i < s->numWidgets; i++)
    {
        s->widgets[i]->meta = NULL;
    }
    
    // remove all widgets from previous screen
    NSArray<UIView*>* elems = [view_handle subviews];
    for (UIView* view in elems)
    {
        [view removeFromSuperview];
    }
}
