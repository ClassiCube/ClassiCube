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
#include "../VirtualKeyboard.h"
#include "../VirtualDialog.h"

#include <xsdk/present.h>
#include <xsdk/texture.h>

struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;

void Window_PreInit(void) {
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
}

#define MAX_GPU_BACKBUFFERS 2
#define GPU_SWAP_INTERVAL   1
void Window_Init(void) {
	uint32_t width  = 0;
	uint32_t height = 0;
	uint32_t status = xsdk_present_open(XSDK_DISPLAY_720P, MAX_GPU_BACKBUFFERS, GPU_SWAP_INTERVAL, &width, &height);

	if (!NT_SUCCESS(status)) {
		Platform_Log1("xsdk_present_open failed: %h", &status);
		Process_Abort2(status, "Opening display");
	}

	DisplayInfo.Width  = width;
	DisplayInfo.Height = height;

	Window_Main.Width    = DisplayInfo.Width;
	Window_Main.Height   = DisplayInfo.Height;
	Window_Main.Focused  = true;
	
	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;

	Window_Main.SoftKeyboard   = SOFT_KEYBOARD_VIRTUAL;
	//xenon_ata_init();
	//xenon_atapi_init();
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
	/* TODO implement */
}


/*########################################################################################################################*
*----------------------------------------------------Input processing-----------------------------------------------------*
*#########################################################################################################################*/
void Window_ProcessEvents(float delta) {
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for Xbox

void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }
void Window_UpdateRawMouse(void)  { }


/*########################################################################################################################*
*-------------------------------------------------------Gamepads----------------------------------------------------------*
*#########################################################################################################################*/
static const BindMapping defaults_xbox360[BIND_COUNT] = {
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

void Gamepads_PreInit(void) { }
void Gamepads_Init(void)    { }

static void HandleButtons(int port, INPUT_GAMEPAD* pad) {
	Gamepad_SetButton(port, CCPAD_L,      pad->left_trigger  > 100);
	Gamepad_SetButton(port, CCPAD_R,      pad->right_trigger > 100);
	Gamepad_SetButton(port, CCPAD_LSTICK, pad->buttons & INPUT_GAMEPAD_LEFT_THUMB);
	Gamepad_SetButton(port, CCPAD_RSTICK, pad->buttons & INPUT_GAMEPAD_RIGHT_THUMB);

	// TODO what are these actually ?
	Gamepad_SetButton(port, CCPAD_ZL, pad->buttons & INPUT_GAMEPAD_LEFT_SHOULDER);
	Gamepad_SetButton(port, CCPAD_ZR, pad->buttons & INPUT_GAMEPAD_RIGHT_SHOULDER);
	// TODO guide button?
	
	Gamepad_SetButton(port, CCPAD_1, pad->buttons & INPUT_GAMEPAD_A);
	Gamepad_SetButton(port, CCPAD_2, pad->buttons & INPUT_GAMEPAD_B);
	Gamepad_SetButton(port, CCPAD_3, pad->buttons & INPUT_GAMEPAD_X);
	Gamepad_SetButton(port, CCPAD_4, pad->buttons & INPUT_GAMEPAD_Y);
	
	Gamepad_SetButton(port, CCPAD_START,  pad->buttons & INPUT_GAMEPAD_START);
	Gamepad_SetButton(port, CCPAD_SELECT, pad->buttons & INPUT_GAMEPAD_BACK);
	
	Gamepad_SetButton(port, CCPAD_LEFT,   pad->buttons & INPUT_GAMEPAD_DPAD_LEFT);
	Gamepad_SetButton(port, CCPAD_RIGHT,  pad->buttons & INPUT_GAMEPAD_DPAD_RIGHT);
	Gamepad_SetButton(port, CCPAD_UP,     pad->buttons & INPUT_GAMEPAD_DPAD_UP);
	Gamepad_SetButton(port, CCPAD_DOWN,   pad->buttons & INPUT_GAMEPAD_DPAD_DOWN);
}

#define AXIS_SCALE 8192.0f
static void HandleJoystick(int port, int axis, int x, int y, float delta) {
	if (Math_AbsI(x) <= 4096) x = 0;
	if (Math_AbsI(y) <= 4096) y = 0;	
	
	Gamepad_SetAxis(port, axis, x / AXIS_SCALE, -y / AXIS_SCALE, delta);
}

void Gamepads_Process(float delta) {
	int port = Gamepad_Connect(0xB0360, defaults_xbox360);

	INPUT_STATE input = { 0 };
	uint32_t status   = XamInputGetState(0, 0, &input);
	if (status != ERROR_SUCCESS) {
		Platform_Log1("gamepad failed: %h", &status);
		return;
	}
	INPUT_GAMEPAD* pad = &input.gamepad;
	
	HandleButtons(port, pad);
	HandleJoystick(port, PAD_AXIS_LEFT,  pad->thumb_lx, pad->thumb_ly, delta);
	HandleJoystick(port, PAD_AXIS_RIGHT, pad->thumb_rx, pad->thumb_ry, delta);
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	bmp->width  = width;
	bmp->height = height;
}

#define PRESENT_TIMEOUT_MS 5000
void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	// https://github.com/Free60Project/libxenon/blob/master/libxenon/drivers/console/console.c#L166
	// https://github.com/Free60Project/libxenon/blob/master/libxenon/drivers/console/console.c#L57
	uint32_t address = 0;
	uint32_t status  = xsdk_present_begin(&address, PRESENT_TIMEOUT_MS);
	//Platform_Log3("Presenting %i x %i to %h", &DisplayInfo.Width, &DisplayInfo.Height, &address);
	//return;

	if (!NT_SUCCESS(status)) {
		Platform_Log1("xsdk_present_begin failed: %h", &status);
		return;
	}

	XsdkTexture frame;
	xsdk_texture_init(&frame, XSDK_FORMAT_A8R8G8B8, DisplayInfo.Width, DisplayInfo.Height, 1, true);
	xsdk_texture_attach(&frame, xsdk_present_pixels(address));

	xsdk_texture_upload_rows(&frame, 0, r.y, r.height, Bitmap_GetRow(bmp, r.y), bmp->width * 4);

	xsdk_present_end();
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
