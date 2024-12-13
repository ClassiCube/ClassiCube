#include "Core.h"
#if defined CC_BUILD_PSVITA
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
#include <vitasdk.h>

static cc_bool launcherMode;
static SceTouchPanelInfo frontPanel, backPanel;

struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;

#define DISPLAY_WIDTH   960
#define DISPLAY_HEIGHT  544
#define DISPLAY_STRIDE 1024

extern void Gfx_InitGXM(void);
extern void Gfx_AllocFramebuffers(void);
extern void Gfx_NextFramebuffer(void);
extern void Gfx_UpdateCommonDialogBuffers(void);
extern void (*DQ_OnNextFrame)(void* fb);
static void DQ_OnNextFrame2D(void* fb);

void Window_PreInit(void) {
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK,  SCE_TOUCH_SAMPLING_STATE_START);
}

void Window_Init(void) {
	DisplayInfo.Width  = DISPLAY_WIDTH;
	DisplayInfo.Height = DISPLAY_HEIGHT;
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	Window_Main.Width    = DISPLAY_WIDTH;
	Window_Main.Height   = DISPLAY_HEIGHT;
	Window_Main.Focused  = true;
	
	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;

	Window_Main.SoftKeyboard = SOFT_KEYBOARD_VIRTUAL;
	Input_SetTouchMode(true);
	
	sceTouchGetPanelInfo(SCE_TOUCH_PORT_FRONT, &frontPanel);
	sceTouchGetPanelInfo(SCE_TOUCH_PORT_BACK,  &backPanel);
	Gfx_InitGXM();
	Gfx_AllocFramebuffers();
}

void Window_Free(void) { }

void Window_Create2D(int width, int height) { 
	launcherMode   = true;  
	DQ_OnNextFrame = DQ_OnNextFrame2D;
}

void Window_Create3D(int width, int height) { 
	launcherMode = false; 
}

void Window_Destroy(void) { }

void Window_SetTitle(const cc_string* title) { }
void Clipboard_GetText(cc_string* value) { } // TODO sceClipboardGetText
void Clipboard_SetText(const cc_string* value) { } // TODO sceClipboardSetText

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
static void AdjustTouchPress(const SceTouchPanelInfo* panel, int* x, int* y) {
	if (!panel->maxDispX || !panel->maxDispY) return;
	// TODO: Shouldn't ever happen? need to check
	
	// rescale from touch range to screen range
	*x = (*x - panel->minDispX) * DISPLAY_WIDTH  / panel->maxDispX;
	*y = (*y - panel->minDispY) * DISPLAY_HEIGHT / panel->maxDispY;
}

static int touch_pressed;
static void ProcessTouchInput(int port, int id, const SceTouchPanelInfo* panel) {
	SceTouchData touch;
	
	// sceTouchRead is blocking (seems to block until vblank), and don't want that
	int res = sceTouchPeek(port, &touch, 1);
	if (res == 0) return; // no data available yet
	if (res < 0)  return; // error occurred
	int idx = 1 << id;
	
	cc_bool isPressed = touch.reportNum > 0;
	if (isPressed) {
		int x = touch.report[0].x;
		int y = touch.report[0].y;
		AdjustTouchPress(panel, &x, &y);

		Input_AddTouch(id, x, y);
		touch_pressed |= idx;
	} else if (touch_pressed & idx) {
		// touch.report[0].xy will be 0 when touch.reportNum is 0
		Input_RemoveTouch(id, Pointers[id].x, Pointers[id].y);
		touch_pressed &= ~idx;
	}
}

void Window_ProcessEvents(float delta) {
	ProcessTouchInput(SCE_TOUCH_PORT_FRONT, 0, &frontPanel);
	ProcessTouchInput(SCE_TOUCH_PORT_BACK,  1, &backPanel);
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for PS Vita

void Window_EnableRawMouse(void)  { Input.RawMode = true; }
void Window_UpdateRawMouse(void)  {  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }


/*########################################################################################################################*
*-------------------------------------------------------Gamepads----------------------------------------------------------*
*#########################################################################################################################*/
static cc_bool circle_main;

void Gamepads_Init(void) {
	Input.Sources |= INPUT_SOURCE_GAMEPAD;

	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

	SceAppUtilInitParam init_param = { 0 };
	SceAppUtilBootParam boot_param = { 0 };
	sceAppUtilInit(&init_param, &boot_param);

	int ret = 0;
 	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &ret);
	circle_main = ret == 0;
	
	Input_DisplayNames[CCPAD_1] = circle_main ? "CIRCLE" : "CROSS";
	Input_DisplayNames[CCPAD_2] = circle_main ? "CROSS" : "CIRCLE";
	Input_DisplayNames[CCPAD_3] = "SQUARE";
	Input_DisplayNames[CCPAD_4] = "TRIANGLE";
}

static void HandleButtons(int port, int mods) {
	Gamepad_SetButton(port, CCPAD_1, mods & (circle_main ? SCE_CTRL_CIRCLE : SCE_CTRL_CROSS));
	Gamepad_SetButton(port, CCPAD_2, mods & (circle_main ? SCE_CTRL_CROSS  : SCE_CTRL_CIRCLE));
	Gamepad_SetButton(port, CCPAD_3, mods & SCE_CTRL_SQUARE);
	Gamepad_SetButton(port, CCPAD_4, mods & SCE_CTRL_TRIANGLE);
      
	Gamepad_SetButton(port, CCPAD_START,  mods & SCE_CTRL_START);
	Gamepad_SetButton(port, CCPAD_SELECT, mods & SCE_CTRL_SELECT);

	Gamepad_SetButton(port, CCPAD_LEFT,   mods & SCE_CTRL_LEFT);
	Gamepad_SetButton(port, CCPAD_RIGHT,  mods & SCE_CTRL_RIGHT);
	Gamepad_SetButton(port, CCPAD_UP,     mods & SCE_CTRL_UP);
	Gamepad_SetButton(port, CCPAD_DOWN,   mods & SCE_CTRL_DOWN);
	
	Gamepad_SetButton(port, CCPAD_L, mods & SCE_CTRL_LTRIGGER);
	Gamepad_SetButton(port, CCPAD_R, mods & SCE_CTRL_RTRIGGER);
}

#define AXIS_SCALE 16.0f
static void ProcessCircleInput(int port, int axis, int x, int y, float delta) {
	// May not be exactly 0 on actual hardware
	if (Math_AbsI(x) <= 32) x = 0;
	if (Math_AbsI(y) <= 32) y = 0;
	
	Gamepad_SetAxis(port, axis, x / AXIS_SCALE, y / AXIS_SCALE, delta);
}

static void ProcessPadInput(float delta) {
	int port = Gamepad_Connect(0x503, PadBind_Defaults);
	SceCtrlData pad;
	
	// sceCtrlReadBufferPositive is blocking (seems to block until vblank), and don't want that
	int res = sceCtrlPeekBufferPositive(0, &pad, 1);
	if (res == 0) return; // no data available yet
	if (res < 0)  return; // error occurred
	// TODO: need to use cached version still? like GameCube/Wii
	
	HandleButtons(port, pad.buttons);
	ProcessCircleInput(port, PAD_AXIS_LEFT,  pad.lx - 127, pad.ly - 127, delta);
	ProcessCircleInput(port, PAD_AXIS_RIGHT, pad.rx - 127, pad.ry - 127, delta);
}

void Gamepads_Process(float delta) {
	ProcessPadInput(delta);
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
static struct Bitmap fb_bmp;
void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	bmp->width  = width;
	bmp->height = height;
	fb_bmp      = *bmp;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	sceDisplayWaitVblankStart();
	Gfx_NextFramebuffer();
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}

static void DQ_OnNextFrame2D(void* fb) {
	cc_uint32* src = (cc_uint32*)fb_bmp.scan0;
	cc_uint32* dst = (cc_uint32*)fb;
	
	for (int y = 0; y < DISPLAY_HEIGHT; y++) 
	{
		Mem_Copy(dst + y * DISPLAY_STRIDE, src + y * DISPLAY_WIDTH, DISPLAY_WIDTH * 4);
	}
}
/*########################################################################################################################*
*-------------------------------------------------------Misc/Other--------------------------------------------------------*
*#########################################################################################################################*/
static void DQ_DialogCallback(void* fb) {
	// TODO: Only clear framebuffers once at start
	// NOTE: This also doesn't work properly on real hardware
	//Mem_Set(fb, 128, 4 * DISPLAY_STRIDE * DISPLAY_HEIGHT);
}

static void DisplayDialog(const char* msg) {
	SceMsgDialogParam param = { 0 };
	SceMsgDialogUserMessageParam msgParam = { 0 };

	sceMsgDialogParamInit(&param);
	param.mode          = SCE_MSG_DIALOG_MODE_USER_MSG;
	param.userMsgParam  = &msgParam;
	
	msgParam.msg        = msg;
	msgParam.buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_OK;

	int ret = sceMsgDialogInit(&param);
	if (ret) { Platform_Log1("ERROR SHOWING DIALOG: %e", &ret); return; }
	
	void (*prevCallback)(void* fb);	
	prevCallback   = DQ_OnNextFrame;
	DQ_OnNextFrame = DQ_DialogCallback;
    
	while (sceMsgDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_RUNNING)
	{
		Gfx_UpdateCommonDialogBuffers();
		Gfx_NextFramebuffer();
		sceDisplayWaitVblankStart();
	}
	
	sceMsgDialogTerm();
	DQ_OnNextFrame = prevCallback;
}

void Window_ShowDialog(const char* title, const char* msg) {
	/* TODO implement */
	Platform_LogConst(title);
	Platform_LogConst(msg);
	DisplayDialog(msg);
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) {
	kb_tileWidth = KB_TILE_SIZE * 2;

	VirtualKeyboard_Open(args, launcherMode);
}

void OnscreenKeyboard_SetText(const cc_string* text) {
	VirtualKeyboard_SetText(text);
}

void OnscreenKeyboard_Close(void) {
	VirtualKeyboard_Close();
}
#endif
