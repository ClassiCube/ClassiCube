#include "../Window.h"
#include "../Platform.h"
#include "../Input.h"
#include "../InputHandler.h"
#include "../Event.h"
#include "../String_.h"
#include "../Funcs.h"
#include "../Bitmap.h"
#include "../Errors.h"
#include "../ExtMath.h"
#include "../Graphics.h"
#include "../Options.h"
#include "../VirtualKeyboard.h"
#include "../VirtualDialog.h"

#include <gccore.h>
#if defined HW_RVL
#include <ogc/usbmouse.h>
#include <wiiuse/wpad.h>
#include <wiikeyboard/keyboard.h>
#endif

static cc_bool needsFBUpdate;
static int mouseSupported;
#include "../VirtualCursor.h"
static void* xfb;

GXRModeObj* cur_mode;
void* Window_XFB;
struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;


static void OnPowerOff(void) {
	Window_Main.Exists = false;
	Window_RequestClose();
}
static void InitVideo(void) {
	VIDEO_Init();

	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	cur_mode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(cur_mode));
	Window_XFB = xfb;	

	VIDEO_Configure(cur_mode);
	VIDEO_SetNextFramebuffer(xfb);

	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	VIDEO_ClearFrameBuffer(cur_mode, xfb, COLOR_GRAY);
	VIDEO_WaitVSync();
}

void Window_PreInit(void) {
	// TODO: SYS_SetResetCallback(reload); too? not sure how reset differs on GC/WII
	#if defined HW_RVL
	SYS_SetPowerCallback(OnPowerOff);
	#endif
	InitVideo();

	DisplayInfo.Width  = cur_mode->fbWidth;
	DisplayInfo.Height = cur_mode->xfbHeight;
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
}

void Window_Init(void) {
	Window_Main.Width    = DisplayInfo.Width;
	Window_Main.Height   = DisplayInfo.Height;
	Window_Main.Focused  = true;
	
	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;

	DisplayInfo.ContentOffsetX = Option_GetOffsetX(10);
	DisplayInfo.ContentOffsetY = Option_GetOffsetY(10);
	Window_Main.SoftKeyboard   = SOFT_KEYBOARD_VIRTUAL;

	#if defined HW_RVL
	KEYBOARD_Init(NULL);
	mouseSupported = MOUSE_Init() >= 0;
	#endif
}

void Window_Free(void) { }

void Window_Create2D(int width, int height) {
	Window_Main.Is3D = false; 
}

void Window_Create3D(int width, int height) { 
	Window_Main.Is3D = true;
}

void Window_Destroy(void) { }

void Window_RequestClose(void) {
	Event_RaiseVoid(&WindowEvents.Closing);
}


/*########################################################################################################################*
*---------------------------------------------GameCube controller processing----------------------------------------------*
*#########################################################################################################################*/
static PADStatus gc_pads[PAD_CHANMAX];

static const BindMapping defaults_gc[BIND_COUNT] = {
	[BIND_FLY_UP]       = { CCPAD_UP    },  
	[BIND_FLY_DOWN]     = { CCPAD_DOWN  },
	[BIND_HOTBAR_LEFT]  = { CCPAD_LEFT  },  
	[BIND_HOTBAR_RIGHT] = { CCPAD_RIGHT },
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
};

#define PAD_AXIS_SCALE 8.0f
static void ProcessPAD_Joystick(int port, int axis, int x, int y, float delta) {
	// May not be exactly 0 on actual hardware
	if (Math_AbsI(x) <= 8) x = 0;
	if (Math_AbsI(y) <= 8) y = 0;
	
	Gamepad_SetAxis(port, axis, x / PAD_AXIS_SCALE, -y / PAD_AXIS_SCALE, delta);
}

static void ProcessPAD_Buttons(int port, int mods) {
	Gamepad_SetButton(port, CCPAD_L, mods & PAD_TRIGGER_L);
	Gamepad_SetButton(port, CCPAD_R, mods & PAD_TRIGGER_R);
	
	Gamepad_SetButton(port, CCPAD_1, mods & PAD_BUTTON_A);
	Gamepad_SetButton(port, CCPAD_2, mods & PAD_BUTTON_B);
	Gamepad_SetButton(port, CCPAD_3, mods & PAD_BUTTON_X);
	Gamepad_SetButton(port, CCPAD_4, mods & PAD_BUTTON_Y);
	
	Gamepad_SetButton(port, CCPAD_START,  mods & PAD_BUTTON_START);
	Gamepad_SetButton(port, CCPAD_SELECT, mods & PAD_TRIGGER_Z);
	
	Gamepad_SetButton(port, CCPAD_LEFT,   mods & PAD_BUTTON_LEFT);
	Gamepad_SetButton(port, CCPAD_RIGHT,  mods & PAD_BUTTON_RIGHT);
	Gamepad_SetButton(port, CCPAD_UP,     mods & PAD_BUTTON_UP);
	Gamepad_SetButton(port, CCPAD_DOWN,   mods & PAD_BUTTON_DOWN);
}

static void ProcessPADInput(PADStatus* pad, int i, float delta) {
	int error = pad->err;

	if (error == PAD_ERR_NONE) {
		gc_pads[i] = *pad; // new state arrived
	} else if (error == PAD_ERR_TRANSFER) {
		// usually means still busy transferring state - use last state
	} else {
		if (error == PAD_ERR_NO_CONTROLLER) PAD_Reset(PAD_CHAN0_BIT >> i);
		return; // not connected, still busy, etc
	}
	
	int port = Gamepad_Connect(0x5C + i, defaults_gc);
	ProcessPAD_Buttons(port, gc_pads[i].button);
	ProcessPAD_Joystick(port, PAD_AXIS_LEFT,  gc_pads[i].stickX,    gc_pads[i].stickY,    delta);
	ProcessPAD_Joystick(port, PAD_AXIS_RIGHT, gc_pads[i].substickX, gc_pads[i].substickY, delta);
}

static void ProcessPADInputs(float delta) {
	PADStatus pads[PAD_CHANMAX];
	PAD_Read(pads);

	for (int i = 0; i < PAD_CHANMAX; i++)
	{
		ProcessPADInput(&pads[i], i, delta);
	}
}


/*########################################################################################################################*
*--------------------------------------------------Keyboard processing----------------------------------------------------*
*#########################################################################################################################*/
#if defined HW_RVL
static const cc_uint8 key_map[] = {
/* 0x00 */ 0,0,0,0,         'A','B','C','D', 
/* 0x08 */ 'E','F','G','H', 'I','J','K','L',
/* 0x10 */ 'M','N','O','P', 'Q','R','S','T',
/* 0x18 */ 'U','V','W','X', 'Y','Z','1','2',
/* 0x20 */ '3','4','5','6', '7','8','9','0',
/* 0x28 */ CCKEY_ENTER,CCKEY_ESCAPE,CCKEY_BACKSPACE,CCKEY_TAB, CCKEY_SPACE,CCKEY_MINUS,CCKEY_EQUALS,CCKEY_LBRACKET,
/* 0x30 */ CCKEY_RBRACKET,CCKEY_BACKSLASH,0,CCKEY_SEMICOLON, CCKEY_QUOTE,CCKEY_TILDE,CCKEY_COMMA,CCKEY_PERIOD,
/* 0x38 */ CCKEY_SLASH,CCKEY_CAPSLOCK,CCKEY_F1,CCKEY_F2, CCKEY_F3,CCKEY_F4,CCKEY_F5,CCKEY_F6,
/* 0x40 */ CCKEY_F7,CCKEY_F8,CCKEY_F9,CCKEY_F10, CCKEY_F11,CCKEY_F12,CCKEY_PRINTSCREEN,CCKEY_SCROLLLOCK,
/* 0x48 */ CCKEY_PAUSE,CCKEY_INSERT,CCKEY_HOME,CCKEY_PAGEUP, CCKEY_DELETE,CCKEY_END,CCKEY_PAGEDOWN,CCKEY_RIGHT,
/* 0x50 */ CCKEY_LEFT,CCKEY_DOWN,CCKEY_UP,CCKEY_NUMLOCK, CCKEY_KP_DIVIDE,CCKEY_KP_MULTIPLY,CCKEY_KP_MINUS,CCKEY_KP_PLUS,
/* 0x58 */ CCKEY_KP_ENTER,CCKEY_KP1,CCKEY_KP2,CCKEY_KP3, CCKEY_KP4,CCKEY_KP5,CCKEY_KP6,CCKEY_KP7,
/* 0x60 */ CCKEY_KP8,CCKEY_KP9,CCKEY_KP0,CCKEY_KP_DECIMAL, 0,0,0,0,
/* 0x68 */ 0,0,0,0, 0,0,0,0,
/* 0x70 */ 0,0,0,0, 0,0,0,0,
/* 0x78 */ 0,0,0,0, 0,0,0,0,
/* 0x80 */ 0,0,0,0, 0,0,0,0,
/* 0x88 */ 0,0,0,0, 0,0,0,0,
/* 0x90 */ 0,0,0,0, 0,0,0,0,
/* 0x98 */ 0,0,0,0, 0,0,0,0,
/* 0xA0 */ 0,0,0,0, 0,0,0,0,
/* 0xA8 */ 0,0,0,0, 0,0,0,0,
/* 0xB0 */ 0,0,0,0, 0,0,0,0,
/* 0xB8 */ 0,0,0,0, 0,0,0,0,
/* 0xC0 */ 0,0,0,0, 0,0,0,0,
/* 0xC8 */ 0,0,0,0, 0,0,0,0,
/* 0xD0 */ 0,0,0,0, 0,0,0,0,
/* 0xD8 */ 0,0,0,0, 0,0,0,0,
/* 0xE0 */ CCKEY_LCTRL,CCKEY_LSHIFT,CCKEY_LALT,CCKEY_LWIN, CCKEY_RCTRL,CCKEY_RSHIFT,CCKEY_RALT,CCKEY_RWIN
};

static int MapNativeKey(unsigned key) {
	return key < Array_Elems(key_map) ? key_map[key] : 0;
}

static void ProcessKeyboardInput(void) {
	keyboard_event ke;
	int res = KEYBOARD_GetEvent(&ke);
	int key;
	
	if (!res) return;
	Input.Sources |= INPUT_SOURCE_NORMAL;
	
	if (res && ke.type == KEYBOARD_PRESSED)
	{
		key = MapNativeKey(ke.keycode);
		if (key) Input_SetPressed(key);
		//Platform_Log2("KEYCODE: %i (%i)", &ke.keycode, &ke.type);
		if (ke.symbol) Event_RaiseInt(&InputEvents.Press, ke.symbol);
	}
	if (res && ke.type == KEYBOARD_RELEASED)
	{
		key = MapNativeKey(ke.keycode);
		if (key) Input_SetReleased(key);
	}
}
#endif


/*########################################################################################################################*
*---------------------------------------------------Mouse processing------------------------------------------------------*
*#########################################################################################################################*/
#if defined HW_RVL
static void ProcessMouseInput(float delta) {
	if (!mouseSupported)      return;
	if (!MOUSE_IsConnected()) return;

	mouse_event event;
    if (MOUSE_GetEvent(&event) == 0) return;

	Input_SetNonRepeatable(CCMOUSE_L, event.button & 1);
	Input_SetNonRepeatable(CCMOUSE_R, event.button & 2);
	Input_SetNonRepeatable(CCMOUSE_M, event.button & 4);
	Mouse_ScrollVWheel(event.rz * 0.5f);

	if (!vc_hooked) {
		Pointer_SetPosition(0, Window_Main.Width / 2, Window_Main.Height / 2);
	}
	VirtualCursor_SetPosition(Pointers[0].x + event.rx, Pointers[0].y + event.ry);
	
	if (!Input.RawMode) return;	
	float scale = (delta * 60.0) / 2.0f;
	Event_RaiseRawMove(&PointerEvents.RawMoved, 
				event.rx * scale, event.ry * scale);
}
#endif


/*########################################################################################################################*
*----------------------------------------------------Input processing-----------------------------------------------------*
*#########################################################################################################################*/
#if defined HW_RVL
static int dragCurX, dragCurY;
static int dragStartX, dragStartY;
static cc_bool dragActive;

void Window_ProcessEvents(float delta) {
	ProcessKeyboardInput();
    ProcessMouseInput(delta);
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

static void ScanAndGetIRPos(int* x, int* y) {
	u32 type;
	WPAD_ScanPads();
	
	int res = WPAD_Probe(0, &type);
	GetIRPos(res, x, y);
}


static void ProcessWPADDrag(int res, u32 mods) {
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
	VirtualCursor_SetPosition(x, y);
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
void Window_ProcessEvents(float delta) {
}

void Window_UpdateRawMouse(void) { }
#endif

void Cursor_SetPosition(int x, int y) { } // No point in GameCube/Wii
// TODO: Display cursor on Wii when not raw mode
void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }


/*########################################################################################################################*
*-------------------------------------------------------Gamepads----------------------------------------------------------*
*#########################################################################################################################*/
void Gamepads_PreInit(void) {
	#if defined HW_RVL
	WPAD_Init();
	for (int i = 0; i < 4; i++)
		WPAD_SetDataFormat(i, WPAD_FMT_BTNS_ACC_IR);
	#endif
	PAD_Init();
}

void Gamepads_Init(void) { }

#if defined HW_RVL
static const BindMapping default_nunchuck[BIND_COUNT] = {
	[BIND_FORWARD] = { CCPAD_CUP,    0 },
	[BIND_BACK]    = { CCPAD_CDOWN,  0 },
	[BIND_LEFT]    = { CCPAD_CLEFT,  0 },
	[BIND_RIGHT]   = { CCPAD_CRIGHT, 0 },
	
	[BIND_THIRD_PERSON] = { CCPAD_UP,    0 },
	[BIND_FLY_DOWN]     = { CCPAD_DOWN,  0 },
	[BIND_FLY]          = { CCPAD_LEFT,  0 },
	[BIND_HOTBAR_RIGHT] = { CCPAD_RIGHT, 0 },
	
	[BIND_JUMP]         = { CCPAD_1, 0 },
	[BIND_CHAT]         = { CCPAD_L, 0 },
	[BIND_INVENTORY]    = { CCPAD_R, 0 },
	
	[BIND_PLACE_BLOCK]  = { CCPAD_5, 0 },
	[BIND_DELETE_BLOCK] = { CCPAD_6, 0 },
	[BIND_SET_SPAWN]    = { CCPAD_START, 0 },
	[BIND_SEND_CHAT]    = { CCPAD_START, 0 },
};

static int dragCurX, dragCurY;
static int dragStartX, dragStartY;
static cc_bool dragActive;

static void ProcessWPAD_Buttons(int port, int mods) {
	Gamepad_SetButton(port, CCPAD_L, mods & WPAD_BUTTON_1);
	Gamepad_SetButton(port, CCPAD_R, mods & WPAD_BUTTON_2);
      
	Gamepad_SetButton(port, CCPAD_1, mods & WPAD_BUTTON_A);
	Gamepad_SetButton(port, CCPAD_2, mods & WPAD_BUTTON_B);
	Gamepad_SetButton(port, CCPAD_3, mods & WPAD_BUTTON_PLUS);
      
	Gamepad_SetButton(port, CCPAD_START,  mods & WPAD_BUTTON_HOME);
	Gamepad_SetButton(port, CCPAD_SELECT, mods & WPAD_BUTTON_MINUS);

	Gamepad_SetButton(port, CCPAD_LEFT,   mods & WPAD_BUTTON_LEFT);
	Gamepad_SetButton(port, CCPAD_RIGHT,  mods & WPAD_BUTTON_RIGHT);
	Gamepad_SetButton(port, CCPAD_UP,     mods & WPAD_BUTTON_UP);
	Gamepad_SetButton(port, CCPAD_DOWN,   mods & WPAD_BUTTON_DOWN);
}

static void ProcessNunchuck(int port, WPADData* wd, int mods) {
	joystick_t analog = wd->exp.nunchuk.js;
	Gamepad_SetButton(port, CCPAD_5, mods & WPAD_NUNCHUK_BUTTON_Z);
	Gamepad_SetButton(port, CCPAD_6, mods & WPAD_NUNCHUK_BUTTON_C);

	const float ANGLE_DELTA = 50;
	bool nunchuckUp    = (analog.ang > -ANGLE_DELTA)    && (analog.ang < ANGLE_DELTA)     && (analog.mag > 0.5);
	bool nunchuckDown  = (analog.ang > 180-ANGLE_DELTA) && (analog.ang < 180+ANGLE_DELTA) && (analog.mag > 0.5);
	bool nunchuckLeft  = (analog.ang > -90-ANGLE_DELTA) && (analog.ang < -90+ANGLE_DELTA) && (analog.mag > 0.5);
	bool nunchuckRight = (analog.ang > 90-ANGLE_DELTA)  && (analog.ang < 90+ANGLE_DELTA)  && (analog.mag > 0.5);

	Gamepad_SetButton(port, CCPAD_CLEFT,  nunchuckLeft);
	Gamepad_SetButton(port, CCPAD_CRIGHT, nunchuckRight);
	Gamepad_SetButton(port, CCPAD_CUP,    nunchuckUp);
	Gamepad_SetButton(port, CCPAD_CDOWN,  nunchuckDown);
}

#define CLASSIC_AXIS_SCALE 2.0f
static void ProcessClassic_Joystick(int port, int axis, struct joystick_t* js, float delta) {
	// TODO: need to account for min/max?? see libogc
	int x = js->pos.x - js->center.x;
	int y = js->pos.y - js->center.y;
	
	if (Math_AbsI(x) <= 8) x = 0;
	if (Math_AbsI(y) <= 8) y = 0;
	
	Gamepad_SetAxis(port, axis, x / CLASSIC_AXIS_SCALE, -y / CLASSIC_AXIS_SCALE, delta);
}

static void ProcessClassicButtons(int port, int mods) {
	Gamepad_SetButton(port, CCPAD_L, mods & CLASSIC_CTRL_BUTTON_FULL_L);
	Gamepad_SetButton(port, CCPAD_R, mods & CLASSIC_CTRL_BUTTON_FULL_R);
      
	Gamepad_SetButton(port, CCPAD_1, mods & CLASSIC_CTRL_BUTTON_A);
	Gamepad_SetButton(port, CCPAD_2, mods & CLASSIC_CTRL_BUTTON_B);
	Gamepad_SetButton(port, CCPAD_3, mods & CLASSIC_CTRL_BUTTON_X);
	Gamepad_SetButton(port, CCPAD_4, mods & CLASSIC_CTRL_BUTTON_Y);
      
	Gamepad_SetButton(port, CCPAD_START,  mods & CLASSIC_CTRL_BUTTON_PLUS);
	Gamepad_SetButton(port, CCPAD_SELECT, mods & CLASSIC_CTRL_BUTTON_MINUS);

	Gamepad_SetButton(port, CCPAD_LEFT,   mods & CLASSIC_CTRL_BUTTON_LEFT);
	Gamepad_SetButton(port, CCPAD_RIGHT,  mods & CLASSIC_CTRL_BUTTON_RIGHT);
	Gamepad_SetButton(port, CCPAD_UP,     mods & CLASSIC_CTRL_BUTTON_UP);
	Gamepad_SetButton(port, CCPAD_DOWN,   mods & CLASSIC_CTRL_BUTTON_DOWN);
	
	Gamepad_SetButton(port, CCPAD_ZL, mods & CLASSIC_CTRL_BUTTON_ZL);
	Gamepad_SetButton(port, CCPAD_ZR, mods & CLASSIC_CTRL_BUTTON_ZR);
}

static void ProcessClassicInput(int port, WPADData* wd, float delta) {
	classic_ctrl_t ctrls = wd->exp.classic;
	int mods = ctrls.btns | ctrls.btns_held;

	ProcessClassicButtons(port, mods);
	ProcessClassic_Joystick(port, PAD_AXIS_LEFT,  &ctrls.ljs, delta);
	ProcessClassic_Joystick(port, PAD_AXIS_RIGHT, &ctrls.rjs, delta);
}

static int frame;
static void ProcessWPADInput(int i, float delta) {
	WPAD_ScanPads();
	// First time WPADs are scanned, type is 0 even for classic/nunchuck it seems
	//  (in Dolphin at least). So delay for a little bit
	if (frame < 4 * 5) { frame++; return; }

	u32 type;
	int res = WPAD_Probe(i, &type);
	if (res) return;

	WPADData* wd = WPAD_Data(i);
	u32 mods = wd->btns_h | wd->btns_d; // buttons held | buttons down now
	int port;

	if (type == WPAD_EXP_CLASSIC) {
		port = Gamepad_Connect(0xC1 + i, PadBind_Defaults);
		ProcessClassicInput(port, wd, delta);
	} else if (type == WPAD_EXP_NUNCHUK) {
		port = Gamepad_Connect(0xCC + i, default_nunchuck);
		ProcessWPAD_Buttons(port, mods);
		ProcessNunchuck(port, wd, mods);
	} else {
		port = Gamepad_Connect(0x11 + i, PadBind_Defaults);
		ProcessWPAD_Buttons(port, mods);
	}

	ProcessWPADDrag(res, mods);
}

void Gamepads_Process(float delta) {
	for (int i = 0; i < 4; i++)
	{
		ProcessWPADInput(i, delta);
	}
	ProcessPADInputs(delta);
}
#else
void Gamepads_Process(float delta) {
	ProcessPADInputs(delta);
}
#endif


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	bmp->width  = width;
	bmp->height = height;

	needsFBUpdate = true;
}

// TODO: Get rid of this complexity and use the 3D API instead..
// https://github.com/extremscorner/gamecube-examples/blob/master/graphics/fb/pageflip/source/flip.c
static u32 CvtRGB(u8 r1, u8 g1, u8 b1, u8 r2, u8 g2, u8 b2) {
	u8 y1, cb1, cr1, y2, cb2, cr2, cb, cr;

	y1  = ((16829 * r1 + 33039 * g1 +  6416 * b1) >> 16) +  16;
	cb1 = ((-9714 * r1 - 19071 * g1 + 28784 * b1) >> 16) + 128;
	cr1 = ((28784 * r1 - 24103 * g1 -  4681 * b1) >> 16) + 128;

	y2  = ((16829 * r2 + 33039 * g2 +  6416 * b2) >> 16) +  16;
	cb2 = ((-9714 * r2 - 19071 * g2 + 28784 * b2) >> 16) + 128;
	cr2 = ((28784 * r2 - 24103 * g2 -  4681 * b2) >> 16) + 128;

	cb = (cb1 + cb2) >> 1;
	cr = (cr1 + cr2) >> 1;

	return (y1 << 24) | (cb << 16) | (y2 << 8) | cr;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	// When coming back from the 3D game, framebuffer might have changed
	if (needsFBUpdate) {
		VIDEO_SetNextFramebuffer(xfb);
		VIDEO_Flush();
		needsFBUpdate = false;
	}
	
	VIDEO_WaitVSync();
	r.x &= ~0x01; // round down to nearest even horizontal index
	
	// TODO XFB is raw yuv, but is absolutely a pain to work with..
	for (int y = r.y; y < r.y + r.height; y++) 
	{
		cc_uint32* src = Bitmap_GetRow(bmp, y)              + r.x;
		u16* dst       = (u16*)xfb  + y * cur_mode->fbWidth + r.x;
		
		for (int x = 0; x < r.width / 2; x++) {
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
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) {
	if (Input.Sources & INPUT_SOURCE_NORMAL) return;
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
void Window_SetTitle(const cc_string* title)   { }
void Clipboard_GetText(cc_string* value)       { }
void Clipboard_SetText(const cc_string* value) { }

int Window_GetWindowState(void) { return WINDOW_STATE_FULLSCREEN; }
cc_result Window_EnterFullscreen(void) { return 0; }
cc_result Window_ExitFullscreen(void)  { return 0; }
int Window_IsObscured(void)            { return 0; }

void Window_Show(void) { }
void Window_SetSize(int width, int height) { }

void Window_ShowDialog(const char* title, const char* msg) {
	VirtualDialog_Show(title, msg, false);
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

