#include "../_WindowBase.h"
#include "../Window.h"
#include "../Platform.h"
#include "../Input.h"
#include "../Event.h"
#include "../Graphics.h"
#include "../String_.h"
#include "../Funcs.h"
#include "../Bitmap.h"
#include "../Errors.h"
#include "../ExtMath.h"
#include "../Input.h"
#include "../Gui.h"

#include <switch.h>

static Framebuffer fb;
static PadState pad;
static AppletHookCookie cookie;

struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;

static void SetResolution(void) {
	// check whether the Switch is docked
	// use 720p for handheld, 1080p for docked
	AppletOperationMode opMode = appletGetOperationMode();
	int w = 1280;
	int h = 720;
	if (opMode == AppletOperationMode_Console) {
		w = 1920;
		h = 1080;
	}

	DisplayInfo.Width  = w;
	DisplayInfo.Height = h;

	Window_Main.Width  = w;
	Window_Main.Height = h;
}

static void Applet_Event(AppletHookType type, void* param) {
	if (type == AppletHookType_OnOperationMode) {
		SetResolution();
		Event_RaiseVoid(&WindowEvents.Resized);
	} else if (type == AppletHookType_OnExitRequest) {
		Window_Main.Exists = false;
		Window_RequestClose();
	}
}

void Window_PreInit(void) {
	appletHook(&cookie, Applet_Event, NULL);
}

void Window_Init(void) {
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;

	Window_Main.Focused    = true;
	Window_Main.Exists     = true;
	Window_Main.Handle.ptr = nwindowGetDefault();
	Window_Main.UIScaleX   = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY   = DEFAULT_UI_SCALE_Y;

	Window_Main.SoftKeyboard = SOFT_KEYBOARD_RESIZE;
	Input_SetTouchMode(true);
	Gui_SetTouchUI(true);

	nwindowSetDimensions(Window_Main.Handle.ptr, 1920, 1080);
	SetResolution();
}

void Window_Free(void) {
	appletUnhook(&cookie);
}

void Window_Create2D(int width, int height) {
	Window_Main.Is3D = false;
}

void Window_Create3D(int width, int height) {
	Window_Main.Is3D = true;
}

void Window_Destroy(void) { }

void Window_SetTitle(const cc_string* title) { }
void Clipboard_GetText(cc_string* value) { }
void Clipboard_SetText(const cc_string* value) { }

int Window_GetWindowState(void) { return WINDOW_STATE_FULLSCREEN; }
cc_result Window_EnterFullscreen(void) { return 0; }
cc_result Window_ExitFullscreen(void)  { return 0; }
int Window_IsObscured(void)            { return 0; }

void Window_Show(void) { }
void Window_SetSize(int width, int height) { }

void Window_RequestClose(void) {
	Event_RaiseVoid(&WindowEvents.Closing);
}


/*########################################################################################################################*
*----------------------------------------------------Input processing-----------------------------------------------------*
*#########################################################################################################################*/
static void ProcessTouchInput(void) {
	static int prev_touchcount = 0;
	HidTouchScreenState state = { 0 };
	hidGetTouchScreenStates(&state, 1);

	if (state.count) {
		Input_AddTouch(0,    state.touches[0].x, state.touches[0].y);
	} else if (prev_touchcount) {
		Input_RemoveTouch(0, Pointers[0].x,      Pointers[0].y);
	}
	prev_touchcount = state.count;
}

void Window_ProcessEvents(float delta) {
	// Scan the gamepad. This should be done once for each frame
	padUpdate(&pad);

	if (!appletMainLoop()) {
		Window_Main.Exists = false;
		Window_RequestClose();
		return;
	}
	ProcessTouchInput();
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for PSP
void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }

void Window_UpdateRawMouse(void)  { }


/*########################################################################################################################*
*-------------------------------------------------------Gamepads----------------------------------------------------------*
*#########################################################################################################################*/
static const BindMapping defaults_switch[BIND_COUNT] = {
	[BIND_FORWARD]      = { CCPAD_UP    },  
	[BIND_BACK]         = { CCPAD_DOWN  },
	[BIND_LEFT]         = { CCPAD_LEFT  },  
	[BIND_RIGHT]        = { CCPAD_RIGHT },
	[BIND_JUMP]         = { CCPAD_1     },
	[BIND_SET_SPAWN]    = { CCPAD_START }, 
	[BIND_CHAT]         = { CCPAD_4     },
	[BIND_INVENTORY]    = { CCPAD_3     },
	[BIND_SEND_CHAT]    = { CCPAD_START },
	[BIND_PLACE_BLOCK]  = { CCPAD_L     },
	[BIND_DELETE_BLOCK] = { CCPAD_R     },
	[BIND_SPEED]        = { CCPAD_2, CCPAD_L },
	[BIND_FLY]          = { CCPAD_2, CCPAD_R },
	[BIND_NOCLIP]       = { CCPAD_2, CCPAD_3 },
	[BIND_FLY_UP]       = { CCPAD_2, CCPAD_UP },
	[BIND_FLY_DOWN]     = { CCPAD_2, CCPAD_DOWN },
	[BIND_HOTBAR_LEFT]  = { CCPAD_ZL    }, 
	[BIND_HOTBAR_RIGHT] = { CCPAD_ZR    }
};

void Gamepads_PreInit(void) {
	// Configure our supported input layout: a single player with standard controller styles
	padConfigureInput(1, HidNpadStyleSet_NpadStandard);
	hidInitializeTouchScreen();
	// Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
	padInitializeDefault(&pad);
}

void Gamepads_Init(void) { }

static void HandleButtons(int port, u64 mods) {
	Gamepad_SetButton(port, CCPAD_L,  mods & HidNpadButton_L);
	Gamepad_SetButton(port, CCPAD_R,  mods & HidNpadButton_R);
	Gamepad_SetButton(port, CCPAD_ZL, mods & HidNpadButton_ZL);
	Gamepad_SetButton(port, CCPAD_ZR, mods & HidNpadButton_ZR);
	
	Gamepad_SetButton(port, CCPAD_1, mods & HidNpadButton_A);
	Gamepad_SetButton(port, CCPAD_2, mods & HidNpadButton_B);
	Gamepad_SetButton(port, CCPAD_3, mods & HidNpadButton_X);
	Gamepad_SetButton(port, CCPAD_4, mods & HidNpadButton_Y);
	
	Gamepad_SetButton(port, CCPAD_START,  mods & HidNpadButton_Plus);
	Gamepad_SetButton(port, CCPAD_SELECT, mods & HidNpadButton_Minus);
	
	Gamepad_SetButton(port, CCPAD_LEFT,   mods & HidNpadButton_Left);
	Gamepad_SetButton(port, CCPAD_RIGHT,  mods & HidNpadButton_Right);
	Gamepad_SetButton(port, CCPAD_UP,     mods & HidNpadButton_Up);
	Gamepad_SetButton(port, CCPAD_DOWN,   mods & HidNpadButton_Down);
}

#define AXIS_SCALE 512.0f
static void ProcessJoystickInput(int port, int axis, HidAnalogStickState* pos, float delta) {
	// May not be exactly 0 on actual hardware
	if (Math_AbsI(pos->x) <= 16) pos->x = 0;
	if (Math_AbsI(pos->y) <= 16) pos->y = 0;
	
	Gamepad_SetAxis(port, axis, pos->x / AXIS_SCALE, -pos->y / AXIS_SCALE, delta);
}

void Gamepads_Process(float delta) {
	int port = Gamepad_Connect(0x51C, defaults_switch);
	u64 keys = padGetButtons(&pad);
	HandleButtons(port, keys);

	// Read the sticks' position
	HidAnalogStickState analog_stick_l = padGetStickPos(&pad, 0);
	HidAnalogStickState analog_stick_r = padGetStickPos(&pad, 1);
	ProcessJoystickInput(port, PAD_AXIS_LEFT,  &analog_stick_l, delta);
	ProcessJoystickInput(port, PAD_AXIS_RIGHT, &analog_stick_r, delta);
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	framebufferCreate(&fb, nwindowGetDefault(), DisplayInfo.Width, DisplayInfo.Height, PIXEL_FORMAT_BGRA_8888, 2);
	framebufferMakeLinear(&fb);

	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	bmp->width  = width;
	bmp->height = height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	cc_uint32 stride;
	cc_uint32* framebuf = (cc_uint32*)framebufferBegin(&fb, &stride);

	for (int y = r.y; y < r.y + r.height; y++)
	{
		BitmapCol* src = Bitmap_GetRow(bmp, y);
		cc_uint32* dst = framebuf + y * stride / sizeof(cc_uint32);

		for (int x = r.x; x < r.x + r.width; x++)
		{
			dst[x] = src[x];
		}
	}
	framebufferEnd(&fb);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	framebufferClose(&fb);
	Mem_Free(bmp->scan0);
}

/*########################################################################################################################*
*-----------------------------------------------------OpenGL context------------------------------------------------------*
*#########################################################################################################################*/
#include <EGL/egl.h>
static EGLDisplay ctx_display;
static EGLContext ctx_context;
static EGLSurface ctx_surface;
static EGLConfig  ctx_config;

static void GLContext_InitSurface(void) {
	NWindow* window = (NWindow*)Window_Main.Handle.ptr;
	if (!window) return; /* window not created or lost */

	// terrible, but fixes 720p/1080p resolution change on handheld/docked modes
	int real_w     = window->width;
	int real_h     = window->height;
	window->width  = Window_Main.Width;
	window->height = Window_Main.Height;

	ctx_surface = eglCreateWindowSurface(ctx_display, ctx_config, window, NULL);
	window->width  = real_w;
	window->height = real_h;
	
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
	static const EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

	static const EGLint attribs[] = {
		EGL_RED_SIZE,          8, EGL_GREEN_SIZE,  8,
		EGL_BLUE_SIZE,         8, EGL_ALPHA_SIZE,  0,
		EGL_DEPTH_SIZE,        GLCONTEXT_DEFAULT_DEPTH,
		EGL_STENCIL_SIZE,      0,
		EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
		EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE,   EGL_OPENGL_ES2_BIT, // EGL_OPENGL_BIT
		EGL_NONE
	};

	ctx_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(ctx_display, NULL, NULL);
	eglBindAPI(EGL_OPENGL_ES_API);

	EGLConfig configs[64];
	EGLint numConfig = 0;

	eglChooseConfig(ctx_display, attribs, configs, 64, &numConfig);
	ctx_config = configs[0];

	ctx_context = eglCreateContext(ctx_display, ctx_config, EGL_NO_CONTEXT, context_attribs);
	if (!ctx_context) Process_Abort2(eglGetError(), "Failed to create EGL context");
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
	return (void*)eglGetProcAddress(function);
}

cc_bool GLContext_SwapBuffers(void) {
	if (!ctx_surface) return false;
	if (eglSwapBuffers(ctx_display, ctx_surface)) return true;

	EGLint err = eglGetError();
	/* TODO: figure out what errors need to be handled here */
	Process_Abort2(err, "Failed to swap buffers");
	return false;
}

void GLContext_SetVSync(cc_bool vsync) {
	eglSwapInterval(ctx_display, vsync);
}

void GLContext_GetApiInfo(cc_string* info) { }


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
static void OnscreenTextChanged(const char* text) {
	char tmpBuffer[NATIVE_STR_LEN];
	cc_string tmp = String_FromArray(tmpBuffer);
	String_AppendUtf8(&tmp, text, String_Length(text));
    
	Event_RaiseString(&InputEvents.TextChanged, &tmp);
}

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) {
	const char* btnText = args->type & KEYBOARD_FLAG_SEND ? "Send" : "Enter";
	char input[NATIVE_STR_LEN]  = { 0 };
	char output[NATIVE_STR_LEN] = { 0 };
	String_EncodeUtf8(input, args->text);
	
	int mode = args->type & 0xFF;
	SwkbdType type = (mode == KEYBOARD_TYPE_NUMBER || mode == KEYBOARD_TYPE_INTEGER) ? SwkbdType_NumPad : SwkbdType_Normal;

	SwkbdConfig kbd;
	swkbdCreate(&kbd, 0);

	if (mode == KEYBOARD_TYPE_PASSWORD) {
		swkbdConfigMakePresetPassword(&kbd);
	} else {
		swkbdConfigMakePresetDefault(&kbd);
		swkbdConfigSetType(&kbd, type);
	}

	swkbdConfigSetInitialText(&kbd, input);
	swkbdConfigSetGuideText(&kbd, args->placeholder);
	swkbdConfigSetOkButtonText(&kbd, btnText);

	Result rc = swkbdShow(&kbd, output, sizeof(output));
	if (R_SUCCEEDED(rc))
		OnscreenTextChanged(output);

	swkbdClose(&kbd);
}
void OnscreenKeyboard_SetText(const cc_string* text) { }
void OnscreenKeyboard_Close(void) { /* TODO implement */ }


/*########################################################################################################################*
*-------------------------------------------------------Misc/Other--------------------------------------------------------*
*#########################################################################################################################*/
//void Window_ShowDialog(const char* title, const char* msg) {
static void ShowDialogCore(const char* title, const char* msg) {
	ErrorApplicationConfig c;
	errorApplicationCreate(&c, title, msg);
	errorApplicationShow(&c);
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

