
#include "Window.h"
#define CC_BUILD_X11 1
#if CC_BUILD_X11
#include "DisplayDevice.h"
#include "ErrorHandler.h"
#include <X11/Xlib.h>
#include <GL/glx.h>

Display* win_display;
Window win_rootWin;
int win_screen;

Window win_handle;
XVisualInfo win_visual;

void Window_Create(Int32 x, Int32 y, Int32 width, Int32 height, STRING_REF String* title, struct DisplayDevice* device);
void Window_GetClipboardText(STRING_TRANSIENT String* value);
void Window_SetClipboardText(STRING_PURE String* value);

bool Window_GetVisible(void);
void Window_SetVisible(bool visible);
void* Window_GetWindowHandle(void);
UInt8 Window_GetWindowState(void);
void Window_SetWindowState(UInt8 value);

void Window_SetBounds(struct Rectangle2D rect);
void Window_SetLocation(struct Point2D point) {
	XMoveWindow(win_display, win_handle, point.X, point.Y);
	Window_ProcessEvents();
}
void Window_SetSize(struct Size2D size);

void Window_SetClientSize(struct Size2D size) {
	XResizeWindow(win_display, win_handle, size.Width, size.Height);
	Window_ProcessEvents();
}

void Window_Close(void);
void Window_ProcessEvents(void);

struct Point2D Window_PointToClient(struct Point2D point) {
	Int32 ox, oy;
	Window child;
	XTranslateCoordinates(win_display, win_rootWin, win_handle, point.X, point.Y, &ox, &oy, &child);
	return Point2D_Make(ox, oy);
}

struct Point2D Window_PointToScreen(struct Point2D point) {
	Int32 ox, oy;
	Window child;
	XTranslateCoordinates(win_display, win_handle, win_rootWin, point.X, point.Y, &ox, &oy, &child);
	return Point2D_Make(ox, oy);
}

struct Point2D Window_GetDesktopCursorPos(void) {
	Window root, child;
	Int32 rootX, rootY, childX, childY, mask;
	XQueryPointer(win_display, win_rootWin, &root, &child, &rootX, &rootY, &childX, &childY, &mask);
	return Point2D_Make(rootX, rootY);
}

void Window_SetDesktopCursorPos(struct Point2D point) {
	XWarpPointer(win_display, NULL, win_rootWin, 0, 0, 0, 0, point.X, point.Y);
	XFlush(win_display); /* TODO: not sure if XFlush call is necessary */
}

bool Window_GetCursorVisible(void);
void Window_SetCursorVisible(bool visible);

GLXContext ctx_Handle;
typedef Int32 (*FN_GLXSWAPINTERVAL)(Int32 interval);
FN_GLXSWAPINTERVAL glXSwapIntervalSGI;
bool ctx_supports_vSync;
Int32 ctx_vsync_interval;

void GLContext_Init(struct GraphicsMode mode) {
	ctx_Handle = glXCreateContext(win_display, &win_visual, NULL, true);

	if (ctx_Handle == NULL) {
		Platform_LogConst("Context create failed. Trying indirect...");
		ctx_Handle = glXCreateContext(win_display, &win_visual, NULL, false);
	}
	if (ctx_Handle == NULL) {
		ErrorHandler_Fail("Failed to create context");
	}

	if (!glXIsDirect(win_display, ctx_Handle)) {
		Platform_LogConst("== WARNING: Context is not direct ==");
	}
	if (!glXMakeCurrent(win_display, win_handle, ctx_Handle)) {
		ErrorHandler_Fail("Failed to make context current.");
	}

	glXSwapIntervalSGI = (FN_GLXSWAPINTERVAL)GLContext_GetAddress("glXSwapIntervalSGI");
	ctx_supports_vSync = glXSwapIntervalSGI != NULL;
}

void GLContext_Update(void) { }
void GLContext_Free(void) {
	if (ctx_Handle == NULL) return;

	if (glXGetCurrentContext() == ctx_Handle) {
		glXMakeCurrent(win_display, NULL, NULL);
	}
	glXDestroyContext(win_display, ctx_Handle);
	ctx_Handle = NULL;
}

void* GLContext_GetAddress(const UChar* function) {
	void* address = glXGetProcAddress(function);
	return GLContext_IsInvalidAddress(address) ? NULL : address;
}

void GLContext_SwapBuffers(void) {
	glXSwapBuffers(win_display, win_handle);
}

bool GLContext_GetVSync(void) {
	return ctx_supports_vSync && ctx_vsync_interval;
}

void GLContext_SetVSync(bool enabled) {
	if (!ctx_supports_vSync) return;

	Int32 result = glXSwapIntervalSGI(enabled);
	if (result != 0) {
		Platform_Log1("Set VSync failed, error: %i", &result);
	}
	ctx_vsync_interval = enabled;
}

static void GLContext_GetAttribs(struct GraphicsMode mode, Int32* attribs) {
	Int32 i = 0;
	struct ColorFormat color = mode.Format;
	// See http://www-01.ibm.com/support/knowledgecenter/ssw_aix_61/com.ibm.aix.opengl/doc/openglrf/glXChooseFBConfig.htm%23glxchoosefbconfig
	// See http://www-01.ibm.com/support/knowledgecenter/ssw_aix_71/com.ibm.aix.opengl/doc/openglrf/glXChooseVisual.htm%23b5c84be452rree
	// for the attribute declarations. Note that the attributes are different than those used in Glx.ChooseVisual.


	if (!color.IsIndexed) {
		attribs[i++] = GLX_RGBA;
	}
	attribs[i++] = GLX_RED_SIZE;   attribs[i++] = color.R;
	attribs[i++] = GLX_GREEN_SIZE; attribs[i++] = color.G;
	attribs[i++] = GLX_BLUE_SIZE;  attribs[i++] = color.B;
	attribs[i++] = GLX_ALPHA_SIZE; attribs[i++] = color.A;

	if (mode.DepthBits > 0) {
		attribs[i++] = GLX_DEPTH_SIZE; attribs[i++] = mode.DepthBits;
	}
	if (mode.StencilBits > 0) {
		attribs[i++] = GLX_STENCIL_SIZE; attribs[i++] = mode.StencilBits;
	}
	if (mode.Buffers > 1) {
		attribs[i++] = GLX_DOUBLEBUFFER;
	}
	attribs[i++] = 0;
}

static XVisualInfo GLContext_SelectVisual(struct GraphicsMode mode) {
	Int32 attribs[20];
	GLContext_GetAttribs(mode, attribs);
	Int32 major = 0, minor = 0, fbcount;
	if (!glXQueryVersion(win_display, &major, &minor)) {
		ErrorHandler_Fail("glXQueryVersion failed");
	}

	XVisualInfo* visual = NULL;
	if (major >= 1 && minor >= 3) {
		/* ChooseFBConfig returns an array of GLXFBConfig opaque structures */
		GLXFBConfig* fbconfigs = glXChooseFBConfig(win_display, win_screen, attribs, &fbcount);
		if (fbcount > 0 && fbconfigs != NULL) {
			/* Use the first GLXFBConfig from the fbconfigs array (best match) */
			visual = glXGetVisualFromFBConfig(win_display, *fbconfigs);
			XFree(fbconfigs);
		}
	}

	if (visual == NULL) {
		Platform_LogConst("Falling back to glXChooseVisual.");
		visual = glXChooseVisual(win_display, win_screen, attribs);
	}
	if (visual == NULL) {
		ErorrHandler_Fail("Requested GraphicsMode not available.");
	}

	XVisualInfo info = *visual;
	XFree(visual);
	return info;
}
#endif
