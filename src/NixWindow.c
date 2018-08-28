#include "Window.h"
#if CC_BUILD_NIX
#include "DisplayDevice.h"
#include "ErrorHandler.h"
#include "Input.h"
#include "Funcs.h"
#include "Platform.h"
#include "Event.h"
#include "Stream.h"
#include <X11/Xlib.h>
#include <GL/glx.h>

#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD    1
#define _NET_WM_STATE_TOGGLE 2

Display* win_display;
int win_screen;
Window win_rootWin;

Window win_handle;
XVisualInfo win_visual;
Int32 borderLeft, borderRight, borderTop, borderBottom;
bool win_isExiting;
 
Atom wm_destroy, net_wm_state;
Atom net_wm_state_minimized;
Atom net_wm_state_fullscreen;
Atom net_wm_state_maximized_horizontal;
Atom net_wm_state_maximized_vertical;
Atom net_wm_icon, net_frame_extents;

Atom xa_clipboard, xa_targets, xa_utf8_string, xa_data_sel;
Atom xa_atom = 4, xa_cardinal = 6;
long win_eventMask;

static Key Window_MapKey(KeySym key) {
	if (key >= XK_F1 && key <= XK_F35) { return Key_F1 + (key - XK_F1); }
	if (key >= XK_0 && key <= XK_9) { return Key_0 + (key - XK_0); }
	if (key >= XK_A && key <= XK_Z) { return Key_A + (key - XK_A); }
	if (key >= XK_a && key <= XK_z) { return Key_A + (key - XK_a); }

	if (key >= XK_KP_0 && key <= XK_KP_9) {
		return Key_Keypad0 + (key - XK_KP_9);
	}

	switch (key) {
		case XK_Escape: return Key_Escape;
		case XK_Return: return Key_Enter;
		case XK_space: return Key_Space;
		case XK_BackSpace: return Key_BackSpace;

		case XK_Shift_L: return Key_ShiftLeft;
		case XK_Shift_R: return Key_ShiftRight;
		case XK_Alt_L: return Key_AltLeft;
		case XK_Alt_R: return Key_AltRight;
		case XK_Control_L: return Key_ControlLeft;
		case XK_Control_R: return Key_ControlRight;
		case XK_Super_L: return Key_WinLeft;
		case XK_Super_R: return Key_WinRight;
		case XK_Meta_L: return Key_WinLeft;
		case XK_Meta_R: return Key_WinRight;

		case XK_Menu: return Key_Menu;
		case XK_Tab: return Key_Tab;
		case XK_minus: return Key_Minus;
		case XK_plus: return Key_Plus;
		case XK_equal: return Key_Plus;

		case XK_Caps_Lock: return Key_CapsLock;
		case XK_Num_Lock: return Key_NumLock;

		case XK_Pause: return Key_Pause;
		case XK_Break: return Key_Pause;
		case XK_Scroll_Lock: return Key_Pause;
		case XK_Insert: return Key_PrintScreen;
		case XK_Print: return Key_PrintScreen;
		case XK_Sys_Req: return Key_PrintScreen;

		case XK_backslash: return Key_BackSlash;
		case XK_bar: return Key_BackSlash;
		case XK_braceleft: return Key_BracketLeft;
		case XK_bracketleft: return Key_BracketLeft;
		case XK_braceright: return Key_BracketRight;
		case XK_bracketright: return Key_BracketRight;
		case XK_colon: return Key_Semicolon;
		case XK_semicolon: return Key_Semicolon;
		case XK_quoteright: return Key_Quote;
		case XK_quotedbl: return Key_Quote;
		case XK_quoteleft: return Key_Tilde;
		case XK_asciitilde: return Key_Tilde;

		case XK_comma: return Key_Comma;
		case XK_less: return Key_Comma;
		case XK_period: return Key_Period;
		case XK_greater: return Key_Period;
		case XK_slash: return Key_Slash;
		case XK_question: return Key_Slash;

		case XK_Left: return Key_Left;
		case XK_Down: return Key_Down;
		case XK_Right: return Key_Right;
		case XK_Up: return Key_Up;

		case XK_Delete: return Key_Delete;
		case XK_Home: return Key_Home;
		case XK_End: return Key_End;
		case XK_Page_Up: return Key_PageUp;
		case XK_Page_Down: return Key_PageDown;

		case XK_KP_Add: return Key_KeypadAdd;
		case XK_KP_Subtract: return Key_KeypadSubtract;
		case XK_KP_Multiply: return Key_KeypadMultiply;
		case XK_KP_Divide: return Key_KeypadDivide;
		case XK_KP_Decimal: return Key_KeypadDecimal;
		case XK_KP_Insert: return Key_Keypad0;
		case XK_KP_End: return Key_Keypad1;
		case XK_KP_Down: return Key_Keypad2;
		case XK_KP_Page_Down: return Key_Keypad3;
		case XK_KP_Left: return Key_Keypad4;
		case XK_KP_Right: return Key_Keypad6;
		case XK_KP_Home: return Key_Keypad7;
		case XK_KP_Up: return Key_Keypad8;
		case XK_KP_Page_Up: return Key_Keypad9;
		case XK_KP_Delete: return Key_KeypadDecimal;
		case XK_KP_Enter: return Key_KeypadEnter;
	}
	return Key_None;
}

void Window_RegisterAtoms(void) {
	Display* display = win_display;
	wm_destroy = XInternAtom(display, "WM_DELETE_WINDOW", true);
	net_wm_state = XInternAtom(display, "_NET_WM_STATE", false);
	net_wm_state_minimized = XInternAtom(display, "_NET_WM_STATE_MINIMIZED", false);
	net_wm_state_fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", false);
	net_wm_state_maximized_horizontal = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", false);
	net_wm_state_maximized_vertical = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", false);
	net_wm_icon = XInternAtom(display, "_NEW_WM_ICON", false);
	net_frame_extents = XInternAtom(display, "_NET_FRAME_EXTENTS", false);

	xa_clipboard = XInternAtom(display, "CLIPBOARD", false);
	xa_targets = XInternAtom(display, "TARGETS", false);
	xa_utf8_string = XInternAtom(display, "UTF8_STRING", false);
	xa_data_sel = XInternAtom(display, "CS_SEL_DATA", false);
}

static XVisualInfo GLContext_SelectVisual(struct GraphicsMode* mode);
void Window_Create(Int32 x, Int32 y, Int32 width, Int32 height, STRING_REF String* title, 
	struct GraphicsMode* mode, struct DisplayDevice* device) {
	win_display = DisplayDevice_Meta[0];
	win_screen  = DisplayDevice_Meta[1];
	win_rootWin = DisplayDevice_Meta[2];

	/* Open a display connection to the X server, and obtain the screen and root window */
	UInt64 addr = (UInt64)win_display;
	Platform_Log3("Display: %y, Screen %i, Root window: %y", &addr, &win_screen, &win_rootWin);

	Window_RegisterAtoms();
	win_visual = GLContext_SelectVisual(mode);
	Platform_LogConst("Opening render window... ");

	XSetWindowAttributes attributes = { 0 };
	attributes.colormap = XCreateColormap(win_display, win_rootWin, win_visual.visual, AllocNone);

	win_eventMask = StructureNotifyMask /*| SubstructureNotifyMask*/ | ExposureMask |
		KeyReleaseMask | KeyPressMask | KeymapStateMask | PointerMotionMask |
		FocusChangeMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask |
		LeaveWindowMask | PropertyChangeMask;
	attributes.event_mask = win_eventMask;

	long mask = CWColormap | CWEventMask | CWBackPixel | CWBorderPixel;
	win_handle = XCreateWindow(win_display, win_rootWin, x, y, width, height,
		0, win_visual.depth /* CopyFromParent*/, InputOutput, win_visual.visual, mask, &attributes);

	if (!win_handle) ErrorHandler_Fail("XCreateWindow call failed");
	char str[600]; Platform_ConvertString(str, title);
	XStoreName(win_display, win_handle, str);

	XSizeHints hints = { 0 };
	hints.base_width = width;
	hints.base_height = height;
	hints.flags = PSize | PPosition;
	XSetWMNormalHints(win_display, win_handle, &hints);

	/* Register for window destroy notification */
	Atom atoms[1] = { wm_destroy };
	XSetWMProtocols(win_display, win_handle, atoms, 1);

	/* Set the initial window size to ensure X, Y, Width, Height and the rest
	   return the correct values inside the constructor and the Load event. */
	XEvent e = { 0 };
	e.xconfigure.x = x;
	e.xconfigure.y = y;
	e.xconfigure.width = width;
	e.xconfigure.height = height;
	Window_RefreshBounds(&e);

	/* Request that auto-repeat is only set on devices that support it physically.
	   This typically means that it's turned off for keyboards (which is what we want).
	   We prefer this method over XAutoRepeatOff/On, because the latter needs to
	   be reset before the program exits. */
	bool supported;
	XkbSetDetectableAutoRepeat(win_display, true, &supported);
	Window_Exists = true;
}

char clipboard_copy_buffer[String_BufferSize(256)];
String clipboard_copy_text = String_FromArray(clipboard_copy_buffer);
char clipboard_paste_buffer[String_BufferSize(256)];
String clipboard_paste_text = String_FromArray(clipboard_paste_buffer);

void Window_GetClipboardText(STRING_TRANSIENT String* value) {
	Window owner = XGetSelectionOwner(win_display, xa_clipboard);
	if (!owner) return; /* no window owner */

	XConvertSelection(win_display, xa_clipboard, xa_utf8_string, xa_data_sel, win_handle, 0);
	clipboard_paste_text.length = 0;
	Int32 i;

	/* wait up to 1 second for SelectionNotify event to arrive */
	for (i = 0; i < 10; i++) {
		Window_ProcessEvents();
		if (clipboard_paste_text.length) {
			String_Set(value, &clipboard_paste_text);
			return;
		} else {
			Thread_Sleep(100);
		}
	}
}

void Window_SetClipboardText(STRING_PURE String* value) {
	String_Set(&clipboard_paste_text, value);
	XSetSelectionOwner(win_display, xa_clipboard, win_handle, 0);
}

bool win_visible;
bool Window_GetVisible(void) { return win_visible; }

void Window_SetVisible(bool visible) {
	if (visible == win_visible) return;
	if (visible) {
		XMapWindow(win_display, win_handle);
	} else {
		XUnmapWindow(win_display, win_handle);
	}
}

void* Window_GetWindowHandle(void) { return win_handle; }

UInt8 Window_GetWindowState(void) {
	Atom prop_type;
	long items, bytes_after;
	int prop_format;
	Atom* data = NULL;

	XGetWindowProperty(win_display, win_handle,
		net_wm_state, 0, 256, false, xa_atom, &prop_type,
		&prop_format, &items, &bytes_after, &data);

	bool fullscreen = false, minimised = false;
	Int32 maximised = 0, i;

	/* TODO: Check this works right */
	if (data && items) {
		for (i = 0; i < items; i++) {
			Atom atom = data[i];

			if (atom == net_wm_state_maximized_horizontal ||
				atom == net_wm_state_maximized_vertical) {
				maximised++;
			} else if (atom == net_wm_state_minimized) {
				minimised = true;
			} else if (atom == net_wm_state_fullscreen) {
				fullscreen = true;
			}
		}
	}
	if (data) XFree(data);

	if (minimised)      return WINDOW_STATE_MINIMISED;
	if (maximised == 2) return WINDOW_STATE_MAXIMISED;
	if (fullscreen)     return WINDOW_STATE_FULLSCREEN;
	return WINDOW_STATE_NORMAL;
}

void Window_SendNetWMState(long op, Atom a1, Atom a2) {
	XEvent ev = { 0 };
	ev.xclient.type = ClientMessage;
	ev.xclient.send_event = true;
	ev.xclient.window = win_handle;
	ev.xclient.message_type = net_wm_state;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = op;
	ev.xclient.data.l[1] = a1;
	ev.xclient.data.l[2] = a2;

	XSendEvent(win_display, win_rootWin, false,
		SubstructureRedirectMask | SubstructureNotifyMask, &ev);
}

void Window_SetWindowState(UInt8 state) {
	UInt8 current_state = Window_GetWindowState();
	if (current_state == state) return;

	/* Reset the current window state */
	if (current_state == WINDOW_STATE_MINIMISED) {
		XMapWindow(win_display, win_handle);
	} else if (current_state == WINDOW_STATE_FULLSCREEN) {
		Window_SendNetWMState(_NET_WM_STATE_REMOVE, net_wm_state_fullscreen, 0);
	} else if (current_state == WINDOW_STATE_MAXIMISED) {
		Window_SendNetWMState(_NET_WM_STATE_TOGGLE, net_wm_state_maximized_horizontal, 
			net_wm_state_maximized_vertical);
	}

	XSync(win_display, false);

	switch (state) {
	case WINDOW_STATE_NORMAL:
		XRaiseWindow(win_display, win_handle);
		break;

	case WINDOW_STATE_MAXIMISED:
		Window_SendNetWMState(_NET_WM_STATE_ADD, net_wm_state_maximized_horizontal,
			net_wm_state_maximized_vertical);
		XRaiseWindow(win_display, win_handle);
		break;

	case WINDOW_STATE_MINIMISED:
		/* TODO: multiscreen support */
		XIconifyWindow(win_display, win_handle, win_screen);
		break;

	case WINDOW_STATE_FULLSCREEN:
		Window_SendNetWMState(_NET_WM_STATE_ADD, net_wm_state_fullscreen, 0);
		XRaiseWindow(win_display, win_handle);
		break;
	}
	Window_ProcessEvents();
}

void Window_SetBounds(struct Rectangle2D rect) {
	Int32 width  = rect.Width  - borderLeft - borderRight;
	Int32 height = rect.Height - borderTop  - borderBottom;
	XMoveResizeWindow(win_display, win_handle, rect.X, rect.Y,
		max(width, 1), max(height, 1));
	Window_ProcessEvents();
}

void Window_SetLocation(struct Point2D point) {
	XMoveWindow(win_display, win_handle, point.X, point.Y);
	Window_ProcessEvents();
}

void Window_SetSize(struct Size2D size) {
	Int32 width  = size.Width  - borderLeft - borderRight;
	Int32 height = size.Height - borderTop  - borderBottom;
	XResizeWindow(win_display, win_handle, 
		max(width, 1), max(height, 1));
	Window_ProcessEvents();
}

void Window_SetClientSize(struct Size2D size) {
	XResizeWindow(win_display, win_handle, size.Width, size.Height);
	Window_ProcessEvents();
}

void Window_Close(void) {
	XEvent ev = { 0 };
	ev.type = ClientMessage;
	ev.xclient.format = 32;
	ev.xclient.display = win_display;
	ev.xclient.window = win_handle;
	ev.xclient.data.l[0] = wm_destroy;

	XSendEvent(win_display, win_handle, false, 0, &ev);
	XFlush(win_display);
}

void Window_Destroy(void) {
	XSync(win_display, true);
	XDestroyWindow(win_display, win_handle);
	Window_Exists = false;
}

void Window_RefreshBorders(void) {
	Atom prop_type;
	int prop_format;
	long items, bytes_after;
	long* borders = NULL;
	
	XGetWindowProperty(win_display, win_handle, net_frame_extents, 0, 16, false,
		xa_cardinal, &prop_type, &prop_format, &items, &bytes_after, &borders);

	if (!borders) return;
	if (items == 4) {
		borderLeft = borders[0]; borderRight = borders[1];
		borderTop = borders[2]; borderBottom = borders[3];
	}
	XFree(borders);
}

void Window_RefreshBounds(XEvent* e) {
	Window_RefreshBorders();

	struct Point2D curLoc = Window_GetLocation();
	struct Point2D loc = { e->xconfigure.x - borderLeft, e->xconfigure.y - borderTop };
	if (loc.X != curLoc.X || loc.Y != curLoc.Y) {
		Window_Bounds.X = loc.X; Window_Bounds.Y = loc.Y;
		Event_RaiseVoid(&WindowEvents_Moved);
	}

	/* Note: width and height denote the internal (client) size.
	   To get the external (window) size, we need to add the border size. */
	struct Size2D curSize = Window_GetSize();
	struct Size2D size = {
		e->xconfigure.width + borderLeft + borderRight,
		e->xconfigure.height + borderTop + borderBottom };

	if (curSize.Width != size.Width || curSize.Height != size.Height) {
		Window_Bounds.Width = size.Width; Window_Bounds.Height = size.Height;
		Window_ClientSize = Size2D_Make(e->xconfigure.width, e->xconfigure.height);
		Event_RaiseVoid(&WindowEvents_Resized);
	}
}

void Window_ToggleKey(XKeyEvent* keyEvent, bool pressed) {
	KeySym keysym1 = XLookupKeysym(keyEvent, 0);
	KeySym keysym2 = XLookupKeysym(keyEvent, 1);

	Key key = Window_MapKey(keysym1);
	if (key == Key_None) key = Window_MapKey(keysym2);
	if (key != Key_None) Key_SetPressed(key, pressed);
}

Atom Window_GetSelectionProperty(XEvent* e) {
	Atom prop = e->xselectionrequest.property;
	if (prop) return prop;

	/* For obsolete clients. See ICCCM spec, selections chapter for reasoning. */
	return e->xselectionrequest.target;
}

bool Window_GetPendingEvent(XEvent* e) {
	return XCheckWindowEvent(win_display, win_handle, win_eventMask, e) ||
		XCheckTypedWindowEvent(win_display, win_handle, ClientMessage, e) ||
		XCheckTypedWindowEvent(win_display, win_handle, SelectionNotify, e) ||
		XCheckTypedWindowEvent(win_display, win_handle, SelectionRequest, e);
}

void Window_ProcessEvents(void) {
	XEvent e;
	while (Window_Exists) {
		if (!Window_GetPendingEvent(&e)) break;

		switch (e.type) {
		case MapNotify:
		case UnmapNotify:
		{
			bool wasVisible = win_visible;
			win_visible = e.type == MapNotify;
			if (win_visible != wasVisible) {
				Event_RaiseVoid(&WindowEvents_VisibilityChanged);
			}
		} break;

		case ClientMessage:
			if (!win_isExiting && e.xclient.data.l[0] == wm_destroy) {
				Platform_LogConst("Exit message received.");
				Event_RaiseVoid(&WindowEvents_Closing);

				win_isExiting = true;
				Window_Destroy();
				Event_RaiseVoid(&WindowEvents_Closed);
			} break;

		case DestroyNotify:
			Platform_LogConst("Window destroyed");
			Window_Exists = false;
			break;

		case ConfigureNotify:
			Window_RefreshBounds(&e);
			break;

		case Expose:
			if (e.xexpose.count == 0) {
				Event_RaiseVoid(&WindowEvents_Redraw);
			}
			break;

		case KeyPress:
		{
			Window_ToggleKey(&e.xkey, true);
			UInt8 data[16];
			int status = XLookupString(&e.xkey, data, Array_Elems(data), NULL, NULL);

			/* TODO: Does this work for every non-english layout? works for latin keys (e.g. finnish) */
			UInt8 raw; Int32 i;
			for (i = 0; i < status; i++) {
				if (!Convert_TryUnicodeToCP437(data[i], &raw)) continue;
				Event_RaiseInt(&KeyEvents_Press, raw);
			}
		} break;

		case KeyRelease:
			/* TODO: raise KeyPress event. Use code from */
			/* http://anonsvn.mono-project.com/viewvc/trunk/mcs/class/Managed.Windows.Forms/System.Windows.Forms/X11Keyboard.cs?view=markup */
			Window_ToggleKey(&e.xkey, false);
			break;

		case ButtonPress:
			if (e.xbutton.button == 1)      Mouse_SetPressed(MouseButton_Left,   true);
			else if (e.xbutton.button == 2) Mouse_SetPressed(MouseButton_Middle, true);
			else if (e.xbutton.button == 3) Mouse_SetPressed(MouseButton_Right,  true);
			else if (e.xbutton.button == 4) Mouse_SetWheel(Mouse_Wheel + 1);
			else if (e.xbutton.button == 5) Mouse_SetWheel(Mouse_Wheel - 1);
			else if (e.xbutton.button == 6) Key_SetPressed(Key_XButton1, true);
			else if (e.xbutton.button == 7) Key_SetPressed(Key_XButton2, true);
			break;

		case ButtonRelease:
			if (e.xbutton.button == 1)      Mouse_SetPressed(MouseButton_Left, false);
			else if (e.xbutton.button == 2) Mouse_SetPressed(MouseButton_Middle, false);
			else if (e.xbutton.button == 3) Mouse_SetPressed(MouseButton_Right,  false);
			else if (e.xbutton.button == 6) Key_SetPressed(Key_XButton1, false);
			else if (e.xbutton.button == 7) Key_SetPressed(Key_XButton2, false);
			break;

		case MotionNotify:
			Mouse_SetPosition(e.xmotion.x, e.xmotion.y);
			break;

		case FocusIn:
		case FocusOut:
		{
			bool wasFocused = Window_Focused;
			Window_Focused = e.type == FocusIn;
			if (Window_Focused != wasFocused) {
				Event_RaiseVoid(&WindowEvents_FocusChanged);
			}
		} break;

		case MappingNotify:
			/* 0 == MappingModifier, 1 == MappingKeyboard */
			if (e.xmapping.request == 0 || e.xmapping.request == 1) {
				Platform_LogConst("keybard mapping refreshed");
				XRefreshKeyboardMapping(&e.xmapping);
			}
			break;

		case PropertyNotify:
			if (e.xproperty.atom == net_wm_state) {
				Event_RaiseVoid(&WindowEvents_WindowStateChanged);
			}

			/*if (e.xproperty.atom == net_frame_extents) {
			     RefreshWindowBorders();
			}*/
			break;

		case SelectionNotify:
			clipboard_paste_text.length = 0;

			if (e.xselection.selection == xa_clipboard && e.xselection.target == xa_utf8_string && e.xselection.property == xa_data_sel) {
				Atom prop_type;
				int prop_format;
				long items, bytes_after;
				UInt8* data = NULL;			

				XGetWindowProperty(win_display, win_handle, xa_data_sel, 0, 1024, false, 0,
					&prop_type, &prop_format, &items, &bytes_after, &data);
				XDeleteProperty(win_display, win_handle, xa_data_sel);

				if (data && items && prop_type == xa_utf8_string) {
					clipboard_paste_text.length = 0;
					String_DecodeUtf8(&clipboard_paste_text, data, items);
				}
				if (data) XFree(data);
			}
			break;

		case SelectionRequest:
		{
			XEvent reply = { 0 };
			reply.xselection.type = SelectionNotify;
			reply.xselection.send_event = true;
			reply.xselection.display = win_display;
			reply.xselection.requestor = e.xselectionrequest.requestor;
			reply.xselection.selection = e.xselectionrequest.selection;
			reply.xselection.target = e.xselectionrequest.target;
			reply.xselection.property = NULL;
			reply.xselection.time = e.xselectionrequest.time;

			if (e.xselectionrequest.selection == xa_clipboard && e.xselectionrequest.target == xa_utf8_string && clipboard_copy_text.length) {
				reply.xselection.property = Window_GetSelectionProperty(&e);
				UInt8 data[1024];
				Int32 i, len = 0;

				for (i = 0; i < clipboard_copy_text.length; i++) {
					UInt16 codepoint = Convert_CP437ToUnicode(clipboard_copy_text.buffer[i]);
					UInt8* cur = data + len;
					len += Stream_WriteUtf8(cur, codepoint);
				}

				XChangeProperty(win_display, reply.xselection.requestor, reply.xselection.property, xa_utf8_string, 8,
					PropModeReplace, data, len);
			} else if (e.xselectionrequest.selection == xa_clipboard && e.xselectionrequest.target == xa_targets) {
				reply.xselection.property = Window_GetSelectionProperty(&e);

				Atom data[2] = { xa_utf8_string, xa_targets };
				XChangeProperty(win_display, reply.xselection.requestor, reply.xselection.property, xa_atom, 32,
					PropModeReplace, data, 2);
			}
			XSendEvent(win_display, e.xselectionrequest.requestor, true, 0, &reply);
		} break;
		}
	}
}

struct Point2D Window_PointToClient(struct Point2D point) {
	int ox, oy;
	Window child;
	XTranslateCoordinates(win_display, win_rootWin, win_handle, point.X, point.Y, &ox, &oy, &child);
	return Point2D_Make(ox, oy);
}

struct Point2D Window_PointToScreen(struct Point2D point) {
	int ox, oy;
	Window child;
	XTranslateCoordinates(win_display, win_handle, win_rootWin, point.X, point.Y, &ox, &oy, &child);
	return Point2D_Make(ox, oy);
}

struct Point2D Window_GetDesktopCursorPos(void) {
	Window root, child;
	int rootX, rootY, childX, childY, mask;
	XQueryPointer(win_display, win_rootWin, &root, &child, &rootX, &rootY, &childX, &childY, &mask);
	return Point2D_Make(rootX, rootY);
}

void Window_SetDesktopCursorPos(struct Point2D point) {
	XWarpPointer(win_display, NULL, win_rootWin, 0, 0, 0, 0, point.X, point.Y);
	XFlush(win_display); /* TODO: not sure if XFlush call is necessary */
}

Cursor win_blankCursor;
bool win_cursorVisible = true;
bool Window_GetCursorVisible(void) { return win_cursorVisible; }
void Window_SetCursorVisible(bool visible) {
	win_cursorVisible = visible;
	if (visible) {
		XUndefineCursor(win_display, win_handle);
	} else {
		if (!win_blankCursor) {
			XColor col = { 0 };
			Pixmap pixmap = XCreatePixmap(win_display, win_rootWin, 1, 1, 1);
			win_blankCursor = XCreatePixmapCursor(win_display, pixmap, pixmap, &col, &col, 0, 0);
			XFreePixmap(win_display, pixmap);
		}
		XDefineCursor(win_display, win_handle, win_blankCursor);
	}
}

GLXContext ctx_Handle;
typedef int (*FN_GLXSWAPINTERVAL)(int interval);
FN_GLXSWAPINTERVAL glXSwapIntervalSGI;
bool ctx_supports_vSync;

void GLContext_Init(struct GraphicsMode mode) {
	ctx_Handle = glXCreateContext(win_display, &win_visual, NULL, true);

	if (!ctx_Handle) {
		Platform_LogConst("Context create failed. Trying indirect...");
		ctx_Handle = glXCreateContext(win_display, &win_visual, NULL, false);
	}
	if (!ctx_Handle) ErrorHandler_Fail("Failed to create context");

	if (!glXIsDirect(win_display, ctx_Handle)) {
		Platform_LogConst("== WARNING: Context is not direct ==");
	}
	if (!glXMakeCurrent(win_display, win_handle, ctx_Handle)) {
		ErrorHandler_Fail("Failed to make context current.");
	}

	glXSwapIntervalSGI = (FN_GLXSWAPINTERVAL)GLContext_GetAddress("glXSwapIntervalSGI");
	ctx_supports_vSync = glXSwapIntervalSGI != NULL;
}

void GLContext_Update(void) { }
void GLContext_Free(void) {
	if (!ctx_Handle) return;

	if (glXGetCurrentContext() == ctx_Handle) {
		glXMakeCurrent(win_display, NULL, NULL);
	}
	glXDestroyContext(win_display, ctx_Handle);
	ctx_Handle = NULL;
}

void* GLContext_GetAddress(const char* function) {
	void* address = glXGetProcAddress(function);
	return GLContext_IsInvalidAddress(address) ? NULL : address;
}

void GLContext_SwapBuffers(void) {
	glXSwapBuffers(win_display, win_handle);
}

void GLContext_SetVSync(bool enabled) {
	if (!ctx_supports_vSync) return;

	int result = glXSwapIntervalSGI(enabled);
	if (result != 0) {Platform_Log1("Set VSync failed, error: %i", &result); }
}

static void GLContext_GetAttribs(struct GraphicsMode mode, Int32* attribs) {
	Int32 i = 0;
	struct ColorFormat color = mode.Format;
	/* See http://www-01.ibm.com/support/knowledgecenter/ssw_aix_61/com.ibm.aix.opengl/doc/openglrf/glXChooseFBConfig.htm%23glxchoosefbconfig */
	/* See http://www-01.ibm.com/support/knowledgecenter/ssw_aix_71/com.ibm.aix.opengl/doc/openglrf/glXChooseVisual.htm%23b5c84be452rree */
	/* for the attribute declarations. Note that the attributes are different than those used in Glx.ChooseVisual */

	if (!color.IsIndexed) {
		attribs[i++] = GLX_RGBA;
	}
	attribs[i++] = GLX_RED_SIZE;   attribs[i++] = color.R;
	attribs[i++] = GLX_GREEN_SIZE; attribs[i++] = color.G;
	attribs[i++] = GLX_BLUE_SIZE;  attribs[i++] = color.B;
	attribs[i++] = GLX_ALPHA_SIZE; attribs[i++] = color.A;

	if (mode.DepthBits) {
		attribs[i++] = GLX_DEPTH_SIZE; attribs[i++] = mode.DepthBits;
	}
	if (mode.StencilBits) {
		attribs[i++] = GLX_STENCIL_SIZE; attribs[i++] = mode.StencilBits;
	}
	if (mode.Buffers > 1) {
		attribs[i++] = GLX_DOUBLEBUFFER;
	}
	attribs[i++] = 0;
}

static XVisualInfo GLContext_SelectVisual(struct GraphicsMode* mode) {
	Int32 attribs[20];
	GLContext_GetAttribs(*mode, attribs);
	Int32 major = 0, minor = 0, fbcount;
	if (!glXQueryVersion(win_display, &major, &minor)) {
		ErrorHandler_Fail("glXQueryVersion failed");
	}

	XVisualInfo* visual = NULL;
	if (major >= 1 && minor >= 3) {
		/* ChooseFBConfig returns an array of GLXFBConfig opaque structures */
		GLXFBConfig* fbconfigs = glXChooseFBConfig(win_display, win_screen, attribs, &fbcount);
		if (fbconfigs && fbcount) {
			/* Use the first GLXFBConfig from the fbconfigs array (best match) */
			visual = glXGetVisualFromFBConfig(win_display, *fbconfigs);
			XFree(fbconfigs);
		}
	}

	if (!visual) {
		Platform_LogConst("Falling back to glXChooseVisual.");
		visual = glXChooseVisual(win_display, win_screen, attribs);
	}
	if (!visual) {
		ErrorHandler_Fail("Requested GraphicsMode not available.");
	}

	XVisualInfo info = *visual;
	XFree(visual);
	return info;
}
#endif
