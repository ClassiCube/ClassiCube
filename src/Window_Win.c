#include "Core.h"
#if CC_WIN_BACKEND == CC_WIN_BACKEND_WIN32
/*
   The Open Toolkit Library License
  
   Copyright (c) 2006 - 2009 the Open Toolkit library.
  
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights to
   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
   the Software, and to permit persons to whom the Software is furnished to do
   so, subject to the following conditions:
  
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
  
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.
*/

#include "_WindowBase.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Options.h"
#include "Errors.h"

#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 /* Windows XP */
/* NOTE: Functions not present on older OS versions are dynamically loaded. */
/* Setting WIN32_WINNT to XP just avoids redeclaring structs. */
#endif
#include <windows.h>
/* #include <commdlg.h> */
/* Compatibility versions so compiling works on older Windows SDKs */
#include "../misc/windows/min-commdlg.h"
#include "../misc/windows/min-winuser.h"

/* https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setpixelformat */
#define CC_WIN_STYLE WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN
#define CC_WIN_CLASSNAME TEXT("ClassiCube_Window")
#define Rect_Width(rect)  (rect.right  - rect.left)
#define Rect_Height(rect) (rect.bottom - rect.top)

static HINSTANCE win_instance;
static HDC win_DC;
static cc_bool suppress_resize;
static cc_bool is_ansiWindow, grabCursor;
static int windowX, windowY;

static const cc_uint8 key_map[] = {
/* 00 */ 0, 0, 0, 0, 0, 0, 0, 0, 
/* 08 */ CCKEY_BACKSPACE, CCKEY_TAB, 0, 0, CCKEY_F5, CCKEY_ENTER, 0, 0,
/* 10 */ 0, 0, 0, CCKEY_PAUSE, CCKEY_CAPSLOCK, 0, 0, 0, 
/* 18 */ 0, 0, 0, CCKEY_ESCAPE, 0, 0, 0, 0,
/* 20 */ CCKEY_SPACE, CCKEY_PAGEUP, CCKEY_PAGEDOWN, CCKEY_END, CCKEY_HOME, CCKEY_LEFT, CCKEY_UP, CCKEY_RIGHT, 
/* 28 */ CCKEY_DOWN, 0, CCKEY_PRINTSCREEN, 0, CCKEY_PRINTSCREEN, CCKEY_INSERT, CCKEY_DELETE, 0,
/* 30 */ '0', '1', '2', '3', '4', '5', '6', '7', 
/* 38 */ '8', '9', 0, 0, 0, 0, 0, 0,
/* 40 */ 0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 
/* 48 */ 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
/* 50 */ 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 
/* 58 */ 'X', 'Y', 'Z', CCKEY_LWIN, CCKEY_RWIN, CCKEY_MENU, 0, CCKEY_SLEEP,
/* 60 */ CCKEY_KP0, CCKEY_KP1, CCKEY_KP2, CCKEY_KP3, CCKEY_KP4, CCKEY_KP5, CCKEY_KP6, CCKEY_KP7, 
/* 68 */ CCKEY_KP8, CCKEY_KP9, CCKEY_KP_MULTIPLY, CCKEY_KP_PLUS, 0, CCKEY_KP_MINUS, CCKEY_KP_DECIMAL, CCKEY_KP_DIVIDE,
/* 70 */ CCKEY_F1, CCKEY_F2, CCKEY_F3, CCKEY_F4, CCKEY_F5, CCKEY_F6, CCKEY_F7, CCKEY_F8, 
/* 78 */ CCKEY_F9, CCKEY_F10, CCKEY_F11, CCKEY_F12, CCKEY_F13, CCKEY_F14, CCKEY_F15, CCKEY_F16,
/* 80 */ CCKEY_F17, CCKEY_F18, CCKEY_F19, CCKEY_F20, CCKEY_F21, CCKEY_F22, CCKEY_F23, CCKEY_F24, 
/* 88 */ 0, 0, 0, 0, 0, 0, 0, 0,
/* 90 */ CCKEY_NUMLOCK, CCKEY_SCROLLLOCK, 0, 0, 0, 0, 0, 0, 
/* 98 */ 0, 0, 0, 0, 0, 0, 0, 0,
/* A0 */ CCKEY_LSHIFT, CCKEY_RSHIFT, CCKEY_LCTRL, CCKEY_RCTRL, CCKEY_LALT, CCKEY_RALT, CCKEY_BROWSER_PREV, CCKEY_BROWSER_NEXT, 
/* A8 */ CCKEY_BROWSER_REFRESH, CCKEY_BROWSER_STOP, CCKEY_BROWSER_SEARCH, CCKEY_BROWSER_FAVORITES, CCKEY_BROWSER_HOME, CCKEY_VOLUME_MUTE, CCKEY_VOLUME_DOWN, CCKEY_VOLUME_UP,
/* B0 */ CCKEY_MEDIA_NEXT, CCKEY_MEDIA_PREV, CCKEY_MEDIA_STOP, CCKEY_MEDIA_PLAY, CCKEY_LAUNCH_MAIL, CCKEY_LAUNCH_MEDIA, CCKEY_LAUNCH_APP1, CCKEY_LAUNCH_CALC, 
/* B8 */ 0, 0, CCKEY_SEMICOLON, CCKEY_EQUALS, CCKEY_COMMA, CCKEY_MINUS, CCKEY_PERIOD, CCKEY_SLASH,
/* C0 */ CCKEY_TILDE, 0, 0, 0, 0, 0, 0, 0, 
/* C8 */ 0, 0, 0, 0, 0, 0, 0, 0,
/* D0 */ 0, 0, 0, 0, 0, 0, 0, 0, 
/* D8 */ 0, 0, 0, CCKEY_LBRACKET, CCKEY_BACKSLASH, CCKEY_RBRACKET, CCKEY_QUOTE, 0,
};

static int MapNativeKey(WPARAM vk_key, LPARAM meta) {
	LPARAM ext      = meta & (1UL << 24);
	LPARAM scancode = (meta >> 16) & 0xFF;
	int key;
	if (ext) scancode |= 0xE000;
	
	switch (vk_key)
	{
	case VK_CONTROL:
		return ext ? CCKEY_RCTRL : CCKEY_LCTRL;
	case VK_MENU:
		return ext ? CCKEY_RALT  : CCKEY_LALT;
	case VK_RETURN:
		return ext ? CCKEY_KP_ENTER : CCKEY_ENTER;
	}
	
	key = vk_key < Array_Elems(key_map) ? key_map[vk_key] : 0;
	if (!key) Platform_Log2("Unknown key: %x, %x", &vk_key, &scancode);
	return key;
}

static cc_bool RefreshWindowDimensions(void) {
	HWND hwnd = Window_Main.Handle.ptr;
	RECT rect;
	int width = Window_Main.Width, height = Window_Main.Height;

	GetClientRect(hwnd, &rect);
	Window_Main.Width  = Rect_Width(rect);
	Window_Main.Height = Rect_Height(rect);

	return width != Window_Main.Width || height != Window_Main.Height;
}

static void RefreshWindowPosition(void) {
	HWND hwnd = Window_Main.Handle.ptr;
	POINT topLeft = { 0, 0 };
	/* GetClientRect always returns 0,0 for left,top (see MSDN) */
	ClientToScreen(hwnd, &topLeft);
	windowX = topLeft.x; windowY = topLeft.y;
}

static void GrabCursor(void) {
	HWND hwnd = Window_Main.Handle.ptr;
	RECT rect;
	if (!grabCursor || !Input.RawMode) return;

	GetWindowRect(hwnd, &rect);
	ClipCursor(&rect);
}

static LRESULT CALLBACK Window_Procedure(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) {
	float wheelDelta;
	cc_bool sized;
	HWND hwnd;

	switch (message) {
	case WM_ACTIVATE:
		Window_Main.Focused = LOWORD(wParam) != 0;
		Event_RaiseVoid(&WindowEvents.FocusChanged);
		break;

	case WM_ERASEBKGND:
		return 1; /* Avoid flickering */

	case WM_PAINT:
		hwnd = Window_Main.Handle.ptr;
		ValidateRect(hwnd, NULL);
		Event_RaiseVoid(&WindowEvents.RedrawNeeded);
		return 0;

	case WM_SIZE:
		GrabCursor();
		sized = RefreshWindowDimensions();

		if (sized && !suppress_resize) Event_RaiseVoid(&WindowEvents.Resized);
		Event_RaiseVoid(&WindowEvents.StateChanged);
		break;
	
	case WM_MOVE:
		GrabCursor();
		RefreshWindowPosition();
		break;

	case WM_CHAR:
		/* TODO: Use WM_UNICHAR instead, as WM_CHAR is just utf16 */
		Event_RaiseInt(&InputEvents.Press, (cc_unichar)wParam);
		break;

	case WM_MOUSEMOVE:
		/* Set before position change, in case mouse buttons changed when outside window */
		if (!(wParam & 0x01)) Input_SetReleased(CCMOUSE_L);
		//Input_SetNonRepeatable(CCMOUSE_L, wParam & 0x01);
		Input_SetNonRepeatable(CCMOUSE_R, wParam & 0x02);
		Input_SetNonRepeatable(CCMOUSE_M, wParam & 0x10);
		/* TODO: do we need to set XBUTTON1/XBUTTON2 here */
		Pointer_SetPosition(0, LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_MOUSEWHEEL:
		wheelDelta = ((short)HIWORD(wParam)) / (float)WHEEL_DELTA;
		Mouse_ScrollVWheel(wheelDelta);
		return 0;
	case WM_MOUSEHWHEEL:
		wheelDelta = ((short)HIWORD(wParam)) / (float)WHEEL_DELTA;
		Mouse_ScrollHWheel(wheelDelta);
		return 0;

	case WM_LBUTTONDOWN:
		Input_SetPressed(CCMOUSE_L); break;
	case WM_MBUTTONDOWN:
		Input_SetPressed(CCMOUSE_M); break;
	case WM_RBUTTONDOWN:
		Input_SetPressed(CCMOUSE_R); break;
	case WM_XBUTTONDOWN:
		Input_SetPressed(HIWORD(wParam) == 1 ? CCMOUSE_X1 : CCMOUSE_X2);
		break;

	case WM_LBUTTONUP:
		Input_SetReleased(CCMOUSE_L); break;
	case WM_MBUTTONUP:
		Input_SetReleased(CCMOUSE_M); break;
	case WM_RBUTTONUP:
		Input_SetReleased(CCMOUSE_R); break;
	case WM_XBUTTONUP:
		Input_SetReleased(HIWORD(wParam) == 1 ? CCMOUSE_X1 : CCMOUSE_X2);
		break;

	case WM_INPUT:
	{
		int dx, dy, width, height, absX, absY, isVirtual;
		UINT ret, rawSize = sizeof(RAWINPUT);
		RAWINPUT raw;

		ret = _GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &rawSize, sizeof(RAWINPUTHEADER));
		if (ret == -1 || raw.header.dwType != RIM_TYPEMOUSE) break;		

		if (raw.data.mouse.usFlags == MOUSE_MOVE_RELATIVE) {
			/* Majority of mouse input devices provide relative coordinates */
			dx = raw.data.mouse.lLastX;
			dy = raw.data.mouse.lLastY;
		} else if (raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
			/* This mouse mode is produced by VirtualBox with Mouse Integration on */
			/* To understand reasoning behind the following code, see Remarks in */
			/*  https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-rawmouse */
			static int prevPosX, prevPosY;
			isVirtual = raw.data.mouse.usFlags & MOUSE_VIRTUAL_DESKTOP;

			width  = GetSystemMetrics(isVirtual ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
			height = GetSystemMetrics(isVirtual ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
			absX   = (int)((raw.data.mouse.lLastX / 65535.0f) * width);
			absY   = (int)((raw.data.mouse.lLastY / 65535.0f) * height);

			dx = absX - prevPosX; prevPosX = absX;
			dy = absY - prevPosY; prevPosY = absY;
		} else { break; }

		if (Input.RawMode) Event_RaiseRawMove(&PointerEvents.RawMoved, (float)dx, (float)dy);
	} break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	{
		cc_bool pressed = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
		/* Shift/Control/Alt behave strangely when e.g. ShiftRight is held down and ShiftLeft is pressed
		and released. It looks like neither key is released in this case, or that the wrong key is
		released in the case of Control and Alt.
		To combat this, we are going to release both keys when either is released. Hacky, but should work.
		Win95 does not distinguish left/right key constants (GetAsyncKeyState returns 0).
		In this case, both keys will be reported as pressed. */
		cc_bool lShiftDown, rShiftDown;
		int key;

		if (wParam == VK_SHIFT) {
			/* The behavior of this key is very strange. Unlike Control and Alt, there is no extended bit
			to distinguish between left and right keys. Moreover, pressing both keys and releasing one
			may result in both keys being held down (but not always).*/
			lShiftDown = ((USHORT)GetKeyState(VK_LSHIFT)) >> 15;
			rShiftDown = ((USHORT)GetKeyState(VK_RSHIFT)) >> 15;

			if (!pressed || lShiftDown != rShiftDown) {
				Input_Set(CCKEY_LSHIFT, lShiftDown);
				Input_Set(CCKEY_RSHIFT, rShiftDown);
			}
		} else {
			key = MapNativeKey(wParam, lParam);
			if (key) Input_Set(key, pressed);
		}
		return 0;
	} break;

	case WM_SYSCHAR:
		return 0;

	case WM_KILLFOCUS:
		/* TODO: Keep track of keyboard when focus is lost */
		Input_Clear();
		break;

	case WM_CLOSE:
		Window_Main.Exists = false;
		Window_RequestClose();
		return 0;

	case WM_DESTROY:
		UnregisterClassW(CC_WIN_CLASSNAME, win_instance);
		break;
	}
	return is_ansiWindow ? DefWindowProcA(handle, message, wParam, lParam)
						 : DefWindowProcW(handle, message, wParam, lParam);
}


/*########################################################################################################################*
*--------------------------------------------------Public implementation--------------------------------------------------*
*#########################################################################################################################*/
void Window_PreInit(void) { 
	DisplayInfo.CursorVisible = true;
}

void Window_Init(void) {
	HDC hdc;
	User32_LoadDynamicFuncs();
	Input.Sources = INPUT_SOURCE_NORMAL;

	/* Enable high DPI support */
	DisplayInfo.DPIScaling = Options_GetBool(OPT_DPI_SCALING, false);
	if (DisplayInfo.DPIScaling && _SetProcessDPIAware) _SetProcessDPIAware();

	hdc = GetDC(NULL);
	DisplayInfo.Width  = GetSystemMetrics(SM_CXSCREEN);
	DisplayInfo.Height = GetSystemMetrics(SM_CYSCREEN);
	DisplayInfo.Depth  = GetDeviceCaps(hdc, BITSPIXEL);
	DisplayInfo.ScaleX = GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
	DisplayInfo.ScaleY = GetDeviceCaps(hdc, LOGPIXELSY) / 96.0f;
	ReleaseDC(NULL, hdc);

	/* https://docs.microsoft.com/en-us/windows/win32/opengl/reading-color-values-from-the-framebuffer */
	/* https://devblogs.microsoft.com/oldnewthing/20101013-00/?p=12543  */
	/* TODO probably should always multiply? not just for 16 colors */
	if (DisplayInfo.Depth != 1) return;
	DisplayInfo.Depth *= GetDeviceCaps(hdc, PLANES);
}

void Window_Free(void) { }

static ATOM DoRegisterClass(void) {
	ATOM atom;
	cc_result res;
	WNDCLASSEXW wc   = { 0 };
	wc.cbSize        = sizeof(WNDCLASSEXW);
	wc.style         = CS_OWNDC; /* https://stackoverflow.com/questions/48663815/getdc-releasedc-cs-owndc-with-opengl-and-gdi */
	wc.hInstance     = win_instance;
	wc.lpfnWndProc   = Window_Procedure;
	wc.lpszClassName = CC_WIN_CLASSNAME;

	wc.hIcon   = (HICON)LoadImageA(win_instance, MAKEINTRESOURCEA(1), IMAGE_ICON,
			GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
	wc.hIconSm = (HICON)LoadImageA(win_instance, MAKEINTRESOURCEA(1), IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
	wc.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);

	if ((atom = RegisterClassExW(&wc))) return atom;
	/* Windows 9x does not support W API functions */
	if ((atom = RegisterClassExA((const WNDCLASSEXA*)&wc))) return atom;
	
	/* Windows NT 3.5 does not support RegisterClassExA function */
	res = GetLastError();
	if (res == ERROR_CALL_NOT_IMPLEMENTED) {
		if ((atom = RegisterClassA((const WNDCLASSA*)&wc.style))) return atom;
		res = GetLastError();
	}
	
	Process_Abort2(res, "Failed to register window class");
	return (ATOM)0;
}

static HWND CreateWindowHandle(ATOM atom, int width, int height) {
	cc_result res;
	HWND hwnd;
	RECT r;
	
	/* Calculate final window rectangle after window decorations are added (titlebar, borders etc) */
	r.left = Display_CentreX(width);  r.right  = r.left + width;
	r.top  = Display_CentreY(height); r.bottom = r.top  + height;
	AdjustWindowRect(&r, CC_WIN_STYLE, false);

	if ((hwnd = CreateWindowExW(0, MAKEINTATOM(atom), NULL, CC_WIN_STYLE,
		r.left, r.top, Rect_Width(r), Rect_Height(r), NULL, NULL, win_instance, NULL))) return hwnd;
	res = GetLastError();

	/* Windows 9x does not support W API functions */
	if (res == ERROR_CALL_NOT_IMPLEMENTED) {
		is_ansiWindow = true;
		if ((hwnd = CreateWindowExA(0, (LPCSTR)MAKEINTATOM(atom), NULL, CC_WIN_STYLE,
			r.left, r.top, Rect_Width(r), Rect_Height(r), NULL, NULL, win_instance, NULL))) return hwnd;
		res = GetLastError();
	}
	Process_Abort2(res, "Failed to create window");
	return NULL;
}

static void DoCreateWindow(int width, int height) {
	ATOM atom;
	HWND hwnd;

	win_instance = GetModuleHandleA(NULL);
	/* TODO: UngroupFromTaskbar(); */
	width  = Display_ScaleX(width);
	height = Display_ScaleY(height);

	atom = DoRegisterClass();
	hwnd = CreateWindowHandle(atom, width, height);
	RefreshWindowDimensions();
	RefreshWindowPosition();

	win_DC = GetDC(hwnd);
	if (!win_DC) Process_Abort2(GetLastError(), "Failed to get device context");

	Window_Main.Exists     = true;
	Window_Main.Handle.ptr = hwnd;
	Window_Main.UIScaleX   = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY   = DEFAULT_UI_SCALE_Y;
	
	grabCursor = Options_GetBool(OPT_GRAB_CURSOR, false);
}
void Window_Create2D(int width, int height) { DoCreateWindow(width, height); }
void Window_Create3D(int width, int height) { DoCreateWindow(width, height); }

void Window_Destroy(void) {
	HWND hwnd = Window_Main.Handle.ptr;
	if (win_DC) ReleaseDC(hwnd, win_DC);
	DestroyWindow(hwnd);
}

void Window_SetTitle(const cc_string* title) {
	HWND hwnd = Window_Main.Handle.ptr;
	cc_winstring str;
	Platform_EncodeString(&str, title);
	if (SetWindowTextW(hwnd, str.uni)) return;

	/* Windows 9x does not support W API functions */
	SetWindowTextA(hwnd, str.ansi);
}

void Clipboard_GetText(cc_string* value) {
	HWND hwnd = Window_Main.Handle.ptr;
	cc_bool unicode;
	HANDLE hGlobal;
	_SIZE_T size;
	void* src;
	int i;

	/* retry up to 50 times */
	for (i = 0; i < 50; i++) {
		if (!OpenClipboard(hwnd)) {
			Thread_Sleep(10);
			continue;
		}

		unicode = true;
		hGlobal = GetClipboardData(CF_UNICODETEXT);
		if (!hGlobal) {
			hGlobal = GetClipboardData(CF_TEXT);
			unicode = false;
		}

		if (!hGlobal) { CloseClipboard(); return; }
		src  = GlobalLock(hGlobal);
		size = GlobalSize(hGlobal);

		/* ignore trailing NULL at end */
		/* TODO: Verify it's always there */
		if (unicode) {
			String_AppendUtf16(value,  src, size - 2);
		} else {
			String_AppendCP1252(value, src, size - 1);
		}

		GlobalUnlock(hGlobal);
		CloseClipboard();
		return;
	}
}

void Clipboard_SetText(const cc_string* value) {
	HWND hwnd = Window_Main.Handle.ptr;
	cc_unichar* text;
	HANDLE hGlobal;
	int i;

	/* retry up to 10 times */
	for (i = 0; i < 10; i++) {
		if (!OpenClipboard(hwnd)) {
			Thread_Sleep(100);
			continue;
		}

		hGlobal = GlobalAlloc(GMEM_MOVEABLE, (value->length + 1) * 2);
		if (!hGlobal) { CloseClipboard(); return; }

		text = (cc_unichar*)GlobalLock(hGlobal);
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

int Window_GetWindowState(void) {
	HWND hwnd = Window_Main.Handle.ptr;
	DWORD s   = GetWindowLongA(hwnd, GWL_STYLE);

	if ((s & WS_MINIMIZE))                   return WINDOW_STATE_MINIMISED;
	if ((s & WS_MAXIMIZE) && (s & WS_POPUP)) return WINDOW_STATE_FULLSCREEN;
	return WINDOW_STATE_NORMAL;
}

static void ToggleFullscreen(HWND hwnd, cc_bool fullscreen, UINT finalShow) {
	DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	style |= (fullscreen ? WS_POPUP : WS_OVERLAPPEDWINDOW);

	suppress_resize = true;
	{
		ShowWindow(hwnd, SW_RESTORE); /* reset maximised state */
		SetWindowLongA(hwnd, GWL_STYLE, style);
		SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		ShowWindow(hwnd, finalShow); 
		Window_ProcessEvents(0.0);
	}
	suppress_resize = false;

	/* call Resized event only once */
	RefreshWindowDimensions();
	Event_RaiseVoid(&WindowEvents.Resized);
}

static UINT win_show;
cc_result Window_EnterFullscreen(void) {
	HWND hwnd = Window_Main.Handle.ptr;
	WINDOWPLACEMENT w = { 0 };
	w.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(hwnd, &w);

	win_show = w.showCmd;
	ToggleFullscreen(hwnd, true, SW_MAXIMIZE);
	return 0;
}

cc_result Window_ExitFullscreen(void) {
	HWND hwnd = Window_Main.Handle.ptr;
	ToggleFullscreen(hwnd, false, win_show);
	return 0;
}

int Window_IsObscured(void) { return 0; }

void Window_Show(void) {
	HWND hwnd = Window_Main.Handle.ptr;
	ShowWindow(hwnd, SW_SHOW);
	BringWindowToTop(hwnd);
	SetForegroundWindow(hwnd);
}

void Window_SetSize(int width, int height) {
	HWND hwnd   = Window_Main.Handle.ptr;
	DWORD style = GetWindowLongA(hwnd, GWL_STYLE);
	RECT rect   = { 0, 0, width, height };
	AdjustWindowRect(&rect, style, false);

	SetWindowPos(hwnd, NULL, 0, 0, 
				Rect_Width(rect), Rect_Height(rect), SWP_NOMOVE);
}

void Window_RequestClose(void) {
	Event_RaiseVoid(&WindowEvents.Closing);
}

void Window_ProcessEvents(float delta) {
	HWND foreground;
	HWND hwnd;
	MSG msg;

	if (is_ansiWindow) {
		while (PeekMessageA(&msg, NULL, 0, 0, 1)) {
			TranslateMessage(&msg); DispatchMessageA(&msg);
		}
	} else {
		while (PeekMessageW(&msg, NULL, 0, 0, 1)) {
			TranslateMessage(&msg); DispatchMessageW(&msg);
		}
	}

	foreground = GetForegroundWindow();
	if (foreground) {
		hwnd = Window_Main.Handle.ptr;
		Window_Main.Focused = foreground == hwnd;
	}
}

void Gamepads_Init(void) {

}

void Gamepads_Process(float delta) { }

static void Cursor_GetRawPos(int* x, int* y) {
	POINT point; 
	GetCursorPos(&point);
	*x = point.x; *y = point.y;
}

void Cursor_SetPosition(int x, int y) { 
	SetCursorPos(x + windowX, y + windowY);
}
static void Cursor_DoSetVisible(cc_bool visible) {
	int i;
	/* ShowCursor actually is a counter (returns > 0 if visible, <= 0 if not) */
	/* Try multiple times in case cursor count was changed by something else */
	/*  (there's at least system out there that requires 30 times) */
	if (visible) {
		for (i = 0; i < 50 && ShowCursor(true)  <  0; i++) { }
	} else {
		for (i = 0; i < 50 && ShowCursor(false) >= 0; i++) {}
	}
}

static void ShowDialogCore(const char* title, const char* msg) {
	HWND hwnd = Window_Main.Handle.ptr;
	MessageBoxA(hwnd, msg, title, 0);
}

static cc_result OpenSaveFileDialog(const cc_string* filters, FileDialogCallback callback, cc_bool load,
									const char* const* fileExts, const cc_string* defaultName) {
	union OPENFILENAME_union {
		OPENFILENAMEW wide;
		OPENFILENAMEA ansi;
	} ofn = { 0 }; // less compiler warnings this way
	HWND hwnd = Window_Main.Handle.ptr;
	
	cc_string path; char pathBuffer[NATIVE_STR_LEN];
	cc_winstring str  = { 0 };
	cc_winstring filter;
	cc_result res;
	BOOL ok;
	int i;

	Platform_EncodeString(&str, defaultName);
	Platform_EncodeString(&filter, filters);
	/* NOTE: OPENFILENAME_SIZE_VERSION_400 is used instead of sizeof(OFN), because the size of */
	/*  OPENFILENAME increased after Windows 9x/NT4 with the addition of pvReserved and later fields */
	/* (and Windows 9x/NT4 return an error if a lStructSize > OPENFILENAME_SIZE_VERSION_400 is used) */
	ofn.wide.lStructSize  = OPENFILENAME_SIZE_VERSION_400;
	/* also note that this only works when OFN_HOOK is *not* included in Flags - if it is, then */
	/*  on modern Windows versions the dialogs are altered to show an old Win 9x style appearance */
	/* (see https://github.com/geany/geany/issues/578 for example of this problem) */

	ofn.wide.hwndOwner    = hwnd;
	ofn.wide.lpstrFile    = str.uni;
	ofn.wide.nMaxFile     = MAX_PATH;
	ofn.wide.lpstrFilter  = filter.uni;
	ofn.wide.nFilterIndex = 1;
	ofn.wide.Flags = OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | (load ? OFN_FILEMUSTEXIST : OFN_OVERWRITEPROMPT);

	String_InitArray(path, pathBuffer);
	ok = load ? GetOpenFileNameW(&ofn.wide) : GetSaveFileNameW(&ofn.wide);
	
	if (ok) {
		/* Successfully got a unicode filesystem path */
		for (i = 0; i < MAX_PATH && str.uni[i]; i++) 
		{
			String_Append(&path, Convert_CodepointToCP437(str.uni[i]));
		}
	} else if ((res = CommDlgExtendedError()) == 2) {
		/* CDERR_INITIALIZATION - probably running on Windows 9x */
		ofn.ansi.lpstrFile   = str.ansi;
		ofn.ansi.lpstrFilter = filter.ansi;

		ok = load ? GetOpenFileNameA(&ofn.ansi) : GetSaveFileNameA(&ofn.ansi);
		if (!ok) return CommDlgExtendedError();

		for (i = 0; i < MAX_PATH && str.ansi[i]; i++) 
		{
			String_Append(&path, str.ansi[i]);
		}
	} else {
		return res;
	}

	/* Add default file extension if user didn't provide one */
	if (!load && ofn.wide.nFileExtension == 0 && ofn.wide.nFilterIndex > 0) {
		String_AppendConst(&path, fileExts[ofn.wide.nFilterIndex - 1]);
	}
	callback(&path);
	return 0;
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	const char* const* fileExts = args->filters;
	cc_string filters; char buffer[NATIVE_STR_LEN];
	int i;

	/* Filter tokens are \0 separated - e.g. "Maps (*.cw;*.dat)\0*.cw;*.dat\0 */
	String_InitArray(filters, buffer);
	String_Format1(&filters, "%c (", args->description);
	for (i = 0; fileExts[i]; i++)
	{
		if (i) String_Append(&filters, ';');
		String_Format1(&filters, "*%c", fileExts[i]);
	}
	String_Append(&filters, ')');
	String_Append(&filters, '\0');

	for (i = 0; fileExts[i]; i++)
	{
		if (i) String_Append(&filters, ';');
		String_Format1(&filters, "*%c", fileExts[i]);
	}
	String_Append(&filters, '\0');

	return OpenSaveFileDialog(&filters, args->Callback, true, fileExts, &String_Empty);
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	const char* const* titles   = args->titles;
	const char* const* fileExts = args->filters;
	cc_string filters; char buffer[NATIVE_STR_LEN];
	int i;

	/* Filter tokens are \0 separated - e.g. "Map (*.cw)\0*.cw\0 */
	String_InitArray(filters, buffer);
	for (i = 0; fileExts[i]; i++)
	{
		String_Format2(&filters, "%c (*%c)", titles[i], fileExts[i]);
		String_Append(&filters,  '\0');
		String_Format1(&filters, "*%c", fileExts[i]);
		String_Append(&filters,  '\0');
	}
	return OpenSaveFileDialog(&filters, args->Callback, false, fileExts, &args->defaultName);
}

static HDC draw_DC;
static HBITMAP draw_DIB;
void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	BITMAPINFO hdr = { 0 };
	if (!draw_DC) draw_DC = CreateCompatibleDC(win_DC);
	
	/* sizeof(BITMAPINFO) does not work on Windows 9x */
	hdr.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	hdr.bmiHeader.biWidth    =  width;
	hdr.bmiHeader.biHeight   = -height;
	hdr.bmiHeader.biBitCount = 32;
	hdr.bmiHeader.biPlanes   = 1; 

	draw_DIB = CreateDIBSection(draw_DC, &hdr, DIB_RGB_COLORS, (void**)&bmp->scan0, NULL, 0);
	if (!draw_DIB) Process_Abort2(GetLastError(), "Failed to create DIB");
	bmp->width  = width;
	bmp->height = height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	HGDIOBJ oldSrc = SelectObject(draw_DC, draw_DIB);
	BitBlt(win_DC, r.x, r.y, r.width, r.height, draw_DC, r.x, r.y, SRCCOPY);
	SelectObject(draw_DC, oldSrc);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	DeleteObject(draw_DIB);
}

static cc_bool rawMouseInited, rawMouseSupported;
static void InitRawMouse(void) {
	HWND hwnd = Window_Main.Handle.ptr;
	RAWINPUTDEVICE rid;

	rawMouseSupported = _RegisterRawInputDevices && _GetRawInputData;
	rawMouseSupported &= Options_GetBool(OPT_RAW_INPUT, true);
	if (!rawMouseSupported) { Platform_LogConst("## Raw input unsupported!"); return; }

	rid.usUsagePage = 1; /* HID_USAGE_PAGE_GENERIC; */
	rid.usUsage     = 2; /* HID_USAGE_GENERIC_MOUSE; */
	rid.dwFlags     = RIDEV_INPUTSINK;
	rid.hwndTarget  = hwnd;

	if (_RegisterRawInputDevices(&rid, 1, sizeof(rid))) return;
	Logger_SysWarn(GetLastError(), "initing raw mouse");
	rawMouseSupported = false;
}

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { }
void OnscreenKeyboard_SetText(const cc_string* text) { }
void OnscreenKeyboard_Close(void) { }

void Window_EnableRawMouse(void) {
	DefaultEnableRawMouse();
	if (!rawMouseInited) InitRawMouse();

	rawMouseInited = true;
	GrabCursor();
}

void Window_UpdateRawMouse(void) {
	if (rawMouseSupported) {
		/* handled in WM_INPUT messages */
		CentreMousePosition();
	} else {
		DefaultUpdateRawMouse();
	}
}

void Window_DisableRawMouse(void) { 
	DefaultDisableRawMouse();
	if (grabCursor) ClipCursor(NULL);
}


/*########################################################################################################################*
*-------------------------------------------------------WGL OpenGL--------------------------------------------------------*
*#########################################################################################################################*/
#if CC_GFX_BACKEND_IS_GL() && !defined CC_BUILD_EGL
static HGLRC ctx_handle;
static HDC   ctx_DC;
static void* gl_lib;

static HGLRC (WINAPI *_wglCreateContext)(HDC dc);
static BOOL  (WINAPI *_wglDeleteContext)(HGLRC glrc);
static HDC   (WINAPI *_wglGetCurrentDC)(void);
static BOOL  (WINAPI *_wglMakeCurrent)(HDC dc, HGLRC glrc);
static PROC  (WINAPI *_wglGetProcAddress)(LPCSTR func);

typedef BOOL (WINAPI *FP_SWAPINTERVAL)(int interval);
static FP_SWAPINTERVAL _wglSwapIntervalEXT;

static void GLContext_SelectGraphicsMode(struct GraphicsMode* mode) {
	PIXELFORMATDESCRIPTOR pfd = { 0 };
	int modeIndex;

	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags  = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	/* TODO: PFD_SUPPORT_COMPOSITION FLAG? CHECK IF IT WORKS ON XP */

	pfd.cColorBits = mode->R + mode->G + mode->B;
	pfd.cDepthBits = GLCONTEXT_DEFAULT_DEPTH;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cAlphaBits = mode->A; /* TODO not needed? test on Intel */

	modeIndex = ChoosePixelFormat(win_DC, &pfd);
	if (modeIndex == 0) { Process_Abort("Requested graphics mode not available"); }

	Mem_Set(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;

	/* TODO DescribePixelFormat might be unnecessary? */
	DescribePixelFormat(win_DC, modeIndex, pfd.nSize, &pfd);
	if (!SetPixelFormat(win_DC, modeIndex, &pfd)) {
		Process_Abort2(GetLastError(), "SetPixelFormat failed");
	}
}

void GLContext_Create(void) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_ReqSym(wglCreateContext),
		DynamicLib_ReqSym(wglDeleteContext),
		DynamicLib_ReqSym(wglGetCurrentDC),
		DynamicLib_ReqSym(wglMakeCurrent),
		DynamicLib_OptSym(wglGetProcAddress)
	};
	static const cc_string glPath = String_FromConst("OPENGL32.dll");
	struct GraphicsMode mode;

	InitGraphicsMode(&mode);
	GLContext_SelectGraphicsMode(&mode);
	DynamicLib_LoadAll(&glPath, funcs, Array_Elems(funcs), &gl_lib);

	ctx_handle = _wglCreateContext(win_DC);
	if (!ctx_handle) {
		Process_Abort2(GetLastError(), "Failed to create OpenGL context");
	}

	if (!_wglMakeCurrent(win_DC, ctx_handle)) {
		Process_Abort2(GetLastError(), "Failed to make OpenGL context current");
	}

	ctx_DC = _wglGetCurrentDC();
	_wglSwapIntervalEXT = (FP_SWAPINTERVAL)GLContext_GetAddress("wglSwapIntervalEXT");
}

void GLContext_Update(void) { }
cc_bool GLContext_TryRestore(void) { return true; }

void GLContext_Free(void) {
	if (!ctx_handle) return;
	_wglDeleteContext(ctx_handle);
	ctx_handle = NULL;
}

/* https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions#Windows */
#define GLContext_IsInvalidAddress(ptr) (ptr == (void*)0 || ptr == (void*)1 || ptr == (void*)-1 || ptr == (void*)2)

void* GLContext_GetAddress(const char* function) {
	/* Not present on NT 3.5 */
	if (_wglGetProcAddress) {
		void* addr = (void*)_wglGetProcAddress(function);
		if (!GLContext_IsInvalidAddress(addr)) return addr;
	}

	/* Some drivers return NULL from wglGetProcAddress for core OpenGL functions */
	return DynamicLib_Get2(gl_lib, function);
}

cc_bool GLContext_SwapBuffers(void) {
	if (!SwapBuffers(ctx_DC)) Process_Abort2(GetLastError(), "Failed to swap buffers");
	return true;
}

void GLContext_SetVSync(cc_bool vsync) {
	if (!_wglSwapIntervalEXT) return;
	_wglSwapIntervalEXT(vsync);
}
void GLContext_GetApiInfo(cc_string* info) { }
#endif
#endif
