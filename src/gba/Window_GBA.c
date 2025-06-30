#include "../Window.h"
#include "../Platform.h"
#include "../Input.h"
#include "../Event.h"
#include "../Graphics.h"
#include "../String.h"
#include "../Funcs.h"
#include "../Bitmap.h"
#include "../Errors.h"
#include "../ExtMath.h"
#include "../Camera.h"

#include <stdint.h>
#include "gbadefs.h"

#define SCREEN_WIDTH	240
#define SCREEN_HEIGHT	160

/*########################################################################################################################*
*------------------------------------------------------General data-------------------------------------------------------*
*#########################################################################################################################*/
struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;

void Window_PreInit(void) {
	REG_DISP_CTRL = DCTRL_MODE3 | DCTRL_BG2;
}

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
}

void Window_Free(void) { }

void Window_Create2D(int width, int height) {
}

void Window_Create3D(int width, int height) {
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

void Cursor_SetPosition(int x, int y) { } // Makes no sense for PSP
void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }

void Window_UpdateRawMouse(void)  { }


/*########################################################################################################################*
*-------------------------------------------------------Gamepads----------------------------------------------------------*
*#########################################################################################################################*/	
void Gamepads_Init(void) {
	Input.Sources |= INPUT_SOURCE_GAMEPAD;
}

void Gamepads_Process(float delta) {
	int port = Gamepad_Connect(0x5BA, PadBind_Defaults);
	int mods = ~REG_KEYINPUT;
	
	Gamepad_SetButton(port, CCPAD_L, mods & KEY_L);
	Gamepad_SetButton(port, CCPAD_R, mods & KEY_R);
	
	Gamepad_SetButton(port, CCPAD_1, mods & KEY_A);
	Gamepad_SetButton(port, CCPAD_2, mods & KEY_B);
	//Gamepad_SetButton(port, CCPAD_3, mods & KEY_X);
	//Gamepad_SetButton(port, CCPAD_4, mods & KEY_Y);
	
	Gamepad_SetButton(port, CCPAD_START,  mods & KEY_START);
	Gamepad_SetButton(port, CCPAD_SELECT, mods & KEY_SELECT);
	
	Gamepad_SetButton(port, CCPAD_LEFT,   mods & KEY_LEFT);
	Gamepad_SetButton(port, CCPAD_RIGHT,  mods & KEY_RIGHT);
	Gamepad_SetButton(port, CCPAD_UP,     mods & KEY_UP);
	Gamepad_SetButton(port, CCPAD_DOWN,   mods & KEY_DOWN);
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)MEM_VRAM;
	bmp->width  = width;
	bmp->height = height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	// Draws to screen directly anyways
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) {
}

void OnscreenKeyboard_SetText(const cc_string* text) { }

void OnscreenKeyboard_Close(void) {
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

