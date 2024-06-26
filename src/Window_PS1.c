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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <psxapi.h>
#include <psxetc.h>
#include <psxgte.h>
#include <psxgpu.h>
#include <psxpad.h>

#define SCREEN_XRES	320
#define SCREEN_YRES	240

static cc_bool launcherMode;
static char pad_buff[2][34];

struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;

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

	Input.Sources = INPUT_SOURCE_GAMEPAD;
	DisplayInfo.ContentOffsetX = 10;
	DisplayInfo.ContentOffsetY = 10;
	
// http://lameguy64.net/tutorials/pstutorials/chapter1/4-controllers.html
	InitPAD(&pad_buff[0][0], 34, &pad_buff[1][0], 34);
	pad_buff[0][0] = pad_buff[0][1] = 0xff;
	pad_buff[1][0] = pad_buff[1][1] = 0xff;
	StartPAD();
	ChangeClearPAD(0);
}

void Window_Free(void) { }

void Window_Create2D(int width, int height) {
	launcherMode = true;
}
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
static void HandleButtons(int port, int buttons) {
	// Confusingly, it seems that when a bit is on, it means the button is NOT pressed
	// So just flip the bits to make more sense
	buttons = ~buttons;
	
	Gamepad_SetButton(port, CCPAD_A, buttons & PAD_TRIANGLE);
	Gamepad_SetButton(port, CCPAD_B, buttons & PAD_SQUARE);
	Gamepad_SetButton(port, CCPAD_X, buttons & PAD_CROSS);
	Gamepad_SetButton(port, CCPAD_Y, buttons & PAD_CIRCLE);
      
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

void Window_ProcessGamepads(float delta) {
	PADTYPE* pad = (PADTYPE*)&pad_buff[0][0];
	if (pad->stat == 0) ProcessPadInput(0, pad, delta);
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
static DISPENV disp;
static cc_uint16* fb;

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	SetDefDispEnv(&disp, 0, 0, SCREEN_XRES, SCREEN_YRES);
	disp.isinter = 1;

	PutDispEnv(&disp);
	SetDispMask(1);

	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, 4, "window pixels");
	bmp->width  = width;
	bmp->height = height;

	fb = Mem_Alloc(width * height, 2, "real surface");
}

#define BGRA8_to_PS1(src) \
	((src[2] & 0xF8) >> 3) | ((src[1] & 0xF8) << 2) | ((src[0] & 0xF8) << 7) | 0x8000

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	RECT rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = SCREEN_XRES;
	rect.h = SCREEN_YRES;

	for (int y = r.y; y < r.y + r.height; y++)
	{
		cc_uint32* src = Bitmap_GetRow(bmp, y);
		cc_uint16* dst = fb + y * bmp->width;
		
		for (int x = r.x; x < r.x + r.width; x++) {
			cc_uint8* color = (cc_uint8*)&src[x];
			dst[x] = BGRA8_to_PS1(color);
		}
	}

	LoadImage(&rect, fb);
	DrawSync(0);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
	Mem_Free(fb);
}


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { /* TODO implement */ }
void OnscreenKeyboard_SetText(const cc_string* text) { }
void OnscreenKeyboard_Draw2D(Rect2D* r, struct Bitmap* bmp) { }
void OnscreenKeyboard_Draw3D(void) { }
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
