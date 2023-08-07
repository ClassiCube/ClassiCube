#include "Core.h"
#if defined CC_BUILD_DREAMCAST
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
#include <kos.h>
static cc_bool launcherMode;

struct _DisplayData DisplayInfo;
struct _WinData WindowInfo;
// no DPI scaling on 3DS
int Display_ScaleX(int x) { return x; }
int Display_ScaleY(int y) { return y; }


// Note from https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/gfx.h
//  * Please note that the 3DS uses *portrait* screens rotated 90 degrees counterclockwise.
//  * Width/height refer to the physical dimensions of the screen; that is, the top screen
//  * is 240 pixels wide and 400 pixels tall; while the bottom screen is 240x320.
void Window_Init(void) {
	DisplayInfo.Width  = vid_mode->width;
	DisplayInfo.Height = vid_mode->height;
	DisplayInfo.Depth  = 4; // 32 bit
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	WindowInfo.Width   = vid_mode->width;
	WindowInfo.Height  = vid_mode->height;
	WindowInfo.Focused = true;
	WindowInfo.Exists  = true;
	
	// TODO: So wasteful!!!!!
	vid_set_mode(vid_mode->generic, PM_RGB0888);
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
static void ProcessControllerInput(void) {
	maple_device_t* cont;
	cont_state_t* state;

	cont  = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
	if (!cont)  return;
	state = (cont_state_t*)maple_dev_status(cont);
	if (!state) return;
	int mods = state->buttons;
	// TODO: https://github.com/KallistiOS/KallistiOS/blob/90e09d81d7c1f9dc3f31290a8fff94e4d5ff304a/kernel/arch/dreamcast/include/dc/maple/controller.h#L41
	//Input_SetNonRepeatable(CCPAD_L, mods & CONT_L);
	//Input_SetNonRepeatable(CCPAD_R, mods & CONT_R);
      
	Input_SetNonRepeatable(CCPAD_A, mods & CONT_A);
	Input_SetNonRepeatable(CCPAD_B, mods & CONT_B);
	Input_SetNonRepeatable(CCPAD_X, mods & CONT_X);
	Input_SetNonRepeatable(CCPAD_Y, mods & CONT_Y);
      
	Input_SetNonRepeatable(CCPAD_START,  mods & CONT_START);
	Input_SetNonRepeatable(CCPAD_SELECT, mods & CONT_D);
	Input_SetNonRepeatable(CCPAD_LEFT,   mods & CONT_DPAD_LEFT);
	Input_SetNonRepeatable(CCPAD_RIGHT,  mods & CONT_DPAD_RIGHT);
	Input_SetNonRepeatable(CCPAD_UP,     mods & CONT_DPAD_UP);
	Input_SetNonRepeatable(CCPAD_DOWN,   mods & CONT_DPAD_DOWN);
}

void Window_ProcessEvents(double delta) {
	maple_device_t* cont;
	cont_state_t* state;

	cont  = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
	if (!cont)  return;
	state = (cont_state_t*)maple_dev_status(cont);
	if (!state) return;
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for Dreamcast

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

void Window_DrawFramebuffer(Rect2D r) {
	// TODO probably bogus..
	// TODO: Don't redraw everything
	int size = fb_bmp.width * fb_bmp.height * 4;
	
	// TODO: double buffering ??
	//	https://dcemulation.org/phpBB/viewtopic.php?t=99999
	//	https://dcemulation.org/phpBB/viewtopic.php?t=43214
	vid_waitvbl();
	sq_cpy(vram_l, fb_bmp.scan0, size);
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