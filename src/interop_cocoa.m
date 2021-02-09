#include "Logger.h"
#include "ExtMath.h"
#include "Platform.h"
#include "Window.h"
#include "Input.h"
#include "Event.h"
#include "Bitmap.h"
#include "String.h"
#include <Cocoa/Cocoa.h>

/*########################################################################################################################*
*-------------------------------------------------------Cocoa window------------------------------------------------------*
*#########################################################################################################################*/
static NSApplication* appHandle;
static NSWindow* winHandle;
static NSView* viewHandle;
extern int windowX, windowY;
extern void Window_CommonCreate(void);
extern void Window_CommonInit(void);
extern int MapNativeKey(UInt32 key);

#ifndef kCGBitmapByteOrder32Host
/* Undefined in < 10.4 SDK. No issue since < 10.4 is only Big Endian PowerPC anyways */
#define kCGBitmapByteOrder32Host 0
#endif

static void RefreshWindowBounds(void) {
	NSRect win, view;
	int viewY;

	win  = [winHandle frame];
	view = [viewHandle frame];

	/* For cocoa, the 0,0 origin is the bottom left corner of windows/views/screen. */
	/* To get window's real Y screen position, first need to find Y of top. (win.y + win.height) */
	/* Then just subtract from screen height to make relative to top instead of bottom of the screen. */
	/* Of course this is only half the story, since we're really after Y position of the content. */
	/* To work out top Y of view relative to window, it's just win.height - (view.y + view.height) */
	viewY   = (int)win.size.height - ((int)view.origin.y + (int)view.size.height);
	windowX = (int)win.origin.x    + (int)view.origin.x;
	windowY = DisplayInfo.Height   - ((int)win.origin.y  + (int)win.size.height) + viewY;

	WindowInfo.Width  = (int)view.size.width;
	WindowInfo.Height = (int)view.size.height;
}

@interface CCWindow : NSWindow { }
@end
@implementation CCWindow
/* If this isn't overriden, an annoying beep sound plays anytime a key is pressed */
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


static void DoDrawFramebuffer(CGRect dirty);
@interface CCView : NSView { }
@end
@implementation CCView

- (void)drawRect:(CGRect)dirty { DoDrawFramebuffer(dirty); }

- (void)viewDidEndLiveResize {
	/* When the user users left mouse to drag reisze window, this enters 'live resize' mode */
	/*   Although the game receives a left mouse down event, it does NOT receive a left mouse up */
	/*   This causes the game to get stuck with left mouse down after user finishes resizing */
	/* So work arond that by always releasing left mouse when a live resize is finished */
	Input_SetReleased(KEY_LMOUSE);
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

static NSAutoreleasePool* pool;
void Window_Init(void) {
	pool = [[NSAutoreleasePool alloc] init];
	appHandle = [NSApplication sharedApplication];
	[appHandle activateIgnoringOtherApps:YES];
	Window_CommonInit();
}

#ifdef CC_BUILD_ICON
extern const int CCIcon_Data[];
extern const int CCIcon_Width, CCIcon_Height;

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

	/* TODO need to release NSImage here */
	CGImageRelease(image);
	CGDataProviderRelease(provider);
	CGColorSpaceRelease(colSpace);
}
#else
static void ApplyIcon(void) { }
#endif

#define WIN_MASK (NSTitledWindowMask | NSClosableWindowMask | NSResizableWindowMask | NSMiniaturizableWindowMask)
void Window_Create(int width, int height) {
	CCWindowDelegate* del;
	NSRect rect;

	/* Technically the coordinates for the origin are at bottom left corner */
	/* But since the window is in centre of the screen, don't need to care here */
	rect.origin.x    = Display_CentreX(width);  
	rect.origin.y    = Display_CentreY(height);
	rect.size.width  = width; 
	rect.size.height = height;

	winHandle = [CCWindow alloc];
	[winHandle initWithContentRect:rect styleMask:WIN_MASK backing:NSBackingStoreBuffered defer:false];
	[winHandle setAcceptsMouseMovedEvents:YES];
	
	Window_CommonCreate();
	del = [CCWindowDelegate alloc];
	[winHandle setDelegate:del];
	RefreshWindowBounds();
	MakeContentView();
	ApplyIcon();
}

void Window_SetTitle(const cc_string* title) {
	UInt8 str[NATIVE_STR_LEN];
	CFStringRef titleCF;
	int len;

	/* TODO: This leaks memory, old title isn't released */
	len = Platform_EncodeUtf8(str, title);
	titleCF = CFStringCreateWithBytes(kCFAllocatorDefault, str, len, kCFStringEncodingUTF8, false);
	[winHandle setTitle:titleCF];
}

void Window_Show(void) { 
	[winHandle makeKeyAndOrderFront:appHandle];
	RefreshWindowBounds(); // TODO: even necessary?
}

/* NOTE: Only defined since macOS 10.7 SDK */
#define _NSFullScreenWindowMask (1 << 14)
int Window_GetWindowState(void) {
	int flags;

	flags = [winHandle styleMask];
	if (flags & _NSFullScreenWindowMask) return WINDOW_STATE_FULLSCREEN;
	     
	flags = [winHandle isMiniaturized];
	return flags ? WINDOW_STATE_MINIMISED : WINDOW_STATE_NORMAL;
}

// TODO: Only works on 10.7+
cc_result Window_EnterFullscreen(void) {
	[winHandle toggleFullScreen:appHandle];
	return 0;
}
cc_result Window_ExitFullscreen(void) {
	[winHandle toggleFullScreen:appHandle];
	return 0;
}

void Window_SetSize(int width, int height) {
	/* Can't use setContentSize:, because that resizes from the bottom left corner. */
	NSRect rect = [winHandle frame];

	rect.origin.y    += WindowInfo.Height - height;
	rect.size.width  += width  - WindowInfo.Width;
	rect.size.height += height - WindowInfo.Height;
	[winHandle setFrame:rect display:YES];
}

void Window_Close(void) { 
	[winHandle close];
}

static int MapNativeMouse(int button) {
	if (button == 0) return KEY_LMOUSE;
	if (button == 1) return KEY_RMOUSE;
	if (button == 2) return KEY_MMOUSE;
	return 0;
}

static void ProcessKeyChars(id ev) {
	char buffer[128];
	const char* src;
	cc_string str;
	NSString* chars;
	int i, len, flags;

	/* Ignore text input while cmd is held down */
	/* e.g. so Cmd + V to paste doesn't leave behind 'v' */
	flags = [ev modifierFlags];
	if (flags & 0x000008) return;
	if (flags & 0x000010) return;

	chars = [ev characters];
	src   = [chars UTF8String];
	len   = String_Length(src);
	String_InitArray(str, buffer);

	String_AppendUtf8(&str, src, len);
	for (i = 0; i < str.length; i++) {
		Event_RaiseInt(&InputEvents.Press, str.buffer[i]);
	}
}

static cc_bool GetMouseCoords(int* x, int* y) {
	NSPoint loc = [NSEvent mouseLocation];
	*x = (int)loc.x                        - windowX;	
	*y = (DisplayInfo.Height - (int)loc.y) - windowY;
	// TODO: this seems to be off by 1
	return *x >= 0 && *y >= 0 && *x < WindowInfo.Width && *y < WindowInfo.Height;
}

static int TryGetKey(NSEvent* ev) {
	int code = [ev keyCode];
	int key  = MapNativeKey(code);
	if (key) return key;

	Platform_Log1("Unknown key %i", &code);
	return 0;
}

void Window_ProcessEvents(void) {
	NSEvent* ev;
	int key, type, steps, x, y;
	CGFloat dx, dy;
	
	[pool release];
	pool = [[NSAutoreleasePool alloc] init];

	for (;;) {
		ev = [appHandle nextEventMatchingMask:NSAnyEventMask untilDate:Nil inMode:NSDefaultRunLoopMode dequeue:YES];
		if (!ev) break;
		type = [ev type];

		switch (type) {
		case  1: /* NSLeftMouseDown  */
		case  3: /* NSRightMouseDown */
		case 25: /* NSOtherMouseDown */
			key = MapNativeMouse([ev buttonNumber]);
			if (GetMouseCoords(&x, &y) && key) Input_SetPressed(key);
			break;

		case  2: /* NSLeftMouseUp  */
		case  4: /* NSRightMouseUp */
		case 26: /* NSOtherMouseUp */
			key = MapNativeMouse([ev buttonNumber]);
			if (key) Input_SetReleased(key);
			break;

		case 10: /* NSKeyDown */
			key = TryGetKey(ev);
			if (key) Input_SetPressed(key);
			// TODO: Test works properly with other languages
			ProcessKeyChars(ev);
			break;

		case 11: /* NSKeyUp */
			key = TryGetKey(ev);
			if (key) Input_SetReleased(key);
			break;

		case 12: /* NSFlagsChanged */
			key = [ev modifierFlags];
			/* TODO: Figure out how to only get modifiers that changed */
			Input_Set(KEY_LCTRL,    key & 0x000001);
			Input_Set(KEY_LSHIFT,   key & 0x000002);
			Input_Set(KEY_RSHIFT,   key & 0x000004);
			Input_Set(KEY_LWIN,     key & 0x000008);
			Input_Set(KEY_RWIN,     key & 0x000010);
			Input_Set(KEY_LALT,     key & 0x000020);
			Input_Set(KEY_RALT,     key & 0x000040);
			Input_Set(KEY_RCTRL,    key & 0x002000);
			Input_Set(KEY_CAPSLOCK, key & 0x010000);
			break;

		case 22: /* NSScrollWheel */
			dy    = [ev deltaY];
			/* https://bugs.eclipse.org/bugs/show_bug.cgi?id=220175 */
			/* delta is in 'line height' units, but I don't know how to map that to actual units. */
			/* All I know is that scrolling by '1 wheel notch' produces a delta of around 0.1, and that */
			/* sometimes I'll see it go all the way up to 5-6 with a larger wheel scroll. */
			/* So mulitplying by 10 doesn't really seem a good idea, instead I just round outwards. */
			/* TODO: Figure out if there's a better way than this. */
			steps = dy > 0.0f ? Math_Ceil(dy) : Math_Floor(dy);
			Mouse_ScrollWheel(steps);
			break;

		case  5: /* NSMouseMoved */
		case  6: /* NSLeftMouseDragged */
		case  7: /* NSRightMouseDragged */
		case 27: /* NSOtherMouseDragged */
			if (GetMouseCoords(&x, &y)) Pointer_SetPosition(0, x, y);

			if (Input_RawMode) {
				dx = [ev deltaX];
				dy = [ev deltaY];
				Event_RaiseRawMove(&PointerEvents.RawMoved, dx, dy);
			}
			break;
		}
		[appHandle sendEvent:ev];
	}
}

void Cursor_GetRawPos(int* x, int* y) { *x = 0; *y = 0; }
void ShowDialogCore(const char* title, const char* msg) {
	CFStringRef titleCF, msgCF;
	NSAlert* alert;
	
	alert   = [NSAlert alloc];
	alert   = [alert init];
	titleCF = CFStringCreateWithCString(NULL, title, kCFStringEncodingASCII);
	msgCF   = CFStringCreateWithCString(NULL, msg,   kCFStringEncodingASCII);
	
	[alert setMessageText: titleCF];
	[alert setInformativeText: msgCF];
	[alert addButtonWithTitle: CFSTR("OK")];
	
	[alert runModal];
	CFRelease(titleCF);
	CFRelease(msgCF);
}

static struct Bitmap fb_bmp;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	fb_bmp = *bmp;
}

static void DoDrawFramebuffer(CGRect dirty) {
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	CGContextRef context = NULL;
	CGDataProviderRef provider;
	NSGraphicsContext* nsContext;
	CGImageRef image;
	CGRect rect;

	/* Unfortunately CGImageRef is immutable, so changing the */
	/* underlying data doesn't change what shows when drawing. */
	/* TODO: Find a better way of doing this in cocoa.. */
	if (!fb_bmp.scan0) return;
	nsContext = [NSGraphicsContext currentContext];
	context   = [nsContext graphicsPort];

	/* TODO: Only update changed bit.. */
	rect.origin.x = 0; rect.origin.y = 0;
	rect.size.width  = WindowInfo.Width;
	rect.size.height = WindowInfo.Height;

	/* TODO: REPLACE THIS AWFUL HACK */
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
	rect.origin.x    = r.X; 
	rect.origin.y    = WindowInfo.Height - r.Y - r.Height;
	rect.size.width  = r.Width;
	rect.size.height = r.Height;
	
	[viewHandle setNeedsDisplayInRect:rect];
	[viewHandle displayIfNeeded];
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}


/*########################################################################################################################*
*--------------------------------------------------------NSOpenGL---------------------------------------------------------*
*#########################################################################################################################*/
static NSOpenGLContext* ctxHandle;

static NSOpenGLPixelFormat* MakePixelFormat(cc_bool fullscreen) {
	NSOpenGLPixelFormatAttribute attribs[7] = {
		NSOpenGLPFAColorSize,    0,
		NSOpenGLPFADepthSize,    24,
		NSOpenGLPFADoubleBuffer, 0, 0
	};

	attribs[1] = DisplayInfo.Depth;
	attribs[5] = fullscreen ? NSOpenGLPFAFullScreen : 0;
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

void GLContext_Free(void) { 
	[NSOpenGLContext clearCurrentContext];
	[ctxHandle clearDrawable];
	[ctxHandle release];
}

cc_bool GLContext_SwapBuffers(void) {
	[ctxHandle flushBuffer];
	return true;
}

void GLContext_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	int value = vsync ? 1 : 0;
	[ctxHandle setValues:&value forParameter: NSOpenGLCPSwapInterval];
}
