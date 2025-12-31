#include "../Window.h"
#include "../Platform.h"
#include "../Input.h"
#include "../Event.h"
#include "../Graphics.h"
#include "../String_.h"
#include "../Funcs.h"
#include "../Bitmap.h"
#include "../Errors.h"
#include "../ExtMath.h"
#include "../Gui.h"

#include <3ds.h>

static u16 top_width, top_height;
static u16 btm_width, btm_height;
#include "../VirtualKeyboard.h"

struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;
struct cc_window Window_Alt;
cc_bool launcherTop;

void Window_PreInit(void) {
	gfxInit(GSP_BGR8_OES, GSP_BGR8_OES, false);
}

// Note from https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/gfx.h
//  * Please note that the 3DS uses *portrait* screens rotated 90 degrees counterclockwise.
//  * Width/height refer to the physical dimensions of the screen; that is, the top screen
//  * is 240 pixels wide and 400 pixels tall; while the bottom screen is 240x320.
void Window_Init(void) {
	// deliberately swapped
	gfxGetFramebuffer(GFX_TOP,    GFX_LEFT, &top_height, &top_width);
	gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &btm_height, &btm_width);
	
	DisplayInfo.Width  = top_width; 
	DisplayInfo.Height = top_height;
	DisplayInfo.ScaleX = 0.5f;
	DisplayInfo.ScaleY = 0.5f;
	
	Window_Main.Width    = top_width;
	Window_Main.Height   = top_height;
	Window_Main.Focused  = true;
	
	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;

	Window_Main.SoftKeyboard = SOFT_KEYBOARD_RESIZE;
	Input_SetTouchMode(true);
	Gui_SetTouchUI(true);
	
	Window_Alt.Width  = btm_width;
	Window_Alt.Height = btm_height;
}

void Window_Free(void) { irrstExit(); }

void Window_Create2D(int width, int height) {
	Window_Main.Is3D  = false;
	Window_Alt.Is3D   = false;

	DisplayInfo.Width = btm_width;
	Window_Main.Width = btm_width;
	Window_Alt.Width  = top_width;
}

void Window_Create3D(int width, int height) { 
	Window_Main.Is3D  = true;
	Window_Alt.Is3D   = true;

	DisplayInfo.Width = top_width;
	Window_Main.Width = top_width;
	Window_Alt.Width  = btm_width;
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
static void ProcessTouchInput(int mods) {
	touchPosition touch;
	hidTouchRead(&touch);

	if (mods & KEY_TOUCH) {
		Input_AddTouch(0,    touch.px,      touch.py);
	} else if (hidKeysUp() & KEY_TOUCH) {
		Input_RemoveTouch(0, Pointers[0].x, Pointers[0].y);
	}
}

void Window_ProcessEvents(float delta) {
	hidScanInput();

	if (!aptMainLoop()) {
		Window_Main.Exists = false;
		Window_RequestClose();
		return;
	}
	
	u32 mods = hidKeysDown() | hidKeysHeld();
	ProcessTouchInput(mods);
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for 3DS

void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }

void Window_UpdateRawMouse(void)  { }


/*########################################################################################################################*
*-------------------------------------------------------Gamepads----------------------------------------------------------*
*#########################################################################################################################*/
static const BindMapping defaults_3ds[BIND_COUNT] = {
	[BIND_FORWARD]      = { CCPAD_UP    },  
	[BIND_BACK]         = { CCPAD_DOWN  },
	[BIND_LEFT]         = { CCPAD_LEFT  },  
	[BIND_RIGHT]        = { CCPAD_RIGHT },
	[BIND_JUMP]         = { CCPAD_1     },
	[BIND_SET_SPAWN]    = { CCPAD_START }, 
	[BIND_CHAT]         = { CCPAD_4     },
	[BIND_INVENTORY]    = { CCPAD_3     },
	[BIND_SEND_CHAT]    = { CCPAD_START },
	[BIND_PLACE_BLOCK]  = { CCPAD_L     },
	[BIND_DELETE_BLOCK] = { CCPAD_R     },
	[BIND_SPEED]        = { CCPAD_2, CCPAD_L },
	[BIND_FLY]          = { CCPAD_2, CCPAD_R },
	[BIND_NOCLIP]       = { CCPAD_2, CCPAD_3 },
	[BIND_FLY_UP]       = { CCPAD_2, CCPAD_UP },
	[BIND_FLY_DOWN]     = { CCPAD_2, CCPAD_DOWN },
	[BIND_HOTBAR_LEFT]  = { CCPAD_ZL    }, 
	[BIND_HOTBAR_RIGHT] = { CCPAD_ZR    }
};

static Result irrst_result;
void Gamepads_PreInit(void) {
	irrst_result = irrstInit();
}

void Gamepads_Init(void) { }

static void HandleButtons(int port, u32 mods) {
	Gamepad_SetButton(port, CCPAD_L, mods & KEY_L);
	Gamepad_SetButton(port, CCPAD_R, mods & KEY_R);
	
	Gamepad_SetButton(port, CCPAD_1, mods & KEY_A);
	Gamepad_SetButton(port, CCPAD_2, mods & KEY_B);
	Gamepad_SetButton(port, CCPAD_3, mods & KEY_X);
	Gamepad_SetButton(port, CCPAD_4, mods & KEY_Y);
	
	Gamepad_SetButton(port, CCPAD_START,  mods & KEY_START);
	Gamepad_SetButton(port, CCPAD_SELECT, mods & KEY_SELECT);
	
	Gamepad_SetButton(port, CCPAD_LEFT,   mods & KEY_DLEFT);
	Gamepad_SetButton(port, CCPAD_RIGHT,  mods & KEY_DRIGHT);
	Gamepad_SetButton(port, CCPAD_UP,     mods & KEY_DUP);
	Gamepad_SetButton(port, CCPAD_DOWN,   mods & KEY_DDOWN);
	
	Gamepad_SetButton(port, CCPAD_ZL, mods & KEY_ZL);
	Gamepad_SetButton(port, CCPAD_ZR, mods & KEY_ZR);
}

#define AXIS_SCALE 8.0f
static void ProcessCircleInput(int port, int axis, circlePosition* pos, float delta) {
	// May not be exactly 0 on actual hardware
	if (Math_AbsI(pos->dx) <= 24) pos->dx = 0;
	if (Math_AbsI(pos->dy) <= 24) pos->dy = 0;
		
	Gamepad_SetAxis(port, axis, pos->dx / AXIS_SCALE, -pos->dy / AXIS_SCALE, delta);
}

void Gamepads_Process(float delta) {
	u32 mods = hidKeysDown() | hidKeysHeld();
	int port = Gamepad_Connect(0x3D5, defaults_3ds);
	HandleButtons(port, mods);
	
	circlePosition hid_pos;
	hidCircleRead(&hid_pos);

	if (irrst_result == 0) {
		circlePosition stk_pos;
		irrstScanInput();
		irrstCstickRead(&stk_pos);
		
		ProcessCircleInput(port, PAD_AXIS_RIGHT, &stk_pos, delta);
		ProcessCircleInput(port, PAD_AXIS_LEFT,  &hid_pos, delta);
	} else {
		ProcessCircleInput(port, PAD_AXIS_RIGHT, &hid_pos, delta);
	}
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	bmp->width  = width;
	bmp->height = height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	u16 width, height;
	gfxScreen_t screen = launcherTop ? GFX_TOP : GFX_BOTTOM;
	
	gfxSetDoubleBuffering(screen, false);
	u8* fb = gfxGetFramebuffer(screen, GFX_LEFT, &width, &height);
	// SRC y = 0 to 240
	// SRC x = 0 to 400
	// DST X = 0 to 240
	// DST Y = 0 to 400

	for (int y = r.y; y < r.y + r.height; y++)
		for (int x = r.x; x < r.x + r.width; x++)
	{
		BitmapCol color = Bitmap_GetPixel(bmp, x, y);
		int addr   = (width - 1 - y + x * width) * 3; // TODO -1 or not
		fb[addr+0] = BitmapCol_B(color);
		fb[addr+1] = BitmapCol_G(color);
		fb[addr+2] = BitmapCol_R(color);
	}
	// TODO gspWaitForVBlank();
	gfxFlushBuffers();
	//gfxSwapBuffers();
	// TODO: tearing??
	/*
	gfxSetDoubleBuffering(GFX_TOP, false);
	gfxScreenSwapBuffers(GFX_TOP, true);
	gfxSetDoubleBuffering(GFX_TOP, true);
	gfxScreenSwapBuffers(GFX_BOTTOM, true);
	*/
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) {
	kb_tileWidth  = 20;
	kb_tileHeight = 20;

	VirtualKeyboard_Open(args);
}

void OnscreenKeyboard_SetText(const cc_string* text) {
	VirtualKeyboard_SetText(text);
}

void OnscreenKeyboard_Close(void) {
	VirtualKeyboard_Close();
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

