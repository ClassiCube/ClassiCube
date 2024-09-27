#include "Core.h"
#if defined CC_BUILD_MSDOS

#include "_WindowBase.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Options.h"
#include "Errors.h"
#include <dpmi.h>
#include <sys/nearptr.h>
#include <pc.h>

/*########################################################################################################################*
*--------------------------------------------------Public implementation--------------------------------------------------*
*#########################################################################################################################*/
void Window_PreInit(void) {
}

void Window_Init(void) {
	DisplayInfo.Width  = 320;
	DisplayInfo.Height = 200;

	DisplayInfo.ScaleX = 1.0f;
	DisplayInfo.ScaleY = 1.0f;

	// Change VGA mode to 0x13 (320x200x8bpp)
	__dpmi_regs regs;
	Mem_Set(&regs, 0, sizeof regs);
	regs.x.ax = 0x13;
	__dpmi_int(0x10, &regs);

	// Change VGA colour palette (NOTE: only lower 6 bits are used)
	// Fake a linear RGB palette
	outportb(0x3c8, 0);
	for (int i = 0; i < 256; i++) {
    	outportb(0x3c9, ((i >> 0) & 0x03) << 4);
    	outportb(0x3c9, ((i >> 2) & 0x07) << 3);
    	outportb(0x3c9, ((i >> 5) & 0x07) << 3);	
	}
}

void Window_Free(void) { }

static void DoCreateWindow(int width, int height) {
	Window_Main.Width    = 320;
	Window_Main.Height   = 200;
	Window_Main.Focused  = true;
	
	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;
}

void Window_Create2D(int width, int height) { DoCreateWindow(width, height); }
void Window_Create3D(int width, int height) { DoCreateWindow(width, height); }

void Window_Destroy(void) { }

void Window_SetTitle(const cc_string* title) { }

void Clipboard_GetText(cc_string* value) { }

void Clipboard_SetText(const cc_string* value) { }

int Window_GetWindowState(void) {
	return WINDOW_STATE_NORMAL;
}

cc_result Window_EnterFullscreen(void) {
	return 0;
}

cc_result Window_ExitFullscreen(void) {
	return 0;
}

int Window_IsObscured(void) { return 0; }

void Window_Show(void) { }

void Window_SetSize(int width, int height) { }

void Window_RequestClose(void) {
	Event_RaiseVoid(&WindowEvents.Closing);
}

void Window_ProcessEvents(float delta) { }

void Gamepads_Init(void) {

}

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
	Platform_LogConst(title);
	Platform_LogConst(msg);
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	bmp->width  = width;
	bmp->height = height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	char *screen;
	if (__djgpp_nearptr_enable() == 0) return;

	screen = (char*)0xa0000 + __djgpp_conventional_base;
    for (int y = r.y; y < r.y + r.height; ++y) 
	{
        BitmapCol* row = Bitmap_GetRow(bmp, y);
        for (int x = r.x; x < r.x + r.width; ++x) 
		{
            // TODO optimise
            BitmapCol	col = row[x];
			cc_uint8 R = BitmapCol_R(col);
			cc_uint8 G = BitmapCol_G(col);
			cc_uint8 B = BitmapCol_B(col);

            screen[y*320+x] = (R >> 6) | ((G >> 5) << 2) | ((B >> 5) << 5);
        }
    }

	__djgpp_nearptr_disable();
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
#endif
