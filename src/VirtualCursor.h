#include "Core.h"
#include "Funcs.h"
#include "Drawer2D.h"
#include "Input.h"
#include "LBackend.h"
static cc_bool vc_hooked;

#define CURSOR_SIZE   1
#define CURSOR_EXTENT 5

static void VirtualCursor_Display2D(struct Context2D* ctx) {
	int x, y;
	LBackend_MarkAllDirty();

	x = Pointers[0].x;
	y = Pointers[0].y;
	
	Context2D_Clear(ctx, BITMAPCOLOR_WHITE,
					x - CURSOR_EXTENT, y - CURSOR_SIZE, CURSOR_EXTENT * 2, CURSOR_SIZE * 3);
	Context2D_Clear(ctx, BITMAPCOLOR_WHITE,
					x - CURSOR_SIZE, y - CURSOR_EXTENT, CURSOR_SIZE * 3, CURSOR_EXTENT * 2);
}

static void VirtualCursor_SetPosition(int x, int y) {
	x = max(0, min(x, Window_Main.Width  - 1));
	y = max(0, min(y, Window_Main.Height - 1));
	vc_hooked = true;

	if (x == Pointers[0].x && y == Pointers[0].y) return;
	Pointer_SetPosition(0, x, y);
	LBackend_Hooks[3] = VirtualCursor_Display2D;
	
	/* TODO better dirty region tracking */
	if (launcherMode) LBackend_Redraw();
}

