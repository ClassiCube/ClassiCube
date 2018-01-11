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
#include "Funcs.h"

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

void Screen_MakeBack(ButtonWidget* widget, Int32 width, STRING_PURE String* text, Int32 y, FontDesc* font, Gui_MouseHandler onClick) {
	ButtonWidget_Create(widget, text, width, font, onClick);
	Widget_SetLocation(&widget->Base, ANCHOR_CENTRE, ANCHOR_BOTTOM_OR_RIGHT, 0, y);
}

void Screen_MakeDefaultBack(ButtonWidget* widget, bool toGame, FontDesc* font, Gui_MouseHandler onClick) {
	Int32 width = Game_UseClassicOptions ? 400 : 200;
	if (toGame) {
		String msg = String_FromConst("Back to game");
		Screen_MakeBack(widget, width, &msg, 25, font, onClick);
	} else {
		String msg = String_FromConst("Cancel");
		Screen_MakeBack(widget, width, &msg, 25, font, onClick);
	}
}

void Screen_SwitchOptions(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	if (btn == MouseButton_Left) Gui_SetNewScreen(OptionsGroupScreen_MakeInstance());
}
void Screen_SwitchPause(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	if (btn == MouseButton_Left) Gui_SetNewScreen(PauseScreen_MakeInstance());
}


/* These were sourced by taking a screenshot of vanilla
   Then using paint to extract the colour components
   Then using wolfram alpha to solve the glblendfunc equation */
PackedCol Menu_TopBackCol    = PACKEDCOL_CONST(24, 24, 24, 105);
PackedCol Menu_BottomBackCol = PACKEDCOL_CONST(51, 51, 98, 162);

typedef struct ClickableScreen_ {
	GuiElement* Elem;
	Widget** Widgets;
	UInt32 WidgetsCount;
	Int32 LastX, LastY;
	void (*OnWidgetSelected)(GuiElement* elem, Widget* widget);
} ClickableScreen;

void ClickableScreen_RenderMenuBounds(void) {
	GfxCommon_Draw2DGradient(0, 0, Game_Width, Game_Height, Menu_TopBackCol, Menu_BottomBackCol);
}

void ClickableScreen_DefaultWidgetSelected(GuiElement* elem, Widget* widget) { }

bool ClickableScreen_HandleMouseDown(ClickableScreen* data, Int32 x, Int32 y, MouseButton btn) {
	UInt32 i;
	/* iterate backwards (because last elements rendered are shown over others) */
	for (i = data->WidgetsCount; i > 0; i--) {
		Widget* widget = data->Widgets[i - 1];
		if (widget != NULL && Gui_Contains(widget->X, widget->Y, widget->Width, widget->Height, x, y)) {
			if (!widget->Disabled) {
				GuiElement* elem = &widget->Base;
				elem->HandlesMouseDown(elem, x, y, btn);
			}
			return true;
		}
	}
	return false;
}

bool ClickableScreen_HandleMouseMove(ClickableScreen* data, Int32 x, Int32 y) {
	if (data->LastX == x && data->LastY == y) return true;
	UInt32 i;
	for (i = 0; i < data->WidgetsCount; i++) {
		Widget* widget = data->Widgets[i];
		if (widget != NULL) widget->Active = false;
	}

	for (i = data->WidgetsCount; i > 0; i--) {
		Widget* widget = data->Widgets[i];
		if (widget != NULL && Gui_Contains(widget->X, widget->Y, widget->Width, widget->Height, x, y)) {
			widget->Active = true;
			data->LastX = x; data->LastY = y;
			data->OnWidgetSelected(data->Elem, widget);
			return true;
		}
	}

	data->LastX = x; data->LastY = y;
	data->OnWidgetSelected(data->Elem, NULL);
	return false;
}

void ClickableScreen_Init(ClickableScreen* data, GuiElement* elem, Widget** widgets, UInt32 widgetsCount) {
	data->Elem         = elem;
	data->Widgets      = widgets;
	data->WidgetsCount = widgetsCount;
	data->LastX = -1; data->LastY = -1;
	data->OnWidgetSelected = ClickableScreen_DefaultWidgetSelected;
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
	Platform_MakeFont(&screen->Font, &Game_FontName, 16, FONT_STYLE_NORMAL);

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

	TextWidget* status = &screen->Status; TextWidget_Make(status, &screen->Font);
	Widget_SetLocation(&status->Base, ANCHOR_LEFT_OR_TOP, ANCHOR_LEFT_OR_TOP, 2, 2);
	status->ReducePadding = true;
	status->Base.Base.Init(&status->Base.Base);
	StatusScreen_Update(screen, 1.0);

	String chars = String_FromConst("0123456789-, ()");
	String prefix = String_FromConst("Position: ");
	TextAtlas_Make(&screen->PosAtlas, &chars, &screen->Font, &prefix);
	screen->PosAtlas.Tex.Y = (Int16)(status->Base.Height + 2);

	Int32 yOffset = status->Base.Height + screen->PosAtlas.Tex.Height + 2;
	TextWidget* hacks = &screen->HackStates; TextWidget_Make(hacks, &screen->Font);
	Widget_SetLocation(&hacks->Base, ANCHOR_LEFT_OR_TOP, ANCHOR_LEFT_OR_TOP, 2, yOffset);
	hacks->ReducePadding = true;
	hacks->Base.Base.Init(&hacks->Base.Base);
	StatusScreen_UpdateHackState(screen, true);
}

void StatusScreen_Init(GuiElement* elem) {
	StatusScreen* screen = (StatusScreen*)elem;
	Platform_MakeFont(&screen->Font, &Game_FontName, 16, FONT_STYLE_NORMAL);
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
	if (force || hacks->Speeding != screen->Speeding || hacks->HalfSpeeding != screen->HalfSpeeding 
		|| hacks->Noclip != screen->Noclip || hacks->Flying != screen->Fly || Game_Fov != screen->LastFov) {
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


#define FILES_SCREEN_ITEMS 5
#define FILES_SCREEN_BUTTONS (FILES_SCREEN_ITEMS + 3)
typedef struct FilesScreen_ {
	Screen Base;
	FontDesc Font;
	Int32 CurrentIndex;
	Gui_MouseHandler TextButtonClick;
	String TitleText;
	ButtonWidget Buttons[FILES_SCREEN_BUTTONS];
	TextWidget Title;
	Widget* Widgets[FILES_SCREEN_BUTTONS + 1];
	ClickableScreen Clickable;
	StringsBuffer Entries; /* NOTE: this is the last member so we can avoid memsetting it to 0 */
} FilesScreen;
FilesScreen FilesScreen_Instance;

String FilesScreen_Get(UInt32 index) {
	FilesScreen* screen = &FilesScreen_Instance;
	if (index < screen->Entries.Count) {
		return StringsBuffer_UNSAFE_Get(&screen->Entries, index);
	} else {
		String str = String_FromConst("-----"); return str;
	}
}

void FilesScreen_MakeText(ButtonWidget* widget, Int32 x, Int32 y, String* text) {
	FilesScreen* screen = &FilesScreen_Instance;
	ButtonWidget_Create(widget, text, 300, &screen->Font, screen->TextButtonClick);
	Widget_SetLocation(&widget->Base, ANCHOR_CENTRE, ANCHOR_CENTRE, x, y);
}

void FilesScreen_Make(ButtonWidget* widget, Int32 x, Int32 y, String* text, Gui_MouseHandler onClick) {
	FilesScreen* screen = &FilesScreen_Instance;
	ButtonWidget_Create(widget, text, 40, &screen->Font, onClick);
	Widget_SetLocation(&widget->Base, ANCHOR_CENTRE, ANCHOR_CENTRE, x, y);
}

void FilesScreen_UpdateArrows(void) {
	FilesScreen* screen = &FilesScreen_Instance;
	Int32 start = FILES_SCREEN_ITEMS, end = screen->Entries.Count - FILES_SCREEN_ITEMS;
	screen->Buttons[5].Base.Disabled = screen->CurrentIndex < start;
	screen->Buttons[6].Base.Disabled = screen->CurrentIndex >= end;
}

void FilesScreen_SetCurrentIndex(Int32 index) {
	FilesScreen* screen = &FilesScreen_Instance;
	if (index >= screen->Entries.Count) index -= FILES_SCREEN_ITEMS;
	if (index < 0) index = 0;

	UInt32 i;
	for (i = 0; i < FILES_SCREEN_ITEMS; i++) {
		String str = FilesScreen_Get(index + i);
		ButtonWidget_SetText(&screen->Buttons[i], &str);
	}

	screen->CurrentIndex = index;
	FilesScreen_UpdateArrows();
}

void FilesScreen_PageClick(bool forward) {
	FilesScreen* screen = &FilesScreen_Instance;
	Int32 delta = forward ? FILES_SCREEN_ITEMS : -FILES_SCREEN_ITEMS;
	FilesScreen_SetCurrentIndex(screen->CurrentIndex + delta);
}

void FilesScreen_MoveBackwards(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	if (btn == MouseButton_Left) FilesScreen_PageClick(false);
}

void FilesScreen_MoveForwards(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	if (btn == MouseButton_Left) FilesScreen_PageClick(true);
}

void FilesScreen_ContextLost(void) {
	FilesScreen* screen = &FilesScreen_Instance;
	Screen_FreeWidgets(screen->Widgets, Array_NumElements(screen->Widgets));
}

void FilesScreen_ContextRecreated(void) {
	FilesScreen* screen = &FilesScreen_Instance;
	TextWidget_Create(&screen->Title, &screen->TitleText, &screen->Font);
	Widget_SetLocation(&screen->Title.Base, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -155);

	UInt32 i;
	for (i = 0; i < FILES_SCREEN_ITEMS; i++) {
		String str = FilesScreen_Get(i);
		FilesScreen_MakeText(&screen->Buttons[i], 0, 50 * (Int32)i - 100, &str);
	}

	String lArrow = String_FromConst("<");
	FilesScreen_Make(&screen->Buttons[5], -220, 0, &lArrow, FilesScreen_MoveBackwards);
	String rArrow = String_FromConst(">");
	FilesScreen_Make(&screen->Buttons[6],  220, 0, &rArrow, FilesScreen_MoveForwards);
	Screen_MakeDefaultBack(&screen->Buttons[7], false, &screen->Font, Screen_SwitchPause);

	screen->Widgets[0] = &screen->Title.Base;
	for (i = 0; i < FILES_SCREEN_BUTTONS; i++) {
		screen->Widgets[i + 1] = &screen->Buttons[i].Base;
	}
	ClickableScreen_Init(&screen->Clickable, &screen->Base.Base, screen->Widgets, Array_NumElements(screen->Widgets));
	FilesScreen_UpdateArrows();
}

void FilesScreen_Init(GuiElement* elem) {
	FilesScreen* screen = (FilesScreen*)elem;
	Platform_MakeFont(&screen->Font, &Game_FontName, 16, FONT_STYLE_BOLD);
	FilesScreen_ContextRecreated();
	Event_RegisterVoid(&GfxEvents_ContextLost,      FilesScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, FilesScreen_ContextRecreated);
}

void FilesScreen_Render(GuiElement* elem, Real64 delta) {
	FilesScreen* screen = (FilesScreen*)elem;
	ClickableScreen_RenderMenuBounds();
	Gfx_SetTexturing(true);
	Screen_RenderWidgets(screen->Widgets, Array_NumElements(screen->Widgets), delta);
	Gfx_SetTexturing(false);
}

void FilesScreen_Free(GuiElement* elem) {
	FilesScreen* screen = (FilesScreen*)elem;
	Platform_FreeFont(&screen->Font);
	FilesScreen_ContextLost();
	Event_UnregisterVoid(&GfxEvents_ContextLost,      FilesScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, FilesScreen_ContextRecreated);
}

bool FilesScreen_HandlesKeyDown(GuiElement* elem, Key key) {
	FilesScreen* screen = (FilesScreen*)elem;
	if (key == Key_Escape) {
		Gui_SetNewScreen(NULL);
	} else if (key == Key_Left) {
		FilesScreen_PageClick(false);
	} else if (key == Key_Right) {
		FilesScreen_PageClick(true);
	} else {
		return false;
	}
	return true;
}

bool FilesScreen_HandlesMouseMove(GuiElement* elem, Int32 mouseX, Int32 mouseY) {
	FilesScreen* screen = (FilesScreen*)elem;
	return ClickableScreen_HandleMouseMove(&screen->Clickable, mouseX, mouseY);
}

bool FilesScreen_HandlesMouseDown(GuiElement* elem, Int32 mouseX, Int32 mouseY, MouseButton button) {
	FilesScreen* screen = (FilesScreen*)elem;
	return ClickableScreen_HandleMouseDown(&screen->Clickable, mouseX, mouseY, button);
}

void FilesScreen_OnResize(Screen* elem) {
	FilesScreen* screen = (FilesScreen*)elem;
	Screen_RepositionWidgets(screen->Widgets, Array_NumElements(screen->Widgets));
}

Screen* FilesScreen_MakeInstance(void) {
	FilesScreen* screen = &FilesScreen_Instance;
	Platform_MemSet(&screen, 0, sizeof(FilesScreen) - sizeof(StringsBuffer));
	screen->Entries.Count = 0;
	Screen_Reset(&screen->Base);
	
	screen->Base.Base.HandlesKeyDown     = FilesScreen_HandlesKeyDown;
	screen->Base.Base.HandlesMouseDown   = FilesScreen_HandlesMouseDown;
	screen->Base.Base.HandlesMouseMove   = FilesScreen_HandlesMouseMove;

	screen->Base.OnResize    = FilesScreen_OnResize;
	screen->Base.Base.Init   = FilesScreen_Init;
	screen->Base.Base.Render = FilesScreen_Render;
	screen->Base.Base.Free   = FilesScreen_Free;
	screen->Base.HandlesAllInput = true;
	return &screen->Base;
}