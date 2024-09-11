// Silence deprecation warnings on modern iOS
#define GLES_SILENCE_DEPRECATION
#include "Core.h"

#if defined CC_BUILD_IOS
#include "Bitmap.h"
#include "Input.h"
#include "Platform.h"
#include "String.h"
#include "Errors.h"
#include "Drawer2D.h"
#include "Launcher.h"
#include "Funcs.h"
#include "Gui.h"
#include "Window.h"
#include "Event.h"
#include "Logger.h"
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

// shared state with Window_ios.m
extern UIViewController* cc_controller;

UIColor* ToUIColor(BitmapCol color, float A);
NSString* ToNSString(const cc_string* text);
UIInterfaceOrientationMask SupportedOrientations(void);
void LogUnhandledNSErrors(NSException* ex);


@interface CCAppDelegate : UIResponder<UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
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
void LogUnhandledNSErrors(NSException* ex) {
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

#endif
