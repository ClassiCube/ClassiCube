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

static cc_bool launcherMode;
static char padBuf[256] __attribute__((aligned(64)));

struct _DisplayData DisplayInfo;
struct _WindowData WindowInfo;

void Window_PreInit(void) { }
void Window_Init(void) {
	DisplayInfo.Width  = 640;
	DisplayInfo.Height = graph_get_region() == GRAPH_MODE_PAL ? 512 : 448;
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	Window_Main.Width   = DisplayInfo.Width;
	Window_Main.Height  = DisplayInfo.Height;
	Window_Main.Focused = true;
	Window_Main.Exists  = true;

	Input.Sources = INPUT_SOURCE_GAMEPAD;
	DisplayInfo.ContentOffsetX = 10;
	DisplayInfo.ContentOffsetY = 10;
	
	padInit(0);
	padPortOpen(0, 0, padBuf);
}

void Window_Free(void) { }

static cc_bool hasCreated;
static void ResetGfxState(void) {
	if (!hasCreated) { hasCreated = true; return; }
	
	graph_shutdown();
	graph_vram_clear();
	
	dma_channel_shutdown(DMA_CHANNEL_GIF,0);
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
	if (Math_AbsI(x) <= 8) x = 0;
	if (Math_AbsI(y) <= 8) y = 0;
	
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
	for (int port = 0; port < 2; port++)
	{
		ProcessPad(port, delta);
	}
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
static framebuffer_t win_fb;

void Window_Create2D(int width, int height) {
	ResetGfxState();
	launcherMode = true;
	
	win_fb.width   = DisplayInfo.Width;
	win_fb.height  = DisplayInfo.Height;
	win_fb.mask    = 0;
	win_fb.psm     = GS_PSM_32;
	win_fb.address = graph_vram_allocate(win_fb.width, win_fb.height, win_fb.psm, GRAPH_ALIGN_PAGE);
	
	graph_initialize(win_fb.address, win_fb.width, win_fb.height, win_fb.psm, 0, 0);
}

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, 4, "window pixels");
	bmp->width  = width;
	bmp->height = height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	// FlushCache bios call https://psi-rockin.github.io/ps2tek/
	//   mode=0: Flush data cache (invalidate+writeback dirty contents to memory)
	FlushCache(0);
	
	packet_t* packet = packet_init(50, PACKET_NORMAL);
	qword_t* q = packet->data;

	q = draw_texture_transfer(q, bmp->scan0, bmp->width, bmp->height, GS_PSM_32, 
								 win_fb.address, win_fb.width);
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
