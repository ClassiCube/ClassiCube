#include "Core.h"
#include "Funcs.h"
#include "Drawer2D.h"
#include "Utils.h"
#include "LBackend.h"
#include "Window.h"
#include "SystemFonts.h"

static cc_bool dlg_close;
/* Prevent normal input handling from seeing next button press */
static cc_bool VirtualDialog_InputHook(int key, struct InputDevice* device) { return true; }

static void VirtualDialog_OnInputDown(void* obj, int key, cc_bool was, struct InputDevice* device) {
	Platform_LogConst("AAAA");
	dlg_close = true;
}

struct LogPosition { struct Bitmap* bmp; int x, y; };
#define BEG_X 10

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
	pos.x   = BEG_X;
	pos.y   = y;
	
	while (*msg) 
	{
		if (pos.x + 20 >= bmp->width) {
			pos.x = BEG_X;
			pos.y += 20;
		}

		pos.x += FallbackFont_Plot((cc_uint8)*msg, PlotOnscreen, 1, &pos);
		msg++;
	}
	return pos.y;
}

#define DLG_TICK_INTERVAL 50
static void VirtualDialog_Show(const char* title, const char* msg) {
	struct Bitmap bmp;
	Platform_LogConst(title);
	Platform_LogConst(msg);

	if (!LBackend_FB.bmp.scan0) {
		Window_AllocFramebuffer(&bmp, Window_Main.Width, Window_Main.Height);
	} else {
		bmp = LBackend_FB.bmp;
	}

	struct Context2D ctx;
	Context2D_Wrap(&ctx, &bmp);
	Context2D_Clear(&ctx, BitmapCol_Make(50, 50, 50, 255),
					0, 0, bmp.width, bmp.height);

	const char* ipt_msg = "Press any button to continue";
	dlg_close = false;
	int y = 30;

	y = DrawText(title, &bmp, y);
	y = DrawText(msg,   &bmp, y + 20);
	DrawText(ipt_msg,   &bmp, bmp.height - 30);

	Input_DownHook old_hook = Input.DownHook;
	Input.DownHook = VirtualDialog_InputHook;
	Event_Register_(&InputEvents.Down2, NULL, VirtualDialog_OnInputDown);

	Rect2D rect = { 0, 0, bmp.width, bmp.height };

	while (Window_Main.Exists && !dlg_close)
	{
		Window_ProcessEvents(1.0f / DLG_TICK_INTERVAL);
		Gamepads_Process(    1.0f / DLG_TICK_INTERVAL);

		Window_DrawFramebuffer(rect, &bmp);
		Thread_Sleep(DLG_TICK_INTERVAL);
	}

	Context2D_Clear(&ctx, BitmapCol_Make(40, 40, 40, 255),
					0, 0, bmp.width, bmp.height);
	Window_DrawFramebuffer(rect, &bmp);

	if (!LBackend_FB.bmp.scan0) {
		Window_FreeFramebuffer(&bmp);
	} else {
		LBackend_Redraw();
	}

	Input.DownHook = old_hook;
	Event_Unregister_(&InputEvents.Down2, NULL, VirtualDialog_OnInputDown);
}
