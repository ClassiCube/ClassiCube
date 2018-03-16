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

Screen* Gui_Status;
void GuiElement_Recreate(GuiElement* elem) {
	elem->Free(elem);
	elem->Init(elem);
}

bool Gui_FalseKey(GuiElement* elem, Key key) { return false; }
bool Gui_FalseKeyPress(GuiElement* elem, UInt8 keyChar) { return false; }
bool Gui_FalseMouse(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) { return false; }
bool Gui_FalseMouseMove(GuiElement* elem, Int32 x, Int32 y) { return false; }
bool Gui_FalseMouseScroll(GuiElement* elem, Real32 delta) { return false; }

void GuiElement_Reset(GuiElement* elem) {
	elem->Init     = NULL;
	elem->Render   = NULL;
	elem->Free     = NULL;
	elem->Recreate = GuiElement_Recreate;

	elem->HandlesKeyDown     = Gui_FalseKey;
	elem->HandlesKeyUp       = Gui_FalseKey;
	elem->HandlesKeyPress    = Gui_FalseKeyPress;
	elem->HandlesMouseDown   = Gui_FalseMouse;
	elem->HandlesMouseUp     = Gui_FalseMouse;
	elem->HandlesMouseMove   = Gui_FalseMouseMove;
	elem->HandlesMouseScroll = Gui_FalseMouseScroll;
}

void Screen_Reset(Screen* screen) {
	GuiElement_Reset(&screen->Base);
	screen->HandlesAllInput = false;
	screen->BlocksWorld     = false;
	screen->HidesHUD        = false;
	screen->RenderHUDOver   = false;
	screen->OnResize        = NULL;
}

void Widget_DoReposition(Widget* w) {
	w->X = Gui_CalcPos(w->HorAnchor, w->XOffset, w->Width , Game_Width );
	w->Y = Gui_CalcPos(w->VerAnchor, w->YOffset, w->Height, Game_Height);
}

void Widget_Init(Widget* widget) {
	GuiElement_Reset(&widget->Base);
	widget->Active = false;
	widget->Disabled = false;
	widget->Base.HandlesMouseDown = NULL;
	widget->X = 0; widget->Y = 0;
	widget->Width = 0; widget->Height = 0;
	widget->HorAnchor = ANCHOR_LEFT_OR_TOP;
	widget->VerAnchor = ANCHOR_LEFT_OR_TOP;
	widget->XOffset = 0; widget->YOffset = 0;
	widget->Reposition = Widget_DoReposition;
}


Int32 Gui_CalcPos(Anchor anchor, Int32 offset, Int32 size, Int32 axisLen) {
	if (anchor == ANCHOR_LEFT_OR_TOP)     return offset;
	if (anchor == ANCHOR_BOTTOM_OR_RIGHT) return axisLen - size - offset;
	return (axisLen - size) / 2 + offset;
}

bool Gui_Contains(Int32 recX, Int32 recY, Int32 width, Int32 height, Int32 x, Int32 y) {
	return x >= recX && y >= recY && x < recX + width && y < recY + height;
}

void Gui_FileChanged(void* obj, Stream* stream) {
	String gui        = String_FromConst("gui.png");
	String guiClassic = String_FromConst("gui_classic.png");
	String icons      = String_FromConst("icons.png");

	if (String_CaselessEquals(&stream->Name, &gui)) {
		Game_UpdateTexture(&Gui_GuiTex, stream, false);
	} else if (String_CaselessEquals(&stream->Name, &guiClassic)) {
		Game_UpdateTexture(&Gui_GuiClassicTex, stream, false);
	} else if (String_CaselessEquals(&stream->Name, &icons)) {
		Game_UpdateTexture(&Gui_IconsTex, stream, false);
	}
}

void Gui_Init(void) {
	Event_RegisterStream(&TextureEvents_FileChanged, NULL, Gui_FileChanged);
	Gui_Status = StatusScreen_MakeInstance();
	Gui_HUD = HUDScreen_MakeInstance();

	IGameComponent comp;
	comp = StatusScreen_MakeComponent(); Game_AddComponent(&comp);
	comp = HUDScreen_MakeComponent();    Game_AddComponent(&comp);
}

void Gui_Reset(void) {
	UInt32 i;
	for (i = 0; i < Gui_OverlayCount; i++) {
		Gui_Overlays[i]->Base.Free(&Gui_Overlays[i]->Base);
	}
	Gui_OverlayCount = 0;
}

void Gui_Free(void) {
	Event_UnregisterStream(&TextureEvents_FileChanged, NULL, Gui_FileChanged);
	Gui_SetNewScreen(NULL);
	Gui_Status->Base.Free(&Gui_Status->Base);

	if (Gui_Active != NULL) {
		Gui_Active->Base.Free(&Gui_Active->Base);
	}
	Gfx_DeleteTexture(&Gui_GuiTex);
	Gfx_DeleteTexture(&Gui_GuiClassicTex);
	Gfx_DeleteTexture(&Gui_IconsTex);
	Gui_Reset();
}

IGameComponent Gui_MakeGameComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init  = Gui_Init;
	comp.Reset = Gui_Reset;
	comp.Free  = Gui_Free;
	return comp;
}

Screen* Gui_GetActiveScreen(void) {
	return Gui_OverlayCount > 0 ? Gui_Overlays[0] : Gui_GetUnderlyingScreen();
}

Screen* Gui_GetUnderlyingScreen(void) {
	return Gui_Active == NULL ? Gui_HUD : Gui_Active;
}

void Gui_SetScreen(Screen* screen, bool freeOld) {
	game.Input.ScreenChanged(Gui_Active, screen);
	if (Gui_Active != NULL && freeOld) {
		Gui_Active->Base.Free(&Gui_Active->Base);
	}

	if (screen == NULL) {
		Window_SetCursorVisible(false);
		if (Window_GetFocused()) { Camera_ActiveCamera->RegrabMouse(); }
	} else if (Gui_Active == NULL) {
		Window_SetCursorVisible(true);
	}

	if (screen != NULL) {
		screen->Base.Init(&screen->Base);
	}
	Gui_Active = screen;
}

void Gui_SetNewScreen(Screen* screen) { Gui_SetScreen(screen, true); }

void Gui_RefreshHud(void) {
	Gui_HUD->Base.Recreate(&Gui_HUD->Base);
}

void Gui_ShowOverlay(Screen* overlay) {
	if (Gui_OverlayCount == GUI_MAX_OVERLAYS) {
		ErrorHandler_Fail("Cannot have more than 40 overlays");
	}
	bool visible = Game_GetCursorVisible();
	if (Gui_OverlayCount == 0) Game_SetCursorVisible(true);

	Gui_Overlays[Gui_OverlayCount++] = overlay;
	if (Gui_OverlayCount == 1) Game_SetCursorVisible(visible); /* Save cursor visibility state */
	overlay->Base.Init(&overlay->Base);
}

void Gui_RenderGui(Real64 delta) {
	GfxCommon_Mode2D(Game_Width, Game_Height);
	if (Gui_Active == NULL || !Gui_Active->HidesHUD) {
		Gui_Status->Base.Render(&Gui_Status->Base, delta);
	}

	if (Gui_Active == NULL || !Gui_Active->HidesHUD && !Gui_Active->RenderHUDOver) {
		Gui_HUD->Base.Render(&Gui_HUD->Base, delta);
	}
	if (Gui_Active != NULL) {
		Gui_Active->Base.Render(&Gui_Active->Base, delta);
	}
	if (Gui_Active != NULL && !Gui_Active->HidesHUD && Gui_Active->RenderHUDOver) {
		Gui_HUD->Base.Render(&Gui_HUD->Base, delta);
	}

	if (Gui_OverlayCount > 0) {
		Gui_Overlays[0]->Base.Render(&Gui_Overlays[0]->Base, delta);
	}
	GfxCommon_Mode3D();
}

void Gui_OnResize(void) {
	if (Gui_Active != NULL) {
		Gui_Active->OnResize(Gui_Active);
	}
	Gui_HUD->OnResize(Gui_HUD);

	UInt32 i;
	for (i = 0; i < Gui_OverlayCount; i++) {
		Gui_Overlays[i]->OnResize(Gui_Overlays[i]);
	}
}


void TextAtlas_Make(TextAtlas* atlas, STRING_PURE String* chars, FontDesc* font, STRING_PURE String* prefix) {
	DrawTextArgs args; DrawTextArgs_Make(&args, prefix, font, true);
	Size2D size = Drawer2D_MeasureText(&args);
	atlas->Offset = size.Width;
	atlas->FontSize = font->Size;
	size.Width += 16 * chars->length;

	Platform_MemSet(atlas->Widths, 0, sizeof(atlas->Widths));
	Bitmap bmp; Bitmap_AllocatePow2(&bmp, size.Width, size.Height);
	Drawer2D_Begin(&bmp);
		
	Drawer2D_DrawText(&args, 0, 0);
	Int32 i;
	for (i = 0; i < chars->length; i++) {
		args.Text = String_UNSAFE_Substring(chars, i, 1);
		atlas->Widths[i] = Drawer2D_MeasureText(&args).Width;
		Drawer2D_DrawText(&args, atlas->Offset + font->Size * i, 0);
	}

	Drawer2D_End();
	atlas->Tex = Drawer2D_Make2DTexture(&bmp, size, 0, 0);
	Platform_MemFree(bmp.Scan0);

	Drawer2D_ReducePadding_Tex(&atlas->Tex, Math_Floor(font->Size), 4);
	atlas->Tex.U2 = (Real32)atlas->Offset / (Real32)bmp.Width;
	atlas->Tex.Width = (UInt16)atlas->Offset;
	atlas->TotalWidth = bmp.Width;
}

void TextAtlas_Free(TextAtlas* atlas) { Gfx_DeleteTexture(&atlas->Tex.ID); }

void TextAtlas_Add(TextAtlas* atlas, Int32 charI, VertexP3fT2fC4b** vertices) {
	Int32 width = atlas->Widths[charI];
	Texture part = atlas->Tex;
	part.X = (UInt16)atlas->CurX; part.Width = (UInt16)width;
	part.U1 = (atlas->Offset + charI * atlas->FontSize) / (Real32)atlas->TotalWidth;
	part.U2 = part.U1 + width / (Real32)atlas->TotalWidth;

	atlas->CurX += width;
	PackedCol white = PACKEDCOL_WHITE;
	GfxCommon_Make2DQuad(&part, white, vertices);
}

void TextAtlas_AddInt(TextAtlas* atlas, Int32 value, VertexP3fT2fC4b** vertices) {
	if (value < 0) TextAtlas_Add(atlas, 10, vertices); /* - sign */

	UInt32 i, count = 0;
	UInt8 digits[STRING_SIZE];
	/* use a do while loop here, as we still want a '0' digit if input is 0 */
	do {
		digits[count] = (UInt8)Math_AbsI(value % 10);
		value /= 10; count++;
	} while (value != 0);

	for (i = 0; i < count; i++) {
		TextAtlas_Add(atlas, digits[count - 1 - i], vertices);
	}
}