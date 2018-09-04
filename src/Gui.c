#include "Gui.h"
#include "Window.h"
#include "Game.h"
#include "GraphicsCommon.h"
#include "GraphicsAPI.h"
#include "Event.h"
#include "Drawer2D.h"
#include "ExtMath.h"
#include "Screens.h"
#include "Camera.h"
#include "InputHandler.h"
#include "ErrorHandler.h"
#include "Platform.h"

struct Screen* Gui_Status;
void Gui_DefaultRecreate(void* elem) {
	struct GuiElem* e = elem;
	Elem_Free(e); Elem_Init(e);
}

bool Gui_DefaultMouse(void* elem, Int32 x, Int32 y, MouseButton btn) { return false; }
bool Gui_DefaultKey(void* elem, Key key)                { return false; }
bool Gui_DefaultKeyPress(void* elem, char keyChar)     { return false; }
bool Gui_DefaultMouseMove(void* elem, Int32 x, Int32 y) { return false; }
bool Gui_DefaultMouseScroll(void* elem, Real32 delta)   { return false; }

void Screen_CommonInit(void* screen) { 
	struct Screen* s = screen;
	Event_RegisterVoid(&GfxEvents_ContextLost,      s, s->VTABLE->ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, s, s->VTABLE->ContextRecreated);

	if (Gfx_LostContext) return;
	s->VTABLE->ContextRecreated(s);
}

void Screen_CommonFree(void* screen) { struct Screen* s = screen;
	Event_UnregisterVoid(&GfxEvents_ContextLost,      s, s->VTABLE->ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, s, s->VTABLE->ContextRecreated);
	s->VTABLE->ContextLost(s);
}

void Widget_SetLocation(void* widget, UInt8 horAnchor, UInt8 verAnchor, Int32 xOffset, Int32 yOffset) { 
	struct Widget* w = widget;
	w->HorAnchor = horAnchor; w->VerAnchor = verAnchor;
	w->XOffset   = xOffset;   w->YOffset = yOffset;
	Widget_Reposition(w);
}

void Widget_CalcPosition(void* widget) {
	struct Widget* w = widget;
	w->X = Gui_CalcPos(w->HorAnchor, w->XOffset, w->Width , Game_Width );
	w->Y = Gui_CalcPos(w->VerAnchor, w->YOffset, w->Height, Game_Height);
}

void Widget_Reset(void* widget) {
	struct Widget* w = widget;
	w->Active = false;
	w->Disabled = false;
	w->X = 0; w->Y = 0;
	w->Width = 0; w->Height = 0;
	w->HorAnchor = ANCHOR_MIN;
	w->VerAnchor = ANCHOR_MIN;
	w->XOffset = 0; w->YOffset = 0;
	w->MenuClick = NULL;
}

bool Widget_Contains(void* widget, Int32 x, Int32 y) { 
	struct Widget* w = widget;
	return Gui_Contains(w->X, w->Y, w->Width, w->Height, x, y);
}


Int32 Gui_CalcPos(UInt8 anchor, Int32 offset, Int32 size, Int32 axisLen) {
	if (anchor == ANCHOR_MIN)     return offset;
	if (anchor == ANCHOR_MAX) return axisLen - size - offset;
	return (axisLen - size) / 2 + offset;
}

bool Gui_Contains(Int32 recX, Int32 recY, Int32 width, Int32 height, Int32 x, Int32 y) {
	return x >= recX && y >= recY && x < recX + width && y < recY + height;
}

static void Gui_FileChanged(void* obj, struct Stream* stream, String* name) {
	if (String_CaselessEqualsConst(name, "gui.png")) {
		Game_UpdateTexture(&Gui_GuiTex, stream, name, NULL);
	} else if (String_CaselessEqualsConst(name, "gui_classic.png")) {
		Game_UpdateTexture(&Gui_GuiClassicTex, stream, name, NULL);
	} else if (String_CaselessEqualsConst(name, "icons.png")) {
		Game_UpdateTexture(&Gui_IconsTex, stream, name, NULL);
	}
}

static void Gui_Init(void) {
	Event_RegisterEntry(&TextureEvents_FileChanged, NULL, Gui_FileChanged);
	Gui_Status = StatusScreen_MakeInstance();
	Gui_HUD = HUDScreen_MakeInstance();

	struct IGameComponent comp; IGameComponent_MakeEmpty(&comp);
	StatusScreen_MakeComponent(&comp); Game_AddComponent(&comp);
	HUDScreen_MakeComponent(&comp);    Game_AddComponent(&comp);
}

static void Gui_Reset(void) {
	Int32 i;
	for (i = 0; i < Gui_OverlaysCount; i++) {
		Elem_TryFree(Gui_Overlays[i]);
	}
	Gui_OverlaysCount = 0;
}

static void Gui_Free(void) {
	Event_UnregisterEntry(&TextureEvents_FileChanged, NULL, Gui_FileChanged);
	Gui_ReplaceActive(NULL);
	Elem_TryFree(Gui_Status);

	if (Gui_Active) { Elem_TryFree(Gui_Active); }
	Gfx_DeleteTexture(&Gui_GuiTex);
	Gfx_DeleteTexture(&Gui_GuiClassicTex);
	Gfx_DeleteTexture(&Gui_IconsTex);
	Gui_Reset();
}

void Gui_MakeComponent(struct IGameComponent* comp) {
	comp->Init  = Gui_Init;
	comp->Reset = Gui_Reset;
	comp->Free  = Gui_Free;
}

struct Screen* Gui_GetActiveScreen(void) {
	return Gui_OverlaysCount ? Gui_Overlays[0] : Gui_GetUnderlyingScreen();
}

struct Screen* Gui_GetUnderlyingScreen(void) {
	return Gui_Active == NULL ? Gui_HUD : Gui_Active;
}

void Gui_ReplaceActive(struct Screen* screen) { 
	Gui_FreeActive();
	Gui_SetActive(screen);
}
void Gui_FreeActive(void) {
	if (Gui_Active) { Elem_TryFree(Gui_Active); }
}

void Gui_SetActive(struct Screen* screen) {
	InputHandler_ScreenChanged(Gui_Active, screen);
	if (screen) { Elem_Init(screen); }

	Gui_Active = screen;
	Gui_CalcCursorVisible();
}
void Gui_RefreshHud(void) { Elem_Recreate(Gui_HUD); }

void Gui_ShowOverlay(struct Screen* overlay, bool atFront) {
	if (Gui_OverlaysCount == GUI_MAX_OVERLAYS) {
		ErrorHandler_Fail("Gui_ShowOverlay - hit max count");
	}

	if (atFront) {
		Int32 i;
		/* Insert overlay at start of list */
		for (i = Gui_OverlaysCount - 1; i > 0; i--) {
			Gui_Overlays[i] = Gui_Overlays[i - 1];
		}
		Gui_Overlays[0] = overlay;
	} else {
		Gui_Overlays[Gui_OverlaysCount] = overlay;
	}
	Gui_OverlaysCount++;

	Elem_Init(overlay);
	Gui_CalcCursorVisible();
}

Int32 Gui_IndexOverlay(void* overlay) {
	struct Screen* s = overlay;
	Int32 i;
	for (i = 0; i < Gui_OverlaysCount; i++) {
		if (Gui_Overlays[i] == s) return i;
	}
	return -1;
}

void Gui_FreeOverlay(void* overlay) {
	struct Screen* s = overlay;
	Elem_Free(s);
	Int32 i = Gui_IndexOverlay(overlay);
	if (i == -1) return;

	for (; i < Gui_OverlaysCount - 1; i++) {
		Gui_Overlays[i] = Gui_Overlays[i + 1];
	}

	Gui_OverlaysCount--;
	Gui_Overlays[Gui_OverlaysCount] = NULL;
	Gui_CalcCursorVisible();
}

void Gui_RenderGui(Real64 delta) {
	GfxCommon_Mode2D(Game_Width, Game_Height);
	bool showHUD   = !Gui_Active || !Gui_Active->HidesHUD;
	bool hudBefore = !Gui_Active || !Gui_Active->RenderHUDOver;
	if (showHUD) { Elem_Render(Gui_Status, delta); }

	if (showHUD && hudBefore)  { Elem_Render(Gui_HUD, delta); }
	if (Gui_Active)            { Elem_Render(Gui_Active, delta); }
	if (showHUD && !hudBefore) { Elem_Render(Gui_HUD, delta); }

	if (Gui_OverlaysCount) { Elem_Render(Gui_Overlays[0], delta); }
	GfxCommon_Mode3D();
}

void Gui_OnResize(void) {
	if (Gui_Active) { Screen_OnResize(Gui_Active); }
	Screen_OnResize(Gui_HUD);

	Int32 i;
	for (i = 0; i < Gui_OverlaysCount; i++) {
		Screen_OnResize(Gui_Overlays[i]);
	}
}

bool gui_cursorVisible = true;
void Gui_CalcCursorVisible(void) {
	bool vis = Gui_GetActiveScreen()->HandlesAllInput;
	if (vis == gui_cursorVisible) return;
	gui_cursorVisible = vis;

	Window_SetCursorVisible(vis);
	if (Window_Focused)
		Camera_Active->RegrabMouse();
}


void TextAtlas_Make(struct TextAtlas* atlas, STRING_PURE String* chars, struct FontDesc* font, STRING_PURE String* prefix) {
	struct DrawTextArgs args; DrawTextArgs_Make(&args, prefix, font, true);
	struct Size2D size = Drawer2D_MeasureText(&args);
	atlas->Offset = size.Width;
	atlas->FontSize = font->Size;
	size.Width += 16 * chars->length;

	Mem_Set(atlas->Widths, 0, sizeof(atlas->Widths));
	struct Bitmap bmp; Bitmap_AllocateClearedPow2(&bmp, size.Width, size.Height);
	Drawer2D_Begin(&bmp);
	{
		Drawer2D_DrawText(&args, 0, 0);
		Int32 i;
		for (i = 0; i < chars->length; i++) {
			args.Text = String_UNSAFE_Substring(chars, i, 1);
			atlas->Widths[i] = Drawer2D_MeasureText(&args).Width;
			Drawer2D_DrawText(&args, atlas->Offset + font->Size * i, 0);
		}
	}
	Drawer2D_End();

	Drawer2D_Make2DTexture(&atlas->Tex, &bmp, size, 0, 0);
	Mem_Free(bmp.Scan0);

	Drawer2D_ReducePadding_Tex(&atlas->Tex, Math_Floor(font->Size), 4);
	atlas->uScale = 1.0f / (Real32)bmp.Width;
	atlas->Tex.U2 = atlas->Offset * atlas->uScale;
	atlas->Tex.Width = atlas->Offset;	
}

void TextAtlas_Free(struct TextAtlas* atlas) { Gfx_DeleteTexture(&atlas->Tex.ID); }

void TextAtlas_Add(struct TextAtlas* atlas, Int32 charI, VertexP3fT2fC4b** vertices) {
	Int32 width = atlas->Widths[charI];
	struct Texture part = atlas->Tex;

	part.X = atlas->CurX; part.Width = width;
	part.U1 = (atlas->Offset + charI * atlas->FontSize) * atlas->uScale;
	part.U2 = part.U1 + width * atlas->uScale;

	atlas->CurX += width;
	PackedCol white = PACKEDCOL_WHITE;
	GfxCommon_Make2DQuad(&part, white, vertices);
}

void TextAtlas_AddInt(struct TextAtlas* atlas, Int32 value, VertexP3fT2fC4b** vertices) {
	if (value < 0) {
		TextAtlas_Add(atlas, 10, vertices); value = -value; /* - sign */
	}

	char digits[STRING_INT_CHARS];
	Int32 i, count = String_MakeUInt32((UInt32)value, digits);
	for (i = count - 1; i >= 0; i--) {
		TextAtlas_Add(atlas, digits[i] - '0' , vertices);
	}
}
