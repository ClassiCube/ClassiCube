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
void GuiElement_Recreate(struct GuiElem* elem) {
	elem->VTABLE->Free(elem);
	elem->VTABLE->Init(elem);
}

bool Gui_FalseMouse(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn) { return false; }
bool Gui_FalseKey(struct GuiElem* elem, Key key)                { return false; }
bool Gui_FalseKeyPress(struct GuiElem* elem, UInt8 keyChar)     { return false; }
bool Gui_FalseMouseMove(struct GuiElem* elem, Int32 x, Int32 y) { return false; }
bool Gui_FalseMouseScroll(struct GuiElem* elem, Real32 delta)   { return false; }

void GuiElement_Reset(struct GuiElem* elem) {
	elem->VTABLE->Init     = NULL;
	elem->VTABLE->Render   = NULL;
	elem->VTABLE->Free     = NULL;
	elem->VTABLE->Recreate = GuiElement_Recreate;

	elem->VTABLE->HandlesKeyDown     = Gui_FalseKey;
	elem->VTABLE->HandlesKeyUp       = Gui_FalseKey;
	elem->VTABLE->HandlesKeyPress    = Gui_FalseKeyPress;

	elem->VTABLE->HandlesMouseDown   = Gui_FalseMouse;
	elem->VTABLE->HandlesMouseUp     = Gui_FalseMouse;
	elem->VTABLE->HandlesMouseMove   = Gui_FalseMouseMove;
	elem->VTABLE->HandlesMouseScroll = Gui_FalseMouseScroll;
}

void Screen_Reset(struct Screen* screen) {
	GuiElement_Reset((struct GuiElem*)screen);
	screen->HandlesAllInput = true;
	screen->BlocksWorld     = false;
	screen->HidesHUD        = false;
	screen->RenderHUDOver   = false;
	screen->OnResize        = NULL;
}

void Widget_DoReposition(struct GuiElem* elem) {
	struct Widget* w = (struct Widget*)elem;
	w->X = Gui_CalcPos(w->HorAnchor, w->XOffset, w->Width , Game_Width );
	w->Y = Gui_CalcPos(w->VerAnchor, w->YOffset, w->Height, Game_Height);
}

void Widget_Init(struct Widget* widget) {
	GuiElement_Reset((struct GuiElem*)widget);
	widget->Active = false;
	widget->Disabled = false;
	widget->X = 0; widget->Y = 0;
	widget->Width = 0; widget->Height = 0;
	widget->HorAnchor = ANCHOR_MIN;
	widget->VerAnchor = ANCHOR_MIN;
	widget->XOffset = 0; widget->YOffset = 0;
	widget->Reposition = Widget_DoReposition;
	widget->MenuClick = NULL;
}

bool Widget_Contains(struct Widget* widget, Int32 x, Int32 y) {
	return Gui_Contains(widget->X, widget->Y, widget->Width, widget->Height, x, y);
}


Int32 Gui_CalcPos(UInt8 anchor, Int32 offset, Int32 size, Int32 axisLen) {
	if (anchor == ANCHOR_MIN)     return offset;
	if (anchor == ANCHOR_MAX) return axisLen - size - offset;
	return (axisLen - size) / 2 + offset;
}

bool Gui_Contains(Int32 recX, Int32 recY, Int32 width, Int32 height, Int32 x, Int32 y) {
	return x >= recX && y >= recY && x < recX + width && y < recY + height;
}

static void Gui_FileChanged(void* obj, struct Stream* stream) {
	if (String_CaselessEqualsConst(&stream->Name, "gui.png")) {
		Game_UpdateTexture(&Gui_GuiTex, stream, NULL);
	} else if (String_CaselessEqualsConst(&stream->Name, "gui_classic.png")) {
		Game_UpdateTexture(&Gui_GuiClassicTex, stream, NULL);
	} else if (String_CaselessEqualsConst(&stream->Name, "icons.png")) {
		Game_UpdateTexture(&Gui_IconsTex, stream, NULL);
	}
}

static void Gui_Init(void) {
	Event_RegisterStream(&TextureEvents_FileChanged, NULL, Gui_FileChanged);
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
	Event_UnregisterStream(&TextureEvents_FileChanged, NULL, Gui_FileChanged);
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
		ErrorHandler_Fail("Cannot have more than 6 overlays");
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

Int32 Gui_IndexOverlay(struct Screen* overlay) {
	Int32 i;
	for (i = 0; i < Gui_OverlaysCount; i++) {
		if (Gui_Overlays[i] == overlay) return i;
	}
	return -1;
}

void Gui_FreeOverlay(struct Screen* overlay) {
	Int32 i = Gui_IndexOverlay(overlay);
	Elem_Free(overlay);
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
	if (Gui_Active) {
		Gui_Active->OnResize((struct GuiElem*)Gui_Active);
	}
	Gui_HUD->OnResize((struct GuiElem*)Gui_HUD);

	Int32 i;
	for (i = 0; i < Gui_OverlaysCount; i++) {
		Gui_Overlays[i]->OnResize((struct GuiElem*)Gui_Overlays[i]);
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
	Mem_Free(&bmp.Scan0);

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

	UChar digits[STRING_INT_CHARS];
	Int32 i, count = String_MakeUInt32((UInt32)value, digits);
	for (i = count - 1; i >= 0; i--) {
		TextAtlas_Add(atlas, digits[i] - '0' , vertices);
	}
}
