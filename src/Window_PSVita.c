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
#include <vitasdk.h>
static cc_bool launcherMode;

struct _DisplayData DisplayInfo;
struct _WinData WindowInfo;
// no DPI scaling on Xbox
int Display_ScaleX(int x) { return x; }
int Display_ScaleY(int y) { return y; }

#define BUFFER_WIDTH  960
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

	Input.GamepadSource = true;
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
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
void Window_ProcessEvents(void) {
	SceCtrlData pad;
	/* TODO implement */
	sceCtrlReadBufferPositive(0, &pad, 1);
	int mods = pad.buttons;
	
	int dx = pad.lx - 127;
	int dy = pad.ly - 127;
	if (Input_RawMode && (Math_AbsI(dx) > 1 || Math_AbsI(dy) > 1)) {
		//Platform_Log2("RAW: %i, %i", &dx, &dy);
		Event_RaiseRawMove(&PointerEvents.RawMoved, dx / 32.0f, dy / 32.0f);
	}
			
	
	Input_SetNonRepeatable(KeyBinds[KEYBIND_PLACE_BLOCK],  mods & SCE_CTRL_LTRIGGER);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_DELETE_BLOCK], mods & SCE_CTRL_RTRIGGER);
	
	Input_SetNonRepeatable(KeyBinds[KEYBIND_JUMP],      mods & SCE_CTRL_TRIANGLE);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_CHAT],      mods & SCE_CTRL_CIRCLE);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_INVENTORY], mods & SCE_CTRL_CROSS);
	// PSP_CTRL_SQUARE
	
	Input_SetNonRepeatable(IPT_ENTER,  mods & SCE_CTRL_START);
	Input_SetNonRepeatable(IPT_ESCAPE, mods & SCE_CTRL_SELECT);
	// fake tab with PSP_CTRL_SQUARE for Launcher too
	Input_SetNonRepeatable(IPT_TAB,    mods & SCE_CTRL_SQUARE);
	
	Input_SetNonRepeatable(KeyBinds[KEYBIND_LEFT],  mods & SCE_CTRL_LEFT);
	Input_SetNonRepeatable(IPT_LEFT,                mods & SCE_CTRL_LEFT);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_RIGHT], mods & SCE_CTRL_RIGHT);
	Input_SetNonRepeatable(IPT_RIGHT,               mods & SCE_CTRL_RIGHT);
	
	Input_SetNonRepeatable(KeyBinds[KEYBIND_FORWARD], mods & SCE_CTRL_UP);
	Input_SetNonRepeatable(IPT_UP,                    mods & SCE_CTRL_UP);
	Input_SetNonRepeatable(KeyBinds[KEYBIND_BACK],    mods & SCE_CTRL_DOWN);
	Input_SetNonRepeatable(IPT_DOWN,                  mods & SCE_CTRL_DOWN);
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for PS Vita

void Window_EnableRawMouse(void)  { Input_RawMode = true; }
void Window_UpdateRawMouse(void)  {  }
void Window_DisableRawMouse(void) { Input_RawMode = false; }


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
static struct Bitmap fb_bmp;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	fb_bmp     = *bmp;
}

#define ALIGNUP(size, a) (((size) + ((a) - 1)) & ~((a) - 1))
void Window_DrawFramebuffer(Rect2D r) {
	static SceUID fb_uid;
	static void* fb;
	// TODO: Purge when closing the 2D window, so more memory for 3D ClassiCube
	if (!fb) {
		int size = ALIGNUP(4 * BUFFER_WIDTH * SCREEN_HEIGHT, 256 * 1024);
		// https://wiki.henkaku.xyz/vita/SceSysmem
		fb_uid   = sceKernelAllocMemBlock("CC Framebuffer", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, size, NULL);
		if (fb_uid < 0) Logger_Abort2(fb_uid, "Failed to allocate 2D framebuffer");
		
		int res1 = sceKernelGetMemBlockBase(fb_uid, &fb);
		if (res1 < 0) Logger_Abort2(res1, "Failed to get base of 2D framebuffer");
		
		int res2 = sceGxmMapMemory(fb, size, SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE);
		if (res1 < 0) Logger_Abort2(res2, "Failed to map framebuffer for GPU usage");
		// https://wiki.henkaku.xyz/vita/GPU
	}
	
	sceDisplayWaitVblankStart();
	
	SceDisplayFrameBuf framebuf = { 0 };
	framebuf.size        = sizeof(SceDisplayFrameBuf);
	framebuf.base        = fb;
	framebuf.pitch       = BUFFER_WIDTH * 4;
	framebuf.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
	framebuf.width       = SCREEN_WIDTH;
	framebuf.height      = SCREEN_HEIGHT;
	
	sceDisplaySetFrameBuf(&framebuf, SCE_DISPLAY_SETBUF_NEXTFRAME);

	cc_uint32* src = (cc_uint32*)fb_bmp.scan0 + r.X;
	cc_uint32* dst = (cc_uint32*)fb           + r.X;

	for (int y = r.Y; y < r.Y + r.Height; y++) 
	{
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
