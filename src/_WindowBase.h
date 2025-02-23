#include "Window.h"
#include "Input.h"
#include "Event.h"
#include "Logger.h"
#include "Platform.h"

struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;

#define Display_CentreX(width)  (DisplayInfo.x + (DisplayInfo.Width  - width)  / 2)
#define Display_CentreY(height) (DisplayInfo.y + (DisplayInfo.Height - height) / 2)

static int cursorPrevX, cursorPrevY;
/* Gets the position of the cursor in screen or window coordinates */
static void Cursor_GetRawPos(int* x, int* y);
/* Sets whether the cursor is visible when over this window */
/* NOTE: You MUST BE VERY CAREFUL with this! OS typically uses a counter for visibility, */
/*  so setting invisible multiple times means you must then set visible multiple times. */
static void Cursor_DoSetVisible(cc_bool visible);

/* Sets whether the cursor is visible when over this window */
static void Cursor_SetVisible(cc_bool visible) {
	if (DisplayInfo.CursorVisible == visible) return;
	DisplayInfo.CursorVisible = visible;
	Cursor_DoSetVisible(visible);
}

static void MoveRawUsingCursorDelta(void) {
	int x, y;
	Cursor_GetRawPos(&x, &y);
	Event_RaiseRawMove(&PointerEvents.RawMoved, x - cursorPrevX, y - cursorPrevY);
}

static void CentreMousePosition(void) {
	Cursor_SetPosition(Window_Main.Width / 2, Window_Main.Height / 2);
	/* Fixes issues with large DPI displays on Windows >= 8.0. */
	Cursor_GetRawPos(&cursorPrevX, &cursorPrevY);
}

static void RegrabMouse(void) {
	if (!Window_Main.Focused || !Window_Main.Exists) return;
	CentreMousePosition();
}

static CC_INLINE void DefaultEnableRawMouse(void) {
	Input.RawMode = true;
	RegrabMouse();
	Cursor_SetVisible(false);
}

static CC_INLINE void DefaultUpdateRawMouse(void) {
	MoveRawUsingCursorDelta();
	CentreMousePosition();
}

static CC_INLINE void DefaultDisableRawMouse(void) {
	Input.RawMode = false;
	RegrabMouse();
	Cursor_SetVisible(true);
}

/* The actual windowing system specific method to display a message box */
static void ShowDialogCore(const char* title, const char* msg);
void Window_ShowDialog(const char* title, const char* msg) {
	/* Ensure cursor is usable while showing message box */
	cc_bool rawMode = Input.RawMode;

	if (rawMode) Window_DisableRawMouse();
	ShowDialogCore(title, msg);
	if (rawMode) Window_EnableRawMouse();
}


struct GraphicsMode { int R, G, B, A; };
/* Creates a GraphicsMode compatible with the default display device */
static CC_INLINE void InitGraphicsMode(struct GraphicsMode* m) {
	int bpp = DisplayInfo.Depth;
	m->A = 0;

	switch (bpp) {
	case 32:
		m->R =  8; m->G =  8; m->B =  8; m->A = 8; break;
	case 30:
		m->R = 10; m->G = 10; m->B = 10; m->A = 2; break;
	case 24:
		m->R =  8; m->G =  8; m->B =  8; break;
	case 16:
		m->R =  5; m->G =  6; m->B =  5; break;
	case 15:
		m->R =  5; m->G =  5; m->B =  5; break;
	case 8:
		m->R =  3; m->G =  3; m->B =  2; break;
	case 4:
		m->R =  2; m->G =  2; m->B =  1; break;
	default:
		/* mode->R = 0; mode->G = 0; mode->B = 0; */
		Process_Abort2(bpp, "Unsupported bits per pixel"); break;
	}
}

/* EGL is window system agnostic, other OpenGL context backends are tied to one windowing system */
#if CC_GFX_BACKEND_IS_GL() && defined CC_BUILD_EGL
/*########################################################################################################################*
*-------------------------------------------------------EGL OpenGL--------------------------------------------------------*
*#########################################################################################################################*/
#include <EGL/egl.h>
static EGLDisplay ctx_display;
static EGLContext ctx_context;
static EGLSurface ctx_surface;
static EGLConfig ctx_config;
static cc_uintptr ctx_visualID;

#ifdef CC_BUILD_SWITCH
static void GLContext_InitSurface(void); // replacement in Window_Switch.c for handheld/docked resolution fix
#else
static void GLContext_InitSurface(void) {
	EGLNativeWindowType window = (EGLNativeWindowType)Window_Main.Handle.ptr;
	if (!window) return; /* window not created or lost */
	ctx_surface = eglCreateWindowSurface(ctx_display, ctx_config, window, NULL);

	if (!ctx_surface) return;
	eglMakeCurrent(ctx_display, ctx_surface, ctx_surface, ctx_context);
}
#endif

static void GLContext_FreeSurface(void) {
	if (!ctx_surface) return;
	eglMakeCurrent(ctx_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(ctx_display, ctx_surface);
	ctx_surface = NULL;
}

static void ChooseEGLConfig(EGLConfig* configs, EGLint num_configs) {
	int i;
	ctx_config = configs[0];
	if (!ctx_visualID) return;

	/* In X11 case, bad things happen if EGL visual ID != window visual ID */
	for (i = 0; i < num_configs; i++) {
		EGLint visualID = 0;
		eglGetConfigAttrib(ctx_display, configs[i], EGL_NATIVE_VISUAL_ID, &visualID);
		if (visualID != ctx_visualID) continue;

		ctx_config = configs[i];
		return;
	}
}

void GLContext_Create(void) {
#if CC_GFX_BACKEND == CC_GFX_BACKEND_GL2
	static EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
#else
	static EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 1, EGL_NONE };
#endif
	static EGLint attribs[] = {
		EGL_RED_SIZE,  0, EGL_GREEN_SIZE,  0,
		EGL_BLUE_SIZE, 0, EGL_ALPHA_SIZE,  0,
		EGL_DEPTH_SIZE,        GLCONTEXT_DEFAULT_DEPTH,
		EGL_STENCIL_SIZE,      0,
		EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
		EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,

#if defined CC_BUILD_GLES && (CC_GFX_BACKEND == CC_GFX_BACKEND_GL2)
		EGL_RENDERABLE_TYPE,   EGL_OPENGL_ES2_BIT,
#elif defined CC_BUILD_GLES
		EGL_RENDERABLE_TYPE,   EGL_OPENGL_ES_BIT,
#else
		EGL_RENDERABLE_TYPE,   EGL_OPENGL_BIT,
#endif
		EGL_NONE
	};

	struct GraphicsMode mode;
	InitGraphicsMode(&mode);
	attribs[1] = mode.R; attribs[3] = mode.G;
	attribs[5] = mode.B; attribs[7] = mode.A;

	ctx_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(ctx_display, NULL, NULL);
	eglBindAPI(EGL_OPENGL_ES_API);

	EGLConfig configs[64];
	EGLint numConfig = 0;

	eglChooseConfig(ctx_display, attribs, configs, 64, &numConfig);
	if (!numConfig) {
		attribs[9] = 16; // some older devices only support 16 bit depth buffer
		eglChooseConfig(ctx_display, attribs, configs, 64, &numConfig);
	}

	if (!numConfig) Window_ShowDialog("Warning", "Failed to choose EGL config, ClassiCube may be unable to start");
	ChooseEGLConfig(configs, numConfig);

	EGLint red, green, blue, alpha, depth, vid;
	eglGetConfigAttrib(ctx_display, ctx_config, EGL_RED_SIZE,         &red);
	eglGetConfigAttrib(ctx_display, ctx_config, EGL_GREEN_SIZE,       &green);
	eglGetConfigAttrib(ctx_display, ctx_config, EGL_BLUE_SIZE,        &blue);
	eglGetConfigAttrib(ctx_display, ctx_config, EGL_ALPHA_SIZE,       &alpha);
	eglGetConfigAttrib(ctx_display, ctx_config, EGL_DEPTH_SIZE,       &depth);
	eglGetConfigAttrib(ctx_display, ctx_config, EGL_NATIVE_VISUAL_ID, &vid);

	Platform_Log4("EGL R:%i, G:%i, B:%i, A:%i", &red, &green, &blue, &alpha);
	Platform_Log2("EGL depth: %i, visual: %h",  &depth, &vid);

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
	return eglGetProcAddress(function);
}

cc_bool GLContext_SwapBuffers(void) {
	EGLint err;
	if (!ctx_surface) return false;
	if (eglSwapBuffers(ctx_display, ctx_surface)) return true;

	err = eglGetError();
	/* TODO: figure out what errors need to be handled here */
	Process_Abort2(err, "Failed to swap buffers");
	return false;
}

void GLContext_SetVSync(cc_bool vsync) {
	eglSwapInterval(ctx_display, vsync);
}
void GLContext_GetApiInfo(cc_string* info) { }
#endif
