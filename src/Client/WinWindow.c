#include "Window.h"
#include <Windows.h>
#include "Platform.h"
#include "Mouse.h"
#include "Key.h"
#include "Events.h"

#define Window_Style WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN
#define Window_StyleEx WS_EX_WINDOWEDGE | WS_EX_APPWINDOW
HINSTANCE window_Instance;
HWND window_Handle;
#define Window_ClassName "ClassicalSharp_Window"
bool class_registered, disposed, exists;
WindowState windowState = WindowState_Normal;
bool focused;
bool mouse_outside_window = true;
bool invisible_since_creation; // Set by WindowsMessage.CREATE and consumed by Visible = true (calls BringWindowToFront).
int suppress_resize; // Used in WindowBorder and WindowState in order to avoid rapid, consecutive resize events.

Rectangle2D bounds, client_rectangle, previous_bounds; // Used to restore previous size when leaving fullscreen mode.
Icon icon;

const long ExtendedBit = 1 << 24;           // Used to distinguish left and right control, alt and enter keys.

void Window_Create(Int32 x, Int32 y, Int32 width, Int32 height, STRING_TRANSIENT String* title, DisplayDevice* device) {
	window_Instance = GetModuleHandleA(NULL);
	/* TODO: UngroupFromTaskbar(); */

	/* Find out the final window rectangle, after the WM has added its chrome (titlebar, sidebars etc). */
	RECT rect; rect.left = x; rect.top = y; rect.right = x + width; rect.bottom = y + height;
	AdjustWindowRectEx(&rect, Window_Style, false, Window_StyleEx);

	WNDCLASSEXA wc;
	Platform_MemSet(&wc, 0, sizeof(WNDCLASSEXA));
	wc.cbSize = sizeof(WNDCLASSEXA);
	wc.Style = ClassStyle.OwnDC;
	wc.hInstance = window_Instance;
	wc.lpfnWndProc = Window_Procedure;
	wc.lpszClassName = Window_ClassName;
	/* TODO: Set window icons here */
	wc.hCursor = LoadCursorA(NULL, IDC_ARROW); // CursorName.Arrow

	ATOM atom = RegisterClassExA(&wc);
	if (atom == 0) {
		ErrorHandler_FailWithCode(GetLastError(), "Failed to register window class");
	}

	window_Handle = CreateWindowExA(
		Window_StyleEx, Window_ClassName, title->buffer, Window_Style,
		rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		NULL, NULL, window_Instance, NULL);
	if (window_Handle == NULL) {
		ErrorHandler_FailWithCode(GetLastError(), "Failed to create window");
	}

	exists = true;
}

WNDPROC Window_Procedure(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) {
	bool new_focused_state;
	Real32 wheel_delta;
	switch (message) {

	case WM_ACTIVATE:
		// See http://msdn.microsoft.com/en-us/library/ms646274(VS.85).aspx (WM_ACTIVATE notification):
		// wParam: The low-order word specifies whether the window is being activated or deactivated.
		new_focused_state = Focused;
		focused = LOWORD(wParam);

		if (new_focused_state != Focused)
			FocusedChanged(this, EventArgs.Empty);
		break;

	case WM_ENTERMENULOOP:
	case WM_ENTERSIZEMOVE:
	case WM_EXITMENULOOP:
	case WM_EXITSIZEMOVE:
		break;

	case WM_ERASEBKGND:
		return 1;

	case WM_WINDOWPOSCHANGED:
		WINDOWPOS* pos = (WINDOWPOS*)lParam;
		if (window != null && pos->hwnd == window.handle) {
			Point new_location = new Point(pos->x, pos->y);
			if (Location != new_location) {
				bounds.Location = new_location;
				if (Move != null)
					Move(this, EventArgs.Empty);
			}

			Size new_size = new Size(pos->cx, pos->cy);
			if (Size != new_size) {
				bounds.Width = pos->cx;
				bounds.Height = pos->cy;

				RECT rect;
				GetClientRect(handle, &rect);
				client_rectangle = rect.ToRectangle();

				API.SetWindowPos(window.handle, IntPtr.Zero,
					bounds.X, bounds.Y, bounds.Width, bounds.Height,
					SetWindowPosFlags.NOZORDER | SetWindowPosFlags.NOOWNERZORDER |
					SetWindowPosFlags.NOACTIVATE | SetWindowPosFlags.NOSENDCHANGING);
				if (suppress_resize <= 0 && Resize != null)
					Resize(this, EventArgs.Empty);
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
		SizeMessage state = (SizeMessage)wParam.ToInt64();
		WindowState new_state = windowState;
		switch (state) {
		case SizeMessage.RESTORED: new_state = WindowState.Normal; break;
		case SizeMessage.MINIMIZED: new_state = WindowState.Minimized; break;
		case SizeMessage.MAXIMIZED: new_state = hiddenBorder ?
			WindowState.Fullscreen : WindowState.Maximized;
			break;
		}

		if (new_state != windowState) {
			windowState = new_state;
			if (WindowStateChanged != null)
				WindowStateChanged(this, EventArgs.Empty);
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
		WORD mouse_x = LOWORD(lParam);
		WORD mouse_y = HIWORD(lParam);
		Mouse_SetPosition(mouse_x, mouse_y);

		if (mouse_outside_window) {
			// Once we receive a mouse move event, it means that the mouse has
			// re-entered the window.
			mouse_outside_window = false;
			EnableMouseTracking();

			if (MouseEnter != null)
				MouseEnter(this, EventArgs.Empty);
		}
		break;

	case WM_MOUSELEAVE:
		mouse_outside_window = true;
		// Mouse tracking is disabled automatically by the OS

		if (MouseLeave != null)
			MouseLeave(this, EventArgs.Empty);
		// Set all mouse buttons to off when user leaves window, prevents them being stuck down.
		for (MouseButton btn = 0; btn < MouseButton.LastButton; btn++)
			mouse[btn] = false;
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

		// Keyboard events:
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
				Keyboard[Input.Key.ShiftLeft] = lShiftDown;
				Keyboard[Input.Key.ShiftRight] = rShiftDown;
			}
			return IntPtr.Zero;

		case VK_CONTROL:
			if (extended)
				keyboard[Input.Key.ControlRight] = pressed;
			else
				keyboard[Input.Key.ControlLeft] = pressed;
			return IntPtr.Zero;

		case VK_MENU:
			if (extended)
				keyboard[Input.Key.AltRight] = pressed;
			else
				keyboard[Input.Key.AltLeft] = pressed;
			return IntPtr.Zero;

		case VK_RETURN:
			if (extended)
				keyboard[Key.KeypadEnter] = pressed;
			else
				keyboard[Key.Enter] = pressed;
			return IntPtr.Zero;

		default:
			Key tkKey;
			if (!KeyMap.TryGetMappedKey((VirtualKeys)wParam, out tkKey)) {
				Debug.Print("Virtual key {0} ({1}) not mapped.", (VirtualKeys)wParam, lParam.ToInt64());
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
		CreateStruct cs = (CreateStruct)Marshal.PtrToStructure(lParam, typeof(CreateStruct));
		if (cs.hwndParent == IntPtr.Zero)
		{
			bounds.X = cs.x;
			bounds.Y = cs.y;
			bounds.Width = cs.cx;
			bounds.Height = cs.cy;

			RECT rect;
			API.GetClientRect(handle, &rect);
			client_rectangle = rect.ToRectangle();
			invisible_since_creation = true;
		}
		break;

	case WM_CLOSE:
		System.ComponentModel.CancelEventArgs e = new System.ComponentModel.CancelEventArgs();

		if (Closing != null)
			Closing(this, e);

		if (!e.Cancel)
		{
			if (Unload != null)
				Unload(this, EventArgs.Empty);

			DestroyWindow();
			break;
		}

		return IntPtr.Zero;

	case WM_DESTROY:
		exists = false;

		UnregisterClassA(Window_ClassName, Window_Instance);
		window.Dispose();

		Event_RaiseVoid(&WindowEvents_OnClosed);
		break;
	}
	return DefWindowProcA(handle, message, wParam, lParam);
}

void EnableMouseTracking() {
	TrackMouseEventStructure me = new TrackMouseEventStructure();
	me.Size = TrackMouseEventStructure.SizeInBytes;
	me.TrackWindowHandle = window.handle;
	me.Flags = TrackMouseEventFlags.LEAVE;

	if (!API.TrackMouseEvent(ref me))
		Debug.Print("[Warning] Failed to enable mouse tracking, error: {0}.",
			Marshal.GetLastWin32Error());
}

/// <summary> Starts the teardown sequence for the current window. </summary>
void DestroyWindow() {
	if (exists) {
		API.DestroyWindow(window.handle);
		exists = false;
	}
}

void SetHiddenBorder(bool hidden) {
	suppress_resize++;
	HiddenBorder = hidden;
	ProcessEvents();
	suppress_resize--;
}

void ResetWindowState() {
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

public Rectangle Bounds{
	get{ return bounds; }
	set{
	// Note: the bounds variable is updated when the resize/move message arrives.
	API.SetWindowPos(window.handle, IntPtr.Zero, value.X, value.Y, value.Width, value.Height, 0);
}
}

public Point Location{
	get{ return Bounds.Location; }
	set{
	// Note: the bounds variable is updated when the resize/move message arrives.
	API.SetWindowPos(window.handle, IntPtr.Zero, value.X, value.Y, 0, 0, SetWindowPosFlags.NOSIZE);
}
}

public Size Size{
	get{ return Bounds.Size; }
	set{
	// Note: the bounds variable is updated when the resize/move message arrives.
	API.SetWindowPos(window.handle, IntPtr.Zero, 0, 0, value.Width, value.Height, SetWindowPosFlags.NOMOVE);
}
}

public Rectangle ClientRectangle{
	get{
	if (client_rectangle.Width == 0)
	client_rectangle.Width = 1;
if (client_rectangle.Height == 0)
client_rectangle.Height = 1;
return client_rectangle;
} set{
	ClientSize = value.Size;
}
}

public Size ClientSize{
	get{ return ClientRectangle.Size; }
	set{
	WindowStyle style = (WindowStyle)API.GetWindowLong_N(window.handle, GetWindowLongOffsets.STYLE);
Win32Rectangle rect = Win32Rectangle.From(value);
API.AdjustWindowRect(ref rect, style, false);
Size = new Size(rect.Width, rect.Height);
}
}

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
}

public bool Focused{
	get{ return focused; }
}

public bool Visible{
	get{ return API.IsWindowVisible(window.handle); }
	set{
	if (value) {
		API.ShowWindow(window.handle, ShowWindowCommand.SHOW);
		if (invisible_since_creation) {
			API.BringWindowToTop(window.handle);
			API.SetForegroundWindow(window.handle);
		}
	}
	else {
		API.ShowWindow(window.handle, ShowWindowCommand.HIDE);
	}
}
}

public bool Exists{ get{ return exists; } }

public void Close() {
	API.PostMessage(window.handle, WM_CLOSE, IntPtr.Zero, IntPtr.Zero);
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
	SetForegroundWindow(window_Handle);
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

bool hiddenBorder;
bool HiddenBorder{
	set{
	if (hiddenBorder == value) return;

// We wish to avoid making an invisible window visible just to change the border.
// However, it's a good idea to make a visible window invisible temporarily, to
// avoid garbage caused by the border change.
bool was_visible = Visible;

// To ensure maximized/minimized windows work correctly, reset state to normal,
// change the border, then go back to maximized/minimized.
WindowState state = WindowState;
ResetWindowState();
WindowStyle style = WindowStyle.ClipChildren | WindowStyle.ClipSiblings;
style |= (value ? WindowStyle.Popup : WindowStyle.OverlappedWindow);

// Make sure client size doesn't change when changing the border style.
Win32Rectangle rect = Win32Rectangle.From(bounds);
API.AdjustWindowRectEx(ref rect, style, false, ParentStyleEx);

// This avoids leaving garbage on the background window.
if (was_visible)
Visible = false;

API.SetWindowLong_N(window.handle, GetWindowLongOffsets.STYLE, (IntPtr)(int)style);
API.SetWindowPos(window.handle, IntPtr.Zero, 0, 0, rect.Width, rect.Height,
	SetWindowPosFlags.NOMOVE | SetWindowPosFlags.NOZORDER |
	SetWindowPosFlags.FRAMECHANGED);

// Force window to redraw update its borders, but only if it's
// already visible (invisible windows will change borders when
// they become visible, so no need to make them visiable prematurely).
if (was_visible)
Visible = true;
WindowState = state;
}
}

public Point PointToClient(Point point) {
	if (!API.ScreenToClient(window.handle, ref point))
		throw new InvalidOperationException(String.Format(
			"Could not convert point {0} from client to screen coordinates. Windows error: {1}",
			point.ToString(), Marshal.GetLastWin32Error()));

	return point;
}

public Point PointToScreen(Point p) {
	throw new NotImplementedException();
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

public IWindowInfo WindowInfo{
	get{ return window; }
}

public Point DesktopCursorPos{
	get{
	POINT pos = default(POINT);
API.GetCursorPos(ref pos);
return new Point(pos.X, pos.Y);
}
set{ API.SetCursorPos(value.X, value.Y); }
}

bool cursorVisible = true;
bool Window_GetCursorVisible(void) { return cursorVisible; }
void Window_SetCursorVisible(bool visible) {
	cursorVisible = visible;
	ShowCursor(visible ? 1 : 0);
}