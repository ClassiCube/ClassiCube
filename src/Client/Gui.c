#include "Gui.h"
#include "Window.h"
#include "Game.h"
#include "GraphicsCommon.h"
#include "GraphicsAPI.h"
#include "Events.h"

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

void GuiElement_Init(GuiElement* elem) {
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

void Screen_Init(Screen* screen) {
	GuiElement_Init(&screen->Base);
	screen->HandlesAllInput = false;
	screen->BlocksWorld = false;
	screen->HidesHUD = false;
	screen->RenderHUDOver = false;
	screen->OnResize = NULL;
	screen->OnContextLost = NULL;
	screen->OnContextRecreated = NULL;
}

void Widget_DoReposition(Widget* w) {
	w->X = Gui_CalcPos(w->HorAnchor, w->XOffset, w->Width , Game_Width );
	w->Y = Gui_CalcPos(w->VerAnchor, w->YOffset, w->Height, Game_Height);
}

void Widget_Init(Widget* widget) {
	GuiElement_Init(&widget->Base);
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

void Gui_FileChanged(Stream* stream) {
	String gui        = String_FromConst("gui.png");
	String guiClassic = String_FromConst("gui_classic.png");
	String icons      = String_FromConst("icons.png");

	if (String_Equals(&stream->Name, &gui)) {
		Game_UpdateTexture(&Gui_GuiTex, stream, false);
	} else if (String_Equals(&stream->Name, &guiClassic)) {
		Game_UpdateTexture(&Gui_GuiClassicTex, stream, false);
	} else if (String_Equals(&stream->Name, &icons)) {
		Game_UpdateTexture(&Gui_IconsTex, stream, false);
	}
}

void Gui_Init(void) {
	Event_RegisterStream(&TextureEvents_FileChanged, Gui_FileChanged);
	/* TODO: Init Gui_Status, Gui_HUD*/
}

void Gui_Reset(void) {
	UInt32 i;
	for (i = 0; i < Gui_OverlayCount; i++) {
		Gui_Overlays[i]->Base.Free(&Gui_Overlays[i]->Base);
	}
	Gui_OverlayCount = 0;
}

void Gui_Free(void) {
	Event_UnregisterStream(&TextureEvents_FileChanged, Gui_FileChanged);
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
	comp.Init = Gui_Init;
	comp.Reset = Gui_Reset;
	comp.Free = Gui_Free;
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
		hudScreen.GainFocus();
	} else if (Gui_Active == NULL) {
		hudScreen.LoseFocus();
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