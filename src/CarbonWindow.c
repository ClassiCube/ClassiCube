#include "Window.h"
#ifdef CC_BUILD_OSX
#include <AGL/agl.h>
#include "Event.h"
#include "ErrorHandler.h"
#include "Funcs.h"
#include "Input.h"
#include "Platform.h"
#include "ErrorHandler.h"

WindowRef win_handle;
int title_height;
int win_state;
/* Hacks for fullscreen */
bool ctx_pendingWindowed, ctx_pendingFullscreen;

#define Rect_Width(rect)  (rect.right  - rect.left)
#define Rect_Height(rect) (rect.bottom - rect.top)

/*########################################################################################################################*
*-----------------------------------------------------Private details-----------------------------------------------------*
*#########################################################################################################################*/
static Key Window_MapKey(UInt32 key) {
	/* Sourced from https://www.meandmark.com/keycodes.html */
	switch (key) {
		case 0x00: return Key_A;
		case 0x01: return Key_S;
		case 0x02: return Key_D;
		case 0x03: return Key_F;
		case 0x04: return Key_H;
		case 0x05: return Key_G;
		case 0x06: return Key_Z;
		case 0x07: return Key_X;
		case 0x08: return Key_C;
		case 0x09: return Key_V;
		case 0x0B: return Key_B;
		case 0x0C: return Key_Q;
		case 0x0D: return Key_W;
		case 0x0E: return Key_E;
		case 0x0F: return Key_R;
			
		case 0x10: return Key_Y;
		case 0x11: return Key_T;
		case 0x12: return Key_1;
		case 0x13: return Key_2;
		case 0x14: return Key_3;
		case 0x15: return Key_4;
		case 0x16: return Key_6;
		case 0x17: return Key_5;
		case 0x18: return Key_Plus;
		case 0x19: return Key_9;
		case 0x1A: return Key_7;
		case 0x1B: return Key_Minus;
		case 0x1C: return Key_8;
		case 0x1D: return Key_0;
		case 0x1E: return Key_BracketRight;
		case 0x1F: return Key_O;
			
		case 0x20: return Key_U;
		case 0x21: return Key_BracketLeft;
		case 0x22: return Key_I;
		case 0x23: return Key_P;
		case 0x24: return Key_Enter;
		case 0x25: return Key_L;
		case 0x26: return Key_J;
		case 0x27: return Key_Quote;
		case 0x28: return Key_K;
		case 0x29: return Key_Semicolon;
		case 0x2A: return Key_BackSlash;
		case 0x2B: return Key_Comma;
		case 0x2C: return Key_Slash;
		case 0x2D: return Key_N;
		case 0x2E: return Key_M;
		case 0x2F: return Key_Period;
			
		case 0x30: return Key_Tab;
		case 0x31: return Key_Space;
		case 0x32: return Key_Tilde;
		case 0x33: return Key_BackSpace;
		case 0x35: return Key_Escape;
		/*case 0x37: return Key_WinLeft; */
		/*case 0x38: return Key_ShiftLeft; */
		case 0x39: return Key_CapsLock;
		/*case 0x3A: return Key_AltLeft; */
		/*case 0x3B: return Key_ControlLeft; */
			
		case 0x41: return Key_KeypadDecimal;
		case 0x43: return Key_KeypadMultiply;
		case 0x45: return Key_KeypadAdd;
		case 0x4B: return Key_KeypadDivide;
		case 0x4C: return Key_KeypadEnter;
		case 0x4E: return Key_KeypadSubtract;
			
		case 0x51: return Key_KeypadEnter;
		case 0x52: return Key_Keypad0;
		case 0x53: return Key_Keypad1;
		case 0x54: return Key_Keypad2;
		case 0x55: return Key_Keypad3;
		case 0x56: return Key_Keypad4;
		case 0x57: return Key_Keypad5;
		case 0x58: return Key_Keypad6;
		case 0x59: return Key_Keypad7;
		case 0x5B: return Key_Keypad8;
		case 0x5C: return Key_Keypad9;
		case 0x5D: return Key_N;
		case 0x5E: return Key_M;
		case 0x5F: return Key_Period;
			
		case 0x60: return Key_F5;
		case 0x61: return Key_F6;
		case 0x62: return Key_F7;
		case 0x63: return Key_F3;
		case 0x64: return Key_F8;
		case 0x65: return Key_F9;
		case 0x67: return Key_F11;
		case 0x69: return Key_F13;
		case 0x6B: return Key_F14;
		case 0x6D: return Key_F10;
		case 0x6F: return Key_F12;
			
		case 0x70: return Key_U;
		case 0x71: return Key_F15;
		case 0x72: return Key_Insert;
		case 0x73: return Key_Home;
		case 0x74: return Key_PageUp;
		case 0x75: return Key_Delete;
		case 0x76: return Key_F4;
		case 0x77: return Key_End;
		case 0x78: return Key_F2;
		case 0x79: return Key_PageDown;
		case 0x7A: return Key_F1;
		case 0x7B: return Key_Left;
		case 0x7C: return Key_Right;
		case 0x7D: return Key_Down;
		case 0x7E: return Key_Up;
	}
	return Key_None;
	/* TODO: Verify these differences */
	/*
	 Backspace = 51,  (0x33, Key_Delete according to that link)
	 Return = 52,     (0x34, ??? according to that link)
	 Menu = 110,      (0x6E, ??? according to that ink)
	 */
}

static void Window_Destroy(void) {
	if (!Window_Exists) return;
	DisposeWindow(win_handle);
	Window_Exists = false;
}

static void Window_UpdateSize(void) {
	Rect r;
	OSStatus res;
	if (win_state == WINDOW_STATE_FULLSCREEN) return;
	
	res = GetWindowBounds(win_handle, kWindowStructureRgn, &r);
	if (res) ErrorHandler_Fail2(res, "Getting window bounds");
	Window_Bounds.X = r.left;
	Window_Bounds.Y = r.top;
	Window_Bounds.Width  = Rect_Width(r);
	Window_Bounds.Height = Rect_Height(r);
	
	res = GetWindowBounds(win_handle, kWindowGlobalPortRgn, &r);
	if (res) ErrorHandler_Fail2(res, "Getting window clientsize");
	Window_ClientSize.Width  = Rect_Width(r);
	Window_ClientSize.Height = Rect_Height(r);
}

static void Window_UpdateWindowState(void) {
	Point idealSize;
	OSStatus res;

	switch (win_state) {
	case WINDOW_STATE_FULLSCREEN:
		ctx_pendingFullscreen = true;
		break;

	case WINDOW_STATE_MAXIMISED:
		/* Hack because OSX has no concept of maximised. Instead windows are "zoomed", 
		meaning they are maximised up to their reported ideal size. So report a large ideal size. */
		idealSize.v = 9000; idealSize.h = 9000;
		res = ZoomWindowIdeal(win_handle, inZoomOut, &idealSize);
		if (res) ErrorHandler_Fail2(res, "Maximising window");
		break;

	case WINDOW_STATE_NORMAL:
		if (Window_GetWindowState() == WINDOW_STATE_MAXIMISED) {
			idealSize.v = 0; idealSize.h = 0;
			res = ZoomWindowIdeal(win_handle, inZoomIn, &idealSize);
			if (res) ErrorHandler_Fail2(res, "Un-maximising window");
		}
		break;

	case WINDOW_STATE_MINIMISED:
		res = CollapseWindow(win_handle, true);
		if (res) ErrorHandler_Fail2(res, "Minimising window");
		break;
	}

	Event_RaiseVoid(&WindowEvents_StateChanged);
	Window_UpdateSize();
	Event_RaiseVoid(&WindowEvents_Resized);
}

OSStatus Window_ProcessKeyboardEvent(EventHandlerCallRef inCaller, EventRef inEvent, void* userData) {
	UInt32 kind, code;
	Key key;
	char charCode, raw;
	bool repeat;
	OSStatus res;
	
	kind = GetEventKind(inEvent);
	switch (kind) {
		case kEventRawKeyDown:
		case kEventRawKeyRepeat:
		case kEventRawKeyUp:
			res = GetEventParameter(inEvent, kEventParamKeyCode, typeUInt32, 
									NULL, sizeof(UInt32), NULL, &code);
			if (res) ErrorHandler_Fail2(res, "Getting key button");
			
			res = GetEventParameter(inEvent, kEventParamKeyMacCharCodes, typeChar, 
									NULL, sizeof(char), NULL, &charCode);
			if (res) ErrorHandler_Fail2(res, "Getting key char");
			
			key = Window_MapKey(code);
			if (key == Key_None) {
				Platform_Log1("Key %i not mapped, ignoring press.", &code);
				return 0;
			}
			break;
	}

	switch (kind) {
		/* TODO: Should we be messing with KeyRepeat in kEventRawKeyRepeat here? */
		/* Looking at documentation, probably not */
		case kEventRawKeyDown:
		case kEventRawKeyRepeat:
			Key_SetPressed(key, true);

			/* TODO: Should we be using kEventTextInputUnicodeForKeyEvent for this */
			/* Look at documentation for kEventRawKeyRepeat */
			if (!Convert_TryUnicodeToCP437((uint8_t)charCode, &raw)) return 0;
			Event_RaiseInt(&KeyEvents_Press, raw);
			return 0;
			
		case kEventRawKeyUp:
			Key_SetPressed(key, false);
			return 0;
			
		case kEventRawKeyModifiersChanged:
			res = GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, 
									NULL, sizeof(UInt32), NULL, &code);
			if (res) ErrorHandler_Fail2(res, "Getting key modifiers");
			
			/* TODO: Is this even needed */
			repeat = Key_KeyRepeat; 
			Key_KeyRepeat = false;
			
			Key_SetPressed(Key_ControlLeft, (code & 0x1000) != 0);
			Key_SetPressed(Key_AltLeft,     (code & 0x0800) != 0);
			Key_SetPressed(Key_ShiftLeft,   (code & 0x0200) != 0);
			Key_SetPressed(Key_WinLeft,     (code & 0x0100) != 0);			
			Key_SetPressed(Key_CapsLock,    (code & 0x0400) != 0);
			
			Key_KeyRepeat = repeat;
			return 0;
	}
	return eventNotHandledErr;
}

OSStatus Window_ProcessWindowEvent(EventHandlerCallRef inCaller, EventRef inEvent, void* userData) {
	int width, height;
	
	switch (GetEventKind(inEvent)) {
		case kEventWindowClose:
			Event_RaiseVoid(&WindowEvents_Closing);
			return eventNotHandledErr;
			
		case kEventWindowClosed:
			Window_Exists = false;
			Event_RaiseVoid(&WindowEvents_Closed);
			return 0;
			
		case kEventWindowBoundsChanged:
			width  = Window_ClientSize.Width;
			height = Window_ClientSize.Height;
			Window_UpdateSize();
			
			if (width != Window_ClientSize.Width || height != Window_ClientSize.Height) {
				Event_RaiseVoid(&WindowEvents_Resized);
			}
			return eventNotHandledErr;
			
		case kEventWindowActivated:
			Window_Focused = true;
			Event_RaiseVoid(&WindowEvents_FocusChanged);
			return eventNotHandledErr;
			
		case kEventWindowDeactivated:
			Window_Focused = false;
			Event_RaiseVoid(&WindowEvents_FocusChanged);
			return eventNotHandledErr;
	}
	return eventNotHandledErr;
}

OSStatus Window_ProcessMouseEvent(EventHandlerCallRef inCaller, EventRef inEvent, void* userData) {
	HIPoint pt;
	Point2D mousePos;
	UInt32 kind;
	bool down;
	EventMouseButton button;
	SInt32 delta;
	OSStatus res;	
	
	if (win_state == WINDOW_STATE_FULLSCREEN) {
		res = GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint,
								NULL, sizeof(HIPoint), NULL, &pt);
	} else {
		res = GetEventParameter(inEvent, kEventParamWindowMouseLocation, typeHIPoint, 
								NULL, sizeof(HIPoint), NULL, &pt);
	}
	
	/* this error comes up from the application event handler */
	if (res && res != eventParameterNotFoundErr) {
		ErrorHandler_Fail2(res, "Getting mouse position");
	}
	
	mousePos.X = (int)pt.x; mousePos.Y = (int)pt.y;
	/* Location is relative to structure (i.e. external size) of window */
	if (win_state != WINDOW_STATE_FULLSCREEN) {
		mousePos.Y -= title_height;
	}
	
	kind = GetEventKind(inEvent);
	switch (kind) {
		case kEventMouseDown:
		case kEventMouseUp:
			down = kind == kEventMouseDown;
			res  = GetEventParameter(inEvent, kEventParamMouseButton, typeMouseButton, 
									 NULL, sizeof(EventMouseButton), NULL, &button);
			if (res) ErrorHandler_Fail2(res, "Getting mouse button");
			
			switch (button) {
				case kEventMouseButtonPrimary:
					Mouse_SetPressed(MouseButton_Left, down); break;
				case kEventMouseButtonSecondary:
					Mouse_SetPressed(MouseButton_Right, down); break;
				case kEventMouseButtonTertiary:
					Mouse_SetPressed(MouseButton_Middle, down); break;
			}
			return 0;
			
		case kEventMouseWheelMoved:
			res = GetEventParameter(inEvent, kEventParamMouseWheelDelta, typeSInt32,
									NULL, sizeof(SInt32), NULL, &delta);
			if (res) ErrorHandler_Fail2(res, "Getting mouse wheel delta");
			Mouse_SetWheel(Mouse_Wheel + delta);
			return 0;
			
		case kEventMouseMoved:
		case kEventMouseDragged:
			if (win_state != WINDOW_STATE_FULLSCREEN) {
				/* Ignore clicks in the title bar */
				if (pt.y < 0) return eventNotHandledErr;
			}
			
			if (mousePos.X != Mouse_X || mousePos.Y != Mouse_Y) {
				Mouse_SetPosition(mousePos.X, mousePos.Y);
			}
			return eventNotHandledErr;
	}
	return eventNotHandledErr;
}

static OSStatus Window_EventHandler(EventHandlerCallRef inCaller, EventRef inEvent, void* userData) {
	EventRecord record;
	
	switch (GetEventClass(inEvent)) {
		case kEventClassAppleEvent:
			/* Only event here is the apple event. */
			Platform_LogConst("Processing apple event.");
			ConvertEventRefToEventRecord(inEvent, &record);
			AEProcessAppleEvent(&record);
			break;
			
		case kEventClassKeyboard:
			return Window_ProcessKeyboardEvent(inCaller, inEvent, userData);
		case kEventClassMouse:
			return Window_ProcessMouseEvent(inCaller, inEvent, userData);
		case kEventClassWindow:
			return Window_ProcessWindowEvent(inCaller, inEvent, userData);
	}
	return eventNotHandledErr;
}

static void Window_ConnectEvents(void) {
	static EventTypeSpec eventTypes[] = {
		{ kEventClassApplication, kEventAppActivated },
		{ kEventClassApplication, kEventAppDeactivated },
		{ kEventClassApplication, kEventAppQuit },
		
		{ kEventClassMouse, kEventMouseDown },
		{ kEventClassMouse, kEventMouseUp },
		{ kEventClassMouse, kEventMouseMoved },
		{ kEventClassMouse, kEventMouseDragged },
		{ kEventClassMouse, kEventMouseEntered},
		{ kEventClassMouse, kEventMouseExited },
		{ kEventClassMouse, kEventMouseWheelMoved },
		
		{ kEventClassKeyboard, kEventRawKeyDown },
		{ kEventClassKeyboard, kEventRawKeyRepeat },
		{ kEventClassKeyboard, kEventRawKeyUp },
		{ kEventClassKeyboard, kEventRawKeyModifiersChanged },
		
		{ kEventClassWindow, kEventWindowClose },
		{ kEventClassWindow, kEventWindowClosed },
		{ kEventClassWindow, kEventWindowBoundsChanged },
		{ kEventClassWindow, kEventWindowActivated },
		{ kEventClassWindow, kEventWindowDeactivated },
		
		{ kEventClassAppleEvent, kEventAppleEvent }
	};
	EventTargetRef target;
	OSStatus res;
	
	target = GetApplicationEventTarget();
	/* TODO: Use EventTargetRef target = GetWindowEventTarget(windowRef); instead?? */
	res = InstallEventHandler(target, NewEventHandlerUPP(Window_EventHandler),
							  Array_Elems(eventTypes), eventTypes, NULL, NULL);
	if (res) ErrorHandler_Fail2(res, "Connecting events");
}


/*########################################################################################################################*
 *--------------------------------------------------Public implementation--------------------------------------------------*
 *#########################################################################################################################*/
void Window_Create(int x, int y, int width, int height, struct GraphicsMode* mode) {
	Rect r;
	OSStatus res;
	
	ProcessSerialNumber psn;
	
	r.left = x; r.right  = x + width; 
	r.top  = y; r.bottom = y + height;
	res = CreateNewWindow(kDocumentWindowClass,
						  kWindowStandardDocumentAttributes | kWindowStandardHandlerAttribute |
						  kWindowInWindowMenuAttribute | kWindowLiveResizeAttribute,
						  &r, &win_handle);
	if (res) ErrorHandler_Fail2(res, "Failed to create window");

	Window_SetLocation(r.left, r.right);
	Window_SetSize(Rect_Width(r), Rect_Height(r));
	Window_UpdateSize();
	
	res = GetWindowBounds(win_handle, kWindowTitleBarRgn, &r);
	if (res) ErrorHandler_Fail2(res, "Failed to get titlebar size");
	title_height = Rect_Height(r);
	AcquireRootMenu();
	
	/* TODO: Apparently GetCurrentProcess is needed */
	GetCurrentProcess(&psn);
	/* NOTE: TransformProcessType is OSX 10.3 or later */
	TransformProcessType(&psn, kProcessTransformToForegroundApplication);
	SetFrontProcess(&psn);
	
	/* TODO: Use BringWindowToFront instead.. (look in the file which has RepositionWindow in it) !!!! */
	Window_ConnectEvents();
	Window_Exists = true;
}

void Window_SetTitle(const String* title) {
	char str[600];
	CFStringRef titleCF;
	int len;
	
	len     = Platform_ConvertString(str, title);
	titleCF = CFStringCreateWithBytes(kCFAllocatorDefault, str, len, kCFStringEncodingUTF8, false);
	SetWindowTitleWithCFString(win_handle, titleCF);
}

/* NOTE: All Pasteboard functions are OSX 10.3 or later */
PasteboardRef Window_GetPasteboard(void) {
	PasteboardRef pbRef;
	OSStatus err = PasteboardCreate(kPasteboardClipboard, &pbRef);
	
	if (err) ErrorHandler_Fail2(err, "Creating Pasteboard reference");
	PasteboardSynchronize(pbRef);
	return pbRef;
}
#define FMT_UTF8  CFSTR("public.utf8-plain-text")
#define FMT_UTF16 CFSTR("public.utf16-plain-text")

void Window_GetClipboardText(String* value) {
	PasteboardRef pbRef;
	ItemCount itemCount;
	PasteboardItemID itemID;
	CFDataRef outData;
	const UInt8* ptr;
	OSStatus err;
	
	pbRef = Window_GetPasteboard();
	
	err = PasteboardGetItemCount(pbRef, &itemCount);
	if (err) ErrorHandler_Fail2(err, "Getting item count from Pasteboard");
	if (itemCount < 1) return;
	
	err = PasteboardGetItemIdentifier(pbRef, 1, &itemID);
	if (err) ErrorHandler_Fail2(err, "Getting item identifier from Pasteboard");
	
	if (!(err = PasteboardCopyItemFlavorData(pbRef, itemID, FMT_UTF16, &outData))) {
		ptr = CFDataGetBytePtr(outData);
		if (!ptr) ErrorHandler_Fail("CFDataGetBytePtr() returned null pointer");
		return Marshal.PtrToStringUni(ptr);
	} else if (!(err = PasteboardCopyItemFlavorData(pbRef, itemID, FMT_UTF8, &outData))) {
		ptr = CFDataGetBytePtr(outData);
		if (!ptr) ErrorHandler_Fail("CFDataGetBytePtr() returned null pointer");
		return GetUTF8(ptr);
	}
}

void Window_SetClipboardText(const String* value) {
	PasteboardRef pbRef;
	CFDataRef cfData;
	char str[800];
	int len;
	OSStatus err;

	pbRef = Window_GetPasteboard();
	err   = PasteboardClear(pbRef);
	if (err) ErrorHandler_Fail2(err, "Clearing Pasteboard");
	PasteboardSynchronize(pbRef);

	len = Platform_ConvertString(str, value);
	CFDataCreate(NULL, str, len);
	if (!cfData) ErrorHandler_Fail("CFDataCreate() returned null pointer");

	PasteboardPutItemFlavor(pbRef, 1, FMT_UTF8, cfData, 0);
}
/* TODO: IMPLEMENT void Window_SetIcon(Bitmap* bmp); */

bool Window_GetVisible(void) { return IsWindowVisible(win_handle); }
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

int Window_GetWindowState(void) {
	if (win_state == WINDOW_STATE_FULLSCREEN)
		return WINDOW_STATE_FULLSCREEN;
	if (IsWindowCollapsed(win_handle))
		return WINDOW_STATE_MINIMISED;
	if (IsWindowInStandardState(win_handle, NULL, NULL))
		return WINDOW_STATE_MAXIMISED;
	return WINDOW_STATE_NORMAL;
}
void Window_SetWindowState(int state) {
	int old_state = Window_GetWindowState();
	OSStatus err;

	if (state == old_state) return;
	win_state = state;

	if (old_state == WINDOW_STATE_FULLSCREEN) {
		ctx_pendingWindowed = true;
		/* When returning from full screen, wait until the context is updated to actually do the work. */
		return;
	}
	if (old_state == WINDOW_STATE_MINIMISED) {
		err = CollapseWindow(win_handle, false);
		if (err) ErrorHandler_Fail2(err, "Un-minimising window");
	}
	Window_UpdateWindowState();
}

void Window_SetBounds(Rect2D rect) {
	Window_SetLocation(rect.X, rect.Y);
	Window_SetSize(rect.Width, rect.Height);
}

void Window_SetLocation(int x, int y) {
	MoveWindow(win_handle, x, y, false);
}

void Window_SetSize(int width, int height) {
	/* SizeWindow works in client size */
	/* But SetSize is window size, so reduce it */
	width  -= (Window_Bounds.Width  - Window_ClientSize.Width);
	height -= (Window_Bounds.Height - Window_ClientSize.Height);
	
	SizeWindow(win_handle, width, height, true);
}

void Window_SetClientSize(int width, int height) {
	SizeWindow(win_handle, width, height, true);
}

void Window_Close(void) {
	Event_RaiseVoid(&WindowEvents_Closed);
	/* TODO: Does this raise the event twice? */
	Window_Destroy();
}

void Window_ProcessEvents(void) {
	EventRef theEvent;
	EventTargetRef target = GetEventDispatcherTarget();
	OSStatus res;
	
	for (;;) {
		res = ReceiveNextEvent(0, NULL, 0.0, true, &theEvent);
		if (res == eventLoopTimedOutErr) break;
		
		if (res) {
			Platform_Log1("Message Loop status: %i", &res); break;
		}
		if (!theEvent) break;
		
		SendEventToEventTarget(theEvent, target);
		ReleaseEvent(theEvent);
	}
}

Point2D Window_PointToClient(int x, int y) {
	Rect r;
	Point2D p;
	GetWindowBounds(win_handle, kWindowContentRgn, &r);
	
	p.X = x - r.left; p.Y = y - r.top;
	return p;
}

Point2D Window_PointToScreen(int x, int y) {
	Rect r;
	Point2D p;
	GetWindowBounds(win_handle, kWindowContentRgn, &r);
	
	p.X = x + r.left; p.Y = y + r.top;
	return p;
}

Point2D Window_GetScreenCursorPos(void) {
	HIPoint point;
	Point2D p;
	/* NOTE: HIGetMousePosition is OSX 10.5 or later */
	/* TODO: Use GetGlobalMouse instead!!!! */
	HIGetMousePosition(kHICoordSpaceScreenPixel, NULL, &point);
	
	p.X = (int)point.x; p.Y = (int)point.y;
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
AGLContext ctx_handle;
bool ctx_fullscreen, ctx_firstFullscreen;
Rect2D ctx_windowedBounds;

static void GLContext_Check(int code, const char* place) {
	ReturnCode res;
	if (code) return;

	res = aglGetError();
	if (res) ErrorHandler_Fail2(res, place);
}

static void GLContext_MakeCurrent(void) {
	int code = aglSetCurrentContext(ctx_handle);
	GLContext_Check(code, "Setting GL context");
}

static void GLContext_SetDrawable(void) {
	CGrafPtr windowPort = GetWindowPort(win_handle);
	int code = aglSetDrawable(ctx_handle, windowPort);
	GLContext_Check(code, "Attaching GL context");
}

static void GLContext_GetAttribs(struct GraphicsMode* mode, int* attribs, bool fullscreen) {
	int i = 0;

	if (!mode->IsIndexed) { attribs[i++] = AGL_RGBA; }
	attribs[i++] = AGL_RED_SIZE;   attribs[i++] = mode->R;
	attribs[i++] = AGL_GREEN_SIZE; attribs[i++] = mode->G;
	attribs[i++] = AGL_BLUE_SIZE;  attribs[i++] = mode->B;
	attribs[i++] = AGL_ALPHA_SIZE; attribs[i++] = mode->A;

	if (mode->DepthBits) {
		attribs[i++] = AGL_DEPTH_SIZE;   attribs[i++] = mode->DepthBits;
	}
	if (mode->StencilBits) {
		attribs[i++] = AGL_STENCIL_SIZE; attribs[i++] = mode->StencilBits;
	}

	if (mode->Buffers > 1) { attribs[i++] = AGL_DOUBLEBUFFER; }
	if (fullscreen)        { attribs[i++] = AGL_FULLSCREEN; }

	attribs[i++] = 0;
}

static void GLContext_UnsetFullscreen(void) {
	int code;
	Platform_LogConst("Unsetting AGL fullscreen.");

	code = aglSetDrawable(ctx_handle, NULL);
	GLContext_Check(code, "Unattaching GL context");
	code = aglUpdateContext(ctx_handle);
	GLContext_Check(code, "Updating GL context (from Fullscreen)");

	CGDisplayRelease(CGMainDisplayID());
	GLContext_SetDrawable();

	ctx_fullscreen = false;
	Window_UpdateWindowState();
	Window_SetSize(ctx_windowedBounds.Width, ctx_windowedBounds.Height);
}

static void GLContext_SetFullscreen(void) {
	int displayWidth  = DisplayDevice_Default.Bounds.Width;
	int displayHeight = DisplayDevice_Default.Bounds.Height;
	int code;

	Platform_LogConst("Switching to AGL fullscreen");
	CGDisplayCapture(CGMainDisplayID());

	code = aglSetFullScreen(ctx_handle, displayWidth, displayHeight, 0, 0);
	GLContext_Check(code, "aglSetFullScreen");
	GLContext_MakeCurrent();

	/* This is a weird hack to workaround a bug where the first time a context */
	/* is made fullscreen, we just end up with a blank screen.  So we undo it as fullscreen */
	/* and redo it as fullscreen. */
	if (!ctx_firstFullscreen) {
		ctx_firstFullscreen = true;
		GLContext_UnsetFullscreen();
		GLContext_SetFullscreen();
		return;
	}

	ctx_fullscreen     = true;
	ctx_windowedBounds = Window_Bounds;

	Window_ClientSize.Width = displayWidth;
	Window_ClientSize.Width = displayHeight;

	Window_Bounds = DisplayDevice_Default.Bounds;
	win_state     = WINDOW_STATE_FULLSCREEN;
}

void GLContext_Init(struct GraphicsMode* mode) {
	int attribs[20];
	AGLPixelFormat fmt;
	GDHandle gdevice;
	OSStatus res;

	/* Initially try creating fullscreen compatible context */	
	res = DMGetGDeviceByDisplayID(CGMainDisplayID(), &gdevice, false);
	if (res) ErrorHandler_Fail2(res, "Getting display device failed");

	GLContext_GetAttribs(mode, attribs, true);
	fmt = aglChoosePixelFormat(&gdevice, 1, attribs);
	res = aglGetError();

	/* Try again with non-compatible context if that fails */
	if (!fmt || res == AGL_BAD_PIXELFMT) {
		Platform_LogConst("Failed to create full screen pixel format.");
		Platform_LogConst("Trying again to create a non-fullscreen pixel format.");

		GLContext_GetAttribs(mode, attribs, false);
		fmt = aglChoosePixelFormat(NULL, 0, attribs);
		res = aglGetError();
	}
	if (res) ErrorHandler_Fail2(res, "Choosing pixel format");

	ctx_handle = aglCreateContext(fmt, NULL);
	GLContext_Check(0, "Creating GL context");

	aglDestroyPixelFormat(fmt);
	GLContext_Check(0, "Destroying pixel format");

	GLContext_SetDrawable();
	GLContext_Update();
	GLContext_MakeCurrent();
}

void GLContext_Update(void) {
	if (ctx_pendingFullscreen) {
		ctx_pendingFullscreen = false;
		GLContext_SetFullscreen();
		return;
	} else if (ctx_pendingWindowed) {
		ctx_pendingWindowed = false;
		GLContext_UnsetFullscreen();
	}

	if (ctx_fullscreen) return;
	GLContext_SetDrawable();
	aglUpdateContext(ctx_handle);
}

void GLContext_Free(void) {
	int code;
	if (!ctx_handle) return;

	code = aglSetCurrentContext(NULL);
	GLContext_Check(code, "Unsetting GL context");

	code = aglDestroyContext(ctx_handle);
	GLContext_Check(code, "Destroying GL context");
	ctx_handle = NULL;
}

void* GLContext_GetAddress(const char* function) {
	/* TODO: Apparently we don't need this for OSX */
	return NULL;
}

void GLContext_SwapBuffers(void) {
	aglSwapBuffers(ctx_handle);
	GLContext_Check(0, "Swapping buffers");
}

void GLContext_SetVSync(bool enabled) {
	int value = enabled ? 1 : 0;
	aglSetInteger(ctx_handle, AGL_SWAP_INTERVAL, &value);
}
#endif
