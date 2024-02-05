#include "Core.h"
#if defined CC_BUILD_NDS
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
#include <nds/arm9/background.h>
#include <nds/arm9/input.h>

static int touchActive, touchBegX, touchBegY;
static cc_bool launcherMode;
static int bg_id;
static u16* bg_ptr;

struct _DisplayData DisplayInfo;
struct _WindowData WindowInfo;

void Window_Init(void) {
    
	DisplayInfo.Width  = 256;
	DisplayInfo.Height = 192;
	DisplayInfo.Depth  = 4; // 32 bit
	DisplayInfo.ScaleX = 0.5f;
	DisplayInfo.ScaleY = 0.5f;
	
	Window_Main.Width   = DisplayInfo.Width;
	Window_Main.Height  = DisplayInfo.Height;
	Window_Main.Focused = true;
	Window_Main.Exists  = true;

	Input.Sources = INPUT_SOURCE_GAMEPAD;
	
	consoleDemoInit();
	videoSetMode(MODE_5_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	
	bg_id  = bgInit(2, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
	bg_ptr = bgGetGfxPtr(bg_id);
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
	Event_RaiseVoid(&WindowEvents.Closing);
}


/*########################################################################################################################*
*----------------------------------------------------Input processing-----------------------------------------------------*
*#########################################################################################################################*/
static void HandleButtons(int mods) {
	Input_SetNonRepeatable(CCPAD_L, mods & KEY_L);
	Input_SetNonRepeatable(CCPAD_R, mods & KEY_R);
	
	Input_SetNonRepeatable(CCPAD_A, mods & KEY_A);
	Input_SetNonRepeatable(CCPAD_B, mods & KEY_B);
	Input_SetNonRepeatable(CCPAD_X, mods & KEY_X);
	Input_SetNonRepeatable(CCPAD_Y, mods & KEY_Y);
	
	Input_SetNonRepeatable(CCPAD_START,  mods & KEY_START);
	Input_SetNonRepeatable(CCPAD_SELECT, mods & KEY_SELECT);
	
	Input_SetNonRepeatable(CCPAD_LEFT,   mods & KEY_LEFT);
	Input_SetNonRepeatable(CCPAD_RIGHT,  mods & KEY_RIGHT);
	Input_SetNonRepeatable(CCPAD_UP,     mods & KEY_UP);
	Input_SetNonRepeatable(CCPAD_DOWN,   mods & KEY_DOWN);
}

static void ProcessTouchInput(int mods) {
	touchPosition pos;
	touchRead(&pos);
	touchActive = mods & KEY_TOUCH;
	
	if (touchActive) {
		Pointer_SetPosition(0, pos.px, pos.py);
	}
	// Set starting position for camera movement
	if (mods & KEY_TOUCH) {
		touchBegX = pos.px;
		touchBegY = pos.py;
	}
}

void Window_ProcessEvents(double delta) {
	scanKeys();
	
	int keys = keysDown();
	HandleButtons(keys);
	
	Input_SetNonRepeatable(CCMOUSE_L, keys & KEY_TOUCH);
	ProcessTouchInput(keys);
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for PSP
void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }
void Window_UpdateRawMouse(void)  { }


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	swiWaitForVBlank();
	 
	for (int y = r.y; y < r.y + r.Height; y++)
	{
		BitmapCol* src = Bitmap_GetRow(bmp, y);
		uint16_t*  dst = bg_ptr + 256 * y;
		
		for (int x = r.x; x < r.x + r.Width; x++)
		{
			BitmapCol color = src[x];
			// 888 to 565 (discard least significant bits)
			// quoting libDNS: < Bitmap background with 16 bit color values of the form aBBBBBGGGGGRRRRR (if 'a' is not set, the pixel will be transparent)
			dst[x] = 0x8000 | ((BitmapCol_B(color) & 0xF8) << 7) | ((BitmapCol_G(color) & 0xF8) << 2) | (BitmapCol_R(color) >> 3);
		}
	}
	
	bgShow(bg_id);
    bgUpdate();
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
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
