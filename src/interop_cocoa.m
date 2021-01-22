#include "Logger.h"
#include "Platform.h"
#include "Window.h"
#include "Input.h"
#include "Event.h"
#include "Bitmap.h"
#include "String.h"
#include <Cocoa/Cocoa.h>
#include <OpenGL/OpenGL.h>
#include <objc/message.h>
#include <objc/runtime.h>

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

static void RefreshWindowBounds(void) {
	CGRect win, view;
	int viewY;

	win  = [winHandle frame];
	view = [viewHandle frame];

	/* For cocoa, the 0,0 origin is the bottom left corner of windows/views/screen. */
	/* To get window's real Y screen position, first need to find Y of top. (win.y + win.height) */
	/* Then just subtract from screen height to make relative to top instead of bottom of the screen. */
	/* Of course this is only half the story, since we're really after Y position of the content. */
	/* To work out top Y of view relative to window, it's just win.height - (view.y + view.height) */
	viewY   = (int)win.size.height  - ((int)view.origin.y + (int)view.size.height);
	windowX = (int)win.origin.x     + (int)view.origin.x;
	windowY = DisplayInfo.Height - ((int)win.origin.y  + (int)win.size.height) + viewY;

	WindowInfo.Width  = (int)view.size.width;
	WindowInfo.Height = (int)view.size.height;
}

static void OnDidResize(id self, SEL cmd, id notification) {
	RefreshWindowBounds();
	Event_RaiseVoid(&WindowEvents.Resized);
}

static void OnDidMove(id self, SEL cmd, id notification) {
	RefreshWindowBounds();
	GLContext_Update();
}

static void OnDidBecomeKey(id self, SEL cmd, id notification) {
	WindowInfo.Focused = true;
	Event_RaiseVoid(&WindowEvents.FocusChanged);
}

static void OnDidResignKey(id self, SEL cmd, id notification) {
	WindowInfo.Focused = false;
	Event_RaiseVoid(&WindowEvents.FocusChanged);
}

static void OnDidMiniaturize(id self, SEL cmd, id notification) {
	Event_RaiseVoid(&WindowEvents.StateChanged);
}

static void OnDidDeminiaturize(id self, SEL cmd, id notification) {
	Event_RaiseVoid(&WindowEvents.StateChanged);
}

static void OnWillClose(id self, SEL cmd, id notification) {
	WindowInfo.Exists = false;
	Event_RaiseVoid(&WindowEvents.Closing);
}

/* If this isn't overriden, an annoying beep sound plays anytime a key is pressed */
static void OnKeyDown(id self, SEL cmd, id ev) { }

static Class Window_MakeClass(void) {
	Class c = objc_allocateClassPair(objc_getClass("NSWindow"), "ClassiCube_Window", 0);

	class_addMethod(c, sel_registerName("windowDidResize:"),        OnDidResize,        "v@:@");
	class_addMethod(c, sel_registerName("windowDidMove:"),          OnDidMove,          "v@:@");
	class_addMethod(c, sel_registerName("windowDidBecomeKey:"),     OnDidBecomeKey,     "v@:@");
	class_addMethod(c, sel_registerName("windowDidResignKey:"),     OnDidResignKey,     "v@:@");
	class_addMethod(c, sel_registerName("windowDidMiniaturize:"),   OnDidMiniaturize,   "v@:@");
	class_addMethod(c, sel_registerName("windowDidDeminiaturize:"), OnDidDeminiaturize, "v@:@");
	class_addMethod(c, sel_registerName("windowWillClose:"),        OnWillClose,        "v@:@");
	class_addMethod(c, sel_registerName("keyDown:"),                OnKeyDown,          "v@:@");

	objc_registerClassPair(c);
	return c;
}

/* When the user users left mouse to drag reisze window, this enters 'live resize' mode */
/*   Although the game receives a left mouse down event, it does NOT receive a left mouse up */
/*   This causes the game to get stuck with left mouse down after user finishes resizing */
/* So work arond that by always releasing left mouse when a live resize is finished */
static void DidEndLiveResize(id self, SEL cmd) {
	Input_SetReleased(KEY_LMOUSE);
}

static void View_DrawRect(id self, SEL cmd, CGRect r);
static void MakeContentView(void) {
	CGRect rect;
	id view;
	Class c;

	view = [winHandle contentView];
	rect = [view frame];
	
	c = objc_allocateClassPair(objc_getClass("NSView"), "ClassiCube_View", 0);
	// TODO: test rect is actually correct in View_DrawRect on both 32 and 64 bit
#ifdef __i386__
	class_addMethod(c, sel_registerName("drawRect:"), View_DrawRect, "v@:{NSRect={NSPoint=ff}{NSSize=ff}}");
#else
	class_addMethod(c, sel_registerName("drawRect:"), View_DrawRect, "v@:{NSRect={NSPoint=dd}{NSSize=dd}}");
#endif
	class_addMethod(c, sel_registerName("viewDidEndLiveResize"), DidEndLiveResize, "v@:");
	objc_registerClassPair(c);

	viewHandle = [c alloc];
	[viewHandle initWithFrame:rect];
	[winHandle setContentView:viewHandle];
}

void Window_Init(void) {
	appHandle = [NSApplication sharedApplication];
	[appHandle activateIgnoringOtherApps:true];
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
					kCGBitmapByteOrder32Little | kCGImageAlphaLast, provider, NULL, 0, 0);

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
	Class winClass;
	CGRect rect;

	/* Technically the coordinates for the origin are at bottom left corner */
	/* But since the window is in centre of the screen, don't need to care here */
	rect.origin.x    = Display_CentreX(width);  
	rect.origin.y    = Display_CentreY(height);
	rect.size.width  = width; 
	rect.size.height = height;

	winClass  = Window_MakeClass();
	winHandle = [winClass alloc];
	[winHandle initWithContentRect:rect styleMask:WIN_MASK backing:0 defer:false];
	
	Window_CommonCreate();
	[winHandle setDelegate:winHandle];
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

int Window_GetWindowState(void) {
	int flags;

	flags = [winHandle styleMask];
	if (flags & NSFullScreenWindowMask) return WINDOW_STATE_FULLSCREEN;
	     
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
	CGRect rect = [winHandle frame];

	rect.origin.y    += WindowInfo.Height - height;
	rect.size.width  += width  - WindowInfo.Width;
	rect.size.height += height - WindowInfo.Height;
	[winHandle setFrame:rect display:true];
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
	id chars;
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
	CGPoint loc = [NSEvent mouseLocation];
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

	for (;;) {
		ev = [appHandle nextEventMatchingMask:0xFFFFFFFFU untilDate:NULL inMode:NSDefaultRunLoopMode dequeue:true];
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

static void View_DrawRect(id self, SEL cmd, CGRect r_) {
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	CGContextRef context = NULL;
	CGDataProviderRef provider;
	CGImageRef image;
	CGRect rect;
	id nsContext;

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
		kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst, provider, NULL, 0, 0);

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
	ctxHandle = [ctxHandle initWithFormat:fmt shareContext:NULL];
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
	[ctxHandle setValues:&value forParameter: NSOpenGLContextParameterSwapInterval];
}
