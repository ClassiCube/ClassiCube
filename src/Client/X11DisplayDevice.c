#include "DisplayDevice.h"
#if CC_BUILD_X11
#include "Typedefs.h"
#include "ErrorHandler.h"
#include <X11/Xlib.h>

void DisplayDevice_Init(void) {
	Display* display = XOpenDisplay(NULL);
	if (display == NULL) {
		ErrorHandler_Fail("Failed to open display");
	}

	int screen = XDefaultScreen(display);
	Window rootWin = XRootWindow(display, screen);

	/* TODO: Use Xinerama and XRandR for querying these */
	int screens = XScreenCount(display), i;
	for (i = 0; i < screens; i++) {
		struct DisplayDevice device = { 0 };
		device.Bounds.Width  = DisplayWidth(display, i);
		device.Bounds.Height = DisplayHeight(display, i);
		device.BitsPerPixel = DefaultDepth(display, i);
		if (i == screen) DisplayDevice_Default = device;
	}

	DisplayDevice_Meta[0] = display;
	DisplayDevice_Meta[1] = screen;
	DisplayDevice_Meta[2] = rootWin;
}
#endif
