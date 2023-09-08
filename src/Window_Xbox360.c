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
#include <xenos/xenos.h>
#include <input/input.h>
#include <usb/usbmain.h>

static cc_bool launcherMode;

struct _DisplayData DisplayInfo;
struct _WinData WindowInfo;
// no DPI scaling on Xbox
int Display_ScaleX(int x) { return x; }
int Display_ScaleY(int y) { return y; }

// https://github.com/Free60Project/libxenon/blob/71a411cddfc26c9ccade08d054d87180c359797a/libxenon/drivers/console/console.c#L47
struct ati_info {
	uint32_t unknown1[4];
	uint32_t base;
	uint32_t unknown2[8];
	uint32_t width;
	uint32_t height;
} __attribute__ ((__packed__)) ;

void Window_Init(void) {
	struct ati_info* ai = (struct ati_info*)0xec806100ULL;
	
	DisplayInfo.Width  = ai->width;
	DisplayInfo.Height = ai->height;
	DisplayInfo.Depth  = 4; // 32 bit
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	WindowInfo.Width   = ai->width;
	WindowInfo.Height  = ai->height;
	WindowInfo.Focused = true;
	WindowInfo.Exists  = true;

	Input.GamepadSource = true;
	
	usb_init();
	usb_do_poll();
	
	//xenon_ata_init();
	//xenon_atapi_init();
}

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

void Window_Close(void) {
	/* TODO implement */
}


/*########################################################################################################################*
*----------------------------------------------------Input processing-----------------------------------------------------*
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

static void HandleButtons(struct controller_data_s* pad) {
	Input_SetNonRepeatable(CCPAD_L, pad->lb);
	Input_SetNonRepeatable(CCPAD_R, pad->rb);
	
	Input_SetNonRepeatable(CCPAD_A, pad->a);
	Input_SetNonRepeatable(CCPAD_B, pad->b);
	Input_SetNonRepeatable(CCPAD_X, pad->x);
	Input_SetNonRepeatable(CCPAD_Y, pad->y);
	
	Input_SetNonRepeatable(CCPAD_START,  pad->start);
	Input_SetNonRepeatable(CCPAD_SELECT, pad->back);
	
	Input_SetNonRepeatable(CCPAD_LEFT,   pad->left);
	Input_SetNonRepeatable(CCPAD_RIGHT,  pad->right);
	Input_SetNonRepeatable(CCPAD_UP,     pad->up);
	Input_SetNonRepeatable(CCPAD_DOWN,   pad->down);
}

void Window_ProcessEvents(double delta) {
	usb_do_poll();
	
	struct controller_data_s pad;
 	int res = get_controller_data(&pad, 0);
 	if (res == 0) return;
 	
 	HandleButtons(&pad);
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for Xbox

void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }
void Window_UpdateRawMouse(void)  { }


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
static struct Bitmap fb_bmp;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	fb_bmp = *bmp;
}

void Window_DrawFramebuffer(Rect2D r) {return;
	//void* fb = XVideoGetFB();
	//XVideoWaitForVBlank();

	/*cc_uint32* src = (cc_uint32*)fb_bmp.scan0 + r.X;
	cc_uint32* dst = (cc_uint32*)fb           + r.X;

	for (int y = r.Y; y < r.Y + r.Height; y++) 
	{
		Mem_Copy(dst + y * fb_bmp.width, src + y * fb_bmp.width, r.Width * 4);
	}*/
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
void Window_OpenKeyboard(struct OpenKeyboardArgs* args) { }
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { /* TODO implement */ }


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
