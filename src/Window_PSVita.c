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
struct _WinData WindowInfo;
// no DPI scaling on PS Vita
int Display_ScaleX(int x) { return x; }
int Display_ScaleY(int y) { return y; }

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
	DisplayInfo.Depth  = 4; // 32 bit
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	WindowInfo.Width   = DISPLAY_WIDTH;
	WindowInfo.Height  = DISPLAY_HEIGHT;
	WindowInfo.Focused = true;
	WindowInfo.Exists  = true;

	Input.Sources = INPUT_SOURCE_GAMEPAD;
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK,  SCE_TOUCH_SAMPLING_STATE_START);
	
	sceTouchGetPanelInfo(SCE_TOUCH_PORT_FRONT, &frontPanel);
	
	Gfx_InitGXM();
	Gfx_AllocFramebuffers();
}

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

void Window_Close(void) {
	Event_RaiseVoid(&WindowEvents.Closing);
}


/*########################################################################################################################*
*----------------------------------------------------Input processing-----------------------------------------------------*
*#########################################################################################################################*/
static void HandleButtons(int mods) {
	Input_SetNonRepeatable(CCPAD_A, mods & SCE_CTRL_TRIANGLE);
	Input_SetNonRepeatable(CCPAD_B, mods & SCE_CTRL_SQUARE);
	Input_SetNonRepeatable(CCPAD_X, mods & SCE_CTRL_CROSS);
	Input_SetNonRepeatable(CCPAD_Y, mods & SCE_CTRL_CIRCLE);
      
	Input_SetNonRepeatable(CCPAD_START,  mods & SCE_CTRL_START);
	Input_SetNonRepeatable(CCPAD_SELECT, mods & SCE_CTRL_SELECT);

	Input_SetNonRepeatable(CCPAD_LEFT,   mods & SCE_CTRL_LEFT);
	Input_SetNonRepeatable(CCPAD_RIGHT,  mods & SCE_CTRL_RIGHT);
	Input_SetNonRepeatable(CCPAD_UP,     mods & SCE_CTRL_UP);
	Input_SetNonRepeatable(CCPAD_DOWN,   mods & SCE_CTRL_DOWN);
	
	Input_SetNonRepeatable(CCPAD_L, mods & SCE_CTRL_LTRIGGER);
	Input_SetNonRepeatable(CCPAD_R, mods & SCE_CTRL_RTRIGGER);
}

static void ProcessLCircleInput(SceCtrlData* pad) {
	int dx = pad->lx - 127;
	int dy = pad->ly - 127;
	
	if (Math_AbsI(dx) <= 8) dx = 0;
	if (Math_AbsI(dy) <= 8) dy = 0;
	
	if (dx == 0 && dy == 0) return;
	Input.JoystickMovement = true;
	Input.JoystickAngle    = Math_Atan2(dx, dy);
}

static void ProcessRCircleInput(SceCtrlData* pad, double delta) {
	float scale = (delta * 60.0) / 16.0f;
	int dx = pad->rx - 127;
	int dy = pad->ry - 127;
	
	if (Math_AbsI(dx) <= 8) dx = 0;
	if (Math_AbsI(dy) <= 8) dy = 0;
	
	Event_RaiseRawMove(&PointerEvents.RawMoved, dx * scale, dy * scale);
}

static void ProcessTouchPress(int x, int y) {
	if (!frontPanel.maxDispX || !frontPanel.maxDispY) {
		// TODO: Shouldn't ever happen? need to check
		Pointer_SetPosition(0, x, y);
		return;
	}
	
	// rescale from touch range to screen range
	x = (x - frontPanel.minDispX) * DISPLAY_WIDTH  / frontPanel.maxDispX;
	y = (y - frontPanel.minDispY) * DISPLAY_HEIGHT / frontPanel.maxDispY;
	Pointer_SetPosition(0, x, y);
}

static void ProcessTouchInput(void) {
	SceTouchData touch;
	
	// sceTouchRead is blocking (seems to block until vblank), and don't want that
	int res = sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
	if (res == 0) return; // no data available yet
	if (res < 0)  return; // error occurred
	
	if (touch.reportNum > 0) {
		int x = touch.report[0].x;
		int y = touch.report[0].y;
		ProcessTouchPress(x, y);
	}
	Input_SetNonRepeatable(CCMOUSE_L, touch.reportNum > 0);
}

static void ProcessPadInput(double delta) {
	SceCtrlData pad;
	
	// sceCtrlReadBufferPositive is blocking (seems to block until vblank), and don't want that
	int res = sceCtrlPeekBufferPositive(0, &pad, 1);
	if (res == 0) return; // no data available yet
	if (res < 0)  return; // error occurred
	// TODO: need to use cached version still? like GameCube/Wii
	
	HandleButtons(pad.buttons);
	if (Input.RawMode) {
		ProcessLCircleInput(&pad);
		ProcessRCircleInput(&pad, delta);
	}
}

void Window_ProcessEvents(double delta) {
	Input.JoystickMovement = false;
	
	ProcessPadInput(delta);
	ProcessTouchInput();
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for PS Vita

void Window_EnableRawMouse(void)  { Input.RawMode = true; }
void Window_UpdateRawMouse(void)  {  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
static struct Bitmap fb_bmp;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	fb_bmp     = *bmp;
}

void Window_DrawFramebuffer(Rect2D r) {
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

void Window_OpenKeyboard(struct OpenKeyboardArgs* args) { 
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
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { /* TODO implement */ }


#endif
