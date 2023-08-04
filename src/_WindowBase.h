#include "Window.h"
#include "Input.h"
#include "Event.h"
#include "Logger.h"
#include "Platform.h"

struct _DisplayData DisplayInfo;
struct _WinData WindowInfo;

#define Display_CentreX(width)  (DisplayInfo.X + (DisplayInfo.Width  - width)  / 2)
#define Display_CentreY(height) (DisplayInfo.Y + (DisplayInfo.Height - height) / 2)

int Display_ScaleX(int x) { return (int)(x * DisplayInfo.ScaleX); }
int Display_ScaleY(int y) { return (int)(y * DisplayInfo.ScaleY); }

static int cursorPrevX, cursorPrevY;
static cc_bool cursorVisible = true;
/* Gets the position of the cursor in screen or window coordinates */
static void Cursor_GetRawPos(int* x, int* y);
/* Sets whether the cursor is visible when over this window */
/* NOTE: You MUST BE VERY CAREFUL with this! OS typically uses a counter for visibility, */
/*  so setting invisible multiple times means you must then set visible multiple times. */
static void Cursor_DoSetVisible(cc_bool visible);

/* Sets whether the cursor is visible when over this window */
static void Cursor_SetVisible(cc_bool visible) {
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
	Input.RawMode = true;
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
static void InitGraphicsMode(struct GraphicsMode* m) {
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
		Logger_Abort2(bpp, "Unsupported bits per pixel"); break;
	}
}

/* EGL is window system agnostic, other OpenGL context backends are tied to one windowing system */
#if defined CC_BUILD_GL && defined CC_BUILD_EGL
/*########################################################################################################################*
*-------------------------------------------------------EGL OpenGL--------------------------------------------------------*
*#########################################################################################################################*/
#include <EGL/egl.h>
static EGLDisplay ctx_display;
static EGLContext ctx_context;
static EGLSurface ctx_surface;
static EGLConfig ctx_config;
static EGLint ctx_numConfig;

static void GLContext_InitSurface(void) {
	void* window = WindowInfo.Handle;
	if (!window) return; /* window not created or lost */
	ctx_surface = eglCreateWindowSurface(ctx_display, ctx_config, window, NULL);

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
#ifdef CC_BUILD_GLMODERN
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

#if defined CC_BUILD_GLES && defined CC_BUILD_GLMODERN
		EGL_RENDERABLE_TYPE,   EGL_OPENGL_ES2_BIT,
#elif defined CC_BUILD_GLES
		EGL_RENDERABLE_TYPE,   EGL_OPENGL_ES_BIT,
#else
		#error "Can't determine appropriate EGL_RENDERABLE_TYPE"
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

	eglChooseConfig(ctx_display, attribs, &ctx_config, 1, &ctx_numConfig);
	if (!ctx_config) {
		attribs[9] = 16; // some older devices only support 16 bit depth buffer
		eglChooseConfig(ctx_display, attribs, &ctx_config, 1, &ctx_numConfig);
	}
	if (!ctx_config) Window_ShowDialog("Warning", "Failed to choose EGL config, ClassiCube may be unable to start");

	ctx_context = eglCreateContext(ctx_display, ctx_config, EGL_NO_CONTEXT, context_attribs);
	if (!ctx_context) Logger_Abort2(eglGetError(), "Failed to create EGL context");
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
#endif
