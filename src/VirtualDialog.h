#include "Core.h"
#include "Funcs.h"
#include "Drawer2D.h"
#include "Utils.h"
#include "LBackend.h"
#include "Window.h"
#include "SystemFonts.h"

static cc_bool vd_close;
static short vd_lineSpace;

/* Prevent normal input handling from seeing next button press */
static cc_bool VirtualDialog_InputHook(int key, struct InputDevice* device) { return true; }

static void VirtualDialog_OnInputDown(void* obj, int key, cc_bool was, struct InputDevice* device) {
	vd_close = true;
}

struct LogPosition { struct Bitmap* bmp; int x, y; };
#define VD_BEG_X      10

static void PlotOnscreen(int x, int y, void* ctx) {
	struct LogPosition* pos = ctx;
	x += pos->x;
	y += pos->y;
	if (x >= pos->bmp->width || y >= pos->bmp->height) return;
	
	BitmapCol* row = Bitmap_GetRow(pos->bmp, y);
	row[x] = BITMAPCOLOR_WHITE;
}

static int DrawText(const char* msg, struct Bitmap* bmp, int y) {
	struct LogPosition pos;
	pos.bmp = bmp;
	pos.x   = VD_BEG_X;
	pos.y   = y;

	int scale = max(1, 2 * DisplayInfo.ScaleY);
	
	while (*msg) 
	{
		char c = *msg;
		if (pos.x + 20 >= bmp->width || c == '\n') {
			pos.x = VD_BEG_X;
			pos.y += vd_lineSpace;
		}

		if (c != '\n') { pos.x += FallbackFont_Plot((cc_uint8)c, PlotOnscreen, scale, &pos); }
		msg++;
	}
	return pos.y;
}

#define VD_TICK_INTERVAL 50
#define VD_MAX_ITERS (5000 / VD_TICK_INTERVAL)

void VirtualDialog_Show(const char* title, const char* msg, cc_bool oneshot) {
	struct Bitmap bmp;
	Platform_LogConst(title);
	Platform_LogConst(msg);

	if (!LBackend_FB.bmp.scan0) {
		Window_AllocFramebuffer(&bmp, DisplayInfo.Width, DisplayInfo.Height);
	} else {
		bmp = LBackend_FB.bmp;
	}

	struct Context2D ctx;
	Context2D_Wrap(&ctx, &bmp);
	// TODO Backing surface may be bigger then valid area
	Context2D_Clear(&ctx, BitmapCol_Make(30, 30, 30, 255),
					0, 0, bmp.width, bmp.height);

	const char* ipt_msg = "Press any button to continue (or wait 5 seconds)";
	vd_close = false;

	vd_lineSpace = 20 * DisplayInfo.ScaleY;
	int y = max(30, bmp.height / 2 - 50);

	y = DrawText(title, &bmp, y);
	y = DrawText(msg,   &bmp, y + vd_lineSpace);

	if (!oneshot) { DrawText(ipt_msg, &bmp, bmp.height - vd_lineSpace * 2); }

	Input_DownHook old_hook = Input.DownHook;
	Input.DownHook = VirtualDialog_InputHook;
	Event_Register_(&InputEvents.Down2, NULL, VirtualDialog_OnInputDown);

	Rect2D rect = { 0, 0, bmp.width, bmp.height };
	cc_bool has_window = Window_Main.Exists;

	for (int i = 0; !vd_close && i < VD_MAX_ITERS; i++)
	{
		Window_ProcessEvents(1.0f / VD_TICK_INTERVAL);
		Gamepads_Process(    1.0f / VD_TICK_INTERVAL);

		Window_DrawFramebuffer(rect, &bmp);
		Thread_Sleep(VD_TICK_INTERVAL);
		if (oneshot) break;

		// Window_ProcessEvents processed an app exit event
		if (has_window && !Window_Main.Exists) break;
	}

	if (!oneshot) {
		Context2D_Clear(&ctx, BitmapCol_Make(20, 20, 20, 255),
						0, 0, bmp.width, bmp.height);
		Window_DrawFramebuffer(rect, &bmp);
	}

	if (!LBackend_FB.bmp.scan0) {
		Window_FreeFramebuffer(&bmp);
	} else {
		LBackend_Redraw();
	}

	Input.DownHook = old_hook;
	Event_Unregister_(&InputEvents.Down2, NULL, VirtualDialog_OnInputDown);
}
