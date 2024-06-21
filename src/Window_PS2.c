#include "Core.h"
#if defined CC_BUILD_PS2
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

static cc_bool launcherMode, mouseSupported, kbdSupported;
static char padBuf0[256] __attribute__((aligned(64)));
static char padBuf1[256] __attribute__((aligned(64)));

struct _DisplayData DisplayInfo;
struct _WindowData WindowInfo;
framebuffer_t fb_colors[2];
zbuffer_t     fb_depth;

void Window_PreInit(void) {
	dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
	dma_channel_fast_waits(DMA_CHANNEL_GIF);
}

void Window_Init(void) {
	DisplayInfo.Width  = 640;
	DisplayInfo.Height = graph_get_region() == GRAPH_MODE_PAL ? 512 : 448;
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	Window_Main.Width    = DisplayInfo.Width;
	Window_Main.Height   = DisplayInfo.Height;
	Window_Main.Focused  = true;
	
	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = 1.0f / Window_Main.Height;

	Input.Sources = INPUT_SOURCE_GAMEPAD;
	DisplayInfo.ContentOffsetX = 10;
	DisplayInfo.ContentOffsetY = 10;
	Window_Main.SoftKeyboard   = SOFT_KEYBOARD_VIRTUAL;
	
	padInit(0);
	padPortOpen(0, 0, padBuf0);
	padPortOpen(1, 0, padBuf1);

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

static void ResetGfxState(void) {
	graph_shutdown();
	graph_vram_clear();

	fb_colors[0].width   = DisplayInfo.Width;
	fb_colors[0].height  = DisplayInfo.Height;
	fb_colors[0].mask    = 0;
	fb_colors[0].psm     = GS_PSM_32;
	fb_colors[0].address = graph_vram_allocate(fb_colors[0].width, fb_colors[0].height, fb_colors[0].psm, GRAPH_ALIGN_PAGE);

	fb_colors[1].width   = DisplayInfo.Width;
	fb_colors[1].height  = DisplayInfo.Height;
	fb_colors[1].mask    = 0;
	fb_colors[1].psm     = GS_PSM_32;
	fb_colors[1].address = graph_vram_allocate(fb_colors[1].width, fb_colors[1].height, fb_colors[1].psm, GRAPH_ALIGN_PAGE);

	fb_depth.enable  = 1;
	fb_depth.method  = ZTEST_METHOD_ALLPASS;
	fb_depth.mask    = 0;
	fb_depth.zsm     = GS_ZBUF_32;
	fb_depth.address = graph_vram_allocate(fb_colors[0].width, fb_colors[0].height, fb_depth.zsm, GRAPH_ALIGN_PAGE);

	graph_initialize(fb_colors[1].address, fb_colors[1].width, fb_colors[1].height, fb_colors[1].psm, 0, 0);
}

void Window_Create2D(int width, int height) {
	ResetGfxState();
	launcherMode = true;
}

void Window_Create3D(int width, int height) { 
	ResetGfxState();
	launcherMode = false; 
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
static void ProcessMouseInput(float delta) {
	if (!mouseSupported)     return;
	if (PS2MouseEnum() == 0) return;

	mouse_data mData = { 0 };
	if (PS2MouseRead(&mData) < 0) return;

	Input_SetNonRepeatable(CCMOUSE_L, mData.buttons & PS2MOUSE_BTN1);
	Input_SetNonRepeatable(CCMOUSE_R, mData.buttons & PS2MOUSE_BTN2);
	Input_SetNonRepeatable(CCMOUSE_M, mData.buttons & PS2MOUSE_BTN3);
	Mouse_ScrollVWheel(mData.wheel);
	
	if (!Input.RawMode) return;	
	float scale = (delta * 60.0) / 2.0f;
	Event_RaiseRawMove(&PointerEvents.RawMoved, 
				mData.x * scale, mData.y * scale);
}

static void ProcessKeyboardInput(void) {
	if (!kbdSupported) return;

	PS2KbdRawKey key;
	if (PS2KbdReadRaw(&key) <= 0) return;

	Platform_Log1("%i", &key.key);
}

void Window_ProcessEvents(float delta) {
	ProcessMouseInput(delta);
	ProcessKeyboardInput();
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
	buttons = buttons ^ 0xFFFF;
	//Platform_Log1("BUTTONS: %h", &buttons);
	
	Gamepad_SetButton(port, CCPAD_A, buttons & PAD_TRIANGLE);
	Gamepad_SetButton(port, CCPAD_B, buttons & PAD_SQUARE);
	Gamepad_SetButton(port, CCPAD_X, buttons & PAD_CROSS);
	Gamepad_SetButton(port, CCPAD_Y, buttons & PAD_CIRCLE);
      
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

static cc_bool setMode[INPUT_MAX_GAMEPADS];
static void ProcessPad(int port, float delta) {
	 struct padButtonStatus pad;
	int state = padGetState(port, 0);
	if (state != PAD_STATE_STABLE) return;

	// Change to DUALSHOCK mode so analog joysticks return values
	if (!setMode[port]) { 
		padSetMainMode(port, 0, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK); 
		setMode[port] = true;
	}

	int ret = padRead(port, 0, &pad);
	if (ret != 0) ProcessPadInput(port, delta, &pad);
}

void Window_ProcessGamepads(float delta) {
	ProcessPad(0, delta);
	ProcessPad(1, delta);
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, 4, "window pixels");
	bmp->width  = width;
	bmp->height = height;

	packet_t* packet = packet_init(100, PACKET_NORMAL);
	qword_t* q = packet->data;

	q = draw_setup_environment(q, 0, &fb_colors[1], &fb_depth);
	q = draw_clear(q, 0, 0, 0,
					fb_colors[1].width, fb_colors[1].height, 170, 170, 170);
	q = draw_finish(q);

	dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
	dma_wait_fast();
	packet_free(packet);
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	// FlushCache bios call https://psi-rockin.github.io/ps2tek/
	//   mode=0: Flush data cache (invalidate+writeback dirty contents to memory)
	FlushCache(0);
	
	packet_t* packet = packet_init(200, PACKET_NORMAL);
	qword_t* q = packet->data;

	q = draw_texture_transfer(q, bmp->scan0, bmp->width, bmp->height, GS_PSM_32, 
								 fb_colors[1].address, fb_colors[1].width);
	q = draw_texture_flush(q);

	dma_channel_send_chain(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
	dma_wait_fast();
	packet_free(packet);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) {
	if (Input.Sources & INPUT_SOURCE_NORMAL) return;
	VirtualKeyboard_Open(args, launcherMode);
}

void OnscreenKeyboard_SetText(const cc_string* text) {
	VirtualKeyboard_SetText(text);
}

void OnscreenKeyboard_Draw2D(Rect2D* r, struct Bitmap* bmp) {
	VirtualKeyboard_Display2D(r, bmp);
}

void OnscreenKeyboard_Draw3D(void) {
	VirtualKeyboard_Display3D();
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
