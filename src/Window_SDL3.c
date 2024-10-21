#include "Core.h"
#if CC_WIN_BACKEND == CC_WIN_BACKEND_SDL3
#undef CC_BUILD_EGL /* eglCreateWindowSurface can't use an SDL window */
#include "_WindowBase.h"
#include "Graphics.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Errors.h"
#include <SDL3/SDL.h>
static SDL_Window* win_handle;
static Uint32 dlg_event;

static void RefreshWindowBounds(void) {
	SDL_GetWindowSize(win_handle, &Window_Main.Width, &Window_Main.Height);
}

static void Window_SDLFail(const char* place) {
	char strBuffer[256];
	cc_string str;
	String_InitArray_NT(str, strBuffer);

	String_Format2(&str, "Error when %c: %c", place, SDL_GetError());
	str.buffer[str.length] = '\0';
	Process_Abort(str.buffer);
}

void Window_PreInit(void) {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
	#ifdef CC_BUILD_FLATPAK
	SDL_SetHint(SDL_HINT_APP_ID, "net.classicube.flatpak.client");
	#endif
	DisplayInfo.CursorVisible = true;
}

void Window_Init(void) {
	int displayID = SDL_GetPrimaryDisplay();
	Input.Sources = INPUT_SOURCE_NORMAL;
	
	const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(displayID);
	dlg_event = SDL_RegisterEvents(1);

	DisplayInfo.Width  = mode->w;
	DisplayInfo.Height = mode->h;
	DisplayInfo.Depth  = SDL_BITSPERPIXEL(mode->format);
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
}

void Window_Free(void) { }

#ifdef CC_BUILD_ICON
/* See misc/sdl/sdl_icon_gen.cs for how to generate this file */
#include "../misc/sdl/CCIcon_SDL.h"

static void ApplyIcon(void) {
	SDL_Surface* surface = SDL_CreateSurfaceFrom(CCIcon_Width, CCIcon_Height, SDL_PIXELFORMAT_BGRA8888,
												 (void*)CCIcon_Data, CCIcon_Pitch);

	SDL_SetWindowIcon(win_handle, surface);
}
#else
static void ApplyIcon(void) { }
#endif

static void DoCreateWindow(int width, int height, int flags) {
	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER,  width);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, flags | SDL_WINDOW_RESIZABLE);

	win_handle = SDL_CreateWindowWithProperties(props);
	if (!win_handle) Window_SDLFail("creating window");
	SDL_DestroyProperties(props);

	RefreshWindowBounds();
	Window_Main.Exists     = true;
	Window_Main.Handle.ptr = win_handle;
	Window_Main.UIScaleX   = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY   = DEFAULT_UI_SCALE_Y;

	Window_Main.SoftKeyboardInstant = true;
	ApplyIcon();
	/* TODO grab using SDL_SetWindowGrab? seems to be unnecessary on Linux at least */
}

void Window_Create2D(int width, int height) { DoCreateWindow(width, height, 0); }
#if CC_GFX_BACKEND_IS_GL()
void Window_Create3D(int width, int height) { DoCreateWindow(width, height, SDL_WINDOW_OPENGL); }
#else
void Window_Create3D(int width, int height) { DoCreateWindow(width, height, 0); }
#endif

void Window_Destroy(void) {
	SDL_DestroyWindow(win_handle);
}

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

	if (flags & SDL_WINDOW_MINIMIZED)  return WINDOW_STATE_MINIMISED;
	if (flags & SDL_WINDOW_FULLSCREEN) return WINDOW_STATE_FULLSCREEN;
	return WINDOW_STATE_NORMAL;
}

cc_result Window_EnterFullscreen(void) {
	return SDL_SetWindowFullscreen(win_handle, true);
}
cc_result Window_ExitFullscreen(void) { 
	return SDL_SetWindowFullscreen(win_handle, false);
}

int Window_IsObscured(void) {
	Uint32 flags = SDL_GetWindowFlags(win_handle);
	return flags & SDL_WINDOW_OCCLUDED;
}

void Window_Show(void) {
	 SDL_ShowWindow(win_handle); 
}

void Window_SetSize(int width, int height) {
	SDL_SetWindowSize(win_handle, width, height);
}

void Window_RequestClose(void) {
	SDL_Event e;
	e.type = SDL_EVENT_QUIT;
	SDL_PushEvent(&e);
}

static int MapNativeKey(SDL_Keycode k) {
	if (k >= SDLK_0   && k <= SDLK_9)   { return '0'       + (k - SDLK_0); }
	if (k >= SDLK_A   && k <= SDLK_Z)   { return 'A'       + (k - SDLK_A); }
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
		case SDLK_APOSTROPHE: return CCKEY_QUOTE;
		case SDLK_EQUALS: return CCKEY_EQUALS;
		case SDLK_COMMA:  return CCKEY_COMMA;
		case SDLK_MINUS:  return CCKEY_MINUS;
		case SDLK_PERIOD: return CCKEY_PERIOD;
		case SDLK_SLASH:  return CCKEY_SLASH;
		case SDLK_SEMICOLON:    return CCKEY_SEMICOLON;
		case SDLK_LEFTBRACKET:  return CCKEY_LBRACKET;
		case SDLK_BACKSLASH:    return CCKEY_BACKSLASH;
		case SDLK_RIGHTBRACKET: return CCKEY_RBRACKET;
		case SDLK_GRAVE:        return CCKEY_TILDE;
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
		
		case SDLK_MEDIA_NEXT_TRACK: return CCKEY_MEDIA_NEXT;
		case SDLK_MEDIA_PREVIOUS_TRACK: return CCKEY_MEDIA_PREV;
		case SDLK_MEDIA_PLAY: return CCKEY_MEDIA_PLAY;
		case SDLK_MEDIA_STOP: return CCKEY_MEDIA_STOP;
		
		case SDLK_MUTE:  return CCKEY_VOLUME_MUTE;
		case SDLK_VOLUMEDOWN: return CCKEY_VOLUME_DOWN;
		case SDLK_VOLUMEUP:   return CCKEY_VOLUME_UP;
	}
	return INPUT_NONE;
}

static void OnKeyEvent(const SDL_Event* e) {
	cc_bool pressed = e->key.down == true;
	int key = MapNativeKey(e->key.key);
	if (key) Input_Set(key, pressed);
}

static void OnMouseEvent(const SDL_Event* e) {
	cc_bool pressed = e->button.down == true;
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
	const cc_uint8* src;
	cc_codepoint cp;
	int i, len;

	src = (cc_uint8*)e->text.text;
	len = String_Length(e->text.text);

	while (len > 0) {
		i = Convert_Utf8ToCodepoint(&cp, src, len);
		if (!i) break;

		Event_RaiseInt(&InputEvents.Press, cp);
		src += i; len -= i;
	}
}
static void ProcessDialogEvent(SDL_Event* e);

void Window_ProcessEvents(float delta) {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {

		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
			OnKeyEvent(&e); break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
			OnMouseEvent(&e); break;
		case SDL_EVENT_MOUSE_WHEEL:
			Mouse_ScrollHWheel(e.wheel.x);
			Mouse_ScrollVWheel(e.wheel.y);
			break;
		case SDL_EVENT_MOUSE_MOTION:
			Pointer_SetPosition(0, e.motion.x, e.motion.y);
			if (Input.RawMode) Event_RaiseRawMove(&PointerEvents.RawMoved, e.motion.xrel, e.motion.yrel);
			break;
		case SDL_EVENT_TEXT_INPUT:
			OnTextEvent(&e); break;

		case SDL_EVENT_QUIT:
			Window_Main.Exists = false;
			Event_RaiseVoid(&WindowEvents.Closing);
			break;

		case SDL_EVENT_RENDER_DEVICE_RESET:
			Gfx_LoseContext("SDL device reset event");
			Gfx_RecreateContext();
			break;
			
			
		case SDL_EVENT_WINDOW_EXPOSED:
			Event_RaiseVoid(&WindowEvents.RedrawNeeded);
			break;
		case SDL_EVENT_WINDOW_RESIZED: // TODO SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED 
			RefreshWindowBounds();
			Event_RaiseVoid(&WindowEvents.Resized);
			break;
		case SDL_EVENT_WINDOW_MINIMIZED:
		case SDL_EVENT_WINDOW_MAXIMIZED:
		case SDL_EVENT_WINDOW_RESTORED:
			Event_RaiseVoid(&WindowEvents.StateChanged);
			break;
		case SDL_EVENT_WINDOW_FOCUS_GAINED:
			Window_Main.Focused = true;
			Event_RaiseVoid(&WindowEvents.FocusChanged);
			break;
		case SDL_EVENT_WINDOW_FOCUS_LOST:
			Window_Main.Focused = false;
			Event_RaiseVoid(&WindowEvents.FocusChanged);
			break;
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			Window_RequestClose();
			break;
		default:
			if (e.type == dlg_event) ProcessDialogEvent(&e);
			break;
		}
	}
}

static void Cursor_GetRawPos(int* x, int* y) {
	float xPos, yPos;
	SDL_GetMouseState(&xPos, &yPos);
	*x = xPos; *y = yPos;
}
void Cursor_SetPosition(int x, int y) {
	SDL_WarpMouseInWindow(win_handle, x, y);
}

static void Cursor_DoSetVisible(cc_bool visible) {
	if (visible) {
		SDL_ShowCursor();
	} else {
		SDL_HideCursor();
	}
}

static void ShowDialogCore(const char* title, const char* msg) {
	SDL_ShowSimpleMessageBox(0, title, msg, win_handle);
}	

static FileDialogCallback dlgCallback;
static SDL_DialogFileFilter* save_filters;

static void ProcessDialogEvent(SDL_Event* e) {
	char* result = e->user.data1;
	int length   = e->user.code;
	
	cc_string path; char pathBuffer[1024];
	String_InitArray(path, pathBuffer);
	String_AppendUtf8(&path, result, length);
	
	dlgCallback(&path);
	dlgCallback = NULL;
	Mem_Free(result);
}

static void DialogCallback(void *userdata, const char* const* filelist, int filter) {
	if (!filelist) return; /* Error occurred */
	const char* result = filelist[0];
	if (!result) return; /* No file provided */
	
	char* path    = Mem_Alloc(NATIVE_STR_LEN, 1, "Dialog path");
	cc_string str = String_Init(path, 0, NATIVE_STR_LEN);
	String_AppendUtf8(&str, result, String_Length(result));
	
	// May need to add file extension when saving, e.g. on Windows
	if (save_filters && filter >= 0 && save_filters[filter].pattern)
		String_Format1(&str, ".%c", save_filters[filter].pattern);
	
	// Dialog callback may not be called from the main thread
	//  (E.g. on windows it is called from a background thread)
	SDL_Event e = { 0 };
	e.type = SDL_EVENT_USER;
	e.user.code  = str.length;
	e.user.data1 = path;
	SDL_PushEvent(&e);
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	// TODO free memory
	char* pattern = Mem_Alloc(301, 1, "OpenDialog pattern");
	SDL_DialogFileFilter* filters = Mem_Alloc(2, sizeof(SDL_DialogFileFilter), "OpenDialog filters");
	int i;
	
	cc_string str = String_Init(pattern, 0, 300);
	for (i = 0; ; i++)
	{
		if (!args->filters[i]) break;
		if (i) String_Append(&str, ';');
		String_AppendConst(&str, args->filters[i] + 1);
	}
	
	pattern[str.length] = '\0';
	filters[0].name     = args->description;
	filters[0].pattern  = pattern;
	
	dlgCallback  = args->Callback;
	save_filters = NULL;
	SDL_ShowOpenFileDialog(DialogCallback, NULL, win_handle, filters, 1, NULL, false);
	return 0;
}

#define MAX_SAVE_DIALOG_FILTERS 10
cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	// TODO free memory
	char* defName = Mem_Alloc(NATIVE_STR_LEN, 1, "SaveDialog default");
	SDL_DialogFileFilter* filters = Mem_Alloc(MAX_SAVE_DIALOG_FILTERS + 1, sizeof(SDL_DialogFileFilter), "SaveDialog filters");
	int i;
	String_EncodeUtf8(defName, &args->defaultName);
	
	for (i = 0; i < MAX_SAVE_DIALOG_FILTERS; i++)
	{
		if (!args->filters[i]) break;
		filters[i].name    = args->titles[i];
		filters[i].pattern = args->filters[i] + 1; // skip .
	}
	
	filters[i].name    = NULL;
	filters[i].pattern = NULL;
	
	dlgCallback  = args->Callback;
	save_filters = filters;
	SDL_ShowSaveFileDialog(DialogCallback, NULL, win_handle, filters, 1, defName);
	return 0;
}

static SDL_Surface* win_surface;
static SDL_Surface* blit_surface;

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	win_surface = SDL_GetWindowSurface(win_handle);
	if (!win_surface) Window_SDLFail("getting window surface");

	int bits_per_pixel = SDL_BITSPERPIXEL(win_surface->format);
	if (bits_per_pixel != 32) {
		/* Slow path: e.g. 15 or 16 bit pixels */
		Platform_Log1("Slow color depth: %b bpp", &bits_per_pixel);
		blit_surface = SDL_CreateSurface(win_surface->w, win_surface->h, SDL_PIXELFORMAT_BGRA32);
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
	/* TODO proper stride */
	bmp->width  = width;
	bmp->height = height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	SDL_Rect rect;
	rect.x = r.x; rect.w = r.width;
	rect.y = r.y; rect.h = r.height;

	if (blit_surface) SDL_BlitSurface(blit_surface, &rect, win_surface, &rect);
	SDL_UpdateWindowSurfaceRects(win_handle, &rect, 1);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	if (blit_surface) SDL_DestroySurface(blit_surface);
	blit_surface = NULL;

	/* SDL docs explicitly say to NOT free window surface */
	/* https://wiki.libsdl.org/SDL_GetWindowSurface */
	/* TODO: Do we still need to unlock it though? */
}

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { SDL_StartTextInput(win_handle); }
void OnscreenKeyboard_SetText(const cc_string* text) { }
void OnscreenKeyboard_Close(void) { SDL_StopTextInput(win_handle); }

void Window_EnableRawMouse(void) {
	RegrabMouse();
	SDL_SetWindowRelativeMouseMode(win_handle, true);
	Input.RawMode = true;
}
void Window_UpdateRawMouse(void) { CentreMousePosition(); }

void Window_DisableRawMouse(void) {
	RegrabMouse();
	SDL_SetWindowRelativeMouseMode(win_handle, false);
	Input.RawMode = false;
}


/*########################################################################################################################*
*--------------------------------------------------------Gamepads---------------------------------------------------------*
*#########################################################################################################################*/
#include "ExtMath.h"
static SDL_Gamepad* controllers[INPUT_MAX_GAMEPADS];

static void LoadControllers(void) {
	int count = 0;
	SDL_JoystickID* joysticks = SDL_GetGamepads(&count);

    for (int i = 0; i < count && i < INPUT_MAX_GAMEPADS; i++) 
	{
		Input.Sources |= INPUT_SOURCE_GAMEPAD;
		controllers[i] = SDL_OpenGamepad(joysticks[i]);
    }
	SDL_free(joysticks);
}

void Gamepads_Init(void) {
	LoadControllers();
}

static void ProcessGamepadButtons(int port, SDL_Gamepad* gp) {
	Gamepad_SetButton(port, CCPAD_1, SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_SOUTH));
	Gamepad_SetButton(port, CCPAD_2, SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_EAST));
	Gamepad_SetButton(port, CCPAD_3, SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_WEST));
	Gamepad_SetButton(port, CCPAD_4, SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_NORTH));

	Gamepad_SetButton(port, CCPAD_ZL, SDL_GetGamepadAxis(  gp, SDL_GAMEPAD_AXIS_LEFT_TRIGGER ) > 8000);
	Gamepad_SetButton(port, CCPAD_ZR, SDL_GetGamepadAxis(  gp, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > 8000);
	Gamepad_SetButton(port, CCPAD_L,  SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER));
	Gamepad_SetButton(port, CCPAD_R,  SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER));

	Gamepad_SetButton(port, CCPAD_SELECT, SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_GUIDE));
	Gamepad_SetButton(port, CCPAD_START,  SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_START));
	Gamepad_SetButton(port, CCPAD_LSTICK, SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_LEFT_STICK));
	Gamepad_SetButton(port, CCPAD_RSTICK, SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_RIGHT_STICK));
	
	Gamepad_SetButton(port, CCPAD_UP,    SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_UP));
	Gamepad_SetButton(port, CCPAD_DOWN,  SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_DOWN));
	Gamepad_SetButton(port, CCPAD_LEFT,  SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_LEFT));
	Gamepad_SetButton(port, CCPAD_RIGHT, SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_RIGHT));
}

#define PAD_AXIS_SCALE 32768.0f
static void ProcessJoystick(int port, SDL_Gamepad* gp, int axis, float delta) {
	int x = SDL_GetGamepadAxis(gp, axis == PAD_AXIS_LEFT ? SDL_GAMEPAD_AXIS_LEFTX : SDL_GAMEPAD_AXIS_RIGHTX);
	int y = SDL_GetGamepadAxis(gp, axis == PAD_AXIS_LEFT ? SDL_GAMEPAD_AXIS_LEFTY : SDL_GAMEPAD_AXIS_RIGHTY);

	// May not be exactly 0 on actual hardware
	if (Math_AbsI(x) <= 1024) x = 0;
	if (Math_AbsI(y) <= 1024) y = 0;
	
	Gamepad_SetAxis(port, axis, x / PAD_AXIS_SCALE, -y / PAD_AXIS_SCALE, delta);
}

void Gamepads_Process(float delta) {
	for (int i = 0; i < INPUT_MAX_GAMEPADS; i++)
	{
		SDL_Gamepad* gp = controllers[i];
		if (!gp) continue;
		int port = Gamepad_Connect(0x5D13 + i, PadBind_Defaults);

		ProcessGamepadButtons(port, gp);
		ProcessJoystick(port, gp, PAD_AXIS_LEFT,  delta);
		ProcessJoystick(port, gp, PAD_AXIS_RIGHT, delta);
	}
}


/*########################################################################################################################*
*-----------------------------------------------------OpenGL context------------------------------------------------------*
*#########################################################################################################################*/
#if CC_GFX_BACKEND_IS_GL()
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
#ifdef CC_BUILD_GLES
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

	win_ctx = SDL_GL_CreateContext(win_handle);
	if (!win_ctx) Window_SDLFail("creating OpenGL context");
}

void GLContext_Update(void) { }
cc_bool GLContext_TryRestore(void) { return true; }
void GLContext_Free(void) {
	SDL_GL_DestroyContext(win_ctx);
	win_ctx = NULL;
}

void* GLContext_GetAddress(const char* function) {
	return SDL_GL_GetProcAddress(function);
}

cc_bool GLContext_SwapBuffers(void) {
	SDL_GL_SwapWindow(win_handle);
	return true;
}

void GLContext_SetVSync(cc_bool vsync) {
	SDL_GL_SetSwapInterval(vsync);
}
void GLContext_GetApiInfo(cc_string* info) { }
#endif
#endif
