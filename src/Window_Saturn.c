#include "Core.h"
#if defined CC_BUILD_SATURN
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <yaul.h>

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   224

static cc_bool launcherMode;
static smpc_peripheral_digital_t state;

struct _DisplayData DisplayInfo;
struct _WindowData WindowInfo;

static void OnVblank(void* work) {
	smpc_peripheral_intback_issue();
}

void Window_Init(void) {
	DisplayInfo.Width  = SCREEN_WIDTH;
	DisplayInfo.Height = SCREEN_HEIGHT;
	DisplayInfo.ScaleX = 0.5f;
	DisplayInfo.ScaleY = 0.5f;
	
	Window_Main.Width   = DisplayInfo.Width;
	Window_Main.Height  = DisplayInfo.Height;
	Window_Main.Focused = true;
	Window_Main.Exists  = true;

	Input.Sources = INPUT_SOURCE_GAMEPAD;
	DisplayInfo.ContentOffsetX = 10;
	DisplayInfo.ContentOffsetY = 10;

	vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A, VDP2_TVMD_VERT_224);
	vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(0, 0), RGB1555(1, 19, 0, 0));
	vdp2_tvmd_display_set();

	smpc_peripheral_init();
	vdp_sync_vblank_out_set(OnVblank, NULL);
}

void Window_Free(void) { }

void Window_Create3D(int width, int height) { 
	launcherMode = false; 
}

void Window_SetTitle(const cc_string* title) { }
void Clipboard_GetText(cc_string* value) { }
void Clipboard_SetText(const cc_string* value) { }

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
void Window_ProcessEvents(double delta) {
	smpc_peripheral_process();
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for PS Vita

void Window_EnableRawMouse(void)  { Input.RawMode = true; }
void Window_UpdateRawMouse(void)  {  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }


/*########################################################################################################################*
*-------------------------------------------------------Gamepads----------------------------------------------------------*
*#########################################################################################################################*/
static void ProcessButtons(int mods) {
	Gamepad_SetButton(CCPAD_A, mods & PERIPHERAL_DIGITAL_A);
	Gamepad_SetButton(CCPAD_B, mods & PERIPHERAL_DIGITAL_B);
	Gamepad_SetButton(CCPAD_X, mods & PERIPHERAL_DIGITAL_C);

	Gamepad_SetButton(CCPAD_L, mods & PERIPHERAL_DIGITAL_L);
	Gamepad_SetButton(CCPAD_R, mods & PERIPHERAL_DIGITAL_R);
      
	Gamepad_SetButton(CCPAD_START,  mods & PERIPHERAL_DIGITAL_START);
	Gamepad_SetButton(CCPAD_SELECT, mods & PERIPHERAL_DIGITAL_Z);

	Gamepad_SetButton(CCPAD_LEFT,   mods & PERIPHERAL_DIGITAL_LEFT);
	Gamepad_SetButton(CCPAD_RIGHT,  mods & PERIPHERAL_DIGITAL_RIGHT);
	Gamepad_SetButton(CCPAD_UP,     mods & PERIPHERAL_DIGITAL_UP);
	Gamepad_SetButton(CCPAD_DOWN,   mods & PERIPHERAL_DIGITAL_DOWN);
}

void Window_ProcessGamepads(double delta) {
	smpc_peripheral_digital_port(1, &state);
	ProcessButtons(state.pressed.raw | state.held.raw);
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_Create2D(int width, int height) {
	launcherMode = true;

	const vdp2_scrn_bitmap_format_t format = {
		.scroll_screen = VDP2_SCRN_NBG0,
		.ccc           = VDP2_SCRN_CCC_RGB_32768,
		.bitmap_size   = VDP2_SCRN_BITMAP_SIZE_512X256,
		.palette_base  = 0x00000000,
		.bitmap_base   = VDP2_VRAM_ADDR(0, 0x00000)
	};

	vdp2_scrn_bitmap_format_set(&format);
	vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
	vdp2_scrn_display_set(VDP2_SCRN_DISP_NBG0);

        const vdp2_vram_cycp_t vram_cycp = {
                .pt[0].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t5 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t6 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t7 = VDP2_VRAM_CYCP_CHPNDR_NBG0,

                .pt[1].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t5 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t6 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t7 = VDP2_VRAM_CYCP_CHPNDR_NBG0,

                .pt[2].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t5 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t6 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t7 = VDP2_VRAM_CYCP_CHPNDR_NBG0,

                .pt[3].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[3].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[3].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[3].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[3].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[3].t5 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[3].t6 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[3].t7 = VDP2_VRAM_CYCP_CHPNDR_NBG0
        };

        vdp2_vram_cycp_set(&vram_cycp);
}

void Window_AllocFramebuffer(struct Bitmap* bmp) {
	// bottom half of VRAM is allocated for the 512x256 16 bit per pixel "bitmap"
	// but since only draw 224 rows anyways, can move the launcher framebuffer 
	// slightly into the end of the bottom half of VRAM
	bmp->scan0 = (BitmapCol*)VDP2_VRAM_ADDR(1, SCREEN_HEIGHT * 512);
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	volatile rgb1555_t* vram = (volatile rgb1555_t*)VDP2_VRAM_ADDR(0, 0x00000);

	// TODO: Partial redraws seem to produce some corrupt pixels ???
	for (int y = r.y; y < r.y + r.height; y++) 
	{
		BitmapCol* row = Bitmap_GetRow(bmp, y);
		for (int x = r.x; x < r.x + r.width; x++) 
		{
			// TODO optimise
			BitmapCol col = row[x];
			cc_uint8 r = BitmapCol_R(col);
			cc_uint8 g = BitmapCol_G(col);
			cc_uint8 b = BitmapCol_B(col);
			vram[x + (y * 512)] = RGB1555(0, r >> 3, g >> 3, b >> 3);
		}
	}

	vdp2_sync();
	vdp2_sync_wait();
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = NULL;
}


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { /* TODO implement */ }
void OnscreenKeyboard_SetText(const cc_string* text) { }
void OnscreenKeyboard_Draw2D(Rect2D* r, struct Bitmap* bmp) { }
void OnscreenKeyboard_Draw3D(void) { }
void OnscreenKeyboard_Close(void) { /* TODO implement */ }


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
