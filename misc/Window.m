#include "Window.h"
#include "Platform.h"
#import <Cocoa/Cocoa.h>

// this is the source used to generate the raw code used in window.c
// clang -rewrite-objc winmm.m
// generates a winmm.cpp with the objective C code as C++

static NSApplication* appHandle;
static NSWindow* winHandle;

extern void Window_CommonInit(void);
void Window_Init(void) {
	appHandle = [NSApplication sharedApplication];
	[appHandle activateIgnoringOtherApps: YES];
	Window_CommonInit();
}

#define Display_CentreX(width)  (Display_Bounds.X + (Display_Bounds.Width  - width)  / 2)
#define Display_CentreY(height) (Display_Bounds.Y + (Display_Bounds.Height - height) / 2)

void Window_Create(int width, int height) {
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
}

void Window_SetTitle(const String* title) {
	UInt8 str[600];
	CFStringRef titleCF;
	int len;

	/* TODO: This leaks memory, old title isn't released */
	len = Platform_ConvertString(str, title);
	titleCF = CFStringCreateWithBytes(kCFAllocatorDefault, str, len, kCFStringEncodingUTF8, false);
	[winHandle setTitle: (NSString*)titleCF];
}

static int Window_MapMouse(int button) {
	if (button == 0) return Key_
}

void Window_ProcessEvents(void) {
	NSEvent* ev;
	int type, button;

	for (;;) {
		ev = [appHandle nextEventMatchingMask: 0xFFFFFFFFU untilDate:Nil inMode:NSDefaultRunLoopMode dequeue:YES];
		if (!ev) break;
		type = [ev type];

		switch (type) {
		case NSLeftMouseDown:
		case NSRightMouseDown:
		case NSOtherMouseDown:
			button = [ev buttonNumber];

		case NSLeftMouseUp:
		case NSRightMouseUp:
		case NSOtherMouseUp:
			button = [ev buttonNumber];
		}

		Platform_Log1("EVENT: %i", &type);
		[appHandle sendEvent:ev];
	}
}
