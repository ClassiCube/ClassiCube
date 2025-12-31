#include "../_WindowBase.h"
#include "../String_.h"
#include "../Funcs.h"
#include "../Bitmap.h"
#include "../Options.h"
#include "../Errors.h"


/*########################################################################################################################*
*--------------------------------------------------Public implementation--------------------------------------------------*
*#########################################################################################################################*/
void Window_PreInit(void) {

}

void Window_Init(void) {
	DisplayInfo.Width  = 640;
	DisplayInfo.Height = 480;

	DisplayInfo.ScaleX = 1.0f;
	DisplayInfo.ScaleY = 1.0f;
}

void Window_Free(void) { }

static void DoCreateWindow(int width, int height) {
	if (Window_Main.Exists) return;

	Window_Main.Width    = 640;
	Window_Main.Height   = 480;
	Window_Main.Focused  = true;

	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;
}

void Window_Create2D(int width, int height) { DoCreateWindow(width, height); }
void Window_Create3D(int width, int height) { DoCreateWindow(width, height); }

void Window_Destroy(void) { }

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
	Event_RaiseVoid(&WindowEvents.Closing);
}

void Window_ProcessEvents(float delta) {
}

void Gamepads_PreInit(void) { }

void Gamepads_Init(void) { }

void Gamepads_Process(float delta) { }

static void Cursor_GetRawPos(int* x, int* y) {
	*x = 0;
	*y = 0;
}

void Cursor_SetPosition(int x, int y) { 
}

static void Cursor_DoSetVisible(cc_bool visible) {
}

static void ShowDialogCore(const char* title, const char* msg) {
	// TODO
	Platform_LogConst(title);
	Platform_LogConst(msg);
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	// TODO
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	// TODO
	return ERR_NOT_SUPPORTED;
}

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	bmp->width  = width;
	bmp->height = height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { }
void OnscreenKeyboard_SetText(const cc_string* text) { }
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
