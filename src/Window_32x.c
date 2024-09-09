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

// framebuffer only 128 kb
#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   204

static cc_bool launcherMode;

struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;

static void OnVblank(void* work) {
}

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
}

void Window_Free(void) { }

void Window_Create3D(int width, int height) { 
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
}

void Gamepads_Process(float delta) {
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_Create2D(int width, int height) {
	launcherMode = true;
}

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = NULL; // TODO
	bmp->width  = width;
	bmp->height = height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	uint16_t* vram = NULL;

	// TODO: Partial redraws seem to produce some corrupt pixels ???
	for (int y = r.y; y < r.y + r.height; y++) 
	{
		BitmapCol* row = Bitmap_GetRow(bmp, y);
		for (int x = r.x; x < r.x + r.width; x++) 
		{
			// TODO optimise
			vram[x + (y * 512)] = row[x];
		}
	}
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
