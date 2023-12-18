#include "_WindowBase.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "String.h"
#include "Options.h"
#include <Cocoa/Cocoa.h>
#include <ApplicationServices/ApplicationServices.h>

static int windowX, windowY;
static NSApplication* appHandle;
static NSWindow* winHandle;
static NSView* viewHandle;
static cc_bool canCheckOcclusion;
static cc_bool legacy_fullscreen;
static cc_bool scroll_debugging;

/*########################################################################################################################*
*---------------------------------------------------Shared with Carbon----------------------------------------------------*
*#########################################################################################################################*/
extern size_t CGDisplayBitsPerPixel(CGDirectDisplayID display);
// TODO: Try replacing with NSBitsPerPixelFromDepth([NSScreen mainScreen].depth) instead

// NOTE: If code here is changed, don't forget to update corresponding code in Window_Carbon.c
static void Window_CommonInit(void) {
	CGDirectDisplayID display = CGMainDisplayID();
	CGRect bounds = CGDisplayBounds(display);

	DisplayInfo.x      = (int)bounds.origin.x;
	DisplayInfo.y      = (int)bounds.origin.y;
	DisplayInfo.Width  = (int)bounds.size.width;
	DisplayInfo.Height = (int)bounds.size.height;
	DisplayInfo.Depth  = CGDisplayBitsPerPixel(display);
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
}

static pascal OSErr HandleQuitMessage(const AppleEvent* ev, AppleEvent* reply, long handlerRefcon) {
	Window_Close();
	return 0;
}

static void Window_CommonCreate(void) {
	scroll_debugging = Options_GetBool("scroll-debug", false);
	// for quit buttons in dock and menubar
	AEInstallEventHandler(kCoreEventClass, kAEQuitApplication,
		NewAEEventHandlerUPP(HandleQuitMessage), 0, false);
}

// Sourced from https://www.meandmark.com/keycodes.html
static const cc_uint8 key_map[8 * 16] = {
/* 0x00 */ 'A', 'S', 'D', 'F', 'H', 'G', 'Z', 'X',
/* 0x08 */ 'C', 'V',   0, 'B', 'Q', 'W', 'E', 'R',
/* 0x10 */ 'Y', 'T', '1', '2', '3', '4', '6', '5',
/* 0x18 */ CCKEY_EQUALS, '9', '7', CCKEY_MINUS, '8', '0', CCKEY_RBRACKET, 'O',
/* 0x20 */ 'U', CCKEY_LBRACKET, 'I', 'P', CCKEY_ENTER, 'L', 'J', CCKEY_QUOTE,
/* 0x28 */ 'K', CCKEY_SEMICOLON, CCKEY_BACKSLASH, CCKEY_COMMA, CCKEY_SLASH, 'N', 'M', CCKEY_PERIOD,
/* 0x30 */ CCKEY_TAB, CCKEY_SPACE, CCKEY_TILDE, CCKEY_BACKSPACE, 0, CCKEY_ESCAPE, 0, 0,
/* 0x38 */ 0, CCKEY_CAPSLOCK, 0, 0, 0, 0, 0, 0,
/* 0x40 */ 0, CCKEY_KP_DECIMAL, 0, CCKEY_KP_MULTIPLY, 0, CCKEY_KP_PLUS, 0, CCKEY_NUMLOCK,
/* 0x48 */ 0, 0, 0, CCKEY_KP_DIVIDE, CCKEY_KP_ENTER, 0, CCKEY_KP_MINUS, 0,
/* 0x50 */ 0, CCKEY_KP_ENTER, CCKEY_KP0, CCKEY_KP1, CCKEY_KP2, CCKEY_KP3, CCKEY_KP4, CCKEY_KP5,
/* 0x58 */ CCKEY_KP6, CCKEY_KP7, 0, CCKEY_KP8, CCKEY_KP9, 'N', 'M', CCKEY_PERIOD,
/* 0x60 */ CCKEY_F5, CCKEY_F6, CCKEY_F7, CCKEY_F3, CCKEY_F8, CCKEY_F9, 0, CCKEY_F11,
/* 0x68 */ 0, CCKEY_F13, 0, CCKEY_F14, 0, CCKEY_F10, 0, CCKEY_F12,
/* 0x70 */ 'U', CCKEY_F15, CCKEY_INSERT, CCKEY_HOME, CCKEY_PAGEUP, CCKEY_DELETE, CCKEY_F4, CCKEY_END,
/* 0x78 */ CCKEY_F2, CCKEY_PAGEDOWN, CCKEY_F1, CCKEY_LEFT, CCKEY_RIGHT, CCKEY_DOWN, CCKEY_UP, 0,
};
static int MapNativeKey(UInt32 key) { return key < Array_Elems(key_map) ? key_map[key] : 0; }
// TODO: Check these..
//   case 0x37: return CCKEY_LWIN;
//   case 0x38: return CCKEY_LSHIFT;
//   case 0x3A: return CCKEY_LALT;
//   case 0x3B: return Key_ControlLeft;

// TODO: Verify these differences from OpenTK
//Backspace = 51,  (0x33, CCKEY_DELETE according to that link)
//Return = 52,     (0x34, ??? according to that link)
//Menu = 110,      (0x6E, ??? according to that link)


/*########################################################################################################################*
 *---------------------------------------------------------Cursor---------------------------------------------------------*
 *#########################################################################################################################*/
static cc_bool warping;
static int warpDX, warpDY;

static cc_bool GetMouseCoords(int* x, int* y) {
	NSPoint loc = [NSEvent mouseLocation];
	*x = (int)loc.x                        - windowX;
	*y = (DisplayInfo.Height - (int)loc.y) - windowY;
	// TODO: this seems to be off by 1
	return *x >= 0 && *y >= 0 && *x < WindowInfo.Width && *y < WindowInfo.Height;
}

static void ProcessRawMouseMovement(NSEvent* ev) {
	float dx = [ev deltaX];
	float dy = [ev deltaY];

	if (warping) { dx -= warpDX; dy -= warpDY; }
	Event_RaiseRawMove(&PointerEvents.RawMoved, dx, dy);
}


void Cursor_SetPosition(int x, int y) {
	int curX, curY;
	GetMouseCoords(&curX, &curY);

	CGPoint point;
	point.x = x + windowX;
	point.y = y + windowY;
	CGDisplayMoveCursorToPoint(CGMainDisplayID(), point);

	// Next mouse movement event will include the delta from
	//  this warp - so need to adjust processing to remove the delta
	warping = true;
	warpDX  = x - curX;
	warpDY  = y - curY;
}

static void Cursor_DoSetVisible(cc_bool visible) {
	if (visible) {
		CGDisplayShowCursor(CGMainDisplayID());
	} else {
		CGDisplayHideCursor(CGMainDisplayID());
	}
}

void Window_EnableRawMouse(void) {
	CGAssociateMouseAndMouseCursorPosition(NO);
	DefaultEnableRawMouse();
}

void Window_UpdateRawMouse(void) { }
void Cursor_GetRawPos(int* x, int* y) { *x = 0; *y = 0; }

void Window_DisableRawMouse(void) {
	DefaultDisableRawMouse();
	CGAssociateMouseAndMouseCursorPosition(YES);
}


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
void Clipboard_GetText(cc_string* value) {
	NSPasteboard* pasteboard;
	const char* src;
	NSString* str;
	int len;

	pasteboard = [NSPasteboard generalPasteboard];
	str        = [pasteboard stringForType:NSStringPboardType];

	if (!str) return;
	src = [str UTF8String];
	len = String_Length(src);
	String_AppendUtf8(value, src, len);
}

void Clipboard_SetText(const cc_string* value) {
	NSPasteboard* pasteboard;
	char raw[NATIVE_STR_LEN];
	NSString* str;

	String_EncodeUtf8(raw, value);
	str        = [NSString stringWithUTF8String:raw];
	pasteboard = [NSPasteboard generalPasteboard];

	[pasteboard declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
	[pasteboard setString:str forType:NSStringPboardType];
}


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

static NSAutoreleasePool* pool;
void Window_Init(void) {
	NSSetUncaughtExceptionHandler(LogUnhandledNSErrors);
	Input.Sources = INPUT_SOURCE_NORMAL;

	// https://www.cocoawithlove.com/2009/01/demystifying-nsapplication-by.html
	pool = [[NSAutoreleasePool alloc] init];
	appHandle = [NSApplication sharedApplication];
	[appHandle activateIgnoringOtherApps:YES];
	Window_CommonInit();

	// NSApplication sometimes replaces the uncaught exception handler, so set it again
	NSSetUncaughtExceptionHandler(LogUnhandledNSErrors);
}


/*########################################################################################################################*
*-----------------------------------------------------------Window--------------------------------------------------------*
*#########################################################################################################################*/
#if !defined MAC_OS_X_VERSION_10_4
// Doesn't exist in < 10.4 SDK. No issue since < 10.4 is only Big Endian PowerPC anyways
#define kCGBitmapByteOrder32Host 0
#endif

static void RefreshWindowBounds(void) {
	if (legacy_fullscreen) {
		CGRect rect = CGDisplayBounds(CGMainDisplayID());
		windowX = (int)rect.origin.x; // usually 0
		windowY = (int)rect.origin.y; // usually 0
		// TODO is it correct to use display bounds and not just 0?
		
		WindowInfo.Width  = (int)rect.size.width;
		WindowInfo.Height = (int)rect.size.height;
		return;
	}

	NSRect win  = [winHandle frame];
	NSRect view = [viewHandle frame];
	int viewY;

	// For cocoa, the 0,0 origin is the bottom left corner of windows/views/screen.
	// To get window's real Y screen position, first need to find Y of top. (win.y + win.height)
	// Then just subtract from screen height to make relative to top instead of bottom of the screen.
	// Of course this is only half the story, since we're really after Y position of the content.
	// To work out top Y of view relative to window, it's just win.height - (view.y + view.height)
	viewY   = (int)win.size.height - ((int)view.origin.y + (int)view.size.height);
	windowX = (int)win.origin.x    + (int)view.origin.x;
	windowY = DisplayInfo.Height   - ((int)win.origin.y  + (int)win.size.height) + viewY;

	WindowInfo.Width  = (int)view.size.width;
	WindowInfo.Height = (int)view.size.height;
}

@interface CCWindow : NSWindow { }
@end
@implementation CCWindow
// If this isn't overriden, an annoying beep sound plays anytime a key is pressed
- (void)keyDown:(NSEvent *)event { }
@end

@interface CCWindowDelegate : NSObject { }
@end
@implementation CCWindowDelegate
- (void)windowDidResize:(NSNotification *)notification {
	RefreshWindowBounds();
	Event_RaiseVoid(&WindowEvents.Resized);
}

- (void)windowDidMove:(NSNotification *)notification {
	RefreshWindowBounds();
	GLContext_Update();
}

- (void)windowDidBecomeKey:(NSNotification *)notification {
	WindowInfo.Focused = true;
	Event_RaiseVoid(&WindowEvents.FocusChanged);
}

- (void)windowDidResignKey:(NSNotification *)notification {
	WindowInfo.Focused = false;
	Event_RaiseVoid(&WindowEvents.FocusChanged);
}

- (void)windowDidMiniaturize:(NSNotification *)notification {
	Event_RaiseVoid(&WindowEvents.StateChanged);
}

- (void)windowDidDeminiaturize:(NSNotification *)notification {
	Event_RaiseVoid(&WindowEvents.StateChanged);
}

- (void)windowWillClose:(NSNotification *)notification {
	WindowInfo.Exists = false;
	Event_RaiseVoid(&WindowEvents.Closing);
}
@end


static void DoDrawFramebuffer(NSRect dirty);
@interface CCView : NSView { }
@end
@implementation CCView

- (void)drawRect:(NSRect)dirty { DoDrawFramebuffer(dirty); }

- (void)viewDidEndLiveResize {
	// When the user users left mouse to drag reisze window, this enters 'live resize' mode
	//   Although the game receives a left mouse down event, it does NOT receive a left mouse up
	//   This causes the game to get stuck with left mouse down after user finishes resizing
	// So work arond that by always releasing left mouse when a live resize is finished
	Input_SetReleased(CCMOUSE_L);
}
@end

static void MakeContentView(void) {
	NSRect rect;
	NSView* view;

	view = [winHandle contentView];
	rect = [view frame];

	viewHandle = [CCView alloc];
	[viewHandle initWithFrame:rect];
	[winHandle setContentView:viewHandle];
}

#ifdef CC_BUILD_ICON
// See misc/mac_icon_gen.cs for how to generate this file
#include "_CCIcon_mac.h"

static void ApplyIcon(void) {
	CGColorSpaceRef colSpace;
	CGDataProviderRef provider;
	CGImageRef image;
	CGSize size;
	NSImage* img;

	colSpace = CGColorSpaceCreateDeviceRGB();
	provider = CGDataProviderCreateWithData(NULL, CCIcon_Data,
					Bitmap_DataSize(CCIcon_Width, CCIcon_Height), NULL);
	image    = CGImageCreate(CCIcon_Width, CCIcon_Height, 8, 32, CCIcon_Width * 4, colSpace,
					kCGBitmapByteOrder32Host | kCGImageAlphaLast, provider, NULL, 0, 0);

	size.width = 0; size.height = 0;
	img = [NSImage alloc];
	[img initWithCGImage:image size:size];
	[appHandle setApplicationIconImage:img];

	// TODO need to release NSImage here
	CGImageRelease(image);
	CGDataProviderRelease(provider);
	CGColorSpaceRelease(colSpace);
}
#else
static void ApplyIcon(void) { }
#endif

#define WIN_MASK (NSTitledWindowMask | NSClosableWindowMask | NSResizableWindowMask | NSMiniaturizableWindowMask)
static void DoCreateWindow(int width, int height) {
	CCWindowDelegate* del;
	NSRect rect;

	// Technically the coordinates for the origin are at bottom left corner
	// But since the window is in centre of the screen, don't need to care here
	rect.origin.x    = Display_CentreX(width);  
	rect.origin.y    = Display_CentreY(height);
	rect.size.width  = width; 
	rect.size.height = height;

	winHandle = [CCWindow alloc];
	[winHandle initWithContentRect:rect styleMask:WIN_MASK backing:NSBackingStoreBuffered defer:false];
	[winHandle setAcceptsMouseMovedEvents:YES];
	
	Window_CommonCreate();
	WindowInfo.Exists = true;
	WindowInfo.Handle = winHandle;
	// CGAssociateMouseAndMouseCursorPosition implicitly grabs cursor

	del = [CCWindowDelegate alloc];
	[winHandle setDelegate:del];
	RefreshWindowBounds();
	MakeContentView();
	ApplyIcon();

	canCheckOcclusion = [winHandle respondsToSelector:@selector(occlusionState)];
}
void Window_Create2D(int width, int height) { DoCreateWindow(width, height); }
void Window_Create3D(int width, int height) { DoCreateWindow(width, height); }

void Window_SetTitle(const cc_string* title) {
	char raw[NATIVE_STR_LEN];
	NSString* str;
	String_EncodeUtf8(raw, title);

	str = [NSString stringWithUTF8String:raw];
	[winHandle setTitle:str];
	[str release];
}

// NOTE: Only defined since macOS 10.7 SDK
#define _NSFullScreenWindowMask (1 << 14)
int Window_GetWindowState(void) {
	int flags;

	// modern fullscreen using toggleFullScreen
	flags = [winHandle styleMask];
	if (flags & _NSFullScreenWindowMask) return WINDOW_STATE_FULLSCREEN;

	// legacy fullscreen using CGLSetFullscreen
	if (legacy_fullscreen) return WINDOW_STATE_FULLSCREEN;

	flags = [winHandle isMiniaturized];
	return flags ? WINDOW_STATE_MINIMISED : WINDOW_STATE_NORMAL;
}

// NOTE: Only defined since macOS 10.9 SDK
#define _NSWindowOcclusionStateVisible (1 << 1)
int Window_IsObscured(void) {
    if (!canCheckOcclusion)
        return [winHandle isMiniaturized];
    
    // covers both minimised and hidden behind another window
    int flags = [winHandle occlusionState];
    return !(flags & _NSWindowOcclusionStateVisible);
}

void Window_Show(void) { 
	[winHandle makeKeyAndOrderFront:appHandle];
	RefreshWindowBounds(); // TODO: even necessary?
}

void Window_SetSize(int width, int height) {
	// Can't use setContentSize:, because that resizes from the bottom left corner
	NSRect rect = [winHandle frame];

	rect.origin.y    += WindowInfo.Height - height;
	rect.size.width  += width  - WindowInfo.Width;
	rect.size.height += height - WindowInfo.Height;
	[winHandle setFrame:rect display:YES];
}

void Window_Close(void) { 
	[winHandle close];
}


/*########################################################################################################################*
*-----------------------------------------------------Event processing----------------------------------------------------*
*#########################################################################################################################*/
static int MapNativeMouse(long button) {
	if (button == 0) return CCMOUSE_L;
	if (button == 1) return CCMOUSE_R;
	if (button == 2) return CCMOUSE_M;
	if (button == 3) return CCMOUSE_X1;
	if (button == 4) return CCMOUSE_X2;
	return 0;
}

static void ProcessKeyChars(id ev) {
	const char* src;
	cc_codepoint cp;
	NSString* chars;
	int i, len, flags;

	// Ignore text input while cmd is held down
	// e.g. so Cmd + V to paste doesn't leave behind 'v'
	flags = [ev modifierFlags];
	if (flags & 0x000008) return;
	if (flags & 0x000010) return;

	chars = [ev characters];
	src   = [chars UTF8String];
	len   = String_Length(src);

	while (len > 0) {
		i = Convert_Utf8ToCodepoint(&cp, src, len);
		if (!i) break;

		Event_RaiseInt(&InputEvents.Press, cp);
		src += i; len -= i;
	}
}

static int TryGetKey(NSEvent* ev) {
	int code = [ev keyCode];
	int key  = MapNativeKey(code);
	if (key) return key;

	Platform_Log1("Unknown key %i", &code);
	return 0;
}

static void DebugScrollEvent(NSEvent* ev) {
#ifdef kCGScrollWheelEventDeltaAxis1
	float dy = [ev deltaY];
	int steps = dy > 0.0f ? Math_Ceil(dy) : Math_Floor(dy);
	
	CGEventRef ref = [ev CGEvent];
	if (!ref) return;
	int raw = CGEventGetIntegerValueField(ref, kCGScrollWheelEventDeltaAxis1);
	
	Platform_Log3("SCROLL: %i.0 = (%i, %f3)", &steps, &raw, &dy);
#endif
}

void Window_ProcessEvents(double delta) {
	NSEvent* ev;
	int key, type, steps, x, y;
	float dy;
	
	// https://wiki.freepascal.org/Cocoa_Internals/Application 
	[pool release];
	pool = [[NSAutoreleasePool alloc] init];

	for (;;) {
		ev = [appHandle nextEventMatchingMask:NSAnyEventMask untilDate:Nil inMode:NSDefaultRunLoopMode dequeue:YES];
		if (!ev) break;
		type = [ev type];

		switch (type) {
		case  1: // NSLeftMouseDown 
		case  3: // NSRightMouseDown
		case 25: // NSOtherMouseDown
			key = MapNativeMouse([ev buttonNumber]);
			if (GetMouseCoords(&x, &y) && key) Input_SetPressed(key);
			break;

		case  2: // NSLeftMouseUp 
		case  4: // NSRightMouseUp
		case 26: // NSOtherMouseUp
			key = MapNativeMouse([ev buttonNumber]);
			if (key) Input_SetReleased(key);
			break;

		case 10: // NSKeyDown
			key = TryGetKey(ev);
			if (key) Input_SetPressed(key);
			// TODO: Test works properly with other languages
			ProcessKeyChars(ev);
			break;

		case 11: // NSKeyUp
			key = TryGetKey(ev);
			if (key) Input_SetReleased(key);
			break;

		case 12: // NSFlagsChanged
			key = [ev modifierFlags];
			// TODO: Figure out how to only get modifiers that changed
			Input_Set(CCKEY_LCTRL,    key & 0x000001);
			Input_Set(CCKEY_LSHIFT,   key & 0x000002);
			Input_Set(CCKEY_RSHIFT,   key & 0x000004);
			Input_Set(CCKEY_LWIN,     key & 0x000008);
			Input_Set(CCKEY_RWIN,     key & 0x000010);
			Input_Set(CCKEY_LALT,     key & 0x000020);
			Input_Set(CCKEY_RALT,     key & 0x000040);
			Input_Set(CCKEY_RCTRL,    key & 0x002000);
			Input_Set(CCKEY_CAPSLOCK, key & 0x010000);
			break;

		case 22: // NSScrollWheel
			if (scroll_debugging) DebugScrollEvent(ev);
			dy    = [ev deltaY];
			// https://bugs.eclipse.org/bugs/show_bug.cgi?id=220175
			//  delta is in 'line height' units, but I don't know how to map that to actual units.
			// All I know is that scrolling by '1 wheel notch' produces a delta of around 0.1, and that
			//  sometimes I'll see it go all the way up to 5-6 with a larger wheel scroll.
			// So mulitplying by 10 doesn't really seem a good idea, instead I just round outwards.
			//  TODO: Figure out if there's a better way than this. */
			steps = dy > 0.0f ? Math_Ceil(dy) : Math_Floor(dy);
			Mouse_ScrollWheel(steps);
			break;

		case  5: // NSMouseMoved
		case  6: // NSLeftMouseDragged
		case  7: // NSRightMouseDragged
		case 27: // NSOtherMouseDragged
			if (GetMouseCoords(&x, &y)) Pointer_SetPosition(0, x, y);

			if (Input.RawMode) ProcessRawMouseMovement(ev);
			warping = false;
			break;
		}
		[appHandle sendEvent:ev];
	}
}


/*########################################################################################################################*
*-----------------------------------------------------------Dialogs-------------------------------------------------------*
*#########################################################################################################################*/
void ShowDialogCore(const char* title, const char* msg) {
	CFStringRef titleCF, msgCF;
	NSAlert* alert;
	
	titleCF = CFStringCreateWithCString(NULL, title, kCFStringEncodingASCII);
	msgCF   = CFStringCreateWithCString(NULL, msg,   kCFStringEncodingASCII);
	
	alert = [NSAlert alloc];
	alert = [alert init];
	
	[alert setMessageText: titleCF];
	[alert setInformativeText: msgCF];
	[alert addButtonWithTitle: @"OK"];
	
	[alert runModal];
	CFRelease(titleCF);
	CFRelease(msgCF);
}

static NSMutableArray* GetOpenSaveFilters(const char* const* filters) {
    NSMutableArray* types = [NSMutableArray array];
    int i;

    for (i = 0; filters[i]; i++)
    {
        NSString* filter = [NSString stringWithUTF8String:filters[i]];
        filter = [filter substringFromIndex:1];
        [types addObject:filter];
    }
    return types;
}

static void OpenSaveDoCallback(NSURL* url, FileDialogCallback callback) {
    NSString* str;
    const char* src;
    int len;
    
    str = [url path];
    src = [str UTF8String];
    len = String_Length(src);
    
    cc_string path; char pathBuffer[NATIVE_STR_LEN];
    String_InitArray(path, pathBuffer);
    String_AppendUtf8(&path, src, len);
    callback(&path);
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	NSSavePanel* dlg = [NSSavePanel savePanel];
	
	// TODO: Use args->defaultName, but only macOS 10.6

    NSMutableArray* types = GetOpenSaveFilters(args->filters);
    [dlg setAllowedFileTypes:types];
	if ([dlg runModal] != NSOKButton) return 0;

	NSURL* file = [dlg URL];
    if (file) OpenSaveDoCallback(file, args->Callback);
 	return 0;
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
    NSOpenPanel* dlg = [NSOpenPanel openPanel];
    
    NSMutableArray* types = GetOpenSaveFilters(args->filters);
    [dlg setCanChooseFiles: YES];
    if ([dlg runModalForTypes:types] != NSOKButton) return 0;
    // unfortunately below code doesn't work when linked against SDK < 10.6
    //   https://developer.apple.com/documentation/appkit/nssavepanel/1534419-allowedfiletypes
    // [dlg setAllowedFileTypes:types];
    // if ([dlg runModal] != NSOKButton) return 0;
    
    NSArray* files = [dlg URLs];
    if ([files count] < 1) return 0;
    
    NSURL* file = [files objectAtIndex:0];
    OpenSaveDoCallback(file, args->Callback);
    return 0;
}


/*########################################################################################################################*
*--------------------------------------------------------Framebuffer------------------------------------------------------*
*#########################################################################################################################*/
static struct Bitmap fb_bmp;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	fb_bmp = *bmp;
}

static void DoDrawFramebuffer(NSRect dirty) {
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	CGContextRef context = NULL;
	CGDataProviderRef provider;
	NSGraphicsContext* nsContext;
	CGImageRef image;
	CGRect rect;
	
	Event_RaiseVoid(&WindowEvents.Redrawing);

	// Unfortunately CGImageRef is immutable, so changing the
	//  underlying data doesn't change what shows when drawing.
	// TODO: Find a better way of doing this in cocoa..
	if (!fb_bmp.scan0) return;
	nsContext = [NSGraphicsContext currentContext];
	context   = [nsContext graphicsPort];

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
	NSRect rect;
	rect.origin.x    = r.x; 
	rect.origin.y    = WindowInfo.Height - r.y - r.Height;
	rect.size.width  = r.Width;
	rect.size.height = r.Height;
	
	[viewHandle setNeedsDisplayInRect:rect];
	[viewHandle displayIfNeeded];
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}

void Window_OpenKeyboard(struct OpenKeyboardArgs* args) { }
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { }


/*########################################################################################################################*
*--------------------------------------------------------NSOpenGL---------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_GL && !defined CC_BUILD_EGL
static NSOpenGLContext* ctxHandle;
#include <OpenGL/OpenGL.h>

// SDKs < macOS 10.7 do not have this defined
#ifndef kCGLRPVideoMemoryMegabytes
#define kCGLRPVideoMemoryMegabytes 131
#endif

static int SupportsModernFullscreen(void) {
	return [winHandle respondsToSelector:@selector(toggleFullScreen:)];
}

static NSOpenGLPixelFormat* MakePixelFormat(cc_bool fullscreen) {
	// TODO: Is there a penalty for fullscreen contexts in 10.7 and later?
	// Need to test whether there is a performance penalty or not
	if (SupportsModernFullscreen()) fullscreen = false;
	
	NSOpenGLPixelFormatAttribute attribs[] = {
		NSOpenGLPFAColorSize,    DisplayInfo.Depth,
		NSOpenGLPFADepthSize,    24,
		NSOpenGLPFADoubleBuffer,
		fullscreen ? NSOpenGLPFAFullScreen : 0,
		// TODO do we have to mask to main display? or can we just use -1 for all displays?
		NSOpenGLPFAScreenMask,   CGDisplayIDToOpenGLDisplayMask(CGMainDisplayID()),
		0
	};
	return [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
}

void GLContext_Create(void) {
	NSOpenGLPixelFormat* fmt;
	fmt = MakePixelFormat(true);
	if (!fmt) {
		Platform_LogConst("Failed to create full screen pixel format.");
		Platform_LogConst("Trying again to create a non-fullscreen pixel format.");
		fmt = MakePixelFormat(false);
	}
	if (!fmt) Logger_Abort("Choosing pixel format");

	ctxHandle = [NSOpenGLContext alloc];
	ctxHandle = [ctxHandle initWithFormat:fmt shareContext:Nil];
	if (!ctxHandle) Logger_Abort("Failed to create OpenGL context");

	[ctxHandle setView:viewHandle];
	[fmt release];
	[ctxHandle makeCurrentContext];
	[ctxHandle update];
}

void GLContext_Update(void) {
	// TODO: Why does this crash on resizing
	[ctxHandle update];
}
cc_bool GLContext_TryRestore(void) { return true; }

void GLContext_Free(void) { 
	[NSOpenGLContext clearCurrentContext];
	[ctxHandle clearDrawable];
	[ctxHandle release];
}

void* GLContext_GetAddress(const char* function) {
	static const cc_string glPath = String_FromConst("/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL");
	static void* lib;

	if (!lib) lib = DynamicLib_Load2(&glPath);
	return DynamicLib_Get2(lib, function);
}

cc_bool GLContext_SwapBuffers(void) {
	[ctxHandle flushBuffer];
	return true;
}

void GLContext_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	int value = vsync ? 1 : 0;
	[ctxHandle setValues:&value forParameter: NSOpenGLCPSwapInterval];
}

// kCGLCPCurrentRendererID is only available on macOS 10.4 and later
#if defined MAC_OS_X_VERSION_10_4
static const char* GetAccelerationMode(CGLContextObj ctx) {
	GLint fGPU, vGPU;
	
	// NOTE: only macOS 10.4 or later
	if (CGLGetParameter(ctx, kCGLCPGPUFragmentProcessing, &fGPU)) return NULL;
	if (CGLGetParameter(ctx, kCGLCPGPUVertexProcessing,   &vGPU)) return NULL;
	
	if (fGPU && vGPU) return "Fully";
	if (fGPU || vGPU) return "Partially";
	return "Not";
}

void GLContext_GetApiInfo(cc_string* info) {
	CGLContextObj ctx = [ctxHandle CGLContextObj];
	GLint rendererID;
	CGLGetParameter(ctx, kCGLCPCurrentRendererID, &rendererID);
	
	GLint nRenders = 0;
	CGLRendererInfoObj rend;
	CGLQueryRendererInfo(-1, &rend, &nRenders);
	
	for (int i = 0; i < nRenders; i++)
	{
		GLint curID = -1;
		CGLDescribeRenderer(rend, i, kCGLRPRendererID, &curID);
		if (curID != rendererID) continue;
		
		GLint acc = 0;
		CGLDescribeRenderer(rend, i, kCGLRPAccelerated, &acc);
		const char* mode = GetAccelerationMode(ctx);
		
		GLint vram = 0;
		if (!CGLDescribeRenderer(rend, i, kCGLRPVideoMemoryMegabytes, &vram)) {
			// preferred path (macOS 10.7 or later)
		} else if (!CGLDescribeRenderer(rend, i, kCGLRPVideoMemory, &vram)) {
			vram /= (1024 * 1024); // TODO: use float instead?
		} else {
			vram = -1; // TODO show a better error?
		}
		
		if (mode && acc) {
			String_Format2(info, "VRAM: %i MB, %c HW accelerated\n", &vram, mode);
		} else {
			String_Format2(info, "VRAM: %i MB, %c\n",
						   &vram, acc ? "HW accelerated" : "no HW acceleration");
		}
		break;
	}
	CGLDestroyRendererInfo(rend);
}
#else
// macOS 10.3 and earlier case
void GLContext_GetApiInfo(cc_string* info) {
	// TODO: retrieve rendererID from a CGLPixelFormatObj, but this isn't all that important
}
#endif

cc_result Window_EnterFullscreen(void) {
	if (SupportsModernFullscreen()) {
		[winHandle toggleFullScreen:appHandle];
		return 0;
	}

	Platform_LogConst("Falling back to legacy fullscreen..");
	legacy_fullscreen = true;
	[ctxHandle clearDrawable];
	CGDisplayCapture(CGMainDisplayID());

	// setFullScreen doesn't return an error code, which is unfortunate
	//  because if setFullScreen fails, you're left with a blank window
	//  that's still rendering thousands of frames per second
	//[ctxHandle setFullScreen];
	//return 0;
	
	// CGLSetFullScreenOnDisplay is the preferable API, because it  
	//  works properly on macOS 10.7 and all later versions
	// However, because this API was only introduced in 10.7, it 
	//  is essentially useless for us - because the superior 
	//  toggleFullScreen API is already used in macOS 10.7+
	//cc_result res = CGLSetFullScreenOnDisplay([ctxHandle CGLContextObj], CGDisplayIDToOpenGLDisplayMask(CGMainDisplayID()));
	
	// CGLSetFullsScreen has existed since macOS 10.1, however
	//  it was deprecated in 10.6 - and by deprecated, Apple
	//  REALLY means deprecated. If the SDK ClassiCube is compiled
	//  against is 10.6 or later, then CGLSetFullScreen will always
	//  fail to work (CGLSetFullScreenOnDisplay still works) though
	// So make sure you compile ClassiCube with an older SDK version
	cc_result res = CGLSetFullScreen([ctxHandle CGLContextObj]);

	if (res) Window_ExitFullscreen();
	RefreshWindowBounds();
	Event_RaiseVoid(&WindowEvents.Resized);
	return res;
}

cc_result Window_ExitFullscreen(void) {
	if (SupportsModernFullscreen()) {
		[winHandle toggleFullScreen:appHandle];
		return 0;
	}

	legacy_fullscreen = false;
	CGDisplayRelease(CGMainDisplayID());
	[ctxHandle clearDrawable];
	[ctxHandle setView:viewHandle];

	RefreshWindowBounds();
	Event_RaiseVoid(&WindowEvents.Resized);
	return 0;
}
#endif
