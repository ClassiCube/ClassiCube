#include "Core.h"
#if defined CC_BUILD_WIIU
#include "Window.h"
#include "Platform.h"
#include "Input.h"
#include "Event.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Errors.h"
#include "ExtMath.h"
#include "Graphics.h"
#include "Launcher.h"
#include <coreinit/memheap.h>
#include <coreinit/cache.h>
#include <coreinit/memfrmheap.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>
#include <coreinit/screen.h>
#include <proc_ui/procui.h>
#include <gx2/display.h>
#include <vpad/input.h>
#include <whb/proc.h>
#include <padscore/kpad.h>
#include <whb/gfx.h>
#include <gx2/mem.h>

static cc_bool launcherMode;
struct _DisplayData DisplayInfo;
struct _WindowData WindowInfo;
struct _WindowData Window_Alt;
cc_bool launcherTop;

static void LoadTVDimensions(void) {
	switch(GX2GetSystemTVScanMode())
	{
	case GX2_TV_SCAN_MODE_480I:
	case GX2_TV_SCAN_MODE_480P:
		DisplayInfo.Width  = 854;
		DisplayInfo.Height = 480;
		break;
	case GX2_TV_SCAN_MODE_1080I:
	case GX2_TV_SCAN_MODE_1080P:
		DisplayInfo.Width  = 1920;
		DisplayInfo.Height = 1080;
		break;
	case GX2_TV_SCAN_MODE_720P:
	default:
		DisplayInfo.Width  = 1280;
		DisplayInfo.Height =  720;
	break;
	}
}

static uint32_t OnAcquired(void* context) {
	Window_Main.Inactive = false;
	Event_RaiseVoid(&WindowEvents.InactiveChanged);
	return 0;
}

static uint32_t OnReleased(void* context) {
	Window_Main.Inactive = true;
	Event_RaiseVoid(&WindowEvents.InactiveChanged);
	return 0;
}

void Window_Init(void) {
	LoadTVDimensions();
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	Window_Main.Width   = DisplayInfo.Width;
	Window_Main.Height  = DisplayInfo.Height;
	Window_Main.Focused = true;
	Window_Main.Exists  = true;

	Input.Sources = INPUT_SOURCE_GAMEPAD;
	DisplayInfo.ContentOffsetX = 10;
	DisplayInfo.ContentOffsetY = 10;

	Window_Main.SoftKeyboard = SOFT_KEYBOARD_RESIZE;
	Input_SetTouchMode(true);
	
	Window_Alt.Width  = 854;
	Window_Alt.Height = 480;
		
	KPADInit();
	VPADInit();

	ProcUIRegisterCallback(PROCUI_CALLBACK_ACQUIRE, OnAcquired, NULL, 100);
	ProcUIRegisterCallback(PROCUI_CALLBACK_RELEASE, OnReleased, NULL, 100);
	WHBGfxInit();
}

void Window_Free(void) { }

// OSScreen is always this buffer size, regardless of the current TV resolution
#define OSSCREEN_TV_WIDTH  1280
#define OSSCREEN_TV_HEIGHT  720
#define OSSCREEN_DRC_WIDTH  854
#define OSSCREEN_DRC_HEIGHT 480
static void LauncherInactiveChanged(void* obj);
static void Init2DResources(void);

void Window_Create2D(int width, int height) {
	Window_Main.Width  = OSSCREEN_DRC_WIDTH;
	Window_Main.Height = OSSCREEN_DRC_HEIGHT;

	launcherMode = true;
	Event_Register_(&WindowEvents.InactiveChanged,   LauncherInactiveChanged, NULL);
	Init2DResources();
}

void Window_Create3D(int width, int height) { 
	Window_Main.Width   = DisplayInfo.Width;
	Window_Main.Height  = DisplayInfo.Height;

	launcherMode = false; 
	Event_Unregister_(&WindowEvents.InactiveChanged, LauncherInactiveChanged, NULL);
}

void Window_RequestClose(void) {
	Event_RaiseVoid(&WindowEvents.Closing);
}


/*########################################################################################################################*
*----------------------------------------------------Input processing-----------------------------------------------------*
*#########################################################################################################################*/
extern Rect2D dirty_rect;
void Window_ProcessEvents(float delta) {
	if (!dirty_rect.width) dirty_rect.width = 1;

	if (!WHBProcIsRunning()) {
		Window_Main.Exists = false;
		Window_RequestClose();
	}
}

void Window_UpdateRawMouse(void) { }

void Cursor_SetPosition(int x, int y) { }
void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }


/*########################################################################################################################*
*-------------------------------------------------------Gamepads----------------------------------------------------------*
*#########################################################################################################################*/
static void ProcessKPAD(float delta) {
	KPADStatus kpad = { 0 };
	int res = KPADRead(0, &kpad, 1);
	if (res != KPAD_ERROR_OK) return;
	
	switch (kpad.extensionType)
	{
	
	}
}


#define AXIS_SCALE 4.0f
static void ProcessVpadStick(int port, int axis, float x, float y, float delta) {
	// May not be exactly 0 on actual hardware
	if (Math_AbsF(x) <= 0.1f) x = 0;
	if (Math_AbsF(y) <= 0.1f) y = 0;
	
	Gamepad_SetAxis(port, axis, x * AXIS_SCALE, -y * AXIS_SCALE, delta);
}
   
static void ProcessVpadButtons(int port, int mods) {
	Gamepad_SetButton(port, CCPAD_L,  mods & VPAD_BUTTON_L);
	Gamepad_SetButton(port, CCPAD_R,  mods & VPAD_BUTTON_R);
	Gamepad_SetButton(port, CCPAD_ZL, mods & VPAD_BUTTON_ZL);
	Gamepad_SetButton(port, CCPAD_ZR, mods & VPAD_BUTTON_ZR);
      
	Gamepad_SetButton(port, CCPAD_A, mods & VPAD_BUTTON_A);
	Gamepad_SetButton(port, CCPAD_B, mods & VPAD_BUTTON_B);
	Gamepad_SetButton(port, CCPAD_X, mods & VPAD_BUTTON_X);
	Gamepad_SetButton(port, CCPAD_Y, mods & VPAD_BUTTON_Y);
      
	Gamepad_SetButton(port, CCPAD_START,  mods & VPAD_BUTTON_PLUS);
	Gamepad_SetButton(port, CCPAD_SELECT, mods & VPAD_BUTTON_MINUS);

	Gamepad_SetButton(port, CCPAD_LEFT,   mods & VPAD_BUTTON_LEFT);
	Gamepad_SetButton(port, CCPAD_RIGHT,  mods & VPAD_BUTTON_RIGHT);
	Gamepad_SetButton(port, CCPAD_UP,     mods & VPAD_BUTTON_UP);
	Gamepad_SetButton(port, CCPAD_DOWN,   mods & VPAD_BUTTON_DOWN);
	
}

static void ProcessVpadTouch(VPADTouchData* data) {
	static int was_touched;

	// TODO rescale to main screen size
	if (data->touched) {
		int x = data->x;
		int y = data->y;
		Platform_Log2("TOUCH: %i, %i", &x, &y);

		x = x * Window_Main.Width  / 1280;
		y = y * Window_Main.Height /  720;

		Input_AddTouch(0, x, y);
	} else if (was_touched) {
		Input_RemoveTouch(0, Pointers[0].x, Pointers[0].y);
	}
	was_touched = data->touched;
}

static void ProcessVPAD(float delta) {
	VPADStatus vpadStatus;
	VPADReadError error = VPAD_READ_SUCCESS;
	VPADRead(VPAD_CHAN_0, &vpadStatus, 1, &error);
	if (error != VPAD_READ_SUCCESS) return;
	
	VPADGetTPCalibratedPoint(VPAD_CHAN_0, &vpadStatus.tpNormal, &vpadStatus.tpNormal);
	ProcessVpadButtons(0, vpadStatus.hold);
	ProcessVpadTouch(&vpadStatus.tpNormal);
	
	ProcessVpadStick(0, PAD_AXIS_LEFT,  vpadStatus.leftStick.x,  vpadStatus.leftStick.y,  delta);
	ProcessVpadStick(0, PAD_AXIS_RIGHT, vpadStatus.rightStick.x, vpadStatus.rightStick.y, delta);
}


void Window_ProcessGamepads(float delta) {
	ProcessVPAD(delta);
	ProcessKPAD(delta);
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID framebuffer_vb;

static void LauncherInactiveChanged(void* obj) {
	// TODO
}

static void Init2DResources(void) {
	Gfx_Create();
	if (framebuffer_vb) return;

	struct VertexTextured* data = (struct VertexTextured*)Gfx_RecreateAndLockVb(&framebuffer_vb,
															VERTEX_FORMAT_TEXTURED, 4);
	data[0].x = -1.0f; data[0].y = -1.0f; data[0].z = 0.0f; data[0].Col = PACKEDCOL_WHITE; data[0].U = 0.0f; data[0].V = 1.0f;
	data[1].x =  1.0f; data[1].y = -1.0f; data[1].z = 0.0f; data[1].Col = PACKEDCOL_WHITE; data[1].U = 1.0f; data[1].V = 1.0f;
	data[2].x =  1.0f; data[2].y =  1.0f; data[2].z = 0.0f; data[2].Col = PACKEDCOL_WHITE; data[2].U = 1.0f; data[2].V = 0.0f;
	data[3].x = -1.0f; data[3].y =  1.0f; data[3].z = 0.0f; data[3].Col = PACKEDCOL_WHITE; data[3].U = 0.0f; data[2].V = 0.0f;

	Gfx_UnlockVb(framebuffer_vb);
}

static GX2Texture fb;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	fb.surface.width    = bmp->width;
	fb.surface.height   = bmp->height;
	fb.surface.depth    = 1;
	fb.surface.dim      = GX2_SURFACE_DIM_TEXTURE_2D;
	fb.surface.format   = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
	fb.surface.tileMode = GX2_TILE_MODE_LINEAR_ALIGNED;
	fb.viewNumSlices    = 1;
	fb.compMap          = 0x00010203;
	GX2CalcSurfaceSizeAndAlignment(&fb.surface);
	GX2InitTextureRegs(&fb);

	fb.surface.image = MEMAllocFromDefaultHeapEx(fb.surface.imageSize, fb.surface.alignment);
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
}

static void DrawIt(struct Bitmap* bmp) {
	Gfx_LoadIdentityMatrix(MATRIX_VIEW);
	Gfx_LoadIdentityMatrix(MATRIX_PROJECTION);
	Gfx_SetDepthTest(false);

	Gfx_SetVertexFormat(VERTEX_FORMAT_COLOURED);
	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_BindTexture(&fb);
	Gfx_BindVb(framebuffer_vb);
	Gfx_DrawVb_IndexedTris(4);
}

static void DrawTV(struct Bitmap* bmp) {
	WHBGfxBeginRenderTV();
	WHBGfxClearColor(0.0f, 0.0f, 1.0f, 1.0f);
	DrawIt(bmp);	
	WHBGfxFinishRenderTV();
}

static void DrawDRC(struct Bitmap* bmp) {
	WHBGfxBeginRenderDRC();
	WHBGfxClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	DrawIt(bmp);
	WHBGfxFinishRenderDRC();

}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	if (launcherTop || Window_Main.Inactive) return;

	struct Bitmap part;
	part.scan0  = Bitmap_GetRow(bmp, r.y) + r.x;
	part.width  = r.width;
	part.height = r.height;
	Gfx_UpdateTexture(&fb, r.x, r.y, &part, bmp->width, false);

	WHBGfxBeginRender();
	DrawTV(bmp);
	DrawDRC(bmp);
	WHBGfxFinishRender();
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	MEMFreeToDefaultHeap(fb.surface.image);
	Mem_Free(bmp->scan0);
}


/*########################################################################################################################*
*-------------------------------------------------------Misc/Other--------------------------------------------------------*
*#########################################################################################################################*/
void Window_SetTitle(const cc_string* title)   { }
void Clipboard_GetText(cc_string* value)       { }
void Clipboard_SetText(const cc_string* value) { }

int Window_GetWindowState(void) { return WINDOW_STATE_FULLSCREEN; }
cc_result Window_EnterFullscreen(void) { return 0; }
cc_result Window_ExitFullscreen(void)  { return 0; }
int Window_IsObscured(void)            { return 0; }

void Window_Show(void) { }
void Window_SetSize(int width, int height) { }


void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { /* TODO implement */ }
void OnscreenKeyboard_SetText(const cc_string* text) { }
void OnscreenKeyboard_Draw2D(Rect2D* r, struct Bitmap* bmp) { }
void OnscreenKeyboard_Draw3D(void) { }
void OnscreenKeyboard_Close(void) { /* TODO implement */ }

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
