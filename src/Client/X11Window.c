
#include "Window.h"
#define CC_BUILD_X11 1
#if CC_BUILD_X11
#include "DisplayDevice.h"
#include "ErrorHandler.h"
#include "Input.h"
#include <X11/Xlib.h>
#include <GL/glx.h>

Display* win_display;
Window win_rootWin;
int win_screen;

Window win_handle;
XVisualInfo win_visual;

static Key Window_MapKey(int key) {
	if (key >= XK_F1 && key <= XK_F35) { return Key_F1 + (key - XK_F1); }
	if (key >= XK_0 && key <= XK_9) { return Key_0 + (key - XK_0); }
	if (key >= XK_A && key <= XK_Z) { return Key_A + (key - XK_A); }
	if (key >= XK_a && key <= XK_z) { return Key_A + (key - XK_z); }

	if (key >= XK_KP_0 && key <= XK_KP_9) {
		return Key_Keypad0 + (key - XK_KP_9);
	}

	switch (key) {
		case XK_Escape: return Key_Escape;
		case XK_Return: return Key_Enter;
		case XK_space: return Key_Space;
		case XK_BackSpace: return Key_BackSpace;

		case XK_Shift_L: return Key_ShiftLeft;
		case XK_Shift_R: return Key_ShiftRight;
		case XK_Alt_L: return Key_AltLeft;
		case XK_Alt_R: return Key_AltRight;
		case XK_Control_L: return Key_ControlLeft;
		case XK_Control_R: return Key_ControlRight;
		case XK_Super_L: return Key_WinLeft;
		case XK_Super_R: return Key_WinRight;
		case XK_Meta_L: return Key_WinLeft;
		case XK_Meta_R: return Key_WinRight;

		case XK_Menu: return Key_Menu;
		case XK_Tab: return Key_Tab;
		case XK_minus: return Key_Minus;
		case XK_plus: return Key_Plus;
		case XK_equal: return Key_Plus;

		case XK_Caps_Lock: return Key_CapsLock;
		case XK_Num_Lock: return Key_NumLock;

		case XK_Pause: return Key_Pause;
		case XK_Break: return Key_Pause;
		case XK_Scroll_Lock: return Key_Pause;
		case XK_Insert: return Key_PrintScreen;
		case XK_Print: return Key_PrintScreen;
		case XK_Sys_Req: return Key_PrintScreen;

		case XK_backslash: return Key_BackSlash;
		case XK_bar: return Key_BackSlash;
		case XK_braceleft: return Key_BracketLeft;
		case XK_bracketleft: return Key_BracketLeft;
		case XK_braceright: return Key_BracketRight;
		case XK_bracketright: return Key_BracketRight;
		case XK_colon: return Key_Semicolon;
		case XK_semicolon: return Key_Semicolon;
		case XK_quoteright: return Key_Quote;
		case XK_quotedbl: return Key_Quote;
		case XK_quoteleft: return Key_Tilde;
		case XK_asciitilde: return Key_Tilde;

		case XK_comma: return Key_Comma;
		case XK_less: return Key_Comma;
		case XK_period: return Key_Period;
		case XK_greater: return Key_Period;
		case XK_slash: return Key_Slash;
		case XK_question: return Key_Slash;

		case XK_Left: return Key_Left;
		case XK_Down: return Key_Down;
		case XK_Right: return Key_Right;
		case XK_Up: return Key_Up;

		case XK_Delete: return Key_Delete;
		case XK_Home: return Key_Home;
		case XK_End: return Key_End;
		case XK_Page_Up: return Key_PageUp;
		case XK_Page_Down: return Key_PageDown;

		case XK_KP_Add: return Key_KeypadAdd;
		case XK_KP_Subtract: return Key_KeypadSubtract;
		case XK_KP_Multiply: return Key_KeypadMultiply;
		case XK_KP_Divide: return Key_KeypadDivide;
		case XK_KP_Decimal: return Key_KeypadDecimal;
		case XK_KP_Insert: return Key_Keypad0;
		case XK_KP_End: return Key_Keypad1;
		case XK_KP_Down: return Key_Keypad2;
		case XK_KP_Page_Down: return Key_Keypad3;
		case XK_KP_Left: return Key_Keypad4;
		case XK_KP_Right: return Key_Keypad6;
		case XK_KP_Home: return Key_Keypad7;
		case XK_KP_Up: return Key_Keypad8;
		case XK_KP_Page_Up: return Key_Keypad9;
		case XK_KP_Delete: return Key_KeypadDecimal;
		case XK_KP_Enter: return Key_KeypadEnter;
	}
	return Key_None;
}

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

Cursor win_blankCursor;
bool win_cursorVisible = true;
bool Window_GetCursorVisible(void) { return win_cursorVisible; }
void Window_SetCursorVisible(bool visible) {
	win_cursorVisible = visible;
	if (visible) {
		XUndefineCursor(win_display, win_handle);
	} else {
		if (win_blankCursor == NULL) {
			XColor col = { 0 };
			Pixmap pixmap = XCreatePixmap(win_display, win_rootWindow, 1, 1, 1);
			win_blankCursor = XCreatePixmapCursor(win_display, pixmap, pixmap, &col, &col, 0, 0);
			XFreePixmap(win_display, pixmap);
		}
		XDefineCursor(win_display, win_handle, win_blankCursor);
	}
}

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
