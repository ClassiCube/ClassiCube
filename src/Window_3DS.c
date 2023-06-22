#include "Core.h"
#if defined CC_BUILD_3DS
#include "Graphics.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Errors.h"
#include <3ds.h>
#include "_WindowBase.h"

// Note from https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/gfx.h
//  * Please note that the 3DS uses *portrait* screens rotated 90 degrees counterclockwise.
//  * Width/height refer to the physical dimensions of the screen; that is, the top screen
//  * is 240 pixels wide and 400 pixels tall; while the bottom screen is 240x320.
void Window_Init(void) {
	//gfxInit(GSP_BGR8_OES,GSP_BGR8_OES,false); 
	//gfxInit(GSP_BGR8_OES,GSP_RGBA8_OES,false);
	//gfxInit(GSP_RGBA8_OES, GSP_BGR8_OES, false); 
	gfxInit(GSP_BGR8_OES,GSP_BGR8_OES,false);
	
	//gfxInit(GSP_RGBA8_OES,GSP_RGBA8_OES,false);
	consoleInit(GFX_BOTTOM, NULL);
	
	u16 width, height;
	gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &width, &height);
	
	DisplayInfo.Width  = height; // deliberately swapped
	DisplayInfo.Height = width;  // deliberately swapped
	DisplayInfo.Depth  = 4; // 32 bit
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	WindowInfo.Width   = height; // deliberately swapped
	WindowInfo.Height  = width;  // deliberately swapped
	WindowInfo.Focused = true;
}

static void DoCreateWindow(int _3d) {
	WindowInfo.Exists = true;
}
void Window_Create2D(int width, int height) { DoCreateWindow(0); }
void Window_Create3D(int width, int height) { DoCreateWindow(1); }

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

void Window_ProcessEvents(void) {
	hidScanInput();
	/* TODO implement */
	
	//u32 m1 = hidKeysDown(), m2 = hidKeysHeld();
	//Platform_Log2("MODS: %h | %h", &m1, &m2);
	
	// hidKeysDown hidKeysUp
	//u32 mods = hidKeysDownRepeat();
	u32 mods = hidKeysDown() | hidKeysHeld();
	//Platform_Log1("MODS: %h", &mods);
	Input_SetNonRepeatable(IPT_LMOUSE, mods & KEY_TOUCH);
	
	
	Input_SetNonRepeatable(KeyBinds[KEYBIND_PLACE_BLOCK],  mods & KEY_L);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_DELETE_BLOCK], mods & KEY_R);
	
	Input_SetNonRepeatable(KeyBinds[KEYBIND_JUMP],      mods & KEY_A);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_CHAT],      mods & KEY_X);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_INVENTORY], mods & KEY_Y);
	
	Input_SetNonRepeatable(IPT_ENTER,  mods & KEY_START);
	Input_SetNonRepeatable(IPT_ESCAPE, mods & KEY_SELECT);
	// fake tab with down for Launcher
	Input_SetNonRepeatable(IPT_TAB, mods & KEY_DDOWN);
	
	Input_SetNonRepeatable(KeyBinds[KEYBIND_LEFT],  mods & KEY_DLEFT);
	Input_SetNonRepeatable(IPT_LEFT,                mods & KEY_DLEFT);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_RIGHT], mods & KEY_DRIGHT);
	Input_SetNonRepeatable(IPT_RIGHT,               mods & KEY_DRIGHT);
	
	Input_SetNonRepeatable(KeyBinds[KEYBIND_FORWARD], mods & KEY_DUP);
	Input_SetNonRepeatable(IPT_UP,                    mods & KEY_DUP);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_BACK],    mods & KEY_DDOWN);
	Input_SetNonRepeatable(IPT_DOWN,                  mods & KEY_DDOWN);
	
	if (hidKeysHeld() & KEY_TOUCH) {
		int x, y;
		Cursor_GetRawPos(&x, &y);
		Pointer_SetPosition(0, x, y);
	}
}

static void Cursor_GetRawPos(int* x, int* y) {
	touchPosition touch;
	hidTouchRead(&touch);

	*x = touch.px;
	*y = touch.py;
}

static void Cursor_DoSetVisible(cc_bool visible) {
	/* TODO implement */
}

void Cursor_SetPosition(int x, int y) {
	/* TODO implement */
}

static void ShowDialogCore(const char* title, const char* msg) {
	/* TODO implement */
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

static struct Bitmap fb_bmp;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	fb_bmp = *bmp;
}

void Window_DrawFramebuffer(Rect2D r) {
	u16 width, height;
	gfxSetDoubleBuffering(GFX_TOP, false);
	u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &width, &height);

	// SRC y = 0 to 240
	// SRC x = 0 to 400
	// DST X = 0 to 240
	// DST Y = 0 to 400

	for (int y = r.Y; y < r.Y + r.Height; y++)
		for (int x = r.X; x < r.X + r.Width; x++)
	{
		BitmapCol color = Bitmap_GetPixel(&fb_bmp, x, y);
		int addr   = (width - 1 - y + x * width) * 3; // TODO -1 or not
		fb[addr+0] = BitmapCol_B(color);
		fb[addr+1] = BitmapCol_G(color);
		fb[addr+2] = BitmapCol_R(color);
	}
	// TODO implement
	// TODO gspWaitForVBlank();
	Platform_LogConst("DRAW FB!!!");
	gfxFlushBuffers();
	//gfxSwapBuffers();
	// TODO: tearing??
	gfxSetDoubleBuffering(GFX_TOP, false);
	gfxScreenSwapBuffers(GFX_TOP, true);
	gfxSetDoubleBuffering(GFX_TOP, true);
	gfxScreenSwapBuffers(GFX_BOTTOM, true);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	/* TODO implement */
}

static void OnscreenTextChanged(const char* text) {
	char tmpBuffer[NATIVE_STR_LEN];
	cc_string tmp = String_FromArray(tmpBuffer);
	String_AppendUtf8(&tmp, text, String_Length(text));
    
	Event_RaiseString(&InputEvents.TextChanged, &tmp);
}

void Window_OpenKeyboard(struct OpenKeyboardArgs* args) {
	const char* btnText = args->type & KEYBOARD_FLAG_SEND ? "Send" : "Enter";
	char input[NATIVE_STR_LEN]  = { 0 };
	char output[NATIVE_STR_LEN] = { 0 };
	SwkbdState swkbd;
	String_EncodeUtf8(input, args->text);
	
	int mode = args->type & 0xFF;
	int type = (mode == KEYBOARD_TYPE_NUMBER || mode == KEYBOARD_TYPE_INTEGER) ? SWKBD_TYPE_NUMPAD : SWKBD_TYPE_WESTERN;
	
	swkbdInit(&swkbd, type, 3, -1);
	swkbdSetInitialText(&swkbd, input);
	swkbdSetHintText(&swkbd, args->placeholder);
	//swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, "Cancel", false);
	//swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, btnText, true);
	swkbdSetButton(&swkbd, SWKBD_BUTTON_CONFIRM, btnText, true);
	
	if (type == KEYBOARD_TYPE_PASSWORD)
		swkbdSetPasswordMode(&swkbd, SWKBD_PASSWORD_HIDE_DELAY);
	if (args->multiline)
		swkbdSetFeatures(&swkbd, SWKBD_MULTILINE);
		
	// TODO filter callbacks and Window_Setkeyboardtext ??
	int btn = swkbdInputText(&swkbd, output, sizeof(output));
	if (btn != SWKBD_BUTTON_CONFIRM) return;
	OnscreenTextChanged(output);
}
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { /* TODO implement */ }

void Window_EnableRawMouse(void)  { DefaultEnableRawMouse();  }
void Window_UpdateRawMouse(void)  { DefaultUpdateRawMouse();  }
void Window_DisableRawMouse(void) { DefaultDisableRawMouse(); }
#endif