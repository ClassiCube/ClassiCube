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
#include <hal/video.h>
#include <usbh_lib.h>
#include <xid_driver.h>

static cc_bool launcherMode;
static xid_dev_t* xid_ctrl;
static xid_gamepad_in gp_state;

struct _DisplayData DisplayInfo;
struct _WinData WindowInfo;
// no DPI scaling on Xbox
int Display_ScaleX(int x) { return x; }
int Display_ScaleY(int y) { return y; }

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
    xid_dev_t* xid_dev = usbh_xid_get_device_list();
    Platform_LogConst("DEVICE CHECK!!!");
	
    while (xid_dev)
    {
    	int DEV = xid_dev->xid_desc.bType;
    	Platform_Log1("DEV: %i", &DEV);
        if (xid_dev->xid_desc.bType != XID_TYPE_GAMECONTROLLER)
        	continue;
        	
        xid_ctrl = xid_dev;
        usbh_xid_read(xid_ctrl, 0, OnDataReceived);
        return;
    }
    xid_ctrl = NULL;
}

void Window_Init(void) {
	XVideoSetMode(640, 480, 32, REFRESH_DEFAULT); // TODO not call
	VIDEO_MODE mode = XVideoGetMode();
	
	DisplayInfo.Width  = mode.width;
	DisplayInfo.Height = mode.height;
	DisplayInfo.Depth  = 4; // 32 bit
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	WindowInfo.Width   = mode.width;
	WindowInfo.Height  = mode.height;
	WindowInfo.Focused = true;
	WindowInfo.Exists  = true;

	Input.Sources = INPUT_SOURCE_GAMEPAD;
	DisplayInfo.ContentOffsetX = 10;
	DisplayInfo.ContentOffsetY = 10;

	usbh_core_init();
	usbh_xid_init();
	
	usbh_install_xid_conn_callback(OnDeviceChanged, OnDeviceChanged);
	OnDeviceChanged(NULL, 0); // TODO useless call?
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

static void HandleButtons(xid_gamepad_in* gp) {
	int mods = gp->dButtons;
	Input_SetNonRepeatable(CCPAD_L,  gp->l     > 0x7F);
	Input_SetNonRepeatable(CCPAD_R,  gp->r     > 0x7F);
	Input_SetNonRepeatable(CCPAD_ZL, gp->white > 0x7F);
	Input_SetNonRepeatable(CCPAD_ZR, gp->black > 0x7F);
	
	Input_SetNonRepeatable(CCPAD_A, gp->a > 0x7F);
	Input_SetNonRepeatable(CCPAD_B, gp->b > 0x7F);
	Input_SetNonRepeatable(CCPAD_X, gp->x > 0x7F);
	Input_SetNonRepeatable(CCPAD_Y, gp->y > 0x7F);
	
	Input_SetNonRepeatable(CCPAD_START,  mods & XINPUT_GAMEPAD_START);
	Input_SetNonRepeatable(CCPAD_SELECT, mods & XINPUT_GAMEPAD_BACK);
	Input_SetNonRepeatable(CCPAD_LSTICK, mods & XINPUT_GAMEPAD_LEFT_THUMB);
	Input_SetNonRepeatable(CCPAD_RSTICK, mods & XINPUT_GAMEPAD_RIGHT_THUMB);
	
	Input_SetNonRepeatable(CCPAD_LEFT,   mods & XINPUT_GAMEPAD_DPAD_LEFT);
	Input_SetNonRepeatable(CCPAD_RIGHT,  mods & XINPUT_GAMEPAD_DPAD_RIGHT);
	Input_SetNonRepeatable(CCPAD_UP,     mods & XINPUT_GAMEPAD_DPAD_UP);
	Input_SetNonRepeatable(CCPAD_DOWN,   mods & XINPUT_GAMEPAD_DPAD_DOWN);
}

static void HandleJoystick_Left(int x, int y) {
	if (Math_AbsI(x) <= 256) x = 0;
	if (Math_AbsI(y) <= 256) y = 0;	
	
	if (x == 0 && y == 0) return;
	Input.JoystickMovement = true;
	Input.JoystickAngle    = Math_Atan2(x, -y);
}

static void HandleJoystick_Right(int x, int y, double delta) {
	float scale = (delta * 60.0) / 8192.0f;
	
	if (Math_AbsI(x) <= 256) x = 0;
	if (Math_AbsI(y) <= 256) y = 0;
	
	Event_RaiseRawMove(&PointerEvents.RawMoved, x * scale, -y * scale);	
}

void Window_ProcessEvents(double delta) {
	Input.JoystickMovement = false;
	usbh_pooling_hubs();
	if (!xid_ctrl) return;
	
	HandleButtons(&gp_state);
	HandleJoystick_Left( gp_state.leftStickX,  gp_state.leftStickY );
	HandleJoystick_Right(gp_state.rightStickX, gp_state.rightStickY, delta);
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for Xbox

void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }
void Window_UpdateRawMouse(void)  { }


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
static struct Bitmap fb_bmp;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	fb_bmp = *bmp;
}

void Window_DrawFramebuffer(Rect2D r) {
	void* fb = XVideoGetFB();
	//XVideoWaitForVBlank();
	// XVideoWaitForVBlank installs an interrupt handler for VBlank - 
	//  however this will cause pbkit's attempt to install an interrupt
	//  handler fail - so instead just accept tearing in the launcher

	cc_uint32* src = (cc_uint32*)fb_bmp.scan0 + r.x;
	cc_uint32* dst = (cc_uint32*)fb           + r.x;

	for (int y = r.y; y < r.y + r.Height; y++) 
	{
		Mem_Copy(dst + y * fb_bmp.width, src + y * fb_bmp.width, r.Width * 4);
	}
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
void Window_OpenKeyboard(struct OpenKeyboardArgs* args) { }
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
