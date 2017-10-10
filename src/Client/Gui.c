#include "Gui.h"
#include "Window.h"
#include "Game.h"
#include "GraphicsCommon.h"
#include "GraphicsAPI.h"
#include "Events.h"

void GuiElement_Fail(void) {

}

bool GuiElement_ReturnFalse(void) {
	return false;
}
void GuiElement_Init(GuiElement* elem) {
	/* Initalises state of this GUI element */
	Gui_Void Init;
	/* Draws this gui element on screen */
	Gui_Render Render;
	/* Frees the state of this GUI element */
	Gui_Void Free;
	/* Recreates all sub-elements and/or textures. (e.g. for when bitmap font changes) */
	Gui_Void Recreate;
	/* Returns whether this GUI element handles a key being pressed. */
	Gui_KeyHandler HandlesKeyDown;
	/* Returns whether this GUI element handles a key being released. */
	Gui_KeyHandler HandlesKeyUp;
	/* Returns whether this GUI element handles a character being input */
	Gui_KeyPress HandlesKeyPress;
	/* Returns whether this GUI element handles a mouse button being pressed. */
	Gui_MouseHandler HandlesMouseDown;
	/* Returns whether this GUI element handles a mouse button being released. */
	Gui_MouseHandler HandlesMouseUp;
	/* Returns whether this GUI element handles the mouse being moved. */
	Gui_MouseMove HandlesMouseMove;
	/* Returns whether this GUI element handles the mouse being scrolled. */
	Gui_MouseScroll HandlesMouseScroll;
}

void Screen_Init(Screen* screen) {
	GuiElement_Init(&screen->Base);
	screen->HandlesAllInput = false;
	screen->BlocksWorld = false;
	screen->HidesHUD = false;
	screen->RenderHUDOver = false;
	screen->
}

void Widget_Init(Widget* widget) {

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
	String gui        = String_FromConstant("gui.png");
	String guiClassic = String_FromConstant("gui_classic.png");
	String icons      = String_FromConstant("icons.png");

	if (String_Equals(&stream->Name, &gui)) {
		Game_UpdateTexture(&Gui_GuiTex, stream, false);
	} else if (String_Equals(&stream->Name, &guiClassic)) {
		Game_UpdateTexture(&Gui_GuiClassicTex, stream, false);
	} else if (String_Equals(&stream->Name, &icons)) {
		Game_UpdateTexture(&Gui_IconsTex, stream, false);
	}
}

void Gui_Init() {
	Event_RegisterStream(&TextureEvents_FileChanged, Gui_FileChanged);
}

void Gui_Reset(void) {
	UInt32 i;
	for (i = 0; i < Gui_OverlayCount; i++) {
		overlays[i].Dispose();
		Gui_Overlays[i]->Base.Free(Gui_Overlays[i]);
	}
	Gui_OverlayCount = 0;
}

void Gui_Free(void) {
	Event_UnregisterStream(&TextureEvents_FileChanged, Gui_FileChanged);
	Gui_SetScreen(NULL);
	statusScreen.Dispose();

	if (Gui_Active != null) {
		Gui_Active->Base.Free(Gui_Active);
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

void Gui_SetScreen(Screen* screen) {
	game.Input.ScreenChanged(activeScreen, screen);
	if (activeScreen != NULL && disposeOld)
		activeScreen.Dispose();

	if (screen == NULL) {
		hudScreen.GainFocus();
	} else if (activeScreen == null) {
		hudScreen.LoseFocus();
	}

	if (screen != null)
		screen.Init();
	activeScreen = screen;
}

void Gui_RefreshHud(void) {
	Gui_HUD->Base.Recreate(Gui_HUD);
}

void Gui_ShowOverlay(Screen* overlay) 
bool cursorVis = game.CursorVisible;
if (overlays.Count == 0) game.CursorVisible = true;
overlays.Add(overlay);
if (overlays.Count == 1) game.CursorVisible = cursorVis;
// Save cursor visibility state
overlay.Init();
}

void Gui_RenderGui(Real64 delta) {
	GfxCommon_Mode2D(Game_Width, Game_Height);
	if (Gui_Active == NULL || !Gui_Active->HidesHUD) {
		statusScreen.Render(delta)
	}

	if (Gui_Active == NULL || !Gui_Active->HidesHUD && !Gui_Active->RenderHUDOver) {
		Gui_HUD->Base.Render(Gui_HUD, delta);
	}
	if (Gui_Active != NULL) {
		Gui_Active->Base.Render(Gui_Active, delta);
	}
	if (Gui_Active != NULL && !Gui_Active->HidesHUD && Gui_Active->RenderHUDOver) {
		Gui_HUD->Base.Render(Gui_HUD, delta);
	}

	if (Gui_OverlayCount > 0) {
		Gui_Overlays[0]->Base.Render(Gui_Overlays[0], delta);
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