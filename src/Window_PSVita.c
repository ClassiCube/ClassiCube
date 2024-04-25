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
#include <vitasdk.h>

static cc_bool launcherMode;
static SceTouchPanelInfo frontPanel;

struct _DisplayData DisplayInfo;
struct _WindowData WindowInfo;

#define DISPLAY_WIDTH   960
#define DISPLAY_HEIGHT  544
#define DISPLAY_STRIDE 1024

extern void Gfx_InitGXM(void);
extern void Gfx_AllocFramebuffers(void);
extern void Gfx_NextFramebuffer(void);
extern void Gfx_UpdateCommonDialogBuffers(void);
extern void (*DQ_OnNextFrame)(void* fb);
static void DQ_OnNextFrame2D(void* fb);

void Window_Init(void) {
	DisplayInfo.Width  = DISPLAY_WIDTH;
	DisplayInfo.Height = DISPLAY_HEIGHT;
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	Window_Main.Width   = DISPLAY_WIDTH;
	Window_Main.Height  = DISPLAY_HEIGHT;
	Window_Main.Focused = true;
	Window_Main.Exists  = true;

	Window_Main.SoftKeyboard = SOFT_KEYBOARD_RESIZE;
	Input_SetTouchMode(true);
	Input.Sources = INPUT_SOURCE_GAMEPAD;

	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK,  SCE_TOUCH_SAMPLING_STATE_START);
	
	sceTouchGetPanelInfo(SCE_TOUCH_PORT_FRONT, &frontPanel);
	
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
static void AdjustTouchPress(int* x, int* y) {
	if (!frontPanel.maxDispX || !frontPanel.maxDispY) return;
	// TODO: Shouldn't ever happen? need to check
	
	// rescale from touch range to screen range
	*x = (*x - frontPanel.minDispX) * DISPLAY_WIDTH  / frontPanel.maxDispX;
	*y = (*y - frontPanel.minDispY) * DISPLAY_HEIGHT / frontPanel.maxDispY;
}

static cc_bool touch_pressed;
static void ProcessTouchInput(void) {
	SceTouchData touch;
	
	// sceTouchRead is blocking (seems to block until vblank), and don't want that
	int res = sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
	if (res == 0) return; // no data available yet
	if (res < 0)  return; // error occurred
	
	cc_bool isPressed = touch.reportNum > 0;
	if (isPressed) {
		int x = touch.report[0].x;
		int y = touch.report[0].y;
		AdjustTouchPress(&x, &y);

		Input_AddTouch(0, x, y);
		touch_pressed = true;
	} else if (touch_pressed) {
		// touch.report[0].xy will be 0 when touch.reportNum is 0
		Input_RemoveTouch(0, Pointers[0].x, Pointers[0].y);
		touch_pressed = false;
	}
}

void Window_ProcessEvents(double delta) {
	ProcessTouchInput();
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for PS Vita

void Window_EnableRawMouse(void)  { Input.RawMode = true; }
void Window_UpdateRawMouse(void)  {  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }


/*########################################################################################################################*
*-------------------------------------------------------Gamepads----------------------------------------------------------*
*#########################################################################################################################*/
static void HandleButtons(int port, int mods) {
	Gamepad_SetButton(port, CCPAD_A, mods & SCE_CTRL_TRIANGLE);
	Gamepad_SetButton(port, CCPAD_B, mods & SCE_CTRL_SQUARE);
	Gamepad_SetButton(port, CCPAD_X, mods & SCE_CTRL_CROSS);
	Gamepad_SetButton(port, CCPAD_Y, mods & SCE_CTRL_CIRCLE);
      
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
static void ProcessCircleInput(int port, int axis, int x, int y, double delta) {
	// May not be exactly 0 on actual hardware
	if (Math_AbsI(x) <= 8) x = 0;
	if (Math_AbsI(y) <= 8) y = 0;
	
	Gamepad_SetAxis(port, axis, x / AXIS_SCALE, y / AXIS_SCALE, delta);
}

static void ProcessPadInput(double delta) {
	SceCtrlData pad;
	
	// sceCtrlReadBufferPositive is blocking (seems to block until vblank), and don't want that
	int res = sceCtrlPeekBufferPositive(0, &pad, 1);
	if (res == 0) return; // no data available yet
	if (res < 0)  return; // error occurred
	// TODO: need to use cached version still? like GameCube/Wii
	
	HandleButtons(0, pad.buttons);
	ProcessCircleInput(0, PAD_AXIS_LEFT,  pad.lx - 127, pad.ly - 127, delta);
	ProcessCircleInput(0, PAD_AXIS_RIGHT, pad.rx - 127, pad.ry - 127, delta);
}

void Window_ProcessGamepads(double delta) {
	ProcessPadInput(delta);
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
static struct Bitmap fb_bmp;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	fb_bmp     = *bmp;
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
	if (ret) { Platform_Log1("ERROR SHOWING DIALOG: %i", &ret); return; }
	
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
static SceWChar16 imeTitle[33];
static SceWChar16 imeText[33];
static SceWChar16 imeBuffer[SCE_IME_DIALOG_MAX_TEXT_LENGTH];

static void SetIMEString(SceWChar16* dst, const cc_string* src) {
	int len = min(32, src->length);
	// TODO unicode conversion
	for (int i = 0; i < len; i++) dst[i] = src->buffer[i];
	dst[len] = '\0';
}

static void SendIMEResult(void) {
	char buffer[SCE_IME_DIALOG_MAX_TEXT_LENGTH];
	cc_string str;
	String_InitArray(str, buffer);
	
	for (int i = 0; i < SCE_IME_DIALOG_MAX_TEXT_LENGTH && imeBuffer[i]; i++)
	{
		char c = Convert_CodepointToCP437(imeBuffer[i]);
		String_Append(&str, c);
	}
	Event_RaiseString(&InputEvents.TextChanged, &str);
}

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { 
	SetIMEString(imeText,   args->text);
	SetIMEString(imeBuffer, &String_Empty);
	
	int mode = args->type & 0xFF;
	if (mode == KEYBOARD_TYPE_TEXT) {
		SetIMEString(imeTitle,  &(cc_string)String_FromConst("Enter text"));
	} else if (mode == KEYBOARD_TYPE_PASSWORD) {
		SetIMEString(imeTitle,  &(cc_string)String_FromConst("Enter password"));
	} else {
		SetIMEString(imeTitle,  &(cc_string)String_FromConst("Enter number"));
	}

    SceImeDialogParam param;
    sceImeDialogParamInit(&param);

    param.supportedLanguages = SCE_IME_LANGUAGE_ENGLISH_GB;
    param.languagesForced    = SCE_FALSE;
    param.type               = SCE_IME_TYPE_DEFAULT;
    param.option             = 0;
    param.textBoxMode        = SCE_IME_DIALOG_TEXTBOX_MODE_WITH_CLEAR;
    param.maxTextLength      = SCE_IME_DIALOG_MAX_TEXT_LENGTH;

    param.title           = imeTitle;
    param.initialText     = imeText;
    param.inputTextBuffer = imeBuffer;

    int ret = sceImeDialogInit(&param);
	if (ret) { Platform_Log1("ERROR SHOWING IME: %i", &ret); return; }
	
	void (*prevCallback)(void* fb);
	prevCallback   = DQ_OnNextFrame;
	DQ_OnNextFrame = DQ_DialogCallback;
    
	while (sceImeDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_RUNNING)
	{
		Gfx_UpdateCommonDialogBuffers();
		Gfx_NextFramebuffer();
		sceDisplayWaitVblankStart();
	}
	
	sceImeDialogTerm();
	DQ_OnNextFrame = prevCallback;
	SendIMEResult();
/* TODO implement */ 
}
void OnscreenKeyboard_SetText(const cc_string* text) { }
void OnscreenKeyboard_Draw2D(Rect2D* r, struct Bitmap* bmp) { }
void OnscreenKeyboard_Draw3D(void) { }
void OnscreenKeyboard_Close(void) { /* TODO implement */ }


#endif
