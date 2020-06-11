#include "Window.h"
#include "Platform.h"
#include "Input.h"
#include "Event.h"
#include "Logger.h"
#import <Cocoa/Cocoa.h>

// this is the source used to generate the raw code used in window.c
// clang -rewrite-objc winmm.m
// generates a winmm.cpp with the objective C code as C++
// I also compile the game including this file and manually verify assembly output

static NSApplication* appHandle;
static NSWindow* winHandle;
static int windowX, windowY;

struct GraphicsMode { int R, G, B, A, IsIndexed; };
extern void Window_CommonInit(void);
extern int Window_MapKey(UInt32 key);

@interface ClassiCubeWindowDelegate : NSObject { }
@end

@implementation ClassiCubeWindowDelegate
static void Window_RefreshBounds(void);

- (void)windowDidResize:(NSNotification *)notification
{
	Window_RefreshBounds();
	Event_RaiseVoid(&WindowEvents.Resized);
}

- (void)windowDidMove:(NSNotification *)notification
{
	Window_RefreshBounds();
	GLContext_Update();
}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
	WindowInfo.Focused = true;
	Event_RaiseVoid(&WindowEvents.FocusChanged);
}

- (void)windowDidResignKey:(NSNotification *)notification
{
	WindowInfo.Focused = false;
	Event_RaiseVoid(&WindowEvents.FocusChanged);
}

- (void)windowDidMiniaturize:(NSNotification *)notification
{
	Event_RaiseVoid(&WindowEvents.StateChanged);
}

- (void)windowDidDeminiaturize:(NSNotification *)notification
{
	Event_RaiseVoid(&WindowEvents.StateChanged);
}

- (void)windowWillClose:(NSNotification *)notification
{
	Event_RaiseVoid(&WindowEvents.Closing);
}
@end


static void Window_RefreshBounds(void) {
	NSView* view;
	NSRect rect;

	view = [winHandle contentView];
	rect = [view bounds];
	rect = [winHandle convertRectToScreen: rect];

	windowX = (int)rect.origin.x;
	windowY = (int)rect.origin.y;
	WindowInfo.Width  = (int)rect.size.width;
	WindowInfo.Height = (int)rect.size.height;
	Platform_Log2("WINPOS: %i, %i", &windowX, &windowY);
}

void Window_SetSize1(int width, int height) {
	NSSize size; 
	size.width = width; size.height = height;
	[winHandle setContentSize: size];
}

void Window_Close1(void) {
	[winHandle close];
}

void Window_Init1(void) {
	appHandle = [NSApplication sharedApplication];
	[appHandle activateIgnoringOtherApps: YES];
	Window_CommonInit();
}

#define Display_CentreX(width)  (DisplayInfo.X + (DisplayInfo.Width  - width)  / 2)
#define Display_CentreY(height) (DisplayInfo.Y + (DisplayInfo.Height - height) / 2)

void Window_Create1(int width, int height) {
	NSRect rect;
	// TODO: don't set, RefreshBounds
	WindowInfo.Width  = width;
	WindowInfo.Height = height;
	WindowInfo.Exists = true;

	rect.origin.x    = Display_CentreX(width);
	rect.origin.y    = Display_CentreY(height);
	rect.size.width  = width;
	rect.size.height = height;
	// TODO: opentk seems to flip y?

	winHandle = [NSWindow alloc];
	[winHandle initWithContentRect: rect styleMask: NSTitledWindowMask|NSClosableWindowMask|NSResizableWindowMask|NSMiniaturizableWindowMask backing:0 defer:NO];

	[winHandle makeKeyAndOrderFront: appHandle];
	Window_RefreshBounds();
}

void Window_SetTitle1(const String* title) {
	UInt8 str[600];
	CFStringRef titleCF;
	int len;

	/* TODO: This leaks memory, old title isn't released */
	len = Platform_ConvertString(str, title);
	titleCF = CFStringCreateWithBytes(kCFAllocatorDefault, str, len, kCFStringEncodingUTF8, false);
	[winHandle setTitle: (NSString*)titleCF];
}

static int Window_MapMouse(int button) {
	if (button == 0) return KEY_LMOUSE;
	if (button == 1) return KEY_RMOUSE;
	if (button == 2) return KEY_MMOUSE;
	return 0;
}

void Window_ProcessEvents1(void) {
	NSEvent* ev;
	NSPoint loc;
	CGFloat dx, dy;
	int type, key, mouseX, mouseY;

	for (;;) {
		ev = [appHandle nextEventMatchingMask: 0xFFFFFFFFU untilDate:Nil inMode:NSDefaultRunLoopMode dequeue:YES];
		if (!ev) break;
		type = [ev type];

		switch (type) {
		case NSLeftMouseDown:
		case NSRightMouseDown:
		case NSOtherMouseDown:
			key = Window_MapMouse([ev buttonNumber]);
			if (key) Input_SetPressed(key, true);
			break;

		case NSLeftMouseUp:
		case NSRightMouseUp:
		case NSOtherMouseUp:
			key = Window_MapMouse([ev buttonNumber]);
			if (key) Input_SetPressed(key, false);
			break;
		
		case NSKeyDown:
			key = Window_MapKey([ev keyCode]);
			if (key) Input_SetPressed(key, true);
			break;

		case NSKeyUp:
			key = Window_MapKey([ev keyCode]);
			if (key) Input_SetPressed(key, false);
			break;

		case NSScrollWheel:
			Mouse_ScrollWheel([ev deltaY]);
			break;

		case NSMouseMoved:
		case NSLeftMouseDragged:
		case NSRightMouseDragged:
		case NSOtherMouseDragged:
			loc = [NSEvent mouseLocation];
			dx  = [ev deltaX];
			dy  = [ev deltaY];

			mouseX = (int)loc.x - windowX;
			mouseY = (int)loc.y - windowY;
			/* need to flip Y coordinates because cocoa has window origin at bottom left */
			mouseY = WindowInfo.Height - mouseY;
			Pointer_SetPosition(0, mouseX, mouseY);

			if (Input_RawMode) Event_RaiseRawMove(&PointerEvents.RawMoved, dx, dy);
			break;
		}

		Platform_Log1("EVENT: %i", &type);
		[appHandle sendEvent:ev];
	}
}

static NSOpenGLContext* ctxHandle;
static NSOpenGLPixelFormat* SelectPixelFormat(struct GraphicsMode* mode, bool fullscreen) {
	NSOpenGLPixelFormat* fmt;
	uint32_t attribs[7] = {
		NSOpenGLPFAColorSize,     0,
		NSOpenGLPFADepthSize,     GLCONTEXT_DEFAULT_DEPTH,
		NSOpenGLPFADoubleBuffer,  0, 0
	};

	attribs[1] = mode->R + mode->G + mode->B + mode->A;
	attribs[5] = fullscreen ? NSOpenGLPFAFullScreen : 0;

	fmt = [NSOpenGLPixelFormat alloc];
	return [fmt initWithAttributes: attribs];
}

void GLContext_Init1(struct GraphicsMode* mode) {
	NSView* view;
	NSOpenGLPixelFormat* fmt;
	
	fmt = SelectPixelFormat(mode, true);
	if (!fmt) {
		Platform_LogConst("Failed to create full screen pixel format.");
		Platform_LogConst("Trying again to create a non-fullscreen pixel format.");
		fmt = SelectPixelFormat(mode, false);
	}
	if (!fmt) Logger_Abort("Choosing pixel format");

	ctxHandle = [NSOpenGLContext alloc];
	ctxHandle = [ctxHandle initWithFormat:fmt shareContext:Nil];
	if (!ctxHandle) Logger_Abort("Failed to create OpenGL context");

	view = [winHandle contentView];
	[ctxHandle setView:view];
	/* TODO: Support high DPI OSX */
	/* [ctxHandle setWantsBestResolutionOpenGLSurface:YES]; */

	[fmt release];
	[ctxHandle makeCurrentContext];
	[ctxHandle update];
}

void GLContext_Update1(void) {
	[ctxHandle update];
}

void GLContext_Free1(void) {
	[NSOpenGLContext clearCurrentContext];
	[ctxHandle clearDrawable];
	[ctxHandle release];
}

bool GLContext_SwapBuffers1(void) {
	[ctxHandle flushBuffer];
	return true; 
}

void GLContext_SetFpsLimit1(bool vsync, float minFrameMs) {
	GLint value = vsync ? 1 : 0;
	[ctxHandle setValues: &value forParameter: NSOpenGLCPSwapInterval];
}
