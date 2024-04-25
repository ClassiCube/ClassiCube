#include "Core.h"
#if defined CC_BUILD_XBOX360
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
#include "VirtualKeyboard.h"
#include <xenos/xenos.h>
#include <input/input.h>
#include <usb/usbmain.h>
#include <pci/io.h>

static cc_bool launcherMode;

struct _DisplayData DisplayInfo;
struct _WindowData WindowInfo;

static uint32_t reg_read32(int reg)
{
	return read32n(0xec800000 + reg);
}

void Window_Init(void) {
	DisplayInfo.Width  = reg_read32(D1GRPH_X_END);
	DisplayInfo.Height = reg_read32(D1GRPH_Y_END);
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	Window_Main.Width   = DisplayInfo.Width;
	Window_Main.Height  = DisplayInfo.Height;
	Window_Main.Focused = true;
	Window_Main.Exists  = true;

	Input.Sources = INPUT_SOURCE_GAMEPAD;
	
	usb_init();
	usb_do_poll();
	
	//xenon_ata_init();
	//xenon_atapi_init();
}

void Window_Free(void) { }

void Window_Create2D(int width, int height) { launcherMode = true;  }
void Window_Create3D(int width, int height) { launcherMode = false; }

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
	/* TODO implement */
}


/*########################################################################################################################*
*----------------------------------------------------Input processing-----------------------------------------------------*
*#########################################################################################################################*/
void Window_ProcessEvents(double delta) {
	usb_do_poll();
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for Xbox

void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }
void Window_UpdateRawMouse(void)  { }


/*########################################################################################################################*
*-------------------------------------------------------Gamepads----------------------------------------------------------*
*#########################################################################################################################*/
/*
struct controller_data_s
{
	signed short s1_x, s1_y, s2_x, s2_y;
	int s1_z, s2_z, lb, rb, start, back, a, b, x, y, up, down, left, right;
	unsigned char lt, rt;
	int logo;
};
*/

static void HandleButtons(int port, struct controller_data_s* pad) {
	Gamepad_SetButton(port, CCPAD_L, pad->lb);
	Gamepad_SetButton(port, CCPAD_R, pad->rb);
	
	Gamepad_SetButton(port, CCPAD_A, pad->a);
	Gamepad_SetButton(port, CCPAD_B, pad->b);
	Gamepad_SetButton(port, CCPAD_X, pad->x);
	Gamepad_SetButton(port, CCPAD_Y, pad->y);
	
	Gamepad_SetButton(port, CCPAD_START,  pad->start);
	Gamepad_SetButton(port, CCPAD_SELECT, pad->back);
	
	Gamepad_SetButton(port, CCPAD_LEFT,   pad->left);
	Gamepad_SetButton(port, CCPAD_RIGHT,  pad->right);
	Gamepad_SetButton(port, CCPAD_UP,     pad->up);
	Gamepad_SetButton(port, CCPAD_DOWN,   pad->down);
}

void Window_ProcessGamepads(double delta) {
	struct controller_data_s pad;
	int res = get_controller_data(&pad, 0);
	if (res == 0) return;
	
	HandleButtons(0, &pad);
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	// https://github.com/Free60Project/libxenon/blob/master/libxenon/drivers/console/console.c#L166
	// https://github.com/Free60Project/libxenon/blob/master/libxenon/drivers/console/console.c#L57
	uint32_t* fb = (uint32_t*)(reg_read32(D1GRPH_PRIMARY_SURFACE_ADDRESS) | 0x80000000);
	/* round up size to tiles of 32x32 */
	int width = ((DisplayInfo.Width + 31) >> 5) << 5;
	
#define FB_INDEX(x, y) (((y >> 5)*32*width + ((x >> 5)<<10) + (x&3) + ((y&1)<<2) + (((x&31)>>2)<<3) + (((y&31)>>1)<<6)) ^ ((y&8)<<2))

	for (int y = r.y; y < r.y + r.height; y++) 
	{
		cc_uint32* src = bmp->scan0 + y * bmp->width;
		
		for (int x = r.x; x < r.x + r.width; x++) {
			// TODO: Can the uint be copied directly ?
			int R = BitmapCol_R(src[x]);
			int G = BitmapCol_G(src[x]);
			int B = BitmapCol_B(src[x]);
			
			fb[FB_INDEX(x, y)] = (B << 24) | (G << 16) | (R << 8) | 0xFF;
		}
	}
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) {
	if (Input.Sources & INPUT_SOURCE_NORMAL) return;
	VirtualKeyboard_Open(args, launcherMode);
}

void OnscreenKeyboard_SetText(const cc_string* text) {
	VirtualKeyboard_SetText(text);
}

void OnscreenKeyboard_Draw2D(Rect2D* r, struct Bitmap* bmp) {
	VirtualKeyboard_Display2D(r, bmp);
}

void OnscreenKeyboard_Draw3D(void) {
	VirtualKeyboard_Display3D();
}

void OnscreenKeyboard_Close(void) {
	VirtualKeyboard_Close();
}


/*########################################################################################################################*
*-------------------------------------------------------Misc/Other--------------------------------------------------------*
*#########################################################################################################################*/
void Window_ShowDialog(const char* title, const char* msg) {
	/* TODO implement */
	Platform_LogConst(title);
	Platform_LogConst(msg);
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}
#endif
