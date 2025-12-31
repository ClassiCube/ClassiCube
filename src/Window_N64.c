#include "Core.h"
#if defined CC_BUILD_N64
#include "Window.h"
#include "Platform.h"
#include "Input.h"
#include "InputHandler.h"
#include "Event.h"
#include "Graphics.h"
#include "String_.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Errors.h"
#include "ExtMath.h"
#include "VirtualKeyboard.h"
#include "Options.h"
#include <libdragon.h>

#include "VirtualCursor.h"

struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;

void Window_PreInit(void) {
	int buffers = is_memory_expanded() ? 3 : 2;
	display_init(RESOLUTION_320x240, DEPTH_32_BPP, buffers, GAMMA_NONE, FILTERS_DISABLED);
	//display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);    
}

void Window_Init(void) {
	DisplayInfo.Width  = display_get_width();
	DisplayInfo.Height = display_get_height();
	DisplayInfo.ScaleX = 0.5f;
	DisplayInfo.ScaleY = 0.5f;
	
	Window_Main.Width    = DisplayInfo.Width;
	Window_Main.Height   = DisplayInfo.Height;
	Window_Main.Focused  = true;
	
	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;

	DisplayInfo.ContentOffsetX = Option_GetOffsetX(10);
	DisplayInfo.ContentOffsetY = Option_GetOffsetY(10);
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

static void ProcessMouse(joypad_inputs_t* inputs, float delta) {
	Input_SetNonRepeatable(CCMOUSE_L, inputs->btn.a);
	Input_SetNonRepeatable(CCMOUSE_R, inputs->btn.b);

	// TODO check stick_x/y is right
	if (!vc_hooked) {
		Pointer_SetPosition(0, Window_Main.Width / 2, Window_Main.Height / 2);
	}
	VirtualCursor_SetPosition(Pointers[0].x + inputs->stick_x, Pointers[0].y + inputs->stick_y);
	
	if (!Input.RawMode) return;	
	float scale = (delta * 60.0) / 2.0f;
	Event_RaiseRawMove(&PointerEvents.RawMoved, 
				inputs->stick_x * scale, inputs->stick_y * scale);
}


/*########################################################################################################################*
*-------------------------------------------------------Gamepads----------------------------------------------------------*
*#########################################################################################################################*/
static const BindMapping defaults_n64[BIND_COUNT] = {
	[BIND_FORWARD] = { CCPAD_CUP    },
	[BIND_BACK]    = { CCPAD_CDOWN  },
	[BIND_LEFT]    = { CCPAD_CLEFT  },
	[BIND_RIGHT]   = { CCPAD_CRIGHT },
	
	[BIND_FLY_UP]  = { CCPAD_UP    },
	[BIND_FLY_DOWN]= { CCPAD_DOWN  },
	[BIND_SPEED]   = { CCPAD_LEFT  },
	[BIND_FLY]     = { CCPAD_RIGHT },
	
	[BIND_JUMP]         = { CCPAD_1 },
	[BIND_INVENTORY]    = { CCPAD_2 },
	[BIND_PLACE_BLOCK]  = { CCPAD_5 },
	[BIND_HOTBAR_RIGHT] = { CCPAD_L },
	[BIND_DELETE_BLOCK] = { CCPAD_R },
	
	[BIND_SET_SPAWN]    = { CCPAD_START },
};

void Gamepads_PreInit(void) {
	joypad_init();
}

void Gamepads_Init(void) { }

static void HandleButtons(int port, joypad_buttons_t btns) {
	Gamepad_SetButton(port, CCPAD_L, btns.l);
	Gamepad_SetButton(port, CCPAD_R, btns.r);
	
	Gamepad_SetButton(port, CCPAD_1, btns.a);
	Gamepad_SetButton(port, CCPAD_2, btns.b);
	Gamepad_SetButton(port, CCPAD_5, btns.z);
	
	Gamepad_SetButton(port, CCPAD_START,  btns.start);
	
	Gamepad_SetButton(port, CCPAD_LEFT,   btns.d_left);
	Gamepad_SetButton(port, CCPAD_RIGHT,  btns.d_right);
	Gamepad_SetButton(port, CCPAD_UP,     btns.d_up);
	Gamepad_SetButton(port, CCPAD_DOWN,   btns.d_down);

	Gamepad_SetButton(port, CCPAD_CLEFT,  btns.c_left);
	Gamepad_SetButton(port, CCPAD_CRIGHT, btns.c_right);
	Gamepad_SetButton(port, CCPAD_CUP,    btns.c_up);
	Gamepad_SetButton(port, CCPAD_CDOWN,  btns.c_down);
}

#define AXIS_SCALE 8.0f
static void ProcessAnalogInput(int port, joypad_inputs_t* inputs, float delta) {
	int x = inputs->stick_x;
	int y = inputs->stick_y;

	if (Math_AbsI(x) <= 8) x = 0;
	if (Math_AbsI(y) <= 8) y = 0;
	
	Gamepad_SetAxis(port, PAD_AXIS_RIGHT, x / AXIS_SCALE, -y / AXIS_SCALE, delta);
}

void Gamepads_Process(float delta) {
	joypad_poll();

	for (int i = 0; i < JOYPAD_PORT_COUNT; i++)
	{
		if (!joypad_is_connected(i)) continue;
		joypad_inputs_t inputs = joypad_get_inputs(i);
		
		if (joypad_get_style(i) == JOYPAD_STYLE_MOUSE) {
			ProcessMouse(&inputs, delta); continue;
		}

		int port = Gamepad_Connect(0x64 + i, defaults_n64);
		HandleButtons(port, inputs.btn);
		ProcessAnalogInput(port, &inputs, delta);
	}
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	bmp->width  = width;
	bmp->height = height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	surface_t* fb  = display_get();
	cc_uint32* src = (cc_uint32*)bmp->scan0;
	cc_uint8*  dst = (cc_uint8*)fb->buffer;

	for (int y = 0; y < bmp->height; y++) 
	{
		Mem_Copy(dst + y * fb->stride,
				 src + y * bmp->width, 
				 bmp->width * 4);
	}
	
    display_show(fb);
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
