#include "Core.h"
#if defined CC_BUILD_XBOX
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
#include "VirtualKeyboard.h"
#include <hal/video.h>
#include <usbh_lib.h>
#include <xid_driver.h>

static cc_bool launcherMode;
struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;

void Window_PreInit(void) {
	XVideoSetMode(640, 480, 32, REFRESH_DEFAULT); // TODO not call
}

void Window_Init(void) {
	VIDEO_MODE mode = XVideoGetMode();
	
	DisplayInfo.Width  = mode.width;
	DisplayInfo.Height = mode.height;
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	Window_Main.Width    = mode.width;
	Window_Main.Height   = mode.height;
	Window_Main.Focused  = true;
	
	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;

	DisplayInfo.ContentOffsetX = 10;
	DisplayInfo.ContentOffsetY = 10;
	Window_Main.SoftKeyboard   = SOFT_KEYBOARD_VIRTUAL;
}

void Window_Free(void) { usbh_core_deinit(); }

void Window_Create2D(int width, int height) { launcherMode = true;  }
void Window_Create3D(int width, int height) { launcherMode = false; }

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

void Cursor_SetPosition(int x, int y) { } // Makes no sense for Xbox

void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }
void Window_UpdateRawMouse(void)  { }


/*########################################################################################################################*
*-------------------------------------------------------Gamepads----------------------------------------------------------*
*#########################################################################################################################*/
static xid_dev_t* xid_ctrl;
static xid_gamepad_in gp_state;

// TODO No idea if this even works
static void OnDataReceived(UTR_T* utr) {
	xid_dev_t* xid_dev = (xid_dev_t*)utr->context;

	if (utr->status < 0 || !xid_ctrl || xid_dev != xid_ctrl) return;

	int len = min(utr->xfer_len, sizeof(gp_state));
	Mem_Copy(&gp_state, utr->buff, len);
	int mods = gp_state.dButtons;

	// queue USB transfer again
	// TODO don't call
	utr->xfer_len        = 0;
	utr->bIsTransferDone = 0;
	usbh_int_xfer(utr);
}

static void OnDeviceChanged(xid_dev_t *xid_dev__, int status__) {
    Platform_LogConst("Getting devices");
    xid_dev_t* xid_dev = usbh_xid_get_device_list();
    Platform_LogConst("Devices check");
	
    for (; xid_dev; xid_dev = xid_dev->next)
    {
    	int DEV = xid_dev->xid_desc.bType;
    	Platform_Log1("DEVICE: %i", &DEV);
        if (xid_dev->xid_desc.bType != XID_TYPE_GAMECONTROLLER)
        	continue;
        	
        xid_ctrl = xid_dev;
        usbh_xid_read(xid_dev, 0, OnDataReceived);
        return;
    }
    xid_ctrl = NULL;
}

void Gamepads_Init(void) {
	Input.Sources |= INPUT_SOURCE_GAMEPAD;

#ifndef CC_BUILD_CXBX
	usbh_core_init();
	usbh_xid_init();
	
	usbh_install_xid_conn_callback(OnDeviceChanged, OnDeviceChanged);
	OnDeviceChanged(NULL, 0); // TODO useless call?
#endif
}

// https://docs.microsoft.com/en-us/windows/win32/api/xinput/ns-xinput-xinput_gamepad
// NOTE: Analog buttons use dedicated field rather than being part of dButtons
#define XINPUT_GAMEPAD_DPAD_UP     0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN   0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT   0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT  0x0008
#define XINPUT_GAMEPAD_START       0x0010
#define XINPUT_GAMEPAD_BACK        0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB  0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB 0x0080

static void HandleButtons(int port, xid_gamepad_in* gp) {
	int mods = gp->dButtons;
	Gamepad_SetButton(port, CCPAD_L,  gp->l     > 0x7F);
	Gamepad_SetButton(port, CCPAD_R,  gp->r     > 0x7F);
	Gamepad_SetButton(port, CCPAD_ZL, gp->white > 0x7F);
	Gamepad_SetButton(port, CCPAD_ZR, gp->black > 0x7F);
	
	Gamepad_SetButton(port, CCPAD_1, gp->a > 0x7F);
	Gamepad_SetButton(port, CCPAD_2, gp->b > 0x7F);
	Gamepad_SetButton(port, CCPAD_3, gp->x > 0x7F);
	Gamepad_SetButton(port, CCPAD_4, gp->y > 0x7F);
	
	Gamepad_SetButton(port, CCPAD_START,  mods & XINPUT_GAMEPAD_START);
	Gamepad_SetButton(port, CCPAD_SELECT, mods & XINPUT_GAMEPAD_BACK);
	Gamepad_SetButton(port, CCPAD_LSTICK, mods & XINPUT_GAMEPAD_LEFT_THUMB);
	Gamepad_SetButton(port, CCPAD_RSTICK, mods & XINPUT_GAMEPAD_RIGHT_THUMB);
	
	Gamepad_SetButton(port, CCPAD_LEFT,   mods & XINPUT_GAMEPAD_DPAD_LEFT);
	Gamepad_SetButton(port, CCPAD_RIGHT,  mods & XINPUT_GAMEPAD_DPAD_RIGHT);
	Gamepad_SetButton(port, CCPAD_UP,     mods & XINPUT_GAMEPAD_DPAD_UP);
	Gamepad_SetButton(port, CCPAD_DOWN,   mods & XINPUT_GAMEPAD_DPAD_DOWN);
}

#define AXIS_SCALE 8192.0f
static void HandleJoystick(int port, int axis, int x, int y, float delta) {
	if (Math_AbsI(x) <= 512) x = 0;
	if (Math_AbsI(y) <= 512) y = 0;	
	
	Gamepad_SetAxis(port, axis, x / AXIS_SCALE, -y / AXIS_SCALE, delta);
}

void Gamepads_Process(float delta) {
#ifndef CC_BUILD_CXBX
	usbh_pooling_hubs();
#endif
	if (!xid_ctrl) return;
	int port = Gamepad_Connect(0xB0, PadBind_Defaults);
	
	HandleButtons(port, &gp_state);
	HandleJoystick(port, PAD_AXIS_LEFT,  gp_state.leftStickX,  gp_state.leftStickY,  delta);
	HandleJoystick(port, PAD_AXIS_RIGHT, gp_state.rightStickX, gp_state.rightStickY, delta);
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
	void* fb = XVideoGetFB();
	//XVideoWaitForVBlank();
	// XVideoWaitForVBlank installs an interrupt handler for VBlank - 
	//  however this will cause pbkit's attempt to install an interrupt
	//  handler fail - so instead just accept tearing in the launcher

	cc_uint32* src = (cc_uint32*)bmp->scan0 + r.x;
	cc_uint32* dst = (cc_uint32*)fb         + r.x;

	for (int y = r.y; y < r.y + r.height; y++) 
	{
		Mem_Copy(dst + y * bmp->width, src + y * bmp->width, r.width * 4);
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
	VirtualKeyboard_Open(args, launcherMode);
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
