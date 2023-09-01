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

//#define BUFFER_WIDTH  960 TODO: 1024?
#define SCREEN_WIDTH  960
#define SCREEN_HEIGHT 544

void Window_Init(void) {
	DisplayInfo.Width  = SCREEN_WIDTH;
	DisplayInfo.Height = SCREEN_HEIGHT;
	DisplayInfo.Depth  = 4; // 32 bit
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	WindowInfo.Width   = SCREEN_WIDTH;
	WindowInfo.Height  = SCREEN_HEIGHT;
	WindowInfo.Focused = true;
	WindowInfo.Exists  = true;

	Input.Sources = INPUT_SOURCE_GAMEPAD;
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK,  SCE_TOUCH_SAMPLING_STATE_START);
	
	sceTouchGetPanelInfo(SCE_TOUCH_PORT_FRONT, &frontPanel);
}

void Window_Create2D(int width, int height) { launcherMode = true;  }
void Window_Create3D(int width, int height) { launcherMode = false; }

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
	/* TODO implement */
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

static void ProcessCircleInput(SceCtrlData* pad, double delta) {
	float scale = (delta * 60.0) / 16.0f;
	int dx = pad->lx - 127;
	int dy = pad->ly - 127;
	
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
	x = (x - frontPanel.minDispX) * SCREEN_WIDTH  / frontPanel.maxDispX;
	y = (y - frontPanel.minDispY) * SCREEN_HEIGHT / frontPanel.maxDispY;
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
		ProcessTouchPress(X, Y);
	}
	Input_SetNonRepeatable(CCMOUSE_L, touch.reportNum > 0);
}

static void ProcessPadInput(double delta) {
	SceCtrlData pad;
	
	// sceCtrlReadBufferPositive is blocking (seems to block until vblank), and don't want that
	int res = sceCtrlPeekBufferPositive(0, &pad, 1);
	if (res == 0) return; // no data available yet
	if (res < 0)  return; // error occurred
	
	HandleButtons(pad.buttons);
	if (Input.RawMode)
		ProcessCircleInput(&pad, delta);
}

void Window_ProcessEvents(double delta) {
	/* TODO implement */
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

extern void* AllocGPUMemory(int size, int type, int gpu_access, SceUID* ret_uid);

void Window_DrawFramebuffer(Rect2D r) {
	static SceUID fb_uid;
	static void* fb;
	
	// TODO: Purge when closing the 2D window, so more memory for 3D ClassiCube
	// TODO: Use framebuffers directly instead of our own internal framebuffer too..
	if (!fb) {
		int size = 4 * SCREEN_WIDTH * SCREEN_HEIGHT;
		fb = AllocGPUMemory(size, SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, 
							SCE_GXM_MEMORY_ATTRIB_RW, &fb_uid);
	}
		
	sceDisplayWaitVblankStart();
	
	SceDisplayFrameBuf framebuf = { 0 };
	framebuf.size        = sizeof(SceDisplayFrameBuf);
	framebuf.base        = fb;
	framebuf.pitch       = SCREEN_WIDTH;
	framebuf.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
	framebuf.width       = SCREEN_WIDTH;
	framebuf.height      = SCREEN_HEIGHT;

	sceDisplaySetFrameBuf(&framebuf, SCE_DISPLAY_SETBUF_NEXTFRAME);

	cc_uint32* src = (cc_uint32*)fb_bmp.scan0 + r.X;
	cc_uint32* dst = (cc_uint32*)fb           + r.X;

	for (int y = r.Y; y < r.Y + r.Height; y++) 
	{
		Mem_Copy(dst + y * SCREEN_WIDTH, src + y * fb_bmp.width, r.Width * 4);
	}
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
