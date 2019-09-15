#include "Window.h"
#include "Platform.h"
#include "Input.h"
#include "Event.h"
#import <Cocoa/Cocoa.h>

// this is the source used to generate the raw code used in window.c
// clang -rewrite-objc winmm.m
// generates a winmm.cpp with the objective C code as C++
// I also compile the game including this file and manually verify assembly output

static NSApplication* appHandle;
static NSWindow* winHandle;
static int windowX, windowY;

extern void Window_CommonInit(void);
extern int Window_MapKey(UInt32 key);

static void Window_RefreshBounds(void) {
	NSView* view;
	NSRect rect;

	view = [winHandle contentView];
	rect = [view frame];

	windowX = (int)rect.origin.x;
	windowY = (int)rect.origin.y;
	Window_Width  = (int)rect.size.width;
	Window_Height = (int)rect.size.height;
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

#define Display_CentreX(width)  (Display_Bounds.X + (Display_Bounds.Width  - width)  / 2)
#define Display_CentreY(height) (Display_Bounds.Y + (Display_Bounds.Height - height) / 2)

void Window_Create1(int width, int height) {
	NSRect rect;
	// TODO: don't set, RefreshBounds
	Window_Width  = width;
	Window_Height = height;
	Window_Exists = true;

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
			Mouse_SetWheel(Mouse_Wheel + [ev deltaY]);
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
			Pointer_SetPosition(0, mouseX, mouseY);

			if (Input_RawMode) Event_RaiseMove(&PointerEvents.RawMoved, 0, dx, dy);
			break;
		}

		Platform_Log1("EVENT: %i", &type);
		[appHandle sendEvent:ev];
	}
}
