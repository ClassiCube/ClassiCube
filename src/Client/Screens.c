#include "Screens.h"
#include "Widgets.h"
#include "Game.h"
#include "Event.h"
#include "GraphicsCommon.h"
#include "Platform.h"
#include "Inventory.h"
#include "Drawer2D.h"
#include "GraphicsAPI.h"
#include "Player.h"

void Screen_FreeWidgets(Widget** widgets, UInt32 widgetsCount) {
	if (widgets == NULL) return;
	UInt32 i;
	for (i = 0; i < widgetsCount; i++) {
		if (widgets[i] == NULL) continue;
		widgets[i]->Base.Free(&widgets[i]->Base);
	}
}

void Screen_RepositionWidgets(Widget** widgets, UInt32 widgetsCount) {
	if (widgets == NULL) return;
	UInt32 i;
	for (i = 0; i < widgetsCount; i++) {
		if (widgets[i] == NULL) continue;
		widgets[i]->Reposition(widgets[i]);
	}
}

void Screen_RenderWidgets(Widget** widgets, UInt32 widgetsCount, Real64 delta) {
	if (widgets == NULL) return;

	UInt32 i;
	for (i = 0; i < widgetsCount; i++) {
		if (widgets[i] == NULL) continue;
		widgets[i]->Base.Render(&widgets[i]->Base, delta);
	}
}



/* These were sourced by taking a screenshot of vanilla
   Then using paint to extract the colour components
   Then using wolfram alpha to solve the glblendfunc equation */
PackedCol Menu_TopBackCol    = PACKEDCOL_CONST(24, 24, 24, 105);
PackedCol Menu_BottomBackCol = PACKEDCOL_CONST(51, 51, 98, 162);

void ClickableScreen_RenderMenuBounds(void) {
	GfxCommon_Draw2DGradient(0, 0, Game_Width, Game_Height, Menu_TopBackCol, Menu_BottomBackCol);
}

void ClickableScreen_DefaultWidgetSelected(GuiElement* elem, Widget* widget) { }

bool ClickableScreen_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	ClickableScreen* screen = (ClickableScreen*)elem;
	UInt32 i;
	/* iterate backwards (because last elements rendered are shown over others) */
	for (i = screen->WidgetsCount; i > 0; i--) {
		Widget* widget = screen->Widgets[i - 1];
		if (widget != NULL && Gui_Contains(widget->X, widget->Y, widget->Width, widget->Height, x, y)) {
			if (!widget->Disabled) {
				elem = &widget->Base;
				elem->HandlesMouseDown(elem, x, y, btn);
			}
			return true;
		}
	}
	return false;
}

bool ClickableScreens_HandleMouseMove(GuiElement* elem, Int32 x, Int32 y) {
	ClickableScreen* screen = (ClickableScreen*)elem;
	if (screen->LastX == x && screen->LastY == y) return true;
	UInt32 i;
	for (i = 0; i < screen->WidgetsCount; i++) {
		Widget* widget = screen->Widgets[i];
		if (widget != NULL) widget->Active = false;
	}

	for (i = screen->WidgetsCount; i > 0; i--) {
		Widget* widget = screen->Widgets[i];
		if (widget != NULL && Gui_Contains(widget->X, widget->Y, widget->Width, widget->Height, x, y)) {
			widget->Active = true;
			screen->LastX = x; screen->LastY = y;
			screen->OnWidgetSelected(elem, widget);
			return true;
		}
	}

	screen->LastX = x; screen->LastY = y;
	screen->OnWidgetSelected(elem, NULL);
	return false;
}

void ClickableScreen_Create(ClickableScreen* screen) {
	Screen_Reset(&screen->Base);
	screen->Base.Base.HandlesMouseDown = ClickableScreen_HandlesMouseDown;
	screen->Base.Base.HandlesMouseMove = ClickableScreens_HandleMouseMove;

	screen->LastX = -1; screen->LastY = -1;
	screen->OnWidgetSelected = ClickableScreen_DefaultWidgetSelected;
}


typedef struct InventoryScreen_ {
	Screen Base;
	FontDesc Font;
	TableWidget Table;
	bool ReleasedInv;
} InventoryScreen;
InventoryScreen InventoryScreen_Instance;

void InventoryScreen_OnBlockChanged(void) {
	TableWidget_OnInventoryChanged(&InventoryScreen_Instance.Table);
}

void InventoryScreen_ContextLost(void) {
	GuiElement* elem = &InventoryScreen_Instance.Table.Base.Base;
	elem->Free(elem);
}

void InventoryScreen_ContextRecreated(void) {
	GuiElement* elem = &InventoryScreen_Instance.Table.Base.Base;
	elem->Recreate(elem);
}

void InventoryScreen_Init(GuiElement* elem) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	FontDesc_Make(&screen->Font, 16, FONT_STYLE_NORMAL);
	Platform_MakeFont(&screen->Font, &Game_FontName);

	elem = &screen->Table.Base.Base;
	TableWidget_Create(&screen->Table);
	screen->Table.Font = screen->Font;
	screen->Table.ElementsPerRow = Game_PureClassic ? 9 : 10;
	elem->Init(elem);

	Key_KeyRepeat = true;
	Event_RegisterVoid(&BlockEvents_PermissionsChanged, InventoryScreen_OnBlockChanged);
	Event_RegisterVoid(&BlockEvents_BlockDefChanged,    InventoryScreen_OnBlockChanged);	
	Event_RegisterVoid(&GfxEvents_ContextLost,          InventoryScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated,     InventoryScreen_ContextRecreated);
}

void InventoryScreen_Render(GuiElement* elem, Real64 delta) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	elem = &screen->Table.Base.Base;
	elem->Render(elem, delta);
}

void InventoryScreen_OnResize(Screen* elem) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	Widget* widget = &screen->Table.Base;
	widget->Reposition(widget);
}

void InventoryScreen_Free(GuiElement* elem) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	Platform_FreeFont(&screen->Font);
	elem = &screen->Table.Base.Base;
	elem->Free(elem);

	Key_KeyRepeat = false;
	Event_UnregisterVoid(&BlockEvents_PermissionsChanged, InventoryScreen_OnBlockChanged);
	Event_UnregisterVoid(&BlockEvents_BlockDefChanged,    InventoryScreen_OnBlockChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost,          InventoryScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated,     InventoryScreen_ContextRecreated);
}

bool InventoryScreen_HandlesKeyDown(GuiElement* elem, Key key) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	TableWidget* table = &screen->Table;
	elem = &screen->Table.Base.Base;

	if (key == KeyBind_Get(KeyBind_PauseOrExit)) {
		Gui_SetNewScreen(NULL);
	} else if (key == KeyBind_Get(KeyBind_Inventory) && screen->ReleasedInv) {
		Gui_SetNewScreen(NULL);
	} else if (key == Key_Enter && table->SelectedIndex != -1) {
		Inventory_SetSelectedBlock(table->Elements[table->SelectedIndex]);
		Gui_SetNewScreen(NULL);
	} else if (elem->HandlesKeyDown(elem, key)) {
	} else {
		game.Gui.hudScreen.hotbar.HandlesKeyDown(key);
	}
	return true;
}

bool InventoryScreen_HandlesKeyUp(GuiElement* elem, Key key) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	if (key == KeyBind_Get(KeyBind_Inventory)) {
		screen->ReleasedInv = true; return true;
	}
	return game.Gui.hudScreen.hotbar.HandlesKeyUp(key);
}

bool InventoryScreen_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	TableWidget* table = &screen->Table;
	elem = &screen->Table.Base.Base;
	if (table->Scroll.DraggingMouse || game.Gui.hudScreen.hotbar.HandlesMouseDown(x, y, btn))
		return true;

	bool handled = elem->HandlesMouseDown(elem, x, y, btn);
	if ((!handled || table->PendingClose) && btn == MouseButton_Left) {
		bool hotbar = Key_IsControlPressed();
		if (!hotbar) Gui_SetNewScreen(NULL);
	}
	return true;
}

bool InventoryScreen_HandlesMouseUp(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	elem = &screen->Table.Base.Base;
	return elem->HandlesMouseUp(elem, x, y, btn);
}

bool InventoryScreen_HandlesMouseMove(GuiElement* elem, Int32 x, Int32 y) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	elem = &screen->Table.Base.Base;
	return elem->HandlesMouseMove(elem, x, y);
}

bool InventoryScreen_HandlesMouseScroll(GuiElement* elem, Real32 delta) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	elem = &screen->Table.Base.Base;

	bool hotbar = Key_IsAltPressed() || Key_IsControlPressed() || Key_IsShiftPressed();
	if (hotbar) return false;
	return elem->HandlesMouseScroll(elem, delta);
}

Screen* InventoryScreen_MakeInstance(void) {
	InventoryScreen* screen = &InventoryScreen_Instance;
	Platform_MemSet(&screen, 0, sizeof(InventoryScreen));
	Screen_Reset(&screen->Base);

	screen->Base.Base.HandlesKeyDown     = InventoryScreen_HandlesKeyDown;
	screen->Base.Base.HandlesKeyUp       = InventoryScreen_HandlesKeyUp;
	screen->Base.Base.HandlesMouseDown   = InventoryScreen_HandlesMouseDown;
	screen->Base.Base.HandlesMouseUp     = InventoryScreen_HandlesMouseUp;
	screen->Base.Base.HandlesMouseMove   = InventoryScreen_HandlesMouseMove;
	screen->Base.Base.HandlesMouseScroll = InventoryScreen_HandlesMouseScroll;

	screen->Base.OnResize           = InventoryScreen_OnResize;
	screen->Base.Base.Init          = InventoryScreen_Init;
	screen->Base.Base.Render        = InventoryScreen_Render;
	screen->Base.Base.Free          = InventoryScreen_Free;
	return &screen->Base;
}
extern Screen* InventoryScreen_UNSAFE_RawPointer = &InventoryScreen_Instance.Base;


typedef struct StatusScreen_ {
	Screen Base;
	FontDesc Font;
	TextWidget Status, HackStates;
	TextAtlas PosAtlas;
	Real64 Accumulator;
	Int32 Frames, FPS;
	bool Speeding, HalfSpeeding, Noclip, Fly;
	Int32 LastFov;
} StatusScreen;
StatusScreen StatusScreen_Instance;
	
void StatusScreen_Render(GuiElement* elem, Real64 delta) {
	StatusScreen* screen = (StatusScreen*)elem;
	StatusScreen_Update(screen, delta);
	if (Game_HideGui || !Game_ShowFPS) return;

	Gfx_SetTexturing(true);
	elem = &screen->Status.Base.Base;
	elem->Render(elem, delta);

	if (!Game_ClassicMode && Gui_Active == NULL) {
		StatusScreen_UpdateHackState(screen, false);
		StatusScreen_DrawPosition(screen);
		elem = &screen->HackStates.Base.Base;
		elem->Render(elem, delta);
	}
	Gfx_SetTexturing(false);
}

void StatusScreen_MakeText(StatusScreen* screen, STRING_TRANSIENT String* status) {
	screen->FPS = (Int32)(screen->Frames / screen->Accumulator);
	String_AppendInt32(status, screen->FPS);
	String_AppendConst(status, " fps, ");

	if (Game_ClassicMode) {
		String_AppendInt32(status, Game_ChunkUpdates);
		String_AppendConst(status, " chunk updates");
	} else {
		if (Game_ChunkUpdates > 0) {
			String_AppendInt32(status, Game_ChunkUpdates);
			String_AppendConst(status, " chunks/s, ");
		}

		Int32 indices = ICOUNT(Game_Vertices);
		String_AppendInt32(status, indices);
		String_AppendConst(status, " vertices");

		Int32 ping = PingList.AveragePingMilliseconds();
		if (ping != 0) {
			String_AppendConst(status, ", ping ");
			String_AppendInt32(status, ping);
			String_AppendConst(status, " ms");
		}
	}
}

void StatusScreen_Update(StatusScreen* screen, Real64 delta) {
	screen->Frames++;
	screen->Accumulator += delta;
	if (screen->Accumulator < 1.0) return;

	UInt8 statusBuffer[String_BufferSize(STRING_SIZE * 2)];
	String status = String_InitAndClearArray(statusBuffer);
	StatusScreen_MakeText(screen, &status);

	TextWidget_SetText(&screen->Status, &status);
	screen->Accumulator = 0.0;
	screen->Frames = 0;
	Game_ChunkUpdates = 0;
}

void StatusScreen_OnResize(Screen* screen) { }
void StatusScreen_ChatFontChanged(void) {
	StatusScreen* screen = &StatusScreen_Instance;
	GuiElement* elem = &screen->Base.Base;
	elem->Recreate(elem);
}

void StatusScreen_ContextLost(void) {
	StatusScreen* screen = &StatusScreen_Instance;
	TextAtlas_Free(&screen->PosAtlas);
	GuiElement* elem;

	elem = &screen->Status.Base.Base;
	elem->Free(elem);
	elem = &screen->HackStates.Base.Base;
	elem->Free(elem);
}

void StatusScreen_ContextRecreated(void) {
	StatusScreen* screen = &StatusScreen_Instance;
	status = new TextWidget(game, font)
		.SetLocation(Anchor.LeftOrTop, Anchor.LeftOrTop, 2, 2);
	status.ReducePadding = true;
	status.Init();
	string msg = statusBuffer.Length > 0 ? statusBuffer.ToString() : "FPS: no data yet";
	status.SetText(msg);

	posAtlas = new TextAtlas(game, 16);
	String chars = String_FromConst("0123456789-, ()");
	String prefix = String_FromConst("Position: ");
	TextAtlas_Make(&screen->PosAtlas, &chars, &screen->Font, &prefix);
	screen->PosAtlas.Tex.Y = (Int16)(status.Height + 2);

	Int32 yOffset = status.Height + posAtlas.tex.Height + 2;
	hackStates = new TextWidget(game, font)
		.SetLocation(Anchor.LeftOrTop, Anchor.LeftOrTop, 2, yOffset);
	hackStates.ReducePadding = true;
	hackStates.Init();
	StatusScreen_UpdateHackState(screen, true);
}

void StatusScreen_Init(GuiElement* elem) {
	StatusScreen* screen = (StatusScreen*)elem;
	FontDesc_Make(&screen->Font, 16, FONT_STYLE_NORMAL);
	StatusScreen_ContextRecreated();

	Event_RegisterVoid(&ChatEvents_FontChanged,     StatusScreen_ChatFontChanged);
	Event_RegisterVoid(&GfxEvents_ContextLost,      StatusScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, StatusScreen_ContextRecreated);
}

void StatusScreen_Free(GuiElement* elem) {
	StatusScreen* screen = (StatusScreen*)elem;
	Platform_FreeFont(&screen->Font);
	StatusScreen_ContextLost();

	Event_UnregisterVoid(&ChatEvents_FontChanged,     StatusScreen_ChatFontChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost,      StatusScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, StatusScreen_ContextRecreated);
}

void StatusScreen_DrawPosition(StatusScreen* screen) {
	TextAtlas* atlas = &screen->PosAtlas;
	VertexP3fT2fC4b vertices[4 * 8];
	VertexP3fT2fC4b* ptr = vertices;

	Texture tex = atlas->Tex; tex.X = 2; tex.Width = (UInt16)atlas->Offset;
	PackedCol col = PACKEDCOL_WHITE;
	GfxCommon_Make2DQuad(&tex, col, &ptr);

	Vector3I pos; Vector3I_Floor(&pos, &LocalPlayer_Instance.Base.Base.Position);
	atlas->CurX = atlas->Offset + 2;

	TextAtlas_Add(atlas, 13, &ptr);
	TextAtlas_AddInt(atlas, pos.X, &ptr);
	TextAtlas_Add(atlas, 11, &ptr);
	TextAtlas_AddInt(atlas, pos.Y, &ptr);
	TextAtlas_Add(atlas, 11, &ptr);
	TextAtlas_AddInt(atlas, pos.Z, &ptr);
	TextAtlas_Add(atlas, 14, &ptr);

	Gfx_BindTexture(atlas->Tex.ID);
	/* TODO: Do we need to use a separate VB here? */
	GfxCommon_UpdateDynamicVb_IndexedTris(game.ModelCache.vb, vertices, index);
}

void StatusScreen_UpdateHackState(StatusScreen* screen, bool force) {
	HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	if (force || hacks->Speeding != screen->Speeding || hacks->HalfSpeeding != screen->HalfSpeeding || hacks->Noclip != screen->Noclip || hacks->Flying != screen->Fly || Game_Fov != screen->LastFov) {
		screen->Speeding = hacks->Speeding; screen->Noclip = hacks->Noclip;
		screen->HalfSpeeding = hacks->HalfSpeeding; screen->Fly = hacks->Flying;
		screen->LastFov = Game_Fov;

		UInt8 statusBuffer[String_BufferSize(STRING_SIZE * 2)];
		String status = String_InitAndClearArray(statusBuffer);

		if (Game_Fov != Game_DefaultFov) {
			String_AppendConst(&status, "Zoom fov ");
			String_AppendInt32(&status, Game_Fov);
			String_AppendConst(&status, "  ");
		}
		if (hacks->Flying) String_AppendConst(&status, "Fly ON   ");

		bool speed = (hacks->Speeding || hacks->HalfSpeeding) && (hacks->CanSpeed || hacks->BaseHorSpeed > 1);
		if (speed) String_AppendConst(&status, "Speed ON   ");
		if (hacks->Noclip) String_AppendConst(&status, "Noclip ON   ");

		TextWidget_SetText(&screen->HackStates, &status);
	}
}

Screen* StatusScreen_MakeInstance(void) {
	StatusScreen* screen = &StatusScreen_Instance;
	Platform_MemSet(&screen, 0, sizeof(StatusScreen));
	Screen_Reset(&screen->Base);

	screen->Base.OnResize    = StatusScreen_OnResize;
	screen->Base.Base.Init   = StatusScreen_Init;
	screen->Base.Base.Render = StatusScreen_Render;
	screen->Base.Base.Free   = StatusScreen_Free;
	return &screen->Base;
}

void StatusScreen_Ready(void) {
	GuiElement* elem = &StatusScreen_Instance.Base.Base;
	elem->Init(elem);
}

IGameComponent StatusScreen_MakeComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Ready = StatusScreen_Ready;
	return comp;
}