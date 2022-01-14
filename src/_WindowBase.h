#include "Window.h"
#include "Input.h"
#include "Event.h"
#include "Logger.h"
#include "Platform.h"

struct _DisplayData DisplayInfo;
struct _WinData WindowInfo;

int Display_ScaleX(int x) { return (int)(x * DisplayInfo.ScaleX); }
int Display_ScaleY(int y) { return (int)(y * DisplayInfo.ScaleY); }

static int cursorPrevX, cursorPrevY;
static cc_bool cursorVisible = true;
/* Gets the position of the cursor in screen or window coordinates. */
static void Cursor_GetRawPos(int* x, int* y);
static void Cursor_DoSetVisible(cc_bool visible);

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
static void ShowDialogCore(const char* title, const char* msg);
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

#ifdef CC_BUILD_GL
/* OpenGL contexts are heavily tied to the window, so for simplicitly are also included here */
/* EGL is window system agnostic, other OpenGL context backends are tied to one windowing system. */

void GLContext_GetAll(const struct DynamicLibSym* syms, int count) {
	int i;
	for (i = 0; i < count; i++) {
		*syms[i].symAddr = GLContext_GetAddress(syms[i].name);
	}
}

/*########################################################################################################################*
*-------------------------------------------------------EGL OpenGL--------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_EGL
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
	static EGLint attribs[19] = {
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

	ctx_context = eglCreateContext(ctx_display, ctx_config, EGL_NO_CONTEXT, context_attribs);
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
#endif
