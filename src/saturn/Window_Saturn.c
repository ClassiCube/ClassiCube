#include "../Window.h"
#include "../Platform.h"
#include "../Input.h"
#include "../Event.h"
#include "../Graphics.h"
#include "../String_.h"
#include "../Funcs.h"
#include "../Bitmap.h"
#include "../Errors.h"
#include "../ExtMath.h"
#include "../Options.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <yaul.h>

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   224

struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;

static void OnVblank(void* work) {
	smpc_peripheral_intback_issue();
}

void Window_PreInit(void) {
	DisplayInfo.Width  = SCREEN_WIDTH;
	DisplayInfo.Height = SCREEN_HEIGHT;
	DisplayInfo.ScaleX = 0.5f;
	DisplayInfo.ScaleY = 0.5f;
}

void Window_Init(void) {
	Window_Main.Width    = DisplayInfo.Width;
	Window_Main.Height   = DisplayInfo.Height;
	Window_Main.Focused  = true;
	
	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;

	DisplayInfo.ContentOffsetX = Option_GetOffsetX(10);
	DisplayInfo.ContentOffsetY = Option_GetOffsetY(10);

	vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A, VDP2_TVMD_VERT_224);
	vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(0, 0), RGB1555(1, 19, 0, 0));
	vdp2_tvmd_display_set();

	vdp_sync_vblank_out_set(OnVblank, NULL);
}

void Window_Free(void) { }

void Window_Create3D(int width, int height) { 
	Window_Main.Is3D = true;
}

void Window_Destroy(void) { }

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
void Window_ProcessEvents(float delta) {
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for PS Vita

void Window_EnableRawMouse(void)  { Input.RawMode = true; }
void Window_UpdateRawMouse(void)  {  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }


/*########################################################################################################################*
*-------------------------------------------------------Gamepads----------------------------------------------------------*
*#########################################################################################################################*/
static const BindMapping saturn_defaults[BIND_COUNT] = {
	[BIND_LOOK_UP]      = { CCPAD_4, CCPAD_UP },
	[BIND_LOOK_DOWN]    = { CCPAD_4, CCPAD_DOWN },
	[BIND_LOOK_LEFT]    = { CCPAD_4, CCPAD_LEFT },
	[BIND_LOOK_RIGHT]   = { CCPAD_4, CCPAD_RIGHT },
	[BIND_FORWARD]      = { CCPAD_UP,    0 },
	[BIND_BACK]         = { CCPAD_DOWN,  0 },
	[BIND_LEFT]         = { CCPAD_LEFT,  0 },
	[BIND_RIGHT]        = { CCPAD_RIGHT, 0 },
	[BIND_JUMP]         = { CCPAD_1,     0 },
	[BIND_SET_SPAWN]    = { CCPAD_START, 0 }, 
	[BIND_INVENTORY]    = { CCPAD_3,     0 },
	[BIND_SPEED]        = { CCPAD_2, CCPAD_L},
	[BIND_NOCLIP]       = { CCPAD_2, CCPAD_3},
	[BIND_FLY]          = { CCPAD_2, CCPAD_R },
	[BIND_FLY_UP]       = { CCPAD_2, CCPAD_UP },
	[BIND_FLY_DOWN]     = { CCPAD_2, CCPAD_DOWN },
	[BIND_PLACE_BLOCK]  = { CCPAD_L },
	[BIND_DELETE_BLOCK] = { CCPAD_R },
};

void Gamepads_PreInit(void) {
	smpc_peripheral_init();
}

void Gamepads_Init(void) {
	Input_DisplayNames[CCPAD_1] = "A";
	Input_DisplayNames[CCPAD_2] = "B";
	Input_DisplayNames[CCPAD_3] = "C";
	Input_DisplayNames[CCPAD_4] = "X";
	Input_DisplayNames[CCPAD_5] = "Y";
}

static void ProcessButtons(int port, int mods) {
	Gamepad_SetButton(port, CCPAD_1, mods & PERIPHERAL_DIGITAL_A);
	Gamepad_SetButton(port, CCPAD_2, mods & PERIPHERAL_DIGITAL_B);
	Gamepad_SetButton(port, CCPAD_3, mods & PERIPHERAL_DIGITAL_C);
	
	Gamepad_SetButton(port, CCPAD_4, mods & PERIPHERAL_DIGITAL_X);
	Gamepad_SetButton(port, CCPAD_5, mods & PERIPHERAL_DIGITAL_Y);

	Gamepad_SetButton(port, CCPAD_L, mods & PERIPHERAL_DIGITAL_L);
	Gamepad_SetButton(port, CCPAD_R, mods & PERIPHERAL_DIGITAL_R);
      
	Gamepad_SetButton(port, CCPAD_START,  mods & PERIPHERAL_DIGITAL_START);
	Gamepad_SetButton(port, CCPAD_SELECT, mods & PERIPHERAL_DIGITAL_Z);

	Gamepad_SetButton(port, CCPAD_LEFT,   mods & PERIPHERAL_DIGITAL_LEFT);
	Gamepad_SetButton(port, CCPAD_RIGHT,  mods & PERIPHERAL_DIGITAL_RIGHT);
	Gamepad_SetButton(port, CCPAD_UP,     mods & PERIPHERAL_DIGITAL_UP);
	Gamepad_SetButton(port, CCPAD_DOWN,   mods & PERIPHERAL_DIGITAL_DOWN);
}

static smpc_peripheral_digital_t dig_state;
static smpc_peripheral_analog_t  ana_state;

void Gamepads_Process(float delta) {
	int port = Gamepad_Connect(0x5A, saturn_defaults);
	smpc_peripheral_process();

	smpc_peripheral_digital_port(1, &dig_state);
	ProcessButtons(port, dig_state.pressed.raw | dig_state.held.raw);
	
	smpc_peripheral_analog_port(1, &ana_state);
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
static const vdp2_vram_cycp_t vram_cycp = {
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
		
void Window_Create2D(int width, int height) {
	Window_Main.Is3D = false;

	const vdp2_scrn_bitmap_format_t format = {
		.scroll_screen = VDP2_SCRN_NBG0,
		.ccc           = VDP2_SCRN_CCC_RGB_32768,
		.bitmap_size   = VDP2_SCRN_BITMAP_SIZE_512X256,
		.palette_base  = 0x00000000,
		.bitmap_base   = VDP2_VRAM_ADDR(0, 0x00000)
	};

	vdp2_scrn_bitmap_format_set(&format);
	vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 5);
	vdp2_scrn_display_set(VDP2_SCRN_DISP_NBG0);
	
	vdp2_vram_cycp_set(&vram_cycp);
}

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	// bottom half of VRAM is allocated for the 512x256 16 bit per pixel "bitmap"
	// but since only draw 224 rows anyways, can move the launcher framebuffer 
	// slightly into the end of the bottom half of VRAM
	bmp->scan0  = (BitmapCol*)VDP2_VRAM_ADDR(1, SCREEN_HEIGHT * 512);
	bmp->width  = width;
	bmp->height = height;
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
			vram[x + (y * 512)].raw = row[x];
		}
	}

	vdp2_sync();
	vdp2_sync_wait();
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
}


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { /* TODO implement */ }
void OnscreenKeyboard_SetText(const cc_string* text) { }
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
