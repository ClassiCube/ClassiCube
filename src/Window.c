#include "Window.h"
#include "String.h"
#include "Platform.h"
#include "Input.h"
#include "Event.h"
#include "Logger.h"
#include "Funcs.h"
#include "ExtMath.h"
#include "Bitmap.h"
#include "Options.h"
#include "Errors.h"

struct _DisplayData DisplayInfo;
struct _WinData WindowInfo;

int Display_ScaleX(int x) { return (int)(x * DisplayInfo.ScaleX); }
int Display_ScaleY(int y) { return (int)(y * DisplayInfo.ScaleY); }
#define Display_CentreX(width)  (DisplayInfo.X + (DisplayInfo.Width  - width)  / 2)
#define Display_CentreY(height) (DisplayInfo.Y + (DisplayInfo.Height - height) / 2)

#ifndef CC_BUILD_WEB
void Clipboard_RequestText(RequestClipboardCallback callback, void* obj) {
	cc_string text; char textBuffer[2048];
	String_InitArray(text, textBuffer);

	Clipboard_GetText(&text);
	callback(&text, obj);
}
#endif

#ifdef CC_BUILD_IOS
/* iOS implements some functions in external interop_ios.m file */
#define CC_MAYBE_OBJC extern
#else
/* All other platforms implement internally in this file */
#define CC_MAYBE_OBJC static
#endif


static int cursorPrevX, cursorPrevY;
static cc_bool cursorVisible = true;
/* Gets the position of the cursor in screen or window coordinates. */
CC_MAYBE_OBJC void Cursor_GetRawPos(int* x, int* y);
CC_MAYBE_OBJC void Cursor_DoSetVisible(cc_bool visible);

void Cursor_SetVisible(cc_bool visible) {
	if (cursorVisible == visible) return;
	cursorVisible = visible;
	Cursor_DoSetVisible(visible);
}

static void CentreMousePosition(void) {
	Cursor_SetPosition(WindowInfo.Width / 2, WindowInfo.Height / 2);
	/* Fixes issues with large DPI displays on Windows >= 8.0. */
	Cursor_GetRawPos(&cursorPrevX, &cursorPrevY);
}

static void RegrabMouse(void) {
	if (!WindowInfo.Focused || !WindowInfo.Exists) return;
	CentreMousePosition();
}

static void DefaultEnableRawMouse(void) {
	Input_RawMode = true;
	RegrabMouse();
	Cursor_SetVisible(false);
}

static void DefaultUpdateRawMouse(void) {
	int x, y;
	Cursor_GetRawPos(&x, &y);
	Event_RaiseRawMove(&PointerEvents.RawMoved, x - cursorPrevX, y - cursorPrevY);
	CentreMousePosition();
}

static void DefaultDisableRawMouse(void) {
	Input_RawMode = false;
	RegrabMouse();
	Cursor_SetVisible(true);
}

/* The actual windowing system specific method to display a message box */
CC_MAYBE_OBJC void ShowDialogCore(const char* title, const char* msg);
void Window_ShowDialog(const char* title, const char* msg) {
	/* Ensure cursor is visible while showing message box */
	cc_bool visible = cursorVisible;

	if (!visible) Cursor_SetVisible(true);
	ShowDialogCore(title, msg);
	if (!visible) Cursor_SetVisible(false);
}

void OpenKeyboardArgs_Init(struct OpenKeyboardArgs* args, STRING_REF const cc_string* text, int type) {
	args->text = text;
	args->type = type;
	args->placeholder = "";
}


struct GraphicsMode { int R, G, B, A, IsIndexed; };
/* Creates a GraphicsMode compatible with the default display device */
static void InitGraphicsMode(struct GraphicsMode* m) {
	int bpp = DisplayInfo.Depth;
	m->IsIndexed = bpp < 15;

	m->A = 0;
	switch (bpp) {
	case 32:
		m->R = 8; m->G = 8; m->B = 8; m->A = 8; break;
	case 24:
		m->R = 8; m->G = 8; m->B = 8; break;
	case 16:
		m->R = 5; m->G = 6; m->B = 5; break;
	case 15:
		m->R = 5; m->G = 5; m->B = 5; break;
	case 8:
		m->R = 3; m->G = 3; m->B = 2; break;
	case 4:
		m->R = 2; m->G = 2; m->B = 1; break;
	default:
		/* mode->R = 0; mode->G = 0; mode->B = 0; */
		Logger_Abort2(bpp, "Unsupported bits per pixel"); break;
	}
}


/*########################################################################################################################*
*-------------------------------------------------------SDL window--------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_SDL
#include <SDL2/SDL.h>
#include "Graphics.h"
static SDL_Window* win_handle;

static void RefreshWindowBounds(void) {
	SDL_GetWindowSize(win_handle, &WindowInfo.Width, &WindowInfo.Height);
}

static void Window_SDLFail(const char* place) {
	char strBuffer[256];
	cc_string str;
	String_InitArray_NT(str, strBuffer);

	String_Format2(&str, "Error when %c: %c", place, SDL_GetError());
	str.buffer[str.length] = '\0';
	Logger_Abort(str.buffer);
}

void Window_Init(void) {
	SDL_DisplayMode mode = { 0 };
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GetDesktopDisplayMode(0, &mode);

	DisplayInfo.Width  = mode.w;
	DisplayInfo.Height = mode.h;
	DisplayInfo.Depth  = SDL_BITSPERPIXEL(mode.format);
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
}

void Window_Create(int width, int height) {
	int x = Display_CentreX(width);
	int y = Display_CentreY(height);

	/* TODO: Don't set this flag for launcher window */
	win_handle = SDL_CreateWindow(NULL, x, y, width, height, SDL_WINDOW_OPENGL);
	if (!win_handle) Window_SDLFail("creating window");

	RefreshWindowBounds();
	WindowInfo.Exists = true;
	WindowInfo.Handle = win_handle;
}

void Window_SetTitle(const cc_string* title) {
	char str[NATIVE_STR_LEN];
	Platform_EncodeUtf8(str, title);
	SDL_SetWindowTitle(win_handle, str);
}

void Clipboard_GetText(cc_string* value) {
	char* ptr = SDL_GetClipboardText();
	if (!ptr) return;

	int len = String_Length(ptr);
	String_AppendUtf8(value, ptr, len);
	SDL_free(ptr);
}

void Clipboard_SetText(const cc_string* value) {
	char str[NATIVE_STR_LEN];
	Platform_EncodeUtf8(str, value);
	SDL_SetClipboardText(str);
}

void Window_Show(void) { SDL_ShowWindow(win_handle); }

int Window_GetWindowState(void) {
	Uint32 flags = SDL_GetWindowFlags(win_handle);

	if (flags & SDL_WINDOW_MINIMIZED)          return WINDOW_STATE_MINIMISED;
	if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) return WINDOW_STATE_FULLSCREEN;
	return WINDOW_STATE_NORMAL;
}

cc_result Window_EnterFullscreen(void) {
	return SDL_SetWindowFullscreen(win_handle, SDL_WINDOW_FULLSCREEN_DESKTOP);
}
cc_result Window_ExitFullscreen(void) { SDL_RestoreWindow(win_handle); return 0; }

void Window_SetSize(int width, int height) {
	SDL_SetWindowSize(win_handle, width, height);
}

void Window_Close(void) {
	SDL_Event e;
	e.type = SDL_QUIT;
	SDL_PushEvent(&e);
}

static int MapNativeKey(SDL_Keycode k) {
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
		case SDLK_EQUALS: return KEY_EQUALS;
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

static void OnKeyEvent(const SDL_Event* e) {
	cc_bool pressed = e->key.state == SDL_PRESSED;
	int key = MapNativeKey(e->key.keysym.sym);
	if (key) Input_Set(key, pressed);
}

static void OnMouseEvent(const SDL_Event* e) {
	cc_bool pressed = e->button.state == SDL_PRESSED;
	int btn;
	switch (e->button.button) {
		case SDL_BUTTON_LEFT:   btn = KEY_LMOUSE; break;
		case SDL_BUTTON_MIDDLE: btn = KEY_MMOUSE; break;
		case SDL_BUTTON_RIGHT:  btn = KEY_RMOUSE; break;
		case SDL_BUTTON_X1:     btn = KEY_XBUTTON1; break;
		case SDL_BUTTON_X2:     btn = KEY_XBUTTON2; break;
		default: return;
	}
	Input_Set(btn, pressed);
}

static void OnTextEvent(const SDL_Event* e) {
	char buffer[SDL_TEXTINPUTEVENT_TEXT_SIZE];
	cc_string str;
	int i, len;

	String_InitArray(str, buffer);
	len = String_CalcLen(e->text.text, SDL_TEXTINPUTEVENT_TEXT_SIZE);
	String_AppendUtf8(&str, e->text.text, len);

	for (i = 0; i < str.length; i++) {
		Event_RaiseInt(&InputEvents.Press, str.buffer[i]);
	}
}

static void OnWindowEvent(const SDL_Event* e) {
	switch (e->window.event) {
		case SDL_WINDOWEVENT_EXPOSED:
			Event_RaiseVoid(&WindowEvents.Redraw);
			break;
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			RefreshWindowBounds();
			Event_RaiseVoid(&WindowEvents.Resized);
			break;
		case SDL_WINDOWEVENT_MINIMIZED:
		case SDL_WINDOWEVENT_MAXIMIZED:
		case SDL_WINDOWEVENT_RESTORED:
			Event_RaiseVoid(&WindowEvents.StateChanged);
			break;
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			WindowInfo.Focused = true;
			Event_RaiseVoid(&WindowEvents.FocusChanged);
			break;
		case SDL_WINDOWEVENT_FOCUS_LOST:
			WindowInfo.Focused = false;
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
			OnKeyEvent(&e); break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			OnMouseEvent(&e); break;
		case SDL_MOUSEWHEEL:
			Mouse_ScrollWheel(e.wheel.y);
			break;
		case SDL_MOUSEMOTION:
			Pointer_SetPosition(0, e.motion.x, e.motion.y);
			if (Input_RawMode) Event_RaiseRawMove(&PointerEvents.RawMoved, e.motion.xrel, e.motion.yrel);
			break;
		case SDL_TEXTINPUT:
			OnTextEvent(&e); break;
		case SDL_WINDOWEVENT:
			OnWindowEvent(&e); break;

		case SDL_QUIT:
			WindowInfo.Exists = false;
			Event_RaiseVoid(&WindowEvents.Closing);
			SDL_DestroyWindow(win_handle);
			break;

		case SDL_RENDER_DEVICE_RESET:
			Gfx_LoseContext("SDL device reset event");
			Gfx_RecreateContext();
			break;
		}
	}
}

static void Cursor_GetRawPos(int* x, int* y) {
	SDL_GetMouseState(x, y);
}
void Cursor_SetPosition(int x, int y) {
	SDL_WarpMouseInWindow(win_handle, x, y);
}

static void Cursor_DoSetVisible(cc_bool visible) {
	SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

static void ShowDialogCore(const char* title, const char* msg) {
	SDL_ShowSimpleMessageBox(0, title, msg, win_handle);
}

static SDL_Surface* surface;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	surface = SDL_GetWindowSurface(win_handle);
	if (!surface) Window_SDLFail("getting window surface");

	if (SDL_MUSTLOCK(surface)) {
		int ret = SDL_LockSurface(surface);
		if (ret < 0) Window_SDLFail("locking window surface");
	}
	bmp->scan0 = surface->pixels;
}

void Window_DrawFramebuffer(Rect2D r) {
	SDL_Rect rect;
	rect.x = r.X; rect.w = r.Width;
	rect.y = r.Y; rect.h = r.Height;
	SDL_UpdateWindowSurfaceRects(win_handle, &rect, 1);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	/* SDL docs explicitly say to NOT free the surface */
	/* https://wiki.libsdl.org/SDL_GetWindowSurface */
	/* TODO: Do we still need to unlock it though? */
}

void Window_OpenKeyboard(const struct OpenKeyboardArgs* args) { SDL_StartTextInput(); }
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { SDL_StopTextInput(); }

void Window_EnableRawMouse(void) {
	RegrabMouse();
	SDL_SetRelativeMouseMode(true);
	Input_RawMode = true;
}
void Window_UpdateRawMouse(void) { CentreMousePosition(); }

void Window_DisableRawMouse(void) {
	RegrabMouse();
	SDL_SetRelativeMouseMode(false);
	Input_RawMode = false;
}


/*########################################################################################################################*
*------------------------------------------------------Win32 window-------------------------------------------------------*
*#########################################################################################################################*/
#elif defined CC_BUILD_WINGUI
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

#define CC_WIN_STYLE WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN
#define CC_WIN_CLASSNAME TEXT("ClassiCube_Window")
#define Rect_Width(rect)  (rect.right  - rect.left)
#define Rect_Height(rect) (rect.bottom - rect.top)

#ifndef WM_XBUTTONDOWN
/* Missing if _WIN32_WINNT isn't defined */
#define WM_XBUTTONDOWN 0x020B
#define WM_XBUTTONUP   0x020C
#endif

typedef BOOL (WINAPI *FUNC_RegisterRawInput)(PCRAWINPUTDEVICE devices, UINT numDevices, UINT size);
static FUNC_RegisterRawInput _registerRawInput;
typedef UINT (WINAPI *FUNC_GetRawInputData)(HRAWINPUT hRawInput, UINT cmd, void* data, UINT* size, UINT headerSize);
static FUNC_GetRawInputData _getRawInputData;

static HINSTANCE win_instance;
static HWND win_handle;
static HDC win_DC;
static cc_bool suppress_resize;
static int win_totalWidth, win_totalHeight; /* Size of window including titlebar and borders */
static int windowX, windowY;
static cc_bool is_ansiWindow;

static const cc_uint8 key_map[14 * 16] = {
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
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_SEMICOLON, KEY_EQUALS, KEY_COMMA, KEY_MINUS, KEY_PERIOD, KEY_SLASH,
	KEY_TILDE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_LBRACKET, KEY_BACKSLASH, KEY_RBRACKET, KEY_QUOTE, 0,
};
static int MapNativeKey(WPARAM key, LPARAM meta) {
	LPARAM ext = meta & (1UL << 24);
	switch (key)
	{
	case VK_CONTROL:
		return ext ? KEY_RCTRL : KEY_LCTRL;
	case VK_MENU:
		return ext ? KEY_RALT  : KEY_LALT;
	case VK_RETURN:
		return ext ? KEY_KP_ENTER : KEY_ENTER;
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

static LRESULT CALLBACK Window_Procedure(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) {
	char keyChar;
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
		Event_RaiseVoid(&WindowEvents.Redraw);
		return 0;

	case WM_WINDOWPOSCHANGED:
	{
		WINDOWPOS* pos = (WINDOWPOS*)lParam;
		if (pos->hwnd != win_handle) break;
		cc_bool sized = pos->cx != win_totalWidth || pos->cy != win_totalHeight;

		RefreshWindowBounds();
		if (sized && !suppress_resize) Event_RaiseVoid(&WindowEvents.Resized);
	} break;

	case WM_SIZE:
		Event_RaiseVoid(&WindowEvents.StateChanged);
		break;

	case WM_CHAR:
		/* TODO: Use WM_UNICHAR instead, as WM_CHAR is just utf16 */
		if (Convert_TryCodepointToCP437((cc_unichar)wParam, &keyChar)) {
			Event_RaiseInt(&InputEvents.Press, keyChar);
		}
		break;

	case WM_MOUSEMOVE:
		/* Set before position change, in case mouse buttons changed when outside window */
		Input_SetNonRepeatable(KEY_LMOUSE, wParam & 0x01);
		Input_SetNonRepeatable(KEY_RMOUSE, wParam & 0x02);
		Input_SetNonRepeatable(KEY_MMOUSE, wParam & 0x10);
		/* TODO: do we need to set XBUTTON1/XBUTTON2 here */
		Pointer_SetPosition(0, LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_MOUSEWHEEL:
		wheelDelta = ((short)HIWORD(wParam)) / (float)WHEEL_DELTA;
		Mouse_ScrollWheel(wheelDelta);
		return 0;

	case WM_LBUTTONDOWN:
		Input_SetPressed(KEY_LMOUSE); break;
	case WM_MBUTTONDOWN:
		Input_SetPressed(KEY_MMOUSE); break;
	case WM_RBUTTONDOWN:
		Input_SetPressed(KEY_RMOUSE); break;
	case WM_XBUTTONDOWN:
		Input_SetPressed(HIWORD(wParam) == 1 ? KEY_XBUTTON1 : KEY_XBUTTON2);
		break;

	case WM_LBUTTONUP:
		Input_SetReleased(KEY_LMOUSE); break;
	case WM_MBUTTONUP:
		Input_SetReleased(KEY_MMOUSE); break;
	case WM_RBUTTONUP:
		Input_SetReleased(KEY_RMOUSE); break;
	case WM_XBUTTONUP:
		Input_SetReleased(HIWORD(wParam) == 1 ? KEY_XBUTTON1 : KEY_XBUTTON2);
		break;

	case WM_INPUT:
	{
		RAWINPUT raw;
		UINT ret, rawSize = sizeof(RAWINPUT);
		int dx, dy;

		ret = _getRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &rawSize, sizeof(RAWINPUTHEADER));
		if (ret == -1 || raw.header.dwType != RIM_TYPEMOUSE) break;		

		if (raw.data.mouse.usFlags == MOUSE_MOVE_RELATIVE) {
			dx = raw.data.mouse.lLastX;
			dy = raw.data.mouse.lLastY;
		} else if (raw.data.mouse.usFlags == MOUSE_MOVE_ABSOLUTE) {
			static int prevPosX, prevPosY;
			dx = raw.data.mouse.lLastX - prevPosX;
			dy = raw.data.mouse.lLastY - prevPosY;

			prevPosX = raw.data.mouse.lLastX;
			prevPosY = raw.data.mouse.lLastY;
		} else { break; }

		if (Input_RawMode) Event_RaiseRawMove(&PointerEvents.RawMoved, (float)dx, (float)dy);
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
				Input_Set(KEY_LSHIFT, lShiftDown);
				Input_Set(KEY_RSHIFT, rShiftDown);
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
	HDC hdc = GetDC(NULL);
	DisplayInfo.Width  = GetSystemMetrics(SM_CXSCREEN);
	DisplayInfo.Height = GetSystemMetrics(SM_CYSCREEN);
	DisplayInfo.Depth  = GetDeviceCaps(hdc, BITSPIXEL);
	DisplayInfo.ScaleX = GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
	DisplayInfo.ScaleY = GetDeviceCaps(hdc, LOGPIXELSY) / 96.0f;
	ReleaseDC(NULL, hdc);
}

static ATOM DoRegisterClass(void) {
	ATOM atom;
	WNDCLASSEXW wc = { 0 };
	wc.cbSize     = sizeof(WNDCLASSEXW);
	wc.style      = CS_OWNDC;
	wc.hInstance  = win_instance;
	wc.lpfnWndProc   = Window_Procedure;
	wc.lpszClassName = CC_WIN_CLASSNAME;

	wc.hIcon   = (HICON)LoadImage(win_instance, MAKEINTRESOURCE(1), IMAGE_ICON,
			GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
	wc.hIconSm = (HICON)LoadImage(win_instance, MAKEINTRESOURCE(1), IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
	wc.hCursor = LoadCursorA(NULL, IDC_ARROW);

	if ((atom = RegisterClassExW(&wc))) return atom;
	/* Windows 9x does not support W API functions */
	return RegisterClassExA((const WNDCLASSEXA*)&wc);
}

static void DoCreateWindow(ATOM atom, int width, int height) {
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

void Window_Create(int width, int height) {
	ATOM atom;
	win_instance = GetModuleHandle(NULL);
	/* TODO: UngroupFromTaskbar(); */
	width  = Display_ScaleX(width);
	height = Display_ScaleY(height);

	atom = DoRegisterClass();
	DoCreateWindow(atom, width, height);
	RefreshWindowBounds();

	win_DC = GetDC(win_handle);
	if (!win_DC) Logger_Abort2(GetLastError(), "Failed to get device context");
	WindowInfo.Exists = true;
	WindowInfo.Handle = win_handle;
}

void Window_SetTitle(const cc_string* title) {
	WCHAR str[NATIVE_STR_LEN];
	Platform_EncodeUtf16(str, title);
	SetWindowTextW(win_handle, str);
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

void Window_Show(void) {
	ShowWindow(win_handle, SW_SHOW);
	BringWindowToTop(win_handle);
	SetForegroundWindow(win_handle);
}

int Window_GetWindowState(void) {
	DWORD s = GetWindowLongW(win_handle, GWL_STYLE);

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
		SetWindowLongW(win_handle, GWL_STYLE, style);
		SetWindowPos(win_handle, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		ShowWindow(win_handle, finalShow); 
		Window_ProcessEvents();
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


void Window_SetSize(int width, int height) {
	DWORD style = GetWindowLongW(win_handle, GWL_STYLE);
	RECT rect   = { 0, 0, width, height };
	AdjustWindowRect(&rect, style, false);

	SetWindowPos(win_handle, NULL, 0, 0, 
				Rect_Width(rect), Rect_Height(rect), SWP_NOMOVE);
}

void Window_Close(void) {
	PostMessageW(win_handle, WM_CLOSE, 0, 0);
}

void Window_ProcessEvents(void) {
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
	if (visible) {
		for (i = 0; i < 10 && ShowCursor(true)  <  0; i++) { }
	} else {
		for (i = 0; i < 10 && ShowCursor(false) >= 0; i++) {}
	}
}

static void ShowDialogCore(const char* title, const char* msg) {
	MessageBoxA(win_handle, msg, title, 0);
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
	BitBlt(win_DC, r.X, r.Y, r.Width, r.Height, draw_DC, r.X, r.Y, SRCCOPY);
	SelectObject(draw_DC, oldSrc);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	DeleteObject(draw_DIB);
}

static cc_bool rawMouseInited, rawMouseSupported;
static void InitRawMouse(void) {
	static const cc_string user32 = String_FromConst("USER32.DLL");
	void* lib;
	RAWINPUTDEVICE rid;

	if ((lib = DynamicLib_Load2(&user32))) {
		_registerRawInput = (FUNC_RegisterRawInput)DynamicLib_Get2(lib, "RegisterRawInputDevices");
		_getRawInputData  = (FUNC_GetRawInputData) DynamicLib_Get2(lib, "GetRawInputData");
		rawMouseSupported = _registerRawInput && _getRawInputData;
	}
	if (!rawMouseSupported) { Platform_LogConst("Raw input unsupported!"); return; }

	rid.usUsagePage = 1; /* HID_USAGE_PAGE_GENERIC; */
	rid.usUsage     = 2; /* HID_USAGE_GENERIC_MOUSE; */
	rid.dwFlags     = RIDEV_INPUTSINK;
	rid.hwndTarget  = win_handle;

	if (_registerRawInput(&rid, 1, sizeof(rid))) return;
	Logger_SysWarn(GetLastError(), "initing raw mouse");
	rawMouseSupported = false;
}

void Window_OpenKeyboard(const struct OpenKeyboardArgs* args) { }
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { }

void Window_EnableRawMouse(void) {
	DefaultEnableRawMouse();
	if (!rawMouseInited) InitRawMouse();
	rawMouseInited = true;
}

void Window_UpdateRawMouse(void) {
	if (rawMouseSupported) {
		/* handled in WM_INPUT messages */
		CentreMousePosition();
	} else {
		DefaultUpdateRawMouse();
	}
}

void Window_DisableRawMouse(void) { DefaultDisableRawMouse(); }


/*########################################################################################################################*
*-------------------------------------------------------X11 window--------------------------------------------------------*
*#########################################################################################################################*/
#elif defined CC_BUILD_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XInput2.h>

#ifdef X_HAVE_UTF8_STRING
#define CC_BUILD_XIM
/* XIM support based off details described in */
/* https://tedyin.com/posts/a-brief-intro-to-linux-input-method-framework/ */
#endif

#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD    1
#define _NET_WM_STATE_TOGGLE 2

static Display* win_display;
static Window win_rootWin, win_handle;
static XVisualInfo win_visual;
#ifdef CC_BUILD_XIM
static XIM win_xim;
static XIC win_xic;
#endif
 
static Atom wm_destroy, net_wm_state, net_wm_ping;
static Atom net_wm_state_minimized;
static Atom net_wm_state_fullscreen;

static Atom xa_clipboard, xa_targets, xa_utf8_string, xa_data_sel;
static Atom xa_atom = 4;
static long win_eventMask;
static cc_bool grabCursor;

static int MapNativeKey(KeySym key, unsigned int state) {
	if (key >= XK_0 && key <= XK_9) { return '0' + (key - XK_0); }
	if (key >= XK_A && key <= XK_Z) { return 'A' + (key - XK_A); }
	if (key >= XK_a && key <= XK_z) { return 'A' + (key - XK_a); }

	if (key >= XK_F1 && key <= XK_F24)    { return KEY_F1  + (key - XK_F1); }
	if (key >= XK_KP_0 && key <= XK_KP_9) { return KEY_KP0 + (key - XK_KP_0); }

	/* Same Num Lock behaviour as Windows and text editors */
	if (key >= XK_KP_Home && key <= XK_KP_Delete && !(state & Mod2Mask)) {
		if (key == XK_KP_Home) return KEY_HOME;
		if (key == XK_KP_Up)   return KEY_UP;
		if (key == XK_KP_Page_Up) return KEY_PAGEUP;

		if (key == XK_KP_Left)   return KEY_LEFT;
		if (key == XK_KP_Insert) return KEY_INSERT;
		if (key == XK_KP_Right)  return KEY_RIGHT;

		if (key == XK_KP_End)  return KEY_END;
		if (key == XK_KP_Down) return KEY_DOWN;
		if (key == XK_KP_Page_Down) return KEY_PAGEDOWN;
	}

	/* A chromebook user reported issues with pressing some keys: */
	/*  tilde - "Unknown key press: (8000060, 800007E) */
	/*  quote - "Unknown key press: (8000027, 8000022) */
	/* Note if 8000 is stripped, you get '0060' (XK_grave) and 0027 (XK_apostrophe) */
	/*   ChromeOS seems to also mask to 0xFFFF, so I also do so here */
	/* https://chromium.googlesource.com/chromium/src/+/lkgr/ui/events/keycodes/keyboard_code_conversion_x.cc */
	key &= 0xFFFF;

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

		case XK_Menu:  return KEY_MENU;
		case XK_Tab:   return KEY_TAB;
		case XK_minus: return KEY_MINUS;
		case XK_plus:  return KEY_EQUALS;
		case XK_equal: return KEY_EQUALS;

		case XK_Caps_Lock: return KEY_CAPSLOCK;
		case XK_Num_Lock:  return KEY_NUMLOCK;

		case XK_Pause: return KEY_PAUSE;
		case XK_Break: return KEY_PAUSE;
		case XK_Scroll_Lock: return KEY_SCROLLLOCK;
		case XK_Insert:  return KEY_INSERT;
		case XK_Print:   return KEY_PRINTSCREEN;
		case XK_Sys_Req: return KEY_PRINTSCREEN;

		case XK_backslash: return KEY_BACKSLASH;
		case XK_bar:       return KEY_BACKSLASH;
		case XK_braceleft:    return KEY_LBRACKET;
		case XK_bracketleft:  return KEY_LBRACKET;
		case XK_braceright:   return KEY_RBRACKET;
		case XK_bracketright: return KEY_RBRACKET;
		case XK_colon:      return KEY_SEMICOLON;
		case XK_semicolon:  return KEY_SEMICOLON;
		case XK_quoteright: return KEY_QUOTE;
		case XK_quotedbl:   return KEY_QUOTE;
		case XK_quoteleft:  return KEY_TILDE;
		case XK_asciitilde: return KEY_TILDE;

		case XK_comma: return KEY_COMMA;
		case XK_less:  return KEY_COMMA;
		case XK_period:  return KEY_PERIOD;
		case XK_greater: return KEY_PERIOD;
		case XK_slash:    return KEY_SLASH;
		case XK_question: return KEY_SLASH;

		case XK_Left:  return KEY_LEFT;
		case XK_Down:  return KEY_DOWN;
		case XK_Right: return KEY_RIGHT;
		case XK_Up:    return KEY_UP;

		case XK_Delete: return KEY_DELETE;
		case XK_Home:   return KEY_HOME;
		case XK_End:    return KEY_END;
		case XK_Page_Up:   return KEY_PAGEUP;
		case XK_Page_Down: return KEY_PAGEDOWN;

		case XK_KP_Add: return KEY_KP_PLUS;
		case XK_KP_Subtract: return KEY_KP_MINUS;
		case XK_KP_Multiply: return KEY_KP_MULTIPLY;
		case XK_KP_Divide:  return KEY_KP_DIVIDE;
		case XK_KP_Decimal: return KEY_KP_DECIMAL;
		case XK_KP_Insert: return KEY_KP0;
		case XK_KP_End:  return KEY_KP1;
		case XK_KP_Down: return KEY_KP2;
		case XK_KP_Page_Down: return KEY_KP3;
		case XK_KP_Left:  return KEY_KP4;
		case XK_KP_Begin: return KEY_KP5;
		case XK_KP_Right: return KEY_KP6;
		case XK_KP_Home: return KEY_KP7;
		case XK_KP_Up:   return KEY_KP8;
		case XK_KP_Page_Up: return KEY_KP9;
		case XK_KP_Delete:  return KEY_KP_DECIMAL;
		case XK_KP_Enter:   return KEY_KP_ENTER;
	}
	return KEY_NONE;
}

/* NOTE: This may not be entirely accurate, because user can configure keycode mappings */
static const cc_uint8 keycodeMap[136] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_ESCAPE, '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', KEY_MINUS, KEY_EQUALS, KEY_BACKSPACE, KEY_TAB, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
	'O',  'P', KEY_LBRACKET, KEY_RBRACKET, KEY_ENTER, KEY_LCTRL, 'A', 'S', 'D', 'F', 'G', 'H',  'J', 'K', 'L', KEY_SEMICOLON,
	KEY_QUOTE, KEY_TILDE, KEY_LSHIFT, KEY_BACKSLASH, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', KEY_PERIOD, KEY_COMMA, KEY_SLASH, KEY_RSHIFT, KEY_KP_MULTIPLY,
	KEY_LALT, KEY_SPACE, KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUMLOCK, KEY_SCROLLLOCK, KEY_KP7,
	KEY_KP8, KEY_KP9, KEY_KP_MINUS, KEY_KP4, KEY_KP5, KEY_KP6, KEY_KP_PLUS, KEY_KP1, KEY_KP2, KEY_KP3, KEY_KP0, KEY_KP_DECIMAL, 0, 0, 0, KEY_F11,
	KEY_F12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	KEY_RALT, KEY_RCTRL, KEY_HOME, KEY_UP, KEY_PAGEUP, KEY_LEFT, KEY_RIGHT, KEY_END, KEY_DOWN, KEY_PAGEDOWN, KEY_INSERT, KEY_DELETE, 0, 0, 0, 0,
	0, 0, 0, KEY_PAUSE, 0, 0, 0, 0, 0, KEY_LWIN, 0, KEY_RWIN
};

static int MapNativeKeycode(unsigned int keycode) {
	return keycode < Array_Elems(keycodeMap) ? keycodeMap[keycode] : 0;
}

static void RegisterAtoms(void) {
	Display* display = win_display;
	wm_destroy = XInternAtom(display, "WM_DELETE_WINDOW", true);
	net_wm_state = XInternAtom(display, "_NET_WM_STATE", false);
	net_wm_ping  = XInternAtom(display, "_NET_WM_PING",  false);
	net_wm_state_minimized  = XInternAtom(display, "_NET_WM_STATE_MINIMIZED",  false);
	net_wm_state_fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", false);

	xa_clipboard   = XInternAtom(display, "CLIPBOARD",   false);
	xa_targets     = XInternAtom(display, "TARGETS",     false);
	xa_utf8_string = XInternAtom(display, "UTF8_STRING", false);
	xa_data_sel    = XInternAtom(display, "CC_SEL_DATA", false);
}

static void RefreshWindowBounds(int width, int height) {
	if (width != WindowInfo.Width || height != WindowInfo.Height) {
		WindowInfo.Width  = width;
		WindowInfo.Height = height;
		Event_RaiseVoid(&WindowEvents.Resized);
	}
}

typedef int (*X11_ErrorHandler)(Display* dpy, XErrorEvent* ev);
typedef int (*X11_IOErrorHandler)(Display* dpy);
static X11_ErrorHandler   realXErrorHandler;
static X11_IOErrorHandler realXIOErrorHandler;

static void LogXErrorCore(const char* msg) {
	char traceBuffer[2048];
	cc_string trace;
	Platform_LogConst(msg);

	String_InitArray(trace, traceBuffer);
	Logger_Backtrace(&trace, NULL);
	Platform_Log(traceBuffer, trace.length);
}

static int LogXError(Display* dpy, XErrorEvent* ev) {
	LogXErrorCore("== unhandled X11 error ==");
	return realXErrorHandler(dpy, ev);
}

static int LogXIOError(Display* dpy) {
	LogXErrorCore("== unhandled XIO error ==");
	return realXIOErrorHandler(dpy);
}

static void HookXErrors(void) {
	realXErrorHandler   = XSetErrorHandler(LogXError);
	realXIOErrorHandler = XSetIOErrorHandler(LogXIOError);
}


/*########################################################################################################################*
*--------------------------------------------------Public implementation--------------------------------------------------*
*#########################################################################################################################*/
static XVisualInfo GLContext_SelectVisual(void);
void Window_Init(void) {
	Display* display = XOpenDisplay(NULL);
	int screen;

	if (!display) Logger_Abort("Failed to open display");
	screen = DefaultScreen(display);
	HookXErrors();

	win_display = display;
	win_rootWin = RootWindow(display, screen);

	/* TODO: Use Xinerama and XRandR for querying these */
	DisplayInfo.Width  = DisplayWidth(display,  screen);
	DisplayInfo.Height = DisplayHeight(display, screen);
	DisplayInfo.Depth  = DefaultDepth(display,  screen);
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
}

#ifdef CC_BUILD_ICON
extern const long CCIcon_Data[];
extern const int  CCIcon_Size;

static void ApplyIcon(void) {
	Atom net_wm_icon = XInternAtom(win_display, "_NET_WM_ICON", false);
	Atom xa_cardinal = XInternAtom(win_display, "CARDINAL", false);
	XChangeProperty(win_display, win_handle, net_wm_icon, xa_cardinal, 32, PropModeReplace, CCIcon_Data, CCIcon_Size);
}
#else
static void ApplyIcon(void) { }
#endif

void Window_Create(int width, int height) {
	XSetWindowAttributes attributes = { 0 };
	XSizeHints hints = { 0 };
	Atom protocols[2];
	int supported, x, y;

	x = Display_CentreX(width);
	y = Display_CentreY(height);
	RegisterAtoms();

	win_eventMask = StructureNotifyMask /*| SubstructureNotifyMask*/ | ExposureMask |
		KeyReleaseMask  | KeyPressMask    | KeymapStateMask   | PointerMotionMask |
		FocusChangeMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask |
		LeaveWindowMask | PropertyChangeMask;
	win_visual = GLContext_SelectVisual();

	Platform_LogConst("Opening render window... ");
	attributes.colormap   = XCreateColormap(win_display, win_rootWin, win_visual.visual, AllocNone);
	attributes.event_mask = win_eventMask;

	win_handle = XCreateWindow(win_display, win_rootWin, x, y, width, height,
		0, win_visual.depth /* CopyFromParent*/, InputOutput, win_visual.visual, 
		CWColormap | CWEventMask | CWBackPixel | CWBorderPixel, &attributes);
	if (!win_handle) Logger_Abort("XCreateWindow failed");

#ifdef CC_BUILD_XIM
	win_xim = XOpenIM(win_display, NULL, NULL, NULL);
	win_xic = XCreateIC(win_xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
						XNClientWindow, win_handle, NULL);
#endif

	/* Set hints to try to force WM to create window at requested x,y */
	/* Without this, some WMs will instead place the window whereever */
	hints.base_width  = width;
	hints.base_height = height;
	hints.flags = PSize | PPosition;
	XSetWMNormalHints(win_display, win_handle, &hints);

	/* Register for window destroy notification */
	protocols[0] = wm_destroy;
	protocols[1] = net_wm_ping;
	XSetWMProtocols(win_display, win_handle, protocols, 2);

	/* Request that auto-repeat is only set on devices that support it physically.
	   This typically means that it's turned off for keyboards (which is what we want).
	   We prefer this method over XAutoRepeatOff/On, because the latter needs to
	   be reset before the program exits. */
	XkbSetDetectableAutoRepeat(win_display, true, &supported);

	RefreshWindowBounds(width, height);
	WindowInfo.Exists = true;
	WindowInfo.Handle = (void*)win_handle;
	grabCursor = Options_GetBool(OPT_GRAB_CURSOR, false);
	
	/* So right name appears in e.g. Ubuntu Unity launchbar */
	XClassHint hint = { 0 };
	hint.res_name   = GAME_APP_TITLE;
	hint.res_class  = GAME_APP_TITLE;
	XSetClassHint(win_display, win_handle, &hint);
	ApplyIcon();
}

void Window_SetTitle(const cc_string* title) {
	char str[NATIVE_STR_LEN];
	Platform_EncodeUtf8(str, title);
	XStoreName(win_display, win_handle, str);
}

static char clipboard_copy_buffer[256];
static char clipboard_paste_buffer[256];
static cc_string clipboard_copy_text  = String_FromArray(clipboard_copy_buffer);
static cc_string clipboard_paste_text = String_FromArray(clipboard_paste_buffer);

void Clipboard_GetText(cc_string* value) {
	Window owner = XGetSelectionOwner(win_display, xa_clipboard);
	int i;
	if (!owner) return; /* no window owner */

	XConvertSelection(win_display, xa_clipboard, xa_utf8_string, xa_data_sel, win_handle, 0);
	clipboard_paste_text.length = 0;

	/* wait up to 1 second for SelectionNotify event to arrive */
	for (i = 0; i < 100; i++) {
		Window_ProcessEvents();
		if (clipboard_paste_text.length) {
			String_AppendString(value, &clipboard_paste_text);
			return;
		} else {
			Thread_Sleep(10);
		}
	}
}

void Clipboard_SetText(const cc_string* value) {
	String_Copy(&clipboard_copy_text, value);
	XSetSelectionOwner(win_display, xa_clipboard, win_handle, 0);
}

void Window_Show(void) { XMapWindow(win_display, win_handle); }

int Window_GetWindowState(void) {
	cc_bool fullscreen = false, minimised = false;
	Atom prop_type;
	unsigned long items, after;
	int i, prop_format;
	Atom* data = NULL;

	XGetWindowProperty(win_display, win_handle,
		net_wm_state, 0, 256, false, xa_atom, &prop_type,
		&prop_format, &items, &after, &data);

	if (data) {
		for (i = 0; i < items; i++) {
			Atom atom = data[i];

			if (atom == net_wm_state_minimized) {
				minimised  = true;
			} else if (atom == net_wm_state_fullscreen) {
				fullscreen = true;
			}
		}
		XFree(data);
	}

	if (fullscreen) return WINDOW_STATE_FULLSCREEN;
	if (minimised)  return WINDOW_STATE_MINIMISED;
	return WINDOW_STATE_NORMAL;
}

static void ToggleFullscreen(long op) {
	XEvent ev = { 0 };
	ev.xclient.type   = ClientMessage;
	ev.xclient.window = win_handle;
	ev.xclient.message_type = net_wm_state;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = op;
	ev.xclient.data.l[1] = net_wm_state_fullscreen;

	XSendEvent(win_display, win_rootWin, false,
		SubstructureRedirectMask | SubstructureNotifyMask, &ev);
	XSync(win_display, false);
	XRaiseWindow(win_display, win_handle);
	Window_ProcessEvents();
}

cc_result Window_EnterFullscreen(void) {
	ToggleFullscreen(_NET_WM_STATE_ADD); return 0;
}
cc_result Window_ExitFullscreen(void) {
	ToggleFullscreen(_NET_WM_STATE_REMOVE); return 0;
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

static int MapNativeMouse(int button) {
	if (button == 1) return KEY_LMOUSE;
	if (button == 2) return KEY_MMOUSE;
	if (button == 3) return KEY_RMOUSE;
	if (button == 8) return KEY_XBUTTON1;
	if (button == 9) return KEY_XBUTTON2;
	return 0;
}

static int TryGetKey(XKeyEvent* ev) {
	KeySym keysym1 = XLookupKeysym(ev, 0);
	KeySym keysym2 = XLookupKeysym(ev, 1);

	int key = MapNativeKey(keysym1, ev->state);
	if (!key) key = MapNativeKey(keysym2, ev->state);
	if (key) return key;

	Platform_Log3("Unknown key %i (%x, %x)", &ev->keycode, &keysym1, &keysym2);
	/* The user may be using a keyboard layout such as cryllic - */
	/*   fallback to trying to conver the raw scancodes instead */
	return MapNativeKeycode(ev->keycode);
}

static Atom Window_GetSelectionProperty(XEvent* e) {
	Atom prop = e->xselectionrequest.property;
	if (prop) return prop;

	/* For obsolete clients. See ICCCM spec, selections chapter for reasoning. */
	return e->xselectionrequest.target;
}

static Bool FilterEvent(Display* d, XEvent* e, XPointer w) { 
	return
		e->xany.window == (Window)w ||
		!e->xany.window || /* KeymapNotify events don't have a window */
		e->type == GenericEvent; /* For XInput events */
}

static void HandleWMDestroy(void) {
	Platform_LogConst("Exit message received.");
	Event_RaiseVoid(&WindowEvents.Closing);

	/* sync and discard all events queued */
	XSync(win_display, true);
	XDestroyWindow(win_display, win_handle);
	WindowInfo.Exists = false;
}

static void HandleWMPing(XEvent* e) {
	e->xany.window = win_rootWin;
	XSendEvent(win_display, win_rootWin, false,
		SubstructureRedirectMask | SubstructureNotifyMask, e);
}
static void HandleGenericEvent(XEvent* e);

void Window_ProcessEvents(void) {
	XEvent e;
	int i, btn, key, status;

	while (WindowInfo.Exists) {
		if (!XCheckIfEvent(win_display, &e, FilterEvent, (XPointer)win_handle)) break;
		if (XFilterEvent(&e, None) == True) continue;

		switch (e.type) {
		case GenericEvent:
			HandleGenericEvent(&e); break;
		case ClientMessage:
			if (e.xclient.data.l[0] == wm_destroy) {
				HandleWMDestroy();
			} else if (e.xclient.data.l[0] == net_wm_ping) {
				HandleWMPing(&e);
			}
			break;

		case DestroyNotify:
			Platform_LogConst("Window destroyed");
			WindowInfo.Exists = false;
			break;

		case ConfigureNotify:
			RefreshWindowBounds(e.xconfigure.width, e.xconfigure.height);
			break;

		case Expose:
			if (e.xexpose.count == 0) Event_RaiseVoid(&WindowEvents.Redraw);
			break;

		case KeyPress:
		{
			char data[64], c;
			key = TryGetKey(&e.xkey);
			if (key) Input_SetPressed(key);
			
#ifdef CC_BUILD_XIM
			cc_codepoint cp;
			char* chars = data;

			status = Xutf8LookupString(win_xic, &e.xkey, data, Array_Elems(data), NULL, NULL);
			for (; status > 0; status -= i) {
				i = Convert_Utf8ToCodepoint(&cp, chars, status);
				if (!i) break;

				if (Convert_TryCodepointToCP437(cp, &c)) Event_RaiseInt(&InputEvents.Press, c);
				chars += i;
			}
#else
			/* This only really works for latin keys (e.g. so some finnish keys still work) */
			status = XLookupString(&e.xkey, data, Array_Elems(data), NULL, NULL);
			for (i = 0; i < status; i++) {
				if (!Convert_TryCodepointToCP437((cc_uint8)data[i], &c)) continue;
				Event_RaiseInt(&InputEvents.Press, c);
			}
#endif
		} break;

		case KeyRelease:
			key = TryGetKey(&e.xkey);
			if (key) Input_SetReleased(key);
			break;

		case ButtonPress:
			btn = MapNativeMouse(e.xbutton.button);
			if (btn) Input_SetPressed(btn);
			else if (e.xbutton.button == 4) Mouse_ScrollWheel(+1);
			else if (e.xbutton.button == 5) Mouse_ScrollWheel(-1);
			break;

		case ButtonRelease:
			btn = MapNativeMouse(e.xbutton.button);
			if (btn) Input_SetReleased(btn);
			break;

		case MotionNotify:
			Pointer_SetPosition(0, e.xmotion.x, e.xmotion.y);
			break;

		case FocusIn:
		case FocusOut:
			/* Don't lose focus when another app grabs key or mouse */
			if (e.xfocus.mode == NotifyGrab || e.xfocus.mode == NotifyUngrab) break;

			WindowInfo.Focused = e.type == FocusIn;
			Event_RaiseVoid(&WindowEvents.FocusChanged);
			/* TODO: Keep track of keyboard when focus is lost */
			if (!WindowInfo.Focused) Input_Clear();
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
			break;

		case SelectionNotify:
			clipboard_paste_text.length = 0;

			if (e.xselection.selection == xa_clipboard && e.xselection.target == xa_utf8_string && e.xselection.property == xa_data_sel) {
				Atom prop_type;
				int prop_format;
				unsigned long items, after;
				cc_uint8* data = NULL;

				XGetWindowProperty(win_display, win_handle, xa_data_sel, 0, 1024, false, 0,
					&prop_type, &prop_format, &items, &after, &data);
				XDeleteProperty(win_display, win_handle, xa_data_sel);

				if (data && items && prop_type == xa_utf8_string) {
					clipboard_paste_text.length = 0;
					String_AppendUtf8(&clipboard_paste_text, data, items);
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
			reply.xselection.property = 0;
			reply.xselection.time = e.xselectionrequest.time;

			if (e.xselectionrequest.selection == xa_clipboard && e.xselectionrequest.target == xa_utf8_string && clipboard_copy_text.length) {
				reply.xselection.property = Window_GetSelectionProperty(&e);
				char str[800];
				int len = Platform_EncodeUtf8(str, &clipboard_copy_text);

				XChangeProperty(win_display, reply.xselection.requestor, reply.xselection.property, xa_utf8_string, 8,
					PropModeReplace, (unsigned char*)str, len);
			} else if (e.xselectionrequest.selection == xa_clipboard && e.xselectionrequest.target == xa_targets) {
				reply.xselection.property = Window_GetSelectionProperty(&e);

				Atom data[2] = { xa_utf8_string, xa_targets };
				XChangeProperty(win_display, reply.xselection.requestor, reply.xselection.property, xa_atom, 32,
					PropModeReplace, (unsigned char*)data, 2);
			}
			XSendEvent(win_display, e.xselectionrequest.requestor, true, 0, &reply);
		} break;
		}
	}
}

static void Cursor_GetRawPos(int* x, int* y) {
	Window rootW, childW;
	int childX, childY;
	unsigned int mask;
	XQueryPointer(win_display, win_rootWin, &rootW, &childW, x, y, &childX, &childY, &mask);
}

void Cursor_SetPosition(int x, int y) {
	XWarpPointer(win_display, None, win_handle, 0, 0, 0, 0, x, y);
	XFlush(win_display); /* TODO: not sure if XFlush call is necessary */
}

static Cursor blankCursor;
static void Cursor_DoSetVisible(cc_bool visible) {
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
struct X11MessageBox {
	Window win;
	Display* dpy;
	GC gc;
	unsigned long white, black, background;
	unsigned long btnBorder, highlight, shadow;
};

static unsigned long X11_Col(struct X11MessageBox* m, cc_uint8 r, cc_uint8 g, cc_uint8 b) {
	Colormap cmap = XDefaultColormap(m->dpy, DefaultScreen(m->dpy));
	XColor col = { 0 };
	col.red   = r << 8;
	col.green = g << 8;
	col.blue  = b << 8;
	col.flags = DoRed | DoGreen | DoBlue;

	XAllocColor(m->dpy, cmap, &col);
	return col.pixel;
}

static void X11MessageBox_Init(struct X11MessageBox* m) {
	m->black = BlackPixel(m->dpy, DefaultScreen(m->dpy));
	m->white = WhitePixel(m->dpy, DefaultScreen(m->dpy));
	m->background = X11_Col(m, 206, 206, 206);

	m->btnBorder = X11_Col(m, 60,  60,  60);
	m->highlight = X11_Col(m, 144, 144, 144);
	m->shadow    = X11_Col(m, 49,  49,  49);

	m->win = XCreateSimpleWindow(m->dpy, DefaultRootWindow(m->dpy), 0, 0, 100, 100,
								0, m->black, m->background);
	XSelectInput(m->dpy, m->win, ExposureMask   | StructureNotifyMask |
							   KeyReleaseMask   | PointerMotionMask |
								ButtonPressMask | ButtonReleaseMask );

	m->gc = XCreateGC(m->dpy, m->win, 0, NULL);
	XSetForeground(m->dpy, m->gc, m->black);
	XSetBackground(m->dpy, m->gc, m->background);
}

static void X11MessageBox_Free(struct X11MessageBox* m) {
	XFreeGC(m->dpy, m->gc);
	XDestroyWindow(m->dpy, m->win);
}

struct X11Textbox {
	int x, y, width, height;
	int lineHeight, descent;
	const char* text;
};

static void X11Textbox_Measure(struct X11Textbox* t, XFontStruct* font) {
	cc_string str = String_FromReadonly(t->text), line;
	XCharStruct overall;
	int direction, ascent, descent, lines = 0;

	for (; str.length; lines++) {
		String_UNSAFE_SplitBy(&str, '\n', &line);
		XTextExtents(font, line.buffer, line.length, &direction, &ascent, &descent, &overall);
		t->width = max(overall.width, t->width);
	}

	t->lineHeight = ascent + descent;
	t->descent    = descent;
	t->height     = t->lineHeight * lines;
}

static void X11Textbox_Draw(struct X11Textbox* t, struct X11MessageBox* m) {
	cc_string str = String_FromReadonly(t->text), line;
	int y = t->y + t->lineHeight - t->descent; /* TODO: is -descent even right? */

	for (; str.length; y += t->lineHeight) {
		String_UNSAFE_SplitBy(&str, '\n', &line);
		XDrawString(m->dpy, m->win, m->gc, t->x, y, line.buffer, line.length);		
	}
}

struct X11Button {
	int x, y, width, height;
	cc_bool clicked;
	struct X11Textbox text;
};

static void X11Button_Draw(struct X11Button* b, struct X11MessageBox* m) {
	struct X11Textbox* t;
	int begX, endX, begY, endY;

	XSetForeground(m->dpy, m->gc,  m->btnBorder);
	XDrawRectangle(m->dpy, m->win, m->gc, b->x, b->y,
					b->width, b->height);

	t = &b->text;
	begX = b->x + 1; endX = b->x + b->width  - 1;
	begY = b->y + 1; endY = b->y + b->height - 1;

	if (b->clicked) {
		XSetForeground(m->dpy, m->gc,  m->highlight);
		XDrawRectangle(m->dpy, m->win, m->gc, begX, begY,
						endX - begX, endY - begY);
	} else {
		XSetForeground(m->dpy, m->gc, m->white);
		XDrawLine(m->dpy, m->win, m->gc, begX, begY,
					endX - 1, begY);
		XDrawLine(m->dpy, m->win, m->gc, begX, begY,
					begX, endY - 1);

		XSetForeground(m->dpy, m->gc, m->highlight);
		XDrawLine(m->dpy, m->win, m->gc, begX + 1, endY - 1,
					endX - 1, endY - 1);
		XDrawLine(m->dpy, m->win, m->gc, endX - 1, begY + 1,
					endX - 1, endY - 1);

		XSetForeground(m->dpy, m->gc, m->shadow);
		XDrawLine(m->dpy, m->win, m->gc, begX, endY, endX, endY);
		XDrawLine(m->dpy, m->win, m->gc, endX, begY, endX, endY);
	}

	XSetForeground(m->dpy, m->gc, m->black);
	t->x = b->x + b->clicked + (b->width  - t->width)  / 2;
	t->y = b->y + b->clicked + (b->height - t->height) / 2;
	X11Textbox_Draw(t, m);
}

static int X11Button_Contains(struct X11Button* b, int x, int y) {
	return x >= b->x && x < (b->x + b->width) &&
		   y >= b->y && y < (b->y + b->height);
}

static Bool X11_FilterEvent(Display* d, XEvent* e, XPointer w) { return e->xany.window == (Window)w; }
static void X11_MessageBox(const char* title, const char* text, struct X11MessageBox* m) {
	struct X11Button ok    = { 0 };
	struct X11Textbox body = { 0 };

	Atom protocols[2];
	XFontStruct* font;
	int x, y, width, height;
	XSizeHints hints = { 0 };
	int mouseX = -1, mouseY = -1, over;
	XEvent e;

	X11MessageBox_Init(m);
	XMapWindow(m->dpy, m->win);
	XStoreName(m->dpy, m->win, title);

	protocols[0] = XInternAtom(m->dpy, "WM_DELETE_WINDOW", false);
	protocols[1] = XInternAtom(m->dpy, "_NET_WM_PING",     false);
	XSetWMProtocols(m->dpy, m->win, protocols, 2);

	font = XQueryFont(m->dpy, XGContextFromGC(m->gc));
	if (!font) return;

	/* Compute size of widgets */
	body.text = text;
	X11Textbox_Measure(&body, font);
	ok.text.text = "OK";
	X11Textbox_Measure(&ok.text, font);
	ok.width  = ok.text.width  + 70;
	ok.height = ok.text.height + 10;

	/* Compute size and position of window */
	width  = body.width                   + 20;
	height = body.height + 20 + ok.height + 20;
	x = DisplayWidth (m->dpy, DefaultScreen(m->dpy))/2 -  width/2;
	y = DisplayHeight(m->dpy, DefaultScreen(m->dpy))/2 - height/2;
	XMoveResizeWindow(m->dpy, m->win, x, y, width, height);

	/* Adjust bounds of widgets */
	body.x = 10; body.y = 10;
	ok.x = width/2 - ok.width/2;
	ok.y = height  - ok.height - 10;

	/* This marks the window as popup window of the main window */
	/* http://tronche.com/gui/x/icccm/sec-4.html#WM_TRANSIENT_FOR */
	/* Depending on WM, removes minimise and doesn't show in taskbar */
	if (win_handle) XSetTransientForHint(m->dpy, m->win, win_handle);

	XFreeFontInfo(NULL, font, 1);
	XUnmapWindow(m->dpy, m->win); /* Make window non resizeable */

	hints.flags      = PSize | PMinSize | PMaxSize;
	hints.min_width  = hints.max_width  = hints.base_width  = width;
	hints.min_height = hints.max_height = hints.base_height = height;

	XSetWMNormalHints(m->dpy, m->win, &hints);
	XMapRaised(m->dpy, m->win);
	XFlush(m->dpy);

    for (;;) {
		/* The naive solution is to use XNextEvent(m->dpy, &e) here. */
		/* However this causes issues as that removes events that */
		/* should have been delivered to the main game window. */
		/* (e.g. breaks initial window resize with i3 WM) */
		XIfEvent(m->dpy, &e, X11_FilterEvent, (XPointer)m->win);

		switch (e.type)
		{
		case ButtonPress:
		case ButtonRelease:
			if (e.xbutton.button != Button1) break;
			over = X11Button_Contains(&ok, mouseX, mouseY);

			if (ok.clicked && e.type == ButtonRelease) {
				if (over) return;
			}
			ok.clicked = e.type == ButtonPress && over;
			/* fallthrough to redraw window */

		case Expose:
		case MapNotify:
			XClearWindow(m->dpy, m->win);
			X11Textbox_Draw(&body, m);
			X11Button_Draw(&ok, m);
			XFlush(m->dpy);
			break;

		case KeyRelease:
			if (XLookupKeysym(&e.xkey, 0) == XK_Escape) return;
			break;

		case ClientMessage:
			/* { WM_DELETE_WINDOW, _NET_WM_PING } */
			if (e.xclient.data.l[0] == protocols[0]) return;
			if (e.xclient.data.l[0] == protocols[1]) HandleWMPing(&e);
			break;

		case MotionNotify:
			mouseX = e.xmotion.x; mouseY = e.xmotion.y;
			break;
		}
	}
}

static void ShowDialogCore(const char* title, const char* msg) {
	struct X11MessageBox m = { 0 };
	m.dpy = win_display;
	
	/* Failing to create a display means can't display a message box. */
	/* However the user might have launched the game through terminal, */
	/* so fallback to console instead of just dying from a segfault */
	if (!m.dpy) {
		Platform_LogConst("### MESSAGE ###");
		Platform_LogConst(title);
		Platform_LogConst(msg);
		return;
	}
	
	X11_MessageBox(title, msg, &m);
	X11MessageBox_Free(&m);
	XFlush(m.dpy); /* flush so window disappears immediately */
}

static GC fb_gc;
static XImage* fb_image;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	if (!fb_gc) fb_gc = XCreateGC(win_display, win_handle, 0, NULL);

	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	fb_image   = XCreateImage(win_display, win_visual.visual,
		win_visual.depth, ZPixmap, 0, (char*)bmp->scan0,
		bmp->width, bmp->height, 32, 0);
}

void Window_DrawFramebuffer(Rect2D r) {
	XPutImage(win_display, win_handle, fb_gc, fb_image,
		r.X, r.Y, r.X, r.Y, r.Width, r.Height);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	XFree(fb_image);
	Mem_Free(bmp->scan0);
}

void Window_OpenKeyboard(const struct OpenKeyboardArgs* args) { }
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { }

static cc_bool rawMouseInited, rawMouseSupported;
static int xiOpcode;

static void CheckMovementDelta(double dx, double dy) {
	/* Despite the assumption that XI_RawMotion is relative,     */
	/*  unfortunately there's a few buggy corner cases out there */
	/*  where absolute coordinates are provided instead.         */
	/* The ugly code belows tries to detect these corner cases,  */
	/*  and disables XInput2 when that happens                   */
	static int valid, fails;

	if (valid) return;
	/* The default window resolution is 854 x 480, so if there's */
	/*  a delta less than half of that, then it's almost certain */
	/*  that the provided coordinates are relative.*/
	if (dx < 300 || dy < 200) { valid = true; return; }

	if (fails++ < 20) return;
	/* Checked over 20 times now, but no relative coordinates,   */
	/*  so give up trying to use XInput2 anymore.                */
	Platform_LogConst("Buggy XInput2 detected, disabling it.."); 
	rawMouseSupported = false;
}

static void HandleGenericEvent(XEvent* e) {
	const double* values;
	XIRawEvent* ev;
	double dx, dy;

	if (!rawMouseSupported || e->xcookie.extension != xiOpcode) return;
	if (!XGetEventData(win_display, &e->xcookie)) return;

	if (e->xcookie.evtype == XI_RawMotion && Input_RawMode) {
		ev     = (XIRawEvent*)e->xcookie.data;
		values = ev->raw_values;

		/* Raw motion events may not always have values for both axes */
		dx = XIMaskIsSet(ev->valuators.mask, 0) ? *values++ : 0;
		dy = XIMaskIsSet(ev->valuators.mask, 1) ? *values++ : 0;

		CheckMovementDelta(dx, dy);
		/* Using 0.5f here makes the sensitivity about same as normal cursor movement */
		Event_RaiseRawMove(&PointerEvents.RawMoved, dx * 0.5f, dy * 0.5f);
	}
	XFreeEventData(win_display, &e->xcookie);
}

static void InitRawMouse(void) {
	XIEventMask evmask;
	unsigned char masks[XIMaskLen(XI_LASTEVENT)] = { 0 };
	int ev, err, major, minor;

	if (!XQueryExtension(win_display, "XInputExtension", &xiOpcode, &ev, &err)) {
		Platform_LogConst("XInput unsupported");
		return;
	}

	/* Only XInput 2.0 is actually required. However, 2.0 has the annoying */
	/* behaviour where raw input is NOT delivered while pointer is grabbed. */
	/* (i.e. if you press mouse button, no more raw mouse movement events) */
	/* http://wine.1045685.n8.nabble.com/PATCH-0-9-Implement-DInput8-mouse-using-RawInput-and-XInput2-RawEvents-only-td6016923.html */
	/* Thankfully XInput >= 2.1 corrects this behaviour */
	/* http://who-t.blogspot.com/2011/09/whats-new-in-xi-21-raw-events.html */
	major = 2; minor = 2;
	if (XIQueryVersion(win_display, &major, &minor) != Success) {
		Platform_Log2("Only XInput %i.%i supported", &major, &minor);
		return;
	}

	/* Sometimes XIQueryVersion will report Success, even though the */
	/* supported version is only 2.0! So make sure to handle that. */
	if (major < 2 || minor < 2) {
		Platform_Log2("Only XInput %i.%i supported", &major, &minor);
		return;
	}

	XISetMask(masks, XI_RawMotion);
	evmask.deviceid = XIAllMasterDevices;
	evmask.mask_len = sizeof(masks);
	evmask.mask     = masks;

	XISelectEvents(win_display, win_rootWin, &evmask, 1);
	rawMouseSupported = true;
}

void Window_EnableRawMouse(void) {
	DefaultEnableRawMouse();
	if (!rawMouseInited) InitRawMouse();
	rawMouseInited = true;

	if (!grabCursor) return;
	XGrabPointer(win_display, win_handle, True, ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
		GrabModeAsync, GrabModeAsync, win_handle, blankCursor, CurrentTime);
}

void Window_UpdateRawMouse(void) {
	if (rawMouseSupported) {
		/* Handled by XI_RawMotion generic event */
		CentreMousePosition();
	} else {
		DefaultUpdateRawMouse();
	}
}

void Window_DisableRawMouse(void) {
	DefaultDisableRawMouse();
	if (!grabCursor) return;
	XUngrabPointer(win_display, CurrentTime);
}


/*########################################################################################################################*
*---------------------------------------------------Carbon/Cocoa window---------------------------------------------------*
*#########################################################################################################################*/
#elif defined CC_BUILD_CARBON || defined CC_BUILD_COCOA
#include <ApplicationServices/ApplicationServices.h>
static int windowX, windowY;

static void Window_CommonInit(void) {
	CGDirectDisplayID display = CGMainDisplayID();
	CGRect bounds = CGDisplayBounds(display);

	DisplayInfo.X      = (int)bounds.origin.x;
	DisplayInfo.Y      = (int)bounds.origin.y;
	DisplayInfo.Width  = (int)bounds.size.width;
	DisplayInfo.Height = (int)bounds.size.height;
	DisplayInfo.Depth  = CGDisplayBitsPerPixel(display);
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
}

static pascal OSErr HandleQuitMessage(const AppleEvent* ev, AppleEvent* reply, long handlerRefcon) {
	Window_Close();
	return 0;
}

static void Window_CommonCreate(void) {
	WindowInfo.Exists = true;
	/* for quit buttons in dock and menubar */
	AEInstallEventHandler(kCoreEventClass, kAEQuitApplication,
		NewAEEventHandlerUPP(HandleQuitMessage), 0, false);
}

/* Sourced from https://www.meandmark.com/keycodes.html */
static const cc_uint8 key_map[8 * 16] = {
	'A', 'S', 'D', 'F', 'H', 'G', 'Z', 'X', 'C', 'V', 0, 'B', 'Q', 'W', 'E', 'R',
	'Y', 'T', '1', '2', '3', '4', '6', '5', KEY_EQUALS, '9', '7', KEY_MINUS, '8', '0', KEY_RBRACKET, 'O',
	'U', KEY_LBRACKET, 'I', 'P', KEY_ENTER, 'L', 'J', KEY_QUOTE, 'K', KEY_SEMICOLON, KEY_BACKSLASH, KEY_COMMA, KEY_SLASH, 'N', 'M', KEY_PERIOD,
	KEY_TAB, KEY_SPACE, KEY_TILDE, KEY_BACKSPACE, 0, KEY_ESCAPE, 0, 0, 0, KEY_CAPSLOCK, 0, 0, 0, 0, 0, 0,
	0, KEY_KP_DECIMAL, 0, KEY_KP_MULTIPLY, 0, KEY_KP_PLUS, 0, KEY_NUMLOCK, 0, 0, 0, KEY_KP_DIVIDE, KEY_KP_ENTER, 0, KEY_KP_MINUS, 0,
	0, KEY_KP_ENTER, KEY_KP0, KEY_KP1, KEY_KP2, KEY_KP3, KEY_KP4, KEY_KP5, KEY_KP6, KEY_KP7, 0, KEY_KP8, KEY_KP9, 'N', 'M', KEY_PERIOD,
	KEY_F5, KEY_F6, KEY_F7, KEY_F3, KEY_F8, KEY_F9, 0, KEY_F11, 0, KEY_F13, 0, KEY_F14, 0, KEY_F10, 0, KEY_F12,
	'U', KEY_F15, KEY_INSERT, KEY_HOME, KEY_PAGEUP, KEY_DELETE, KEY_F4, KEY_END, KEY_F2, KEY_PAGEDOWN, KEY_F1, KEY_LEFT, KEY_RIGHT, KEY_DOWN, KEY_UP, 0,
};
static int MapNativeKey(UInt32 key) { return key < Array_Elems(key_map) ? key_map[key] : 0; }
/* TODO: Check these.. */
/*   case 0x37: return KEY_LWIN; */
/*   case 0x38: return KEY_LSHIFT; */
/*   case 0x3A: return KEY_LALT; */
/*   case 0x3B: return Key_ControlLeft; */

/* TODO: Verify these differences from OpenTK */
/*Backspace = 51,  (0x33, KEY_DELETE according to that link)*/
/*Return = 52,     (0x34, ??? according to that link)*/
/*Menu = 110,      (0x6E, ??? according to that link)*/

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

void Clipboard_GetText(cc_string* value) {
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
		if (ptr) String_AppendUtf16(value, ptr, len);
	} else if (!(err = PasteboardCopyItemFlavorData(pbRef, itemID, FMT_UTF8, &outData))) {
		ptr = CFDataGetBytePtr(outData);
		len = CFDataGetLength(outData);
		if (ptr) String_AppendUtf8(value, ptr, len);
	}
}

void Clipboard_SetText(const cc_string* value) {
	PasteboardRef pbRef;
	CFDataRef cfData;
	UInt8 str[800];
	int len;
	OSStatus err;

	pbRef = Window_GetPasteboard();
	err   = PasteboardClear(pbRef);
	if (err) Logger_Abort2(err, "Clearing Pasteboard");
	PasteboardSynchronize(pbRef);

	len    = Platform_EncodeUtf8(str, value);
	cfData = CFDataCreate(NULL, str, len);
	if (!cfData) Logger_Abort("CFDataCreate() returned null pointer");

	PasteboardPutItemFlavor(pbRef, 1, FMT_UTF8, cfData, 0);
}

void Cursor_SetPosition(int x, int y) {
	CGPoint point;
	point.x = x + windowX;
	point.y = y + windowY;
	CGDisplayMoveCursorToPoint(CGMainDisplayID(), point);
}

static void Cursor_DoSetVisible(cc_bool visible) {
	if (visible) {
		CGDisplayShowCursor(CGMainDisplayID());
	} else {
		CGDisplayHideCursor(CGMainDisplayID());
	}
}

void Window_OpenKeyboard(const struct OpenKeyboardArgs* args) { }
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { }

void Window_EnableRawMouse(void) {
	DefaultEnableRawMouse();
	CGAssociateMouseAndMouseCursorPosition(0);
}

void Window_UpdateRawMouse(void) { CentreMousePosition(); }
void Window_DisableRawMouse(void) {
	CGAssociateMouseAndMouseCursorPosition(1);
	DefaultDisableRawMouse();
}


/*########################################################################################################################*
*------------------------------------------------------Carbon window------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_CARBON
#include <Carbon/Carbon.h>

static WindowRef win_handle;
static cc_bool win_fullscreen, showingDialog;

/* fullscreen is tied to OpenGL context unfortunately */
static cc_result GLContext_UnsetFullscreen(void);
static cc_result GLContext_SetFullscreen(void);

static void RefreshWindowBounds(void) {
	Rect r;
	if (win_fullscreen) return;
	
	/* TODO: kWindowContentRgn ??? */
	GetWindowBounds(win_handle, kWindowGlobalPortRgn, &r);
	windowX = r.left; WindowInfo.Width  = r.right  - r.left;
	windowY = r.top;  WindowInfo.Height = r.bottom - r.top;
}

static OSStatus Window_ProcessKeyboardEvent(EventRef inEvent) {
	UInt32 kind, code;
	int key;
	OSStatus res;
	
	kind = GetEventKind(inEvent);
	switch (kind) {
		case kEventRawKeyDown:
		case kEventRawKeyRepeat:
		case kEventRawKeyUp:
			res = GetEventParameter(inEvent, kEventParamKeyCode, typeUInt32,
									NULL, sizeof(UInt32), NULL, &code);
			if (res) Logger_Abort2(res, "Getting key button");

			key = MapNativeKey(code);
			if (key) {
				Input_Set(key, kind != kEventRawKeyUp);
			} else {
				Platform_Log1("Ignoring unknown key %i", &code);
			}
			return eventNotHandledErr;
			
		case kEventRawKeyModifiersChanged:
			res = GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, 
									NULL, sizeof(UInt32), NULL, &code);
			if (res) Logger_Abort2(res, "Getting key modifiers");
			
			Input_Set(KEY_LCTRL,    code & 0x1000);
			Input_Set(KEY_LALT,     code & 0x0800);
			Input_Set(KEY_LSHIFT,   code & 0x0200);
			Input_Set(KEY_LWIN,     code & 0x0100);			
			Input_Set(KEY_CAPSLOCK, code & 0x0400);
			return eventNotHandledErr;
	}
	return eventNotHandledErr;
}

static OSStatus Window_ProcessWindowEvent(EventRef inEvent) {
	int oldWidth, oldHeight;
	
	switch (GetEventKind(inEvent)) {
		case kEventWindowClose:
			WindowInfo.Exists = false;
			Event_RaiseVoid(&WindowEvents.Closing);
			return eventNotHandledErr;
			
		case kEventWindowClosed:
			WindowInfo.Exists = false;
			return 0;
			
		case kEventWindowBoundsChanged:
			oldWidth = WindowInfo.Width; oldHeight = WindowInfo.Height;
			RefreshWindowBounds();
			
			if (oldWidth != WindowInfo.Width || oldHeight != WindowInfo.Height) {
				Event_RaiseVoid(&WindowEvents.Resized);
			}
			return eventNotHandledErr;
			
		case kEventWindowActivated:
			WindowInfo.Focused = true;
			Event_RaiseVoid(&WindowEvents.FocusChanged);
			return eventNotHandledErr;
			
		case kEventWindowDeactivated:
			WindowInfo.Focused = false;
			Event_RaiseVoid(&WindowEvents.FocusChanged);
			return eventNotHandledErr;
			
		case kEventWindowDrawContent:
			Event_RaiseVoid(&WindowEvents.Redraw);
			return eventNotHandledErr;
	}
	return eventNotHandledErr;
}

static OSStatus Window_ProcessMouseEvent(EventRef inEvent) {
	int mouseX, mouseY;
	HIPoint pt, raw;
	UInt32 kind;
	cc_bool down;
	EventMouseButton button;
	SInt32 delta;
	OSStatus res;	
	
	res = GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint,
							NULL, sizeof(HIPoint), NULL, &pt);
	/* this error comes up from the application event handler */
	if (res && res != eventParameterNotFoundErr) {
		Logger_Abort2(res, "Getting mouse position");
	}

	if (Input_RawMode) {
		raw.x = 0; raw.y = 0;
		GetEventParameter(inEvent, kEventParamMouseDelta, typeHIPoint, NULL, sizeof(HIPoint), NULL, &raw);
		Event_RaiseRawMove(&PointerEvents.RawMoved, raw.x, raw.y);
	}
	
	if (showingDialog) return eventNotHandledErr;
	mouseX = (int)pt.x; mouseY = (int)pt.y;

	/* kEventParamMouseLocation is in screen coordinates */
	/* So need to make sure mouse click lies inside window */
	/* Otherwise this breaks close/minimise/maximise in titlebar and dock */
	if (!win_fullscreen) {
		mouseX -= windowX; mouseY -= windowY;

		if (mouseX < 0 || mouseX >= WindowInfo.Width)  return eventNotHandledErr;
		if (mouseY < 0 || mouseY >= WindowInfo.Height) return eventNotHandledErr;
	}
	
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
					Input_Set(KEY_LMOUSE, down); break;
				case kEventMouseButtonSecondary:
					Input_Set(KEY_RMOUSE, down); break;
				case kEventMouseButtonTertiary:
					Input_Set(KEY_MMOUSE, down); break;
			}		
			return eventNotHandledErr;
			
		case kEventMouseWheelMoved:
			res = GetEventParameter(inEvent, kEventParamMouseWheelDelta, typeSInt32,
									NULL, sizeof(SInt32), NULL, &delta);
			if (res) Logger_Abort2(res, "Getting mouse wheel delta");
			Mouse_ScrollWheel(delta);
			return 0;
			
		case kEventMouseMoved:
		case kEventMouseDragged:
			Pointer_SetPosition(0, mouseX, mouseY);
			return eventNotHandledErr;
	}
	return eventNotHandledErr;
}

static OSStatus Window_ProcessTextEvent(EventRef inEvent) {
	UInt32 kind;
	EventRef keyEvent = NULL;
	UniChar chars[17] = { 0 };
	char keyChar;
	int i;
	OSStatus res;
	
	kind = GetEventKind(inEvent);
	if (kind != kEventTextInputUnicodeForKeyEvent) return eventNotHandledErr;

	/* When command key is pressed, text input should be ignored */
	res = GetEventParameter(inEvent, kEventParamTextInputSendKeyboardEvent,
							typeEventRef, NULL, sizeof(keyEvent), NULL, &keyEvent);
	if (!res && keyEvent) {
		UInt32 modifiers = 0;
		GetEventParameter(keyEvent, kEventParamKeyModifiers,
							typeUInt32, NULL, sizeof(modifiers), NULL, &modifiers);
		if (modifiers & 0x0100) return eventNotHandledErr;
	}
	
	/* TODO: is the assumption we only get 1-4 characters always valid */
	res = GetEventParameter(inEvent, kEventParamTextInputSendText,
							typeUnicodeText, NULL, 16 * sizeof(UniChar), NULL, chars);
	if (res) Logger_Abort2(res, "Getting text chars");

	for (i = 0; i < 16 && chars[i]; i++) {
		/* TODO: UTF16 to codepoint conversion */
		if (Convert_TryCodepointToCP437(chars[i], &keyChar)) {
			Event_RaiseInt(&InputEvents.Press, keyChar);
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

typedef EventTargetRef (*GetMenuBarEventTarget_Func)(void);

static void HookEvents(void) {
	static const EventTypeSpec winEventTypes[] = {
		{ kEventClassKeyboard, kEventRawKeyDown },
		{ kEventClassKeyboard, kEventRawKeyRepeat },
		{ kEventClassKeyboard, kEventRawKeyUp },
		{ kEventClassKeyboard, kEventRawKeyModifiersChanged },
		
		{ kEventClassWindow, kEventWindowClose },
		{ kEventClassWindow, kEventWindowClosed },
		{ kEventClassWindow, kEventWindowBoundsChanged },
		{ kEventClassWindow, kEventWindowActivated },
		{ kEventClassWindow, kEventWindowDeactivated },
		{ kEventClassWindow, kEventWindowDrawContent },

		{ kEventClassTextInput,  kEventTextInputUnicodeForKeyEvent },
		{ kEventClassAppleEvent, kEventAppleEvent }
	};
	static const EventTypeSpec appEventTypes[] = {
		{ kEventClassMouse, kEventMouseDown },
		{ kEventClassMouse, kEventMouseUp },
		{ kEventClassMouse, kEventMouseMoved },
		{ kEventClassMouse, kEventMouseDragged },
		{ kEventClassMouse, kEventMouseWheelMoved }
	};
	GetMenuBarEventTarget_Func getMenuBarEventTarget;
	EventTargetRef target;
	
	target = GetWindowEventTarget(win_handle);
	InstallEventHandler(target, NewEventHandlerUPP(Window_EventHandler),
						Array_Elems(winEventTypes), winEventTypes, NULL, NULL);

	target = GetApplicationEventTarget();
	InstallEventHandler(target, NewEventHandlerUPP(Window_EventHandler),
						Array_Elems(appEventTypes), appEventTypes, NULL, NULL);

	/* The code below is to get the menubar working. */
	/* The documentation for 'RunApplicationEventLoop' states that it installs */
	/* the standard application event handler which lets the menubar work. */
	/* However, we cannot use that since the event loop is managed by us instead. */
	/* Unfortunately, there is no proper API to duplicate that behaviour, so rely */
	/* on the undocumented GetMenuBarEventTarget to achieve similar behaviour. */
	/* TODO: This is wrong for PowerPC. But at least it doesn't crash or anything. */
#define _RTLD_DEFAULT ((void*)-2)
	getMenuBarEventTarget = DynamicLib_Get2(_RTLD_DEFAULT, "GetMenuBarEventTarget");
	InstallStandardEventHandler(GetApplicationEventTarget());

	/* TODO: Why does this not work properly until user activates another window */
	/* Followup: Seems to be a bug in OSX http://www.sheepsystems.com/developers_blog/transformprocesstype--bette.html */
	if (getMenuBarEventTarget) {
		InstallStandardEventHandler(getMenuBarEventTarget());
	} else {
		Platform_LogConst("MenuBar won't work!");
	}
	/* MenuRef menu = AcquireRootMenu(); */
	/* InstallStandardEventHandler(GetMenuEventTarget(menu)); */
}


/*########################################################################################################################*
 *--------------------------------------------------Public implementation--------------------------------------------------*
 *#########################################################################################################################*/
void Window_Init(void) { Window_CommonInit(); }

/* Private CGS/CGL stuff */
typedef int CGSConnectionID;
typedef int CGSWindowID;
extern CGSConnectionID _CGSDefaultConnection(void);
extern CGSWindowID GetNativeWindowFromWindowRef(WindowRef window);
extern CGContextRef CGWindowContextCreate(CGSConnectionID conn, CGSWindowID win, void* opts);

static CGSConnectionID conn;
static CGSWindowID winId;

#ifdef CC_BUILD_ICON
extern const int CCIcon_Data[];
extern const int CCIcon_Width, CCIcon_Height;

static void ApplyIcon(void) {
	CGColorSpaceRef colSpace;
	CGDataProviderRef provider;
	CGImageRef image;

	colSpace = CGColorSpaceCreateDeviceRGB();
	provider = CGDataProviderCreateWithData(NULL, CCIcon_Data,
					Bitmap_DataSize(CCIcon_Width, CCIcon_Height), NULL);
	image    = CGImageCreate(CCIcon_Width, CCIcon_Height, 8, 32, CCIcon_Width * 4, colSpace,
					kCGBitmapByteOrder32Little | kCGImageAlphaLast, provider, NULL, 0, 0);

	SetApplicationDockTileImage(image);
	CGImageRelease(image);
	CGDataProviderRelease(provider);
	CGColorSpaceRelease(colSpace);
}
#else
static void ApplyIcon(void) { }
#endif

void Window_Create(int width, int height) {
	Rect r;
	OSStatus res;
	ProcessSerialNumber psn;
	
	r.left = Display_CentreX(width);  r.right  = r.left + width; 
	r.top  = Display_CentreY(height); r.bottom = r.top  + height;
	res = CreateNewWindow(kDocumentWindowClass,
						  kWindowStandardDocumentAttributes | kWindowStandardHandlerAttribute |
						  kWindowInWindowMenuAttribute | kWindowLiveResizeAttribute, &r, &win_handle);

	if (res) Logger_Abort2(res, "Creating window failed");
	RefreshWindowBounds();

	AcquireRootMenu();	
	GetCurrentProcess(&psn);
	SetFrontProcess(&psn);
	
	/* TODO: Use BringWindowToFront instead.. (look in the file which has RepositionWindow in it) !!!! */
	HookEvents();
	Window_CommonCreate();
	WindowInfo.Handle = win_handle;

	conn  = _CGSDefaultConnection();
	winId = GetNativeWindowFromWindowRef(win_handle);
	ApplyIcon();
}

void Window_SetTitle(const cc_string* title) {
	UInt8 str[NATIVE_STR_LEN];
	CFStringRef titleCF;
	int len;
	
	/* TODO: This leaks memory, old title isn't released */
	len     = Platform_EncodeUtf8(str, title);
	titleCF = CFStringCreateWithBytes(kCFAllocatorDefault, str, len, kCFStringEncodingUTF8, false);
	SetWindowTitleWithCFString(win_handle, titleCF);
}

void Window_Show(void) {
	ShowWindow(win_handle);
	/* TODO: Do we actually need to reposition */
	RepositionWindow(win_handle, NULL, kWindowCenterOnMainScreen);
	SelectWindow(win_handle);
}

int Window_GetWindowState(void) {
	if (win_fullscreen)                return WINDOW_STATE_FULLSCREEN;
	if (IsWindowCollapsed(win_handle)) return WINDOW_STATE_MINIMISED;
	return WINDOW_STATE_NORMAL;
}

static void UpdateWindowState(void) {
	Event_RaiseVoid(&WindowEvents.StateChanged);
	RefreshWindowBounds();
	Event_RaiseVoid(&WindowEvents.Resized);
}

cc_result Window_EnterFullscreen(void) {
	cc_result res = GLContext_SetFullscreen();
	UpdateWindowState();
	return res;
}
cc_result Window_ExitFullscreen(void) {
	cc_result res = GLContext_UnsetFullscreen();
	UpdateWindowState();
	return res;
}

void Window_SetSize(int width, int height) {
	SizeWindow(win_handle, width, height, true);
}

void Window_Close(void) {
	/* DisposeWindow only sends a kEventWindowClosed */
	Event_RaiseVoid(&WindowEvents.Closing);
	if (WindowInfo.Exists) DisposeWindow(win_handle);
	WindowInfo.Exists = false;
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

static void Cursor_GetRawPos(int* x, int* y) {
	Point point;
	GetGlobalMouse(&point);
	*x = (int)point.h; *y = (int)point.v;
}

static void ShowDialogCore(const char* title, const char* msg) {
	CFStringRef titleCF = CFStringCreateWithCString(NULL, title, kCFStringEncodingASCII);
	CFStringRef msgCF   = CFStringCreateWithCString(NULL, msg,   kCFStringEncodingASCII);
	DialogRef dialog;
	DialogItemIndex itemHit;

	showingDialog = true;
	CreateStandardAlert(kAlertPlainAlert, titleCF, msgCF, NULL, &dialog);
	CFRelease(titleCF);
	CFRelease(msgCF);

	RunStandardAlert(dialog, NULL, &itemHit);
	showingDialog = false;
}

static CGrafPtr fb_port;
static struct Bitmap fb_bmp;
static CGColorSpaceRef colorSpace;

void Window_AllocFramebuffer(struct Bitmap* bmp) {
	if (!fb_port) fb_port = GetWindowPort(win_handle);

	bmp->scan0 = Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	colorSpace = CGColorSpaceCreateDeviceRGB();
	fb_bmp     = *bmp;
}

void Window_DrawFramebuffer(Rect2D r) {
	CGContextRef context = NULL;
	CGDataProviderRef provider;
	CGImageRef image;
	CGRect rect;
	OSStatus err;

	/* Unfortunately CGImageRef is immutable, so changing the */
	/* underlying data doesn't change what shows when drawing. */
	/* TODO: Use QuickDraw alternative instead */

	/* TODO: Only update changed bit.. */
	rect.origin.x = 0; rect.origin.y = 0;
	rect.size.width  = WindowInfo.Width;
	rect.size.height = WindowInfo.Height;

	err = QDBeginCGContext(fb_port, &context);
	if (err) Logger_Abort2(err, "Begin draw");
	/* TODO: REPLACE THIS AWFUL HACK */

	provider = CGDataProviderCreateWithData(NULL, fb_bmp.scan0,
		Bitmap_DataSize(fb_bmp.width, fb_bmp.height), NULL);
#ifdef CC_BIG_ENDIAN
	image    = CGImageCreate(fb_bmp.width, fb_bmp.height, 8, 32, fb_bmp.width * 4, colorSpace,
				kCGImageAlphaNoneSkipFirst, provider, NULL, 0, 0);
#else
	image    = CGImageCreate(fb_bmp.width, fb_bmp.height, 8, 32, fb_bmp.width * 4, colorSpace,
				kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst, provider, NULL, 0, 0);
#endif

	CGContextDrawImage(context, rect, image);
	CGContextSynchronize(context);
	err = QDEndCGContext(fb_port, &context);
	if (err) Logger_Abort2(err, "End draw");

	CGImageRelease(image);
	CGDataProviderRelease(provider);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
	CGColorSpaceRelease(colorSpace);
}


/*########################################################################################################################*
*-------------------------------------------------------Cocoa window------------------------------------------------------*
*#########################################################################################################################*/
#elif defined CC_BUILD_COCOA
#include <objc/message.h>
#include <objc/runtime.h>
static id appHandle, winHandle, viewHandle;
extern void* NSDefaultRunLoopMode;

static SEL selFrame, selDeltaX, selDeltaY;
static SEL selNextEvent, selType, selSendEvent;
static SEL selButton, selKeycode, selModifiers;
static SEL selCharacters, selUtf8String, selMouseLoc;
static SEL selCurrentContext, selGraphicsPort;
static SEL selSetNeedsDisplay, selDisplayIfNeeded;
static SEL selUpdate, selFlushBuffer;

static void RegisterSelectors(void) {
	selFrame  = sel_registerName("frame");
	selDeltaX = sel_registerName("deltaX");
	selDeltaY = sel_registerName("deltaY");

	selNextEvent = sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:");
	selType      = sel_registerName("type");
	selSendEvent = sel_registerName("sendEvent:");

	selButton    = sel_registerName("buttonNumber");
	selKeycode   = sel_registerName("keyCode");
	selModifiers = sel_registerName("modifierFlags");

	selCharacters = sel_registerName("characters");
	selUtf8String = sel_registerName("UTF8String");
	selMouseLoc   = sel_registerName("mouseLocation");

	selCurrentContext  = sel_registerName("currentContext");
	selGraphicsPort    = sel_registerName("graphicsPort");
	selSetNeedsDisplay = sel_registerName("setNeedsDisplayInRect:");
	selDisplayIfNeeded = sel_registerName("displayIfNeeded");

	selUpdate      = sel_registerName("update");
	selFlushBuffer = sel_registerName("flushBuffer");
}

static CC_INLINE CGFloat Send_CGFloat(id receiver, SEL sel) {
	/* Sometimes we have to use fpret and sometimes we don't. See this for more details: */
	/* http://www.sealiesoftware.com/blog/archive/2008/11/16/objc_explain_objc_msgSend_fpret.html */
	/* return type is void*, but we cannot cast a void* to a float or double */

#ifdef __i386__
	return ((CGFloat(*)(id, SEL))(void *)objc_msgSend_fpret)(receiver, sel);
#else
	return ((CGFloat(*)(id, SEL))(void *)objc_msgSend)(receiver, sel);
#endif
}

static CC_INLINE CGPoint Send_CGPoint(id receiver, SEL sel) {
	/* on x86 and x86_64 CGPoint fits the requirements for 'struct returned in registers' */
	return ((CGPoint(*)(id, SEL))(void *)objc_msgSend)(receiver, sel);
}

static void RefreshWindowBounds(void) {
	CGRect win, view;
	int viewY;

	win  = ((CGRect(*)(id, SEL))(void *)objc_msgSend_stret)(winHandle,  selFrame);
	view = ((CGRect(*)(id, SEL))(void *)objc_msgSend_stret)(viewHandle, selFrame);

	/* For cocoa, the 0,0 origin is the bottom left corner of windows/views/screen. */
	/* To get window's real Y screen position, first need to find Y of top. (win.y + win.height) */
	/* Then just subtract from screen height to make relative to top instead of bottom of the screen. */
	/* Of course this is only half the story, since we're really after Y position of the content. */
	/* To work out top Y of view relative to window, it's just win.height - (view.y + view.height) */
	viewY   = (int)win.size.height  - ((int)view.origin.y + (int)view.size.height);
	windowX = (int)win.origin.x     + (int)view.origin.x;
	windowY = DisplayInfo.Height - ((int)win.origin.y  + (int)win.size.height) + viewY;

	WindowInfo.Width  = (int)view.size.width;
	WindowInfo.Height = (int)view.size.height;
}

static void OnDidResize(id self, SEL cmd, id notification) {
	RefreshWindowBounds();
	Event_RaiseVoid(&WindowEvents.Resized);
}

static void OnDidMove(id self, SEL cmd, id notification) {
	RefreshWindowBounds();
	GLContext_Update();
}

static void OnDidBecomeKey(id self, SEL cmd, id notification) {
	WindowInfo.Focused = true;
	Event_RaiseVoid(&WindowEvents.FocusChanged);
}

static void OnDidResignKey(id self, SEL cmd, id notification) {
	WindowInfo.Focused = false;
	Event_RaiseVoid(&WindowEvents.FocusChanged);
}

static void OnDidMiniaturize(id self, SEL cmd, id notification) {
	Event_RaiseVoid(&WindowEvents.StateChanged);
}

static void OnDidDeminiaturize(id self, SEL cmd, id notification) {
	Event_RaiseVoid(&WindowEvents.StateChanged);
}

static void OnWillClose(id self, SEL cmd, id notification) {
	WindowInfo.Exists = false;
	Event_RaiseVoid(&WindowEvents.Closing);
}

/* If this isn't overriden, an annoying beep sound plays anytime a key is pressed */
static void OnKeyDown(id self, SEL cmd, id ev) { }

static Class Window_MakeClass(void) {
	Class c = objc_allocateClassPair(objc_getClass("NSWindow"), "ClassiCube_Window", 0);

	class_addMethod(c, sel_registerName("windowDidResize:"),        OnDidResize,        "v@:@");
	class_addMethod(c, sel_registerName("windowDidMove:"),          OnDidMove,          "v@:@");
	class_addMethod(c, sel_registerName("windowDidBecomeKey:"),     OnDidBecomeKey,     "v@:@");
	class_addMethod(c, sel_registerName("windowDidResignKey:"),     OnDidResignKey,     "v@:@");
	class_addMethod(c, sel_registerName("windowDidMiniaturize:"),   OnDidMiniaturize,   "v@:@");
	class_addMethod(c, sel_registerName("windowDidDeminiaturize:"), OnDidDeminiaturize, "v@:@");
	class_addMethod(c, sel_registerName("windowWillClose:"),        OnWillClose,        "v@:@");
	class_addMethod(c, sel_registerName("keyDown:"),                OnKeyDown,          "v@:@");

	objc_registerClassPair(c);
	return c;
}

/* When the user users left mouse to drag reisze window, this enters 'live resize' mode */
/*   Although the game receives a left mouse down event, it does NOT receive a left mouse up */
/*   This causes the game to get stuck with left mouse down after user finishes resizing */
/* So work arond that by always releasing left mouse when a live resize is finished */
static void DidEndLiveResize(id self, SEL cmd) {
	Input_SetReleased(KEY_LMOUSE);
}

static void View_DrawRect(id self, SEL cmd, CGRect r);
static void MakeContentView(void) {
	CGRect rect;
	id view;
	Class c;

	view = objc_msgSend(winHandle, sel_registerName("contentView"));
	rect = ((CGRect(*)(id, SEL))(void *)objc_msgSend_stret)(view, selFrame);
	
	c = objc_allocateClassPair(objc_getClass("NSView"), "ClassiCube_View", 0);
	// TODO: test rect is actually correct in View_DrawRect on both 32 and 64 bit
#ifdef __i386__
	class_addMethod(c, sel_registerName("drawRect:"), View_DrawRect, "v@:{NSRect={NSPoint=ff}{NSSize=ff}}");
#else
	class_addMethod(c, sel_registerName("drawRect:"), View_DrawRect, "v@:{NSRect={NSPoint=dd}{NSSize=dd}}");
#endif
	class_addMethod(c, sel_registerName("viewDidEndLiveResize"), DidEndLiveResize, "v@:");
	objc_registerClassPair(c);

	viewHandle = objc_msgSend(c, sel_registerName("alloc"));
	objc_msgSend(viewHandle, sel_registerName("initWithFrame:"),  rect);
	objc_msgSend(winHandle,  sel_registerName("setContentView:"), viewHandle);
}

void Window_Init(void) {
	appHandle = objc_msgSend((id)objc_getClass("NSApplication"), sel_registerName("sharedApplication"));
	objc_msgSend(appHandle, sel_registerName("activateIgnoringOtherApps:"), true);
	Window_CommonInit();
	RegisterSelectors();
}

#ifdef CC_BUILD_ICON
extern const int CCIcon_Data[];
extern const int CCIcon_Width, CCIcon_Height;

static void ApplyIcon(void) {
	CGColorSpaceRef colSpace;
	CGDataProviderRef provider;
	CGImageRef image;
	CGSize size;
	void* img;

	colSpace = CGColorSpaceCreateDeviceRGB();
	provider = CGDataProviderCreateWithData(NULL, CCIcon_Data,
					Bitmap_DataSize(CCIcon_Width, CCIcon_Height), NULL);
	image    = CGImageCreate(CCIcon_Width, CCIcon_Height, 8, 32, CCIcon_Width * 4, colSpace,
					kCGBitmapByteOrder32Little | kCGImageAlphaLast, provider, NULL, 0, 0);

	size.width = 0; size.height = 0;
	img = objc_msgSend((id)objc_getClass("NSImage"), sel_registerName("alloc"));
	objc_msgSend(img, sel_registerName("initWithCGImage:size:"), image, size);
	objc_msgSend(appHandle, sel_registerName("setApplicationIconImage:"), img);

	/* TODO need to release NSImage here */
	CGImageRelease(image);
	CGDataProviderRelease(provider);
	CGColorSpaceRelease(colSpace);
}
#else
static void ApplyIcon(void) { }
#endif

#define NSTitledWindowMask         (1 << 0)
#define NSClosableWindowMask       (1 << 1)
#define NSMiniaturizableWindowMask (1 << 2)
#define NSResizableWindowMask      (1 << 3)
#define NSFullScreenWindowMask     (1 << 14)

void Window_Create(int width, int height) {
	Class winClass;
	CGRect rect;

	/* Technically the coordinates for the origin are at bottom left corner */
	/* But since the window is in centre of the screen, don't need to care here */
	rect.origin.x    = Display_CentreX(width);  
	rect.origin.y    = Display_CentreY(height);
	rect.size.width  = width; 
	rect.size.height = height;

	winClass  = Window_MakeClass();
	winHandle = objc_msgSend(winClass, sel_registerName("alloc"));
	objc_msgSend(winHandle, sel_registerName("initWithContentRect:styleMask:backing:defer:"), rect, (NSTitledWindowMask | NSClosableWindowMask | NSResizableWindowMask | NSMiniaturizableWindowMask), 0, false);
	
	Window_CommonCreate();
	objc_msgSend(winHandle, sel_registerName("setDelegate:"), winHandle);
	RefreshWindowBounds();
	MakeContentView();
	ApplyIcon();
}

void Window_SetTitle(const cc_string* title) {
	UInt8 str[NATIVE_STR_LEN];
	CFStringRef titleCF;
	int len;

	/* TODO: This leaks memory, old title isn't released */
	len = Platform_EncodeUtf8(str, title);
	titleCF = CFStringCreateWithBytes(kCFAllocatorDefault, str, len, kCFStringEncodingUTF8, false);
	objc_msgSend(winHandle, sel_registerName("setTitle:"), titleCF);
}

void Window_Show(void) { 
	objc_msgSend(winHandle, sel_registerName("makeKeyAndOrderFront:"), appHandle);
	RefreshWindowBounds(); // TODO: even necessary?
}

int Window_GetWindowState(void) {
	int flags;

	flags = (int)objc_msgSend(winHandle, sel_registerName("styleMask"));
	if (flags & NSFullScreenWindowMask) return WINDOW_STATE_FULLSCREEN;
	     
	flags = (int)objc_msgSend(winHandle, sel_registerName("isMiniaturized"));
	return flags ? WINDOW_STATE_MINIMISED : WINDOW_STATE_NORMAL;
}

// TODO: Only works on 10.7+
cc_result Window_EnterFullscreen(void) {
	objc_msgSend(winHandle, sel_registerName("toggleFullScreen:"), appHandle);
	return 0;
}
cc_result Window_ExitFullscreen(void) {
	objc_msgSend(winHandle, sel_registerName("toggleFullScreen:"), appHandle);
	return 0;
}

void Window_SetSize(int width, int height) {
	/* Can't use setContentSize:, because that resizes from the bottom left corner. */
	CGRect rect = ((CGRect(*)(id, SEL))(void *)objc_msgSend_stret)(winHandle, selFrame);

	rect.origin.y    += WindowInfo.Height - height;
	rect.size.width  += width  - WindowInfo.Width;
	rect.size.height += height - WindowInfo.Height;
	objc_msgSend(winHandle, sel_registerName("setFrame:display:"), rect, true);
}

void Window_Close(void) { 
	objc_msgSend(winHandle, sel_registerName("close"));
}

static int MapNativeMouse(int button) {
	if (button == 0) return KEY_LMOUSE;
	if (button == 1) return KEY_RMOUSE;
	if (button == 2) return KEY_MMOUSE;
	return 0;
}

static void ProcessKeyChars(id ev) {
	char buffer[128];
	const char* src;
	cc_string str;
	id chars;
	int i, len, flags;

	/* Ignore text input while cmd is held down */
	/* e.g. so Cmd + V to paste doesn't leave behind 'v' */
	flags = (int)objc_msgSend(ev, selModifiers);
	if (flags & 0x000008) return;
	if (flags & 0x000010) return;

	chars = objc_msgSend(ev,    selCharacters);
	src   = objc_msgSend(chars, selUtf8String);
	len   = String_Length(src);
	String_InitArray(str, buffer);

	String_AppendUtf8(&str, src, len);
	for (i = 0; i < str.length; i++) {
		Event_RaiseInt(&InputEvents.Press, str.buffer[i]);
	}
}

static cc_bool GetMouseCoords(int* x, int* y) {
	CGPoint loc = Send_CGPoint((id)objc_getClass("NSEvent"), selMouseLoc);
	*x = (int)loc.x                        - windowX;	
	*y = (DisplayInfo.Height - (int)loc.y) - windowY;
	// TODO: this seems to be off by 1
	return *x >= 0 && *y >= 0 && *x < WindowInfo.Width && *y < WindowInfo.Height;
}

static int TryGetKey(id ev) {
	int code = (int)objc_msgSend(ev, selKeycode);
	int key  = MapNativeKey(code);
	if (key) return key;

	Platform_Log1("Unknown key %i", &code);
	return 0;
}

void Window_ProcessEvents(void) {
	id ev;
	int key, type, steps, x, y;
	CGFloat dx, dy;

	for (;;) {
		ev = objc_msgSend(appHandle, selNextEvent, 0xFFFFFFFFU, NULL, NSDefaultRunLoopMode, true);
		if (!ev) break;
		type = (int)objc_msgSend(ev, selType);

		switch (type) {
		case  1: /* NSLeftMouseDown  */
		case  3: /* NSRightMouseDown */
		case 25: /* NSOtherMouseDown */
			key = MapNativeMouse((int)objc_msgSend(ev, selButton));
			if (GetMouseCoords(&x, &y) && key) Input_SetPressed(key);
			break;

		case  2: /* NSLeftMouseUp  */
		case  4: /* NSRightMouseUp */
		case 26: /* NSOtherMouseUp */
			key = MapNativeMouse((int)objc_msgSend(ev, selButton));
			if (key) Input_SetReleased(key);
			break;

		case 10: /* NSKeyDown */
			key = TryGetKey(ev);
			if (key) Input_SetPressed(key);
			// TODO: Test works properly with other languages
			ProcessKeyChars(ev);
			break;

		case 11: /* NSKeyUp */
			key = TryGetKey(ev);
			if (key) Input_SetReleased(key);
			break;

		case 12: /* NSFlagsChanged */
			key = (int)objc_msgSend(ev, selModifiers);
			/* TODO: Figure out how to only get modifiers that changed */
			Input_Set(KEY_LCTRL,    key & 0x000001);
			Input_Set(KEY_LSHIFT,   key & 0x000002);
			Input_Set(KEY_RSHIFT,   key & 0x000004);
			Input_Set(KEY_LWIN,     key & 0x000008);
			Input_Set(KEY_RWIN,     key & 0x000010);
			Input_Set(KEY_LALT,     key & 0x000020);
			Input_Set(KEY_RALT,     key & 0x000040);
			Input_Set(KEY_RCTRL,    key & 0x002000);
			Input_Set(KEY_CAPSLOCK, key & 0x010000);
			break;

		case 22: /* NSScrollWheel */
			dy    = Send_CGFloat(ev, selDeltaY);
			/* https://bugs.eclipse.org/bugs/show_bug.cgi?id=220175 */
			/* delta is in 'line height' units, but I don't know how to map that to actual units. */
			/* All I know is that scrolling by '1 wheel notch' produces a delta of around 0.1, and that */
			/* sometimes I'll see it go all the way up to 5-6 with a larger wheel scroll. */
			/* So mulitplying by 10 doesn't really seem a good idea, instead I just round outwards. */
			/* TODO: Figure out if there's a better way than this. */
			steps = dy > 0.0f ? Math_Ceil(dy) : Math_Floor(dy);
			Mouse_ScrollWheel(steps);
			break;

		case  5: /* NSMouseMoved */
		case  6: /* NSLeftMouseDragged */
		case  7: /* NSRightMouseDragged */
		case 27: /* NSOtherMouseDragged */
			if (GetMouseCoords(&x, &y)) Pointer_SetPosition(0, x, y);

			if (Input_RawMode) {
				dx = Send_CGFloat(ev, selDeltaX);
				dy = Send_CGFloat(ev, selDeltaY);
				Event_RaiseRawMove(&PointerEvents.RawMoved, dx, dy);
			}
			break;
		}
		objc_msgSend(appHandle, selSendEvent, ev);
	}
}

static void Cursor_GetRawPos(int* x, int* y) { *x = 0; *y = 0; }
static void ShowDialogCore(const char* title, const char* msg) {
	CFStringRef titleCF, msgCF;
	id alert;
	
	alert   = objc_msgSend((id)objc_getClass("NSAlert"), sel_registerName("alloc"));
	alert   = objc_msgSend(alert, sel_registerName("init"));
	titleCF = CFStringCreateWithCString(NULL, title, kCFStringEncodingASCII);
	msgCF   = CFStringCreateWithCString(NULL, msg,   kCFStringEncodingASCII);
	
	objc_msgSend(alert, sel_registerName("setMessageText:"),     titleCF);
	objc_msgSend(alert, sel_registerName("setInformativeText:"), msgCF);
	objc_msgSend(alert, sel_registerName("addButtonWithTitle:"), CFSTR("OK"));
	
	objc_msgSend(alert, sel_registerName("runModal"));
	CFRelease(titleCF);
	CFRelease(msgCF);
}

static struct Bitmap fb_bmp;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	fb_bmp = *bmp;
}

static void View_DrawRect(id self, SEL cmd, CGRect r_) {
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	CGContextRef context = NULL;
	CGDataProviderRef provider;
	CGImageRef image;
	CGRect rect;
	id nsContext;

	/* Unfortunately CGImageRef is immutable, so changing the */
	/* underlying data doesn't change what shows when drawing. */
	/* TODO: Find a better way of doing this in cocoa.. */
	if (!fb_bmp.scan0) return;
	nsContext = objc_msgSend((id)objc_getClass("NSGraphicsContext"), selCurrentContext);
	context   = objc_msgSend(nsContext, selGraphicsPort);

	/* TODO: Only update changed bit.. */
	rect.origin.x = 0; rect.origin.y = 0;
	rect.size.width  = WindowInfo.Width;
	rect.size.height = WindowInfo.Height;

	/* TODO: REPLACE THIS AWFUL HACK */
	provider = CGDataProviderCreateWithData(NULL, fb_bmp.scan0,
		Bitmap_DataSize(fb_bmp.width, fb_bmp.height), NULL);
	image = CGImageCreate(fb_bmp.width, fb_bmp.height, 8, 32, fb_bmp.width * 4, colorSpace,
		kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst, provider, NULL, 0, 0);

	CGContextDrawImage(context, rect, image);
	CGContextSynchronize(context);

	CGImageRelease(image);
	CGDataProviderRelease(provider);
	CGColorSpaceRelease(colorSpace);
}

void Window_DrawFramebuffer(Rect2D r) {
	CGRect rect;
	rect.origin.x    = r.X; 
	rect.origin.y    = WindowInfo.Height - r.Y - r.Height;
	rect.size.width  = r.Width;
	rect.size.height = r.Height;

	objc_msgSend(viewHandle, selSetNeedsDisplay, rect);
	objc_msgSend(viewHandle, selDisplayIfNeeded);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}
#endif


/*########################################################################################################################*
*------------------------------------------------Emscripten canvas window-------------------------------------------------*
*#########################################################################################################################*/
#elif defined CC_BUILD_WEB
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/key_codes.h>
static cc_bool keyboardOpen, needResize;

static int RawDpiScale(int x)    { return (int)(x * emscripten_get_device_pixel_ratio()); }
static int GetCanvasWidth(void)  { return EM_ASM_INT_V({ return Module['canvas'].width  }); }
static int GetCanvasHeight(void) { return EM_ASM_INT_V({ return Module['canvas'].height }); }
static int GetScreenWidth(void)  { return RawDpiScale(EM_ASM_INT_V({ return screen.width;  })); }
static int GetScreenHeight(void) { return RawDpiScale(EM_ASM_INT_V({ return screen.height; })); }

static void UpdateWindowBounds(void) {
	int width  = GetCanvasWidth();
	int height = GetCanvasHeight();
	if (width == WindowInfo.Width && height == WindowInfo.Height) return;

	WindowInfo.Width  = width;
	WindowInfo.Height = height;
	Event_RaiseVoid(&WindowEvents.Resized);
}

static void SetFullscreenBounds(void) {
	int width  = GetScreenWidth();
	int height = GetScreenHeight();
	emscripten_set_canvas_element_size("#canvas", width, height);
}

/* Browser only allows pointer lock requests in response to user input */
static void DeferredEnableRawMouse(void) {
	EmscriptenPointerlockChangeEvent status;
	if (!Input_RawMode) return;

	status.isActive = false;
	emscripten_get_pointerlock_status(&status);
	if (!status.isActive) emscripten_request_pointerlock("#canvas", false);
}

static EM_BOOL OnMouseWheel(int type, const EmscriptenWheelEvent* ev, void* data) {
	/* TODO: The scale factor isn't standardised.. is there a better way though? */
	Mouse_ScrollWheel(-Math_Sign(ev->deltaY));
	DeferredEnableRawMouse();
	return true;
}

static EM_BOOL OnMouseButton(int type, const EmscriptenMouseEvent* ev, void* data) {
	cc_bool down = type == EMSCRIPTEN_EVENT_MOUSEDOWN;
	switch (ev->button) {
		case 0: Input_Set(KEY_LMOUSE, down); break;
		case 1: Input_Set(KEY_MMOUSE, down); break;
		case 2: Input_Set(KEY_RMOUSE, down); break;
	}

	DeferredEnableRawMouse();
	return true;
}
 
/* input coordinates are CSS pixels, remap to internal pixels */
static void RescaleXY(int* x, int* y) {
	double css_width, css_height;
	emscripten_get_element_css_size("#canvas", &css_width, &css_height);

	if (css_width && css_height) {
		*x = (int)(*x * WindowInfo.Width  / css_width );
		*y = (int)(*y * WindowInfo.Height / css_height);
	} else {
		/* If css width or height is 0, something is bogus    */
		/* Better to avoid divsision by 0 in that case though */
	}
}

static EM_BOOL OnMouseMove(int type, const EmscriptenMouseEvent* ev, void* data) {
	int x, y, buttons = ev->buttons;
	/* Set before position change, in case mouse buttons changed when outside window */
	Input_SetNonRepeatable(KEY_LMOUSE, buttons & 0x01);
	Input_SetNonRepeatable(KEY_RMOUSE, buttons & 0x02);
	Input_SetNonRepeatable(KEY_MMOUSE, buttons & 0x04);

	x = ev->targetX; y = ev->targetY;
	RescaleXY(&x, &y);
	Pointer_SetPosition(0, x, y);
	if (Input_RawMode) Event_RaiseRawMove(&PointerEvents.RawMoved, ev->movementX, ev->movementY);
	return true;
}

/* TODO: Also query mouse coordinates globally and reuse adjustXY here */
/* Adjust from document coordinates to element coordinates */
static void AdjustXY(int* x, int* y) {
	EM_ASM_({
		var canvasRect = Module['canvas'].getBoundingClientRect();
		HEAP32[$0 >> 2] = HEAP32[$0 >> 2] - canvasRect.left;
		HEAP32[$1 >> 2] = HEAP32[$1 >> 2] - canvasRect.top;
	}, x, y);
}

static EM_BOOL OnTouchStart(int type, const EmscriptenTouchEvent* ev, void* data) {
	const EmscriptenTouchPoint* t;
	int i, x, y;
	for (i = 0; i < ev->numTouches; ++i) {
		t = &ev->touches[i];
		if (!t->isChanged) continue;
		x = t->targetX; y = t->targetY;

		AdjustXY( &x, &y);
		RescaleXY(&x, &y);
		Input_AddTouch(t->identifier, x, y);
	}
	/* Don't intercept touchstart events while keyboard is open, that way */
	/* user can still touch to move the caret position in input textbox. */
	return !keyboardOpen;
}

static EM_BOOL OnTouchMove(int type, const EmscriptenTouchEvent* ev, void* data) {
	const EmscriptenTouchPoint* t;
	int i, x, y;
	for (i = 0; i < ev->numTouches; ++i) {
		t = &ev->touches[i];
		if (!t->isChanged) continue;
		x = t->targetX; y = t->targetY;

		AdjustXY( &x, &y);
		RescaleXY(&x, &y);
		Input_UpdateTouch(t->identifier, x, y);
	}
	/* Don't intercept touchmove events while keyboard is open, that way */
	/* user can still touch to move the caret position in input textbox. */
	return !keyboardOpen;
}

static EM_BOOL OnTouchEnd(int type, const EmscriptenTouchEvent* ev, void* data) {
	const EmscriptenTouchPoint* t;
	int i, x, y;
	for (i = 0; i < ev->numTouches; ++i) {
		t = &ev->touches[i];
		if (!t->isChanged) continue;
		x = t->targetX; y = t->targetY;

		AdjustXY( &x, &y);
		RescaleXY(&x, &y);
		Input_RemoveTouch(t->identifier, x, y);
	}
	/* Don't intercept touchend events while keyboard is open, that way */
	/* user can still touch to move the caret position in input textbox. */
	return !keyboardOpen;
}

static EM_BOOL OnFocus(int type, const EmscriptenFocusEvent* ev, void* data) {
	WindowInfo.Focused = type == EMSCRIPTEN_EVENT_FOCUS;
	Event_RaiseVoid(&WindowEvents.FocusChanged);
	return true;
}

static EM_BOOL OnResize(int type, const EmscriptenUiEvent* ev, void *data) {
	UpdateWindowBounds(); needResize = true;
	return true;
}
/* This is only raised when going into fullscreen */
static EM_BOOL OnCanvasResize(int type, const void* reserved, void *data) {
	UpdateWindowBounds(); needResize = true;
	return false;
}
static EM_BOOL OnFullscreenChange(int type, const EmscriptenFullscreenChangeEvent* ev, void *data) {
	UpdateWindowBounds(); needResize = true;
	return false;
}

static const char* OnBeforeUnload(int type, const void* ev, void *data) {
	Window_Close();
	return NULL;
}

static int MapNativeKey(int k) {
	if (k >= '0' && k <= '9') return k;
	if (k >= 'A' && k <= 'Z') return k;
	if (k >= DOM_VK_F1      && k <= DOM_VK_F24)     { return KEY_F1  + (k - DOM_VK_F1); }
	if (k >= DOM_VK_NUMPAD0 && k <= DOM_VK_NUMPAD9) { return KEY_KP0 + (k - DOM_VK_NUMPAD0); }

	switch (k) {
	case DOM_VK_BACK_SPACE: return KEY_BACKSPACE;
	case DOM_VK_TAB:        return KEY_TAB;
	case DOM_VK_RETURN:     return KEY_ENTER;
	case DOM_VK_SHIFT:      return KEY_LSHIFT;
	case DOM_VK_CONTROL:    return KEY_LCTRL;
	case DOM_VK_ALT:        return KEY_LALT;
	case DOM_VK_PAUSE:      return KEY_PAUSE;
	case DOM_VK_CAPS_LOCK:  return KEY_CAPSLOCK;
	case DOM_VK_ESCAPE:     return KEY_ESCAPE;
	case DOM_VK_SPACE:      return KEY_SPACE;

	case DOM_VK_PAGE_UP:     return KEY_PAGEUP;
	case DOM_VK_PAGE_DOWN:   return KEY_PAGEDOWN;
	case DOM_VK_END:         return KEY_END;
	case DOM_VK_HOME:        return KEY_HOME;
	case DOM_VK_LEFT:        return KEY_LEFT;
	case DOM_VK_UP:          return KEY_UP;
	case DOM_VK_RIGHT:       return KEY_RIGHT;
	case DOM_VK_DOWN:        return KEY_DOWN;
	case DOM_VK_PRINTSCREEN: return KEY_PRINTSCREEN;
	case DOM_VK_INSERT:      return KEY_INSERT;
	case DOM_VK_DELETE:      return KEY_DELETE;

	case DOM_VK_SEMICOLON:   return KEY_SEMICOLON;
	case DOM_VK_EQUALS:      return KEY_EQUALS;
	case DOM_VK_WIN:         return KEY_LWIN;
	case DOM_VK_MULTIPLY:    return KEY_KP_MULTIPLY;
	case DOM_VK_ADD:         return KEY_KP_PLUS;
	case DOM_VK_SUBTRACT:    return KEY_KP_MINUS;
	case DOM_VK_DECIMAL:     return KEY_KP_DECIMAL;
	case DOM_VK_DIVIDE:      return KEY_KP_DIVIDE;
	case DOM_VK_NUM_LOCK:    return KEY_NUMLOCK;
	case DOM_VK_SCROLL_LOCK: return KEY_SCROLLLOCK;
		
	case DOM_VK_HYPHEN_MINUS:  return KEY_MINUS;
	case DOM_VK_COMMA:         return KEY_COMMA;
	case DOM_VK_PERIOD:        return KEY_PERIOD;
	case DOM_VK_SLASH:         return KEY_SLASH;
	case DOM_VK_BACK_QUOTE:    return KEY_TILDE;
	case DOM_VK_OPEN_BRACKET:  return KEY_LBRACKET;
	case DOM_VK_BACK_SLASH:    return KEY_BACKSLASH;
	case DOM_VK_CLOSE_BRACKET: return KEY_RBRACKET;
	case DOM_VK_QUOTE:         return KEY_QUOTE;

	/* chrome */
	case 186: return KEY_SEMICOLON;
	case 187: return KEY_EQUALS;
	case 189: return KEY_MINUS;
	}
	return KEY_NONE;
}

static EM_BOOL OnKey(int type, const EmscriptenKeyboardEvent* ev, void* data) {
	int key = MapNativeKey(ev->keyCode);

	if (ev->location == DOM_KEY_LOCATION_RIGHT) {
		switch (key) {
		case KEY_LALT:   key = KEY_RALT; break;
		case KEY_LCTRL:  key = KEY_RCTRL; break;
		case KEY_LSHIFT: key = KEY_RSHIFT; break;
		case KEY_LWIN:   key = KEY_RWIN; break;
		}
	}
	else if (ev->location == DOM_KEY_LOCATION_NUMPAD) {
		switch (key) {
		case KEY_ENTER: key = KEY_KP_ENTER; break;
		}
	}

	if (key) Input_Set(key, type == EMSCRIPTEN_EVENT_KEYDOWN);
	DeferredEnableRawMouse();

	if (!key) return false;
	/* KeyUp always intercepted */
	if (type != EMSCRIPTEN_EVENT_KEYDOWN) return true;

	/* If holding down Ctrl or Alt, keys aren't going to generate a KeyPress event anyways. */
	/* This intercepts Ctrl+S etc. Ctrl+C and Ctrl+V are not intercepted for clipboard. */
	if (Key_IsAltPressed() || Key_IsWinPressed()) return true;
	if (Key_IsControlPressed() && key != 'C' && key != 'V') return true;

	/* Space needs special handling, as intercepting this prevents the ' ' key press event */
	/* But on Safari, space scrolls the page - so need to intercept when keyboard is NOT open */
	if (key == KEY_SPACE) return !keyboardOpen;

	/* Must not intercept KeyDown for regular keys, otherwise KeyPress doesn't get raised. */
	/* However, do want to prevent browser's behaviour on F11, F5, home etc. */
	/* e.g. not preventing F11 means browser makes page fullscreen instead of just canvas */
	return (key >= KEY_F1  && key <= KEY_F24)  || (key >= KEY_UP    && key <= KEY_RIGHT) ||
		(key >= KEY_INSERT && key <= KEY_MENU) || (key >= KEY_ENTER && key <= KEY_NUMLOCK);
}

static EM_BOOL OnKeyPress(int type, const EmscriptenKeyboardEvent* ev, void* data) {
	char keyChar;
	DeferredEnableRawMouse();
	/* When on-screen keyboard is open, we don't want to intercept any key presses, */
	/* because they should be sent to the HTML text input instead. */
	/* (Chrome for android sends keypresses sometimes for '0' to '9' keys) */
	/* - If any keys are intercepted, this causes the HTML text input to become */
	/*   desynchronised from the chat/menu input widget the user sees in game. */
	/* - This causes problems such as attempting to backspace all text later to */
	/*   not actually backspace everything. (because the HTML text input does not */
	/*   have these intercepted key presses in its text buffer) */
	if (Input_TouchMode && keyboardOpen) return false;

	if (Convert_TryCodepointToCP437(ev->charCode, &keyChar)) {
		Event_RaiseInt(&InputEvents.Press, keyChar);
	}
	return true;
}

/* Really old emscripten versions (e.g. 1.38.21) don't have this defined */
/* Can't just use "#window", newer versions switched to const int instead */
#ifndef EMSCRIPTEN_EVENT_TARGET_WINDOW
#define EMSCRIPTEN_EVENT_TARGET_WINDOW "#window"
#endif

static void HookEvents(void) {
	emscripten_set_wheel_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 0, OnMouseWheel);
	emscripten_set_mousedown_callback("#canvas",                  NULL, 0, OnMouseButton);
	emscripten_set_mouseup_callback("#canvas",                    NULL, 0, OnMouseButton);
	emscripten_set_mousemove_callback("#canvas",                  NULL, 0, OnMouseMove);
	emscripten_set_fullscreenchange_callback("#canvas",           NULL, 0, OnFullscreenChange);

	emscripten_set_focus_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,  NULL, 0, OnFocus);
	emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,   NULL, 0, OnFocus);
	emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 0, OnResize);
	emscripten_set_beforeunload_callback(                          NULL,    OnBeforeUnload);

	emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,  NULL, 0, OnKey);
	emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,    NULL, 0, OnKey);
	emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 0, OnKeyPress);

	emscripten_set_touchstart_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,  NULL, 0, OnTouchStart);
	emscripten_set_touchmove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,   NULL, 0, OnTouchMove);
	emscripten_set_touchend_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,    NULL, 0, OnTouchEnd);
	emscripten_set_touchcancel_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 0, OnTouchEnd);
}

static void UnhookEvents(void) {
	emscripten_set_wheel_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 0, NULL);
	emscripten_set_mousedown_callback("#canvas",                  NULL, 0, NULL);
	emscripten_set_mouseup_callback("#canvas",                    NULL, 0, NULL);
	emscripten_set_mousemove_callback("#canvas",                  NULL, 0, NULL);

	emscripten_set_focus_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,  NULL, 0, NULL);
	emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,   NULL, 0, NULL);
	emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 0, NULL);
	emscripten_set_beforeunload_callback(                          NULL,    NULL);

	emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,  NULL, 0, NULL);
	emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,    NULL, 0, NULL);
	emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 0, NULL);

	emscripten_set_touchstart_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,  NULL, 0, NULL);
	emscripten_set_touchmove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,   NULL, 0, NULL);
	emscripten_set_touchend_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,    NULL, 0, NULL);
	emscripten_set_touchcancel_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 0, NULL);
}

void Window_Init(void) {
	int is_ios, droid;
	DisplayInfo.Width  = GetScreenWidth();
	DisplayInfo.Height = GetScreenHeight();
	DisplayInfo.Depth  = 24;

	DisplayInfo.ScaleX = emscripten_get_device_pixel_ratio();
	DisplayInfo.ScaleY = DisplayInfo.ScaleX;

	/* Copy text, but only if user isn't selecting something else on the webpage */
	/* (don't check window.clipboardData here, that's handled in Clipboard_SetText instead) */
	EM_ASM(window.addEventListener('copy', 
		function(e) {
			if (window.getSelection && window.getSelection().toString()) return;
			if (window.cc_copyText) {
				if (e.clipboardData) { e.clipboardData.setData('text/plain', window.cc_copyText); }
				e.preventDefault();
				window.cc_copyText = null;
			}	
		});
	);

	/* Paste text (window.clipboardData is handled in Clipboard_RequestText instead) */
	EM_ASM(window.addEventListener('paste',
		function(e) {
			var contents = e.clipboardData ? e.clipboardData.getData('text/plain') : "";
			ccall('Window_GotClipboardText', 'void', ['string'], [contents]);
		});
	);

	droid  = EM_ASM_INT_V({ return /Android/i.test(navigator.userAgent); });
	/* iOS 13 on iPad doesn't identify itself as iPad by default anymore */
	/*  https://stackoverflow.com/questions/57765958/how-to-detect-ipad-and-ipad-os-version-in-ios-13-and-up */
	is_ios = EM_ASM_INT_V({
		return /iPhone|iPad|iPod/i.test(navigator.userAgent) || 
		(navigator.platform === 'MacIntel' && navigator.maxTouchPoints && navigator.maxTouchPoints > 2);
	});
	Input_TouchMode = is_ios || droid;
	Pointers_Count  = Input_TouchMode ? 0 : 1;

	/* iOS shifts the whole webpage up when opening chat, which causes problems */
	/*  as the chat/send butons are positioned at the top of the canvas - they */
	/*  get pushed offscreen and can't be used at all anymore. So handle this */
	/*  case specially by positioning them at the bottom instead for iOS. */
	WindowInfo.SoftKeyboard = is_ios ? SOFT_KEYBOARD_SHIFT : SOFT_KEYBOARD_RESIZE;

	/* Let the webpage know it needs to force a mobile layout */
	if (!Input_TouchMode) return;
	EM_ASM( if (typeof(forceTouchLayout) === 'function') forceTouchLayout(); );
}

void Window_Create(int width, int height) {
	WindowInfo.Exists  = true;
	WindowInfo.Focused = true;
	HookEvents();
	/* Let the webpage decide on initial bounds */
	WindowInfo.Width  = GetCanvasWidth();
	WindowInfo.Height = GetCanvasHeight();

	/* Create wrapper div if necessary */
	EM_ASM({
		var agent  = navigator.userAgent;	
		var canvas = Module['canvas'];
		window.cc_container = document.body;

		if (/Android/i.test(agent) && /Chrome/i.test(agent)) {
			var wrapper = document.createElement("div");
			wrapper.id  = 'canvas_wrapper';

			canvas.parentNode.insertBefore(wrapper, canvas);
			wrapper.appendChild(canvas);
			window.cc_container = wrapper;
		}
	});
}

void Window_SetTitle(const cc_string* title) {
	char str[NATIVE_STR_LEN];
	Platform_EncodeUtf8(str, title);
	EM_ASM_({ document.title = UTF8ToString($0); }, str);
}

static RequestClipboardCallback clipboard_func;
static void* clipboard_obj;

EMSCRIPTEN_KEEPALIVE void Window_GotClipboardText(char* src) {
	cc_string str; char strBuffer[512];
	if (!clipboard_func) return;

	String_InitArray(str, strBuffer);
	String_AppendUtf8(&str, src, String_CalcLen(src, 2048));
	clipboard_func(&str, clipboard_obj);
	clipboard_func = NULL;
}

void Clipboard_GetText(cc_string* value) { }
void Clipboard_SetText(const cc_string* value) {
	char str[NATIVE_STR_LEN];
	Platform_EncodeUtf8(str, value);

	/* For IE11, use window.clipboardData to set the clipboard */
	/* For other browsers, instead use the window.copy events */
	EM_ASM_({ 
		if (window.clipboardData) {
			if (window.getSelection && window.getSelection().toString()) return;
			window.clipboardData.setData('Text', UTF8ToString($0));
		} else {
			window.cc_copyText = UTF8ToString($0);
		}
	}, str);
}

void Clipboard_RequestText(RequestClipboardCallback callback, void* obj) {
	clipboard_func = callback;
	clipboard_obj  = obj;

	/* For IE11, use window.clipboardData to get the clipboard */
	EM_ASM_({
		if (!window.clipboardData) return;
		var contents = window.clipboardData.getData('Text');
		ccall('Window_GotClipboardText', 'void', ['string'], [contents]);
	});
}

void Window_Show(void) { }

int Window_GetWindowState(void) {
	EmscriptenFullscreenChangeEvent status = { 0 };
	emscripten_get_fullscreen_status(&status);
	return status.isFullscreen ? WINDOW_STATE_FULLSCREEN : WINDOW_STATE_NORMAL;
}

cc_result Window_EnterFullscreen(void) {
	EmscriptenFullscreenStrategy strategy;
	const char* target;
	int res;
	strategy.scaleMode                 = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH;
	strategy.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF;
	strategy.filteringMode             = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;

	strategy.canvasResizedCallback         = OnCanvasResize;
	strategy.canvasResizedCallbackUserData = NULL;

	/* For chrome on android, need to make container div fullscreen instead */
	res    = EM_ASM_INT_V({ return document.getElementById('canvas_wrapper') ? 1 : 0; });
	target = res ? "canvas_wrapper" : "#canvas";

	res = emscripten_request_fullscreen_strategy(target, 1, &strategy);
	if (res == EMSCRIPTEN_RESULT_NOT_SUPPORTED) res = ERR_NOT_SUPPORTED;
	if (res) return res;

	/* emscripten sets css size to screen's base width/height, */
	/*  except that becomes wrong when device rotates. */
	/* Better to just set CSS width/height to always be 100% */
	EM_ASM({
		var canvas = Module['canvas'];
		canvas.style.width  = '100%';
		canvas.style.height = '100%';
	});

	/* By default, pressing Escape will immediately exit fullscreen - which is */
	/*   quite annoying given that it is also the Menu key. Some browsers allow */
	/*   'locking' the Escape key, so that you have to hold down Escape to exit. */
	/* NOTE: This ONLY works when the webpage is a https:// one */
	EM_ASM({ try { navigator.keyboard.lock(["Escape"]); } catch (ex) { } });
	return 0;
}

cc_result Window_ExitFullscreen(void) {
	emscripten_exit_fullscreen();
	UpdateWindowBounds();
	return 0;
}

void Window_SetSize(int width, int height) {
	emscripten_set_canvas_element_size("#canvas", width, height);
	/* CSS size is in CSS units not pixel units */
	emscripten_set_element_css_size("#canvas", width / DisplayInfo.ScaleX, height / DisplayInfo.ScaleY);
	UpdateWindowBounds();
}

void Window_Close(void) {
	WindowInfo.Exists = false;
	Event_RaiseVoid(&WindowEvents.Closing);
	/* If the game is closed while in fullscreen, the last rendered frame stays */
	/* shown in fullscreen, but the game can't be interacted with anymore */
	Window_ExitFullscreen();

	/* Don't want cursor stuck on the dead 0,0 canvas */
	Window_DisableRawMouse();
	Window_SetSize(0, 0);
	UnhookEvents();
}

void Window_ProcessEvents(void) {
	if (!needResize) return;
	needResize = false;
	if (!WindowInfo.Exists) return;

	if (Window_GetWindowState() == WINDOW_STATE_FULLSCREEN) {
		SetFullscreenBounds();
	} else {
		EM_ASM( if (typeof(resizeGameCanvas) === 'function') resizeGameCanvas(); );
	}
	UpdateWindowBounds();
}

/* Not needed because browser provides relative mouse and touch events */
static void Cursor_GetRawPos(int* x, int* y) { *x = 0; *y = 0; }
/* Not allowed to move cursor from javascript */
void Cursor_SetPosition(int x, int y) { }

static void Cursor_DoSetVisible(cc_bool visible) {
	if (visible) {
		EM_ASM(Module['canvas'].style['cursor'] = 'default'; );
	} else {
		EM_ASM(Module['canvas'].style['cursor'] = 'none'; );
	}
}

static void ShowDialogCore(const char* title, const char* msg) {
	EM_ASM_({ alert(UTF8ToString($0) + "\n\n" + UTF8ToString($1)); }, title, msg);
}

static OpenFileDialogCallback uploadCallback;
EMSCRIPTEN_KEEPALIVE void Window_OnFileUploaded(const char* src) { 
	cc_string file; char buffer[FILENAME_SIZE];
	String_InitArray(file, buffer);

	String_AppendUtf8(&file, src, String_Length(src));
	uploadCallback(&file);
	uploadCallback = NULL;
}

cc_result Window_OpenFileDialog(const char* filter, OpenFileDialogCallback callback) {
	uploadCallback = callback;
	EM_ASM_({
		var elem = window.cc_uploadElem;
		if (!elem) {
			elem = document.createElement('input');
			elem.setAttribute('type', 'file');
			elem.setAttribute('style', 'display: none');
			elem.accept = UTF8ToString($0);

			elem.addEventListener('change', 
				function(ev) {
					var files = ev.target.files;
					for (var i = 0; i < files.length; i++) {
						var reader = new FileReader();
						var name   = files[i].name;

						reader.onload = function(e) { 
							var data = new Uint8Array(e.target.result);
							FS.createDataFile('/', name, data, true, true, true);
							ccall('Window_OnFileUploaded', 'void', ['string'], ['/' + name]);
							FS.unlink('/' + name);
						};
						reader.readAsArrayBuffer(files[i]);
					}
					window.cc_container.removeChild(window.cc_uploadElem);
					window.cc_uploadElem = null;
				}, false);
			window.cc_uploadElem = elem;
			window.cc_container.appendChild(elem);
		}
		elem.click();
	}, filter);
	return ERR_NOT_SUPPORTED;
}

void Window_AllocFramebuffer(struct Bitmap* bmp) { }
void Window_DrawFramebuffer(Rect2D r)     { }
void Window_FreeFramebuffer(struct Bitmap* bmp)  { }

EMSCRIPTEN_KEEPALIVE void Window_OnTextChanged(const char* src) { 
	cc_string str; char buffer[800];
	String_InitArray(str, buffer);

	String_AppendUtf8(&str, src, String_CalcLen(src, 3200));
	Event_RaiseString(&InputEvents.TextChanged, &str);
}

void Window_OpenKeyboard(const struct OpenKeyboardArgs* args) {
	char str[NATIVE_STR_LEN];
	keyboardOpen = true;
	if (!Input_TouchMode) return;
	Platform_EncodeUtf8(str, args->text);
	Platform_LogConst("OPEN SESAME");

	EM_ASM_({
		var elem = window.cc_inputElem;
		if (!elem) {
			if ($1 == 1) {
				elem = document.createElement('input');
				elem.setAttribute('inputmode', 'decimal');
			} else {
				elem = document.createElement('textarea');
			}
			elem.setAttribute('style', 'position:absolute; left:0; bottom:0; margin: 0px; width: 100%');
			elem.setAttribute('placeholder', UTF8ToString($2));
			elem.value = UTF8ToString($0);

			elem.addEventListener('input', 
				function(ev) {
					ccall('Window_OnTextChanged', 'void', ['string'], [ev.target.value]);
				}, false);
			window.cc_inputElem = elem;

			window.cc_divElem = document.createElement('div');
			window.cc_divElem.setAttribute('style', 'position:absolute; left:0; top:0; width:100%; height:100%; background-color: black; opacity:0.4; resize:none; pointer-events:none;');
			
			window.cc_container.appendChild(window.cc_divElem);
			window.cc_container.appendChild(elem);
		}
		elem.focus();
		elem.click();
	}, str, args->type, args->placeholder);
}

void Window_SetKeyboardText(const cc_string* text) {
	char str[NATIVE_STR_LEN];
	if (!Input_TouchMode) return;
	Platform_EncodeUtf8(str, text);

	EM_ASM_({
		if (!window.cc_inputElem) return;
		var str = UTF8ToString($0);

		if (str == window.cc_inputElem.value) return;
		window.cc_inputElem.value = str;
	}, str);
}

void Window_CloseKeyboard(void) {
	keyboardOpen = false;
	if (!Input_TouchMode) return;

	EM_ASM({
		if (!window.cc_inputElem) return;
		window.cc_container.removeChild(window.cc_divElem);
		window.cc_container.removeChild(window.cc_inputElem);
		window.cc_divElem   = null;
		window.cc_inputElem = null;
	});
}

void Window_EnableRawMouse(void) {
	RegrabMouse();
	/* defer pointerlock request until next user input */
	Input_RawMode = true;
}
void Window_UpdateRawMouse(void) { }

void Window_DisableRawMouse(void) {
	RegrabMouse();
	emscripten_exit_pointerlock();
	Input_RawMode = false;
}


/*########################################################################################################################*
*------------------------------------------------Android activity window-------------------------------------------------*
*#########################################################################################################################*/
#elif defined CC_BUILD_ANDROID
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/keycodes.h>
static ANativeWindow* win_handle;

static void RefreshWindowBounds(void) {
	WindowInfo.Width  = ANativeWindow_getWidth(win_handle);
	WindowInfo.Height = ANativeWindow_getHeight(win_handle);
	Platform_Log2("SCREEN BOUNDS: %i,%i", &WindowInfo.Width, &WindowInfo.Height);
	Event_RaiseVoid(&WindowEvents.Resized);
}

static int MapNativeKey(int code) {
	if (code >= AKEYCODE_0  && code <= AKEYCODE_9)   return (code - AKEYCODE_0)  + '0';
	if (code >= AKEYCODE_A  && code <= AKEYCODE_Z)   return (code - AKEYCODE_A)  + 'A';
	if (code >= AKEYCODE_F1 && code <= AKEYCODE_F12) return (code - AKEYCODE_F1) + KEY_F1;
	if (code >= AKEYCODE_NUMPAD_0 && code <= AKEYCODE_NUMPAD_9) return (code - AKEYCODE_NUMPAD_0) + KEY_KP0;

	switch (code) {
		/* TODO: AKEYCODE_STAR */
		/* TODO: AKEYCODE_POUND */
	case AKEYCODE_BACK:   return KEY_ESCAPE;
	case AKEYCODE_COMMA:  return KEY_COMMA;
	case AKEYCODE_PERIOD: return KEY_PERIOD;
	case AKEYCODE_ALT_LEFT:    return KEY_LALT;
	case AKEYCODE_ALT_RIGHT:   return KEY_RALT;
	case AKEYCODE_SHIFT_LEFT:  return KEY_LSHIFT;
	case AKEYCODE_SHIFT_RIGHT: return KEY_RSHIFT;
	case AKEYCODE_TAB:    return KEY_TAB;
	case AKEYCODE_SPACE:  return KEY_SPACE;
	case AKEYCODE_ENTER:  return KEY_ENTER;
	case AKEYCODE_DEL:    return KEY_BACKSPACE;
	case AKEYCODE_GRAVE:  return KEY_TILDE;
	case AKEYCODE_MINUS:  return KEY_MINUS;
	case AKEYCODE_EQUALS: return KEY_EQUALS;
	case AKEYCODE_LEFT_BRACKET:  return KEY_LBRACKET;
	case AKEYCODE_RIGHT_BRACKET: return KEY_RBRACKET;
	case AKEYCODE_BACKSLASH:  return KEY_BACKSLASH;
	case AKEYCODE_SEMICOLON:  return KEY_SEMICOLON;
	case AKEYCODE_APOSTROPHE: return KEY_QUOTE;
	case AKEYCODE_SLASH:      return KEY_SLASH;
		/* TODO: AKEYCODE_AT */
		/* TODO: AKEYCODE_PLUS */
		/* TODO: AKEYCODE_MENU */
	case AKEYCODE_PAGE_UP:     return KEY_PAGEUP;
	case AKEYCODE_PAGE_DOWN:   return KEY_PAGEDOWN;
	case AKEYCODE_ESCAPE:      return KEY_ESCAPE;
	case AKEYCODE_FORWARD_DEL: return KEY_DELETE;
	case AKEYCODE_CTRL_LEFT:   return KEY_LCTRL;
	case AKEYCODE_CTRL_RIGHT:  return KEY_RCTRL;
	case AKEYCODE_CAPS_LOCK:   return KEY_CAPSLOCK;
	case AKEYCODE_SCROLL_LOCK: return KEY_SCROLLLOCK;
	case AKEYCODE_META_LEFT:   return KEY_LWIN;
	case AKEYCODE_META_RIGHT:  return KEY_RWIN;
	case AKEYCODE_SYSRQ:    return KEY_PRINTSCREEN;
	case AKEYCODE_BREAK:    return KEY_PAUSE;
	case AKEYCODE_INSERT:   return KEY_INSERT;
	case AKEYCODE_NUM_LOCK: return KEY_NUMLOCK;
	case AKEYCODE_NUMPAD_DIVIDE:   return KEY_KP_DIVIDE;
	case AKEYCODE_NUMPAD_MULTIPLY: return KEY_KP_MULTIPLY;
	case AKEYCODE_NUMPAD_SUBTRACT: return KEY_KP_MINUS;
	case AKEYCODE_NUMPAD_ADD:      return KEY_KP_PLUS;
	case AKEYCODE_NUMPAD_DOT:      return KEY_KP_DECIMAL;
	case AKEYCODE_NUMPAD_ENTER:    return KEY_KP_ENTER;
	}
	return KEY_NONE;
}

static void JNICALL java_processKeyDown(JNIEnv* env, jobject o, jint code) {
	int key = MapNativeKey(code);
	Platform_Log2("KEY - DOWN %i,%i", &code, &key);
	if (key) Input_SetPressed(key);
}

static void JNICALL java_processKeyUp(JNIEnv* env, jobject o, jint code) {
	int key = MapNativeKey(code);
	Platform_Log2("KEY - UP %i,%i", &code, &key);
	if (key) Input_SetReleased(key);
}

static void JNICALL java_processKeyChar(JNIEnv* env, jobject o, jint code) {
	char keyChar;
	int key = MapNativeKey(code);
	Platform_Log2("KEY - PRESS %i,%i", &code, &key);

	if (Convert_TryCodepointToCP437(code, &keyChar)) {
		Event_RaiseInt(&InputEvents.Press, keyChar);
	}
}

static void JNICALL java_processKeyText(JNIEnv* env, jobject o, jstring str) {
	char buffer[NATIVE_STR_LEN];
	cc_string text = JavaGetString(env, str, buffer);
	Platform_Log1("KEY - TEXT %s", &text);
	Event_RaiseString(&InputEvents.TextChanged, &text);
}

static void JNICALL java_processMouseDown(JNIEnv* env, jobject o, jint id, jint x, jint y) {
	Platform_Log3("MOUSE %i - DOWN %i,%i", &id, &x, &y);
	Input_AddTouch(id, x, y);
}

static void JNICALL java_processMouseUp(JNIEnv* env, jobject o, jint id, jint x, jint y) {
	Platform_Log3("MOUSE %i - UP   %i,%i", &id, &x, &y);
	Input_RemoveTouch(id, x, y);
}

static void JNICALL java_processMouseMove(JNIEnv* env, jobject o, jint id, jint x, jint y) {
	Platform_Log3("MOUSE %i - MOVE %i,%i", &id, &x, &y);
	Input_UpdateTouch(id, x, y);
}

static void JNICALL java_processSurfaceCreated(JNIEnv* env, jobject o, jobject surface) {
	Platform_LogConst("WIN - CREATED");
	win_handle        = ANativeWindow_fromSurface(env, surface);
	WindowInfo.Handle = win_handle;
	RefreshWindowBounds();
	/* TODO: Restore context */
	Event_RaiseVoid(&WindowEvents.Created);
}

#include "Graphics.h"
static void JNICALL java_processSurfaceDestroyed(JNIEnv* env, jobject o) {
	Platform_LogConst("WIN - DESTROYED");
	if (win_handle) ANativeWindow_release(win_handle);

	win_handle        = NULL;
	WindowInfo.Handle = NULL;
	/* eglSwapBuffers might return EGL_BAD_SURFACE, EGL_BAD_ALLOC, or some other error */
	/* Instead the context is lost here in a consistent manner */
	if (Gfx.Created) Gfx_LoseContext("surface lost");
	JavaCallVoid(env, "processedSurfaceDestroyed", "()V", NULL);
}

static void JNICALL java_processSurfaceResized(JNIEnv* env, jobject o, jobject surface) {
	Platform_LogConst("WIN - RESIZED");
	RefreshWindowBounds();
}

static void JNICALL java_processSurfaceRedrawNeeded(JNIEnv* env, jobject o) {
	Platform_LogConst("WIN - REDRAW");
	Event_RaiseVoid(&WindowEvents.Redraw);
}

static void JNICALL java_onStart(JNIEnv* env, jobject o) {
	Platform_LogConst("APP - ON START");
}

static void JNICALL java_onStop(JNIEnv* env, jobject o) {
	Platform_LogConst("APP - ON STOP");
}

static void JNICALL java_onResume(JNIEnv* env, jobject o) {
	Platform_LogConst("APP - ON RESUME");
	/* TODO: Resume rendering */
}

static void JNICALL java_onPause(JNIEnv* env, jobject o) {
	Platform_LogConst("APP - ON PAUSE");
	/* TODO: Disable rendering */
}

static void JNICALL java_onDestroy(JNIEnv* env, jobject o) {
	Platform_LogConst("APP - ON DESTROY");

	if (WindowInfo.Exists) Window_Close();
	/* TODO: signal to java code we're done */
	JavaCallVoid(env, "processedDestroyed", "()V", NULL);
}

static void JNICALL java_onGotFocus(JNIEnv* env, jobject o) {
	Platform_LogConst("APP - GOT FOCUS");
	WindowInfo.Focused = true;
	Event_RaiseVoid(&WindowEvents.FocusChanged);
}

static void JNICALL java_onLostFocus(JNIEnv* env, jobject o) {
	Platform_LogConst("APP - LOST FOCUS");
	WindowInfo.Focused = false;
	Event_RaiseVoid(&WindowEvents.FocusChanged);
	/* TODO: Disable rendering? */
}

static void JNICALL java_onLowMemory(JNIEnv* env, jobject o) {
	Platform_LogConst("APP - LOW MEM");
	/* TODO: Low memory */
}

static const JNINativeMethod methods[19] = {
	{ "processKeyDown",   "(I)V", java_processKeyDown },
	{ "processKeyUp",     "(I)V", java_processKeyUp },
	{ "processKeyChar",   "(I)V", java_processKeyChar },
	{ "processKeyText",   "(Ljava/lang/String;)V", java_processKeyText },

	{ "processMouseDown", "(III)V", java_processMouseDown },
	{ "processMouseUp",   "(III)V", java_processMouseUp },
	{ "processMouseMove", "(III)V", java_processMouseMove },

	{ "processSurfaceCreated",      "(Landroid/view/Surface;)V",  java_processSurfaceCreated },
	{ "processSurfaceDestroyed",    "()V",                        java_processSurfaceDestroyed },
	{ "processSurfaceResized",      "(Landroid/view/Surface;)V",  java_processSurfaceResized },
	{ "processSurfaceRedrawNeeded", "()V",                        java_processSurfaceRedrawNeeded },

	{ "processOnStart",   "()V", java_onStart },
	{ "processOnStop",    "()V", java_onStop },
	{ "processOnResume",  "()V", java_onResume },
	{ "processOnPause",   "()V", java_onPause },
	{ "processOnDestroy", "()V", java_onDestroy },

	{ "processOnGotFocus",  "()V", java_onGotFocus },
	{ "processOnLostFocus", "()V", java_onLostFocus },
	{ "processOnLowMemory", "()V", java_onLowMemory }
};

void Window_Init(void) {
	JNIEnv* env;
	/* TODO: ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_FULLSCREEN, 0); */
	JavaGetCurrentEnv(env);
	JavaRegisterNatives(env, methods);

	WindowInfo.SoftKeyboard = SOFT_KEYBOARD_RESIZE;
	Input_TouchMode         = true;
	DisplayInfo.Depth  = 32;
	DisplayInfo.ScaleX = JavaCallFloat(env, "getDpiX", "()F", NULL);
	DisplayInfo.ScaleY = JavaCallFloat(env, "getDpiY", "()F", NULL);
}

void Window_Create(int width, int height) {
	WindowInfo.Exists = true;
	/* actual window creation is done when processSurfaceCreated is received */
}

static cc_bool winCreated;
static void OnWindowCreated(void* obj) { winCreated = true; }
cc_bool Window_RemakeSurface(void) {
	JNIEnv* env;
	JavaGetCurrentEnv(env);
	winCreated = false;

	/* Force window to be destroyed and re-created */
	/* (see comments in setupForGame for why this has to be done) */
	JavaCallVoid(env, "setupForGame", "()V", NULL);
	Event_Register_(&WindowEvents.Created, NULL, OnWindowCreated);
	Platform_LogConst("Entering wait for window exist loop..");

	/* Loop until window gets created async */
	while (WindowInfo.Exists && !winCreated) {
		Window_ProcessEvents();
		Thread_Sleep(10);
	}

	Platform_LogConst("OK window created..");
	Event_Unregister_(&WindowEvents.Created, NULL, OnWindowCreated);
	return winCreated;
}

void Window_SetTitle(const cc_string* title) {
	/* TODO: Implement this somehow */
	/* Maybe https://stackoverflow.com/questions/2198410/how-to-change-title-of-activity-in-android */
}

void Clipboard_GetText(cc_string* value) {
	JavaCall_Void_String("getClipboardText", value);
}
void Clipboard_SetText(const cc_string* value) {
	JavaCall_String_Void("setClipboardText", value);
}

void Window_Show(void) { } /* Window already visible */
int Window_GetWindowState(void) { 
	JNIEnv* env;
	JavaGetCurrentEnv(env);
	return JavaCallInt(env, "getWindowState", "()I", NULL);
}

cc_result Window_EnterFullscreen(void) {
	JNIEnv* env;
	JavaGetCurrentEnv(env);
	JavaCallVoid(env, "enterFullscreen", "()V", NULL);
	return 0; 
}

cc_result Window_ExitFullscreen(void) {
	JNIEnv* env;
	JavaGetCurrentEnv(env);
	JavaCallVoid(env, "exitFullscreen", "()V", NULL);
	return 0; 
}

void Window_SetSize(int width, int height) { }

void Window_Close(void) {
	WindowInfo.Exists = false;
	Event_RaiseVoid(&WindowEvents.Closing);
	/* TODO: Do we need to call finish here */
	/* ANativeActivity_finish(app->activity); */
}

void Window_ProcessEvents(void) {
	JNIEnv* env;
	JavaGetCurrentEnv(env);
	/* TODO: Cache the java env and cache the method ID!!!!! */
	JavaCallVoid(env, "processEvents", "()V", NULL);
}

/* No actual mouse cursor */
static void Cursor_GetRawPos(int* x, int* y) { *x = 0; *y = 0; }
void Cursor_SetPosition(int x, int y) { }
static void Cursor_DoSetVisible(cc_bool visible) { }

static void ShowDialogCore(const char* title, const char* msg) {
	JNIEnv* env;
	jvalue args[2];
	JavaGetCurrentEnv(env);

	Platform_LogConst(title);
	Platform_LogConst(msg);

	args[0].l = JavaMakeConst(env, title);
	args[1].l = JavaMakeConst(env, msg);
	JavaCallVoid(env, "showAlert", "(Ljava/lang/String;Ljava/lang/String;)V", args);
	(*env)->DeleteLocalRef(env, args[0].l);
	(*env)->DeleteLocalRef(env, args[1].l);
}

static struct Bitmap fb_bmp;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	fb_bmp     = *bmp;
}

void Window_DrawFramebuffer(Rect2D r) {
	ANativeWindow_Buffer buffer;
	cc_uint32* src;
	cc_uint32* dst;
	ARect b;
	int y, res, size;

	/* window not created yet */
	if (!win_handle) return;
	b.left = r.X; b.right  = r.X + r.Width;
	b.top  = r.Y; b.bottom = r.Y + r.Height;

	/* Platform_Log4("DIRTY: %i,%i - %i,%i", &b.left, &b.top, &b.right, &b.bottom); */
	res  = ANativeWindow_lock(win_handle, &buffer, &b);
	if (res) Logger_Abort2(res, "Locking window pixels");
	/* Platform_Log4("ADJUS: %i,%i - %i,%i", &b.left, &b.top, &b.right, &b.bottom); */

	/* In some rare cases, the returned locked region will be entire area of the surface */
	/* This can cause a crash if the surface has been resized (i.e. device rotated), */
	/* but the framebuffer has not been resized yet. So always constrain bounds. */
	b.left = min(b.left, fb_bmp.width);  b.right  = min(b.right,  fb_bmp.width);
	b.top  = min(b.top,  fb_bmp.height); b.bottom = min(b.bottom, fb_bmp.height);

	src  = (cc_uint32*)fb_bmp.scan0 + b.left;
	dst  = (cc_uint32*)buffer.bits  + b.left;
	size = (b.right - b.left) * 4;

	for (y = b.top; y < b.bottom; y++) {
		Mem_Copy(dst + y * buffer.stride, src + y * fb_bmp.width, size);
	}
	res = ANativeWindow_unlockAndPost(win_handle);
	if (res) Logger_Abort2(res, "Unlocking window pixels");
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}

void Window_OpenKeyboard(const struct OpenKeyboardArgs* args) {
	JNIEnv* env;
	jvalue args[2];
	JavaGetCurrentEnv(env);

	args[0].l = JavaMakeString(env, args->text);
	args[1].i = args->type;
	JavaCallVoid(env, "openKeyboard", "(Ljava/lang/String;I)V", args);
	(*env)->DeleteLocalRef(env, args[0].l);
}

void Window_SetKeyboardText(const cc_string* text) {
	JNIEnv* env;
	jvalue args[1];
	JavaGetCurrentEnv(env);

	args[0].l = JavaMakeString(env, text);
	JavaCallVoid(env, "setKeyboardText", "(Ljava/lang/String;)V", args);
	(*env)->DeleteLocalRef(env, args[0].l);
}

void Window_CloseKeyboard(void) {
	JNIEnv* env;
	JavaGetCurrentEnv(env);
	JavaCallVoid(env, "closeKeyboard", "()V", NULL);
}

void Window_EnableRawMouse(void)  { DefaultEnableRawMouse(); }
void Window_UpdateRawMouse(void)  { }
void Window_DisableRawMouse(void) { DefaultDisableRawMouse(); }
#endif



#ifdef CC_BUILD_GL
/* OpenGL contexts are heavily tied to the window, so for simplicitly are also included here */
/* SDL and EGL are platform agnostic, other OpenGL context backends are tied to one windowing system. */
#define GLCONTEXT_DEFAULT_DEPTH 24
#define GLContext_IsInvalidAddress(ptr) (ptr == (void*)0 || ptr == (void*)1 || ptr == (void*)-1 || ptr == (void*)2)

void GLContext_GetAll(const struct DynamicLibSym* syms, int count) {
	int i;
	for (i = 0; i < count; i++) {
		*syms[i].symAddr = GLContext_GetAddress(syms[i].name);
	}
}

/*########################################################################################################################*
*-------------------------------------------------------SDL OpenGL--------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_SDL
static SDL_GLContext win_ctx;

void GLContext_Create(void) {
	struct GraphicsMode mode;
	InitGraphicsMode(&mode);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   mode.R);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, mode.G);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  mode.B);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, mode.A);

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   GLCONTEXT_DEFAULT_DEPTH);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, true);

	win_ctx = SDL_GL_CreateContext(win_handle);
	if (!win_ctx) Window_SDLFail("creating OpenGL context");
}

void GLContext_Update(void) { }
cc_bool GLContext_TryRestore(void) { return true; }
void GLContext_Free(void) {
	SDL_GL_DeleteContext(win_ctx);
	win_ctx = NULL;
}

void* GLContext_GetAddress(const char* function) {
	return SDL_GL_GetProcAddress(function);
}

cc_bool GLContext_SwapBuffers(void) {
	SDL_GL_SwapWindow(win_handle);
	return true;
}

void GLContext_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	SDL_GL_SetSwapInterval(vsync);
}
void GLContext_GetApiInfo(cc_string* info) { }


/*########################################################################################################################*
*-------------------------------------------------------EGL OpenGL--------------------------------------------------------*
*#########################################################################################################################*/
#elif defined CC_BUILD_EGL
#include <EGL/egl.h>
static EGLDisplay ctx_display;
static EGLContext ctx_context;
static EGLSurface ctx_surface;
static EGLConfig ctx_config;
static EGLint ctx_numConfig;

#ifdef CC_BUILD_X11
static XVisualInfo GLContext_SelectVisual(void) {
	XVisualInfo info;
	cc_result res;
	int screen = DefaultScreen(win_display);

	res = XMatchVisualInfo(win_display, screen, 24, TrueColor, &info) ||
		  XMatchVisualInfo(win_display, screen, 32, TrueColor, &info);

	if (!res) Logger_Abort("Selecting visual");
	return info;
}
#endif

static void GLContext_InitSurface(void) {
	if (!win_handle) return; /* window not created or lost */
	ctx_surface = eglCreateWindowSurface(ctx_display, ctx_config, win_handle, NULL);

	if (!ctx_surface) return;
	eglMakeCurrent(ctx_display, ctx_surface, ctx_surface, ctx_context);
}

static void GLContext_FreeSurface(void) {
	if (!ctx_surface) return;
	eglMakeCurrent(ctx_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(ctx_display, ctx_surface);
	ctx_surface = NULL;
}

void GLContext_Create(void) {
	static EGLint contextAttribs[3] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	static EGLint attribs[19] = {
		EGL_RED_SIZE,  0, EGL_GREEN_SIZE,  0,
		EGL_BLUE_SIZE, 0, EGL_ALPHA_SIZE,  0,
		EGL_DEPTH_SIZE,        GLCONTEXT_DEFAULT_DEPTH,
		EGL_STENCIL_SIZE,      0,
		EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
		EGL_RENDERABLE_TYPE,   EGL_OPENGL_ES2_BIT,
		EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
		EGL_NONE
	};

	struct GraphicsMode mode;
	InitGraphicsMode(&mode);
	attribs[1] = mode.R; attribs[3] = mode.G;
	attribs[5] = mode.B; attribs[7] = mode.A;

	ctx_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(ctx_display, NULL, NULL);
	eglBindAPI(EGL_OPENGL_ES_API);
	eglChooseConfig(ctx_display, attribs, &ctx_config, 1, &ctx_numConfig);

	ctx_context = eglCreateContext(ctx_display, ctx_config, EGL_NO_CONTEXT, contextAttribs);
	GLContext_InitSurface();
}

void GLContext_Update(void) {
	GLContext_FreeSurface();
	GLContext_InitSurface();
}

cc_bool GLContext_TryRestore(void) {
	GLContext_FreeSurface();
	GLContext_InitSurface();
	return ctx_surface != NULL;
}

void GLContext_Free(void) {
	GLContext_FreeSurface();
	eglDestroyContext(ctx_display, ctx_context);
	eglTerminate(ctx_display);
}

void* GLContext_GetAddress(const char* function) {
	return eglGetProcAddress(function);
}

cc_bool GLContext_SwapBuffers(void) {
	EGLint err;
	if (!ctx_surface) return false;
	if (eglSwapBuffers(ctx_display, ctx_surface)) return true;

	err = eglGetError();
	/* TODO: figure out what errors need to be handled here */
	Logger_Abort2(err, "Failed to swap buffers");
	return false;
}

void GLContext_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	eglSwapInterval(ctx_display, vsync);
}
void GLContext_GetApiInfo(cc_string* info) { }


/*########################################################################################################################*
*-------------------------------------------------------WGL OpenGL--------------------------------------------------------*
*#########################################################################################################################*/
#elif defined CC_BUILD_WINGUI
static HGLRC ctx_handle;
static HDC ctx_DC;
typedef BOOL (WINAPI *FP_SWAPINTERVAL)(int interval);
static FP_SWAPINTERVAL wglSwapIntervalEXT;

static void GLContext_SelectGraphicsMode(struct GraphicsMode* mode) {
	PIXELFORMATDESCRIPTOR pfd = { 0 };
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	/* TODO: PFD_SUPPORT_COMPOSITION FLAG? CHECK IF IT WORKS ON XP */
	pfd.cColorBits = mode->R + mode->G + mode->B;
	pfd.cDepthBits = GLCONTEXT_DEFAULT_DEPTH;

	pfd.iPixelType = mode->IsIndexed ? PFD_TYPE_COLORINDEX : PFD_TYPE_RGBA;
	pfd.cRedBits   = mode->R;
	pfd.cGreenBits = mode->G;
	pfd.cBlueBits  = mode->B;
	pfd.cAlphaBits = mode->A;

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

void GLContext_Create(void) {
	struct GraphicsMode mode;
	InitGraphicsMode(&mode);
	GLContext_SelectGraphicsMode(&mode);

	ctx_handle = wglCreateContext(win_DC);
	if (!ctx_handle) ctx_handle = wglCreateContext(win_DC);

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

void* GLContext_GetAddress(const char* function) {
	void* addr = (void*)wglGetProcAddress(function);
	return GLContext_IsInvalidAddress(addr) ? NULL : addr;
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


/*########################################################################################################################*
*-------------------------------------------------------glX OpenGL--------------------------------------------------------*
*#########################################################################################################################*/
#elif defined CC_BUILD_X11
#include <GL/glx.h>
static GLXContext ctx_handle;
typedef int  (*FP_SWAPINTERVAL)(int interval);
typedef Bool (*FP_QUERYRENDERER)(int attribute, unsigned int* value);
static FP_SWAPINTERVAL swapIntervalMESA, swapIntervalSGI;
static FP_QUERYRENDERER queryRendererMESA;

void GLContext_Create(void) {
	static const cc_string vsync_mesa = String_FromConst("GLX_MESA_swap_control");
	static const cc_string vsync_sgi  = String_FromConst("GLX_SGI_swap_control");
	static const cc_string info_mesa  = String_FromConst("GLX_MESA_query_renderer");

	const char* raw_exts;
	cc_string exts;
	ctx_handle = glXCreateContext(win_display, &win_visual, NULL, true);

	if (!ctx_handle) {
		Platform_LogConst("Context create failed. Trying indirect...");
		ctx_handle = glXCreateContext(win_display, &win_visual, NULL, false);
	}
	if (!ctx_handle) Logger_Abort("Failed to create OpenGL context");

	if (!glXIsDirect(win_display, ctx_handle)) {
		Platform_LogConst("== WARNING: Context is not direct ==");
	}
	if (!glXMakeCurrent(win_display, win_handle, ctx_handle)) {
		Logger_Abort("Failed to make OpenGL context current.");
	}

	/* GLX may return non-null function pointers that don't actually work */
	/* So we need to manually check the extensions string for support */
	raw_exts = glXQueryExtensionsString(win_display, DefaultScreen(win_display));
	exts = String_FromReadonly(raw_exts);

	if (String_CaselessContains(&exts, &vsync_mesa)) {
		swapIntervalMESA  = (FP_SWAPINTERVAL)GLContext_GetAddress("glXSwapIntervalMESA");
	}
	if (String_CaselessContains(&exts, &vsync_sgi)) {
		swapIntervalSGI   = (FP_SWAPINTERVAL)GLContext_GetAddress("glXSwapIntervalSGI");
	}
	if (String_CaselessContains(&exts, &info_mesa)) {
		queryRendererMESA = (FP_QUERYRENDERER)GLContext_GetAddress("glXQueryCurrentRendererIntegerMESA");
	}
}

void GLContext_Update(void) { }
cc_bool GLContext_TryRestore(void) { return true; }
void GLContext_Free(void) {
	if (!ctx_handle) return;
	glXMakeCurrent(win_display, None, NULL);
	glXDestroyContext(win_display, ctx_handle);
	ctx_handle = NULL;
}

void* GLContext_GetAddress(const char* function) {
	void* addr = (void*)glXGetProcAddress((const GLubyte*)function);
	return GLContext_IsInvalidAddress(addr) ? NULL : addr;
}

cc_bool GLContext_SwapBuffers(void) {
	glXSwapBuffers(win_display, win_handle);
	return true;
}

void GLContext_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	int res = 0;
	if (swapIntervalMESA) {
		res = swapIntervalMESA(vsync);
	} else if (swapIntervalSGI) {
		res = swapIntervalSGI(vsync);
	}
	if (res) Platform_Log1("Set VSync failed, error: %i", &res);
}

void GLContext_GetApiInfo(cc_string* info) {
	unsigned int vram, acc;
	if (!queryRendererMESA) return;

	queryRendererMESA(0x8186, &acc);
	queryRendererMESA(0x8187, &vram);
	String_Format2(info, "VRAM: %i MB, %c", &vram,
		acc ? "HW accelerated" : "no HW acceleration");
}

static void GetAttribs(struct GraphicsMode* mode, int* attribs, int depth) {
	int i = 0;
	/* See http://www-01.ibm.com/support/knowledgecenter/ssw_aix_61/com.ibm.aix.opengl/doc/openglrf/glXChooseFBConfig.htm%23glxchoosefbconfig */
	/* See http://www-01.ibm.com/support/knowledgecenter/ssw_aix_71/com.ibm.aix.opengl/doc/openglrf/glXChooseVisual.htm%23b5c84be452rree */
	/* for the attribute declarations. Note that the attributes are different than those used in glxChooseVisual */

	if (!mode->IsIndexed) { attribs[i++] = GLX_RGBA; }
	attribs[i++] = GLX_RED_SIZE;   attribs[i++] = mode->R;
	attribs[i++] = GLX_GREEN_SIZE; attribs[i++] = mode->G;
	attribs[i++] = GLX_BLUE_SIZE;  attribs[i++] = mode->B;
	attribs[i++] = GLX_ALPHA_SIZE; attribs[i++] = mode->A;
	attribs[i++] = GLX_DEPTH_SIZE; attribs[i++] = depth;

	attribs[i++] = GLX_DOUBLEBUFFER;
	attribs[i++] = 0;
}

static XVisualInfo GLContext_SelectVisual(void) {
	int attribs[20];
	int major, minor;
	XVisualInfo* visual = NULL;

	int fbcount, screen;
	GLXFBConfig* fbconfigs;
	XVisualInfo info;
	struct GraphicsMode mode;

	InitGraphicsMode(&mode);
	GetAttribs(&mode, attribs, GLCONTEXT_DEFAULT_DEPTH);
	if (!glXQueryVersion(win_display, &major, &minor)) {
		Logger_Abort("glXQueryVersion failed");
	}
	screen = DefaultScreen(win_display);

	if (major >= 1 && minor >= 3) {
		/* ChooseFBConfig returns an array of GLXFBConfig opaque structures */
		fbconfigs = glXChooseFBConfig(win_display, screen, attribs, &fbcount);
		if (fbconfigs && fbcount) {
			/* Use the first GLXFBConfig from the fbconfigs array (best match) */
			visual = glXGetVisualFromFBConfig(win_display, *fbconfigs);
			XFree(fbconfigs);
		}
	}

	if (!visual) {
		Platform_LogConst("Falling back to glXChooseVisual.");
		visual = glXChooseVisual(win_display, screen, attribs);
	}
	/* Some really old devices will only supply 16 bit depths */
	if (!visual) {
		GetAttribs(&mode, attribs, 16);
		visual = glXChooseVisual(win_display, screen, attribs);
	}
	if (!visual) Logger_Abort("Requested GraphicsMode not available.");

	info = *visual;
	XFree(visual);
	return info;
}


/*########################################################################################################################*
*-------------------------------------------------------AGL OpenGL--------------------------------------------------------*
*#########################################################################################################################*/
#elif defined CC_BUILD_CARBON
#include <AGL/agl.h>

static AGLContext ctx_handle;
static cc_bool ctx_firstFullscreen;
static int ctx_windowWidth, ctx_windowHeight;

static void GLContext_Check(int code, const char* place) {
	cc_result res;
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

static void GLContext_GetAttribs(struct GraphicsMode* mode, GLint* attribs, cc_bool fullscreen) {
	int i = 0;

	if (!mode->IsIndexed) { attribs[i++] = AGL_RGBA; }
	attribs[i++] = AGL_RED_SIZE;   attribs[i++] = mode->R;
	attribs[i++] = AGL_GREEN_SIZE; attribs[i++] = mode->G;
	attribs[i++] = AGL_BLUE_SIZE;  attribs[i++] = mode->B;
	attribs[i++] = AGL_ALPHA_SIZE; attribs[i++] = mode->A;
	attribs[i++] = AGL_DEPTH_SIZE; attribs[i++] = GLCONTEXT_DEFAULT_DEPTH;

	attribs[i++] = AGL_DOUBLEBUFFER;
	if (fullscreen) { attribs[i++] = AGL_FULLSCREEN; }
	attribs[i++] = 0;
}

static cc_result GLContext_UnsetFullscreen(void) {
	int code;
	Platform_LogConst("Unsetting fullscreen.");

	code = aglSetDrawable(ctx_handle, NULL);
	if (!code) return aglGetError();
	/* TODO: I don't think this is necessary */
	code = aglUpdateContext(ctx_handle);
	if (!code) return aglGetError();

	CGDisplayRelease(CGMainDisplayID());
	GLContext_SetDrawable();

	win_fullscreen = false;
	/* TODO: Eliminate this if possible */
	Window_SetSize(ctx_windowWidth, ctx_windowHeight);
	return 0;
}

static cc_result GLContext_SetFullscreen(void) {
	int width  = DisplayInfo.Width;
	int height = DisplayInfo.Height;
	int code;

	Platform_LogConst("Switching to fullscreen");
	/* TODO: Does aglSetFullScreen capture the screen anyways? */
	CGDisplayCapture(CGMainDisplayID());

	if (!aglSetFullScreen(ctx_handle, width, height, 0, 0)) {
		code = aglGetError();
		GLContext_UnsetFullscreen();
		return code;
	}
	/* TODO: Do we really need to call this? */
	GLContext_MakeCurrent();

	/* This is a weird hack to workaround a bug where the first time a context */
	/* is made fullscreen, we just end up with a blank screen.  So we undo it as fullscreen */
	/* and redo it as fullscreen. */
	/* TODO: We really should'd need to do this. Need to debug on real hardware. */
	if (!ctx_firstFullscreen) {
		ctx_firstFullscreen = true;
		GLContext_UnsetFullscreen();
		return GLContext_SetFullscreen();
	}

	win_fullscreen   = true;
	ctx_windowWidth  = WindowInfo.Width;
	ctx_windowHeight = WindowInfo.Height;

	windowX = DisplayInfo.X; WindowInfo.Width  = DisplayInfo.Width;
	windowY = DisplayInfo.Y; WindowInfo.Height = DisplayInfo.Height;
	return 0;
}

void GLContext_Create(void) {
	GLint attribs[20];
	AGLPixelFormat fmt;
	GDHandle gdevice;
	OSStatus res;
	struct GraphicsMode mode;
	InitGraphicsMode(&mode);

	/* Initially try creating fullscreen compatible context */	
	res = DMGetGDeviceByDisplayID(CGMainDisplayID(), &gdevice, false);
	if (res) Logger_Abort2(res, "Getting display device failed");

	GLContext_GetAttribs(&mode, attribs, true);
	fmt = aglChoosePixelFormat(&gdevice, 1, attribs);
	res = aglGetError();

	/* Try again with non-compatible context if that fails */
	if (!fmt || res == AGL_BAD_PIXELFMT) {
		Platform_LogConst("Failed to create full screen pixel format.");
		Platform_LogConst("Trying again to create a non-fullscreen pixel format.");

		GLContext_GetAttribs(&mode, attribs, false);
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
	if (win_fullscreen) return;
	GLContext_SetDrawable();
	aglUpdateContext(ctx_handle);
}
cc_bool GLContext_TryRestore(void) { return true; }

void GLContext_Free(void) {
	if (!ctx_handle) return;
	aglSetCurrentContext(NULL);
	aglDestroyContext(ctx_handle);
	ctx_handle = NULL;
}

void* GLContext_GetAddress(const char* function) {
	static const cc_string glPath = String_FromConst("/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL");
	static void* lib;
	void* addr;

	if (!lib) lib = DynamicLib_Load2(&glPath);
	addr = DynamicLib_Get2(lib, function);
	return GLContext_IsInvalidAddress(addr) ? NULL : addr;
}

cc_bool GLContext_SwapBuffers(void) {
	aglSwapBuffers(ctx_handle);
	GLContext_Check(0, "Swapping buffers");
	return true;
}

void GLContext_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	int value = vsync ? 1 : 0;
	aglSetInteger(ctx_handle, AGL_SWAP_INTERVAL, &value);
}
void GLContext_GetApiInfo(cc_string* info) { }


/*########################################################################################################################*
*--------------------------------------------------------NSOpenGL---------------------------------------------------------*
*#########################################################################################################################*/
#elif defined CC_BUILD_COCOA
#define NSOpenGLPFADoubleBuffer 5
#define NSOpenGLPFAColorSize    8
#define NSOpenGLPFADepthSize    12
#define NSOpenGLPFAFullScreen   54
#define NSOpenGLContextParameterSwapInterval 222

static id ctxHandle;
static id MakePixelFormat(struct GraphicsMode* mode, cc_bool fullscreen) {
	id fmt;
	uint32_t attribs[7] = {
		NSOpenGLPFAColorSize,    0,
		NSOpenGLPFADepthSize,    GLCONTEXT_DEFAULT_DEPTH,
		NSOpenGLPFADoubleBuffer, 0, 0
	};

	attribs[1] = mode->R + mode->G + mode->B + mode->A;
	attribs[5] = fullscreen ? NSOpenGLPFAFullScreen : 0;
	fmt = objc_msgSend((id)objc_getClass("NSOpenGLPixelFormat"), sel_registerName("alloc"));
	return objc_msgSend(fmt, sel_registerName("initWithAttributes:"), attribs);
}

void GLContext_Create(void) {
	struct GraphicsMode mode;
	id view, fmt;

	InitGraphicsMode(&mode);
	fmt = MakePixelFormat(&mode, true);
	if (!fmt) {
		Platform_LogConst("Failed to create full screen pixel format.");
		Platform_LogConst("Trying again to create a non-fullscreen pixel format.");
		fmt = MakePixelFormat(&mode, false);
	}
	if (!fmt) Logger_Abort("Choosing pixel format");

	ctxHandle = objc_msgSend((id)objc_getClass("NSOpenGLContext"), sel_registerName("alloc"));
	ctxHandle = objc_msgSend(ctxHandle, sel_registerName("initWithFormat:shareContext:"), fmt, NULL);
	if (!ctxHandle) Logger_Abort("Failed to create OpenGL context");

	objc_msgSend(ctxHandle, sel_registerName("setView:"), viewHandle);
	objc_msgSend(fmt,       sel_registerName("release"));
	objc_msgSend(ctxHandle, sel_registerName("makeCurrentContext"));
	objc_msgSend(ctxHandle, selUpdate);
}

void GLContext_Update(void) {
	// TODO: Why does this crash on resizing
	objc_msgSend(ctxHandle, selUpdate);
}
cc_bool GLContext_TryRestore(void) { return true; }

void GLContext_Free(void) { 
	objc_msgSend((id)objc_getClass("NSOpenGLContext"), sel_registerName("clearCurrentContext"));
	objc_msgSend(ctxHandle, sel_registerName("clearDrawable"));
	objc_msgSend(ctxHandle, sel_registerName("release"));
}

void* GLContext_GetAddress(const char* function) {
	static const cc_string glPath = String_FromConst("/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL");
	static void* lib;
	void* addr;

	if (!lib) lib = DynamicLib_Load2(&glPath);
	addr = DynamicLib_Get2(lib, function);
	return GLContext_IsInvalidAddress(addr) ? NULL : addr;
}

cc_bool GLContext_SwapBuffers(void) {
	objc_msgSend(ctxHandle, selFlushBuffer);
	return true;
}

void GLContext_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	int value = vsync ? 1 : 0;
	objc_msgSend(ctxHandle, sel_registerName("setValues:forParameter:"), &value, NSOpenGLContextParameterSwapInterval);
}
void GLContext_GetApiInfo(cc_string* info) { }


/*########################################################################################################################*
*------------------------------------------------Emscripten WebGL context-------------------------------------------------*
*#########################################################################################################################*/
#elif defined CC_BUILD_WEB
#include "Graphics.h"
static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx_handle;

static EM_BOOL GLContext_OnLost(int eventType, const void *reserved, void *userData) {
	Gfx_LoseContext("WebGL context lost");
	return 1;
}

void GLContext_Create(void) {
	EmscriptenWebGLContextAttributes attribs;
	emscripten_webgl_init_context_attributes(&attribs);
	attribs.alpha     = false;
	attribs.depth     = true;
	attribs.stencil   = false;
	attribs.antialias = false;

	ctx_handle = emscripten_webgl_create_context("#canvas", &attribs);
	if (!ctx_handle) Window_ShowDialog("WebGL unsupported", "WebGL is required to run ClassiCube");

	emscripten_webgl_make_context_current(ctx_handle);
	emscripten_set_webglcontextlost_callback("#canvas", NULL, 0, GLContext_OnLost);
}

void GLContext_Update(void) {
	/* TODO: do we need to do something here.... ? */
}
cc_bool GLContext_TryRestore(void) {
	return !emscripten_is_webgl_context_lost(0);
}

void GLContext_Free(void) {
	emscripten_webgl_destroy_context(ctx_handle);
	emscripten_set_webglcontextlost_callback("#canvas", NULL, 0, NULL);
}

void* GLContext_GetAddress(const char* function) { return NULL; }
cc_bool GLContext_SwapBuffers(void) { return true; /* Browser implicitly does this */ }

void GLContext_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	if (vsync) {
		emscripten_set_main_loop_timing(EM_TIMING_RAF, 1);
	} else {
		emscripten_set_main_loop_timing(EM_TIMING_SETTIMEOUT, (int)minFrameMs);
	}
}

void GLContext_GetApiInfo(cc_string* info) { 
	char buffer[NATIVE_STR_LEN];
	int len;

	EM_ASM_({
		var dbg = GLctx.getExtension('WEBGL_debug_renderer_info');
		var str = dbg ? GLctx.getParameter(dbg.UNMASKED_RENDERER_WEBGL) : "";
		stringToUTF8(str, $0, $1);
	}, buffer, NATIVE_STR_LEN);

	len = String_CalcLen(buffer, NATIVE_STR_LEN);
	if (!len) return;
	String_AppendConst(info, "GPU: ");
	String_AppendUtf8(info, buffer, len);
}
#endif
#endif
