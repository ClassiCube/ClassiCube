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
#include "../VirtualKeyboard.h"
#include "../VirtualDialog.h"

#include <pspdisplay.h>
#include <pspge.h>
#include <pspctrl.h>
#include <pspkernel.h>

#define BUFFER_WIDTH  512
#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 272

struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;

void Window_PreInit(void) {
	DisplayInfo.Width  = SCREEN_WIDTH;
	DisplayInfo.Height = SCREEN_HEIGHT;
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;

	sceDisplaySetMode(0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

void Window_Init(void) {
	
	Window_Main.Width    = SCREEN_WIDTH;
	Window_Main.Height   = SCREEN_HEIGHT;
	Window_Main.Focused  = true;
	
	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;
	Window_Main.SoftKeyboard   = SOFT_KEYBOARD_VIRTUAL;
}

void Window_Free(void) { }

void Window_Create2D(int width, int height) { Window_Main.Is3D = false; }
void Window_Create3D(int width, int height) { Window_Main.Is3D = true;  }

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
static const BindMapping defaults_psp[BIND_COUNT] = {
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

void Gamepads_PreInit(void) {
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
}

void Gamepads_Init(void) {
	Input_DisplayNames[CCPAD_1] = "CIRCLE";
	Input_DisplayNames[CCPAD_2] = "CROSS";
	Input_DisplayNames[CCPAD_3] = "SQUARE";
	Input_DisplayNames[CCPAD_4] = "TRIANGLE";
}

static void HandleButtons(int port, int mods) {
	Gamepad_SetButton(port, CCPAD_L, mods & PSP_CTRL_LTRIGGER);
	Gamepad_SetButton(port, CCPAD_R, mods & PSP_CTRL_RTRIGGER);
	
	Gamepad_SetButton(port, CCPAD_1, mods & PSP_CTRL_CIRCLE);
	Gamepad_SetButton(port, CCPAD_2, mods & PSP_CTRL_CROSS);
	Gamepad_SetButton(port, CCPAD_3, mods & PSP_CTRL_SQUARE);
	Gamepad_SetButton(port, CCPAD_4, mods & PSP_CTRL_TRIANGLE);
	
	Gamepad_SetButton(port, CCPAD_START,  mods & PSP_CTRL_START);
	Gamepad_SetButton(port, CCPAD_SELECT, mods & PSP_CTRL_SELECT);
	
	Gamepad_SetButton(port, CCPAD_LEFT,   mods & PSP_CTRL_LEFT);
	Gamepad_SetButton(port, CCPAD_RIGHT,  mods & PSP_CTRL_RIGHT);
	Gamepad_SetButton(port, CCPAD_UP,     mods & PSP_CTRL_UP);
	Gamepad_SetButton(port, CCPAD_DOWN,   mods & PSP_CTRL_DOWN);
}

#define AXIS_SCALE 16.0f
static void ProcessCircleInput(int port, SceCtrlData* pad, float delta) {
	int x = pad->Lx - 127;
	int y = pad->Ly - 127;

	if (Math_AbsI(x) <= 8) x = 0;
	if (Math_AbsI(y) <= 8) y = 0;

	Gamepad_SetAxis(port, PAD_AXIS_RIGHT, x / AXIS_SCALE, y / AXIS_SCALE, delta);
}

void Gamepads_Process(float delta) {
	int port = Gamepad_Connect(0x503, defaults_psp);
	SceCtrlData pad;
	
	/* TODO implement */
	int ret = sceCtrlPeekBufferPositive(&pad, 1);
	if (ret <= 0) return;
	// TODO: need to use cached version still? like GameCube/Wii

	HandleButtons(port, pad.Buttons);
	ProcessCircleInput(port, &pad, delta);
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	// Add 8192 to fix in PPSSPP, launcher-> in-game -> launcher resulting in in-game frame continuing to be drawn
	void* fb = sceGeEdramGetAddr() + 8192;

	bmp->scan0  = fb;
	bmp->width  = BUFFER_WIDTH;
	bmp->height = height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	sceDisplayWaitVblankStart();
	sceKernelDcacheWritebackAll();
	sceDisplaySetFrameBuf(bmp->scan0, BUFFER_WIDTH, PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_SETBUF_NEXTFRAME);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = NULL;
}


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) {
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
	VirtualDialog_Show(title, msg, false);
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}
