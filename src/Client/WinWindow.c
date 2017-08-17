#include "Window.h"
#include <Windows.h>
#include "Platform.h"
#include "Mouse.h"
#include "Key.h"
#include "Events.h"

#define win_Style WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN
#define win_StyleEx WS_EX_WINDOWEDGE | WS_EX_APPWINDOW
#define win_ClassName "ClassiCube_Window"
#define RECT_WIDTH(rect) (rect.right - rect.width)
#define RECT_HEIGHT(rect) (rect.bottom - rect.top)

HINSTANCE win_Instance;
HWND win_Handle;
WindowState win_State = WindowState_Normal;
bool win_Exists, win_Focused;
bool mouse_outside_window = true;
bool invisible_since_creation; // Set by WindowsMessage.CREATE and consumed by Visible = true (calls BringWindowToFront).
Int32 suppress_resize; // Used in WindowBorder and WindowState in order to avoid rapid, consecutive resize events.

Rectangle2D win_Bounds;
Rectangle2D win_ClientRect;
Rectangle2D previous_bounds; // Used to restore previous size when leaving fullscreen mode.

const long ExtendedBit = 1 << 24;           // Used to distinguish left and right control, alt and enter keys.

void Window_Create(Int32 x, Int32 y, Int32 width, Int32 height, STRING_TRANSIENT String* title, DisplayDevice* device) {
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
		win_StyleEx, win_ClassName, title->buffer, win_Style,
		rect.left, rect.top, RECT_WIDTH(rect), RECT_HEIGHT(rect),
		NULL, NULL, win_Instance, NULL);
	if (win_Handle == NULL) {
		ErrorHandler_FailWithCode(GetLastError(), "Failed to create window");
	}

	exists = true;
}

WNDPROC Window_Procedure(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) {
	bool new_focused_state;
	Real32 wheel_delta;
	WORD mouse_x, mouse_y;
	MouseButton btn;
	WINDOWPOS* pos;
	WindowState new_state;

	switch (message) {

	case WM_ACTIVATE:
		new_focused_state = LOWORD(wParam);
		if (new_focused_state != focused) {
			focused = new_focused_state;
			Event_RaiseVoid(&WindowEvents_OnFocusedChanged);
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
			Point new_location = new Point(pos->x, pos->y);
			if (Location != new_location) {
				bounds.Location = new_location;
				Event_RaiseVoid(&WindowEvents_OnMove);
			}

			Size new_size = new Size(pos->cx, pos->cy);
			if (Size != new_size) {
				bounds.Width = pos->cx;
				bounds.Height = pos->cy;

				RECT rect;
				GetClientRect(handle, &rect);
				client_rectangle = rect.ToRectangle();

				SetWindowPos(win_Handle, NULL,
					bounds.X, bounds.Y, bounds.Width, bounds.Height,
					SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

				if (suppress_resize <= 0) {
					Event_RaiseVoid(&Window_OnResize);
				}
			}
		}
		break;

	case WM_STYLECHANGED:
		if (wParam == GWL_STYLE) {
			DWORD style = ((STYLESTRUCT*)lParam)->styleNew;
			if ((style & WS_POPUP) != 0) {
				hiddenBorder = true;
			} else if ((style & WS_THICKFRAME) != 0) {
				hiddenBorder = false;
			}
		}
		break;

	case WM_SIZE:
		new_state = win_State;
		switch (wParam) {
		case SIZE_RESTORED: new_state = WindowState_Normal; break;
		case SIZE_MINIMIZED: new_state = WindowState_Minimized; break;
		case SIZE_MAXIMIZED: new_state = hiddenBorder ? WindowState_Fullscreen : WindowState_Maximized; break;
		}

		if (new_state != win_State) {
			win_State = new_state;
			Event_RaiseVoid(&WindowEvents_OnWindowStateChanged);
		}
		break;


	case WM_CHAR:
		if (IntPtr.Size == 4)
			key_press.KeyChar = (char)wParam.ToInt32();
		else
			key_press.KeyChar = (char)wParam.ToInt64();

		if (KeyPress != null)
			KeyPress(this, key_press);
		break;

	case WM_MOUSEMOVE:
		mouse_x = LOWORD(lParam);
		mouse_y = HIWORD(lParam);
		Mouse_SetPosition(mouse_x, mouse_y);

		if (mouse_outside_window) {
			/* Once we receive a mouse move event, it means that the mouse has re-entered the window. */
			mouse_outside_window = false;
			EnableMouseTracking();
			Event_RaiseVoid(&WindowEvents_OnMouseEnter);
		}
		break;

	case WM_MOUSELEAVE:
		mouse_outside_window = true;
		/* Mouse tracking is disabled automatically by the OS */
		Event_RaiseVoid(&WindowEvents_OnMouseLeave);

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
		bool pressed = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;

		// Shift/Control/Alt behave strangely when e.g. ShiftRight is held down and ShiftLeft is pressed
		// and released. It looks like neither key is released in this case, or that the wrong key is
		// released in the case of Control and Alt.
		// To combat this, we are going to release both keys when either is released. Hacky, but should work.
		// Win95 does not distinguish left/right key constants (GetAsyncKeyState returns 0).
		// In this case, both keys will be reported as pressed.

		bool extended = (lParam.ToInt64() & ExtendedBit) != 0;
		switch (wParam)
		{
		case VK_SHIFT:
			// The behavior of this key is very strange. Unlike Control and Alt, there is no extended bit
			// to distinguish between left and right keys. Moreover, pressing both keys and releasing one
			// may result in both keys being held down (but not always).
			bool lShiftDown = (API.GetKeyState(VK_LSHIFT) >> 15) == 1;
			bool rShiftDown = (API.GetKeyState(VK_RSHIFT) >> 15) == 1;

			if (!pressed || lShiftDown != rShiftDown) {
				Key_SetPressed(Key_ShiftLeft, lShiftDown);
				Key_SetPressed(Key_ShiftRight, rShiftDown);
			}
			return IntPtr.Zero;

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
			Key tkKey;
			if (!KeyMap.TryGetMappedKey((VirtualKeys)wParam, out tkKey)) {
				break;
			} else {
				Key_SetPressed(tkKey, pressed);
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
		CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
		if (cs->hwndParent == NULL) {
			win_Bounds.X = cs.x; win_Bounds.Y = cs.y;
			win_Bounds.Width = cs.cx; win_Bounds.Height = cs.cy;

			RECT rect;
			API.GetClientRect(handle, &rect);
			win_ClientRect = rect.ToRectangle();
			invisible_since_creation = true;
		}
		break;

	case WM_CLOSE:
		Event_RaiseVoid(&WindowEvents_Closing);
		if (Unload != null)
			Unload(this, EventArgs.Empty);
		DestroyWindow();
		break;

	case WM_DESTROY:
		win_Exists = false;
		UnregisterClassA(Window_ClassName, win_Instance);
		window.Dispose();
		Event_RaiseVoid(&WindowEvents_OnClosed);
		break;
	}
	return DefWindowProcA(handle, message, wParam, lParam);
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

void Window_Destroy(void) {
	if (!win_Exists) return;
	
	DestroyWindow(win_Handle);
	win_Exists = false;
}

void Window_SetHiddenBorder(bool hidden) {
	suppress_resize++;
	HiddenBorder = hidden;
	ProcessEvents();
	suppress_resize--;
}

void Window_ResetWindowState(void) {
	suppress_resize++;
	WindowState = WindowState_Normal;
	ProcessEvents();
	suppress_resize--;
}

public unsafe string GetClipboardText() {
	// retry up to 10 times
	for (int i = 0; i < 10; i++) {
		if (!API.OpenClipboard(window.handle)) {
			Thread.Sleep(100);
			continue;
		}

		bool isUnicode = true;
		IntPtr hGlobal = API.GetClipboardData(CF_UNICODETEXT);
		if (hGlobal == IntPtr.Zero) {
			hGlobal = API.GetClipboardData(CF_TEXT);
			isUnicode = false;
		}
		if (hGlobal == IntPtr.Zero) { API.CloseClipboard(); return ""; }

		IntPtr src = API.GlobalLock(hGlobal);
		string value = isUnicode ? new String((char*)src) : new String((sbyte*)src);
		API.GlobalUnlock(hGlobal);

		API.CloseClipboard();
		return value;
	}
	return "";
}

public unsafe void SetClipboardText(string value) {
	UIntPtr dstSize = (UIntPtr)((value.Length + 1) * Marshal.SystemDefaultCharSize);
	// retry up to 10 times
	for (int i = 0; i < 10; i++) {
		if (!API.OpenClipboard(window.handle)) {
			Thread.Sleep(100);
			continue;
		}

		IntPtr hGlobal = API.GlobalAlloc(GMEM_MOVEABLE, dstSize);
		if (hGlobal == IntPtr.Zero) { API.CloseClipboard(); return; }

		IntPtr dst = API.GlobalLock(hGlobal);
		fixed(char* src = value) {
			CopyString_Unicode((IntPtr)src, dst, value.Length);
		}
		API.GlobalUnlock(hGlobal);

		API.EmptyClipboard();
		API.SetClipboardData(CF_UNICODETEXT, hGlobal);
		API.CloseClipboard();
		return;
	}
}

unsafe static void CopyString_Unicode(IntPtr src, IntPtr dst, int numChars) {
	char* src2 = (char*)src, dst2 = (char*)dst;
	for (int i = 0; i < numChars; i++) { dst2[i] = src2[i]; }
	dst2[numChars] = '\0';
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
void Window_SetClientRectangle(Rectangle rect) {
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

bool Window_GetFocused(void) { return focused; }

bool Window_GetVisible(void) { return IsWindowVisible(win_Handle); }
void Window_SetVisible(bool visible) {
	if (visible) {
		ShowWindow(win_Handle, SW_SHOW);
		if (invisible_since_creation) {
			BringWindowToTop(win_Handle);
			SetForegroundWindow(win_Handle);
		}
	} else {
		ShowWindow(win_Handle, SW_HIDE);
	}
}

bool Window_GetExists(void) { return exists; }

void Window_Close(void) {
	PostMessageA(win_Handle, WM_CLOSE, NULL, NULL);
}

public WindowState WindowState{
	get{ return windowState; }
	set{
	if (WindowState == value)
	return;

ShowWindowCommand command = 0;
bool exiting_fullscreen = false;

switch (value) {
case WindowState.Normal:
	command = ShowWindowCommand.RESTORE;

	// If we are leaving fullscreen mode we need to restore the border.
	if (WindowState == WindowState.Fullscreen)
		exiting_fullscreen = true;
	break;

case WindowState.Maximized:
	// Reset state to avoid strange interactions with fullscreen/minimized windows.
	ResetWindowState();
	command = ShowWindowCommand.MAXIMIZE;
	break;

case WindowState.Minimized:
	command = ShowWindowCommand.MINIMIZE;
	break;

case WindowState.Fullscreen:
	// We achieve fullscreen by hiding the window border and sending the MAXIMIZE command.
	// We cannot use the WindowState.Maximized directly, as that will not send the MAXIMIZE
	// command for windows with hidden borders.

	// Reset state to avoid strange side-effects from maximized/minimized windows.
	ResetWindowState();
	previous_bounds = Bounds;
	SetHiddenBorder(true);

	command = ShowWindowCommand.MAXIMIZE;
	SetForegroundWindow(win_Handle);
	break;
}

if (command != 0)
API.ShowWindow(window.handle, command);

// Restore previous window border or apply pending border change when leaving fullscreen mode.
if (exiting_fullscreen)
SetHiddenBorder(false);

// Restore previous window size/location if necessary
if (command == ShowWindowCommand.RESTORE && previous_bounds != Rectangle.Empty) {
	Bounds = previous_bounds;
	previous_bounds = Rectangle.Empty;
}
}
}

bool win_hiddenBorder;
void Window_SetHiddenBorder(bool value) {
	if (win_hiddenBorder == value) return;

	/* We wish to avoid making an invisible window visible just to change the border.
	 However, it's a good idea to make a visible window invisible temporarily, to
	 avoid garbage caused by the border change. */
	bool was_visible = Visible;

	/* To ensure maximized/minimized windows work correctly, reset state to normal,
	 change the border, then go back to maximized/minimized. */
	WindowState state = WindowState;
	Window_ResetWindowState();
	DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	style |= (value ? WS_POPUP : WS_OVERLAPPEDWINDOW);

	/* Make sure client size doesn't change when changing the border style.*/
	Win32Rectangle rect = Win32Rectangle.From(bounds);
	AdjustWindowRectEx(&rect, style, false, win_ExStyle);

	/* This avoids leaving garbage on the background window. */
	if (was_visible) Window_SetVisible(false);

	SetWindowLong(window.handle, GWL_STYLE, style);
	SetWindowPos(win_Handle, NULL, 0, 0, rect.Width, rect.Height,
		SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);

	/* Force window to redraw update its borders, but only if it's
	 already visible (invisible windows will change borders when
	 they become visible, so no need to make them visiable prematurely).*/
	if (was_visible) Window_SetVisible(true);
	WindowState = state;
}

public Point PointToClient(Point point) {
	if (!API.ScreenToClient(window.handle, ref point))
		throw new InvalidOperationException(String.Format(
			"Could not convert point {0} from client to screen coordinates. Windows error: {1}",
			point.ToString(), Marshal.GetLastWin32Error()));

	return point;
}

Point2D Window_PointToScreen(Point2D p) {
	ErrorHandler_Fail("NOT IMPLEMENTED");
}

public event EventHandler<EventArgs> Load;
public event EventHandler<EventArgs> Unload;

MSG msg;
void Window_ProcessEvents() {
	while (PeekMessageA(&msg, NULL, 0, 0, 1)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	HWND foreground = GetForegroundWindow();
	if (foreground != NULL)
		focused = foreground == window.handle;
}

Point2D Window_GetDesktopCursorPos(void) {
	POINT p; GetCursorPos(&p);
	return Point2D_Make(p.X, p.Y);
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

internal WinKeyMap(VK key) {
	if (key >= VK_F1 && key <= VK_F24) {
		return Key.F1 + (key - VK_F1);
	}
	if (key >= '0' && key <= '9') {
		return Key.Number0 + (key - '0');
	}
	if (key >= 'A' && key <= 'Z') {
		return Key.A + (key - 'A');
	}
	// Keypad
	for (int i = 0; i <= 9; i++) {
		AddKey((VirtualKeys)((int)VirtualKeys.NUMPAD0 + i), Key.Keypad0 + i);
	}

			AddKey(VirtualKeys.ESCAPE, Key.Escape);
			AddKey(VirtualKeys.TAB, Key.Tab);
			AddKey(VirtualKeys.CAPITAL, Key.CapsLock);
			AddKey(VirtualKeys.LCONTROL, Key.ControlLeft);
			AddKey(VirtualKeys.LSHIFT, Key.ShiftLeft);
			AddKey(VirtualKeys.LWIN, Key.WinLeft);
			AddKey(VirtualKeys.LMENU, Key.AltLeft);
			AddKey(VirtualKeys.SPACE, Key.Space);
			AddKey(VirtualKeys.RMENU, Key.AltRight);
			AddKey(VirtualKeys.RWIN, Key.WinRight);
			AddKey(VirtualKeys.APPS, Key.Menu);
			AddKey(VirtualKeys.RCONTROL, Key.ControlRight);
			AddKey(VirtualKeys.RSHIFT, Key.ShiftRight);
			AddKey(VirtualKeys.RETURN, Key.Enter);
			AddKey(VirtualKeys.BACK, Key.BackSpace);

			AddKey(VirtualKeys.OEM_1, Key.Semicolon);      // Varies by keyboard, ;: on Win2K/US
			AddKey(VirtualKeys.OEM_2, Key.Slash);          // Varies by keyboard, /? on Win2K/US
			AddKey(VirtualKeys.OEM_3, Key.Tilde);          // Varies by keyboard, `~ on Win2K/US
			AddKey(VirtualKeys.OEM_4, Key.BracketLeft);    // Varies by keyboard, [{ on Win2K/US
			AddKey(VirtualKeys.OEM_5, Key.BackSlash);      // Varies by keyboard, \| on Win2K/US
			AddKey(VirtualKeys.OEM_6, Key.BracketRight);   // Varies by keyboard, ]} on Win2K/US
			AddKey(VirtualKeys.OEM_7, Key.Quote);          // Varies by keyboard, '" on Win2K/US
			AddKey(VirtualKeys.OEM_PLUS, Key.Plus);        // Invariant: +
			AddKey(VirtualKeys.OEM_COMMA, Key.Comma);      // Invariant: ,
			AddKey(VirtualKeys.OEM_MINUS, Key.Minus);      // Invariant: -
			AddKey(VirtualKeys.OEM_PERIOD, Key.Period);    // Invariant: .

			AddKey(VirtualKeys.HOME, Key.Home);
			AddKey(VirtualKeys.END, Key.End);
			AddKey(VirtualKeys.DELETE, Key.Delete);
			AddKey(VirtualKeys.PRIOR, Key.PageUp);
			AddKey(VirtualKeys.NEXT, Key.PageDown);
			AddKey(VirtualKeys.PRINT, Key.PrintScreen);
			AddKey(VirtualKeys.PAUSE, Key.Pause);
			AddKey(VirtualKeys.NUMLOCK, Key.NumLock);

			AddKey(VirtualKeys.SCROLL, Key.ScrollLock);
			AddKey(VirtualKeys.SNAPSHOT, Key.PrintScreen);
			AddKey(VirtualKeys.INSERT, Key.Insert);

			AddKey(VirtualKeys.DECIMAL, Key.KeypadDecimal);
			AddKey(VirtualKeys.ADD, Key.KeypadAdd);
			AddKey(VirtualKeys.SUBTRACT, Key.KeypadSubtract);
			AddKey(VirtualKeys.DIVIDE, Key.KeypadDivide);
			AddKey(VirtualKeys.MULTIPLY, Key.KeypadMultiply);

			// Navigation
			AddKey(VirtualKeys.UP, Key.Up);
			AddKey(VirtualKeys.DOWN, Key.Down);
			AddKey(VirtualKeys.LEFT, Key.Left);
			AddKey(VirtualKeys.RIGHT, Key.Right);
		}