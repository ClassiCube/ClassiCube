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
#include "Camera.h"
#include <nds/arm9/background.h>
#include <nds/arm9/input.h>
#include <nds/arm9/console.h>
#include <nds/arm9/keyboard.h>
#include <nds/interrupts.h>

static cc_bool launcherMode;
cc_bool keyboardOpen;
static int bg_id;
static u16* bg_ptr;

struct _DisplayData DisplayInfo;
struct _WindowData WindowInfo;

// Console and Keyboard combined need more than 32 kb of H VRAM bank
// The simple solution is to allocate the C VRAM bank, but ClassiCube
// needs as much VRAM as it can get for textures
// So the solution is to share the H VRAM bank between console and keyboard
static void ResetHBank(void) {
    // Map all VRAM banks to LCDC mode so that the CPU can access it
    vramSetBankH(VRAM_H_LCD);
    dmaFillWords(0, VRAM_H, 32 * 1024);
    vramSetBankH(VRAM_H_SUB_BG);
}

static void InitConsoleWindow(void) {
    consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 14, 0, false, true);
}

void Window_Init(void) {  
	DisplayInfo.Width  = SCREEN_WIDTH;
	DisplayInfo.Height = SCREEN_HEIGHT;
	DisplayInfo.Depth  = 4; // 32 bit
	DisplayInfo.ScaleX = 0.5f;
	DisplayInfo.ScaleY = 0.5f;
	
	Window_Main.Width   = DisplayInfo.Width;
	Window_Main.Height  = DisplayInfo.Height;
	Window_Main.Focused = true;
	Window_Main.Exists  = true;

	Input_SetTouchMode(true);
	Input.Sources = INPUT_SOURCE_GAMEPAD;

    videoSetModeSub(MODE_0_2D);
    vramSetBankH(VRAM_H_SUB_BG);
    setBrightness(2, 0);
	InitConsoleWindow();
}

void Window_Free(void) { }

void Window_Create2D(int width, int height) { 
    launcherMode = true;
	videoSetMode(MODE_5_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	
	bg_id  = bgInit(2, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
	bg_ptr = bgGetGfxPtr(bg_id);
}

void Window_Create3D(int width, int height) { 
    launcherMode = false;
	videoSetMode(MODE_0_3D);
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
static void HandleButtons(int mods) {
	Gamepad_SetButton(CCPAD_L, mods & KEY_L);
	Gamepad_SetButton(CCPAD_R, mods & KEY_R);
	
	Gamepad_SetButton(CCPAD_A, mods & KEY_A);
	Gamepad_SetButton(CCPAD_B, mods & KEY_B);
	Gamepad_SetButton(CCPAD_X, mods & KEY_X);
	Gamepad_SetButton(CCPAD_Y, mods & KEY_Y);
	
	Gamepad_SetButton(CCPAD_START,  mods & KEY_START);
	Gamepad_SetButton(CCPAD_SELECT, mods & KEY_SELECT);
	
	Gamepad_SetButton(CCPAD_LEFT,   mods & KEY_LEFT);
	Gamepad_SetButton(CCPAD_RIGHT,  mods & KEY_RIGHT);
	Gamepad_SetButton(CCPAD_UP,     mods & KEY_UP);
	Gamepad_SetButton(CCPAD_DOWN,   mods & KEY_DOWN);
}

// Copied from Window_3DS.c
static void ProcessTouchInput(int mods) {
	static int curX, curY;  // current touch position
	touchPosition touch;
	touchRead(&touch);
    Camera.Sensitivity = 100; // TODO not hardcode this
	
	if (keysDown() & KEY_TOUCH) {  // stylus went down
		curX = touch.px;
		curY = touch.py;
		Input_AddTouch(0, curX, curY);
	} else if (mods & KEY_TOUCH) {  // stylus is down
		curX = touch.px;
		curY = touch.py;
		Input_UpdateTouch(0, curX, curY);
	} else if (keysUp() & KEY_TOUCH) {  // stylus was lifted
		Input_RemoveTouch(0, curX, curY);
	}
}

void Window_ProcessEvents(double delta) {
	scanKeys();	
	int keys = keysDown() | keysHeld();
	HandleButtons(keys);
	
    if (keyboardOpen) {
        keyboardUpdate();
    } else {
	    ProcessTouchInput(keys);
    }
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
static char kbBuffer[NATIVE_STR_LEN + 1];
static cc_string kbText;

static void OnKeyPressed(int key) {
    if (key == 0) {
        Window_CloseKeyboard();
    } else if (key == DVK_ENTER) {
        Window_CloseKeyboard();
        Input_SetPressed(CCKEY_ENTER);
        Input_SetReleased(CCKEY_ENTER);
    } else if (key == DVK_BACKSPACE) {
        if (kbText.length) kbText.length--;
        Event_RaiseString(&InputEvents.TextChanged, &kbText);
    } else if (key > 0) {
        String_Append(&kbText, key);
        Event_RaiseString(&InputEvents.TextChanged, &kbText);
    }
}

void Window_OpenKeyboard(struct OpenKeyboardArgs* args) { 
    Keyboard* kbd = keyboardGetDefault();
    videoBgDisableSub(0); // hide console

    ResetHBank(); // reset shared VRAM
    keyboardInit(kbd, 3, BgType_Text4bpp, BgSize_T_256x512,
                       14, 0, false, true);
    keyboardShow();

    kbd->OnKeyPressed = OnKeyPressed;
    String_InitArray(kbText, kbBuffer);
	String_AppendString(&kbText, args->text);
    keyboardOpen = true;
}

void Window_SetKeyboardText(const cc_string* text) { }

void Window_CloseKeyboard(void) {
    keyboardHide();
    keyboardOpen = false;
    ResetHBank(); // reset shared VRAM

    videoBgEnableSub(0); // show console
    InitConsoleWindow();
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
