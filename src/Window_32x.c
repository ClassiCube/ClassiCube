#include "Core.h"
#if defined CC_BUILD_32X
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
#include "../misc/32x/32x.h"
#include "../misc/32x/hw_32x.h"

// framebuffer only 128 kb
#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   200

static cc_bool launcherMode;

struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;

void Window_PreInit(void) { }
void Window_Init(void) {
	DisplayInfo.Width  = SCREEN_WIDTH;
	DisplayInfo.Height = SCREEN_HEIGHT;
	DisplayInfo.ScaleX = 0.5f;
	DisplayInfo.ScaleY = 0.5f;
	
	Window_Main.Width    = DisplayInfo.Width;
	Window_Main.Height   = DisplayInfo.Height;
	Window_Main.Focused  = true;
	
	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;

	DisplayInfo.ContentOffsetX = 10;
	DisplayInfo.ContentOffsetY = 10;
	DisplayInfo.FullRedraw     = true;

	Hw32xInit(MARS_VDP_MODE_32K, 0);
}

void Window_Free(void) { }

void Window_Create2D(int width, int height) {
	Hw32xScreenClear();
	launcherMode = true;
}

void Window_Create3D(int width, int height) {
	Hw32xScreenClear();
	launcherMode = false; 
}

void Window_Destroy(void) { }

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
void Window_ProcessEvents(float delta) {
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for PS Vita

void Window_EnableRawMouse(void)  { Input.RawMode = true; }
void Window_UpdateRawMouse(void)  {  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }


/*########################################################################################################################*
*-------------------------------------------------------Gamepads----------------------------------------------------------*
*#########################################################################################################################*/
void Gamepads_Init(void) {
	Input.Sources |= INPUT_SOURCE_GAMEPAD;
}

void Gamepads_Process(float delta) {
	int port = Gamepad_Connect(0x32, PadBind_Defaults);
	int mods = HwMdReadPad(0);
	
	Gamepad_SetButton(port, CCPAD_L, mods & SEGA_CTRL_X);
	Gamepad_SetButton(port, CCPAD_R, mods & SEGA_CTRL_Y);
	
	Gamepad_SetButton(port, CCPAD_1, mods & SEGA_CTRL_A);
	Gamepad_SetButton(port, CCPAD_2, mods & SEGA_CTRL_B);
	Gamepad_SetButton(port, CCPAD_3, mods & SEGA_CTRL_C);
	Gamepad_SetButton(port, CCPAD_4, mods & SEGA_CTRL_Z);
	
	Gamepad_SetButton(port, CCPAD_START,  mods & SEGA_CTRL_START);
	Gamepad_SetButton(port, CCPAD_SELECT, mods & SEGA_CTRL_MODE);
	
	Gamepad_SetButton(port, CCPAD_LEFT,   mods & SEGA_CTRL_LEFT);
	Gamepad_SetButton(port, CCPAD_RIGHT,  mods & SEGA_CTRL_RIGHT);
	Gamepad_SetButton(port, CCPAD_UP,     mods & SEGA_CTRL_UP);
	Gamepad_SetButton(port, CCPAD_DOWN,   mods & SEGA_CTRL_DOWN);
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
    volatile uint16_t* vram = &MARS_FRAMEBUFFER + 0x100;
	bmp->scan0  = vram;
	bmp->width  = width;
	bmp->height = height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	Hw32xScreenFlip(true);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
}


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { /* TODO implement */ }
void OnscreenKeyboard_SetText(const cc_string* text) { }
void OnscreenKeyboard_Close(void) { /* TODO implement */ }


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
