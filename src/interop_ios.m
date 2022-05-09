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
#include <UIKit/UIPasteboard.h>
#include <UIKit/UIKit.h>
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include <CoreText/CoreText.h>

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
- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    return SupportedOrientations();
}

- (BOOL)shouldAutorotate {
    return YES;
}

/*- (BOOL)prefersStatusBarHidden {
    
}*/
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
void Window_SetSize(int width, int height) { }

void Window_Show(void) {
    [win_handle makeKeyAndVisible];
}

void Window_Close(void) {
    WindowInfo.Exists = false;
    Event_RaiseVoid(&WindowEvents.Closing);
}

void Window_ProcessEvents(void) {
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
    [controller presentViewController:alert animated:YES completion: Nil];
    
    // TODO clicking outside message box crashes launcher
    // loop until alert is closed TODO avoid sleeping
    while (!completed) {
        Window_ProcessEvents();
        Thread_Sleep(16);
    }
}

static void LInput_SetKeyboardType(UITextField* fld, int flags);
static void LInput_SetPlaceholder(UITextField* fld, const char* placeholder);
static UITextField* text_input;

void Window_OpenKeyboard(const struct OpenKeyboardArgs* args) {
    text_input = [[UITextField alloc] initWithFrame:CGRectZero];
    text_input.hidden = YES;
    
    LInput_SetKeyboardType(text_input, args->type);
    LInput_SetPlaceholder(text_input,  args->placeholder);
    
    [view_handle addSubview:text_input];
    [text_input becomeFirstResponder];
}

void Window_SetKeyboardText(const cc_string* text) {
    char raw[NATIVE_STR_LEN];
    NSString* str;
    
    Platform_EncodeUtf8(raw, text);
    str = [NSString stringWithUTF8String:raw];
    text_input.text = str;
}

void Window_CloseKeyboard(void) {
    [text_input resignFirstResponder];
}

int Window_GetWindowState(void) { return WINDOW_STATE_NORMAL; }
cc_result Window_EnterFullscreen(void) { return ERR_NOT_SUPPORTED; }
cc_result Window_ExitFullscreen(void) { return ERR_NOT_SUPPORTED; }
int Window_IsObscured(void) { return 0; }

void Window_EnableRawMouse(void)  { }
void Window_UpdateRawMouse(void)  { }
void Window_DisableRawMouse(void) { }

void Window_LockLandscapeOrientation(cc_bool lock) {
    // TODO doesn't work
    landscape_locked = lock;
    [UIViewController attemptRotationToDeviceOrientation];
}

cc_result Window_OpenFileDialog(const char* const* filters, OpenFileDialogCallback callback) {
	return ERR_NOT_SUPPORTED;
}


/*#########################################################################################################################*
 *--------------------------------------------------------2D window--------------------------------------------------------*
 *#########################################################################################################################*/
void Window_Create2D(int width, int height) {
    CGRect bounds = DoCreateWindow();
    
    view_handle = [[UIView alloc] initWithFrame:bounds];
    view_handle.multipleTouchEnabled = true;
    controller.view = view_handle;
}


/*#########################################################################################################################*
 *--------------------------------------------------------3D window--------------------------------------------------------*
 *#########################################################################################################################*/
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
    char raw[NATIVE_STR_LEN];
    NSURL* url;
    NSString* str;
    
    Platform_EncodeUtf8(raw, args);
    str = [NSString stringWithUTF8String:raw];
    url = [[NSURL alloc] initWithString:str];
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
    // TODO this is the API should actually be using.. eventually
    /*NSArray* array = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
    if ([array count] <= 0) return ERR_NOT_SUPPORTED;
    
    NSString* str = [array objectAtIndex:0];
    const char* name = [str fileSystemRepresentation];
    return chdir(name) == -1 ? errno : 0;*/
    
    char path[NATIVE_STR_LEN + 1] = { 0 };
    uint32_t size = NATIVE_STR_LEN;
    if (_NSGetExecutablePath(path, &size)) return ERR_INVALID_ARGUMENT;
    
    // despite what you'd assume, size is NOT changed to length of path
    int len = String_CalcLen(path, NATIVE_STR_LEN);
    
    // get rid of filename at end of directory
    for (int i = len - 1; i >= 0; i--, len--)
    {
        if (path[i] == '/') break;
    }

    path[len] = '\0';
    return chdir(path) == -1 ? errno : 0;
}

void Platform_ShareScreenshot(const cc_string* filename) {
    cc_string path; char pathBuffer[FILENAME_SIZE];
    String_InitArray(path, pathBuffer);
    char tmp[NATIVE_STR_LEN];
    
    String_Format1(&path, "screenshots/%s", filename);
    Platform_EncodeUtf8(tmp, &path);
    
    // TODO unify with ToNSString
    NSString* pathStr = [NSString stringWithUTF8String:tmp];
    UIImage* img = [UIImage imageWithContentsOfFile:pathStr];
    
    // https://stackoverflow.com/questions/31955140/sharing-image-using-uiactivityviewcontroller
    UIActivityViewController* act;
    act = [UIActivityViewController alloc];
    act = [act initWithActivityItems:@[ @"Share screenshot via", img] applicationActivities:Nil];
    [controller presentViewController:act animated:true completion:Nil];
}

void GetDeviceUUID(cc_string* str) {
    UIDevice* device = [UIDevice currentDevice];
    NSString* string = [[device identifierForVendor] UUIDString];
    
    // TODO avoid code duplication
    const char* src = [string UTF8String];
    String_AppendUtf8(str, src, String_Length(src));
}


/*########################################################################################################################*
 *------------------------------------------------------UI Backend--------------------------------------------------------*
 *#########################################################################################################################*/
static UIColor* ToUIColor(BitmapCol color, float A) {
    float R = BitmapCol_R(color) / 255.0f;
    float G = BitmapCol_G(color) / 255.0f;
    float B = BitmapCol_B(color) / 255.0f;
    return [UIColor colorWithRed:R green:G blue:B alpha:A];
}

static NSString* ToNSString(const cc_string* text) {
    char raw[NATIVE_STR_LEN];
    Platform_EncodeUtf8(raw, text);
    return [NSString stringWithUTF8String:raw];
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
    [view setFrame:r];
}

void LBackend_SetScreen(struct LScreen* s) {
    for (int i = 0; i < s->numWidgets; i++)
    {
        void* obj = s->widgets[i]->meta;
        if (!obj) continue;
        
        UIView* view = (__bridge UIView*)obj;
        [view_handle addSubview:view];
        
        /*[view addConstraint:[NSLayoutConstraint constraintWithItem:view
                                                        attribute: NSLayoutAttributeLeft
                                                        relatedBy:NSLayoutRelationEqual
                                                        toItem:view_handle
                                                        attribute:NSLayoutAttributeLeft
                                                        multiplier:1.0f
                                                        constant:s->widgets[i]->layouts[0].offset]];*/
    }
}

void LBackend_CloseScreen(struct LScreen* s) {
    if (!s) return;
    
    // remove all widgets from previous screen
    NSArray<UIView*>* elems = [view_handle subviews];
    for (UIView* view in elems)
    {
        [view removeFromSuperview];
    }
}

static void AssignView(void* widget, UIView* view) {
    struct LWidget* w = widget;
    // doesn't work, the view get auto garbage collected
    //  after LBackend_CloseScreen removes the subviews
    //w->meta = (__bridge void*)view;
    w->meta = CFBridgingRetain(view);
}

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

static NSString* cellID = @"CC_Cell";
@interface CCUIController : NSObject<UITableViewDataSource, UITableViewDelegate>
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
    const char* str    = [[src text] UTF8String];
    struct LInput* ipt = (struct LInput*)w;
    
    ipt->text.length = 0;
    String_AppendUtf8(&ipt->text, str, String_Length(str));
    if (ipt->TextChanged) ipt->TextChanged(ipt);
}

- (void)handleValueChanged:(id)sender {
    struct LWidget* w = FindWidgetForView(sender);
    if (w == NULL) return;
    
    UISwitch* src        = (UISwitch*)sender;
    struct LCheckbox* cb = (struct LCheckbox*)w;
    
    cb->value = [src isOn];
    if (!cb->ValueChanged) return;
    cb->ValueChanged(cb);
}

// === UITableViewDataSource ===
- (nonnull UITableViewCell *)tableView:(nonnull UITableView *)tableView cellForRowAtIndexPath:(nonnull NSIndexPath *)indexPath {
    //UITableViewCell* cell = [tableView dequeueReusableCellWithIdentifier:cellID forIndexPath:indexPath];
    UITableViewCell* cell = [tableView dequeueReusableCellWithIdentifier:cellID];
    if (cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:cellID];
    }
    
    struct ServerInfo* server = LTable_Get([indexPath row]);
    struct Flag* flag = Flags_Get(server);
    
    char descBuffer[128];
    cc_string desc = String_FromArray(descBuffer);
    String_Format2(&desc, "%i/%i players, up for ", &server->players, &server->maxPlayers);
    LTable_FormatUptime(&desc, server->uptime);
    
    if (flag && !flag->meta && flag->bmp.scan0) {
        UIImage* img = ToUIImage(&flag->bmp);
        flag->meta   = CFBridgingRetain(img);
    }
    if (flag && flag->meta)
        cell.imageView.image = (__bridge UIImage*)flag->meta;
    
    cell.textLabel.text       = ToNSString(&server->name);
    cell.detailTextLabel.text = ToNSString(&desc);//[ToNSString(&desc) stringByAppendingString:@"\nLine2"];
    return cell;
}

- (NSInteger)tableView:(nonnull UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return FetchServersTask.numServers;
}

// === UITableViewDelegate ===
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    struct LTable* w = FindWidgetForView(tableView);
    if (w == NULL) return;
    LTable_RowClick(w, [indexPath row]);
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
void LBackend_UpdateLogoFont(void) { }

static void DrawText(NSAttributedString* str, struct Bitmap* bmp, int x, int y) {
    CTLineRef line = CTLineCreateWithAttributedString((CFAttributedStringRef)str);
    CGRect bounds  = CTLineGetImageBounds(line, win_ctx);
    int centreX    = (int)(bmp->width / 2.0f - bounds.size.width / 2.0f);
    
    CGContextSetTextPosition(win_ctx, centreX + x, bmp->height - y);
    CTLineDraw(line, win_ctx);
}

void LBackend_DrawLogo(struct Bitmap* bmp, const char* title) {
    if (Launcher_BitmappedText()) {
        struct FontDesc font;
        Launcher_MakeLogoFont(&font);
        Launcher_DrawLogo(&font, title, bmp);
        // bitmapped fonts don't need to be freed
        return;
    }
    UIFont* font   = [UIFont systemFontOfSize:42 weight:0.2f]; //UIFontWeightSemibold
    NSString* text = [NSString stringWithCString:title encoding:NSASCIIStringEncoding];
        
    NSDictionary* attrs_bg =
    @{
      NSFontAttributeName : font,
      NSForegroundColorAttributeName : [UIColor blackColor]
    };
    NSAttributedString* str_bg = [[NSAttributedString alloc] initWithString:text attributes:attrs_bg];
    DrawText(str_bg, bmp, 4, 42);
        
    NSDictionary* attrs_fg =
    @{
      NSFontAttributeName : font,
      NSForegroundColorAttributeName : [UIColor whiteColor]
    };
    NSAttributedString* str_fg = [[NSAttributedString alloc] initWithString:text attributes:attrs_fg];
    DrawText(str_fg, bmp, 0, 38);
}

void LBackend_InitFramebuffer(void) { }
void LBackend_FreeFramebuffer(void) { }

void LBackend_Redraw(void) {
    struct Bitmap bmp;
    bmp.width  = max(WindowInfo.Width,  1);
    bmp.height = max(WindowInfo.Height, 1);
    bmp.scan0  = (BitmapCol*)Mem_Alloc(bmp.width * bmp.height, 4, "window pixels");
    
    win_ctx = CGBitmapContextCreate(bmp.scan0, bmp.width, bmp.height, 8, bmp.width * 4,
                                    CGColorSpaceCreateDeviceRGB(), kCGBitmapByteOrder32Host | kCGImageAlphaNoneSkipFirst);
    Launcher_Active->DrawBackground(Launcher_Active, &bmp);
    
    view_handle.layer.contents = CFBridgingRelease(CGBitmapContextCreateImage(win_ctx));
    Mem_Free(bmp.scan0);
    CGContextRelease(win_ctx);
}

static void LButton_UpdateBackground(struct LButton* w);
void LBackend_ThemeChanged(void) {
    struct LScreen* s = Launcher_Active;
    LBackend_Redraw();
    
    for (int i = 0; i < s->numWidgets; i++)
    {
        struct LWidget* w = s->widgets[i];
        if (w->type != LWIDGET_BUTTON) continue;
        LButton_UpdateBackground((struct LButton*)w);
    }
}

/*########################################################################################################################*
 *------------------------------------------------------ButtonWidget-------------------------------------------------------*
 *#########################################################################################################################*/
static void LButton_UpdateBackground(struct LButton* w) {
    UIButton* btn = (__bridge UIButton*)w->meta;
    CGRect rect   = [btn frame];
    int width     = (int)rect.size.width;
    int height    = (int)rect.size.height;
    // memory freeing deferred until UIImage is freed (see FreeContents)
    struct Bitmap bmp1, bmp2;
    
    Bitmap_Allocate(&bmp1, width, height);
    LButton_DrawBackground(&bmp1, 0, 0, width, height, false);
    [btn setBackgroundImage:ToUIImage(&bmp1) forState:UIControlStateNormal];
    
    Bitmap_Allocate(&bmp2, width, height);
    LButton_DrawBackground(&bmp2, 0, 0, width, height, true);
    [btn setBackgroundImage:ToUIImage(&bmp2) forState:UIControlStateHighlighted];
}

void LBackend_ButtonInit(struct LButton* w, int width, int height) {
    UIButton* btn = [[UIButton alloc] init];
    btn.frame = CGRectMake(0, 0, width, height);
    [btn addTarget:ui_controller action:@selector(handleButtonPress:) forControlEvents:UIControlEventTouchUpInside];
    
    AssignView(w, btn);
    LButton_UpdateBackground(w);
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
void LBackend_CheckboxInit(struct LCheckbox* w) {
    UIView* root  = [[UIView alloc] init];
    UISwitch* swt = [[UISwitch alloc] init];
    [swt setOn:w->value];
    [swt addTarget:ui_controller action:@selector(handleValueChanged:) forControlEvents:UIControlEventValueChanged];
    
    UILabel* lbl  = [[UILabel alloc] init];
    lbl.textColor = [UIColor whiteColor];
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
    AssignView(w, root);
}
void LBackend_CheckboxDraw(struct LCheckbox* w) { }


/*########################################################################################################################*
 *------------------------------------------------------InputWidget--------------------------------------------------------*
 *#########################################################################################################################*/
static void LInput_SetKeyboardType(UITextField* fld, int flags) {
    int type = flags & 0xFF;
    if (type == KEYBOARD_TYPE_INTEGER) {
        [fld setKeyboardType:UIKeyboardTypeNumberPad];
    } else if (type == KEYBOARD_TYPE_PASSWORD) {
        fld.secureTextEntry = YES;
    }
    
    if (flags & KEYBOARD_FLAG_SEND) {
        [fld setReturnKeyType:UIReturnKeySend];
    }
}

static void LInput_SetPlaceholder(UITextField* fld, const char* placeholder) {
    if (!placeholder) return;
    
    cc_string hint  = String_FromReadonly(placeholder);
    fld.placeholder = ToNSString(&hint);
}

void LBackend_InputInit(struct LInput* w, int width) {
    UITextField* fld = [[UITextField alloc] init];
    fld.frame           = CGRectMake(0, 0, width, LINPUT_HEIGHT);
    fld.borderStyle     = UITextBorderStyleBezel;
    fld.backgroundColor = [UIColor whiteColor];
    [fld addTarget:ui_controller action:@selector(handleTextChanged:) forControlEvents:UIControlEventEditingChanged];
    
    LInput_SetKeyboardType(fld, w->inputType);
    LInput_SetPlaceholder(fld,  w->hintText);
    
    AssignView(w, fld);
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
void LBackend_LabelInit(struct LLabel* w) {
    UILabel* lbl  = [[UILabel alloc] init];
    lbl.textColor = [UIColor whiteColor];
    
    AssignView(w, lbl);
}

void LBackend_LabelUpdate(struct LLabel* w) {
    UILabel* lbl = (__bridge UILabel*)w->meta;
    lbl.text     = ToNSString(&w->text);
    [lbl sizeToFit]; // adjust label to fit text
}
void LBackend_LabelDraw(struct LLabel* w) { }


/*########################################################################################################################*
 *-------------------------------------------------------LineWidget--------------------------------------------------------*
 *#########################################################################################################################*/
void LBackend_LineInit(struct LLine* w, int width) {
    UIView* view = [[UIView alloc] init];
    view.frame   = CGRectMake(0, 0, width, LLINE_HEIGHT);
    
    BitmapCol color = LLine_GetColor();
    view.backgroundColor = ToUIColor(color, 0.5f);
    
    AssignView(w, view);
}
void LBackend_LineDraw(struct LLine* w) { }


/*########################################################################################################################*
 *------------------------------------------------------SliderWidget-------------------------------------------------------*
 *#########################################################################################################################*/
void LBackend_SliderInit(struct LSlider* w, int width, int height) {
    UIProgressView* prg = [[UIProgressView alloc] init];
    prg.frame           = CGRectMake(0, 0, width, height);
    prg.progressTintColor = ToUIColor(w->color, 1.0f);

    AssignView(w, prg);
}

void LBackend_SliderUpdate(struct LSlider* w) {
    UIProgressView* prg = (__bridge UIProgressView*)w->meta;
    
    prg.progress = w->value / 100.0f;
}
void LBackend_SliderDraw(struct LSlider* w) { }


/*########################################################################################################################*
 *------------------------------------------------------TableWidget-------------------------------------------------------*
 *#########################################################################################################################*/
void LBackend_TableInit(struct LTable* w) {
    UITableView* tbl = [[UITableView alloc] init];
    tbl.delegate   = ui_controller;
    tbl.dataSource = ui_controller;
    
    //[tbl registerClass:UITableViewCell.class forCellReuseIdentifier:cellID];
    AssignView(w, tbl);
}

void LBackend_TableUpdate(struct LTable* w) {
    UITableView* tbl = (__bridge UITableView*)w->meta;
    [tbl reloadData];
}

// TODO only redraw flags
void LBackend_TableFlagAdded(struct LTable* w) {
    UITableView* tbl = (__bridge UITableView*)w->meta;
    [tbl reloadData];
}

void LBackend_TableDraw(struct LTable* w) { }
void LBackend_TableReposition(struct LTable* w) { }
void LBackend_TableMouseDown(struct LTable* w, int idx) { }
void LBackend_TableMouseUp(struct   LTable* w, int idx) { }
void LBackend_TableMouseMove(struct LTable* w, int idx) { }
