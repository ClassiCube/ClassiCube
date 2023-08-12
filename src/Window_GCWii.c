#include "Core.h"
#if defined CC_BUILD_GCWII
#include "Window.h"
#include "Platform.h"
#include "Input.h"
#include "Event.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Errors.h"
#include "ExtMath.h"
#include "Graphics.h"
#include <gccore.h>
#if defined HW_RVL
#include <wiiuse/wpad.h>
#include <wiikeyboard/keyboard.h>
#endif

static cc_bool launcherMode;
static void* xfb;
static GXRModeObj* rmode;
void* Window_XFB;
struct _DisplayData DisplayInfo;
struct _WinData WindowInfo;
// no DPI scaling on Wii/GameCube
int Display_ScaleX(int x) { return x; }
int Display_ScaleY(int y) { return y; }


static void OnPowerOff(void) {
	Event_RaiseVoid(&WindowEvents.Closing);
	WindowInfo.Exists = false;
}

void Window_Init(void) {	
	// TODO: SYS_SetResetCallback(reload); too? not sure how reset differs on GC/WII
	#if defined HW_RVL
	SYS_SetPowerCallback(OnPowerOff);
	#endif
	
	// Initialise the video system
	VIDEO_Init();

	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);
	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	
	Window_XFB = xfb;	
	//console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);
	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);

	// Make the display visible
	VIDEO_SetBlack(FALSE);
	// Flush the video register changes to the hardware
	VIDEO_Flush();
	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	
	DisplayInfo.Width  = rmode->fbWidth;
	DisplayInfo.Height = rmode->xfbHeight;
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	WindowInfo.Width   = rmode->fbWidth;
	WindowInfo.Height  = rmode->xfbHeight;
	WindowInfo.Focused = true;
	WindowInfo.Exists  = true;

	Input.GamepadSource = true;
	#if defined HW_RVL
	WPAD_Init();
	WPAD_SetDataFormat(0, WPAD_FMT_BTNS_ACC_IR);
	KEYBOARD_Init(NULL);
	#endif
	PAD_Init();
}

void Window_Create2D(int width, int height) { launcherMode = true;  }
void Window_Create3D(int width, int height) { launcherMode = false; }

void Window_Close(void) {
	/* TODO implement */
}


/*########################################################################################################################*
*---------------------------------------------GameCube controller processing----------------------------------------------*
*#########################################################################################################################*/
static void ProcessPAD_Launcher(PADStatus* pad) {
	int mods = pad->button;	
	
	Input_SetNonRepeatable(CCKEY_ENTER,  mods & PAD_BUTTON_A);
	Input_SetNonRepeatable(CCKEY_ESCAPE, mods & PAD_BUTTON_B);
	// fake tab with down for Launcher
	//Input_SetNonRepeatable(CCKEY_TAB, mods & PAD_BUTTON_DOWN);
	
	Input_SetNonRepeatable(CCPAD_LEFT,  mods & PAD_BUTTON_LEFT);
	Input_SetNonRepeatable(CCPAD_RIGHT, mods & PAD_BUTTON_RIGHT);
	Input_SetNonRepeatable(CCPAD_UP,    mods & PAD_BUTTON_UP);
	Input_SetNonRepeatable(CCPAD_DOWN,  mods & PAD_BUTTON_DOWN);
}
static void ProcessPAD_LeftJoystick(PADStatus* pad) {
	int dx = pad->stickX;
	int dy = pad->stickY;

	// May not be exactly 0 on actual hardware
	if (Math_AbsI(dx) <= 8) dx = 0;
	if (Math_AbsI(dy) <= 8) dy = 0;
	
	if (dx == 0 && dy == 0) return;	
	Input.JoystickMovement = true;
	Input.JoystickAngle    = Math_Atan2(dx, -dy);
}
static void ProcessPAD_RightJoystick(PADStatus* pad) {
	int dx = pad->substickX;
	int dy = pad->substickY;

	// May not be exactly 0 on actual hardware
	if (Math_AbsI(dx) <= 8) dx = 0;
	if (Math_AbsI(dy) <= 8) dy = 0;
	
	Event_RaiseRawMove(&PointerEvents.RawMoved, dx / 8.0f, -dy / 8.0f);		
}

static void ProcessPAD_Game(PADStatus* pad) {
	int mods = pad->button;

	if (Input.RawMode) {
		ProcessPAD_LeftJoystick(pad);
		ProcessPAD_RightJoystick(pad);
	}		
	
	Input_SetNonRepeatable(CCPAD_L, mods & PAD_TRIGGER_L);
	Input_SetNonRepeatable(CCPAD_R, mods & PAD_TRIGGER_R);
	
	Input_SetNonRepeatable(CCPAD_A, mods & PAD_BUTTON_A);
	Input_SetNonRepeatable(CCPAD_B, mods & PAD_BUTTON_B);
	Input_SetNonRepeatable(CCPAD_X, mods & PAD_BUTTON_X);
	Input_SetNonRepeatable(CCPAD_Y, mods & PAD_BUTTON_Y);
	
	Input_SetNonRepeatable(CCPAD_START,  mods & PAD_BUTTON_START);
	Input_SetNonRepeatable(CCPAD_SELECT, mods & PAD_TRIGGER_Z);
	
	Input_SetNonRepeatable(CCPAD_LEFT,  mods & PAD_BUTTON_LEFT);
	Input_SetNonRepeatable(CCPAD_RIGHT, mods & PAD_BUTTON_RIGHT);
	Input_SetNonRepeatable(CCPAD_UP,    mods & PAD_BUTTON_UP);
	Input_SetNonRepeatable(CCPAD_DOWN,  mods & PAD_BUTTON_DOWN);
}

static void ProcessPADInput(void) {
	PADStatus pads[4];
	PAD_Read(pads);
	if (pads[0].err) return;
	
	if (launcherMode) {
		ProcessPAD_Launcher(&pads[0]);
	} else {
		ProcessPAD_Game(&pads[0]);
	}
}


/*########################################################################################################################*
*----------------------------------------------------Input processing-----------------------------------------------------*
*#########################################################################################################################*/
#if defined HW_RVL
static int dragCurX, dragCurY;
static int dragStartX, dragStartY;
static cc_bool dragActive;

static void ProcessWPAD_Launcher(int mods) {
	Input_SetNonRepeatable(CCKEY_ENTER,  mods & WPAD_BUTTON_A);
	Input_SetNonRepeatable(CCKEY_ESCAPE, mods & WPAD_BUTTON_B);

	Input_SetNonRepeatable(CCPAD_LEFT,   mods & WPAD_BUTTON_LEFT);
	Input_SetNonRepeatable(CCPAD_RIGHT,  mods & WPAD_BUTTON_RIGHT);
	Input_SetNonRepeatable(CCPAD_UP,     mods & WPAD_BUTTON_UP);
	Input_SetNonRepeatable(CCPAD_DOWN,   mods & WPAD_BUTTON_DOWN);
}

static void ProcessWPAD_Game(int mods) {
	Input_SetNonRepeatable(CCPAD_L, mods & WPAD_BUTTON_1);
	Input_SetNonRepeatable(CCPAD_R, mods & WPAD_BUTTON_2);
      
	Input_SetNonRepeatable(CCPAD_A, mods & WPAD_BUTTON_A);
	Input_SetNonRepeatable(CCPAD_B, mods & WPAD_BUTTON_B);
	Input_SetNonRepeatable(CCPAD_X, mods & WPAD_BUTTON_PLUS);
      
	Input_SetNonRepeatable(CCPAD_START,  mods & WPAD_BUTTON_HOME);
	Input_SetNonRepeatable(CCPAD_SELECT, mods & WPAD_BUTTON_MINUS);

	Input_SetNonRepeatable(CCPAD_LEFT,   mods & WPAD_BUTTON_LEFT);
	Input_SetNonRepeatable(CCPAD_RIGHT,  mods & WPAD_BUTTON_RIGHT);
	Input_SetNonRepeatable(CCPAD_UP,     mods & WPAD_BUTTON_UP);
	Input_SetNonRepeatable(CCPAD_DOWN,   mods & WPAD_BUTTON_DOWN);
}

static void ProcessNunchuck_Game(int mods, double delta) {
	WPADData* wd = WPAD_Data(0);
	joystick_t analog = wd->exp.nunchuk.js;

	Input_SetNonRepeatable(CCPAD_L, mods & WPAD_NUNCHUK_BUTTON_C);
	Input_SetNonRepeatable(CCPAD_R, mods & WPAD_NUNCHUK_BUTTON_Z);
      
	Input_SetNonRepeatable(CCPAD_A, mods & WPAD_BUTTON_A);
	Input_SetNonRepeatable(CCPAD_Y, mods & WPAD_BUTTON_1);
	Input_SetNonRepeatable(CCPAD_X, mods & WPAD_BUTTON_2);

	Input_SetNonRepeatable(CCPAD_START,  mods & WPAD_BUTTON_HOME);
	Input_SetNonRepeatable(CCPAD_SELECT, mods & WPAD_BUTTON_MINUS);

	Input_SetNonRepeatable(KeyBinds[KEYBIND_FLY], mods & WPAD_BUTTON_LEFT);

	if (mods & WPAD_BUTTON_RIGHT) {
		Mouse_ScrollWheel(1.0*delta);
	}

	Input_SetNonRepeatable(KeyBinds[KEYBIND_THIRD_PERSON], mods & WPAD_BUTTON_UP);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_FLY_DOWN],    mods & WPAD_BUTTON_DOWN);

	const float ANGLE_DELTA = 50;
	bool nunchuckUp    = (analog.ang > -ANGLE_DELTA)    && (analog.ang < ANGLE_DELTA)     && (analog.mag > 0.5);
	bool nunchuckDown  = (analog.ang > 180-ANGLE_DELTA) && (analog.ang < 180+ANGLE_DELTA) && (analog.mag > 0.5);
	bool nunchuckLeft  = (analog.ang > -90-ANGLE_DELTA) && (analog.ang < -90+ANGLE_DELTA) && (analog.mag > 0.5);
	bool nunchuckRight = (analog.ang > 90-ANGLE_DELTA)  && (analog.ang < 90+ANGLE_DELTA)  && (analog.mag > 0.5);

	Input_SetNonRepeatable(CCPAD_LEFT,  nunchuckLeft);
	Input_SetNonRepeatable(CCPAD_RIGHT, nunchuckRight);
	Input_SetNonRepeatable(CCPAD_UP,    nunchuckUp);
	Input_SetNonRepeatable(CCPAD_DOWN,  nunchuckDown);
}

static void ProcessClassic_LeftJoystick(struct joystick_t* js) {
	// TODO: need to account for min/max??
	int dx = js->pos.x - js->center.x;
	int dy = js->pos.y - js->center.y;
	
	if (Math_AbsI(dx) <= 8) dx = 0;
	if (Math_AbsI(dy) <= 8) dy = 0;
	
	if (dx == 0 && dy == 0) return;
	Input.JoystickMovement = true;
	Input.JoystickAngle    = (js->ang - 90) * MATH_DEG2RAD;
}

static void ProcessClassic_RightJoystick(struct joystick_t* js) {
	// TODO: need to account for min/max??
	int dx = js->pos.x - js->center.x;
	int dy = js->pos.y - js->center.y;
	
	if (Math_AbsI(dx) <= 8) dx = 0;
	if (Math_AbsI(dy) <= 8) dy = 0;
	
	Event_RaiseRawMove(&PointerEvents.RawMoved, dx / 8.0f, -dy / 8.0f);
}

static void ProcessClassic_Game(void) {
	WPADData* wd = WPAD_Data(0);
	classic_ctrl_t ctrls = wd->exp.classic;
	int mods = ctrls.btns | ctrls.btns_held;

	Input_SetNonRepeatable(CCPAD_L, mods & CLASSIC_CTRL_BUTTON_FULL_L);
	Input_SetNonRepeatable(CCPAD_R, mods & CLASSIC_CTRL_BUTTON_FULL_R);
      
	Input_SetNonRepeatable(CCPAD_A, mods & CLASSIC_CTRL_BUTTON_A);
	Input_SetNonRepeatable(CCPAD_B, mods & CLASSIC_CTRL_BUTTON_B);
	Input_SetNonRepeatable(CCPAD_X, mods & CLASSIC_CTRL_BUTTON_X);
	Input_SetNonRepeatable(CCPAD_Y, mods & CLASSIC_CTRL_BUTTON_Y);
      
	Input_SetNonRepeatable(CCPAD_START,  mods & CLASSIC_CTRL_BUTTON_PLUS);
	Input_SetNonRepeatable(CCPAD_SELECT, mods & CLASSIC_CTRL_BUTTON_MINUS);

	Input_SetNonRepeatable(CCPAD_LEFT,   mods & CLASSIC_CTRL_BUTTON_LEFT);
	Input_SetNonRepeatable(CCPAD_RIGHT,  mods & CLASSIC_CTRL_BUTTON_RIGHT);
	Input_SetNonRepeatable(CCPAD_UP,     mods & CLASSIC_CTRL_BUTTON_UP);
	Input_SetNonRepeatable(CCPAD_DOWN,   mods & CLASSIC_CTRL_BUTTON_DOWN);
	
	if (Input.RawMode) {
		ProcessClassic_LeftJoystick(&ctrls.ljs);
		ProcessClassic_RightJoystick(&ctrls.rjs);
	}
}

static void GetIRPos(int res, int* x, int* y) {
	if (res == WPAD_ERR_NONE) {
		WPADData* wd = WPAD_Data(0);

		*x = wd->ir.x;
		*y = wd->ir.y;
	} else {
		*x = 0; 
		*y = 0;
	}
}

static void ProcessKeyboardInput(void) {
	keyboard_event ke;
	int res = KEYBOARD_GetEvent(&ke);
	
	if (res && ke.type == KEYBOARD_PRESSED)
	{
		//Platform_Log2("KEYCODE: %i (%i)", &ke.keycode, &ke.type);
		if (ke.symbol) Event_RaiseInt(&InputEvents.Press, ke.symbol);
	}
}

void Window_ProcessEvents(double delta) {
	Input.JoystickMovement = false;
	
	WPAD_ScanPads();
	u32 mods = WPAD_ButtonsDown(0) | WPAD_ButtonsHeld(0);
	u32 type;
	int res  = WPAD_Probe(0, &type);

	if (launcherMode) {
		ProcessWPAD_Launcher(mods);
	} else if (type == WPAD_EXP_NUNCHUK) {
		ProcessNunchuck_Game(mods, delta);
	} else if (type == WPAD_EXP_CLASSIC) {
		ProcessClassic_Game();
	} else {
		ProcessWPAD_Game(mods);
	}

	int x, y;
	GetIRPos(res, &x, &y);
	
	if (mods & WPAD_BUTTON_B) {
		if (!dragActive) {
			dragStartX = dragCurX = x;
			dragStartY = dragCurY = y;
		}
		dragActive = true;
	} else {
		dragActive = false;
	}
	Pointer_SetPosition(0, x, y);
	
	ProcessPADInput();
	ProcessKeyboardInput();
}

static void ScanAndGetIRPos(int* x, int* y) {
	u32 type;
	WPAD_ScanPads();
	
	int res = WPAD_Probe(0, &type);
	GetIRPos(res, x, y);
}
#define FACTOR 2
void Window_UpdateRawMouse(void)  {
	if (!dragActive) return;
	int x, y;
	ScanAndGetIRPos(&x, &y);
   
	// TODO: Refactor the logic. is it 100% right too?
	dragCurX = dragStartX + (dragCurX - dragStartX) / FACTOR;
	dragCurY = dragStartY + (dragCurY - dragStartY) / FACTOR;
	
	int dx = x - dragCurX; Math_Clamp(dx, -40, 40);
	int dy = y - dragCurY; Math_Clamp(dy, -40, 40);
	Event_RaiseRawMove(&PointerEvents.RawMoved, dx, dy);
	
	dragCurX = x; dragCurY = y;
}
#else
void Window_ProcessEvents(double delta) {
	Input.JoystickMovement = false;
	
	ProcessPADInput();
}

void Window_UpdateRawMouse(void) { }
#endif

void Cursor_SetPosition(int x, int y) { } // No point in GameCube/Wii
// TODO: Display cursor on Wii when not raw mode
void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
static struct Bitmap fb_bmp;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	fb_bmp     = *bmp;
}

// TODO: Get rid of this complexity and use the 3D API instead..
// https://github.com/devkitPro/gamecube-examples/blob/master/graphics/fb/pageflip/source/flip.c
static u32 CvtRGB (u8 r1, u8 g1, u8 b1, u8 r2, u8 g2, u8 b2)
{
  int y1, cb1, cr1, y2, cb2, cr2, cb, cr;
 
  y1  = (299 * r1 + 587 * g1 + 114 * b1) / 1000;
  cb1 = (-16874 * r1 - 33126 * g1 + 50000 * b1 + 12800000) / 100000;
  cr1 = (50000 * r1 - 41869 * g1 - 8131 * b1 + 12800000) / 100000;
 
  y2  = (299 * r2 + 587 * g2 + 114 * b2) / 1000;
  cb2 = (-16874 * r2 - 33126 * g2 + 50000 * b2 + 12800000) / 100000;
  cr2 = (50000 * r2 - 41869 * g2 - 8131 * b2 + 12800000) / 100000;
 
  cb = (cb1 + cb2) >> 1;
  cr = (cr1 + cr2) >> 1;
 
  return (y1 << 24) | (cb << 16) | (y2 << 8) | cr;
}

void Window_DrawFramebuffer(Rect2D r) {
	VIDEO_WaitVSync();
	r.X &= ~0x01; // round down to nearest even horizontal index
	
	// TODO XFB is raw yuv, but is absolutely a pain to work with..
	for (int y = r.Y; y < r.Y + r.Height; y++) 
	{
		cc_uint32* src = fb_bmp.scan0 + y * fb_bmp.width   + r.X;
		u16* dst       = (u16*)xfb    + y * rmode->fbWidth + r.X;
		
		for (int x = 0; x < r.Width / 2; x++) {
			cc_uint32 rgb0 = src[(x<<1) + 0];
			cc_uint32 rgb1 = src[(x<<1) + 1];
			
			((u32*)dst)[x] = CvtRGB(BitmapCol_R(rgb0), BitmapCol_G(rgb0), BitmapCol_B(rgb0),
					BitmapCol_R(rgb1),  BitmapCol_G(rgb1), BitmapCol_B(rgb1));
		}
	}
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}


/*########################################################################################################################*
*-------------------------------------------------------Misc/Other--------------------------------------------------------*
*#########################################################################################################################*/
void Window_SetTitle(const cc_string* title)   { }
void Clipboard_GetText(cc_string* value)       { }
void Clipboard_SetText(const cc_string* value) { }

int Window_GetWindowState(void) { return WINDOW_STATE_FULLSCREEN; }
cc_result Window_EnterFullscreen(void) { return 0; }
cc_result Window_ExitFullscreen(void)  { return 0; }
int Window_IsObscured(void)            { return 0; }

void Window_Show(void) { }
void Window_SetSize(int width, int height) { }


void Window_OpenKeyboard(struct OpenKeyboardArgs* args) { /* TODO implement */ }
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { /* TODO implement */ }

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