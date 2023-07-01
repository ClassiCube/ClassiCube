#include "Core.h"
#if defined CC_BUILD_GCWII
#include "_WindowBase.h"
#include "Graphics.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Errors.h"
#include "ExtMath.h"
#include <gccore.h>
#if defined HW_RVL
#include <wiiuse/wpad.h>
#endif

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
	WindowInfo.Focused = true;
	
	#if defined HW_RVL
	WPAD_Init();
	#elif defined HW_DOL
	PAD_Init();
	#endif
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

#if defined HW_RVL
void Window_ProcessEvents(void) {
	/* TODO implement */
	WPAD_ScanPads();
	u32 mods = WPAD_ButtonsDown(0) | WPAD_ButtonsHeld(0);		
	
	Input_SetNonRepeatable(KeyBinds[KEYBIND_PLACE_BLOCK],  mods & WPAD_BUTTON_1);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_DELETE_BLOCK], mods & WPAD_BUTTON_2);
	
	Input_SetNonRepeatable(KeyBinds[KEYBIND_JUMP],      mods & WPAD_BUTTON_A);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_CHAT],      mods & WPAD_BUTTON_B);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_INVENTORY], mods & WPAD_BUTTON_PLUS);
	
	Input_SetNonRepeatable(IPT_ENTER,  mods & WPAD_BUTTON_HOME);
	Input_SetNonRepeatable(IPT_ESCAPE, mods & WPAD_BUTTON_MINUS);
	
	Input_SetNonRepeatable(KeyBinds[KEYBIND_LEFT],  mods & WPAD_BUTTON_LEFT);
	Input_SetNonRepeatable(IPT_LEFT,                mods & WPAD_BUTTON_LEFT);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_RIGHT], mods & WPAD_BUTTON_RIGHT);
	Input_SetNonRepeatable(IPT_RIGHT,               mods & WPAD_BUTTON_RIGHT);
	
	Input_SetNonRepeatable(KeyBinds[KEYBIND_FORWARD], mods & WPAD_BUTTON_UP);
	Input_SetNonRepeatable(IPT_UP,                    mods & WPAD_BUTTON_UP);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_BACK],    mods & WPAD_BUTTON_DOWN);
	Input_SetNonRepeatable(IPT_DOWN,                  mods & WPAD_BUTTON_DOWN);
	
}
#elif defined HW_DOL
void Window_ProcessEvents(void) {
	/* TODO implement */
	PADStatus pads[4];
	PAD_Read(pads);
	int mods = pads[0].button;
	
	int dx = pads[0].stickX;
	int dy = pads[0].stickY;
	if (Input_RawMode && (Math_AbsI(dx) > 1 || Math_AbsI(dy) > 1)) {
		Event_RaiseRawMove(&PointerEvents.RawMoved, dx / 32.0f, dy / 32.0f);
	}
			
	
	Input_SetNonRepeatable(KeyBinds[KEYBIND_PLACE_BLOCK],  mods & PAD_TRIGGER_L);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_DELETE_BLOCK], mods & PAD_TRIGGER_R);
	
	Input_SetNonRepeatable(KeyBinds[KEYBIND_JUMP],      mods & PAD_BUTTON_A);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_CHAT],      mods & PAD_BUTTON_X);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_INVENTORY], mods & PAD_BUTTON_Y);
	// PAD_BUTTON_B
	
	Input_SetNonRepeatable(IPT_ENTER,  mods & PAD_BUTTON_START);
	Input_SetNonRepeatable(IPT_ESCAPE, mods & PAD_TRIGGER_Z);
	// fake tab with PAD_BUTTON_B for Launcher too
	Input_SetNonRepeatable(IPT_TAB,    mods & PAD_BUTTON_B);
	
	Input_SetNonRepeatable(KeyBinds[KEYBIND_LEFT],  mods & PAD_BUTTON_LEFT);
	Input_SetNonRepeatable(IPT_LEFT,                mods & PAD_BUTTON_LEFT);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_RIGHT], mods & PAD_BUTTON_RIGHT);
	Input_SetNonRepeatable(IPT_RIGHT,               mods & PAD_BUTTON_RIGHT);
	
	Input_SetNonRepeatable(KeyBinds[KEYBIND_FORWARD], mods & PAD_BUTTON_UP);
	Input_SetNonRepeatable(IPT_UP,                    mods & PAD_BUTTON_UP);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_BACK],    mods & PAD_BUTTON_DOWN);
	Input_SetNonRepeatable(IPT_DOWN,                  mods & PAD_BUTTON_DOWN);
}
#endif

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



// TODO: Get rid of this complexity and use the 3D API instead..
// https://github.com/devkitPro/gamecube-examples/blob/master/graphics/fb/pageflip/source/flip.c
static u32 CvtRGB (u8 r1, u8 g1, u8 b1, u8 r2, u8 g2, u8 b2)
{
  int y1, cb1, cr1, y2, cb2, cr2, cb, cr;
 
  y1  = (299 * r1 + 587 * g1 + 114 * b1) / 1000;
  cb1 = (-16874 * r1 - 33126 * g1 + 50000 * b1 + 12800000) / 100000;
  cr1 = (50000 * r1 - 41869 * g1 - 8131 * b1 + 12800000) / 100000;
 
  y2  = (299 * r2 + 587 * g2 + 114 * b2) / 1000;
  cb2 = (-16874 * r2 - 33126 * g2 + 50000 * b2 + 12800000) / 100000;
  cr2 = (50000 * r2 - 41869 * g2 - 8131 * b2 + 12800000) / 100000;
 
  cb = (cb1 + cb2) >> 1;
  cr = (cr1 + cr2) >> 1;
 
  return (y1 << 24) | (cb << 16) | (y2 << 8) | cr;
}

void Window_DrawFramebuffer(Rect2D r) {
	VIDEO_WaitVSync();
	r.X &= ~0x01; // round down to nearest even horizontal index
	
	// TODO XFB is raw yuv, but is absolutely a pain to work with..
	for (int y = r.Y; y < r.Y + r.Height; y++) 
	{
		cc_uint32* src = fb_bmp.scan0 + y * fb_bmp.width   + r.X;
		u16* dst       = (u16*)xfb    + y * rmode->fbWidth + r.X;
		
		for (int x = 0; x < r.Width / 2; x++) {
			cc_uint32 rgb0 = src[(x<<1) + 0];
			cc_uint32 rgb1 = src[(x<<1) + 1];
			
			((u32*)dst)[x] = CvtRGB(BitmapCol_R(rgb0), BitmapCol_G(rgb0), BitmapCol_B(rgb0),
					BitmapCol_R(rgb1),  BitmapCol_G(rgb1), BitmapCol_B(rgb1));
		}
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