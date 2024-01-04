#include "Core.h"
#if defined CC_BUILD_WINGUI && !defined CC_BUILD_SDL
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
/* NOTE: Functions that are not present on Windows 2000 are dynamically loaded. */
/* Hence the actual minimum supported OS is Windows 2000. This just avoids redeclaring structs. */
#endif
#include <windows.h>
#include <commdlg.h>

/* https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setpixelformat */
#define CC_WIN_STYLE WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN
#define CC_WIN_CLASSNAME TEXT("ClassiCube_Window")
#define Rect_Width(rect)  (rect.right  - rect.left)
#define Rect_Height(rect) (rect.bottom - rect.top)

#ifndef WM_XBUTTONDOWN
/* Missing if _WIN32_WINNT isn't defined */
#define WM_XBUTTONDOWN 0x020B
#define WM_XBUTTONUP   0x020C
#endif
#ifndef WM_INPUT
/* Missing when compiling with some older winapi SDKs */
#define WM_INPUT       0x00FF
#endif

static BOOL (WINAPI *_RegisterRawInputDevices)(PCRAWINPUTDEVICE devices, UINT numDevices, UINT size);
static UINT (WINAPI *_GetRawInputData)(HRAWINPUT hRawInput, UINT cmd, void* data, UINT* size, UINT headerSize);
static BOOL (WINAPI* _SetProcessDPIAware)(void);

static HINSTANCE win_instance;
static HWND win_handle;
static HDC win_DC;
static cc_bool suppress_resize;
static int win_totalWidth, win_totalHeight; /* Size of window including titlebar and borders */
static cc_bool is_ansiWindow, grabCursor;
static int windowX, windowY;

static const cc_uint8 key_map[14 * 16] = {
	0, 0, 0, 0, 0, 0, 0, 0, CCKEY_BACKSPACE, CCKEY_TAB, 0, 0, 0, CCKEY_ENTER, 0, 0,
	0, 0, 0, CCKEY_PAUSE, CCKEY_CAPSLOCK, 0, 0, 0, 0, 0, 0, CCKEY_ESCAPE, 0, 0, 0, 0,
	CCKEY_SPACE, CCKEY_PAGEUP, CCKEY_PAGEDOWN, CCKEY_END, CCKEY_HOME, CCKEY_LEFT, CCKEY_UP, CCKEY_RIGHT, CCKEY_DOWN, 0, CCKEY_PRINTSCREEN, 0, CCKEY_PRINTSCREEN, CCKEY_INSERT, CCKEY_DELETE, 0,
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 0, 0, 0, 0, 0, 0,
	0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', CCKEY_LWIN, CCKEY_RWIN, CCKEY_MENU, 0, 0,
	CCKEY_KP0, CCKEY_KP1, CCKEY_KP2, CCKEY_KP3, CCKEY_KP4, CCKEY_KP5, CCKEY_KP6, CCKEY_KP7, CCKEY_KP8, CCKEY_KP9, CCKEY_KP_MULTIPLY, CCKEY_KP_PLUS, 0, CCKEY_KP_MINUS, CCKEY_KP_DECIMAL, CCKEY_KP_DIVIDE,
	CCKEY_F1, CCKEY_F2, CCKEY_F3, CCKEY_F4, CCKEY_F5, CCKEY_F6, CCKEY_F7, CCKEY_F8, CCKEY_F9, CCKEY_F10, CCKEY_F11, CCKEY_F12, CCKEY_F13, CCKEY_F14, CCKEY_F15, CCKEY_F16,
	CCKEY_F17, CCKEY_F18, CCKEY_F19, CCKEY_F20, CCKEY_F21, CCKEY_F22, CCKEY_F23, CCKEY_F24, 0, 0, 0, 0, 0, 0, 0, 0,
	CCKEY_NUMLOCK, CCKEY_SCROLLLOCK, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	CCKEY_LSHIFT, CCKEY_RSHIFT, CCKEY_LCTRL, CCKEY_RCTRL, CCKEY_LALT, CCKEY_RALT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CCKEY_SEMICOLON, CCKEY_EQUALS, CCKEY_COMMA, CCKEY_MINUS, CCKEY_PERIOD, CCKEY_SLASH,
	CCKEY_TILDE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CCKEY_LBRACKET, CCKEY_BACKSLASH, CCKEY_RBRACKET, CCKEY_QUOTE, 0,
};
static int MapNativeKey(WPARAM key, LPARAM meta) {
	LPARAM ext = meta & (1UL << 24);
	switch (key)
	{
	case VK_CONTROL:
		return ext ? CCKEY_RCTRL : CCKEY_LCTRL;
	case VK_MENU:
		return ext ? CCKEY_RALT  : CCKEY_LALT;
	case VK_RETURN:
		return ext ? CCKEY_KP_ENTER : CCKEY_ENTER;
	default:
		return key < Array_Elems(key_map) ? key_map[key] : 0;
	}
}

static void RefreshWindowBounds(void) {
	RECT rect;
	POINT topLeft = { 0, 0 };

	GetWindowRect(win_handle, &rect);
	win_totalWidth  = Rect_Width(rect);
	win_totalHeight = Rect_Height(rect);

	GetClientRect(win_handle, &rect);
	WindowInfo.Width  = Rect_Width(rect);
	WindowInfo.Height = Rect_Height(rect);

	/* GetClientRect always returns 0,0 for left,top (see MSDN) */
	ClientToScreen(win_handle, &topLeft);
	windowX = topLeft.x; windowY = topLeft.y;
}

static void GrabCursor(void) {
	RECT rect;
	if (!grabCursor || !Input.RawMode) return;

	GetWindowRect(win_handle, &rect);
	ClipCursor(&rect);
}

static LRESULT CALLBACK Window_Procedure(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) {
	float wheelDelta;

	switch (message) {
	case WM_ACTIVATE:
		WindowInfo.Focused = LOWORD(wParam) != 0;
		Event_RaiseVoid(&WindowEvents.FocusChanged);
		break;

	case WM_ERASEBKGND:
		return 1; /* Avoid flickering */

	case WM_PAINT:
		ValidateRect(win_handle, NULL);
		Event_RaiseVoid(&WindowEvents.RedrawNeeded);
		return 0;

	case WM_WINDOWPOSCHANGED:
	{
		WINDOWPOS* pos = (WINDOWPOS*)lParam;
		cc_bool sized  = pos->cx != win_totalWidth || pos->cy != win_totalHeight;
		if (pos->hwnd != win_handle) break;

		GrabCursor();
		RefreshWindowBounds();
		if (sized && !suppress_resize) Event_RaiseVoid(&WindowEvents.Resized);
	} break;

	case WM_SIZE:
		Event_RaiseVoid(&WindowEvents.StateChanged);
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
		Mouse_ScrollWheel(wheelDelta);
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
			else Platform_Log1("Unknown key: %x", &wParam);
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
		Event_RaiseVoid(&WindowEvents.Closing);
		if (WindowInfo.Exists) DestroyWindow(win_handle);
		WindowInfo.Exists = false;
		break;

	case WM_DESTROY:
		WindowInfo.Exists = false;
		UnregisterClassW(CC_WIN_CLASSNAME, win_instance);

		if (win_DC) ReleaseDC(win_handle, win_DC);
		break;
	}
	return is_ansiWindow ? DefWindowProcA(handle, message, wParam, lParam)
						 : DefWindowProcW(handle, message, wParam, lParam);
}


/*########################################################################################################################*
*--------------------------------------------------Public implementation--------------------------------------------------*
*#########################################################################################################################*/
void Window_Init(void) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_Sym(RegisterRawInputDevices),
		DynamicLib_Sym(GetRawInputData),
		DynamicLib_Sym(SetProcessDPIAware)
	};
	static const cc_string user32 = String_FromConst("USER32.DLL");
	void* lib;
	HDC hdc;

	DynamicLib_LoadAll(&user32, funcs, Array_Elems(funcs), &lib);
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

static ATOM DoRegisterClass(void) {
	ATOM atom;
	WNDCLASSEXW wc = { 0 };
	wc.cbSize     = sizeof(WNDCLASSEXW);
	wc.style      = CS_OWNDC; /* https://stackoverflow.com/questions/48663815/getdc-releasedc-cs-owndc-with-opengl-and-gdi */
	wc.hInstance  = win_instance;
	wc.lpfnWndProc   = Window_Procedure;
	wc.lpszClassName = CC_WIN_CLASSNAME;

	wc.hIcon   = (HICON)LoadImageA(win_instance, MAKEINTRESOURCEA(1), IMAGE_ICON,
			GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
	wc.hIconSm = (HICON)LoadImageA(win_instance, MAKEINTRESOURCEA(1), IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
	wc.hCursor = LoadCursorA(NULL, IDC_ARROW);

	if ((atom = RegisterClassExW(&wc))) return atom;
	/* Windows 9x does not support W API functions */
	return RegisterClassExA((const WNDCLASSEXA*)&wc);
}

static void CreateWindowHandle(ATOM atom, int width, int height) {
	cc_result res;
	RECT r;
	/* Calculate final window rectangle after window decorations are added (titlebar, borders etc) */
	r.left = Display_CentreX(width);  r.right  = r.left + width;
	r.top  = Display_CentreY(height); r.bottom = r.top  + height;
	AdjustWindowRect(&r, CC_WIN_STYLE, false);

	if ((win_handle = CreateWindowExW(0, MAKEINTATOM(atom), NULL, CC_WIN_STYLE,
		r.left, r.top, Rect_Width(r), Rect_Height(r), NULL, NULL, win_instance, NULL))) return;
	res = GetLastError();

	/* Windows 9x does not support W API functions */
	if (res == ERROR_CALL_NOT_IMPLEMENTED) {
		is_ansiWindow   = true;
		if ((win_handle = CreateWindowExA(0, MAKEINTATOM(atom), NULL, CC_WIN_STYLE,
			r.left, r.top, Rect_Width(r), Rect_Height(r), NULL, NULL, win_instance, NULL))) return;
		res = GetLastError();
	}
	Logger_Abort2(res, "Failed to create window");
}

static void DoCreateWindow(int width, int height) {
	ATOM atom;
	win_instance = GetModuleHandleA(NULL);
	/* TODO: UngroupFromTaskbar(); */
	width  = Display_ScaleX(width);
	height = Display_ScaleY(height);

	atom = DoRegisterClass();
	CreateWindowHandle(atom, width, height);
	RefreshWindowBounds();

	win_DC = GetDC(win_handle);
	if (!win_DC) Logger_Abort2(GetLastError(), "Failed to get device context");

	WindowInfo.Exists = true;
	WindowInfo.Handle = win_handle;
	grabCursor = Options_GetBool(OPT_GRAB_CURSOR, false);
}
void Window_Create2D(int width, int height) { DoCreateWindow(width, height); }
void Window_Create3D(int width, int height) { DoCreateWindow(width, height); }

void Window_SetTitle(const cc_string* title) {
	cc_winstring str;
	Platform_EncodeString(&str, title);
	if (SetWindowTextW(win_handle, str.uni)) return;

	/* Windows 9x does not support W API functions */
	SetWindowTextA(win_handle, str.ansi);
}

void Clipboard_GetText(cc_string* value) {
	cc_bool unicode;
	HANDLE hGlobal;
	LPVOID src;
	SIZE_T size;
	int i;

	/* retry up to 50 times */
	for (i = 0; i < 50; i++) {
		if (!OpenClipboard(win_handle)) {
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
			String_DecodeCP1252(value, src, size - 1);
		}

		GlobalUnlock(hGlobal);
		CloseClipboard();
		return;
	}
}

void Clipboard_SetText(const cc_string* value) {
	cc_unichar* text;
	HANDLE hGlobal;
	int i;

	/* retry up to 10 times */
	for (i = 0; i < 10; i++) {
		if (!OpenClipboard(win_handle)) {
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
	DWORD s = GetWindowLongA(win_handle, GWL_STYLE);

	if ((s & WS_MINIMIZE))                   return WINDOW_STATE_MINIMISED;
	if ((s & WS_MAXIMIZE) && (s & WS_POPUP)) return WINDOW_STATE_FULLSCREEN;
	return WINDOW_STATE_NORMAL;
}

static void ToggleFullscreen(cc_bool fullscreen, UINT finalShow) {
	DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	style |= (fullscreen ? WS_POPUP : WS_OVERLAPPEDWINDOW);

	suppress_resize = true;
	{
		ShowWindow(win_handle, SW_RESTORE); /* reset maximised state */
		SetWindowLongA(win_handle, GWL_STYLE, style);
		SetWindowPos(win_handle, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		ShowWindow(win_handle, finalShow); 
		Window_ProcessEvents(0.0);
	}
	suppress_resize = false;

	/* call Resized event only once */
	RefreshWindowBounds();
	Event_RaiseVoid(&WindowEvents.Resized);
}

static UINT win_show;
cc_result Window_EnterFullscreen(void) {
	WINDOWPLACEMENT w = { 0 };
	w.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(win_handle, &w);

	win_show = w.showCmd;
	ToggleFullscreen(true, SW_MAXIMIZE);
	return 0;
}

cc_result Window_ExitFullscreen(void) {
	ToggleFullscreen(false, win_show);
	return 0;
}

int Window_IsObscured(void) { return 0; }

void Window_Show(void) {
	ShowWindow(win_handle, SW_SHOW);
	BringWindowToTop(win_handle);
	SetForegroundWindow(win_handle);
}

void Window_SetSize(int width, int height) {
	DWORD style = GetWindowLongA(win_handle, GWL_STYLE);
	RECT rect   = { 0, 0, width, height };
	AdjustWindowRect(&rect, style, false);

	SetWindowPos(win_handle, NULL, 0, 0, 
				Rect_Width(rect), Rect_Height(rect), SWP_NOMOVE);
}

void Window_Close(void) {
	PostMessageA(win_handle, WM_CLOSE, 0, 0);
}

void Window_ProcessEvents(double delta) {
	HWND foreground;
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
		WindowInfo.Focused = foreground == win_handle;
	}
}

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
	MessageBoxA(win_handle, msg, title, 0);
}

static cc_result OpenSaveFileDialog(const cc_string* filters, FileDialogCallback callback, cc_bool load,
									const char* const* fileExts, const cc_string* defaultName) {
	union OPENFILENAME_union {
		OPENFILENAMEW wide;
		OPENFILENAMEA ansi;
	} ofn = { 0 }; // less compiler warnings this way
	
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
	/* also note that this only works when you *don't* have OFN_HOOK in Flags - if you do, then */
	/*  on modern Windows versions the dialogs are altered to show an old Win 9x style appearance */
	/* (see https://github.com/geany/geany/issues/578 for example of this problem) */

	ofn.wide.hwndOwner    = win_handle;
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
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	BITMAPINFO hdr = { 0 };
	if (!draw_DC) draw_DC = CreateCompatibleDC(win_DC);
	
	/* sizeof(BITMAPINFO) does not work on Windows 9x */
	hdr.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	hdr.bmiHeader.biWidth    =  bmp->width;
	hdr.bmiHeader.biHeight   = -bmp->height;
	hdr.bmiHeader.biBitCount = 32;
	hdr.bmiHeader.biPlanes   = 1; 

	draw_DIB = CreateDIBSection(draw_DC, &hdr, DIB_RGB_COLORS, (void**)&bmp->scan0, NULL, 0);
	if (!draw_DIB) Logger_Abort2(GetLastError(), "Failed to create DIB");
}

void Window_DrawFramebuffer(Rect2D r) {
	HGDIOBJ oldSrc = SelectObject(draw_DC, draw_DIB);
	BitBlt(win_DC, r.x, r.y, r.Width, r.Height, draw_DC, r.x, r.y, SRCCOPY);
	SelectObject(draw_DC, oldSrc);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	DeleteObject(draw_DIB);
}

static cc_bool rawMouseInited, rawMouseSupported;
static void InitRawMouse(void) {
	RAWINPUTDEVICE rid;

	rawMouseSupported = _RegisterRawInputDevices && _GetRawInputData;
	rawMouseSupported &= Options_GetBool(OPT_RAW_INPUT, true);
	if (!rawMouseSupported) { Platform_LogConst("## Raw input unsupported!"); return; }

	rid.usUsagePage = 1; /* HID_USAGE_PAGE_GENERIC; */
	rid.usUsage     = 2; /* HID_USAGE_GENERIC_MOUSE; */
	rid.dwFlags     = RIDEV_INPUTSINK;
	rid.hwndTarget  = win_handle;

	if (_RegisterRawInputDevices(&rid, 1, sizeof(rid))) return;
	Logger_SysWarn(GetLastError(), "initing raw mouse");
	rawMouseSupported = false;
}

void Window_OpenKeyboard(struct OpenKeyboardArgs* args) { }
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { }

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
#if defined CC_BUILD_GL && !defined CC_BUILD_EGL
static HGLRC ctx_handle;
static HDC ctx_DC;
typedef BOOL (WINAPI *FP_SWAPINTERVAL)(int interval);
static FP_SWAPINTERVAL wglSwapIntervalEXT;

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
	if (modeIndex == 0) { Logger_Abort("Requested graphics mode not available"); }

	Mem_Set(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;

	/* TODO DescribePixelFormat might be unnecessary? */
	DescribePixelFormat(win_DC, modeIndex, pfd.nSize, &pfd);
	if (!SetPixelFormat(win_DC, modeIndex, &pfd)) {
		Logger_Abort2(GetLastError(), "SetPixelFormat failed");
	}
}

void GLContext_Create(void) {
	struct GraphicsMode mode;
	InitGraphicsMode(&mode);
	GLContext_SelectGraphicsMode(&mode);

	ctx_handle = wglCreateContext(win_DC);
	if (!ctx_handle) {
		Logger_Abort2(GetLastError(), "Failed to create OpenGL context");
	}

	if (!wglMakeCurrent(win_DC, ctx_handle)) {
		Logger_Abort2(GetLastError(), "Failed to make OpenGL context current");
	}

	ctx_DC = wglGetCurrentDC();
	wglSwapIntervalEXT = (FP_SWAPINTERVAL)GLContext_GetAddress("wglSwapIntervalEXT");
}

void GLContext_Update(void) { }
cc_bool GLContext_TryRestore(void) { return true; }
void GLContext_Free(void) {
	if (!ctx_handle) return;
	wglDeleteContext(ctx_handle);
	ctx_handle = NULL;
}

/* https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions#Windows */
#define GLContext_IsInvalidAddress(ptr) (ptr == (void*)0 || ptr == (void*)1 || ptr == (void*)-1 || ptr == (void*)2)

void* GLContext_GetAddress(const char* function) {
	static const cc_string glPath = String_FromConst("OPENGL32.dll");
	static void* lib;

	void* addr = (void*)wglGetProcAddress(function);
	if (!GLContext_IsInvalidAddress(addr)) return addr;

	/* Some drivers return NULL from wglGetProcAddress for core OpenGL functions */
	if (!lib) lib = DynamicLib_Load2(&glPath);
	return DynamicLib_Get2(lib, function);
}

cc_bool GLContext_SwapBuffers(void) {
	if (!SwapBuffers(ctx_DC)) Logger_Abort2(GetLastError(), "Failed to swap buffers");
	return true;
}

void GLContext_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	if (!wglSwapIntervalEXT) return;
	wglSwapIntervalEXT(vsync);
}
void GLContext_GetApiInfo(cc_string* info) { }
#endif
#endif
