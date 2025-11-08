#include "Core.h"
#include "Funcs.h"
#include "Drawer2D.h"
#include "Input.h"
#include "LBackend.h"
#include "Graphics.h"
#include "Game.h"

static cc_bool vc_hooked;
static struct Texture vc_texture;
static GfxResourceID  vc_vb;

#define CURSOR_SIZE   1
#define CURSOR_EXTENT 5

static void VirtualCursor_Draw(struct Context2D* ctx, int x, int y) {
	Context2D_Clear(ctx, BITMAPCOLOR_WHITE,
					x - CURSOR_EXTENT, y - CURSOR_SIZE, CURSOR_EXTENT * 2, CURSOR_SIZE * 3);
	Context2D_Clear(ctx, BITMAPCOLOR_WHITE,
					x - CURSOR_SIZE, y - CURSOR_EXTENT, CURSOR_SIZE * 3, CURSOR_EXTENT * 2);
}

static void VirtualCursor_Display2D(void) {
	LBackend_MarkAllDirty();
	VirtualCursor_Draw(&LBackend_FB, Pointers[0].x, Pointers[0].y);
}

static void VirtualCursor_MakeTexture(void) {
	struct Context2D ctx;
	Context2D_Alloc(&ctx, CURSOR_EXTENT * 2, CURSOR_EXTENT * 2);
	{
		VirtualCursor_Draw(&ctx, CURSOR_EXTENT, CURSOR_EXTENT);
		Context2D_MakeTexture(&vc_texture, &ctx);
	}
	Context2D_Free(&ctx);
}

/* TODO hook into context lost etc */
static void VirtualCursor_Display3D(float delta) {
	if (!DisplayInfo.CursorVisible) return;
	
	if (!vc_vb) {
		vc_vb = Gfx_CreateDynamicVb(VERTEX_FORMAT_TEXTURED, 4);
		if (!vc_vb) return;
	}	
	
	if (!vc_texture.ID) {
		VirtualCursor_MakeTexture();
		if (!vc_texture.ID) return;
	}
	
	vc_texture.x = Pointers[0].x - CURSOR_EXTENT;
	vc_texture.y = Pointers[0].y - CURSOR_EXTENT;
	
	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_BindTexture(vc_texture.ID);
	
	struct VertexTextured* data = (struct VertexTextured*)Gfx_LockDynamicVb(vc_vb, VERTEX_FORMAT_TEXTURED, 4);
	struct VertexTextured** ptr = &data;
	Gfx_Make2DQuad(&vc_texture, PACKEDCOL_WHITE, ptr);
	Gfx_UnlockDynamicVb(vc_vb);
	Gfx_DrawVb_IndexedTris(4);
}

static void VirtualCursor_SetPosition(int x, int y) {
	x = max(0, min(x, Window_Main.Width  - 1));
	y = max(0, min(y, Window_Main.Height - 1));
	vc_hooked = true;

	if (x == Pointers[0].x && y == Pointers[0].y) return;
	Pointer_SetPosition(0, x, y);
	LBackend_Hooks[3]   = VirtualCursor_Display2D;
	Game.Draw2DHooks[3] = VirtualCursor_Display3D;
	
	/* TODO better dirty region tracking */
	if (!Window_Main.Is3D) LBackend_Redraw();
}

