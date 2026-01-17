// Silence deprecation warnings on modern iOS
#define GLES_SILENCE_DEPRECATION
#include "../_WindowBase.h"
#include "../Bitmap.h"
#include "../Input.h"
#include "../Platform.h"
#include "../String_.h"
#include "../Errors.h"
#include "../Drawer2D.h"
#include "../Launcher.h"
#include "../Funcs.h"
#include "../Gui.h"
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <UIKit/UIKit.h>
#include <UIKit/UIPasteboard.h>
#include <CoreText/CoreText.h>

#ifdef TARGET_OS_TV
    // NSFontAttributeName etc - iOS 6
    #define TEXT_ATTRIBUTE_FONT  NSFontAttributeName
    #define TEXT_ATTRIBUTE_COLOR NSForegroundColorAttributeName
#else
    // UITextAttributeFont etc - iOS 5
    #define TEXT_ATTRIBUTE_FONT  UITextAttributeFont
    #define TEXT_ATTRIBUTE_COLOR UITextAttributeTextColor
#endif

// shared state with LBackend_ios.m and interop_ios.m
CGContextRef win_ctx;
UIView* view_handle;
UIViewController* cc_controller;

UIColor* ToUIColor(BitmapCol color, float A);
NSString* ToNSString(const cc_string* text);
void LInput_SetKeyboardType(UITextField* fld, int flags);
void LInput_SetPlaceholder(UITextField* fld, const char* placeholder);
UIInterfaceOrientationMask SupportedOrientations(void);
void LogUnhandledNSErrors(NSException* ex);


static UITextField* kb_widget;
void Window_SetKBWidget(UITextField* widget) {
	if (kb_widget) [kb_widget autorelease];
	
	if (widget) [widget retain];
	kb_widget = widget;
}

@interface CCWindow : UIWindow
@end

@interface CCViewController : UIViewController<UIDocumentPickerDelegate, UIAlertViewDelegate>
@end
static UIWindow* win_handle;

static void AddTouch(UITouch* t) {
    CGPoint loc = [t locationInView:view_handle];
    //int x = loc.x, y = loc.y; long ui_id = (long)t;
    //Platform_Log3("POINTER %x - DOWN %i,%i", &ui_id, &x, &y);
    Input_AddTouch((long)t, loc.x, loc.y);
}

static void UpdateTouch(UITouch* t) {
    CGPoint loc = [t locationInView:view_handle];
    //int x = loc.x, y = loc.y; long ui_id = (long)t;
    //Platform_Log3("POINTER %x - MOVE %i,%i", &ui_id, &x, &y);
    Input_UpdateTouch((long)t, loc.x, loc.y);
}

static void RemoveTouch(UITouch* t) {
    CGPoint loc = [t locationInView:view_handle];
    //int x = loc.x, y = loc.y; long ui_id = (long)t;
    //Platform_Log3("POINTER %x - UP %i,%i", &ui_id, &x, &y);
    Input_RemoveTouch((long)t, loc.x, loc.y);
}

static cc_bool landscape_locked;
UIInterfaceOrientationMask SupportedOrientations(void) {
    if (landscape_locked)
        return UIInterfaceOrientationMaskLandscape;
    return UIInterfaceOrientationMaskAll;
}

static cc_bool fullscreen = true;
static void UpdateStatusBar(void) {
    if ([cc_controller respondsToSelector:@selector(setNeedsStatusBarAppearanceUpdate)]) {
        // setNeedsStatusBarAppearanceUpdate - iOS 7
        [cc_controller setNeedsStatusBarAppearanceUpdate];
    } else {
        UIApplication* app = [UIApplication sharedApplication];
        [app setStatusBarHidden:fullscreen withAnimation:UIStatusBarAnimationNone];
    }
}

static CGRect GetViewFrame(void) {
    UIScreen* screen = [UIScreen mainScreen];
    return fullscreen ? [screen bounds] : [screen applicationFrame];
}

@implementation CCWindow

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent *)event {
    // touchesBegan:withEvent - iOS 2
    for (UITouch* t in touches) AddTouch(t);
    
    // clicking on the background should dismiss onscreen keyboard
    if (!Window_Main.Is3D) { [view_handle endEditing:NO]; }
}

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent *)event {
    // touchesMoved:withEvent - iOS 2
    for (UITouch* t in touches) UpdateTouch(t);
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent *)event {
    // touchesEnded:withEvent - iOS 2
    for (UITouch* t in touches) RemoveTouch(t);
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent *)event {
    // touchesCancelled:withEvent - iOS 2
    for (UITouch* t in touches) RemoveTouch(t);
}

- (BOOL)isOpaque { return YES; }
@end


@implementation CCViewController
- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    // supportedInterfaceOrientations - iOS 6
    return SupportedOrientations();
}

- (BOOL)shouldAutorotate {
    // shouldAutorotate - iOS 6
    return YES;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)ori {
    // shouldAutorotateToInterfaceOrientation - iOS 2
    if (landscape_locked && !(ori == UIInterfaceOrientationLandscapeLeft || ori == UIInterfaceOrientationLandscapeRight))
        return NO;
    
    return YES;
}

- (void)viewDidLayoutSubviews {
    // viewDidLayoutSubviews - iOS 5
	[super viewDidLayoutSubviews];

	CGRect frame = [view_handle frame];
	int width  = (int)frame.size.width;
	int height = (int)frame.size.height;
	if (width == Window_Main.Width && height == Window_Main.Height) return;
	
	Window_Main.Width  = width;
	Window_Main.Height = height;
	Event_RaiseVoid(&WindowEvents.Resized);
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
    // documentPicker:didPickDocumentAtURL - iOS 8
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
    // documentPickerWasCancelled - iOS 8
    DeleteExportTempFile();
}

static cc_bool kb_active;
- (void)keyboardDidShow:(NSNotification*)notification {
    NSDictionary* info = [notification userInfo];
    if (kb_active) return;
    // TODO this is wrong
    // TODO this doesn't actually trigger view resize???
    kb_active = true;
    
    double interval   = [[info objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
    NSInteger curve   = [[info objectForKey:UIKeyboardAnimationCurveUserInfoKey] integerValue];
    CGRect kbFrame    = [[info objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
    CGRect winFrame   = [view_handle frame];
    
    cc_bool can_shift = true;
    // would the active input widget be pushed offscreen?
    if (kb_widget) {
        CGRect curFrame = [kb_widget frame];
        can_shift = curFrame.origin.y > kbFrame.size.height;
    }
    if (can_shift) winFrame.origin.y = -kbFrame.size.height;
    Window_SetKBWidget(nil);
    
    Platform_LogConst("APPEAR");
    [UIView animateWithDuration:interval delay: 0.0 options:curve animations:^{
        [view_handle setFrame:winFrame];
    } completion:nil];
}

- (void)keyboardDidHide:(NSNotification*)notification {
    NSDictionary* info = [notification userInfo];
    if (!kb_active) return;
    kb_active = false;
	Window_SetKBWidget(nil);
    
    double interval   = [[info objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
    NSInteger curve   = [[info objectForKey:UIKeyboardAnimationCurveUserInfoKey] integerValue];
    CGRect winFrame   = [view_handle frame];
    winFrame.origin.y = 0;
    
    Platform_LogConst("VANISH");
    [UIView animateWithDuration:interval delay: 0.0 options:curve animations:^{
       [view_handle setFrame:winFrame];
    } completion:nil];
}

- (BOOL)prefersStatusBarHidden {
    // prefersStatusBarHidden - iOS 7
    return fullscreen;
}

- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures {
    // preferredScreenEdgesDeferringSystemGestures - iOS 11
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

// iOS textfields manage ctrl+c/v
void Clipboard_GetText(cc_string* value) { }
void Clipboard_SetText(const cc_string* value) { }


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

void Window_PreInit(void) { 
    DisplayInfo.CursorVisible = true;
}

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
void Window_SetSize(int width, int height) { }

void Window_Show(void) {
    [win_handle makeKeyAndVisible];
}

void Window_RequestClose(void) {
    Event_RaiseVoid(&WindowEvents.Closing);
}

void Window_ProcessEvents(float delta) {
    SInt32 res;
    // manually tick event queue
    do {
        res = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, TRUE);
    } while (res == kCFRunLoopRunHandledSource);
}

void Gamepads_PreInit(void) { }

void Gamepads_Init(void) { }

void Gamepads_Process(float delta) { }


@interface CCKBController : NSObject<UITextFieldDelegate>
@end

@implementation CCKBController
- (void)handleTextChanged:(id)sender {
    UITextField* src = (UITextField*)sender;
    NSString* text   = [src text];
    const char* str  = [text UTF8String];
    
    char tmpBuffer[NATIVE_STR_LEN];
    cc_string tmp = String_FromArray(tmpBuffer);
    String_AppendUtf8(&tmp, str, String_Length(str));
    
    Event_RaiseString(&InputEvents.TextChanged, &tmp);
}

// === UITextFieldDelegate ===
- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    // textFieldShouldReturn - iOS 2
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
		// NOTE: don't need to call [retain], as retain count is initially 1
    }
    DisplayInfo.ShowingSoftKeyboard = true;
    
    text_input = [[UITextField alloc] initWithFrame:CGRectZero];
    [text_input setHidden:YES];
    [text_input setDelegate:kb_controller];
    [text_input addTarget:kb_controller action:@selector(handleTextChanged:) forControlEvents:UIControlEventEditingChanged];
    
    LInput_SetKeyboardType(text_input, args->type);
    LInput_SetPlaceholder(text_input,  args->placeholder);
    
    [view_handle addSubview:text_input];
    [text_input becomeFirstResponder];
	
	[text_input autorelease];
}

void OnscreenKeyboard_SetText(const cc_string* text) {
    NSString* str = ToNSString(text);
    NSString* cur = [text_input text];
    
    // otherwise on iOS 5, this causes an infinite loop
    if (cur && [str isEqualToString:cur]) return;
    [text_input setText:str];
}

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
    
    CGRect frame = GetViewFrame();
    [view_handle setFrame:frame];
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
    // attemptRotationToDeviceOrientation - iOS 5
    // TODO doesn't work properly.. setting 'UIInterfaceOrientationUnknown' apparently
    //  restores orientation, but doesn't actually do that when I tried it
    if (lock) {
        //NSInteger ori    = lock ? UIInterfaceOrientationLandscapeRight : UIInterfaceOrientationUnknown;
        NSInteger ori    = UIInterfaceOrientationLandscapeRight;
        UIDevice* device = [UIDevice currentDevice];
        NSNumber* value  = [NSNumber numberWithInteger:ori];
        [device setValue:value forKey:@"orientation"];
    }
    
    landscape_locked = lock;
    [UIViewController attemptRotationToDeviceOrientation];
}


/*#########################################################################################################################*
 *-----------------------------------------------------Window creation-----------------------------------------------------*
 *#########################################################################################################################*/
@interface CC3DView : UIView
@end
static void Init3DLayer(void);

static UIColor* CalcBackgroundColor(void) {
	// default to purple if no themed background color yet
	if (!Launcher_Theme.BackgroundColor)
		return [UIColor redColor];
	
	return ToUIColor(Launcher_Theme.BackgroundColor, 1.0f);
}

static void AllocWindow(void) {
	// UIKeyboardWillShowNotification - iOS 2
	if (cc_controller) return;
	cc_controller = [CCViewController alloc];
	// NOTE: don't need to call [retain], as retain count is initially 1
	
	CGRect bounds = GetViewFrame();
	win_handle    = [[CCWindow alloc] initWithFrame:bounds];
	[win_handle setRootViewController:cc_controller];
	
	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;
	
	Window_Main.Width  = bounds.size.width;
	Window_Main.Height = bounds.size.height;
	Window_Main.SoftKeyboardInstant = true;
	
	NSNotificationCenter* notifications = [NSNotificationCenter defaultCenter];
	[notifications addObserver:cc_controller selector:@selector(keyboardDidShow:) name:UIKeyboardWillShowNotification object:nil];
	[notifications addObserver:cc_controller selector:@selector(keyboardDidHide:) name:UIKeyboardWillHideNotification object:nil];
}

static void DoCreateWindow(void) {
	AllocWindow();
	UpdateStatusBar();
	
	UIColor* color = CalcBackgroundColor();
	[win_handle setBackgroundColor:color];
}

static void SetRootView(UIView* view) {
	view_handle = view;
	[view setMultipleTouchEnabled:YES];
	[cc_controller setView:view];
	
	// Required, otherwise view doesn't rotate with device anymore after going in-game
	[win_handle setRootViewController:nil];
	[win_handle setRootViewController:cc_controller];
}

void Window_Create2D(int width, int height) {
    Window_Main.Is3D = false;
    DoCreateWindow();
	
    UIView* view = [[UIView alloc] initWithFrame:CGRectZero];
	SetRootView(view);
	
	[view autorelease];
}

void Window_Create3D(int width, int height) {
    Window_Main.Is3D = true;
    DoCreateWindow();
	
    CC3DView* view = [[CC3DView alloc] initWithFrame:CGRectZero];
	SetRootView(view);
    Init3DLayer();
	
	[view autorelease];
}

void Window_Destroy(void) { }


/*########################################################################################################################*
 *--------------------------------------------------------Dialogs---------------------------------------------------------*
 *#########################################################################################################################*/
void ShowDialogCore(const char* title, const char* msg) {
	// UIAlertController - iOS 8
	// UIAlertAction - iOS 8
	// UIAlertView - iOS 2
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
	[alert autorelease];
#endif
	
	// TODO clicking outside message box crashes launcher
	// loop until alert is closed TODO avoid sleeping
	while (!alert_completed) {
		Window_ProcessEvents(0.0);
		Thread_Sleep(16);
	}
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	// UIDocumentPickerViewController - iOS 8
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
	[dlg setDelegate:cc_controller];
	[cc_controller presentViewController:dlg animated:YES completion: Nil];
	
	[dlg autorelease];
	return 0; // TODO still unfinished
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	if (!args->defaultName.length) return SFD_ERR_NEED_DEFAULT_NAME;
	// UIDocumentPickerViewController - iOS 8
	
	// save the item to a temp file, which is then (usually) later deleted by picker callbacks
	Directory_Create2(FILEPATH_RAW("Exported"));
	
	save_path.length = 0;
	String_Format2(&save_path, "Exported/%s%c", &args->defaultName, args->filters[0]);
	args->Callback(&save_path);
	
	NSString* str = ToNSString(&save_path);
	NSURL* url    = [NSURL fileURLWithPath:str isDirectory:NO];
	
	UIDocumentPickerViewController* dlg;
	dlg = [UIDocumentPickerViewController alloc];
	dlg = [dlg initWithURL:url inMode:UIDocumentPickerModeExportToService];
	
	[dlg setDelegate:cc_controller];
	[cc_controller presentViewController:dlg animated:YES completion: Nil];
	
	[dlg autorelease];
	return 0;
}


/*########################################################################################################################*
*--------------------------------------------------------GLContext--------------------------------------------------------*
*#########################################################################################################################*/
#if CC_GFX_BACKEND_IS_GL()
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

static EAGLContext* ctx_handle;
static GLuint framebuffer;
static GLuint color_renderbuffer, depth_renderbuffer;
static int fb_width, fb_height;

static void UpdateColorbuffer(void) {
    CAEAGLLayer* layer = (CAEAGLLayer*)[view_handle layer];
    glBindRenderbuffer(GL_RENDERBUFFER, color_renderbuffer);
    
    if (![ctx_handle renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer])
        Process_Abort("Failed to link renderbuffer to window");
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
        Process_Abort2(status, "Failed to create renderbuffer");
    
    fb_width  = Window_Main.Width;
    fb_height = Window_Main.Height;
}

void GLContext_Create(void) {
    ctx_handle = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    [EAGLContext setCurrentContext:ctx_handle];
    
    // unlike other platforms, have to manually setup render framebuffer
    CreateFramebuffer();
	[ctx_handle autorelease];
}
                  
void GLContext_Update(void) {
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
@end

static void Init3DLayer(void) {
    // CAEAGLLayer - iOS 2
    CAEAGLLayer* layer = (CAEAGLLayer*)[view_handle layer];

    [layer setOpaque:YES];
    NSDictionary* props =
   @{
        kEAGLDrawablePropertyRetainedBacking : [NSNumber numberWithBool:NO],
        kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8
    };
    [layer setDrawableProperties:props];
}
#endif


/*########################################################################################################################*
*-------------------------------------------------------Framebuffer-------------------------------------------------------*
*#########################################################################################################################*/
void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
    bmp->width  = width;
    bmp->height = height;
    bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
    
    win_ctx = CGBitmapContextCreate(bmp->scan0, width, height, 8, width * 4,
                                    CGColorSpaceCreateDeviceRGB(), kCGBitmapByteOrder32Host | kCGImageAlphaNoneSkipFirst);
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
    CGImageRef image = CGBitmapContextCreateImage(win_ctx);
    CALayer* layer = [view_handle layer];
    [layer setContents:CFBridgingRelease(image)];
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
    Mem_Free(bmp->scan0);
    CGContextRelease(win_ctx);
}
