#include "Core.h"
#if defined CC_BUILD_GCWII
#include "_WindowBase.h"
#include "Graphics.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Errors.h"
#include <gccore.h>

static void* xfb;
static GXRModeObj* rmode;

void* Window_XFB;

void Window_Init(void) {
	// Initialise the video system
	VIDEO_Init();

	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);
	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	
	Window_XFB = xfb;	
	//console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);
	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);

	// Make the display visible
	VIDEO_SetBlack(FALSE);
	// Flush the video register changes to the hardware
	VIDEO_Flush();
	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	
	DisplayInfo.Width  = rmode->fbWidth;
	DisplayInfo.Height = rmode->xfbHeight;
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	WindowInfo.Width   = rmode->fbWidth;
	WindowInfo.Height  = rmode->xfbHeight;
	//WindowInfo.Focused = true;
}

static void DoCreateWindow(int _3d) {
	WindowInfo.Exists = true;
}
void Window_Create2D(int width, int height) { DoCreateWindow(0); }
void Window_Create3D(int width, int height) { DoCreateWindow(1); }

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

void Window_ProcessEvents(void) {
	/* TODO implement */
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
	VIDEO_WaitVSync();
	
	// TODO XFB is raw yuv, but is absolutely a pain to work with..
	for (int y = r.Y; y < r.Y + r.Height; y++) 
	{
		cc_uint32* src = fb_bmp.scan0 + y * fb_bmp.width   + r.X;
		u16* dst       = (u16*)xfb    + y * rmode->fbWidth + r.X;
		
		for (int x = 0; x < r.Width; x++)
			dst[x] = src[x];
	}
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}

void Window_OpenKeyboard(struct OpenKeyboardArgs* args) { /* TODO implement */ }
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { /* TODO implement */ }

void Window_EnableRawMouse(void) {
	RegrabMouse();
	Input_RawMode = true;
}
void Window_UpdateRawMouse(void) { CentreMousePosition(); }

void Window_DisableRawMouse(void) {
	RegrabMouse();
	Input_RawMode = false;
}
#endif