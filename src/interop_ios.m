#define GLES_SILENCE_DEPRECATION
#include "_WindowBase.h"
#include "Bitmap.h"
#include "Input.h"
#include "Platform.h"
#include "String.h"
#include "Errors.h"
#include <UIKit/UIPasteboard.h>
#include <UIKit/UIKit.h>
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

@interface CCWindow : UIWindow
@end

@interface CCViewController : UIViewController
@end

@interface CCAppDelegate : UIResponder<UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

static CCViewController* controller;
static UIWindow* win_handle;
static UIView* view_handle;

static void DoDrawFramebuffer(CGRect dirty);

static void AddTouch(UITouch* t) {
    CGPoint loc = [t locationInView:view_handle];
    int x = loc.x, y = loc.y; long ui_id = (long)t;
    Platform_Log3("POINTER %i - DOWN %i,%i", &ui_id, &x, &y);
    Input_AddTouch((long)t, loc.x, loc.y);
}

static void UpdateTouch(UITouch* t) {
    CGPoint loc = [t locationInView:view_handle];
    int x = loc.x, y = loc.y; long ui_id = (long)t;
    Platform_Log3("POINTER %i - MOVE %i,%i", &ui_id, &x, &y);
    Input_UpdateTouch((long)t, loc.x, loc.y);
}

static void RemoveTouch(UITouch* t) {
    CGPoint loc = [t locationInView:view_handle];
    int x = loc.x, y = loc.y; long ui_id = (long)t;
    Platform_Log3("POINTER %i - UP %i,%i", &ui_id, &x, &y);
    Input_RemoveTouch((long)t, loc.x, loc.y);
}

@implementation CCWindow

//- (void)drawRect:(CGRect)dirty { DoDrawFramebuffer(dirty); }

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
@end

@implementation CCAppDelegate

- (void)runMainLoop {
    extern int main_real(int argc, char** argv);
    main_real(1, NULL);
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Override point for customization after application launch.
    // schedule the actual main loop to run in next CFRunLoop iteration
    //  (as calling main_real here doesn't work properly)
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
}
@end

int main(int argc, char * argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([CCAppDelegate class]));
    }
}

void Clipboard_GetText(cc_string* value) {
	const char* raw;
	NSString* str;

	str = [UIPasteboard generalPasteboard].string;
	if (!str) return;

	raw = str.UTF8String;
	String_AppendUtf8(value, raw, String_Length(raw));
}

void Clipboard_SetText(const cc_string* value) {
	char raw[NATIVE_STR_LEN];
	NSString* str;

	Platform_EncodeUtf8(raw, value);
	str = [NSString stringWithUTF8String:raw];
	[UIPasteboard generalPasteboard].string = str;
}

/*########################################################################################################################*
*---------------------------------------------------------Window----------------------------------------------------------*
*#########################################################################################################################*/
void Cursor_GetRawPos(int* x, int* y) { *x = 0; *y = 0; }
void Cursor_SetPosition(int x, int y) { }
void Cursor_DoSetVisible(cc_bool visible) { }

void Window_SetTitle(const cc_string* title) {
	// TODO: Implement this somehow
}

void Window_Init(void) {
    WindowInfo.SoftKeyboard = SOFT_KEYBOARD_RESIZE;
    Input_SetTouchMode(true);
    
    DisplayInfo.Depth  = 32;
    DisplayInfo.ScaleX = 1; // TODO dpi scale
    DisplayInfo.ScaleY = 1; // TODO dpi scale
}

static CGRect DoCreateWindow(void) {
    CGRect bounds = UIScreen.mainScreen.bounds;
    controller = [CCViewController alloc];
    win_handle = [[CCWindow alloc] initWithFrame:bounds];
    
    win_handle.rootViewController = controller;
    win_handle.backgroundColor = UIColor.blueColor;
    WindowInfo.Exists = true;
    WindowInfo.Width  = bounds.size.width;
    WindowInfo.Height = bounds.size.height;
    return bounds;
}
void Window_Create2D(int width, int height) { DoCreateWindow(); }
void Window_SetSize(int width, int height) { }

void Window_Close(void) { }

void Window_Show(void) {
    [win_handle makeKeyAndVisible];
}

void Window_ProcessEvents(void) {
    SInt32 res;
    // manually tick event queue
    do {
        res = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, TRUE);
    } while (res == kCFRunLoopRunHandledSource);
}
void ShowDialogCore(const char* title, const char* msg) { }

void Window_OpenKeyboard(const struct OpenKeyboardArgs* args) { }
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { }

int Window_GetWindowState(void) { return WINDOW_STATE_NORMAL; }
cc_result Window_EnterFullscreen(void) { return ERR_NOT_SUPPORTED; }
cc_result Window_ExitFullscreen(void) { return ERR_NOT_SUPPORTED; }

static struct Bitmap fb_bmp;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
    bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
    fb_bmp = *bmp;
}

static void DoDrawFramebuffer(CGRect dirty) {
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = NULL;
    CGDataProviderRef provider;
    CGImageRef image;
    CGRect rect;
    
    // Unfortunately CGImageRef is immutable, so changing the
    // underlying data doesn't change what shows when drawing.
    // TODO: Find a better way of doing this in cocoa..
    if (!fb_bmp.scan0) return;
    context   = UIGraphicsGetCurrentContext();
    //CGContextTranslateCTM(context, 0, -WindowInfo.Height);
    //CGContextScaleCTM(context, 1.0, -1.0); // invert upside down
    
    // TODO: Only update changed bit..
    rect.origin.x = 0; rect.origin.y = 0;
    rect.size.width  = WindowInfo.Width;
    rect.size.height = WindowInfo.Height;
    
    // TODO: REPLACE THIS AWFUL HACK
    provider = CGDataProviderCreateWithData(NULL, fb_bmp.scan0,
                                            Bitmap_DataSize(fb_bmp.width, fb_bmp.height), NULL);
    image = CGImageCreate(fb_bmp.width, fb_bmp.height, 8, 32, fb_bmp.width * 4, colorSpace,
                          kCGBitmapByteOrder32Host | kCGImageAlphaNoneSkipFirst, provider, NULL, 0, 0);
    
    CGContextDrawImage(context, rect, image);
    CGContextSynchronize(context);
    
    CGImageRelease(image);
    CGDataProviderRelease(provider);
    CGColorSpaceRelease(colorSpace);
}

void Window_DrawFramebuffer(Rect2D r) {
    CGRect rect;
    rect.origin.x    = r.X;
    rect.origin.y    = WindowInfo.Height - r.Y - r.Height;
    rect.size.width  = r.Width;
    rect.size.height = r.Height;
    [win_handle setNeedsDisplayInRect:rect];
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
    Mem_Free(bmp->scan0);
}

void Window_EnableRawMouse(void)  { }
void Window_UpdateRawMouse(void)  { }
void Window_DisableRawMouse(void) { }


cc_bool Window_RemakeSurface(void) {
    // TODO implement
    return true;
}

void Window_LockLandscapeOrientation(cc_bool lock) {
    // TODO implement
}


@interface CCGLView : UIView
@end

@implementation CCGLView

+ (Class)layerClass {
    return [CAEAGLLayer class];
}
@end

void Window_Create3D(int width, int height) {
    CGRect bounds = DoCreateWindow();
    view_handle = [[CCGLView alloc] initWithFrame:bounds];
    view_handle.multipleTouchEnabled = true;
    controller.view = view_handle;
    
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
}

void GLContext_Create(void) {
    ctx_handle = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    [EAGLContext setCurrentContext:ctx_handle];
    
    // unlike other platforms, have to manually setup render framebuffer
    CreateFramebuffer();
}
                  
void GLContext_Update(void) { }
cc_bool GLContext_TryRestore(void) { return false; }

void GLContext_Free(void) {
    [EAGLContext setCurrentContext:Nil];
}

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
