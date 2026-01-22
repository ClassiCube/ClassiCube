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
#include "../Logger.h"
#include "../Options.h"
#include "../VirtualKeyboard.h"
#include "../VirtualDialog.h"

#include <libpad.h>
#include <packet.h>
#include <dma_tags.h>
#include <gif_tags.h>
#include <gs_psm.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>
#include <kernel.h>
#include <libkbd.h>
#include <libmouse.h>

static cc_bool mouseSupported, kbdSupported;
#include "../VirtualCursor.h"
struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;

framebuffer_t fb_colors[2];
zbuffer_t     fb_depth;

static int display_mode;
extern void Gfx_AllocFramebuffers(void);

static void InitDisplay(void) {
	framebuffer_t* fb = &fb_colors[0];
	int interlaced = display_mode == GRAPH_MODE_NTSC || display_mode == GRAPH_MODE_PAL || display_mode == GRAPH_MODE_HDTV_1080I;
	int mode       = interlaced ? GRAPH_MODE_INTERLACED : GRAPH_MODE_NONINTERLACED;
	int display    = interlaced ? GRAPH_MODE_FIELD      : GRAPH_MODE_FRAME;

	graph_set_mode(mode, display_mode, display, GRAPH_ENABLE);
	graph_set_screen(0, 0, fb->width, fb->height);

	graph_set_bgcolor(50, 50, 50);
	graph_set_framebuffer_filtered(fb->address, fb->width, fb->psm, 0, 0);
	graph_enable_output();
}


void Window_PreInit(void) {
	dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
	dma_channel_fast_waits(DMA_CHANNEL_GIF);
	display_mode = graph_get_region();

	DisplayInfo.Width  = 640;
	DisplayInfo.Height = display_mode == GRAPH_MODE_PAL ? 512 : 448;
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;

	Gfx_AllocFramebuffers();
	InitDisplay();
}

void Window_Init(void) {
	Window_Main.Width    = DisplayInfo.Width;
	Window_Main.Height   = DisplayInfo.Height;
	Window_Main.Focused  = true;
	
	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = 1.0f / Window_Main.Height;

	DisplayInfo.ContentOffsetX = Option_GetOffsetX(10);
	DisplayInfo.ContentOffsetY = Option_GetOffsetX(10);
	Window_Main.SoftKeyboard   = SOFT_KEYBOARD_VIRTUAL;

	if (PS2MouseInit() >= 0) {
		PS2MouseSetReadMode(PS2MOUSE_READMODE_DIFF);
		mouseSupported = true;
	}
	if (PS2KbdInit() >= 0) {
		PS2KbdSetReadmode(PS2KBD_READMODE_RAW);
		kbdSupported = true;
	}
}

void Window_Free(void) { }

void Window_Create2D(int width, int height) {
	graph_shutdown();
	InitDisplay();
	Window_Main.Is3D = false;
}

void Window_Create3D(int width, int height) {
	graph_shutdown();
	InitDisplay();
	Window_Main.Is3D = true;
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
*--------------------------------------------------Keyboard processing----------------------------------------------------*
*#########################################################################################################################*/
static const cc_uint8 key_map[] = {
/* 00 */	  0,   0,   0,   0, 'A', 'B', 'C', 'D',
/* 08 */	'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
/* 10 */	'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
/* 18 */	'U', 'V', 'W', 'X', 'Y', 'Z', '1', '2',
/* 20 */	'3', '4', '5', '6', '7', '8', '9', '0',
/* 28 */	CCKEY_ENTER, CCKEY_ESCAPE, CCKEY_BACKSPACE, CCKEY_TAB, CCKEY_SPACE, CCKEY_MINUS, CCKEY_EQUALS, CCKEY_LBRACKET,
/* 30 */	CCKEY_RBRACKET, CCKEY_BACKSLASH, 0, CCKEY_SEMICOLON, CCKEY_QUOTE, CCKEY_TILDE, CCKEY_COMMA, CCKEY_PERIOD,
/* 38 */	CCKEY_SLASH, CCKEY_CAPSLOCK, CCKEY_F1, CCKEY_F2, CCKEY_F3, CCKEY_F4, CCKEY_F5, CCKEY_F6,
/* 40 */	CCKEY_F7, CCKEY_F8, CCKEY_F9, CCKEY_F10, CCKEY_F11, CCKEY_F12, CCKEY_PRINTSCREEN, CCKEY_SCROLLLOCK,
/* 48 */	CCKEY_PAUSE, CCKEY_INSERT, CCKEY_HOME, CCKEY_PAGEUP, CCKEY_DELETE, CCKEY_END, CCKEY_PAGEDOWN, CCKEY_RIGHT,
/* 50 */	CCKEY_LEFT, CCKEY_DOWN, CCKEY_UP, CCKEY_NUMLOCK, CCKEY_KP_DIVIDE, CCKEY_KP_MULTIPLY, CCKEY_KP_MINUS, CCKEY_KP_PLUS,
/* 58 */	CCKEY_KP_ENTER, CCKEY_KP1, CCKEY_KP2, CCKEY_KP3, CCKEY_KP4, CCKEY_KP5, CCKEY_KP6, CCKEY_KP7,
/* 60 */	CCKEY_KP8, CCKEY_KP9, CCKEY_KP0, CCKEY_KP_DECIMAL, 0, CCKEY_SLEEP, CCKEY_LAUNCH_APP1, 0,
/* 68 */	0, 0, 0, 0, 0, 0, 0, 0,
/* 70 */	0, 0, 0, 0, 0, 0, 0, 0,
/* 78 */	0, 0, 0, 0, 0, 0, 0, 0,
/* 80 */	0, 0, 0, 0, 0, 0, 0, 0,
/* 88 */	0, 0, 0, 0, 0, 0, 0, 0,
/* 90 */	0, 0, 0, 0, 0, 0, 0, 0,
/* 98 */	0, 0, 0, 0, 0, 0, 0, 0,
/* A0 */	0, 0, 0, 0, 0, 0, 0, 0,
/* A8 */	0, 0, 0, 0, 0, 0, 0, 0,
/* B0 */	0, 0, 0, 0, 0, 0, 0, 0,
/* B8 */	0, 0, 0, 0, 0, 0, 0, 0,
/* C0 */	0, 0, 0, 0, 0, 0, 0, 0,
/* C8 */	0, 0, 0, 0, 0, 0, 0, 0,
/* D0 */	0, 0, 0, 0, 0, 0, 0, 0,
/* D8 */	0, 0, 0, 0, 0, 0, 0, 0,
/* E0 */	CCKEY_LCTRL, CCKEY_LSHIFT, CCKEY_LALT, 0, CCKEY_RCTRL, CCKEY_RSHIFT, CCKEY_RALT, 0,
};

static void ProcessKeyboardInput(void) {
	if (!kbdSupported) return;

	PS2KbdRawKey key;
	if (PS2KbdReadRaw(&key) <= 0) return;

	Platform_Log1("%i", &key.key);
}


/*########################################################################################################################*
*----------------------------------------------------Input processing-----------------------------------------------------*
*#########################################################################################################################*/
static void ProcessMouseInput(float delta) {
	if (!mouseSupported)     return;
	if (PS2MouseEnum() == 0) return;

	mouse_data mData = { 0 };
	if (PS2MouseRead(&mData) < 0) return;

	Input_SetNonRepeatable(CCMOUSE_L, mData.buttons & PS2MOUSE_BTN1);
	Input_SetNonRepeatable(CCMOUSE_R, mData.buttons & PS2MOUSE_BTN2);
	Input_SetNonRepeatable(CCMOUSE_M, mData.buttons & PS2MOUSE_BTN3);
	Mouse_ScrollVWheel(mData.wheel * 0.5f);

	if (!vc_hooked) {
		Pointer_SetPosition(0, Window_Main.Width / 2, Window_Main.Height / 2);
	}
	VirtualCursor_SetPosition(Pointers[0].x + mData.x, Pointers[0].y + mData.y);
	
	if (!Input.RawMode) return;	
	float scale = (delta * 60.0) / 2.0f;
	Event_RaiseRawMove(&PointerEvents.RawMoved, 
				mData.x * scale, mData.y * scale);
}

void Window_ProcessEvents(float delta) {
	ProcessMouseInput(delta);
	ProcessKeyboardInput();
}

void Cursor_SetPosition(int x, int y) {
	if (vc_hooked) VirtualCursor_SetPosition(x, y);
}

void Window_EnableRawMouse(void)  { Input.RawMode = true; }
void Window_UpdateRawMouse(void)  {  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }


/*########################################################################################################################*
*-------------------------------------------------------Gamepads----------------------------------------------------------*
*#########################################################################################################################*/
static const BindMapping defaults_ps2[BIND_COUNT] = {
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

static char padBuf0[256] CC_ALIGNED(64);
static char padBuf1[256] CC_ALIGNED(64);

void Gamepads_PreInit(void) {
	padInit(0);
	padPortOpen(0, 0, padBuf0);
	padPortOpen(1, 0, padBuf1);
}

void Gamepads_Init(void) {
	Input_DisplayNames[CCPAD_1] = "CIRCLE";
	Input_DisplayNames[CCPAD_2] = "CROSS";
	Input_DisplayNames[CCPAD_3] = "SQUARE";
	Input_DisplayNames[CCPAD_4] = "TRIANGLE";
}

static void HandleButtons(int port, int buttons) {
	// Confusingly, it seems that when a bit is on, it means the button is NOT pressed
	// So just flip the bits to make more sense
	buttons = buttons ^ 0xFFFF;
	//Platform_Log1("BUTTONS: %h", &buttons);
	
	Gamepad_SetButton(port, CCPAD_1, buttons & PAD_CIRCLE);
	Gamepad_SetButton(port, CCPAD_2, buttons & PAD_CROSS);
	Gamepad_SetButton(port, CCPAD_3, buttons & PAD_SQUARE);
	Gamepad_SetButton(port, CCPAD_4, buttons & PAD_TRIANGLE);
      
	Gamepad_SetButton(port, CCPAD_START,  buttons & PAD_START);
	Gamepad_SetButton(port, CCPAD_SELECT, buttons & PAD_SELECT);
	Gamepad_SetButton(port, CCPAD_LSTICK, buttons & PAD_L3);
	Gamepad_SetButton(port, CCPAD_RSTICK, buttons & PAD_L3);

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
	if (Math_AbsI(x) <= 32) x = 0;
	if (Math_AbsI(y) <= 32) y = 0;
	
	Gamepad_SetAxis(port, axis, x / AXIS_SCALE, y / AXIS_SCALE, delta);
}

static void ProcessPadInput(int port, float delta, struct padButtonStatus* pad) {
	HandleButtons(port, pad->btns);
	HandleJoystick(port, PAD_AXIS_LEFT,  pad->ljoy_h - 0x80, pad->ljoy_v - 0x80, delta);
	HandleJoystick(port, PAD_AXIS_RIGHT, pad->rjoy_h - 0x80, pad->rjoy_v - 0x80, delta);
}

static cc_bool setMode[2];
static void ProcessPad(int i, float delta) {
	struct padButtonStatus pad;
	int state = padGetState(i, 0);
	if (state != PAD_STATE_STABLE) return;

	// Change to DUALSHOCK mode so analog joysticks return values
	if (!setMode[i]) { 
		padSetMainMode(i, 0, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK); 
		setMode[i] = true;
	}

	int ret = padRead(i, 0, &pad);
	if (ret == 0) return;
	
	int port = Gamepad_Connect(0x503 + i, defaults_ps2);
	ProcessPadInput(port, delta, &pad);
}

void Gamepads_Process(float delta) {
	ProcessPad(0, delta);
	ProcessPad(1, delta);
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	bmp->width  = width;
	bmp->height = height;
}

extern void Gfx_TransferPixels(void* src, int width, int height, 
								int format, unsigned dst_base, unsigned dst_stride);

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	// FlushCache bios call https://psi-rockin.github.io/ps2tek/
	//   mode=0: Flush data cache (invalidate+writeback dirty contents to memory)
	FlushCache(0);
	
	Gfx_TransferPixels(bmp->scan0, bmp->width, bmp->height, GS_PSM_32, 
						fb_colors[0].address, fb_colors[0].width);
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
void Window_ShowDialog(const char* title, const char* msg) {
	VirtualDialog_Show(title, msg, false);
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

