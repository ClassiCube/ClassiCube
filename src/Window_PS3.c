#include "Core.h"
#if defined CC_BUILD_PS3
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
#include <io/pad.h>
#include <sysutil/video.h>
static cc_bool launcherMode;
static padInfo pad_info;
static padData pad_data;

struct _DisplayData DisplayInfo;
struct _WinData WindowInfo;
// no DPI scaling on PS Vita
int Display_ScaleX(int x) { return x; }
int Display_ScaleY(int y) { return y; }

void Window_Init(void) {
	videoState state;
	videoResolution resolution;
	
	videoGetState(0, 0, &state);
	videoGetResolution(state.displayMode.resolution, &resolution);
      
	DisplayInfo.Width  = resolution.width;
	DisplayInfo.Height = resolution.height;
	DisplayInfo.Depth  = 4; // 32 bit
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	WindowInfo.Width   = resolution.width;
	WindowInfo.Height  = resolution.height;
	WindowInfo.Focused = true;
	WindowInfo.Exists  = true;

	Input.Sources = INPUT_SOURCE_GAMEPAD;
	ioPadInit(7);
}

void Window_Create2D(int width, int height) { 
	launcherMode = true;
	Gfx_Create(); // launcher also uses RSX to draw
}

void Window_Create3D(int width, int height) { 
	launcherMode = false; 
}

void Window_SetTitle(const cc_string* title) { }
void Clipboard_GetText(cc_string* value) { } // TODO sceClipboardGetText
void Clipboard_SetText(const cc_string* value) { } // TODO sceClipboardSetText

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
static void HandleButtons(padData* data) {
	Input_SetNonRepeatable(CCPAD_A, data->BTN_TRIANGLE);
	Input_SetNonRepeatable(CCPAD_B, data->BTN_SQUARE);
	Input_SetNonRepeatable(CCPAD_X, data->BTN_CROSS);
	Input_SetNonRepeatable(CCPAD_Y, data->BTN_CIRCLE);
      
	Input_SetNonRepeatable(CCPAD_START,  data->BTN_START);
	Input_SetNonRepeatable(CCPAD_SELECT, data->BTN_SELECT);

	Input_SetNonRepeatable(CCPAD_LEFT,   data->BTN_LEFT);
	Input_SetNonRepeatable(CCPAD_RIGHT,  data->BTN_RIGHT);
	Input_SetNonRepeatable(CCPAD_UP,     data->BTN_UP);
	Input_SetNonRepeatable(CCPAD_DOWN,   data->BTN_DOWN);
	
	Input_SetNonRepeatable(CCPAD_L,  data->BTN_L1);
	Input_SetNonRepeatable(CCPAD_R,  data->BTN_R1);
	Input_SetNonRepeatable(CCPAD_ZL, data->BTN_L2);
	Input_SetNonRepeatable(CCPAD_ZR, data->BTN_R2);
}

static void ProcessPadInput(double delta, padData* pad) {
	HandleButtons(pad);
	//if (Input.RawMode)
	//	ProcessCircleInput(&pad, delta);
}

void Window_ProcessEvents(double delta) {
	ioPadGetInfo(&pad_info);
	
	if (pad_info.status[0]) {
		ioPadGetData(0, &pad_data);
		ProcessPadInput(delta, &pad_data);
	}
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for PS Vita

void Window_EnableRawMouse(void)  { Input.RawMode = true; }
void Window_UpdateRawMouse(void)  {  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
static struct Bitmap fb_bmp;
static u32 fb_offset;

extern u32* Gfx_AllocImage(u32* offset, s32 w, s32 h);
extern void Gfx_TransferImage(u32 offset, s32 w, s32 h);

void Window_AllocFramebuffer(struct Bitmap* bmp) {
	u32* pixels = Gfx_AllocImage(&fb_offset, bmp->width, bmp->height);
	bmp->scan0  = pixels;
	fb_bmp      = *bmp;
}

void Window_DrawFramebuffer(Rect2D r) {
	// TODO test
	Thread_Sleep(1000);
	Platform_Log1("FRAME START (%h)", &fb_offset);
	Gfx_BeginFrame();
	Gfx_ClearCol(PackedCol_Make(0x40, 0x60, 0x80, 0xFF));
	Gfx_Clear();
	Gfx_TransferImage(fb_offset, fb_bmp.width, fb_bmp.height);
	Gfx_EndFrame();
	Platform_LogConst("FRAME END");
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	//Mem_Free(bmp->scan0);
	/* TODO free framebuffer */
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