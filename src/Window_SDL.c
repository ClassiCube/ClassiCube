#include "Core.h"
#if defined CC_BUILD_SDL
#include "_WindowBase.h"
#include "Graphics.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Errors.h"
#include <SDL2/SDL.h>
static SDL_Window* win_handle;

#error "Some features are missing from the SDL backend. If possible, it is recommended that you use a native windowing backend instead"

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

static void DoCreateWindow(int width, int height, int flags) {
	int x = Display_CentreX(width);
	int y = Display_CentreY(height);

	win_handle = SDL_CreateWindow(NULL, x, y, width, height, 
					flags | SDL_WINDOW_RESIZABLE);
	if (!win_handle) Window_SDLFail("creating window");

	RefreshWindowBounds();
	WindowInfo.Exists = true;
	WindowInfo.Handle = win_handle;
	/* TODO grab using SDL_SetWindowGrab? seems to be unnecessary on Linux at least */
}
void Window_Create2D(int width, int height) { DoCreateWindow(width, height, 0); }
void Window_Create3D(int width, int height) { DoCreateWindow(width, height, SDL_WINDOW_OPENGL); }

void Window_SetTitle(const cc_string* title) {
	char str[NATIVE_STR_LEN];
	String_EncodeUtf8(str, title);
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
	String_EncodeUtf8(str, value);
	SDL_SetClipboardText(str);
}

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

int Window_IsObscured(void) { return 0; }

void Window_Show(void) { SDL_ShowWindow(win_handle); }

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
	if (k >= SDLK_F1  && k <= SDLK_F12) { return CCKEY_F1  + (k - SDLK_F1); }
	if (k >= SDLK_F13 && k <= SDLK_F24) { return CCKEY_F13 + (k - SDLK_F13); }
	/* SDLK_KP_0 isn't before SDLK_KP_1 */
	if (k >= SDLK_KP_1 && k <= SDLK_KP_9) { return CCKEY_KP1 + (k - SDLK_KP_1); }

	switch (k) {
		case SDLK_RETURN: return CCKEY_ENTER;
		case SDLK_ESCAPE: return CCKEY_ESCAPE;
		case SDLK_BACKSPACE: return CCKEY_BACKSPACE;
		case SDLK_TAB:    return CCKEY_TAB;
		case SDLK_SPACE:  return CCKEY_SPACE;
		case SDLK_QUOTE:  return CCKEY_QUOTE;
		case SDLK_EQUALS: return CCKEY_EQUALS;
		case SDLK_COMMA:  return CCKEY_COMMA;
		case SDLK_MINUS:  return CCKEY_MINUS;
		case SDLK_PERIOD: return CCKEY_PERIOD;
		case SDLK_SLASH:  return CCKEY_SLASH;
		case SDLK_SEMICOLON:    return CCKEY_SEMICOLON;
		case SDLK_LEFTBRACKET:  return CCKEY_LBRACKET;
		case SDLK_BACKSLASH:    return CCKEY_BACKSLASH;
		case SDLK_RIGHTBRACKET: return CCKEY_RBRACKET;
		case SDLK_BACKQUOTE:    return CCKEY_TILDE;
		case SDLK_CAPSLOCK:     return CCKEY_CAPSLOCK;
		case SDLK_PRINTSCREEN: return CCKEY_PRINTSCREEN;
		case SDLK_SCROLLLOCK:  return CCKEY_SCROLLLOCK;
		case SDLK_PAUSE:       return CCKEY_PAUSE;
		case SDLK_INSERT:   return CCKEY_INSERT;
		case SDLK_HOME:     return CCKEY_HOME;
		case SDLK_PAGEUP:   return CCKEY_PAGEUP;
		case SDLK_DELETE:   return CCKEY_DELETE;
		case SDLK_END:      return CCKEY_END;
		case SDLK_PAGEDOWN: return CCKEY_PAGEDOWN;
		case SDLK_RIGHT: return CCKEY_RIGHT;
		case SDLK_LEFT:  return CCKEY_LEFT;
		case SDLK_DOWN:  return CCKEY_DOWN;
		case SDLK_UP:    return CCKEY_UP;

		case SDLK_NUMLOCKCLEAR: return CCKEY_NUMLOCK;
		case SDLK_KP_DIVIDE: return CCKEY_KP_DIVIDE;
		case SDLK_KP_MULTIPLY: return CCKEY_KP_MULTIPLY;
		case SDLK_KP_MINUS: return CCKEY_KP_MINUS;
		case SDLK_KP_PLUS: return CCKEY_KP_PLUS;
		case SDLK_KP_ENTER: return CCKEY_KP_ENTER;
		case SDLK_KP_0: return CCKEY_KP0;
		case SDLK_KP_PERIOD: return CCKEY_KP_DECIMAL;

		case SDLK_LCTRL: return CCKEY_LCTRL;
		case SDLK_LSHIFT: return CCKEY_LSHIFT;
		case SDLK_LALT: return CCKEY_LALT;
		case SDLK_LGUI: return CCKEY_LWIN;
		case SDLK_RCTRL: return CCKEY_RCTRL;
		case SDLK_RSHIFT: return CCKEY_RSHIFT;
		case SDLK_RALT: return CCKEY_RALT;
		case SDLK_RGUI: return CCKEY_RWIN;
	}
	return INPUT_NONE;
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
		case SDL_BUTTON_LEFT:   btn = CCMOUSE_L; break;
		case SDL_BUTTON_MIDDLE: btn = CCMOUSE_M; break;
		case SDL_BUTTON_RIGHT:  btn = CCMOUSE_R; break;
		case SDL_BUTTON_X1:     btn = CCMOUSE_X1; break;
		case SDL_BUTTON_X2:     btn = CCMOUSE_X2; break;
		default: return;
	}
	Input_Set(btn, pressed);
}

static void OnTextEvent(const SDL_Event* e) {
	cc_codepoint cp;
	const char* src;
	int i, len;

	src = e->text.text;
	len = String_CalcLen(src, SDL_TEXTINPUTEVENT_TEXT_SIZE);

	while (len > 0) {
		i = Convert_Utf8ToCodepoint(&cp, src, len);
		if (!i) break;

		Event_RaiseInt(&InputEvents.Press, cp);
		src += i; len -= i;
	}
}

static void OnWindowEvent(const SDL_Event* e) {
	switch (e->window.event) {
		case SDL_WINDOWEVENT_EXPOSED:
			Event_RaiseVoid(&WindowEvents.RedrawNeeded);
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

void Window_ProcessEvents(double delta) {
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
			if (Input.RawMode) Event_RaiseRawMove(&PointerEvents.RawMoved, e.motion.xrel, e.motion.yrel);
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

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

static SDL_Surface* win_surface;
static SDL_Surface* blit_surface;

void Window_AllocFramebuffer(struct Bitmap* bmp) {
	SDL_PixelFormat* fmt;
	win_surface = SDL_GetWindowSurface(win_handle);
	if (!win_surface) Window_SDLFail("getting window surface");

	fmt = win_surface->format;
	if (fmt->BitsPerPixel != 32) {
		/* Slow path: e.g. 15 or 16 bit pixels */
		Platform_Log1("Slow color depth: %b bpp", &fmt->BitsPerPixel);
		blit_surface = SDL_CreateRGBSurface(0, win_surface->w, win_surface->h, 32, 0, 0, 0, 0);
		if (!blit_surface) Window_SDLFail("creating blit surface");

		SDL_SetSurfaceBlendMode(blit_surface, SDL_BLENDMODE_NONE);
		bmp->scan0 = blit_surface->pixels;
	} else {
		/* Fast path: 32 bit pixels */
		if (SDL_MUSTLOCK(win_surface)) {
			int ret = SDL_LockSurface(win_surface);
			if (ret < 0) Window_SDLFail("locking window surface");
		}
		bmp->scan0 = win_surface->pixels;
	}
}

void Window_DrawFramebuffer(Rect2D r) {
	SDL_Rect rect;
	rect.x = r.X; rect.w = r.Width;
	rect.y = r.Y; rect.h = r.Height;

	if (blit_surface) SDL_BlitSurface(blit_surface, &rect, win_surface, &rect);
	SDL_UpdateWindowSurfaceRects(win_handle, &rect, 1);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	if (blit_surface) SDL_FreeSurface(blit_surface);
	blit_surface = NULL;

	/* SDL docs explicitly say to NOT free window surface */
	/* https://wiki.libsdl.org/SDL_GetWindowSurface */
	/* TODO: Do we still need to unlock it though? */
}

void Window_OpenKeyboard(struct OpenKeyboardArgs* args) { SDL_StartTextInput(); }
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { SDL_StopTextInput(); }

void Window_EnableRawMouse(void) {
	RegrabMouse();
	SDL_SetRelativeMouseMode(true);
	Input.RawMode = true;
}
void Window_UpdateRawMouse(void) { CentreMousePosition(); }

void Window_DisableRawMouse(void) {
	RegrabMouse();
	SDL_SetRelativeMouseMode(false);
	Input.RawMode = false;
}


/*########################################################################################################################*
*-----------------------------------------------------OpenGL context------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_GL && !defined CC_BUILD_EGL
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
#endif
#endif
