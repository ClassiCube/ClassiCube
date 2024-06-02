#include "Core.h"
#if defined CC_BUILD_MACCLASSIC

#include "_WindowBase.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Options.h"
#include "Errors.h"

#undef true
#undef false
#include <Quickdraw.h>
#include <Dialogs.h>
#include <Fonts.h>
static WindowPtr win;


/*########################################################################################################################*
*--------------------------------------------------Public implementation--------------------------------------------------*
*#########################################################################################################################*/
void Window_PreInit(void) { }
void Window_Init(void) {
	InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
}

void Window_Free(void) { }

static void DoCreateWindow(int width, int height) {
	Rect r = qd.screenBits.bounds;
	/* TODO: Make less-crap method of getting center. */
	int centerX = r.right/2;	int centerY = r.bottom/2;
	int ww = (width/2);			int hh = (height/2);

	// TODO
    SetRect(&r, centerX-ww, centerY-hh, centerX+ww, centerY+hh);
    win = NewWindow(NULL, &r, "\pClassiCube", true, 0, (WindowPtr)-1, false, 0);
	SetPort(win);
}

void Window_Create2D(int width, int height) { DoCreateWindow(width, height); }
void Window_Create3D(int width, int height) { DoCreateWindow(width, height); }

void Window_SetTitle(const cc_string* title) {
	// TODO
}

void Clipboard_GetText(cc_string* value) {
	// TODO
}

void Clipboard_SetText(const cc_string* value) {
	// TODO
}

int Window_GetWindowState(void) {
	return WINDOW_STATE_NORMAL;
	// TODO
}

cc_result Window_EnterFullscreen(void) {
	// TODO
	return 0;
}

cc_result Window_ExitFullscreen(void) {
	// TODO
	return 0;
}

int Window_IsObscured(void) { return 0; }

void Window_Show(void) {
	// TODO
}

void Window_SetSize(int width, int height) {
	// TODO
}

void Window_RequestClose(void) {
	// TODO
}

void Window_ProcessEvents(float delta) {
	// TODO
}

void Window_ProcessGamepads(float delta) { }

static void Cursor_GetRawPos(int* x, int* y) {
	// TODO
*x=0;*y=0;
}

void Cursor_SetPosition(int x, int y) { 
	// TODO
}
static void Cursor_DoSetVisible(cc_bool visible) {
	// TODO
}

static void ShowDialogCore(const char* title, const char* msg) {
	// TODO
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	// TODO
	return 1;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	// TODO
	return 1;
}

void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	// TODO
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { }
void OnscreenKeyboard_SetText(const cc_string* text) { }
void OnscreenKeyboard_Draw2D(Rect2D* r, struct Bitmap* bmp) { }
void OnscreenKeyboard_Draw3D(void) { }
void OnscreenKeyboard_Close(void) { }

void Window_EnableRawMouse(void) {
	DefaultEnableRawMouse();
}

void Window_UpdateRawMouse(void) {
	DefaultUpdateRawMouse();
}

void Window_DisableRawMouse(void) { 
	DefaultDisableRawMouse();
}


/*########################################################################################################################*
*-------------------------------------------------------WGL OpenGL--------------------------------------------------------*
*#########################################################################################################################*/
#if (CC_GFX_BACKEND == CC_GFX_BACKEND_GL) && !defined CC_BUILD_EGL
void GLContext_Create(void) {
	// TODO
}

void GLContext_Update(void) { }
cc_bool GLContext_TryRestore(void) { return true; }
void GLContext_Free(void) {
	// TODO
}

void* GLContext_GetAddress(const char* function) {
	return NULL;
}

cc_bool GLContext_SwapBuffers(void) {
	// TODO
	return true;
}

void GLContext_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	// TODO
}
void GLContext_GetApiInfo(cc_string* info) { }
#endif
#endif
