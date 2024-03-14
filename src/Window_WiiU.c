#include "Core.h"
#if defined CC_BUILD_WIIU
#include "Window.h"
#include "Platform.h"
#include "Input.h"
#include "Event.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Errors.h"
#include "ExtMath.h"
#include "Graphics.h"
#include <coreinit/memheap.h>
#include <coreinit/cache.h>
#include <coreinit/memfrmheap.h>
#include <coreinit/memory.h>
#include <coreinit/screen.h>
#include <proc_ui/procui.h>
#include <gx2/display.h>
#include <vpad/input.h>

static cc_bool launcherMode;
struct _DisplayData DisplayInfo;
struct _WindowData WindowInfo;

void Window_Init(void) {
	switch(GX2GetSystemTVScanMode())
	{
	case GX2_TV_SCAN_MODE_480I:
	case GX2_TV_SCAN_MODE_480P:
		DisplayInfo.Width  = 854;
		DisplayInfo.Height = 480;
		break;
	case GX2_TV_SCAN_MODE_1080I:
	case GX2_TV_SCAN_MODE_1080P:
		DisplayInfo.Width  = 1920;
		DisplayInfo.Height = 1080;
		break;
	case GX2_TV_SCAN_MODE_720P:
	default:
		DisplayInfo.Width  = 1280;
		DisplayInfo.Height =  720;
	break;
	}
	
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	Window_Main.Width   = DisplayInfo.Width;
	Window_Main.Height  = DisplayInfo.Height;
	Window_Main.Focused = true;
	Window_Main.Exists  = true;

	Input.Sources = INPUT_SOURCE_GAMEPAD;
	DisplayInfo.ContentOffsetX = 10;
	DisplayInfo.ContentOffsetY = 10;
	
	VPADInit();
}

void Window_Free(void) { }

void Window_Create2D(int width, int height) {
	// well this works in CEMU at least
	Window_Main.Width   = 1280;
	Window_Main.Height  =  720;
	launcherMode  = true;  
}

void Window_Create3D(int width, int height) { 
	Window_Main.Width   = DisplayInfo.Width;
	Window_Main.Height  = DisplayInfo.Height;
	launcherMode = false; 
}

void Window_RequestClose(void) {
	Event_RaiseVoid(&WindowEvents.Closing);
}
   
static void ProcessVpadButtons(int mods) {
	Input_SetNonRepeatable(CCPAD_L,  mods & VPAD_BUTTON_L);
	Input_SetNonRepeatable(CCPAD_R,  mods & VPAD_BUTTON_R);
	Input_SetNonRepeatable(CCPAD_ZL, mods & VPAD_BUTTON_ZL);
	Input_SetNonRepeatable(CCPAD_ZR, mods & VPAD_BUTTON_ZR);
      
	Input_SetNonRepeatable(CCPAD_A, mods & VPAD_BUTTON_A);
	Input_SetNonRepeatable(CCPAD_B, mods & VPAD_BUTTON_B);
	Input_SetNonRepeatable(CCPAD_X, mods & VPAD_BUTTON_X);
	Input_SetNonRepeatable(CCPAD_Y, mods & VPAD_BUTTON_Y);
      
	Input_SetNonRepeatable(CCPAD_START,  mods & VPAD_BUTTON_PLUS);
	Input_SetNonRepeatable(CCPAD_SELECT, mods & VPAD_BUTTON_MINUS);

	Input_SetNonRepeatable(CCPAD_LEFT,   mods & VPAD_BUTTON_LEFT);
	Input_SetNonRepeatable(CCPAD_RIGHT,  mods & VPAD_BUTTON_RIGHT);
	Input_SetNonRepeatable(CCPAD_UP,     mods & VPAD_BUTTON_UP);
	Input_SetNonRepeatable(CCPAD_DOWN,   mods & VPAD_BUTTON_DOWN);
	
}

void Window_ProcessEvents(double delta) {
	Input.JoystickMovement = false;
	
	if (!WHBProcIsRunning()) {
		Window_Main.Exists = false;
		Window_RequestClose();
		return;
	}
	
	VPADStatus vpadStatus;
	VPADReadError error = VPAD_READ_SUCCESS;
	VPADRead(VPAD_CHAN_0, &vpadStatus, 1, &error);
	if (error != VPAD_READ_SUCCESS) return;
	
	VPADGetTPCalibratedPoint(VPAD_CHAN_0, &vpadStatus.tpNormal, &vpadStatus.tpNormal);
	ProcessVpadButtons(vpadStatus.hold);
}

void Window_UpdateRawMouse(void) { }

void Cursor_SetPosition(int x, int y) { }
void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	WHBLogConsoleInit();  	
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	//OSScreenClearBufferEx(SCREEN_TV, 0x665544FF);
   
	for (int y = r.y; y < r.y + r.Height; y++) 
	{
		cc_uint32* src = bmp->scan0 + y * bmp->width;
		
		for (int x = r.x; x < r.x + r.Width; x++) {
			OSScreenPutPixelEx(SCREEN_TV, x, y, src[x]);
		}
	}

	//DCFlushRange(sBufferTV, sBufferSizeTV);
	OSScreenFlipBuffersEx(SCREEN_TV);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
	WHBLogConsoleFree();
}


/*########################################################################################################################*
*-------------------------------------------------------Misc/Other--------------------------------------------------------*
*#########################################################################################################################*/
void Window_SetTitle(const cc_string* title)   { }
void Clipboard_GetText(cc_string* value)       { }
void Clipboard_SetText(const cc_string* value) { }

int Window_GetWindowState(void) { return WINDOW_STATE_FULLSCREEN; }
cc_result Window_EnterFullscreen(void) { return 0; }
cc_result Window_ExitFullscreen(void)  { return 0; }
int Window_IsObscured(void)            { return 0; }

void Window_Show(void) { }
void Window_SetSize(int width, int height) { }


void Window_OpenKeyboard(struct OpenKeyboardArgs* args) { /* TODO implement */ }
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { /* TODO implement */ }

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