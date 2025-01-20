#include "Core.h"
#if defined CC_BUILD_PS1
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
#include "VirtualKeyboard.h"
#include <psxapi.h>
#include <psxetc.h>
#include <psxgpu.h>
#include <psxpad.h>

#define SCREEN_XRES	320
#define SCREEN_YRES	240

static cc_bool launcherMode;
struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;
static DISPENV disp;

void Window_PreInit(void) { }
void Window_Init(void) {
	DisplayInfo.Width  = SCREEN_XRES;
	DisplayInfo.Height = SCREEN_YRES;
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
	Window_Main.SoftKeyboard   = SOFT_KEYBOARD_VIRTUAL;
}

void Window_Free(void) { }

void Window_Create2D(int width, int height) {
	ResetGraph(0);
	launcherMode = true;

	SetDefDispEnv(&disp, 0, 0, SCREEN_XRES, SCREEN_YRES);
	PutDispEnv(&disp);
	SetDispMask(1);
}

void Window_Create3D(int width, int height) { 
	ResetGraph(0);
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
static char pad_buff[2][34];

void Gamepads_Init(void) {
	Input.Sources |= INPUT_SOURCE_GAMEPAD;
	
	// http://lameguy64.net/tutorials/pstutorials/chapter1/4-controllers.html
	InitPAD(&pad_buff[0][0], 34, &pad_buff[1][0], 34);
	pad_buff[0][0] = pad_buff[0][1] = 0xff;
	pad_buff[1][0] = pad_buff[1][1] = 0xff;
	StartPAD();
	ChangeClearPAD(0);
	
	Input_DisplayNames[CCPAD_1] = "CIRCLE";
	Input_DisplayNames[CCPAD_2] = "CROSS";
	Input_DisplayNames[CCPAD_3] = "SQUARE";
	Input_DisplayNames[CCPAD_4] = "TRIANGLE";
}

static void HandleButtons(int port, int buttons) {
	// Confusingly, it seems that when a bit is on, it means the button is NOT pressed
	// So just flip the bits to make more sense
	buttons = ~buttons;
	
	Gamepad_SetButton(port, CCPAD_1, buttons & PAD_CIRCLE);
	Gamepad_SetButton(port, CCPAD_2, buttons & PAD_CROSS);
	Gamepad_SetButton(port, CCPAD_3, buttons & PAD_SQUARE);
	Gamepad_SetButton(port, CCPAD_4, buttons & PAD_TRIANGLE);
      
	Gamepad_SetButton(port, CCPAD_START,  buttons & PAD_START);
	Gamepad_SetButton(port, CCPAD_SELECT, buttons & PAD_SELECT);

	Gamepad_SetButton(port, CCPAD_LEFT,   buttons & PAD_LEFT);
	Gamepad_SetButton(port, CCPAD_RIGHT,  buttons & PAD_RIGHT);
	Gamepad_SetButton(port, CCPAD_UP,     buttons & PAD_UP);
	Gamepad_SetButton(port, CCPAD_DOWN,   buttons & PAD_DOWN);
	
	Gamepad_SetButton(port, CCPAD_L,  buttons & PAD_L1);
	Gamepad_SetButton(port, CCPAD_R,  buttons & PAD_R1);
	Gamepad_SetButton(port, CCPAD_ZL, buttons & PAD_L2);
	Gamepad_SetButton(port, CCPAD_ZR, buttons & PAD_R2);
}

#define AXIS_SCALE 16.0f
static void HandleJoystick(int port, int axis, int x, int y, float delta) {
	if (Math_AbsI(x) <= 8) x = 0;
	if (Math_AbsI(y) <= 8) y = 0;
	
	Gamepad_SetAxis(port, axis, x / AXIS_SCALE, y / AXIS_SCALE, delta);
}

static void ProcessPadInput(int port, PADTYPE* pad, float delta) {
	HandleButtons(port, pad->btn);

	if (pad->type == PAD_ID_ANALOG_STICK || pad->type == PAD_ID_ANALOG) {
		HandleJoystick(port, PAD_AXIS_LEFT,  pad->ls_x - 0x80, pad->ls_y - 0x80, delta);
		HandleJoystick(port, PAD_AXIS_RIGHT, pad->rs_x - 0x80, pad->rs_y - 0x80, delta);
	}
}

void Gamepads_Process(float delta) {
	PADTYPE* pad = (PADTYPE*)&pad_buff[0][0];
	int port = Gamepad_Connect(0x503E, PadBind_Defaults);
	
	if (pad->stat == 0) ProcessPadInput(port, pad, delta);
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
extern void Gfx_TransferToVRAM(int x, int y, int w, int h, void* pixels);

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	bmp->width  = width;
	bmp->height = height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	Gfx_TransferToVRAM(0, 0, SCREEN_XRES, SCREEN_YRES, bmp->scan0);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) {
	kb_tileWidth  = KB_TILE_SIZE / 2;
	kb_tileHeight = KB_TILE_SIZE / 2;
	VirtualKeyboard_Open(args, launcherMode);
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
#endif
