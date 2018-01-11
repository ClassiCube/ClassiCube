#include "Window.h"
#include <Windows.h>
#include "Platform.h"
#include "Input.h"
#include "Event.h"
#include "String.h"

#define win_Style WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN
#define win_StyleEx WS_EX_WINDOWEDGE | WS_EX_APPWINDOW
#define win_ClassName "ClassiCube_Window"
#define RECT_WIDTH(rect) (rect.right - rect.left)
#define RECT_HEIGHT(rect) (rect.bottom - rect.top)

HINSTANCE win_Instance;
HWND win_Handle;
HDC win_DC;
UInt8 win_State = WINDOW_STATE_NORMAL;
bool win_Exists, win_Focused;
bool mouse_outside_window = true;
bool invisible_since_creation; // Set by WindowsMessage.CREATE and consumed by Visible = true (calls BringWindowToFront).
Int32 suppress_resize; // Used in WindowBorder and WindowState in order to avoid rapid, consecutive resize events.

Rectangle2D win_Bounds;
Rectangle2D win_ClientRect;
Rectangle2D previous_bounds; // Used to restore previous size when leaving fullscreen mode.

Rectangle2D Window_FromRect(RECT rect) {
	Rectangle2D r;
	r.X = rect.left; r.Y = rect.top;
	r.Width = RECT_WIDTH(rect);
	r.Height = RECT_HEIGHT(rect);
	return r;
}


void Window_Destroy(void) {
	if (!win_Exists) return;

	DestroyWindow(win_Handle);
	win_Exists = false;
}

void Window_ResetWindowState(void) {
	suppress_resize++;
	Window_SetWindowState(WINDOW_STATE_NORMAL);
	Window_ProcessEvents();
	suppress_resize--;
}

bool win_hiddenBorder;
void Window_DoSetHiddenBorder(bool value) {
	if (win_hiddenBorder == value) return;

	/* We wish to avoid making an invisible window visible just to change the border.
	However, it's a good idea to make a visible window invisible temporarily, to
	avoid garbage caused by the border change. */
	bool was_visible = Window_GetVisible();

	/* To ensure maximized/minimized windows work correctly, reset state to normal,
	change the border, then go back to maximized/minimized. */
	UInt8 state = win_State;
	Window_ResetWindowState();
	DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	style |= (value ? WS_POPUP : WS_OVERLAPPEDWINDOW);

	/* Make sure client size doesn't change when changing the border style.*/
	RECT rect;
	rect.left = win_Bounds.X; rect.top = win_Bounds.Y;
	rect.right = rect.left + win_Bounds.Width;
	rect.bottom = rect.top + win_Bounds.Height;
	AdjustWindowRectEx(&rect, style, false, win_StyleEx);

	/* This avoids leaving garbage on the background window. */
	if (was_visible) Window_SetVisible(false);

	SetWindowLongA(win_Handle, GWL_STYLE, style);
	SetWindowPos(win_Handle, NULL, 0, 0, RECT_WIDTH(rect), RECT_HEIGHT(rect),
		SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);

	/* Force window to redraw update its borders, but only if it's
	already visible (invisible windows will change borders when
	they become visible, so no need to make them visiable prematurely).*/
	if (was_visible) Window_SetVisible(true);

	Window_SetWindowState(state);
}

void Window_SetHiddenBorder(bool hidden) {
	suppress_resize++;
	Window_DoSetHiddenBorder(hidden);
	Window_ProcessEvents();
	suppress_resize--;
}

void Window_EnableMouseTracking(void) {
	TRACKMOUSEEVENT me;
	Platform_MemSet(&me, 0, sizeof(TRACKMOUSEEVENT));
	me.cbSize = sizeof(TRACKMOUSEEVENT);
	me.hwndTrack = win_Handle;
	me.dwFlags = TME_LEAVE;

	if (!TrackMouseEvent(&me)) {
		ErrorHandler_FailWithCode(GetLastError(), "Enabling mouse tracking");
	}
}

Key Window_MapKey(WPARAM key) {
	if (key >= VK_F1 && key <= VK_F24) {
		return Key_F1 + (key - VK_F1);
	}
	if (key >= '0' && key <= '9') {
		return Key_0 + (key - '0');
	}
	if (key >= 'A' && key <= 'Z') {
		return Key_A + (key - 'A');
	}
	if (key >= VK_NUMPAD0 && key <= VK_NUMPAD9) {
		return Key_Keypad0 + (key - VK_NUMPAD0);
	}

	switch (key) {
	case VK_ESCAPE: return Key_Escape;
	case VK_TAB: return Key_Tab;
	case VK_CAPITAL: return Key_CapsLock;
	case VK_LCONTROL: return Key_ControlLeft;
	case VK_LSHIFT: return Key_ShiftLeft;
	case VK_LWIN: return Key_WinLeft;
	case VK_LMENU: return Key_AltLeft;
	case VK_SPACE: return Key_Space;
	case VK_RMENU: return Key_AltRight;
	case VK_RWIN: return Key_WinRight;
	case VK_APPS: return Key_Menu;
	case VK_RCONTROL: return Key_ControlRight;
	case VK_RSHIFT: return Key_ShiftRight;
	case VK_RETURN: return Key_Enter;
	case VK_BACK: return Key_BackSpace;

	case VK_OEM_1: return Key_Semicolon;      /* Varies by keyboard: return ;: on Win2K/US */
	case VK_OEM_2: return Key_Slash;          /* Varies by keyboard: return /? on Win2K/US */
	case VK_OEM_3: return Key_Tilde;          /* Varies by keyboard: return `~ on Win2K/US */
	case VK_OEM_4: return Key_BracketLeft;    /* Varies by keyboard: return [{ on Win2K/US */
	case VK_OEM_5: return Key_BackSlash;      /* Varies by keyboard: return \| on Win2K/US */
	case VK_OEM_6: return Key_BracketRight;   /* Varies by keyboard: return ]} on Win2K/US */
	case VK_OEM_7: return Key_Quote;          /* Varies by keyboard: return '" on Win2K/US */
	case VK_OEM_PLUS: return Key_Plus;        /* Invariant: +							   */
	case VK_OEM_COMMA: return Key_Comma;      /* Invariant: : return					   */
	case VK_OEM_MINUS: return Key_Minus;      /* Invariant: -							   */
	case VK_OEM_PERIOD: return Key_Period;    /* Invariant: .							   */

	case VK_HOME: return Key_Home;
	case VK_END: return Key_End;
	case VK_DELETE: return Key_Delete;
	case VK_PRIOR: return Key_PageUp;
	case VK_NEXT: return Key_PageDown;
	case VK_PRINT: return Key_PrintScreen;
	case VK_PAUSE: return Key_Pause;
	case VK_NUMLOCK: return Key_NumLock;

	case VK_SCROLL: return Key_ScrollLock;
	case VK_SNAPSHOT: return Key_PrintScreen;
	case VK_INSERT: return Key_Insert;

	case VK_DECIMAL: return Key_KeypadDecimal;
	case VK_ADD: return Key_KeypadAdd;
	case VK_SUBTRACT: return Key_KeypadSubtract;
	case VK_DIVIDE: return Key_KeypadDivide;
	case VK_MULTIPLY: return Key_KeypadMultiply;

	case VK_UP: return Key_Up;
	case VK_DOWN: return Key_Down;
	case VK_LEFT: return Key_Left;
	case VK_RIGHT: return Key_Right;
	}
	return Key_Unknown;
}

LRESULT CALLBACK Window_Procedure(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) {
	bool new_focused_state;
	Real32 wheel_delta;
	WORD mouse_x, mouse_y;
	MouseButton btn;
	WINDOWPOS* pos;
	UInt8 new_state;
	UInt8 keyChar;
	bool pressed, extended, lShiftDown, rShiftDown;
	Key mappedKey;
	CREATESTRUCT* cs;

	switch (message) {

	case WM_ACTIVATE:
		new_focused_state = LOWORD(wParam) != 0;
		if (new_focused_state != win_Focused) {
			win_Focused = new_focused_state;
			Event_RaiseVoid(&WindowEvents_FocusChanged);
		}
		break;

	case WM_ENTERMENULOOP:
	case WM_ENTERSIZEMOVE:
	case WM_EXITMENULOOP:
	case WM_EXITSIZEMOVE:
		break;

	case WM_ERASEBKGND:
		return 1;

	case WM_WINDOWPOSCHANGED:
		pos = (WINDOWPOS*)lParam;
		if (pos->hwnd == win_Handle) {
			Point2D new_location = Point2D_Make(pos->x, pos->y);
			if (!Point2D_Equals(Window_GetLocation(), new_location)) {
				win_Bounds.X = pos->x; win_Bounds.Y = pos->y;
				Event_RaiseVoid(&WindowEvents_Moved);
			}

			Size2D new_size = Size2D_Make(pos->cx, pos->cy);
			if (!Size2D_Equals(Window_GetSize(), new_size)) {
				win_Bounds.Width = pos->cx; win_Bounds.Height = pos->cy;

				RECT rect;
				GetClientRect(handle, &rect);
				win_ClientRect = Window_FromRect(rect);

				SetWindowPos(win_Handle, NULL,
					win_Bounds.X, win_Bounds.Y, win_Bounds.Width, win_Bounds.Height,
					SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

				if (suppress_resize <= 0) {
					Event_RaiseVoid(&WindowEvents_Resized);
				}
			}
		}
		break;

	case WM_STYLECHANGED:
		if (wParam == GWL_STYLE) {
			DWORD style = ((STYLESTRUCT*)lParam)->styleNew;
			if ((style & WS_POPUP) != 0) {
				win_hiddenBorder = true;
			} else if ((style & WS_THICKFRAME) != 0) {
				win_hiddenBorder = false;
			}
		}
		break;

	case WM_SIZE:
		new_state = win_State;
		switch (wParam) {
		case SIZE_RESTORED: new_state = WINDOW_STATE_NORMAL; break;
		case SIZE_MINIMIZED: new_state = WINDOW_STATE_MINIMISED; break;
		case SIZE_MAXIMIZED: new_state = win_hiddenBorder ? WINDOW_STATE_FULLSCREEN : WINDOW_STATE_MAXIMISED; break;
		}

		if (new_state != win_State) {
			win_State = new_state;
			Event_RaiseVoid(&WindowEvents_WindowStateChanged);
		}
		break;


	case WM_CHAR:
		keyChar = Convert_UnicodeToCP437((UInt16)wParam);
		Event_RaiseInt32(&KeyEvents_Press, keyChar);
		break;

	case WM_MOUSEMOVE:
		mouse_x = LOWORD(lParam);
		mouse_y = HIWORD(lParam);
		Mouse_SetPosition(mouse_x, mouse_y);

		if (mouse_outside_window) {
			/* Once we receive a mouse move event, it means that the mouse has re-entered the window. */
			mouse_outside_window = false;
			Window_EnableMouseTracking();
			Event_RaiseVoid(&WindowEvents_MouseEnter);
		}
		break;

	case WM_MOUSELEAVE:
		mouse_outside_window = true;
		/* Mouse tracking is disabled automatically by the OS */
		Event_RaiseVoid(&WindowEvents_MouseLeave);

		/* Set all mouse buttons to off when user leaves window, prevents them being stuck down. */
		for (btn = 0; btn < MouseButton_Count; btn++) {
			Mouse_SetPressed(btn, false);
		}
		break;

	case WM_MOUSEWHEEL:
		wheel_delta = HIWORD(wParam) / (Real32)WHEEL_DELTA;
		Mouse_SetWheel(Mouse_Wheel + wheel_delta);
		return 0;

	case WM_LBUTTONDOWN:
		Mouse_SetPressed(MouseButton_Left, true);
		break;
	case WM_MBUTTONDOWN:
		Mouse_SetPressed(MouseButton_Middle, true);
		break;
	case WM_RBUTTONDOWN:
		Mouse_SetPressed(MouseButton_Right, true);
		break;
	case WM_XBUTTONDOWN:
		Mouse_SetPressed(HIWORD(wParam) == 1 ? MouseButton_Button1 : MouseButton_Button2, true);
		break;
	case WM_LBUTTONUP:
		Mouse_SetPressed(MouseButton_Left, false);
		break;
	case WM_MBUTTONUP:
		Mouse_SetPressed(MouseButton_Middle, false);
		break;
	case WM_RBUTTONUP:
		Mouse_SetPressed(MouseButton_Right, false);
		break;
	case WM_XBUTTONUP:
		Mouse_SetPressed(HIWORD(wParam) == 1 ? MouseButton_Button1 : MouseButton_Button2, false);
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		pressed = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;

		/* Shift/Control/Alt behave strangely when e.g. ShiftRight is held down and ShiftLeft is pressed
		and released. It looks like neither key is released in this case, or that the wrong key is
		released in the case of Control and Alt.
		To combat this, we are going to release both keys when either is released. Hacky, but should work.
		Win95 does not distinguish left/right key constants (GetAsyncKeyState returns 0).
		In this case, both keys will be reported as pressed.	*/

		extended = (lParam & (1UL << 24)) != 0;
		switch (wParam)
		{
		case VK_SHIFT:
			/* The behavior of this key is very strange. Unlike Control and Alt, there is no extended bit
			to distinguish between left and right keys. Moreover, pressing both keys and releasing one
			may result in both keys being held down (but not always).*/
			lShiftDown = (GetKeyState(VK_LSHIFT) >> 15) == 1;
			rShiftDown = (GetKeyState(VK_RSHIFT) >> 15) == 1;

			if (!pressed || lShiftDown != rShiftDown) {
				Key_SetPressed(Key_ShiftLeft, lShiftDown);
				Key_SetPressed(Key_ShiftRight, rShiftDown);
			}
			return 0;

		case VK_CONTROL:
			if (extended) {
				Key_SetPressed(Key_ControlRight, pressed);
			} else {
				Key_SetPressed(Key_ControlLeft, pressed);
			}
			return 0;

		case VK_MENU:
			if (extended) {
				Key_SetPressed(Key_AltRight, pressed);
			} else {
				Key_SetPressed(Key_AltLeft, pressed);
			}
			return 0;

		case VK_RETURN:
			if (extended) {
				Key_SetPressed(Key_KeypadEnter, pressed);
			} else {
				Key_SetPressed(Key_Enter, pressed);
			}
			return 0;

		default:
			mappedKey = Window_MapKey(wParam);
			if (mappedKey != Key_Unknown) {
				Key_SetPressed(mappedKey, pressed);
			}
			return 0;
		}
		break;

	case WM_SYSCHAR:
		return 0;

	case WM_KILLFOCUS:
		Key_Clear();
		break;


	case WM_CREATE:
		cs = (CREATESTRUCT*)lParam;
		if (cs->hwndParent == NULL) {
			win_Bounds.X = cs->x; win_Bounds.Y = cs->y;
			win_Bounds.Width = cs->cx; win_Bounds.Height = cs->cy;

			RECT rect;
			GetClientRect(handle, &rect);
			win_ClientRect = Window_FromRect(rect);
			invisible_since_creation = true;
		}
		break;

	case WM_CLOSE:
		Event_RaiseVoid(&WindowEvents_Closing);
		Window_Destroy();
		break;

	case WM_DESTROY:
		win_Exists = false;
		UnregisterClassA(win_ClassName, win_Instance);
		if (win_DC != NULL) ReleaseDC(win_Handle, win_DC);
		Event_RaiseVoid(&WindowEvents_Closed);
		break;
	}
	return DefWindowProcA(handle, message, wParam, lParam);
}


void Window_Create(Int32 x, Int32 y, Int32 width, Int32 height, STRING_REF String* title, DisplayDevice* device) {
	win_Instance = GetModuleHandleA(NULL);
	/* TODO: UngroupFromTaskbar(); */

	/* Find out the final window rectangle, after the WM has added its chrome (titlebar, sidebars etc). */
	RECT rect; rect.left = x; rect.top = y; rect.right = x + width; rect.bottom = y + height;
	AdjustWindowRectEx(&rect, win_Style, false, win_StyleEx);

	WNDCLASSEXA wc;
	Platform_MemSet(&wc, 0, sizeof(WNDCLASSEXA));
	wc.cbSize = sizeof(WNDCLASSEXA);
	wc.style = CS_OWNDC;
	wc.hInstance = win_Instance;
	wc.lpfnWndProc = Window_Procedure;
	wc.lpszClassName = win_ClassName;
	/* TODO: Set window icons here */
	wc.hCursor = LoadCursorA(NULL, IDC_ARROW);

	ATOM atom = RegisterClassExA(&wc);
	if (atom == 0) {
		ErrorHandler_FailWithCode(GetLastError(), "Failed to register window class");
	}
	win_Handle = CreateWindowExA(
		win_StyleEx, atom, title->buffer, win_Style,
		rect.left, rect.top, RECT_WIDTH(rect), RECT_HEIGHT(rect),
		NULL, NULL, win_Instance, NULL);

	if (win_Handle == NULL) {
		ErrorHandler_FailWithCode(GetLastError(), "Failed to create window");
	}
	win_DC = GetDC(win_Handle);
	if (win_DC == NULL) {
		ErrorHandler_FailWithCode(GetLastError(), "Failed to get device context");
	}
	win_Exists = true;
}

void Window_GetClipboardText(STRING_TRANSIENT String* value) {
	/* retry up to 10 times*/
	Int32 i;
	value->length = 0;
	for (i = 0; i < 10; i++) {
		if (!OpenClipboard(win_Handle)) {
			Platform_ThreadSleep(100);
			continue;
		}

		bool isUnicode = true;
		HANDLE hGlobal = GetClipboardData(CF_UNICODETEXT);
		if (hGlobal == NULL) {
			hGlobal = GetClipboardData(CF_TEXT);
			isUnicode = false;
		}
		if (hGlobal == NULL) { CloseClipboard(); return; }
		LPVOID src = GlobalLock(hGlobal);

		/* TODO: Trim space / tabs from start and end of clipboard text */
		if (isUnicode) {
			UInt16* text = (UInt16*)src;
			while (*text != NULL) {
				String_Append(value, Convert_UnicodeToCP437(*text)); text++;
			}
		} else {
			UInt8* text = (UInt8*)src;
			while (*text != 0) {
				String_Append(value, *text); text++;
			}
		}

		GlobalUnlock(hGlobal);
		CloseClipboard();
		return;
	}
}

void Window_SetClipboardText(STRING_PURE String* value) {
	/* retry up to 10 times*/
	Int32 i;
	value->length = 0;
	for (i = 0; i < 10; i++) {
		if (!OpenClipboard(win_Handle)) {
			Platform_ThreadSleep(100);
			continue;
		}

		HANDLE hGlobal = GlobalAlloc(GMEM_MOVEABLE, String_BufferSize(value->length) * sizeof(UInt16));
		if (hGlobal == NULL) { CloseClipboard(); return; }

		LPVOID dst = GlobalLock(hGlobal);
		UInt16* text = (UInt16*)dst;
		for (i = 0; i < value->length; i++) {
			*text = Convert_CP437ToUnicode(value->buffer[i]); text++;
		}
		*text = NULL;

		GlobalUnlock(hGlobal);
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hGlobal);
		CloseClipboard();
		return;
	}
}


Rectangle2D Window_GetBounds(void) { return win_Bounds; }
void Window_SetBounds(Rectangle2D rect) {
	/* Note: the bounds variable is updated when the resize/move message arrives.*/
	SetWindowPos(win_Handle, NULL, rect.X, rect.Y, rect.Width, rect.Height, 0);
}

Point2D Window_GetLocation(void) { return Point2D_Make(win_Bounds.X, win_Bounds.Y); }
void Window_SetLocation(Point2D point) {
	SetWindowPos(win_Handle, NULL, point.X, point.Y, 0, 0, SWP_NOSIZE);
}

Size2D Window_GetSize(void) { return Size2D_Make(win_Bounds.Width, win_Bounds.Height); }
void Window_SetSize(Size2D size) {
	SetWindowPos(win_Handle, NULL, 0, 0, size.Width, size.Height, SWP_NOMOVE);
}

Rectangle2D Window_GetClientRectangle(void) { return win_ClientRect; }
void Window_SetClientRectangle(Rectangle2D rect) {
	Size2D size = Size2D_Make(rect.Width, rect.Height);
	Window_SetClientSize(size);
}

Size2D Window_GetClientSize(void) {
	return Size2D_Make(win_ClientRect.Width, win_ClientRect.Height);
}
void Window_SetClientSize(Size2D size) {
	DWORD style = GetWindowLongA(win_Handle, GWL_STYLE);
	RECT rect;
	rect.left = 0; rect.top = 0;
	rect.right = size.Width; rect.bottom = size.Height;

	AdjustWindowRect(&rect, style, false);
	Window_SetSize(Size2D_Make(RECT_WIDTH(rect), RECT_HEIGHT(rect)));
}

/* TODO: Set window icon
public Icon Icon{
get{ return icon; }
set{
icon = value;
if (window.handle != IntPtr.Zero)
{
//Icon small = new Icon( value, 16, 16 );
//GC.KeepAlive( small );
API.SendMessage(window.handle, WM_SETICON, (IntPtr)0, icon == null ? IntPtr.Zero : value.Handle);
API.SendMessage(window.handle, WM_SETICON, (IntPtr)1, icon == null ? IntPtr.Zero : value.Handle);
}
}
}*/

bool Window_GetFocused(void) { return win_Focused; }
bool Window_GetExists(void) { return win_Exists; }
void* Window_GetWindowHandle(void) { return win_Handle; }

bool Window_GetVisible(void) { return IsWindowVisible(win_Handle); }
void Window_SetVisible(bool visible) {
	if (visible) {
		ShowWindow(win_Handle, SW_SHOW);
		if (invisible_since_creation) {
			BringWindowToTop(win_Handle);
			SetForegroundWindow(win_Handle);
		}
	}
	else {
		ShowWindow(win_Handle, SW_HIDE);
	}
}


void Window_Close(void) {
	PostMessageA(win_Handle, WM_CLOSE, NULL, NULL);
}

UInt8 Window_GetWindowState(void) { return win_State; }
void Window_SetWindowState(UInt8 value) {
	if (win_State == value) return;

	DWORD command = 0;
	bool exiting_fullscreen = false;

	switch (value) {
	case WINDOW_STATE_NORMAL:
		command = SW_RESTORE;

		/* If we are leaving fullscreen mode we need to restore the border. */
		if (win_State == WINDOW_STATE_FULLSCREEN)
			exiting_fullscreen = true;
		break;

	case WINDOW_STATE_MAXIMISED:
		/* Reset state to avoid strange interactions with fullscreen/minimized windows. */
		Window_ResetWindowState();
		command = SW_MAXIMIZE;
		break;

	case WINDOW_STATE_MINIMISED:
		command = SW_MINIMIZE;
		break;

	case WINDOW_STATE_FULLSCREEN:
		/* We achieve fullscreen by hiding the window border and sending the MAXIMIZE command.
		We cannot use the WindowState.Maximized directly, as that will not send the MAXIMIZE
		command for windows with hidden borders. */

		/* Reset state to avoid strange side-effects from maximized/minimized windows. */
		Window_ResetWindowState();
		previous_bounds = win_Bounds;
		Window_SetHiddenBorder(true);

		command = SW_MAXIMIZE;
		SetForegroundWindow(win_Handle);
		break;
	}

	if (command != 0) ShowWindow(win_Handle, command);

	/* Restore previous window border or apply pending border change when leaving fullscreen mode. */
	if (exiting_fullscreen) Window_SetHiddenBorder(false);

	/* Restore previous window size/location if necessary */
	if (command == SW_RESTORE && !Rectangle2D_Equals(previous_bounds, Rectangle2D_Empty)) {
		Window_SetBounds(previous_bounds);
		previous_bounds = Rectangle2D_Empty;
	}
}

Point2D Window_PointToClient(Point2D point) {
	POINT p; p.x = point.X; p.y = point.Y;
	if (!ScreenToClient(win_Handle, &p)) {
		ErrorHandler_FailWithCode(GetLastError(), "Converting point from client to screen coordinates");
	}
	return Point2D_Make(p.x, p.y);
}

Point2D Window_PointToScreen(Point2D p) {
	ErrorHandler_Fail("PointToScreen NOT IMPLEMENTED");
}

MSG msg;
void Window_ProcessEvents(void) {
	while (PeekMessageA(&msg, NULL, 0, 0, 1)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	HWND foreground = GetForegroundWindow();
	if (foreground != NULL) {
		win_Focused = foreground == win_Handle;
	}
}

Point2D Window_GetDesktopCursorPos(void) {
	POINT p; GetCursorPos(&p);
	return Point2D_Make(p.x, p.y);
}
void Window_SetDesktopCursorPos(Point2D point) {
	SetCursorPos(point.X, point.Y);
}

bool win_cursorVisible = true;
bool Window_GetCursorVisible(void) { return win_cursorVisible; }
void Window_SetCursorVisible(bool visible) {
	win_cursorVisible = visible;
	ShowCursor(visible ? 1 : 0);
}

#if !USE_DX

void GLContext_SelectGraphicsMode(GraphicsMode mode) {
	ColorFormat color = mode.Format;

	PIXELFORMATDESCRIPTOR pfd;
	Platform_MemSet(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
	/* TODO: PFD_SUPPORT_COMPOSITION FLAG? CHECK IF IT WORKS ON XP */
	pfd.cColorBits = (UInt8)(color.R + color.G + color.B);

	pfd.iPixelType = color.IsIndexed ? PFD_TYPE_COLORINDEX : PFD_TYPE_RGBA;
	pfd.cRedBits = (UInt8)color.R;
	pfd.cGreenBits = (UInt8)color.G;
	pfd.cBlueBits = (UInt8)color.B;
	pfd.cAlphaBits = (UInt8)color.A;

	pfd.cDepthBits = (UInt8)mode.Depth;
	pfd.cStencilBits = (UInt8)mode.Stencil;
	if (mode.Depth <= 0) pfd.dwFlags |= PFD_DEPTH_DONTCARE;
	if (mode.Buffers > 1) pfd.dwFlags |= PFD_DOUBLEBUFFER;

	Int32 modeIndex = ChoosePixelFormat(win_DC, &pfd);
	if (modeIndex == 0) {
		ErrorHandler_Fail("Requested graphics mode not available");
	}

	Platform_MemSet(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;

	DescribePixelFormat(win_DC, modeIndex, pfd.nSize, &pfd);
	if (!SetPixelFormat(win_DC, modeIndex, &pfd)) {
		ErrorHandler_FailWithCode(GetLastError(), "SetPixelFormat failed");
	}
}

HGLRC GLContext_Handle;
HDC GLContext_DC;
typedef BOOL (WINAPI *FN_WGLSWAPINTERVAL)(int interval);
typedef int (WINAPI *FN_WGLGETSWAPINTERVAL)(void);
FN_WGLSWAPINTERVAL wglSwapIntervalEXT;
FN_WGLGETSWAPINTERVAL wglGetSwapIntervalEXT;
bool GLContext_vSync;

void GLContext_Init(GraphicsMode mode) {
	GLContext_SelectGraphicsMode(mode);
	GLContext_Handle = wglCreateContext(win_DC);
	if (GLContext_Handle == NULL) {
		GLContext_Handle = wglCreateContext(win_DC);
	}
	if (GLContext_Handle == NULL) {
		ErrorHandler_FailWithCode(GetLastError(), "Failed to create OpenGL context");
	}

	if (!wglMakeCurrent(win_DC, GLContext_Handle)) {
		ErrorHandler_FailWithCode(GetLastError(), "Failed to make OpenGL context current");
	}
	GLContext_DC = wglGetCurrentDC();

	wglGetSwapIntervalEXT = (FN_WGLGETSWAPINTERVAL)GLContext_GetAddress("wglGetSwapIntervalEXT");
	wglSwapIntervalEXT = (FN_WGLSWAPINTERVAL)GLContext_GetAddress("wglSwapIntervalEXT");
	GLContext_vSync = wglGetSwapIntervalEXT != NULL && wglSwapIntervalEXT != NULL;
}

void GLContext_Update(void) { }
void GLContext_Free(void) {
	if (!wglDeleteContext(GLContext_Handle)) {
		ErrorHandler_FailWithCode(GetLastError(), "Failed to destroy OpenGL context");
	}
	GLContext_Handle = NULL;
}

void* GLContext_GetAddress(const UInt8* function) {
	void* address = wglGetProcAddress(function);
	return GLContext_IsInvalidAddress(address) ? NULL : address;
}

void GLContext_SwapBuffers(void) {
	if (!SwapBuffers(GLContext_DC)) {
		ErrorHandler_FailWithCode(GetLastError(), "Failed to swap buffers");
	}
}

bool GLContext_GetVSync(void) {
	return GLContext_vSync && wglGetSwapIntervalEXT();
}

void GLContext_SetVSync(bool enabled) {
	if (GLContext_vSync) wglSwapIntervalEXT(enabled ? 1 : 0);
}
#endif