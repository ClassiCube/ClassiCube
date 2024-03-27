#include "Core.h"
#if defined CC_BUILD_PS1
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
#include "Logger.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <psxapi.h>
#include <psxetc.h>
#include <psxgte.h>
#include <psxgpu.h>
#include <psxpad.h>

#define SCREEN_XRES	320
#define SCREEN_YRES	240

static cc_bool launcherMode;
static char pad_buff[2][34];

struct _DisplayData DisplayInfo;
struct _WindowData WindowInfo;

void Window_Init(void) {
	DisplayInfo.Width  = SCREEN_XRES;
	DisplayInfo.Height = SCREEN_YRES;
	DisplayInfo.Depth  = 4; // 32 bit
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	Window_Main.Width   = DisplayInfo.Width;
	Window_Main.Height  = DisplayInfo.Height;
	Window_Main.Focused = true;
	Window_Main.Exists  = true;

	Input.Sources = INPUT_SOURCE_GAMEPAD;
	DisplayInfo.ContentOffsetX = 10;
	DisplayInfo.ContentOffsetY = 10;
	
// http://lameguy64.net/tutorials/pstutorials/chapter1/4-controllers.html
	InitPAD(&pad_buff[0][0], 34, &pad_buff[1][0], 34);
	pad_buff[0][0] = pad_buff[0][1] = 0xff;
	pad_buff[1][0] = pad_buff[1][1] = 0xff;
	StartPAD();
	ChangeClearPAD(0);
}

void Window_Free(void) { }

void Window_Create3D(int width, int height) { 
	launcherMode = false; 
}

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
	Event_RaiseVoid(&WindowEvents.Closing);
}


/*########################################################################################################################*
*----------------------------------------------------Input processing-----------------------------------------------------*
*#########################################################################################################################*/
static void HandleButtons(int buttons) {
	// Confusingly, it seems that when a bit is on, it means the button is NOT pressed
	// So just flip the bits to make more sense
	buttons = ~buttons;
	
	Input_SetNonRepeatable(CCPAD_A, buttons & PAD_TRIANGLE);
	Input_SetNonRepeatable(CCPAD_B, buttons & PAD_SQUARE);
	Input_SetNonRepeatable(CCPAD_X, buttons & PAD_CROSS);
	Input_SetNonRepeatable(CCPAD_Y, buttons & PAD_CIRCLE);
      
	Input_SetNonRepeatable(CCPAD_START,  buttons & PAD_START);
	Input_SetNonRepeatable(CCPAD_SELECT, buttons & PAD_SELECT);

	Input_SetNonRepeatable(CCPAD_LEFT,   buttons & PAD_LEFT);
	Input_SetNonRepeatable(CCPAD_RIGHT,  buttons & PAD_RIGHT);
	Input_SetNonRepeatable(CCPAD_UP,     buttons & PAD_UP);
	Input_SetNonRepeatable(CCPAD_DOWN,   buttons & PAD_DOWN);
	
	Input_SetNonRepeatable(CCPAD_L,  buttons & PAD_L1);
	Input_SetNonRepeatable(CCPAD_R,  buttons & PAD_R1);
	Input_SetNonRepeatable(CCPAD_ZL, buttons & PAD_L2);
	Input_SetNonRepeatable(CCPAD_ZR, buttons & PAD_R2);
}

static void ProcessPadInput(PADTYPE* pad, double delta) {
	HandleButtons(pad->btn);
}

void Window_ProcessEvents(double delta) {
	PADTYPE* pad = (PADTYPE*)&pad_buff[0][0];
	if (pad->stat == 0) ProcessPadInput(pad, delta);
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for PS Vita

void Window_EnableRawMouse(void)  { Input.RawMode = true; }
void Window_UpdateRawMouse(void)  {  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_Create2D(int width, int height) {
	launcherMode = true;
}

static DISPENV disp;
static cc_uint16* fb;

void Window_AllocFramebuffer(struct Bitmap* bmp) {
	SetDefDispEnv(&disp, 0, 0, SCREEN_XRES, SCREEN_YRES);
	disp.isinter = 1;

	PutDispEnv(&disp);
	SetDispMask(1);

	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	fb         = 			 Mem_Alloc(bmp->width * bmp->height, 2, "real surface");
}

#define BGRA8_to_PS1(src) \
	((src[2] & 0xF8) >> 3) | ((src[1] & 0xF8) << 2) | ((src[0] & 0xF8) << 7) | 0x8000

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	RECT rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = SCREEN_XRES;
	rect.h = SCREEN_YRES;

	HeapUsage usage;
	GetHeapUsage(&usage);

	Platform_Log4("%i / %i / %i / %i", &usage.total, &usage.heap, &usage.stack, &usage.alloc);


	for (int y = 0; y < bmp->height; y++)
	{
		cc_uint32* src = bmp->scan0 + y * bmp->width;
		cc_uint16* dst = fb         + y * bmp->width;
		
		for (int x = 0; x < bmp->width; x++) {
			cc_uint8* color = (cc_uint8*)&src[x];
			dst[x] = BGRA8_to_PS1(0);
		}
	}

	Platform_Log4("%i / %i / %i / %i", &usage.total, &usage.heap, &usage.stack, &usage.alloc);

	LoadImage(&rect, bmp->scan0);
	DrawSync(0);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
	Mem_Free(fb);
}


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
void Window_OpenKeyboard(struct OpenKeyboardArgs* args) { /* TODO implement */ }
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
