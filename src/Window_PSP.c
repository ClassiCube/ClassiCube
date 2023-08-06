#include "Core.h"
#if defined CC_BUILD_PSP
#include "_WindowBase.h"
#include "Graphics.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Errors.h"
#include "ExtMath.h"
#include <pspdisplay.h>
#include <pspge.h>
#include <pspctrl.h>

#define BUFFER_WIDTH  512
#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 272
static cc_bool launcherMode;

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
	
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
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
	/* TODO implement */
}

static void HandleButtons_Launcher(int mods) {
	Input_SetNonRepeatable(CCKEY_ENTER,  mods & PSP_CTRL_TRIANGLE);
	Input_SetNonRepeatable(CCKEY_ESCAPE, mods & PSP_CTRL_SQUARE);
	// fake tab with PSP_CTRL_SQUARE for Launcher too
	//Input_SetNonRepeatable(CCKEY_TAB,    mods & PSP_CTRL_SQUARE);
	
	Input_SetNonRepeatable(CCPAD_LEFT,   mods & PSP_CTRL_LEFT);
	Input_SetNonRepeatable(CCPAD_RIGHT,  mods & PSP_CTRL_RIGHT);
	Input_SetNonRepeatable(CCPAD_UP,     mods & PSP_CTRL_UP);
	Input_SetNonRepeatable(CCPAD_DOWN,   mods & PSP_CTRL_DOWN);
}

static void HandleButtons_Game(int mods) {
	Input_SetNonRepeatable(CCPAD_L, mods & PSP_CTRL_LTRIGGER);
	Input_SetNonRepeatable(CCPAD_R, mods & PSP_CTRL_RTRIGGER);
	
	Input_SetNonRepeatable(CCPAD_A, mods & PSP_CTRL_TRIANGLE);
	Input_SetNonRepeatable(CCPAD_B, mods & PSP_CTRL_SQUARE);
	Input_SetNonRepeatable(CCPAD_X, mods & PSP_CTRL_CROSS);
	Input_SetNonRepeatable(CCPAD_Y, mods & PSP_CTRL_CIRCLE);
	
	Input_SetNonRepeatable(CCPAD_START,  mods & PSP_CTRL_START);
	Input_SetNonRepeatable(CCPAD_SELECT, mods & PSP_CTRL_SELECT);
	
	Input_SetNonRepeatable(CCPAD_LEFT,   mods & PSP_CTRL_LEFT);
	Input_SetNonRepeatable(CCPAD_RIGHT,  mods & PSP_CTRL_RIGHT);
	Input_SetNonRepeatable(CCPAD_UP,     mods & PSP_CTRL_UP);
	Input_SetNonRepeatable(CCPAD_DOWN,   mods & PSP_CTRL_DOWN);
}

void Window_ProcessEvents(double delta) {
	SceCtrlData pad;
	/* TODO implement */
	sceCtrlReadBufferPositive(&pad, 1);
	int mods = pad.Buttons;
	
	int dx = pad.Lx - 127;
	int dy = pad.Ly - 127;
	if (Input.RawMode && (Math_AbsI(dx) > 1 || Math_AbsI(dy) > 1)) {
		//Platform_Log2("RAW: %i, %i", &dx, &dy);
		Event_RaiseRawMove(&PointerEvents.RawMoved, dx / 32.0f, dy / 32.0f);
	}		
	
	if (launcherMode) {
		HandleButtons_Launcher(mods);
	} else {
		HandleButtons_Game(mods);
	}
}

static void Cursor_GetRawPos(int* x, int* y) {
	/* TODO implement */
	*x = 0; *y = 0;
}
void Cursor_SetPosition(int x, int y) {
	/* TODO implement */
}

static void Cursor_DoSetVisible(cc_bool visible) {
	/* TODO implement */
}

static void ShowDialogCore(const char* title, const char* msg) {
	/* TODO implement */
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

static struct Bitmap fb_bmp;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	fb_bmp     = *bmp;
}

void Window_DrawFramebuffer(Rect2D r) {
	void* fb = sceGeEdramGetAddr();
	
	sceDisplayWaitVblankStart();
	sceDisplaySetMode(0, SCREEN_WIDTH, SCREEN_HEIGHT);
	sceDisplaySetFrameBuf(fb, BUFFER_WIDTH, PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_SETBUF_IMMEDIATE);

	cc_uint32* src = (cc_uint32*)fb_bmp.scan0 + r.X;
	cc_uint32* dst = (cc_uint32*)fb           + r.X;

	for (int y = r.Y; y < r.Y + r.Height; y++) {
		Mem_Copy(dst + y * BUFFER_WIDTH, src + y * fb_bmp.width, r.Width * 4);
	}
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}

/*void Window_AllocFramebuffer(struct Bitmap* bmp) {
	void* fb = sceGeEdramGetAddr();
	bmp->scan0  = fb;
	bmp->width  = BUFFER_WIDTH;
	bmp->height = SCREEN_HEIGHT;
}

void Window_DrawFramebuffer(Rect2D r) {
	//sceDisplayWaitVblankStart();
	//sceDisplaySetMode(0, SCREEN_WIDTH, SCREEN_HEIGHT);
	//sceDisplaySetFrameBuf(sceGeEdramGetAddr(), BUFFER_WIDTH, PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_SETBUF_IMMEDIATE);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {

}*/

void Window_OpenKeyboard(struct OpenKeyboardArgs* args) { /* TODO implement */ }
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { /* TODO implement */ }

void Window_EnableRawMouse(void) {
	RegrabMouse();
	Input.RawMode = true;
}
void Window_UpdateRawMouse(void) { CentreMousePosition(); }

void Window_DisableRawMouse(void) {
	RegrabMouse();
	Input.RawMode = false;
}
#endif
