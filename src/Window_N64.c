#include "Core.h"
#if defined CC_BUILD_N64
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
#include <libdragon.h>

static cc_bool launcherMode;

struct _DisplayData DisplayInfo;
struct _WinData WindowInfo;
// no DPI scaling on Wii/GameCube
int Display_ScaleX(int x) { return x; }
int Display_ScaleY(int y) { return y; }

void Window_Init(void) {
    display_init(RESOLUTION_320x240, DEPTH_32_BPP, 2, GAMMA_NONE, FILTERS_DISABLED);
    //display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
    
	DisplayInfo.Width  = display_get_width();
	DisplayInfo.Height = display_get_height();
	DisplayInfo.Depth  = 4; // 32 bit
	DisplayInfo.ScaleX = 0.5f;
	DisplayInfo.ScaleY = 0.5f;
	
	WindowInfo.Width   = DisplayInfo.Width;
	WindowInfo.Height  = DisplayInfo.Height;
	WindowInfo.Focused = true;
	WindowInfo.Exists  = true;

	Input.Sources = INPUT_SOURCE_GAMEPAD;
	DisplayInfo.ContentOffsetX = 10;
	DisplayInfo.ContentOffsetY = 10;
    joypad_init();
}

void Window_Create2D(int width, int height) { launcherMode = true;  }
void Window_Create3D(int width, int height) { launcherMode = false; }

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
static void HandleButtons(joypad_buttons_t btns) {
	Input_SetNonRepeatable(CCPAD_L, btns.l);
	Input_SetNonRepeatable(CCPAD_R, btns.r);
	
	Input_SetNonRepeatable(CCPAD_A, btns.a);
	Input_SetNonRepeatable(CCPAD_B, btns.b);
	Input_SetNonRepeatable(CCPAD_X, btns.z); // TODO: Or Y?
	
	Input_SetNonRepeatable(CCPAD_START,  btns.start);
	
	Input_SetNonRepeatable(CCPAD_LEFT,   btns.d_left);
	Input_SetNonRepeatable(CCPAD_RIGHT,  btns.d_right);
	Input_SetNonRepeatable(CCPAD_UP,     btns.d_up);
	Input_SetNonRepeatable(CCPAD_DOWN,   btns.d_down);
	
	// TODO: How to map the right digital buttons (c_left/c_down etc
}

static void ProcessAnalogInput(joypad_inputs_t* inputs, double delta) {
	float scale = (delta * 60.0) / 32.0f;
	int dx = inputs->stick_x;
	int dy = inputs->stick_y;

	if (Math_AbsI(dx) <= 8) dx = 0;
	if (Math_AbsI(dy) <= 8) dy = 0;

	Event_RaiseRawMove(&PointerEvents.RawMoved, dx * scale, -dy * scale);
}

void Window_ProcessEvents(double delta) {
	joypad_poll();
	
	joypad_inputs_t inputs = joypad_get_inputs(JOYPAD_PORT_1);
	HandleButtons(inputs.btn);
	
	if (Input.RawMode) ProcessAnalogInput(&inputs, delta);
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for PSP
void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }
void Window_UpdateRawMouse(void)  { }


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
static struct Bitmap fb_bmp;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	fb_bmp     = *bmp;
}

void Window_DrawFramebuffer(Rect2D r) {
	surface_t* fb  = display_get();
	cc_uint32* src = (cc_uint32*)fb_bmp.scan0;
	cc_uint8*  dst = (cc_uint8*)fb->buffer;

	for (int y = 0; y < fb_bmp.height; y++) 
	{
		Mem_Copy(dst + y * fb->stride,
				 src + y * fb_bmp.width, 
				 fb_bmp.width * 4);
	}
	
    display_show(fb);
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
