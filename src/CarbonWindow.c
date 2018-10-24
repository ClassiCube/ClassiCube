#include "Window.h"
#ifdef CC_BUILD_OSX
#include "ErrorHandler.h"

WindowRef win_handle;
Rect win_titleSize;
int window_state;

static OSStatus Window_EventHandler(IntPtr inCaller, IntPtr inEvent, IntPtr userData) {
	EventInfo evt = new EventInfo(inEvent);
	switch (evt.EventClass) {
	case EventClass.AppleEvent:
		// only event here is the apple event.
		Debug.Print("Processing apple event.");
		API.ProcessAppleEvent(inEvent);
		break;

	case EventClass.Keyboard:
	case EventClass.Mouse:
		if (WindowEventHandler != null) {
			return WindowEventHandler.DispatchEvent(inCaller, inEvent, evt, userData);
		}
		break;
	}
	return OSStatus.EventNotHandled;
}

static void Window_ConnectEvents(void) {
	EventTypeSpec[] eventTypes = new EventTypeSpec[]{
		new EventTypeSpec(EventClass.Application, AppEventKind.AppActivated),
		new EventTypeSpec(EventClass.Application, AppEventKind.AppDeactivated),
		new EventTypeSpec(EventClass.Application, AppEventKind.AppQuit),

		new EventTypeSpec(EventClass.Mouse, MouseEventKind.MouseDown),
		new EventTypeSpec(EventClass.Mouse, MouseEventKind.MouseUp),
		new EventTypeSpec(EventClass.Mouse, MouseEventKind.MouseMoved),
		new EventTypeSpec(EventClass.Mouse, MouseEventKind.MouseDragged),
		new EventTypeSpec(EventClass.Mouse, MouseEventKind.MouseEntered),
		new EventTypeSpec(EventClass.Mouse, MouseEventKind.MouseExited),
		new EventTypeSpec(EventClass.Mouse, MouseEventKind.WheelMoved),

		new EventTypeSpec(EventClass.Keyboard, KeyboardEventKind.RawKeyDown),
		new EventTypeSpec(EventClass.Keyboard, KeyboardEventKind.RawKeyRepeat),
		new EventTypeSpec(EventClass.Keyboard, KeyboardEventKind.RawKeyUp),
		new EventTypeSpec(EventClass.Keyboard, KeyboardEventKind.RawKeyModifiersChanged),

		new EventTypeSpec(EventClass.Window, WindowEventKind.WindowClose),
		new EventTypeSpec(EventClass.Window, WindowEventKind.WindowClosed),
		new EventTypeSpec(EventClass.Window, WindowEventKind.WindowBoundsChanged),
		new EventTypeSpec(EventClass.Window, WindowEventKind.WindowActivate),
		new EventTypeSpec(EventClass.Window, WindowEventKind.WindowDeactivate),

		new EventTypeSpec(EventClass.AppleEvent, AppleEventKind.AppleEvent),
	};

	uppHandler = NewEventHandlerUPP(handler);
	IntPtr target = GetApplicationEventTarget();
	/* TODO: Use IntPtr target = GetWindowEventTarget(windowRef); instead?? */
	OSStatus result = InstallEventHandler(target, uppHandlerProc, eventTypes.Length, eventTypes, NULL, NULL);
	CheckReturn(result);
}

void Window_Create(int x, int y, int width, int height, const String* title, struct GraphicsMode* mode, struct DisplayDevice* device) {
	Rect r;
	OSStatus err;
	CFStringRef titleCF;
	ProcessSerialNumber psn;

	r.X = x; r.Y = y; r.Width = width; r.Height = height;
	err = CreateNewWindow(kDocumentWindowClass, attrib, &r, &win_handle);
	if (err) ErrorHandler_Fail(err, "Failed to create window");

	/* TODO: Use UTF8 encoding instead!!!! */
	/* Use Platform_ConvertString */
	titleCF = CFStringCreateWithBytes(kCFAllocatorDefault, title->buffer, title->length, kCFStringEncodingASCII, false);
	SetWindowTitleWithCFString(win_handle, titleCF);

	Window_SetLocation(r.X, r.Y);
	Window_SetSize(r.Width, r.Height);
	LoadSize();

	win_titleSize = GetWindowBounds(win_handle, kWindowTitleBarRgn);
	AcquireRootMenu();

	GetCurrentProcess(&psn);
	TransformProcessType(&psn, ProcessApplicationTransformState.kProcessTransformToForegroundApplication);
	SetFrontProcess(& psn);
	/* TODO: Use BringWindowToFront instead.. (look in the file which has RepositionWindow in it) !!!! */
	Window_ConnectEvents();
}

void Window_GetClipboardText(String* value);
void Window_SetClipboardText(const String* value);
/* TODO: IMPLEMENT void Window_SetIcon(Bitmap* bmp); */

bool Window_GetVisible(void) {
	return IsWindowVisible(win_handle);
}

void Window_SetVisible(bool visible) {
	if (visible == Window_GetVisible()) return;

	if (visible) {
		ShowWindow(win_handle);
		RepositionWindow(win_handle, NULL, kWindowCenterOnMainScreen);
		SelectWindow(win_handle);
	} else {
		HideWindow(win_handle);
	}
}

void* Window_GetWindowHandle(void) { return win_handle; }
int Window_GetWindowState(void);
void Window_SetWindowState(int value);

void Window_SetBounds(Rect2D rect) {
	Window_SetLocation(rect.X, rect.Y);
	Window_SetSize(rect.Width, rect.Height);
}

void Window_SetLocation(int x, int y);
void Window_SetSize(int width, int height);
void Window_SetClientSize(int width, int height);

void Window_Close(void);
void Window_ProcessEvents(void);

Point2D Window_PointToClient(int x, int y) {
	Rect r;
	Point2D p;
	GetWindowBounds(win_handle, kWindowContentRgn, &r);

	p.X = x - r.X; p.Y = y - r.Y;
	return p;
}

Point2D Window_PointToScreen(int x, int y) {
	Rect r;
	Point2D p;
	GetWindowBounds(win_handle, kWindowContentRgn, &r);

	p.X = x + r.X; p.Y = y + r.Y;
	return p;
}

Point2D Window_GetScreenCursorPos(void) {
	HIPoint point;
	Point2D p;
	/* NOTE: HIGetMousePosition is only available on OSX 10.5 or later */
	/* TODO: Use GetGlobalMouse instead!!!! */
	HIGetMousePosition(kHICoordSpaceScreenPixel, NULL, &point);

	p.X = (int)point.X; p.Y = (int)point.Y;
	return p;
}

void Window_SetScreenCursorPos(int x, int y) {
	CGPoint point;
	point.x = x; point.y = y;

	CGAssociateMouseAndMouseCursorPosition(0);
	CGDisplayMoveCursorToPoint(CGMainDisplayID(), point);
	CGAssociateMouseAndMouseCursorPosition(1);
}

bool win_cursorVisible;
bool Window_GetCursorVisible(void) { return win_cursorVisible; }

void Window_SetCursorVisible(bool visible) {
	win_cursorVisible = visible;
	if (visible) {
		CGDisplayShowCursor(CGMainDisplayID());
	} else {
		CGDisplayHideCursor(CGMainDisplayID());
	}
}


/*########################################################################################################################*
*-----------------------------------------------------OpenGL context------------------------------------------------------*
*#########################################################################################################################*/
void GLContext_Init(struct GraphicsMode* mode);
void GLContext_Update(void);
void GLContext_Free(void);

void* GLContext_GetAddress(const char* function);
void GLContext_SwapBuffers(void);
void GLContext_SetVSync(bool enabled);
#endif
