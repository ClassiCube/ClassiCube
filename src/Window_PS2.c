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
struct _WinData WindowInfo;
// no DPI scaling on PS Vita
int Display_ScaleX(int x) { return x; }
int Display_ScaleY(int y) { return y; }

void Window_Init(void) {
	DisplayInfo.Width  = 640;
	DisplayInfo.Height = graph_get_region() == GRAPH_MODE_PAL ? 512 : 448;
	DisplayInfo.Depth  = 4; // 32 bit
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	WindowInfo.Width   = DisplayInfo.Width;
	WindowInfo.Height  = DisplayInfo.Height;
	WindowInfo.Focused = true;
	WindowInfo.Exists  = true;

	Input.Sources = INPUT_SOURCE_GAMEPAD;
	DisplayInfo.ContentOffsetX = 10;
	DisplayInfo.ContentOffsetY = 10;
	
	padInit(0);
	padPortOpen(0, 0, padBuf);
}

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

void Window_Close(void) {
	Event_RaiseVoid(&WindowEvents.Closing);
}


/*########################################################################################################################*
*----------------------------------------------------Input processing-----------------------------------------------------*
*#########################################################################################################################*/
static void HandleButtons(int buttons) {
	// Confusingly, it seems that when a bit is on, it means the button is NOT pressed
	// So just flip the bits to make more sense
	buttons = buttons ^ 0xFFFF;
	//Platform_Log1("BUTTONS: %h", &buttons);
	
	Input_SetNonRepeatable(CCPAD_A, buttons & PAD_TRIANGLE);
	Input_SetNonRepeatable(CCPAD_B, buttons & PAD_SQUARE);
	Input_SetNonRepeatable(CCPAD_X, buttons & PAD_CROSS);
	Input_SetNonRepeatable(CCPAD_Y, buttons & PAD_CIRCLE);
      
	Input_SetNonRepeatable(CCPAD_START,  buttons & PAD_START);
	Input_SetNonRepeatable(CCPAD_SELECT, buttons & PAD_SELECT);

	Input_SetNonRepeatable(CCPAD_LEFT,   buttons & PAD_LEFT);
	Input_SetNonRepeatable(CCPAD_RIGHT,  buttons & PAD_RIGHT);
	Input_SetNonRepeatable(CCPAD_UP,     buttons & PAD_UP);
	Input_SetNonRepeatable(CCPAD_DOWN,   buttons & PAD_DOWN);
	
	Input_SetNonRepeatable(CCPAD_L,  buttons & PAD_L1);
	Input_SetNonRepeatable(CCPAD_R,  buttons & PAD_R1);
	Input_SetNonRepeatable(CCPAD_ZL, buttons & PAD_L2);
	Input_SetNonRepeatable(CCPAD_ZR, buttons & PAD_R2);
}

static void HandleJoystick_Left(int x, int y) {
	//Platform_Log2("LEFT: %i, %i", &x, &y);
	if (Math_AbsI(x) <= 8) x = 0;
	if (Math_AbsI(y) <= 8) y = 0;
	
	if (x == 0 && y == 0) return;
	Input.JoystickMovement = true;
	Input.JoystickAngle    = Math_Atan2(x, -y);
}
static void HandleJoystick_Right(int x, int y, double delta) {
	//Platform_Log2("Right: %i, %i", &x, &y);
	float scale = (delta * 60.0) / 16.0f;
	
	if (Math_AbsI(x) <= 8) x = 0;
	if (Math_AbsI(y) <= 8) y = 0;
	
	Event_RaiseRawMove(&PointerEvents.RawMoved, x * scale, y * scale);	
}

static void ProcessPadInput(double delta, struct padButtonStatus* pad) {
	HandleButtons(pad->btns);
	HandleJoystick_Left( pad->ljoy_h - 0x80, pad->ljoy_v - 0x80);
	HandleJoystick_Right(pad->rjoy_h - 0x80, pad->rjoy_v - 0x80, delta);
}

static cc_bool setMode;
void Window_ProcessEvents(double delta) {
    struct padButtonStatus pad;
	Input.JoystickMovement = false;
	
	int state = padGetState(0, 0);
    if (state != PAD_STATE_STABLE) return;
    
    // Change to DUALSHOCK mode so analog joysticks return values
    if (!setMode) { 
    	padSetMainMode(0, 0, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK); 
    	setMode = true;
    }
    
	int ret = padRead(0, 0, &pad);
	if (ret != 0) ProcessPadInput(delta, &pad);
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for PS Vita

void Window_EnableRawMouse(void)  { Input.RawMode = true; }
void Window_UpdateRawMouse(void)  {  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
static framebuffer_t win_fb;
static struct Bitmap fb_bmp;

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

void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	fb_bmp     = *bmp;
}

void Window_DrawFramebuffer(Rect2D r) {
	// FlushCache bios call https://psi-rockin.github.io/ps2tek/
	//   mode=0: Flush data cache (invalidate+writeback dirty contents to memory)
	FlushCache(0);
	
	packet_t* packet = packet_init(50,PACKET_NORMAL);
	qword_t* q = packet->data;

	q = draw_texture_transfer(q, fb_bmp.scan0, fb_bmp.width, fb_bmp.height, GS_PSM_32, 
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
void Window_OpenKeyboard(struct OpenKeyboardArgs* args) { /* TODO implement */ }
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
