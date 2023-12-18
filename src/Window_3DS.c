#include "Core.h"
#if defined CC_BUILD_3DS
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
#include <3ds.h>

static int touchActive, touchBegX, touchBegY;
static cc_bool launcherMode;
static Result irrst_result;

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
	//gfxInit(GSP_BGR8_OES,GSP_BGR8_OES,false); 
	//gfxInit(GSP_BGR8_OES,GSP_RGBA8_OES,false);
	//gfxInit(GSP_RGBA8_OES, GSP_BGR8_OES, false); 
	gfxInit(GSP_BGR8_OES, GSP_BGR8_OES, false);
	
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
	WindowInfo.Exists  = true;

	Input.Sources = INPUT_SOURCE_GAMEPAD;
	irrst_result  = irrstInit();
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
	Event_RaiseVoid(&WindowEvents.Closing);
}

/*########################################################################################################################*
*----------------------------------------------------Input processing-----------------------------------------------------*
*#########################################################################################################################*/
static void HandleButtons(u32 mods) {
	Input_SetNonRepeatable(CCPAD_L, mods & KEY_L);
	Input_SetNonRepeatable(CCPAD_R, mods & KEY_R);
	
	Input_SetNonRepeatable(CCPAD_A, mods & KEY_A);
	Input_SetNonRepeatable(CCPAD_B, mods & KEY_B);
	Input_SetNonRepeatable(CCPAD_X, mods & KEY_X);
	Input_SetNonRepeatable(CCPAD_Y, mods & KEY_Y);
	
	Input_SetNonRepeatable(CCPAD_START,  mods & KEY_START);
	Input_SetNonRepeatable(CCPAD_SELECT, mods & KEY_SELECT);
	
	Input_SetNonRepeatable(CCPAD_LEFT,   mods & KEY_DLEFT);
	Input_SetNonRepeatable(CCPAD_RIGHT,  mods & KEY_DRIGHT);
	Input_SetNonRepeatable(CCPAD_UP,     mods & KEY_DUP);
	Input_SetNonRepeatable(CCPAD_DOWN,   mods & KEY_DDOWN);
	
	Input_SetNonRepeatable(CCPAD_ZL, mods & KEY_ZL);
	Input_SetNonRepeatable(CCPAD_ZR, mods & KEY_ZR);
}

static void ProcessJoystickInput(circlePosition* pos, double delta) {
	float scale = (delta * 60.0) / 8.0f;
	
	// May not be exactly 0 on actual hardware
	if (Math_AbsI(pos->dx) <= 8) pos->dx = 0;
	if (Math_AbsI(pos->dy) <= 8) pos->dy = 0;
		
	Event_RaiseRawMove(&PointerEvents.RawMoved, pos->dx * scale, -pos->dy * scale);
}

static void ProcessTouchInput(int mods) {
	touchPosition touch;
	hidTouchRead(&touch);
	touchActive = mods & KEY_TOUCH;
	
	if (touchActive) {
		// rescale X from [0, bottom_FB_width) to [0, top_FB_width)
		int x = touch.px * WindowInfo.Width / GSP_SCREEN_HEIGHT_BOTTOM;
	 	int y = touch.py;
		Pointer_SetPosition(0, x, y);
	}
	// Set starting position for camera movement
	if (hidKeysDown() & KEY_TOUCH) {
		touchBegX = touch.px;
		touchBegY = touch.py;
	}
}

void Window_ProcessEvents(double delta) {
	hidScanInput();
	/* TODO implement */
	
	if (!aptMainLoop()) {
		Event_RaiseVoid(&WindowEvents.Closing);
		WindowInfo.Exists = false;
		return;
	}
	
	u32 mods = hidKeysDown() | hidKeysHeld();
	HandleButtons(mods);
	
	Input_SetNonRepeatable(CCMOUSE_L, mods & KEY_TOUCH);
	ProcessTouchInput(mods);
	
	if (Input.RawMode) {
		circlePosition pos;
		hidCircleRead(&pos);
		ProcessJoystickInput(&pos, delta);
	}
	
	if (Input.RawMode && irrst_result == 0) {
		circlePosition pos;
		irrstScanInput();
		irrstCstickRead(&pos);
		ProcessJoystickInput(&pos, delta);
	}
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for 3DS

void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }

void Window_UpdateRawMouse(void)  {
	if (!touchActive) return;
	
	touchPosition touch;
	hidTouchRead(&touch);

	Event_RaiseRawMove(&PointerEvents.RawMoved, 
				touch.px - touchBegX, touch.py - touchBegY);	
	touchBegX = touch.px;
	touchBegY = touch.py;
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
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

	for (int y = r.y; y < r.y + r.Height; y++)
		for (int x = r.x; x < r.x + r.Width; x++)
	{
		BitmapCol color = Bitmap_GetPixel(&fb_bmp, x, y);
		int addr   = (width - 1 - y + x * width) * 3; // TODO -1 or not
		fb[addr+0] = BitmapCol_B(color);
		fb[addr+1] = BitmapCol_G(color);
		fb[addr+2] = BitmapCol_R(color);
	}
	// TODO implement
	// TODO gspWaitForVBlank();
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


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
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