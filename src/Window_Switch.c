#include "Core.h"
#if defined CC_BUILD_SWITCH
#include "_WindowBase.h"
#include "Window.h"
#include "Platform.h"
#include "Input.h"
#include "Event.h"
#include "Graphics.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Errors.h"
#include "ExtMath.h"
#include "Input.h"
#include <switch.h>

#include <EGL/egl.h>    // EGL library
#include <EGL/eglext.h> // EGL extensions
#include <glad/glad.h>  // glad library (OpenGL loader)


static EGLDisplay s_display;
static EGLContext s_context;
static EGLSurface s_surface;

static cc_bool launcherMode;
static Framebuffer fb;
static PadState pad;
static AppletHookCookie cookie;

struct _DisplayData DisplayInfo;
struct _WindowData WindowInfo;

static void Set_Resolution(void) {
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

	Window_Main.Width   = DisplayInfo.Width;
	Window_Main.Height  = DisplayInfo.Height;
}

static void Applet_Event(AppletHookType type, void* param) {
	if (type == AppletHookType_OnOperationMode) {
		Set_Resolution();

		if (launcherMode) {
			framebufferClose(&fb);
			framebufferCreate(&fb, nwindowGetDefault(), DisplayInfo.Width, DisplayInfo.Height, PIXEL_FORMAT_BGRA_8888, 2);
			framebufferMakeLinear(&fb);
		}

		Event_RaiseVoid(&WindowEvents.Resized);
	}
}

void Window_Init(void) {
	// Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
	padInitializeDefault(&pad);

	appletHook(&cookie, Applet_Event, NULL);
	Set_Resolution();

	DisplayInfo.Depth  = 4; // 32 bit
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;

	Window_Main.Focused = true;
	Window_Main.Exists  = true;

	Input_SetTouchMode(true);
	Input.Sources = INPUT_SOURCE_GAMEPAD;
}

void Window_Free(void) {
	if (launcherMode)
		framebufferClose(&fb);
	appletUnhook(&cookie);
}

void Window_Create2D(int width, int height) {
	framebufferCreate(&fb, nwindowGetDefault(), DisplayInfo.Width, DisplayInfo.Height, PIXEL_FORMAT_BGRA_8888, 2);
	framebufferMakeLinear(&fb);
	launcherMode = true;
}
void Window_Create3D(int width, int height) {
	framebufferClose(&fb);
	launcherMode = false;
}

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
static void HandleButtons(u64 mods) {
	Input_SetNonRepeatable(CCPAD_L, mods & HidNpadButton_L);
	Input_SetNonRepeatable(CCPAD_R, mods & HidNpadButton_R);
	
	Input_SetNonRepeatable(CCPAD_A, mods & HidNpadButton_A);
	Input_SetNonRepeatable(CCPAD_B, mods & HidNpadButton_B);
	Input_SetNonRepeatable(CCPAD_X, mods & HidNpadButton_X);
	Input_SetNonRepeatable(CCPAD_Y, mods & HidNpadButton_Y);
	
	Input_SetNonRepeatable(CCPAD_START,  mods & HidNpadButton_Plus);
	Input_SetNonRepeatable(CCPAD_SELECT, mods & HidNpadButton_Minus);
	
	Input_SetNonRepeatable(CCPAD_LEFT,   mods & HidNpadButton_Left);
	Input_SetNonRepeatable(CCPAD_RIGHT,  mods & HidNpadButton_Right);
	Input_SetNonRepeatable(CCPAD_UP,     mods & HidNpadButton_Up);
	Input_SetNonRepeatable(CCPAD_DOWN,   mods & HidNpadButton_Down);
}

static void ProcessJoystickInput(HidAnalogStickState* pos) {
	// May not be exactly 0 on actual hardware
	if (Math_AbsI(pos->x) <= 16) pos->x = 0;
	if (Math_AbsI(pos->y) <= 16) pos->y = 0;
		
	Event_RaiseRawMove(&ControllerEvents.RawMoved, pos->x / 512.f, -pos->y / 512.f);
}

static void ProcessTouchInput(void) {
	static int currX, currY, prev_touchcount=0;
	HidTouchScreenState state={0};
    hidGetTouchScreenStates(&state, 1);

	if (state.count && !prev_touchcount) {  // stylus went down
		currX = state.touches[0].x;
		currY = state.touches[0].y;
		Input_AddTouch(0, currX, currY);
	}
	else if (state.count) {  // stylus is down
		currX = state.touches[0].x;
		currY = state.touches[0].y;
		Input_UpdateTouch(0, currX, currY);
	}
	else if (!state.count && prev_touchcount) {  // stylus was lifted
		Input_RemoveTouch(0, currX, currY);
	}

	prev_touchcount = state.count;
}

void Window_ProcessEvents(double delta) {
	// Scan the gamepad. This should be done once for each frame
	padUpdate(&pad);

	if (!appletMainLoop()) {
		Window_Main.Exists = false;
		Window_RequestClose();
		return;
	}

	u64 keys = padGetButtons(&pad);
	HandleButtons(keys);

	// Read the sticks' position
	HidAnalogStickState analog_stick_l = padGetStickPos(&pad, 0);
	HidAnalogStickState analog_stick_r = padGetStickPos(&pad, 1);
	ProcessJoystickInput(&analog_stick_l);
	ProcessJoystickInput(&analog_stick_r);

	ProcessTouchInput();
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for PSP
void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }

void Window_UpdateRawMouse(void)  { }


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	// Retrieve the framebuffer
	cc_uint32 stride;
	cc_uint32* framebuf = (cc_uint32*) framebufferBegin(&fb, &stride);

	// flip upside down
	for (cc_uint32 y=r.y; y<r.y + r.Height; y++)
	{
		for (cc_uint32 x=r.x; x<r.x + r.Width; x++)
		{
			cc_uint32 pos = y * stride / sizeof(cc_uint32) + x;
			framebuf[pos] = bmp->scan0[pos];
		}
	}

	// We're done rendering, so we end the frame here.
	framebufferEnd(&fb);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}

/*########################################################################################################################*
*-----------------------------------------------------OpenGL context------------------------------------------------------*
*#########################################################################################################################*/
void GLContext_Create(void) {
	EGLint err;

	// Retrieve the default window
	NWindow* win = nwindowGetDefault();

	// Connect to the EGL default display
    s_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!s_display)
    {
		err = eglGetError();
        Platform_Log1("Could not connect to display! error: %d", &err);
        return;
    }

    // Initialize the EGL display connection
    eglInitialize(s_display, NULL, NULL);

    // Select OpenGL ES as the desired graphics API
    if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE)
    {
		err = eglGetError();
        Platform_Log1("Could not set API! error: %d", &err);
        eglTerminate(s_display);
		s_display = NULL;
		return;
    }

    // Get an appropriate EGL framebuffer configuration
    EGLConfig config;
    EGLint numConfigs;
    static const EGLint framebufferAttributeList[] =
    {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,     0,
        EGL_GREEN_SIZE,   0,
        EGL_BLUE_SIZE,    0,
        EGL_ALPHA_SIZE,   0,
        EGL_DEPTH_SIZE,   GLCONTEXT_DEFAULT_DEPTH,
        EGL_STENCIL_SIZE, 0,
		EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
		EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
        EGL_NONE
    };
    eglChooseConfig(s_display, framebufferAttributeList, &config, 1, &numConfigs);
    if (numConfigs == 0)
    {
		err = eglGetError();
        Platform_Log1("No config found! error: %d", &err);
        eglTerminate(s_display);
		s_display = NULL;
		return;
    }

    // Create an EGL window surface
    s_surface = eglCreateWindowSurface(s_display, config, win, NULL);
    if (!s_surface)
    {
		err = eglGetError();
        Platform_Log1("Surface creation failed! error: %d", &err);
        eglTerminate(s_display);
		s_display = NULL;
		return;
    }

    // Create an EGL rendering context
    static const EGLint contextAttributeList[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    s_context = eglCreateContext(s_display, config, EGL_NO_CONTEXT, contextAttributeList);
    if (!s_context)
    {
		err = eglGetError();
        Platform_Log1("Context creation failed! error: %d", &err);
        eglDestroySurface(s_display, s_surface);
		s_surface = NULL;
    }

    // Connect the context to the surface
    eglMakeCurrent(s_display, s_surface, s_surface, s_context);

	gladLoadGL();
}

void GLContext_Update(void) { }

cc_bool GLContext_TryRestore(void) {
	return true;
}

void GLContext_Free(void) {
	if (s_display)
	{
		eglMakeCurrent(s_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (s_context)
		{
			eglDestroyContext(s_display, s_context);
			s_context = NULL;
		}
		if (s_surface)
		{
			eglDestroySurface(s_display, s_surface);
			s_surface = NULL;
		}
		eglTerminate(s_display);
		s_display = NULL;
	}
}

void* GLContext_GetAddress(const char* function) {
	return (void*)eglGetProcAddress(function);
}

cc_bool GLContext_SwapBuffers(void) {
	eglSwapBuffers(s_display, s_surface);
	return true;
}

void GLContext_SetFpsLimit(cc_bool vsync, float minFrameMs) { }

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

void Window_OpenKeyboard(struct OpenKeyboardArgs* args) {
	const char* btnText = args->type & KEYBOARD_FLAG_SEND ? "Send" : "Enter";
	char input[NATIVE_STR_LEN]  = { 0 };
	char output[NATIVE_STR_LEN] = { 0 };
	String_EncodeUtf8(input, args->text);
	
	int mode = args->type & 0xFF;
	SwkbdType type = (mode == KEYBOARD_TYPE_NUMBER || mode == KEYBOARD_TYPE_INTEGER) ? SwkbdType_NumPad : SwkbdType_Normal;

	SwkbdConfig kbd;
	swkbdCreate(&kbd, 0);

	if (mode == KEYBOARD_TYPE_PASSWORD)
		swkbdConfigMakePresetPassword(&kbd);
	else
	{
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
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { /* TODO implement */ }


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
#endif
