#include "Window.h"
#include "Platform.h"
#include "Input.h"
#include "Event.h"
#include "Logger.h"
#include "Funcs.h"

bool Window_Exists, Window_Focused;
Rect2D Window_Bounds, Window_ClientBounds;

static bool win_cursorVisible = true;
bool Cursor_GetVisible(void) { return win_cursorVisible; }

void Window_CreateSimple(int width, int height) {
	struct DisplayDevice* device = &DisplayDevice_Default;
	struct GraphicsMode mode;
	int x, y;

	x = device->Bounds.X + (device->Bounds.Width  - width)  / 2;
	y = device->Bounds.Y + (device->Bounds.Height - height) / 2;
	GraphicsMode_MakeDefault(&mode);

	Window_Create(x, y, width, height, &mode);
}


/*########################################################################################################################*
*------------------------------------------------------Win32 window-------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#define _WIN32_IE    0x0400
#define WINVER       0x0500
#define _WIN32_WINNT 0x0500
#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif
#include <windows.h>

#define CC_WIN_STYLE WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN
#define CC_WIN_CLASSNAME TEXT("ClassiCube_Window")
#define Rect_Width(rect)  (rect.right  - rect.left)
#define Rect_Height(rect) (rect.bottom - rect.top)

static HINSTANCE win_instance;
static HWND win_handle;
static HDC win_DC;
static int win_state;
static bool invisible_since_creation; /* Set by WindowsMessage.CREATE and consumed by Visible = true (calls BringWindowToFront) */
static int suppress_resize; /* Used in WindowBorder and WindowState in order to avoid rapid, consecutive resize events */
static Rect2D prev_bounds; /* Used to restore previous size when leaving fullscreen mode */


/*########################################################################################################################*
*-----------------------------------------------------Private details-----------------------------------------------------*
*#########################################################################################################################*/
const static uint8_t key_map[14 * 16] = {
	0, 0, 0, 0, 0, 0, 0, 0, KEY_BACKSPACE, KEY_TAB, 0, 0, 0, KEY_ENTER, 0, 0,
	0, 0, 0, KEY_PAUSE, KEY_CAPSLOCK, 0, 0, 0, 0, 0, 0, KEY_ESCAPE, 0, 0, 0, 0,
	KEY_SPACE, KEY_PAGEUP, KEY_PAGEDOWN, KEY_END, KEY_HOME, KEY_LEFT, KEY_UP, KEY_RIGHT, KEY_DOWN, 0, KEY_PRINTSCREEN, 0, KEY_PRINTSCREEN, KEY_INSERT, KEY_DELETE, 0,
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 0, 0, 0, 0, 0, 0,
	0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', KEY_LWIN, KEY_RWIN, KEY_MENU, 0, 0,
	KEY_KP0, KEY_KP1, KEY_KP2, KEY_KP3, KEY_KP4, KEY_KP5, KEY_KP6, KEY_KP7, KEY_KP8, KEY_KP9, KEY_KP_MULTIPLY, KEY_KP_PLUS, 0, KEY_KP_MINUS, KEY_KP_DECIMAL, KEY_KP_DIVIDE,
	KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_F13, KEY_F14, KEY_F15, KEY_F16,
	KEY_F17, KEY_F18, KEY_F19, KEY_F20, KEY_F21, KEY_F22, KEY_F23, KEY_F24, 0, 0, 0, 0, 0, 0, 0, 0,
	KEY_NUMLOCK, KEY_SCROLLLOCK, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	KEY_LSHIFT, KEY_RSHIFT, KEY_LCTRL, KEY_RCTRL, KEY_LALT, KEY_RALT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_SEMICOLON, KEY_PLUS, KEY_COMMA, KEY_MINUS, KEY_PERIOD, KEY_SLASH,
	KEY_TILDE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_LBRACKET, KEY_BACKSLASH, KEY_RBRACKET, KEY_QUOTE, 0,
};
static Key Window_MapKey(WPARAM key) { return key < Array_Elems(key_map) ? key_map[key] : 0; }

static void Window_ResetWindowState(void) {
	suppress_resize++;
	Window_SetWindowState(WINDOW_STATE_NORMAL);
	Window_ProcessEvents();
	suppress_resize--;
}

static bool win_hiddenBorder;
static void Window_DoSetHiddenBorder(bool value) {
	bool wasVisible;
	RECT rect;
	if (win_hiddenBorder == value) return;

	/* We wish to avoid making an invisible window visible just to change the border.
	However, it's a good idea to make a visible window invisible temporarily, to
	avoid garbage caused by the border change. */
	wasVisible = Window_GetVisible();

	/* To ensure maximized/minimized windows work correctly, reset state to normal,
	change the border, then go back to maximized/minimized. */
	int state = win_state;
	Window_ResetWindowState();
	DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	style |= (value ? WS_POPUP : WS_OVERLAPPEDWINDOW);

	/* Make sure client size doesn't change when changing the border style.*/
	rect.left = Window_Bounds.X; rect.top = Window_Bounds.Y;
	rect.right  = rect.left + Window_Bounds.Width;
	rect.bottom = rect.top  + Window_Bounds.Height;
	AdjustWindowRect(&rect, style, false);

	/* This avoids leaving garbage on the background window. */
	if (wasVisible) Window_SetVisible(false);

	SetWindowLong(win_handle, GWL_STYLE, style);
	SetWindowPos(win_handle, NULL, 0, 0, Rect_Width(rect), Rect_Height(rect),
		SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);

	/* Force window to redraw update its borders, but only if it's
	already visible (invisible windows will change borders when
	they become visible, so no need to make them visiable prematurely).*/
	if (wasVisible) Window_SetVisible(true);

	Window_SetWindowState(state);
}

static void Window_SetHiddenBorder(bool hidden) {
	suppress_resize++;
	Window_DoSetHiddenBorder(hidden);
	Window_ProcessEvents();
	suppress_resize--;
}

static CC_INLINE void Window_SetRect(Rect2D* dst, const RECT* src) {
	dst->X = src->left;
	dst->Y = src->top;
	dst->Width  = src->right  - src->left;
	dst->Height = src->bottom - src->top;
}

static void Window_RefreshBounds(void) {
	RECT rect;
	POINT topLeft = { 0, 0 };

	GetWindowRect(win_handle, &rect);
	Window_SetRect(&Window_Bounds, &rect);
	GetClientRect(win_handle, &rect);
	Window_SetRect(&Window_ClientBounds, &rect);

	/* MSDN says GetClientRect always returns 0,0 for top left */
	ClientToScreen(win_handle, &topLeft);
	Window_ClientBounds.X = topLeft.x;
	Window_ClientBounds.Y = topLeft.y;
}

static LRESULT CALLBACK Window_Procedure(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) {
	char keyChar;
	bool wasFocused;
	float wheelDelta;

	switch (message) {
	case WM_ACTIVATE:
		wasFocused     = Window_Focused;
		Window_Focused = LOWORD(wParam) != 0;

		if (Window_Focused != wasFocused) {
			Event_RaiseVoid(&WindowEvents.FocusChanged);
		}
		break;

	case WM_ERASEBKGND:
		Event_RaiseVoid(&WindowEvents.Redraw);
		return 1;

	case WM_WINDOWPOSCHANGED:
	{
		WINDOWPOS* pos = (WINDOWPOS*)lParam;
		if (pos->hwnd != win_handle) break;

		bool moved = pos->x  != Window_Bounds.X     || pos->y  != Window_Bounds.Y;
		bool sized = pos->cx != Window_Bounds.Width || pos->cy != Window_Bounds.Height;

		if (moved) {
			Window_RefreshBounds();
			Event_RaiseVoid(&WindowEvents.Moved);
		}

		if (sized) {
			Window_RefreshBounds();
			SetWindowPos(win_handle, NULL,
				Window_Bounds.X, Window_Bounds.Y, Window_Bounds.Width, Window_Bounds.Height,
				SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

			if (suppress_resize <= 0) {
				Event_RaiseVoid(&WindowEvents.Resized);
			}
		}
	} break;

	case WM_STYLECHANGED:
		if (wParam == GWL_STYLE) {
			DWORD style = ((STYLESTRUCT*)lParam)->styleNew;
			if (style & WS_POPUP) {
				win_hiddenBorder = true;
			} else if (style & WS_THICKFRAME) {
				win_hiddenBorder = false;
			}
		}
		break;

	case WM_SIZE:
	{
		int new_state = win_state;
		switch (wParam) {
		case SIZE_RESTORED:  new_state = WINDOW_STATE_NORMAL; break;
		case SIZE_MINIMIZED: new_state = WINDOW_STATE_MINIMISED; break;
		case SIZE_MAXIMIZED: new_state = win_hiddenBorder ? WINDOW_STATE_FULLSCREEN : WINDOW_STATE_MAXIMISED; break;
		}

		if (new_state != win_state) {
			win_state = new_state;
			Event_RaiseVoid(&WindowEvents.StateChanged);
		}
	} break;


	case WM_CHAR:
		if (Convert_TryUnicodeToCP437((Codepoint)wParam, &keyChar)) {
			Event_RaiseInt(&KeyEvents.Press, keyChar);
		}
		break;

	case WM_MOUSEMOVE:
		/* set before position change, in case mouse buttons changed when outside window */
		Mouse_SetPressed(MOUSE_LEFT,   (wParam & 0x01) != 0);
		Mouse_SetPressed(MOUSE_RIGHT,  (wParam & 0x02) != 0);
		Mouse_SetPressed(MOUSE_MIDDLE, (wParam & 0x10) != 0);
		/* TODO: do we need to set XBUTTON1/XBUTTON2 here */
		Mouse_SetPosition(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_MOUSEWHEEL:
		wheelDelta = ((short)HIWORD(wParam)) / (float)WHEEL_DELTA;
		Mouse_SetWheel(Mouse_Wheel + wheelDelta);
		return 0;

	case WM_LBUTTONDOWN:
		Mouse_SetPressed(MOUSE_LEFT,   true);
		break;
	case WM_MBUTTONDOWN:
		Mouse_SetPressed(MOUSE_MIDDLE, true);
		break;
	case WM_RBUTTONDOWN:
		Mouse_SetPressed(MOUSE_RIGHT,  true);
		break;
	case WM_XBUTTONDOWN:
		Key_SetPressed(HIWORD(wParam) == 1 ? KEY_XBUTTON1 : KEY_XBUTTON2, true);
		break;
	case WM_LBUTTONUP:
		Mouse_SetPressed(MOUSE_LEFT,   false);
		break;
	case WM_MBUTTONUP:
		Mouse_SetPressed(MOUSE_MIDDLE, false);
		break;
	case WM_RBUTTONUP:
		Mouse_SetPressed(MOUSE_RIGHT,  false);
		break;
	case WM_XBUTTONUP:
		Key_SetPressed(HIWORD(wParam) == 1 ? KEY_XBUTTON1 : KEY_XBUTTON2, false);
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	{
		bool pressed = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
		/* Shift/Control/Alt behave strangely when e.g. ShiftRight is held down and ShiftLeft is pressed
		and released. It looks like neither key is released in this case, or that the wrong key is
		released in the case of Control and Alt.
		To combat this, we are going to release both keys when either is released. Hacky, but should work.
		Win95 does not distinguish left/right key constants (GetAsyncKeyState returns 0).
		In this case, both keys will be reported as pressed.	*/
		bool ext = (lParam & (1UL << 24)) != 0;

		bool lShiftDown, rShiftDown;
		Key mappedKey;
		switch (wParam)
		{
		case VK_SHIFT:
			/* The behavior of this key is very strange. Unlike Control and Alt, there is no extended bit
			to distinguish between left and right keys. Moreover, pressing both keys and releasing one
			may result in both keys being held down (but not always).*/
			lShiftDown = ((USHORT)GetKeyState(VK_LSHIFT)) >> 15;
			rShiftDown = ((USHORT)GetKeyState(VK_RSHIFT)) >> 15;

			if (!pressed || lShiftDown != rShiftDown) {
				Key_SetPressed(KEY_LSHIFT,  lShiftDown);
				Key_SetPressed(KEY_RSHIFT, rShiftDown);
			}
			return 0;

		case VK_CONTROL:
			Key_SetPressed(ext ? KEY_RCTRL : KEY_LCTRL, pressed);
			return 0;
		case VK_MENU:
			Key_SetPressed(ext ? KEY_RALT  : KEY_LALT,  pressed);
			return 0;
		case VK_RETURN:
			Key_SetPressed(ext ? KEY_KP_ENTER : KEY_ENTER, pressed);
			return 0;

		default:
			mappedKey = Window_MapKey(wParam);
			if (mappedKey != KEY_NONE) {
				Key_SetPressed(mappedKey, pressed);
			}
			return 0;
		}
	} break;

	case WM_SYSCHAR:
		return 0;

	case WM_KILLFOCUS:
		/* TODO: Keep track of keyboard when focus is lost */
		Key_Clear();
		break;

	case WM_CREATE:
		invisible_since_creation = true;
		break;

	case WM_CLOSE:
		Event_RaiseVoid(&WindowEvents.Closing);
		if (Window_Exists) DestroyWindow(win_handle);
		Window_Exists = false;
		break;

	case WM_DESTROY:
		Window_Exists = false;
		UnregisterClass(CC_WIN_CLASSNAME, win_instance);

		if (win_DC) ReleaseDC(win_handle, win_DC);
		Event_RaiseVoid(&WindowEvents.Destroyed);
		break;
	}
	return DefWindowProc(handle, message, wParam, lParam);
}


/*########################################################################################################################*
*--------------------------------------------------Public implementation--------------------------------------------------*
*#########################################################################################################################*/
void Window_Create(int x, int y, int width, int height, struct GraphicsMode* mode) {
	win_instance = GetModuleHandle(NULL);
	/* TODO: UngroupFromTaskbar(); */

	/* Find out the final window rectangle, after the WM has added its chrome (titlebar, sidebars etc). */
	RECT rect = { x, y, x + width, y + height };
	AdjustWindowRect(&rect, CC_WIN_STYLE, false);

	WNDCLASSEX wc = { 0 };
	wc.cbSize    = sizeof(WNDCLASSEX);
	wc.style     = CS_OWNDC;
	wc.hInstance = win_instance;
	wc.lpfnWndProc   = Window_Procedure;
	wc.lpszClassName = CC_WIN_CLASSNAME;

	wc.hIcon   = (HICON)LoadImage(win_instance, MAKEINTRESOURCE(1), IMAGE_ICON,
			GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
	wc.hIconSm = (HICON)LoadImage(win_instance, MAKEINTRESOURCE(1), IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	ATOM atom = RegisterClassEx(&wc);
	if (!atom) Logger_Abort2(GetLastError(), "Failed to register window class");

	win_handle = CreateWindowEx(0, atom, NULL, CC_WIN_STYLE,
		rect.left, rect.top, Rect_Width(rect), Rect_Height(rect),
		NULL, NULL, win_instance, NULL);

	if (!win_handle) Logger_Abort2(GetLastError(), "Failed to create window");
	Window_RefreshBounds();

	win_DC = GetDC(win_handle);
	if (!win_DC) Logger_Abort2(GetLastError(), "Failed to get device context");
	Window_Exists = true;
}

void Window_SetTitle(const String* title) {
	TCHAR str[300]; 
	Platform_ConvertString(str, title);
	SetWindowText(win_handle, str);
}

void Window_GetClipboardText(String* value) {
	/* retry up to 50 times*/
	int i;
	value->length = 0;

	for (i = 0; i < 50; i++) {
		if (!OpenClipboard(win_handle)) {
			Thread_Sleep(10);
			continue;
		}

		bool isUnicode = true;
		HANDLE hGlobal = GetClipboardData(CF_UNICODETEXT);
		if (!hGlobal) {
			hGlobal = GetClipboardData(CF_TEXT);
			isUnicode = false;
		}
		if (!hGlobal) { CloseClipboard(); return; }
		LPVOID src  = GlobalLock(hGlobal);
		SIZE_T size = GlobalSize(hGlobal);

		/* ignore trailing NULL at end */
		/* TODO: Verify it's always there */
		if (isUnicode) {
			Convert_DecodeUtf16(value, (Codepoint*)src, size - 2);
		} else {
			Convert_DecodeAscii(value, (uint8_t*)src,   size - 1);
		}

		GlobalUnlock(hGlobal);
		CloseClipboard();
		return;
	}
}

void Window_SetClipboardText(const String* value) {
	/* retry up to 10 times */
	int i;
	for (i = 0; i < 10; i++) {
		if (!OpenClipboard(win_handle)) {
			Thread_Sleep(100);
			continue;
		}

		HANDLE hGlobal = GlobalAlloc(GMEM_MOVEABLE, (value->length + 1) * 2);
		if (!hGlobal) { CloseClipboard(); return; }

		Codepoint* text = GlobalLock(hGlobal);
		for (i = 0; i < value->length; i++, text++) {
			*text = Convert_CP437ToUnicode(value->buffer[i]);
		}
		*text = '\0';

		GlobalUnlock(hGlobal);
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hGlobal);
		CloseClipboard();
		return;
	}
}


void Window_SetLocation(int x, int y) {
	SetWindowPos(win_handle, NULL, x, y, 0, 0, SWP_NOSIZE);
}

void Window_SetSize(int width, int height) {
	DWORD style = GetWindowLong(win_handle, GWL_STYLE);
	RECT rect   = { 0, 0, width, height };
	AdjustWindowRect(&rect, style, false);

	SetWindowPos(win_handle, NULL, 0, 0, 
				Rect_Width(rect), Rect_Height(rect), SWP_NOMOVE);
}

void* Window_GetWindowHandle(void) { return win_handle; }

bool Window_GetVisible(void) { return IsWindowVisible(win_handle); }
void Window_SetVisible(bool visible) {
	if (visible) {
		ShowWindow(win_handle, SW_SHOW);
		if (invisible_since_creation) {
			BringWindowToTop(win_handle);
			SetForegroundWindow(win_handle);
		}
	} else {
		ShowWindow(win_handle, SW_HIDE);
	}
}


void Window_Close(void) {
	PostMessage(win_handle, WM_CLOSE, 0, 0);
}

int Window_GetWindowState(void) { return win_state; }
void Window_SetWindowState(int state) {
	if (win_state == state) return;

	DWORD command = 0;
	bool exiting_fullscreen = false;

	switch (state) {
	case WINDOW_STATE_NORMAL:
		command = SW_RESTORE;

		/* If we are leaving fullscreen mode we need to restore the border. */
		if (win_state == WINDOW_STATE_FULLSCREEN)
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
		prev_bounds = Window_Bounds;
		Window_SetHiddenBorder(true);

		command = SW_MAXIMIZE;
		SetForegroundWindow(win_handle);
		break;
	}

	if (command != 0) ShowWindow(win_handle, command);

	/* Restore previous window border or apply pending border change when leaving fullscreen mode. */
	if (exiting_fullscreen) Window_SetHiddenBorder(false);

	/* Restore previous window size/location if necessary */
	if (command == SW_RESTORE && (prev_bounds.Width || prev_bounds.Height)) {
		Rect2D rect = prev_bounds;
		/* NOTE: the bounds variable is updated when the resize/move message arrives. */
		SetWindowPos(win_handle, NULL, rect.X, rect.Y, rect.Width, rect.Height, 0);

		prev_bounds.Width = 0; prev_bounds.Height = 0;
	}
}

void Window_ProcessEvents(void) {
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, 1)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	HWND foreground = GetForegroundWindow();
	if (foreground) {
		Window_Focused = foreground == win_handle;
	}
}

Point2D Cursor_GetScreenPos(void) {
	POINT point; GetCursorPos(&point);
	Point2D p = { point.x, point.y }; return p;
}
void Cursor_SetScreenPos(int x, int y) {
	SetCursorPos(x, y);
}
void Cursor_SetVisible(bool visible) {
	win_cursorVisible = visible;
	ShowCursor(visible ? 1 : 0);
}

void Window_ShowDialog(const char* title, const char* msg) {
	MessageBoxA(win_handle, msg, title, 0);
}

static HGDIOBJ draw_DC;
static HBITMAP draw_DIB;
void Window_InitRaw(Bitmap* bmp) {
	BITMAPINFO hdr = { 0 };

	if (!draw_DC) draw_DC = CreateCompatibleDC(win_DC);
	if (draw_DIB) DeleteObject(draw_DIB);
	
	hdr.bmiHeader.biSize = sizeof(BITMAPINFO);
	hdr.bmiHeader.biWidth    =  bmp->Width;
	hdr.bmiHeader.biHeight   = -bmp->Height;
	hdr.bmiHeader.biBitCount = 32;
	hdr.bmiHeader.biPlanes   = 1;

	draw_DIB = CreateDIBSection(draw_DC, &hdr, 0, &bmp->Scan0, NULL, 0);
}

void Window_DrawRaw(Rect2D r) {
	HGDIOBJ oldSrc = SelectObject(draw_DC, draw_DIB);
	BOOL success = BitBlt(win_DC, r.X, r.Y, r.Width, r.Height, draw_DC, r.X, r.Y, SRCCOPY);
	SelectObject(draw_DC, oldSrc);
}


/*########################################################################################################################*
*-----------------------------------------------------OpenGL context------------------------------------------------------*
*#########################################################################################################################*/
#ifndef CC_BUILD_D3D9
void GLContext_SelectGraphicsMode(struct GraphicsMode* mode) {
	PIXELFORMATDESCRIPTOR pfd = { 0 };
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	/* TODO: PFD_SUPPORT_COMPOSITION FLAG? CHECK IF IT WORKS ON XP */
	pfd.cColorBits = mode->R + mode->G + mode->B;

	pfd.iPixelType = mode->IsIndexed ? PFD_TYPE_COLORINDEX : PFD_TYPE_RGBA;
	pfd.cRedBits   = mode->R;
	pfd.cGreenBits = mode->G;
	pfd.cBlueBits  = mode->B;
	pfd.cAlphaBits = mode->A;

	pfd.cDepthBits   = mode->DepthBits;
	pfd.cStencilBits = mode->StencilBits;

	int modeIndex = ChoosePixelFormat(win_DC, &pfd);
	if (modeIndex == 0) { Logger_Abort("Requested graphics mode not available"); }

	Mem_Set(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;

	DescribePixelFormat(win_DC, modeIndex, pfd.nSize, &pfd);
	if (!SetPixelFormat(win_DC, modeIndex, &pfd)) {
		Logger_Abort2(GetLastError(), "SetPixelFormat failed");
	}
}

static HGLRC ctx_Handle;
static HDC ctx_DC;
typedef BOOL (WINAPI *FN_WGLSWAPINTERVAL)(int interval);
static FN_WGLSWAPINTERVAL wglSwapIntervalEXT;
static bool ctx_supports_vSync;

void GLContext_Init(struct GraphicsMode* mode) {
	GLContext_SelectGraphicsMode(mode);
	ctx_Handle = wglCreateContext(win_DC);
	if (!ctx_Handle) {
		ctx_Handle = wglCreateContext(win_DC);
	}
	if (!ctx_Handle) {
		Logger_Abort2(GetLastError(), "Failed to create OpenGL context");
	}

	if (!wglMakeCurrent(win_DC, ctx_Handle)) {
		Logger_Abort2(GetLastError(), "Failed to make OpenGL context current");
	}

	ctx_DC = wglGetCurrentDC();
	wglSwapIntervalEXT = (FN_WGLSWAPINTERVAL)GLContext_GetAddress("wglSwapIntervalEXT");
	ctx_supports_vSync = wglSwapIntervalEXT != NULL;
}

void GLContext_Update(void) { }
void GLContext_Free(void) {
	if (!wglDeleteContext(ctx_Handle)) {
		Logger_Abort2(GetLastError(), "Failed to destroy OpenGL context");
	}
	ctx_Handle = NULL;
}

void* GLContext_GetAddress(const char* function) {
	void* address = wglGetProcAddress(function);
	return GLContext_IsInvalidAddress(address) ? NULL : address;
}

void GLContext_SwapBuffers(void) {
	if (!SwapBuffers(ctx_DC)) {
		Logger_Abort2(GetLastError(), "Failed to swap buffers");
	}
}

void GLContext_SetVSync(bool enabled) {
	if (ctx_supports_vSync) wglSwapIntervalEXT(enabled);
}
#endif
#endif


/*########################################################################################################################*
*-------------------------------------------------------X11 window--------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_X11
#include <X11/Xlib.h>
#include <GL/glx.h>

#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD    1
#define _NET_WM_STATE_TOGGLE 2

static Display* win_display;
static int win_screen;
static Window win_rootWin;

static Window win_handle;
static XVisualInfo win_visual;
static int borderLeft, borderRight, borderTop, borderBottom;
 
static Atom wm_destroy, net_wm_state;
static Atom net_wm_state_minimized;
static Atom net_wm_state_fullscreen;
static Atom net_wm_state_maximized_horizontal;
static Atom net_wm_state_maximized_vertical;
static Atom net_wm_icon, net_frame_extents;

static Atom xa_clipboard, xa_targets, xa_utf8_string, xa_data_sel;
static Atom xa_atom = 4, xa_cardinal = 6;
static long win_eventMask;


/*########################################################################################################################*
*-----------------------------------------------------Private details-----------------------------------------------------*
*#########################################################################################################################*/
static Key Window_MapKey(KeySym key) {
	if (key >= XK_0 && key <= XK_9) { return '0' + (key - XK_0); }
	if (key >= XK_A && key <= XK_Z) { return 'A' + (key - XK_A); }
	if (key >= XK_a && key <= XK_z) { return 'A' + (key - XK_a); }

	if (key >= XK_F1 && key <= XK_F35)    { return KEY_F1  + (key - XK_F1); }
	if (key >= XK_KP_0 && key <= XK_KP_9) { return KEY_KP0 + (key - XK_KP_0); }

	switch (key) {
		case XK_Escape: return KEY_ESCAPE;
		case XK_Return: return KEY_ENTER;
		case XK_space: return KEY_SPACE;
		case XK_BackSpace: return KEY_BACKSPACE;

		case XK_Shift_L: return KEY_LSHIFT;
		case XK_Shift_R: return KEY_RSHIFT;
		case XK_Alt_L: return KEY_LALT;
		case XK_Alt_R: return KEY_RALT;
		case XK_Control_L: return KEY_LCTRL;
		case XK_Control_R: return KEY_RCTRL;
		case XK_Super_L: return KEY_LWIN;
		case XK_Super_R: return KEY_RWIN;
		case XK_Meta_L: return KEY_LWIN;
		case XK_Meta_R: return KEY_RWIN;

		case XK_Menu: return KEY_MENU;
		case XK_Tab: return KEY_TAB;
		case XK_minus: return KEY_MINUS;
		case XK_plus: return KEY_PLUS;
		case XK_equal: return KEY_PLUS;

		case XK_Caps_Lock: return KEY_CAPSLOCK;
		case XK_Num_Lock: return KEY_NUMLOCK;

		case XK_Pause: return KEY_PAUSE;
		case XK_Break: return KEY_PAUSE;
		case XK_Scroll_Lock: return KEY_SCROLLLOCK;
		case XK_Insert: return KEY_INSERT;
		case XK_Print: return KEY_PRINTSCREEN;
		case XK_Sys_Req: return KEY_PRINTSCREEN;

		case XK_backslash: return KEY_BACKSLASH;
		case XK_bar: return KEY_BACKSLASH;
		case XK_braceleft: return KEY_LBRACKET;
		case XK_bracketleft: return KEY_LBRACKET;
		case XK_braceright: return KEY_RBRACKET;
		case XK_bracketright: return KEY_RBRACKET;
		case XK_colon: return KEY_SEMICOLON;
		case XK_semicolon: return KEY_SEMICOLON;
		case XK_quoteright: return KEY_QUOTE;
		case XK_quotedbl: return KEY_QUOTE;
		case XK_quoteleft: return KEY_TILDE;
		case XK_asciitilde: return KEY_TILDE;

		case XK_comma: return KEY_COMMA;
		case XK_less: return KEY_COMMA;
		case XK_period: return KEY_PERIOD;
		case XK_greater: return KEY_PERIOD;
		case XK_slash: return KEY_SLASH;
		case XK_question: return KEY_SLASH;

		case XK_Left: return KEY_LEFT;
		case XK_Down: return KEY_DOWN;
		case XK_Right: return KEY_RIGHT;
		case XK_Up: return KEY_UP;

		case XK_Delete: return KEY_DELETE;
		case XK_Home: return KEY_HOME;
		case XK_End: return KEY_END;
		case XK_Page_Up: return KEY_PAGEUP;
		case XK_Page_Down: return KEY_PAGEDOWN;

		case XK_KP_Add: return KEY_KP_PLUS;
		case XK_KP_Subtract: return KEY_KP_MINUS;
		case XK_KP_Multiply: return KEY_KP_MULTIPLY;
		case XK_KP_Divide: return KEY_KP_DIVIDE;
		case XK_KP_Decimal: return KEY_KP_DECIMAL;
		case XK_KP_Insert: return KEY_KP0;
		case XK_KP_End: return KEY_KP1;
		case XK_KP_Down: return KEY_KP2;
		case XK_KP_Page_Down: return KEY_KP3;
		case XK_KP_Left: return KEY_KP4;
		case XK_KP_Begin: return KEY_KP5;
		case XK_KP_Right: return KEY_KP6;
		case XK_KP_Home: return KEY_KP7;
		case XK_KP_Up: return KEY_KP8;
		case XK_KP_Page_Up: return KEY_KP9;
		case XK_KP_Delete: return KEY_KP_DECIMAL;
		case XK_KP_Enter: return KEY_KP_ENTER;
	}
	return KEY_NONE;
}

static void Window_RegisterAtoms(void) {
	Display* display = win_display;
	wm_destroy = XInternAtom(display, "WM_DELETE_WINDOW", true);
	net_wm_state = XInternAtom(display, "_NET_WM_STATE", false);
	net_wm_state_minimized  = XInternAtom(display, "_NET_WM_STATE_MINIMIZED",  false);
	net_wm_state_fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", false);
	net_wm_state_maximized_horizontal = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", false);
	net_wm_state_maximized_vertical   = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", false);
	net_wm_icon = XInternAtom(display, "_NEW_WM_ICON", false);
	net_frame_extents = XInternAtom(display, "_NET_FRAME_EXTENTS", false);

	xa_clipboard   = XInternAtom(display, "CLIPBOARD",   false);
	xa_targets     = XInternAtom(display, "TARGETS",     false);
	xa_utf8_string = XInternAtom(display, "UTF8_STRING", false);
	xa_data_sel    = XInternAtom(display, "CC_SEL_DATA", false);
}

static void Window_RefreshBorders(void) {
	Atom prop_type;
	int prop_format;
	unsigned long items, after;
	long* borders = NULL;
	
	XGetWindowProperty(win_display, win_handle, net_frame_extents, 0, 16, false,
		xa_cardinal, &prop_type, &prop_format, &items, &after, &borders);

	if (!borders) return;
	if (items == 4) {
		borderLeft = borders[0]; borderRight = borders[1];
		borderTop = borders[2]; borderBottom = borders[3];
	}
	XFree(borders);
}

static void Window_RefreshBounds(const XConfigureEvent* e) {
	Window_RefreshBorders();

	if (e->x != Window_ClientBounds.X || e->y != Window_ClientBounds.Y) {
		Window_ClientBounds.X = e->x; Window_ClientBounds.Y = e->y;

		/* To get the external (window) position, need to add the border */
		Window_Bounds.X = e->x - borderLeft;
		Window_Bounds.Y = e->y - borderTop;
		Event_RaiseVoid(&WindowEvents.Moved);
	}

	if (e->width != Window_ClientBounds.Width || e->height != Window_ClientBounds.Height) {
		Window_ClientBounds.Width  = e->width;
		Window_ClientBounds.Height = e->height;

		/* To get the external (window) size, need to add the border size */
		Window_Bounds.Width  = e->width  + borderLeft + borderRight;
		Window_Bounds.Height = e->height + borderTop  + borderBottom;
		Event_RaiseVoid(&WindowEvents.Resized);
	}
}


/*########################################################################################################################*
*--------------------------------------------------Public implementation--------------------------------------------------*
*#########################################################################################################################*/
static XVisualInfo GLContext_SelectVisual(struct GraphicsMode* mode);
void Window_Create(int x, int y, int width, int height, struct GraphicsMode* mode) {
	XSetWindowAttributes attributes = { 0 };
	XSizeHints hints = { 0 };
	uintptr_t addr;
	bool supported;

	win_display = DisplayDevice_Meta;
	win_screen  = DefaultScreen(win_display);
	win_rootWin = RootWindow(win_display, win_screen);

	/* Open a display connection to the X server, and obtain the screen and root window */
	addr = (uintptr_t)win_display;
	Platform_Log3("Display: %x, Screen %i, Root window: %h", &addr, &win_screen, &win_rootWin);
	Window_RegisterAtoms();

	win_eventMask = StructureNotifyMask /*| SubstructureNotifyMask*/ | ExposureMask |
		KeyReleaseMask  | KeyPressMask    | KeymapStateMask   | PointerMotionMask |
		FocusChangeMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask |
		LeaveWindowMask | PropertyChangeMask;
	win_visual = GLContext_SelectVisual(mode);

	Platform_LogConst("Opening render window... ");
	attributes.colormap   = XCreateColormap(win_display, win_rootWin, win_visual.visual, AllocNone);
	attributes.event_mask = win_eventMask;

	win_handle = XCreateWindow(win_display, win_rootWin, x, y, width, height,
		0, win_visual.depth /* CopyFromParent*/, InputOutput, win_visual.visual, 
		CWColormap | CWEventMask | CWBackPixel | CWBorderPixel, &attributes);
	if (!win_handle) Logger_Abort("XCreateWindow failed");

	hints.base_width  = width;
	hints.base_height = height;
	hints.flags = PSize | PPosition;
	XSetWMNormalHints(win_display, win_handle, &hints);

	/* Register for window destroy notification */
	Atom atoms[1] = { wm_destroy };
	XSetWMProtocols(win_display, win_handle, atoms, 1);

	/* Set the initial window size to ensure X, Y, Width, Height and the rest
	   return the correct values inside the constructor and the Load event. */
	XEvent e;
	e.xconfigure.x = x;
	e.xconfigure.y = y;
	e.xconfigure.width = width;
	e.xconfigure.height = height;
	Window_RefreshBounds(&e.xconfigure);

	/* Request that auto-repeat is only set on devices that support it physically.
	   This typically means that it's turned off for keyboards (which is what we want).
	   We prefer this method over XAutoRepeatOff/On, because the latter needs to
	   be reset before the program exits. */
	XkbSetDetectableAutoRepeat(win_display, true, &supported);
	Window_Exists = true;
}

void Window_SetTitle(const String* title) {
	char str[600]; 
	Platform_ConvertString(str, title);
	XStoreName(win_display, win_handle, str);
}

static char clipboard_copy_buffer[256];
static char clipboard_paste_buffer[256];
static String clipboard_copy_text  = String_FromArray(clipboard_copy_buffer);
static String clipboard_paste_text = String_FromArray(clipboard_paste_buffer);

void Window_GetClipboardText(String* value) {
	Window owner = XGetSelectionOwner(win_display, xa_clipboard);
	int i;
	if (!owner) return; /* no window owner */

	XConvertSelection(win_display, xa_clipboard, xa_utf8_string, xa_data_sel, win_handle, 0);
	clipboard_paste_text.length = 0;

	/* wait up to 1 second for SelectionNotify event to arrive */
	for (i = 0; i < 100; i++) {
		Window_ProcessEvents();
		if (clipboard_paste_text.length) {
			String_Copy(value, &clipboard_paste_text);
			return;
		} else {
			Thread_Sleep(10);
		}
	}
}

void Window_SetClipboardText(const String* value) {
	String_Copy(&clipboard_copy_text, value);
	XSetSelectionOwner(win_display, xa_clipboard, win_handle, 0);
}

bool Window_GetVisible(void) {
	XWindowAttributes attr;
	XGetWindowAttributes(win_display, win_handle, &attr);
	return attr.map_state == IsViewable;
}

void Window_SetVisible(bool visible) {
	if (visible) {
		XMapWindow(win_display, win_handle);
	} else {
		XUnmapWindow(win_display, win_handle);
	}
}

void* Window_GetWindowHandle(void) { return win_handle; }

int Window_GetWindowState(void) {
	Atom prop_type;
	unsigned long items, after;
	int prop_format;
	Atom* data = NULL;

	XGetWindowProperty(win_display, win_handle,
		net_wm_state, 0, 256, false, xa_atom, &prop_type,
		&prop_format, &items, &after, &data);

	bool fullscreen = false, minimised = false;
	int maximised = 0, i;

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

static void Window_SendNetWMState(long op, Atom a1, Atom a2) {
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

void Window_SetWindowState(int state) {
	int current_state = Window_GetWindowState();
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

void Window_SetLocation(int x, int y) {
	XMoveWindow(win_display, win_handle, x, y);
	Window_ProcessEvents();
}

void Window_SetSize(int width, int height) {
	XResizeWindow(win_display, win_handle, width, height);
	Window_ProcessEvents();
}

void Window_Close(void) {
	XEvent ev = { 0 };
	ev.type = ClientMessage;
	ev.xclient.format  = 32;
	ev.xclient.display = win_display;
	ev.xclient.window  = win_handle;
	ev.xclient.data.l[0] = wm_destroy;

	XSendEvent(win_display, win_handle, false, 0, &ev);
	XFlush(win_display);
}

static void Window_ToggleKey(XKeyEvent* keyEvent, bool pressed) {
	KeySym keysym1 = XLookupKeysym(keyEvent, 0);
	KeySym keysym2 = XLookupKeysym(keyEvent, 1);

	Key key = Window_MapKey(keysym1);
	if (key == KEY_NONE) key = Window_MapKey(keysym2);
	if (key != KEY_NONE) Key_SetPressed(key, pressed);
}

static Atom Window_GetSelectionProperty(XEvent* e) {
	Atom prop = e->xselectionrequest.property;
	if (prop) return prop;

	/* For obsolete clients. See ICCCM spec, selections chapter for reasoning. */
	return e->xselectionrequest.target;
}

static bool Window_GetPendingEvent(XEvent* e) {
	return XCheckWindowEvent(win_display,   win_handle, win_eventMask, e) ||
		XCheckTypedWindowEvent(win_display, win_handle, ClientMessage, e) ||
		XCheckTypedWindowEvent(win_display, win_handle, SelectionNotify, e) ||
		XCheckTypedWindowEvent(win_display, win_handle, SelectionRequest, e);
}

void Window_ProcessEvents(void) {
	XEvent e;
	bool wasFocused;

	while (Window_Exists) {
		if (!Window_GetPendingEvent(&e)) break;

		switch (e.type) {
		case MapNotify:
		case UnmapNotify:
			Event_RaiseVoid(&WindowEvents.VisibilityChanged);
			break;

		case ClientMessage:
			if (e.xclient.data.l[0] != wm_destroy) break;
			Platform_LogConst("Exit message received.");			
			Event_RaiseVoid(&WindowEvents.Closing);

			/* sync and discard all events queued */
			XSync(win_display, true);
			XDestroyWindow(win_display, win_handle);
			Window_Exists = false;
			break;

		case DestroyNotify:
			Platform_LogConst("Window destroyed");
			Window_Exists = false;
			Event_RaiseVoid(&WindowEvents.Destroyed);
			break;

		case ConfigureNotify:
			Window_RefreshBounds(&e.xconfigure);
			break;

		case Expose:
			if (e.xexpose.count == 0) {
				Event_RaiseVoid(&WindowEvents.Redraw);
			}
			break;

		case KeyPress:
		{
			Window_ToggleKey(&e.xkey, true);
			char data[16];
			int status = XLookupString(&e.xkey, data, Array_Elems(data), NULL, NULL);

			/* TODO: Does this work for every non-english layout? works for latin keys (e.g. finnish) */
			char raw; int i;
			for (i = 0; i < status; i++) {
				if (!Convert_TryUnicodeToCP437((uint8_t)data[i], &raw)) continue;
				Event_RaiseInt(&KeyEvents.Press, raw);
			}
		} break;

		case KeyRelease:
			/* TODO: raise KeyPress event. Use code from */
			/* http://anonsvn.mono-project.com/viewvc/trunk/mcs/class/Managed.Windows.Forms/System.Windows.Forms/X11Keyboard.cs?view=markup */
			Window_ToggleKey(&e.xkey, false);
			break;

		case ButtonPress:
			if (e.xbutton.button == 1)      Mouse_SetPressed(MOUSE_LEFT,   true);
			else if (e.xbutton.button == 2) Mouse_SetPressed(MOUSE_MIDDLE, true);
			else if (e.xbutton.button == 3) Mouse_SetPressed(MOUSE_RIGHT,  true);
			else if (e.xbutton.button == 4) Mouse_SetWheel(Mouse_Wheel + 1);
			else if (e.xbutton.button == 5) Mouse_SetWheel(Mouse_Wheel - 1);
			else if (e.xbutton.button == 6) Key_SetPressed(KEY_XBUTTON1, true);
			else if (e.xbutton.button == 7) Key_SetPressed(KEY_XBUTTON2, true);
			break;

		case ButtonRelease:
			if (e.xbutton.button == 1)      Mouse_SetPressed(MOUSE_LEFT, false);
			else if (e.xbutton.button == 2) Mouse_SetPressed(MOUSE_MIDDLE, false);
			else if (e.xbutton.button == 3) Mouse_SetPressed(MOUSE_RIGHT,  false);
			else if (e.xbutton.button == 6) Key_SetPressed(KEY_XBUTTON1, false);
			else if (e.xbutton.button == 7) Key_SetPressed(KEY_XBUTTON2, false);
			break;

		case MotionNotify:
			Mouse_SetPosition(e.xmotion.x, e.xmotion.y);
			break;

		case FocusIn:
		case FocusOut:
			/* Don't lose focus when another app grabs key or mouse */
			if (e.xfocus.mode == NotifyGrab || e.xfocus.mode == NotifyUngrab) break;
			wasFocused     = Window_Focused;
			Window_Focused = e.type == FocusIn;

			if (Window_Focused != wasFocused) {
				Event_RaiseVoid(&WindowEvents.FocusChanged);
			}
			/* TODO: Keep track of keyboard when focus is lost */
			if (!Window_Focused) Key_Clear();
			break;

		case MappingNotify:
			if (e.xmapping.request == MappingModifier || e.xmapping.request == MappingKeyboard) {
				Platform_LogConst("keybard mapping refreshed");
				XRefreshKeyboardMapping(&e.xmapping);
			}
			break;

		case PropertyNotify:
			if (e.xproperty.atom == net_wm_state) {
				Event_RaiseVoid(&WindowEvents.StateChanged);
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
				unsigned long items, after;
				uint8_t* data = NULL;

				XGetWindowProperty(win_display, win_handle, xa_data_sel, 0, 1024, false, 0,
					&prop_type, &prop_format, &items, &after, &data);
				XDeleteProperty(win_display, win_handle, xa_data_sel);

				if (data && items && prop_type == xa_utf8_string) {
					clipboard_paste_text.length = 0;
					Convert_DecodeUtf8(&clipboard_paste_text, data, items);
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
				char str[800];
				int len = Platform_ConvertString(str, &clipboard_copy_text);

				XChangeProperty(win_display, reply.xselection.requestor, reply.xselection.property, xa_utf8_string, 8,
					PropModeReplace, str, len);
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

Point2D Cursor_GetScreenPos(void) {
	Window rootW, childW;
	Point2D root, child;
	unsigned int mask;

	XQueryPointer(win_display, win_rootWin, &rootW, &childW, &root.X, &root.Y, &child.X, &child.Y, &mask);
	return root;
}

void Cursor_SetScreenPos(int x, int y) {
	XWarpPointer(win_display, None, win_rootWin, 0, 0, 0, 0, x, y);
	XFlush(win_display); /* TODO: not sure if XFlush call is necessary */
}

void Cursor_SetVisible(bool visible) {
	static Cursor blankCursor;
	win_cursorVisible = visible;

	if (visible) {
		XUndefineCursor(win_display, win_handle);
	} else {
		if (!blankCursor) {
			char data  = 0;
			XColor col = { 0 };
			Pixmap pixmap = XCreateBitmapFromData(win_display, win_handle, &data, 1, 1);
			blankCursor   = XCreatePixmapCursor(win_display, pixmap, pixmap, &col, &col, 0, 0);
			XFreePixmap(win_display, pixmap);
		}
		XDefineCursor(win_display, win_handle, blankCursor);
	}
}


/*########################################################################################################################*
*-----------------------------------------------------X11 message box-----------------------------------------------------*
*#########################################################################################################################*/
static Display* dpy;
static unsigned long X11_Col(uint8_t r, uint8_t g, uint8_t b) {
    Colormap cmap = XDefaultColormap(dpy, DefaultScreen(dpy));
    XColor col = { 0 };
    col.red   = r << 8;
    col.green = g << 8;
    col.blue  = b << 8;
    col.flags = DoRed | DoGreen | DoBlue;

    XAllocColor(dpy, cmap, &col);
    return col.pixel;
}

typedef struct {
    Window win;
    GC gc;
    unsigned long white, black, background;
    unsigned long btnBorder, highlight, shadow;
} X11Window;

static void X11Window_Init(X11Window* w) {
    w->black = BlackPixel(dpy, DefaultScreen(dpy));
    w->white = WhitePixel(dpy, DefaultScreen(dpy));
    w->background = X11_Col(206, 206, 206);

    w->btnBorder = X11_Col(60,  60,  60);
    w->highlight = X11_Col(144, 144, 144);
    w->shadow    = X11_Col(49,  49,  49);

    w->win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 100, 100,
                                0, w->black, w->background);
    XSelectInput(dpy, w->win, ExposureMask     | StructureNotifyMask |
                               KeyReleaseMask  | PointerMotionMask |
                               ButtonPressMask | ButtonReleaseMask );

    w->gc = XCreateGC(dpy, w->win, 0, NULL);
    XSetForeground(dpy, w->gc, w->black);
    XSetBackground(dpy, w->gc, w->background);
}

static void X11Window_Free(X11Window* w) {
    XFreeGC(dpy, w->gc);
    XDestroyWindow(dpy, w->win);
}

typedef struct {
    int X, Y, Width, Height;
    int LineHeight, Descent;
    const char* Text;
} X11Textbox;

static void X11Textbox_Measure(X11Textbox* t, XFontStruct* font) {
    String str = String_FromReadonly(t->Text);
    XCharStruct overall;
	int direction, ascent, descent;
	int end, len, lines = 0;

    for (end = 0; end >= 0; lines++) {
        end = String_IndexOf(&str, '\n');
		len = end == -1 ? str.length : end;

        XTextExtents(font, str.buffer, len, &direction, &ascent, &descent, &overall);
        t->Width = max(overall.width, t->Width);
        if (end >= 0) str = String_UNSAFE_SubstringAt(&str, end + 1);
    }

    t->LineHeight = ascent + descent;
    t->Descent    = descent;
    t->Height     = t->LineHeight * lines;
}

static void X11Textbox_Draw(X11Textbox* t, X11Window* w) {
    String str = String_FromReadonly(t->Text);
    int y = t->Y + t->LineHeight - t->Descent; /* TODO: is -Descent even right? */
	int end, len;

    for (end = 0; end >= 0; y += t->LineHeight) {
        end = String_IndexOf(&str, '\n');
		len = end == -1 ? str.length : end;

        XDrawString(dpy, w->win, w->gc, t->X, y, str.buffer, len);
        if (end >= 0) str = String_UNSAFE_SubstringAt(&str, end + 1);
    }
}

typedef struct {
    int X, Y, Width, Height;
    bool Clicked;
    X11Textbox Text;
} X11Button;

static void X11Button_Draw(X11Button* b, X11Window* w) {
	X11Textbox* t;
	int begX, endX, begY, endY;

    XSetForeground(dpy, w->gc, w->btnBorder);
    XDrawRectangle(dpy, w->win, w->gc, b->X, b->Y,
                    b->Width, b->Height);

	t = &b->Text;
	begX = b->X + 1; endX = b->X + b->Width - 1;
	begY = b->Y + 1; endY = b->Y + b->Height - 1;

    if (b->Clicked) {
        XSetForeground(dpy, w->gc, w->highlight);
        XDrawRectangle(dpy, w->win, w->gc, begX, begY,
                        endX - begX, endY - begY);
    } else {
        XSetForeground(dpy, w->gc, w->white);
        XDrawLine(dpy, w->win, w->gc, begX, begY,
                    endX - 1, begY);
        XDrawLine(dpy, w->win, w->gc, begX, begY,
                    begX, endY - 1);

        XSetForeground(dpy, w->gc, w->highlight);
        XDrawLine(dpy, w->win, w->gc, begX + 1, endY - 1,
                    endX - 1, endY - 1);
        XDrawLine(dpy, w->win, w->gc, endX - 1, begY + 1,
                    endX - 1, endY - 1);

        XSetForeground(dpy, w->gc, w->shadow);
        XDrawLine(dpy, w->win, w->gc, begX, endY, endX, endY);
        XDrawLine(dpy, w->win, w->gc, endX, begY, endX, endY);
    }

    XSetForeground(dpy, w->gc, w->black);
    t->X = b->X + b->Clicked + (b->Width  - t->Width)  / 2;
    t->Y = b->Y + b->Clicked + (b->Height - t->Height) / 2;
    X11Textbox_Draw(t, w);
}

static int X11Button_Contains(X11Button* b, int x, int y) {
    return x >= b->X && x < (b->X + b->Width) &&
           y >= b->Y && y < (b->Y + b->Height);
}

static void X11_MessageBox(const char* title, const char* text, X11Window* w) {
    X11Button ok    = { 0 };
    X11Textbox body = { 0 };

	XFontStruct* font;
	Atom wmDelete;
	int x, y, width, height;
	XSizeHints hints = { 0 };
	int mouseX = -1, mouseY = -1, over;
	XEvent e;

    X11Window_Init(w);
    XMapWindow(dpy, w->win);
    XStoreName(dpy, w->win, title);

    wmDelete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, w->win, &wmDelete, 1);

    font = XQueryFont(dpy, XGContextFromGC(w->gc));
    if (!font) return;

    /* Compute size of widgets */
    body.Text = text;
    X11Textbox_Measure(&body, font);
    ok.Text.Text = "OK";
    X11Textbox_Measure(&ok.Text, font);
    ok.Width  = ok.Text.Width  + 70;
    ok.Height = ok.Text.Height + 10;

    /* Compute size and position of window */
    width  = body.Width                   + 20;
    height = body.Height + 20 + ok.Height + 20;
    x = DisplayWidth (dpy, DefaultScreen(dpy))/2 -  width/2;
    y = DisplayHeight(dpy, DefaultScreen(dpy))/2 - height/2;
    XMoveResizeWindow(dpy, w->win, x, y, width, height);

    /* Adjust bounds of widgets */
    body.X = 10; body.Y = 10;
    ok.X = width/2 - ok.Width/2;
    ok.Y = height  - ok.Height - 10;

    XFreeFontInfo(NULL, font, 1);
    XUnmapWindow(dpy, w->win); /* Make window non resizeable */

    hints.flags      = PSize | PMinSize | PMaxSize;
    hints.min_width  = hints.max_width  = hints.base_width  = width;
    hints.min_height = hints.max_height = hints.base_height = height;

    XSetWMNormalHints(dpy, w->win, &hints);
    XMapRaised(dpy, w->win);
    XFlush(dpy);

    for (;;) {
        XNextEvent(dpy, &e);

        switch (e.type)
        {
        case ButtonPress:
        case ButtonRelease:
            if (e.xbutton.button != Button1) break;
            over = X11Button_Contains(&ok, mouseX, mouseY);

            if (ok.Clicked && e.type == ButtonRelease) {
                if (over) return;
            }
            ok.Clicked = e.type == ButtonPress && over;
            /* fallthrough to redraw window */

        case Expose:
        case MapNotify:
            XClearWindow(dpy, w->win);
            X11Textbox_Draw(&body, w);
            X11Button_Draw(&ok, w);
            XFlush(dpy);
            break;

        case KeyRelease:
            if (XLookupKeysym(&e.xkey, 0) == XK_Escape) return;
            break;

        case ClientMessage:
            if (e.xclient.data.l[0] == wmDelete) return;
            break;

        case MotionNotify:
            mouseX = e.xmotion.x; mouseY = e.xmotion.y;
            break;
        }
    }
}

void Window_ShowDialog(const char* title, const char* msg) {
	X11Window w = { 0 };
	dpy = DisplayDevice_Meta;

	X11_MessageBox(title, msg, &w);
	X11Window_Free(&w);
}

static GC win_gc;
static XImage* win_image;
void Window_InitRaw(Bitmap* bmp) {
	if (!win_gc) win_gc = XCreateGC(win_display, win_handle, 0, NULL);
	if (win_image) XFree(win_image);

	Mem_Free(bmp->Scan0);
	bmp->Scan0 = Mem_Alloc(bmp->Width * bmp->Height, 4, "window pixels");

	win_image = XCreateImage(win_display, win_visual.visual,
		win_visual.depth, ZPixmap, 0, bmp->Scan0,
		bmp->Width, bmp->Height, 32, 0);
}

void Window_DrawRaw(Rect2D r) {
	XPutImage(win_display, win_handle, win_gc, win_image,
		r.X, r.Y, r.X, r.Y, r.Width, r.Height);
}


/*########################################################################################################################*
*-----------------------------------------------------OpenGL context------------------------------------------------------*
*#########################################################################################################################*/
static GLXContext ctx_Handle;
typedef int (*FN_GLXSWAPINTERVAL)(int interval);
static FN_GLXSWAPINTERVAL swapIntervalMESA, swapIntervalSGI;
static bool ctx_supports_vSync;

void GLContext_Init(struct GraphicsMode* mode) {
	const static String ext_mesa = String_FromConst("GLX_MESA_swap_control");
	const static String ext_sgi  = String_FromConst("GLX_SGI_swap_control");

	const char* raw_exts;
	String exts;
	ctx_Handle = glXCreateContext(win_display, &win_visual, NULL, true);

	if (!ctx_Handle) {
		Platform_LogConst("Context create failed. Trying indirect...");
		ctx_Handle = glXCreateContext(win_display, &win_visual, NULL, false);
	}
	if (!ctx_Handle) Logger_Abort("Failed to create OpenGL context");

	if (!glXIsDirect(win_display, ctx_Handle)) {
		Platform_LogConst("== WARNING: Context is not direct ==");
	}
	if (!glXMakeCurrent(win_display, win_handle, ctx_Handle)) {
		Logger_Abort("Failed to make OpenGL context current.");
	}

	/* GLX may return non-null function pointers that don't actually work */
	/* So we need to manually check the extensions string for support */
	raw_exts = glXQueryExtensionsString(win_display, win_screen);
	exts = String_FromReadonly(raw_exts);

	if (String_CaselessContains(&exts, &ext_mesa)) {
		swapIntervalMESA = (FN_GLXSWAPINTERVAL)GLContext_GetAddress("glXSwapIntervalMESA");
	}
	if (String_CaselessContains(&exts, &ext_sgi)) {
		swapIntervalSGI  = (FN_GLXSWAPINTERVAL)GLContext_GetAddress("glXSwapIntervalSGI");
	}
	ctx_supports_vSync = swapIntervalMESA || swapIntervalSGI;
}

void GLContext_Update(void) { }
void GLContext_Free(void) {
	if (!ctx_Handle) return;

	if (glXGetCurrentContext() == ctx_Handle) {
		glXMakeCurrent(win_display, None, NULL);
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
	int res;
	if (!ctx_supports_vSync) return;

	if (swapIntervalMESA) {
		res = swapIntervalMESA(enabled);
	} else {
		res = swapIntervalSGI(enabled);
	}
	if (res) Platform_Log1("Set VSync failed, error: %i", &res);
}

static void GLContext_GetAttribs(struct GraphicsMode* mode, int* attribs) {
	int i = 0;
	/* See http://www-01.ibm.com/support/knowledgecenter/ssw_aix_61/com.ibm.aix.opengl/doc/openglrf/glXChooseFBConfig.htm%23glxchoosefbconfig */
	/* See http://www-01.ibm.com/support/knowledgecenter/ssw_aix_71/com.ibm.aix.opengl/doc/openglrf/glXChooseVisual.htm%23b5c84be452rree */
	/* for the attribute declarations. Note that the attributes are different than those used in glxChooseVisual */

	if (!mode->IsIndexed) { attribs[i++] = GLX_RGBA; }
	attribs[i++] = GLX_RED_SIZE;   attribs[i++] = mode->R;
	attribs[i++] = GLX_GREEN_SIZE; attribs[i++] = mode->G;
	attribs[i++] = GLX_BLUE_SIZE;  attribs[i++] = mode->B;
	attribs[i++] = GLX_ALPHA_SIZE; attribs[i++] = mode->A;

	if (mode->DepthBits) {
		attribs[i++] = GLX_DEPTH_SIZE;   attribs[i++] = mode->DepthBits;
	}
	if (mode->StencilBits) {
		attribs[i++] = GLX_STENCIL_SIZE; attribs[i++] = mode->StencilBits;
	}

	attribs[i++] = GLX_DOUBLEBUFFER;
	attribs[i++] = 0;
}

static XVisualInfo GLContext_SelectVisual(struct GraphicsMode* mode) {
	int attribs[20];
	int major, minor;
	XVisualInfo* visual = NULL;

	int fbcount;
	GLXFBConfig* fbconfigs;
	XVisualInfo info;

	GLContext_GetAttribs(mode, attribs);	
	if (!glXQueryVersion(win_display, &major, &minor)) {
		Logger_Abort("glXQueryVersion failed");
	}

	if (major >= 1 && minor >= 3) {
		/* ChooseFBConfig returns an array of GLXFBConfig opaque structures */
		fbconfigs = glXChooseFBConfig(win_display, win_screen, attribs, &fbcount);
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
		Logger_Abort("Requested GraphicsMode not available.");
	}

	info = *visual;
	XFree(visual);
	return info;
}
#endif


/*########################################################################################################################*
*------------------------------------------------------Carbon window------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_OSX
#include <AGL/agl.h>

static WindowRef win_handle;
static int win_state;
/* Hacks for fullscreen */
static bool ctx_pendingWindowed, ctx_pendingFullscreen;

#define Rect_Width(rect)  (rect.right  - rect.left)
#define Rect_Height(rect) (rect.bottom - rect.top)

/*########################################################################################################################*
*-----------------------------------------------------Private details-----------------------------------------------------*
*#########################################################################################################################*/
/* Sourced from https://www.meandmark.com/keycodes.html */
const static uint8_t key_map[8 * 16] = {
	'A', 'S', 'D', 'F', 'H', 'G', 'Z', 'X', 'C', 'V', 0, 'B', 'Q', 'W', 'E', 'R',
	'Y', 'T', '1', '2', '3', '4', '6', '5', KEY_PLUS, '9', '7', KEY_MINUS, '8', '0', KEY_RBRACKET, 'O',
	'U', KEY_LBRACKET, 'I', 'P', KEY_ENTER, 'L', 'J', KEY_QUOTE, 'K', KEY_SEMICOLON, KEY_BACKSLASH, KEY_COMMA, KEY_SLASH, 'N', 'M', KEY_PERIOD,
	KEY_TAB, KEY_SPACE, KEY_TILDE, KEY_BACKSPACE, 0, KEY_ESCAPE, 0, 0, 0, KEY_CAPSLOCK, 0, 0, 0, 0, 0, 0,
	0, KEY_KP_DECIMAL, 0, KEY_KP_MULTIPLY, 0, KEY_KP_PLUS, 0, 0, 0, 0, 0, KEY_KP_DIVIDE, KEY_KP_ENTER, 0, KEY_KP_MINUS, 0,
	0, KEY_KP_ENTER, KEY_KP0, KEY_KP1, KEY_KP2, KEY_KP3, KEY_KP4, KEY_KP5, KEY_KP6, KEY_KP7, 0, KEY_KP8, KEY_KP9, 'N', 'M', KEY_PERIOD,
	KEY_F5, KEY_F6, KEY_F7, KEY_F3, KEY_F8, KEY_F9, 0, KEY_F11, 0, KEY_F13, 0, KEY_F14, 0, KEY_F10, 0, KEY_F12,
	'U', KEY_F15, KEY_INSERT, KEY_HOME, KEY_PAGEUP, KEY_DELETE, KEY_F4, KEY_END, KEY_F2, KEY_PAGEDOWN, KEY_F1, KEY_LEFT, KEY_RIGHT, KEY_DOWN, KEY_UP, 0,
};
static Key Window_MapKey(UInt32 key) { return key < Array_Elems(key_map) ? key_map[key] : 0; }
/* TODO: Check these.. */
/*   case 0x37: return KEY_LWIN; */
/*   case 0x38: return KEY_LSHIFT; */
/*   case 0x3A: return KEY_LALT; */
/*   case 0x3B: return Key_ControlLeft; */

/* TODO: Verify these differences from OpenTK */
/*Backspace = 51,  (0x33, KEY_DELETE according to that link)*/
/*Return = 52,     (0x34, ??? according to that link)*/
/*Menu = 110,      (0x6E, ??? according to that link)*/
static CC_INLINE void Window_SetRect(Rect2D* dst, const Rect* src) {
	dst->X = src->left;
	dst->Y = src->top;
	dst->Width = src->right   - src->left;
	dst->Height = src->bottom - src->top;
}

static void Window_RefreshBounds(void) {
	Rect r;
	OSStatus res;
	if (win_state == WINDOW_STATE_FULLSCREEN) return;
	
	res = GetWindowBounds(win_handle, kWindowStructureRgn, &r);
	if (res) Logger_Abort2(res, "Getting window bounds");
	Window_SetRect(&Window_Bounds, &r);
	
	/* TODO: kWindowContentRgn ??? */
	res = GetWindowBounds(win_handle, kWindowGlobalPortRgn, &r);
	if (res) Logger_Abort2(res, "Getting window clientbounds");
	Window_SetRect(&Window_ClientBounds, &r);
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
		if (res) Logger_Abort2(res, "Maximising window");
		break;

	case WINDOW_STATE_NORMAL:
		if (Window_GetWindowState() == WINDOW_STATE_MAXIMISED) {
			idealSize.v = 0; idealSize.h = 0;
			res = ZoomWindowIdeal(win_handle, inZoomIn, &idealSize);
			if (res) Logger_Abort2(res, "Un-maximising window");
		}
		break;

	case WINDOW_STATE_MINIMISED:
		res = CollapseWindow(win_handle, true);
		if (res) Logger_Abort2(res, "Minimising window");
		break;
	}

	Event_RaiseVoid(&WindowEvents.StateChanged);
	Window_RefreshBounds();
	Event_RaiseVoid(&WindowEvents.Resized);
}

static OSStatus Window_ProcessKeyboardEvent(EventRef inEvent) {
	UInt32 kind, code;
	Key key;
	OSStatus res;
	
	kind = GetEventKind(inEvent);
	switch (kind) {
		case kEventRawKeyDown:
		case kEventRawKeyRepeat:
		case kEventRawKeyUp:
			res = GetEventParameter(inEvent, kEventParamKeyCode, typeUInt32,
									NULL, sizeof(UInt32), NULL, &code);
			if (res) Logger_Abort2(res, "Getting key button");

			key = Window_MapKey(code);
			if (!key) { Platform_Log1("Ignoring unmapped key %i", &code); return 0; }

			Key_SetPressed(key, kind != kEventRawKeyUp);
			return eventNotHandledErr;
			
		case kEventRawKeyModifiersChanged:
			res = GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, 
									NULL, sizeof(UInt32), NULL, &code);
			if (res) Logger_Abort2(res, "Getting key modifiers");
			
			Key_SetPressed(KEY_LCTRL,    (code & 0x1000) != 0);
			Key_SetPressed(KEY_LALT,     (code & 0x0800) != 0);
			Key_SetPressed(KEY_LSHIFT,   (code & 0x0200) != 0);
			Key_SetPressed(KEY_LWIN,     (code & 0x0100) != 0);			
			Key_SetPressed(KEY_CAPSLOCK, (code & 0x0400) != 0);
			return eventNotHandledErr;
	}
	return eventNotHandledErr;
}

static OSStatus Window_ProcessWindowEvent(EventRef inEvent) {
	Rect2D old;
	
	switch (GetEventKind(inEvent)) {
		case kEventWindowClose:
			Window_Exists = false;
			Event_RaiseVoid(&WindowEvents.Closing);
			return eventNotHandledErr;
			
		case kEventWindowClosed:
			Window_Exists = false;
			Event_RaiseVoid(&WindowEvents.Destroyed);
			return 0;
			
		case kEventWindowBoundsChanged:
			old = Window_ClientBounds;
			Window_RefreshBounds();
			
			if (old.X != Window_ClientBounds.X || old.Y != Window_ClientBounds.Y) {
				Event_RaiseVoid(&WindowEvents.Moved);
			}
			if (old.Width != Window_ClientBounds.Width || old.Height != Window_ClientBounds.Height) {
				Event_RaiseVoid(&WindowEvents.Resized);
			}
			return eventNotHandledErr;
			
		case kEventWindowActivated:
			Window_Focused = true;
			Event_RaiseVoid(&WindowEvents.FocusChanged);
			return eventNotHandledErr;
			
		case kEventWindowDeactivated:
			Window_Focused = false;
			Event_RaiseVoid(&WindowEvents.FocusChanged);
			return eventNotHandledErr;
	}
	return eventNotHandledErr;
}

static OSStatus Window_ProcessMouseEvent(EventRef inEvent) {
	HIPoint pt;
	Point2D mousePos;
	UInt32 kind;
	bool down;
	EventMouseButton button;
	SInt32 delta;
	OSStatus res;	
	
	res = GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint,
							NULL, sizeof(HIPoint), NULL, &pt);
	/* this error comes up from the application event handler */
	if (res && res != eventParameterNotFoundErr) {
		Logger_Abort2(res, "Getting mouse position");
	}
	
	mousePos.X = (int)pt.x; mousePos.Y = (int)pt.y;
	/* kEventParamMouseLocation is in screen coordinates */
	if (win_state != WINDOW_STATE_FULLSCREEN) {
		mousePos.X -= Window_ClientBounds.X;
		mousePos.Y -= Window_ClientBounds.Y;
	}

	/* mousePos.Y is < 0 if user clicks or moves when over titlebar */
	/* Don't intercept this, prevents clicking close/minimise/maximise from working */
	if (mousePos.Y < 0) return eventNotHandledErr;
	
	kind = GetEventKind(inEvent);
	switch (kind) {
		case kEventMouseDown:
		case kEventMouseUp:
			down = kind == kEventMouseDown;
			res  = GetEventParameter(inEvent, kEventParamMouseButton, typeMouseButton, 
									 NULL, sizeof(EventMouseButton), NULL, &button);
			if (res) Logger_Abort2(res, "Getting mouse button");
			
			switch (button) {
				case kEventMouseButtonPrimary:
					Mouse_SetPressed(MOUSE_LEFT, down); break;
				case kEventMouseButtonSecondary:
					Mouse_SetPressed(MOUSE_RIGHT, down); break;
				case kEventMouseButtonTertiary:
					Mouse_SetPressed(MOUSE_MIDDLE, down); break;
			}
			return eventNotHandledErr;
			
		case kEventMouseWheelMoved:
			res = GetEventParameter(inEvent, kEventParamMouseWheelDelta, typeSInt32,
									NULL, sizeof(SInt32), NULL, &delta);
			if (res) Logger_Abort2(res, "Getting mouse wheel delta");
			Mouse_SetWheel(Mouse_Wheel + delta);
			return 0;
			
		case kEventMouseMoved:
		case kEventMouseDragged:
			Mouse_SetPosition(mousePos.X, mousePos.Y);
			return eventNotHandledErr;
	}
	return eventNotHandledErr;
}

static OSStatus Window_ProcessTextEvent(EventRef inEvent) {
	UInt32 kind;
	UniChar chars[17] = { 0 };
	char keyChar;
	int i;
	OSStatus res;
	
	kind = GetEventKind(inEvent);
	if (kind != kEventTextInputUnicodeForKeyEvent) return eventNotHandledErr;
	
	/* TODO: is the assumption we only get 1-4 characters always valid */
	res = GetEventParameter(inEvent, kEventParamTextInputSendText,
							typeUnicodeText, NULL, 16 * sizeof(UniChar), NULL, chars);
	if (res) Logger_Abort2(res, "Getting text chars");

	for (i = 0; i < 16 && chars[i]; i++) {
		if (Convert_TryUnicodeToCP437(chars[i], &keyChar)) {
			Event_RaiseInt(&KeyEvents.Press, keyChar);
		}
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
			return Window_ProcessKeyboardEvent(inEvent);
		case kEventClassMouse:
			return Window_ProcessMouseEvent(inEvent);
		case kEventClassWindow:
			return Window_ProcessWindowEvent(inEvent);
		case kEventClassTextInput:
			return Window_ProcessTextEvent(inEvent);
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
		
		{ kEventClassTextInput, kEventTextInputUnicodeForKeyEvent },
		{ kEventClassAppleEvent, kEventAppleEvent }
	};
	EventTargetRef target;
	OSStatus res;
	
	target = GetWindowEventTarget(win_handle);
	res = InstallEventHandler(target, NewEventHandlerUPP(Window_EventHandler),
							  Array_Elems(eventTypes), eventTypes, NULL, NULL);
	if (res) Logger_Abort2(res, "Connecting events");
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
						  kWindowInWindowMenuAttribute | kWindowLiveResizeAttribute, &r, &win_handle);

	if (res) Logger_Abort2(res, "Failed to create window");
	Window_RefreshBounds();

	AcquireRootMenu();	
	GetCurrentProcess(&psn);
	SetFrontProcess(&psn);
	
	/* TODO: Use BringWindowToFront instead.. (look in the file which has RepositionWindow in it) !!!! */
	Window_ConnectEvents();
	Window_Exists = true;
}

void Window_SetTitle(const String* title) {
	char str[600];
	CFStringRef titleCF;
	int len;
	
	/* TODO: This leaks memory, old title isn't released */
	len     = Platform_ConvertString(str, title);
	titleCF = CFStringCreateWithBytes(kCFAllocatorDefault, str, len, kCFStringEncodingUTF8, false);
	SetWindowTitleWithCFString(win_handle, titleCF);
}

/* NOTE: All Pasteboard functions are OSX 10.3 or later */
static PasteboardRef Window_GetPasteboard(void) {
	PasteboardRef pbRef;
	OSStatus err = PasteboardCreate(kPasteboardClipboard, &pbRef);
	
	if (err) Logger_Abort2(err, "Creating Pasteboard reference");
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
	CFIndex len;
	OSStatus err;
	
	pbRef = Window_GetPasteboard();

	err = PasteboardGetItemCount(pbRef, &itemCount);
	if (err) Logger_Abort2(err, "Getting item count from Pasteboard");
	if (itemCount < 1) return;
	
	err = PasteboardGetItemIdentifier(pbRef, 1, &itemID);
	if (err) Logger_Abort2(err, "Getting item identifier from Pasteboard");
	
	if (!(err = PasteboardCopyItemFlavorData(pbRef, itemID, FMT_UTF16, &outData))) {	
		ptr = CFDataGetBytePtr(outData);
		len = CFDataGetLength(outData);
		if (ptr) Convert_DecodeUtf16(value, (Codepoint*)ptr, len);
	} else if (!(err = PasteboardCopyItemFlavorData(pbRef, itemID, FMT_UTF8, &outData))) {
		ptr = CFDataGetBytePtr(outData);
		len = CFDataGetLength(outData);
		if (ptr) Convert_DecodeUtf8(value, (uint8_t*)ptr, len);
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
	if (err) Logger_Abort2(err, "Clearing Pasteboard");
	PasteboardSynchronize(pbRef);

	len    = Platform_ConvertString(str, value);
	cfData = CFDataCreate(NULL, str, len);
	if (!cfData) Logger_Abort("CFDataCreate() returned null pointer");

	PasteboardPutItemFlavor(pbRef, 1, FMT_UTF8, cfData, 0);
}
/* TODO: IMPLEMENT void Window_SetIcon(Bitmap* bmp); */

bool Window_GetVisible(void) { return IsWindowVisible(win_handle); }
void Window_SetVisible(bool visible) {
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
		if (err) Logger_Abort2(err, "Un-minimising window");
	}
	Window_UpdateWindowState();
}

void Window_SetLocation(int x, int y) {
	MoveWindow(win_handle, x, y, false);
}

void Window_SetSize(int width, int height) {
	SizeWindow(win_handle, width, height, true);
}

void Window_Close(void) {
	/* DisposeWindow only sends a kEventWindowClosed */
	Event_RaiseVoid(&WindowEvents.Closing);
	if (Window_Exists) DisposeWindow(win_handle);
	Window_Exists = false;
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

Point2D Cursor_GetScreenPos(void) {
	HIPoint point;
	Point2D p;
	/* NOTE: HIGetMousePosition is OSX 10.5 or later */
	/* TODO: Use GetGlobalMouse instead!!!! */
	HIGetMousePosition(kHICoordSpaceScreenPixel, NULL, &point);
	
	p.X = (int)point.x; p.Y = (int)point.y;
	return p;
}

void Cursor_SetScreenPos(int x, int y) {
	CGPoint point;
	point.x = x; point.y = y;
	
	CGAssociateMouseAndMouseCursorPosition(0);
	CGDisplayMoveCursorToPoint(CGMainDisplayID(), point);
	CGAssociateMouseAndMouseCursorPosition(1);
}

void Cursor_SetVisible(bool visible) {
	win_cursorVisible = visible;
	if (visible) {
		CGDisplayShowCursor(CGMainDisplayID());
	} else {
		CGDisplayHideCursor(CGMainDisplayID());
	}
}

void Window_ShowDialog(const char* title, const char* msg) {
	CFStringRef titleCF = CFStringCreateWithCString(NULL, title, kCFStringEncodingASCII);
	CFStringRef msgCF   = CFStringCreateWithCString(NULL, msg,   kCFStringEncodingASCII);
	DialogRef dialog;
	DialogItemIndex itemHit;

	CreateStandardAlert(kAlertPlainAlert, titleCF, msgCF, NULL, &dialog);
	CFRelease(titleCF);
	CFRelease(msgCF);
	RunStandardAlert(dialog, NULL, &itemHit);
}


/* TODO: WORK OUT WHY THIS IS BROKEN!!!!
static CGrafPtr win_winPort;
static CGImageRef win_image;

void Window_InitRaw(Bitmap* bmp) {
	CGColorSpaceRef colorSpace;
	CGDataProviderRef provider;
	
	if (!win_winPort) win_winPort = GetWindowPort(win_handle);
	Mem_Free(bmp->Scan0);
	bmp->Scan0 = Mem_Alloc(bmp->Width * bmp->Height, 4, "window pixels");
	
	colorSpace = CGColorSpaceCreateDeviceRGB();
	provider   = CGDataProviderCreateWithData(NULL, bmp->Scan0, 
					Bitmap_DataSize(bmp->Width, bmp->Height), NULL);
	
	win_image = CGImageCreate(bmp->Width, bmp->Height, 8, 32, bmp->Width * 4, colorSpace, 
					kCGBitmapByteOrder32Little | kCGImageAlphaFirst, provider, NULL, 0, 0);
	
	CGColorSpaceRelease(colorSpace);
	CGDataProviderRelease(provider);
}

void Window_DrawRaw(Rect2D r) {
	CGContextRef context = NULL;
	CGRect rect;
	OSStatus err;
	
	err = QDBeginCGContext(win_winPort, &context);
	if (err) Logger_Abort2(err, "Begin draw");
	
	// TODO: Only update changed bit.. 
	rect.origin.x = 0; rect.origin.y = 0;
	rect.size.width  = Window_ClientBounds.Width;
	rect.size.height = Window_ClientBounds.Height;
	
	CGContextDrawImage(context, rect, win_image);
	CGContextSynchronize(context);
	err = QDEndCGContext(win_winPort, &context);
	if (err) Logger_Abort2(err, "End draw");
}
*/

static CGrafPtr win_winPort;
static CGImageRef win_image;
static Bitmap* bmp_;
static CGColorSpaceRef colorSpace;
static CGDataProviderRef provider;

void Window_InitRaw(Bitmap* bmp) {
	if (!win_winPort) win_winPort = GetWindowPort(win_handle);
	Mem_Free(bmp->Scan0);
	bmp->Scan0 = Mem_Alloc(bmp->Width * bmp->Height, 4, "window pixels");

	colorSpace = CGColorSpaceCreateDeviceRGB();

	bmp_ = bmp;
	//CGColorSpaceRelease(colorSpace);
}

void Window_DrawRaw(Rect2D r) {
	CGContextRef context = NULL;
	CGRect rect;
	OSStatus err;

	err = QDBeginCGContext(win_winPort, &context);
	if (err) Logger_Abort2(err, "Begin draw");
	/* TODO: REPLACE THIS AWFUL HACK */

	/* TODO: Only update changed bit.. */
	rect.origin.x = 0; rect.origin.y = 0;
	rect.size.width  = Window_ClientBounds.Width;
	rect.size.height = Window_ClientBounds.Height;

	provider = CGDataProviderCreateWithData(NULL, bmp_->Scan0,
		Bitmap_DataSize(bmp_->Width, bmp_->Height), NULL);
	win_image = CGImageCreate(bmp_->Width, bmp_->Height, 8, 32, bmp_->Width * 4, colorSpace,
		kCGBitmapByteOrder32Little | kCGImageAlphaFirst, provider, NULL, 0, 0);

	CGContextDrawImage(context, rect, win_image);
	CGContextSynchronize(context);
	err = QDEndCGContext(win_winPort, &context);
	if (err) Logger_Abort2(err, "End draw");

	CGImageRelease(win_image);
	CGDataProviderRelease(provider);
}


/*########################################################################################################################*
*-----------------------------------------------------OpenGL context------------------------------------------------------*
*#########################################################################################################################*/
static AGLContext ctx_handle;
static bool ctx_fullscreen, ctx_firstFullscreen;
static Rect2D ctx_windowedBounds;

static void GLContext_Check(int code, const char* place) {
	ReturnCode res;
	if (code) return;

	res = aglGetError();
	if (res) Logger_Abort2(res, place);
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

	attribs[i++] = AGL_DOUBLEBUFFER;
	if (fullscreen) { attribs[i++] = AGL_FULLSCREEN; }
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
	ctx_windowedBounds = Window_ClientBounds;

	Window_ClientBounds = DisplayDevice_Default.Bounds;
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
	if (res) Logger_Abort2(res, "Getting display device failed");

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
	if (res) Logger_Abort2(res, "Choosing pixel format");

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


/*########################################################################################################################*
*-------------------------------------------------------SDL window--------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_SDL
#include <SDL2/SDL.h>
static SDL_Window* win_handle;

static void Window_RefreshBounds(void) {
	Rect2D r;
	SDL_GetWindowPosition(win_handle, &r.X, &r.Y);
	SDL_GetWindowSize(win_handle, &r.Width, &r.Height);

	Window_ClientBounds = r;
	/* TODO: get border size somehow */
	Window_Bounds       = r;
}

static void Window_SDLFail(const char* place) {
	char strBuffer[256];
	String str;
	String_InitArray_NT(str, strBuffer);

	String_Format2(&str, "Error when %c: %c", place, SDL_GetError());
	str.buffer[str.length] = '\0';
	Logger_Abort(str.buffer);
}

void Window_Create(int x, int y, int width, int height, struct GraphicsMode* mode) {
	/* TODO: Don't set this flag for launcher window */
	win_handle = SDL_CreateWindow(NULL, x, y, width, height, SDL_WINDOW_OPENGL);
	if (!win_handle) Window_SDLFail("creating window");

	Window_Exists = true;
	Window_RefreshBounds();
}

void Window_SetTitle(const String* title) {
	char str[600];
	Platform_ConvertString(str, title);
	SDL_SetWindowTitle(win_handle, str);
}

void Window_GetClipboardText(String* value) {
	char* ptr = SDL_GetClipboardText();
	if (!ptr) return;

	int len = String_CalcLen(ptr, UInt16_MaxValue);
	Convert_DecodeUtf8(value, ptr, len);
	SDL_free(ptr);
}

void Window_SetClipboardText(const String* value) {
	char str[600];
	Platform_ConvertString(str, value);
	SDL_SetClipboardText(str);
}

bool Window_GetVisible(void) {
	return (SDL_GetWindowFlags(win_handle) & SDL_WINDOW_SHOWN) != 0;
}

void Window_SetVisible(bool visible) {
	if (visible) {
		SDL_ShowWindow(win_handle);
	} else {
		SDL_HideWindow(win_handle);
	}
}

void* Window_GetWindowHandle(void) { return win_handle; }

int Window_GetWindowState(void) {
	Uint32 flags = SDL_GetWindowFlags(win_handle);
	if (flags & SDL_WINDOW_MINIMIZED) return WINDOW_STATE_MINIMISED;
	if (flags & SDL_WINDOW_MAXIMIZED) return WINDOW_STATE_MAXIMISED;

	if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) return WINDOW_STATE_FULLSCREEN;
	return WINDOW_STATE_NORMAL;
}

void Window_SetWindowState(int state) {
	switch (state) {
		case WINDOW_STATE_NORMAL:
			SDL_RestoreWindow(win_handle);
			break;
		case WINDOW_STATE_FULLSCREEN:
			SDL_SetWindowFullscreen(win_handle, SDL_WINDOW_FULLSCREEN_DESKTOP);
			break;
		case WINDOW_STATE_MAXIMISED:
			SDL_MaximizeWindow(win_handle);
			break;
		case WINDOW_STATE_MINIMISED:
			SDL_MinimizeWindow(win_handle);
			break;
	}
}

void Window_SetLocation(int x, int y) {
	SDL_SetWindowPosition(win_handle, x, y);
}

void Window_SetSize(int width, int height) {
	SDL_SetWindowSize(win_handle, width, height);
}

void Window_Close(void) {
	SDL_Event e;
    e.type = SDL_QUIT;
    SDL_PushEvent(&e);
}

static Key Window_MapKey(SDL_Keycode k) {
	if (k >= SDLK_0   && k <= SDLK_9)   { return '0'     + (k - SDLK_0); }
	if (k >= SDLK_a   && k <= SDLK_z)   { return 'A'     + (k - SDLK_a); }
	if (k >= SDLK_F1  && k <= SDLK_F12) { return KEY_F1  + (k - SDLK_F1); }
	if (k >= SDLK_F13 && k <= SDLK_F24) { return KEY_F13 + (k - SDLK_F13); }
	/* SDLK_KP_0 isn't before SDLK_KP_1 */
	if (k >= SDLK_KP_1 && k <= SDLK_KP_9) { return KEY_KP1 + (k - SDLK_KP_1); }

	switch (k) {
		case SDLK_RETURN: return KEY_ENTER;
		case SDLK_ESCAPE: return KEY_ESCAPE;
		case SDLK_BACKSPACE: return KEY_BACKSPACE;
		case SDLK_TAB:    return KEY_TAB;
		case SDLK_SPACE:  return KEY_SPACE;
		case SDLK_QUOTE:  return KEY_QUOTE;
		case SDLK_PLUS:   return KEY_PLUS;
		case SDLK_COMMA:  return KEY_COMMA;
		case SDLK_MINUS:  return KEY_MINUS;
		case SDLK_PERIOD: return KEY_PERIOD;
		case SDLK_SLASH:  return KEY_SLASH;
		case SDLK_SEMICOLON:    return KEY_SEMICOLON;
		case SDLK_LEFTBRACKET:  return KEY_LBRACKET;
		case SDLK_BACKSLASH:    return KEY_BACKSLASH;
		case SDLK_RIGHTBRACKET: return KEY_RBRACKET;
		case SDLK_BACKQUOTE:    return KEY_TILDE;
		case SDLK_CAPSLOCK:     return KEY_CAPSLOCK;
		case SDLK_PRINTSCREEN: return KEY_PRINTSCREEN;
		case SDLK_SCROLLLOCK:  return KEY_SCROLLLOCK;
		case SDLK_PAUSE:       return KEY_PAUSE;
		case SDLK_INSERT:   return KEY_INSERT;
		case SDLK_HOME:     return KEY_HOME;
		case SDLK_PAGEUP:   return KEY_PAGEUP;
		case SDLK_DELETE:   return KEY_DELETE;
		case SDLK_END:      return KEY_END;
		case SDLK_PAGEDOWN: return KEY_PAGEDOWN;
		case SDLK_RIGHT: return KEY_RIGHT;
		case SDLK_LEFT:  return KEY_LEFT;
		case SDLK_DOWN:  return KEY_DOWN;
		case SDLK_UP:    return KEY_UP;

		case SDLK_NUMLOCKCLEAR: return KEY_NUMLOCK;
		case SDLK_KP_DIVIDE: return KEY_KP_DIVIDE;
		case SDLK_KP_MULTIPLY: return KEY_KP_MULTIPLY;
		case SDLK_KP_MINUS: return KEY_KP_MINUS;
		case SDLK_KP_PLUS: return KEY_KP_PLUS;
		case SDLK_KP_ENTER: return KEY_KP_ENTER;
		case SDLK_KP_0: return KEY_KP0;
		case SDLK_KP_PERIOD: return KEY_KP_DECIMAL;

		case SDLK_LCTRL: return KEY_LCTRL;
		case SDLK_LSHIFT: return KEY_LSHIFT;
		case SDLK_LALT: return KEY_LALT;
		case SDLK_LGUI: return KEY_LWIN;
		case SDLK_RCTRL: return KEY_RCTRL;
		case SDLK_RSHIFT: return KEY_RSHIFT;
		case SDLK_RALT: return KEY_RALT;
		case SDLK_RGUI: return KEY_RWIN;
	}
	return KEY_NONE;
}

static void Window_HandleKeyEvent(const SDL_Event* e) {
	bool pressed = e->key.state == SDL_PRESSED;
	Key key = Window_MapKey(e->key.keysym.sym);
	if (key) Key_SetPressed(key, pressed);
}

static void Window_HandleMouseEvent(const SDL_Event* e) {
	bool pressed = e->button.state == SDL_PRESSED;
	switch (e->button.button) {
		case SDL_BUTTON_LEFT:
			Mouse_SetPressed(MOUSE_LEFT,   pressed); break;
		case SDL_BUTTON_MIDDLE:
			Mouse_SetPressed(MOUSE_MIDDLE, pressed); break;
		case SDL_BUTTON_RIGHT:
			Mouse_SetPressed(MOUSE_RIGHT,  pressed); break;
		case SDL_BUTTON_X1:
			Key_SetPressed(KEY_XBUTTON1, pressed); break;
		case SDL_BUTTON_X2:
			Key_SetPressed(KEY_XBUTTON2, pressed); break;
	}
}

static void Window_HandleTextEvent(const SDL_Event* e) {
	char buffer[SDL_TEXTINPUTEVENT_TEXT_SIZE];
	String str;
	int i, len;

	String_InitArray(str, buffer);
	len = String_CalcLen(e->text.text, SDL_TEXTINPUTEVENT_TEXT_SIZE);
	Convert_DecodeUtf8(&str, e->text.text, len);

	for (i = 0; i < str.length; i++) {
		Event_RaiseInt(&KeyEvents.Press, str.buffer[i]);
	}
}

static void Window_HandleWindowEvent(const SDL_Event* e) {
	switch (e->window.event) {
        case SDL_WINDOWEVENT_SHOWN:
        case SDL_WINDOWEVENT_HIDDEN:
            Event_RaiseVoid(&WindowEvents.VisibilityChanged);
            break;
        case SDL_WINDOWEVENT_EXPOSED:
            Event_RaiseVoid(&WindowEvents.Redraw);
            break;
        case SDL_WINDOWEVENT_MOVED:
            Window_RefreshBounds();
            Event_RaiseVoid(&WindowEvents.Moved);
            break;
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            Window_RefreshBounds();
            Event_RaiseVoid(&WindowEvents.Resized);
            break;
        case SDL_WINDOWEVENT_MINIMIZED:
		case SDL_WINDOWEVENT_MAXIMIZED:
		case SDL_WINDOWEVENT_RESTORED:
            Event_RaiseVoid(&WindowEvents.StateChanged);
            break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            Window_Focused = true;
            Event_RaiseVoid(&WindowEvents.FocusChanged);
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            Window_Focused = false;
            Event_RaiseVoid(&WindowEvents.FocusChanged);
            break;
        case SDL_WINDOWEVENT_CLOSE:
            Window_Close();
            break;
        }
}

void Window_ProcessEvents(void) {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {

		case SDL_KEYDOWN:
		case SDL_KEYUP:
			Window_HandleKeyEvent(&e); break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			Window_HandleMouseEvent(&e); break;
		case SDL_MOUSEWHEEL:
			Mouse_SetWheel(Mouse_Wheel + e.wheel.y);
			break;
		case SDL_MOUSEMOTION:
			Mouse_SetPosition(e.motion.x, e.motion.y);
			break;
		case SDL_TEXTINPUT:
			Window_HandleTextEvent(&e); break;
		case SDL_WINDOWEVENT:
			Window_HandleWindowEvent(&e); break;

		case SDL_QUIT:
			Window_Exists = false;
			Event_RaiseVoid(&WindowEvents.Closing);

			SDL_DestroyWindow(win_handle);
			Event_RaiseVoid(&WindowEvents.Closed);
			break;
		}
	}
}

Point2D Cursor_GetScreenPos(void) {
	Point2D p;
	SDL_GetGlobalMouseState(&p.X, &p.Y);
	//SDL_GetMouseState(&p.X, &p.Y);
	//p.X += Window_ClientBounds.X;
	//p.Y += Window_ClientBounds.Y;
	return p;
}

void Cursor_SetScreenPos(int x, int y) {
	SDL_WarpMouseGlobal(x, y);
	//x -= Window_ClientBounds.X;
	//y -= Window_ClientBounds.Y;
	//SDL_WarpMouseInWindow(win_handle, x, y);
}

void Cursor_SetVisible(bool visible) {
	SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

void Window_ShowDialog(const char* title, const char* msg) {
	SDL_ShowSimpleMessageBox(0, title, msg, win_handle);
}

static SDL_Surface* surface;
void Window_InitRaw(Bitmap* bmp) {
	surface = SDL_GetWindowSurface(win_handle);
	if (!surface) Window_SDLFail("getting window surface");

	if (SDL_MUSTLOCK(surface)) {
		int ret = SDL_LockSurface(surface);
		if (ret < 0) Window_SDLFail("locking window surface");
	}
	bmp->Scan0 = surface->pixels;
}

void Window_DrawRaw(Rect2D r) {
	SDL_Rect rect;
	rect.x = r.X; rect.w = r.Width;
	rect.y = r.Y; rect.h = r.Height;
	SDL_UpdateWindowSurfaceRects(win_handle, &rect, 1);
}

#ifndef CC_BUILD_D3D9
static SDL_GLContext win_ctx;
void GLContext_Init(struct GraphicsMode* mode) {
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   mode->R);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, mode->G);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  mode->B);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, mode->A);

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   mode->DepthBits);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, mode->StencilBits);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, true);

	win_ctx = SDL_GL_CreateContext(win_handle);
	if (!win_ctx) Window_SDLFail("creating OpenGL context");
}

void GLContext_Update(void) { }
void GLContext_Free(void) {
	SDL_GL_DeleteContext(win_ctx);
	win_ctx = NULL;
}

void* GLContext_GetAddress(const char* function) {
	return SDL_GL_GetProcAddress(function);
}

void GLContext_SwapBuffers(void) {
	SDL_GL_SwapWindow(win_handle);
}

void GLContext_SetVSync(bool enabled) {
	SDL_GL_SetSwapInterval(enabled);
}
#endif
#endif
