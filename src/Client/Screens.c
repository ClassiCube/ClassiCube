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
#include "TerrainAtlas.h"
#include "ModelCache.h"
#include "MapGenerator.h"
#include "ServerConnection.h"

#define LeftOnly(func) { if (btn == MouseButton_Left) { func; } return true; }
#define Widget_Init(widget) (widget)->Base.Base.Init(&((widget)->Base.Base))
#define Widget_Render(widget, delta) (widget)->Base.Base.Render(&((widget)->Base.Base), delta)
#define Widget_Free(widget) (widget)->Base.Base.Free(&((widget)->Base.Base))
#define Widget_Reposition(widget) (widget)->Base.Reposition(&((widget)->Base))

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

bool Screen_SwitchOptions(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	LeftOnly(Gui_SetNewScreen(OptionsGroupScreen_MakeInstance()));
}
bool Screen_SwitchPause(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	LeftOnly(Gui_SetNewScreen(PauseScreen_MakeInstance()));
}


typedef struct ClickableScreen_ {
	GuiElement* Elem;
	Widget** Widgets;
	UInt32 WidgetsCount;
	Int32 LastX, LastY;
	void (*OnWidgetSelected)(GuiElement* elem, Widget* widget);
} ClickableScreen;

void ClickableScreen_RenderMenuBounds(void) {
	/* These were sourced by taking a screenshot of vanilla
	Then using paint to extract the colour components
	Then using wolfram alpha to solve the glblendfunc equation */
	PackedCol topCol    = PACKEDCOL_CONST(24, 24, 24, 105);
	PackedCol bottomCol = PACKEDCOL_CONST(51, 51, 98, 162);
	GfxCommon_Draw2DGradient(0, 0, Game_Width, Game_Height, topCol, bottomCol);
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

void InventoryScreen_OnBlockChanged(void* obj) {
	InventoryScreen* screen = (InventoryScreen*)obj;
	TableWidget_OnInventoryChanged(&screen->Table);
}

void InventoryScreen_ContextLost(void* obj) {
	InventoryScreen* screen = (InventoryScreen*)obj;
	Widget_Free(&screen->Table);
}

void InventoryScreen_ContextRecreated(void* obj) {
	InventoryScreen* screen = (InventoryScreen*)obj;
	GuiElement* elem = &screen->Table.Base.Base;
	elem->Recreate(elem);
}

void InventoryScreen_Init(GuiElement* elem) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	Platform_MakeFont(&screen->Font, &Game_FontName, 16, FONT_STYLE_NORMAL);

	TableWidget_Create(&screen->Table);
	screen->Table.Font = screen->Font;
	screen->Table.ElementsPerRow = Game_PureClassic ? 9 : 10;
	Widget_Init(&screen->Table);

	Key_KeyRepeat = true;
	Event_RegisterVoid(&BlockEvents_PermissionsChanged, screen, InventoryScreen_OnBlockChanged);
	Event_RegisterVoid(&BlockEvents_BlockDefChanged,    screen, InventoryScreen_OnBlockChanged);	
	Event_RegisterVoid(&GfxEvents_ContextLost,          screen, InventoryScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated,     screen, InventoryScreen_ContextRecreated);
}

void InventoryScreen_Render(GuiElement* elem, Real64 delta) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	Widget_Render(&screen->Table, delta);
}

void InventoryScreen_OnResize(Screen* elem) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	Widget_Reposition(&screen->Table);
}

void InventoryScreen_Free(GuiElement* elem) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	Platform_FreeFont(&screen->Font);
	Widget_Free(&screen->Table);

	Key_KeyRepeat = false;
	Event_UnregisterVoid(&BlockEvents_PermissionsChanged, screen, InventoryScreen_OnBlockChanged);
	Event_UnregisterVoid(&BlockEvents_BlockDefChanged,    screen, InventoryScreen_OnBlockChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost,          screen, InventoryScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated,     screen, InventoryScreen_ContextRecreated);
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
	GfxCommon_UpdateDynamicVb_IndexedTris(ModelCache_Vb, vertices, index);
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
void StatusScreen_ChatFontChanged(void* obj) {
	StatusScreen* screen = (StatusScreen*)obj;
	GuiElement* elem = &screen->Base.Base;
	elem->Recreate(elem);
}

void StatusScreen_ContextLost(void* obj) {
	StatusScreen* screen = (StatusScreen*)obj;
	TextAtlas_Free(&screen->PosAtlas);
	Widget_Free(&screen->Status);
	Widget_Free(&screen->HackStates);
}

void StatusScreen_ContextRecreated(void* obj) {
	StatusScreen* screen = (StatusScreen*)obj;

	TextWidget* status = &screen->Status; TextWidget_Make(status, &screen->Font);
	Widget_SetLocation(&status->Base, ANCHOR_LEFT_OR_TOP, ANCHOR_LEFT_OR_TOP, 2, 2);
	status->ReducePadding = true;
	Widget_Init(status);
	StatusScreen_Update(screen, 1.0);

	String chars = String_FromConst("0123456789-, ()");
	String prefix = String_FromConst("Position: ");
	TextAtlas_Make(&screen->PosAtlas, &chars, &screen->Font, &prefix);
	screen->PosAtlas.Tex.Y = (Int16)(status->Base.Height + 2);

	Int32 yOffset = status->Base.Height + screen->PosAtlas.Tex.Height + 2;
	TextWidget* hacks = &screen->HackStates; TextWidget_Make(hacks, &screen->Font);
	Widget_SetLocation(&hacks->Base, ANCHOR_LEFT_OR_TOP, ANCHOR_LEFT_OR_TOP, 2, yOffset);
	hacks->ReducePadding = true;
	Widget_Init(hacks);
	StatusScreen_UpdateHackState(screen, true);
}

void StatusScreen_Init(GuiElement* elem) {
	StatusScreen* screen = (StatusScreen*)elem;
	Platform_MakeFont(&screen->Font, &Game_FontName, 16, FONT_STYLE_NORMAL);
	StatusScreen_ContextRecreated(screen);

	Event_RegisterVoid(&ChatEvents_FontChanged,     screen, StatusScreen_ChatFontChanged);
	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, StatusScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, StatusScreen_ContextRecreated);
}

void StatusScreen_Render(GuiElement* elem, Real64 delta) {
	StatusScreen* screen = (StatusScreen*)elem;
	StatusScreen_Update(screen, delta);
	if (Game_HideGui || !Game_ShowFPS) return;

	Gfx_SetTexturing(true);
	Widget_Render(&screen->Status, delta);

	if (!Game_ClassicMode && Gui_Active == NULL) {
		StatusScreen_UpdateHackState(screen, false);
		StatusScreen_DrawPosition(screen);
		Widget_Render(&screen->HackStates, delta);
	}
	Gfx_SetTexturing(false);
}

void StatusScreen_Free(GuiElement* elem) {
	StatusScreen* screen = (StatusScreen*)elem;
	Platform_FreeFont(&screen->Font);
	StatusScreen_ContextLost(screen);

	Event_UnregisterVoid(&ChatEvents_FontChanged,     screen, StatusScreen_ChatFontChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost,      screen, StatusScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, screen, StatusScreen_ContextRecreated);
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

bool FilesScreen_MoveBackwards(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	LeftOnly(FilesScreen_PageClick(false));
}

bool FilesScreen_MoveForwards(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	LeftOnly(FilesScreen_PageClick(true));
}

void FilesScreen_ContextLost(void* obj) {
	FilesScreen* screen = (FilesScreen*)obj;
	Screen_FreeWidgets(screen->Widgets, Array_NumElements(screen->Widgets));
}

void FilesScreen_ContextRecreated(void* obj) {
	FilesScreen* screen = (FilesScreen*)obj;
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
	FilesScreen_ContextRecreated(screen);
	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, FilesScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, FilesScreen_ContextRecreated);
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
	FilesScreen_ContextLost(screen);
	Event_UnregisterVoid(&GfxEvents_ContextLost,      screen, FilesScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, screen, FilesScreen_ContextRecreated);
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
	StringsBuffer_UNSAFE_Reset(&screen->Entries);
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


typedef struct LoadingScreen_ {
	Screen Base;
	FontDesc Font;

	String Title, Message;
	Real32 Progress;
	TextWidget TitleWidget;
	TextWidget MessageWidget;

	UInt8 TitleBuffer[String_BufferSize(STRING_SIZE)];
	UInt8 MessageBuffer[String_BufferSize(STRING_SIZE)];
} LoadingScreen;
LoadingScreen LoadingScreen_Instance;

void LoadingScreen_SetTitle(LoadingScreen* screen, STRING_PURE String* title) {
	Widget_Free(&screen->TitleWidget);
	TextWidget_Create(&screen->TitleWidget, title, &screen->Font);
	Widget_SetLocation(&screen->TitleWidget.Base, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -80);
	String_Clear(&screen->Title);
	String_AppendString(&screen->Title, title);
}

void LoadingScreen_SetMessage(LoadingScreen* screen, STRING_PURE String* message) {
	Widget_Free(&screen->MessageWidget);
	TextWidget_Create(&screen->MessageWidget, message, &screen->Font);
	Widget_SetLocation(&screen->MessageWidget.Base, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -30);
	String_Clear(&screen->Message);
	String_AppendString(&screen->Message, message);
}

void LoadingScreen_MapLoading(void* obj, Real32 progress) {
	LoadingScreen* screen = (LoadingScreen*)obj;
	screen->Progress = progress;
}

void LoadingScreen_OnResize(Screen* elem) {
	LoadingScreen* screen = (LoadingScreen*)elem;
	Widget_Reposition(&screen->TitleWidget);
	Widget_Reposition(&screen->MessageWidget);
}

void LoadingScreen_ContextLost(void* obj) {
	LoadingScreen* screen = (LoadingScreen*)obj;
	Widget_Free(&screen->TitleWidget);
	Widget_Free(&screen->MessageWidget);
}

void LoadingScreen_ContextRecreated(void* obj) {
	LoadingScreen* screen = (LoadingScreen*)obj;
	if (Gfx_LostContext) return;
	LoadingScreen_SetTitle(screen, &screen->Title);
	LoadingScreen_SetMessage(screen, &screen->Message);
}

bool LoadingScreen_HandlesKeyDown(GuiElement* elem, Key key) {
	if (key == Key_Tab) return true;
	elem = &Gui_HUD->Base;
	return elem->HandlesKeyDown(elem, key);
}

bool LoadingScreen_HandlesKeyPress(GuiElement* elem, UInt8 key) {
	elem = &Gui_HUD->Base;
	return elem->HandlesKeyPress(elem, key);
}

bool LoadingScreen_HandlesKeyUp(GuiElement* elem, Key key) {
	if (key == Key_Tab) return true;
	elem = &Gui_HUD->Base;
	return elem->HandlesKeyUp(elem, key);
}

bool LoadingScreen_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) { return true; }
bool LoadingScreen_HandlesMouseUp(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) { return true; }
bool LoadingScreen_HandlesMouseMove(GuiElement* elem, Int32 x, Int32 y) { return true; }
bool LoadingScreen_HandlesMouseScroll(GuiElement* elem, Real32 delta) { return true; }

void LoadingScreen_UpdateBackgroundVB(VertexP3fT2fC4b* vertices, Int32 count, Int32 atlasIndex, bool* bound) {
	if (!(*bound)) {
		*bound = true;
		Gfx_BindTexture(Atlas1D_TexIds[atlasIndex]);
	}

	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	/* TODO: Do we need to use a separate VB here? */
	GfxCommon_UpdateDynamicVb_IndexedTris(ModelCache_Vb, vertices, count);
}

#define LOADING_TILE_SIZE 64
void LoadingScreen_DrawBackground(void) {
	VertexP3fT2fC4b vertices[144];
	VertexP3fT2fC4b* ptr = vertices;
	Int32 count = 0, atlasIndex = 0, y = 0;
	PackedCol col = PACKEDCOL_CONST(64, 64, 64, 255);

	TextureLoc texLoc = Block_GetTexLoc(BLOCK_DIRT, FACE_YMAX);
	TextureRec rec = Atlas1D_TexRec(texLoc, 1, &atlasIndex);
	Texture tex = Texture_FromRec(0, 0, 0, Game_Width, LOADING_TILE_SIZE, rec);		
	tex.U2 = (Real32)Game_Width / (Real32)LOADING_TILE_SIZE;

	bool bound = false;
	while (y < Game_Height) {
		tex.Y = (Int16)y;
		y += LOADING_TILE_SIZE;		
		GfxCommon_Make2DQuad(&tex, col, &ptr);
		count += 4;

		if (count < Array_NumElements(vertices)) continue;
		LoadingScreen_UpdateBackgroundVB(vertices, count, atlasIndex, &bound);
		count = 0;
		ptr = vertices;
	}

	if (count == 0) return;
	LoadingScreen_UpdateBackgroundVB(vertices, count, atlasIndex, &bound);
}

void LoadingScreen_Init(GuiElement* elem) {
	LoadingScreen* screen = (LoadingScreen*)elem;
	Gfx_SetFog(false);
	LoadingScreen_ContextRecreated(screen);

	Event_RegisterReal32(&WorldEvents_MapLoading,   screen, LoadingScreen_MapLoading);
	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, LoadingScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, LoadingScreen_ContextRecreated);
}

#define PROG_BAR_WIDTH 220
#define PROG_BAR_HEIGHT 10
void LoadingScreen_Render(GuiElement* elem, Real64 delta) {
	LoadingScreen* screen = (LoadingScreen*)elem;
	Gfx_SetTexturing(true);
	LoadingScreen_DrawBackground();
	Widget_Render(&screen->TitleWidget, delta);
	Widget_Render(&screen->MessageWidget, delta);

	Gfx_SetTexturing(false);

	Int32 x = Game_Width  / 2 - PROG_BAR_WIDTH  / 2;
	Int32 y = Game_Height / 2 - PROG_BAR_HEIGHT / 2;
	Int32 progWidth = (Int32)(PROG_BAR_WIDTH * screen->Progress);

	PackedCol backCol = PACKEDCOL_CONST(128, 128, 128, 255);
	PackedCol progCol = PACKEDCOL_CONST(128, 255, 128, 255);
	GfxCommon_Draw2DFlat(x, y, PROG_BAR_WIDTH, PROG_BAR_HEIGHT, backCol);
	GfxCommon_Draw2DFlat(x, y, progWidth,      PROG_BAR_HEIGHT, progCol);
}

void LoadingScreen_Free(GuiElement* elem) {
	LoadingScreen* screen = (LoadingScreen*)elem;
	Platform_FreeFont(&screen->Font);
	LoadingScreen_ContextLost(screen);

	Event_UnregisterReal32(&WorldEvents_MapLoading,   screen, LoadingScreen_MapLoading);
	Event_UnregisterVoid(&GfxEvents_ContextLost,      screen, LoadingScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, screen, LoadingScreen_ContextRecreated);
}

void LoadingScreen_Make(LoadingScreen* screen, STRING_PURE String* title, STRING_PURE String* message) {
	Platform_MemSet(&screen, 0, sizeof(LoadingScreen));
	Screen_Reset(&screen->Base);

	screen->Base.Base.HandlesKeyDown     = LoadingScreen_HandlesKeyDown;
	screen->Base.Base.HandlesKeyUp       = LoadingScreen_HandlesKeyUp;
	screen->Base.Base.HandlesMouseDown   = LoadingScreen_HandlesMouseDown;
	screen->Base.Base.HandlesMouseUp     = LoadingScreen_HandlesMouseUp;
	screen->Base.Base.HandlesMouseMove   = LoadingScreen_HandlesMouseMove;
	screen->Base.Base.HandlesMouseScroll = LoadingScreen_HandlesMouseScroll;

	screen->Base.OnResize           = LoadingScreen_OnResize;
	screen->Base.Base.Init          = LoadingScreen_Init;
	screen->Base.Base.Render        = LoadingScreen_Render;
	screen->Base.Base.Free          = LoadingScreen_Free;

	screen->Title = String_InitAndClearArray(screen->TitleBuffer);
	String_AppendString(&screen->Title, title);
	screen->Message = String_InitAndClearArray(screen->MessageBuffer);
	String_AppendString(&screen->Message, message);

	Platform_MakeFont(&screen->Font, &Game_FontName, 16, FONT_STYLE_NORMAL);
	screen->Base.BlocksWorld     = true;
	screen->Base.RenderHUDOver   = true;
	screen->Base.HandlesAllInput = true;
}

Screen* LoadingScreen_MakeInstance(STRING_PURE String* title, STRING_PURE String* message) {
	LoadingScreen* screen = &LoadingScreen_Instance;
	LoadingScreen_Make(screen, title, message);
	return &screen->Base;
}


typedef struct GeneratingMapScreen_ {
	LoadingScreen Base;
	String LastState;
	UInt8 LastStateBuffer[String_BufferSize(STRING_SIZE)];
} GeneratingMapScreen;
GeneratingMapScreen GeneratingMapScreen_Instance;

void GeneratingScreen_Render(GuiElement* elem, Real64 delta) {
	GeneratingMapScreen* screen = (GeneratingMapScreen*)elem;
	LoadingScreen_Render(elem, delta);
	if (Gen_Done) { ServerConnection_EndGeneration(); return; }

	String state = Gen_CurrentState;
	screen->Base.Progress = Gen_CurrentProgress;
	if (String_Equals(&state, &screen->LastState)) return;

	String_Clear(&screen->LastState);
	String_AppendString(&screen->LastState, &state);
	LoadingScreen_SetMessage(&screen->Base, &state);
}

Screen* GeneratingScreen_MakeInstance(void) {
	GeneratingMapScreen* screen = &GeneratingMapScreen_Instance;
	String title   = String_FromConst("Generating level");
	String message = String_FromConst("Generating..");
	LoadingScreen_Make(&screen->Base, &title, &message);

	screen->Base.Base.Base.Render = GeneratingScreen_Render;
	screen->LastState = String_InitAndClearArray(screen->LastStateBuffer);
	return &screen->Base.Base;
}


typedef struct HUDScreen_ {
	Screen Base;
	Screen* Chat;
	HotbarWidget Hotbar;
	PlayerListWidget PlayerList;
	FontDesc PlayerFont;
	bool ShowingList, WasShowingList;
} HUDScreen;
HUDScreen HUDScreen_Instance;


#define CH_EXTENT 16
#define CH_WEIGHT 2
void HUDScreen_DrawCrosshairs(void) {
	if (Gui_IconsTex == NULL) return;
	TextureRec chRec = { 0.0f, 0.0f, 15.0f / 256.0f, 15 / 256.0f };

	Int32 extent = (Int32)(CH_EXTENT * Game_Scale(Game_Height / 480.0f));
	Int32 chX = (Game_Width / 2) - extent, chY = (Game_Height / 2) - extent;
	Texture chTex = Texture_FromRec(Gui_IconsTex, chX, chY, extent * 2, extent * 2, chRec);
	Texture_Render(&chTex);
}

void HUDScreen_FreePlayerList(HUDScreen* screen) {
	screen->WasShowingList = screen->ShowingList;
	if (screen->ShowingList) {
		Widget_Free(&screen->PlayerList);
	}
	screen->ShowingList = false;
}

void HUDScreen_ContextLost(void* obj) {
	HUDScreen* screen = (HUDScreen*)obj;
	HUDScreen_FreePlayerList(screen);
	Widget_Free(&screen->Hotbar);
}

void HUDScreen_ContextRecreated(void* obj) {
	HUDScreen* screen = (HUDScreen*)obj;
	Widget_Free(&screen->Hotbar);
	Widget_Init(&screen->Hotbar);

	if (!screen->WasShowingList) return;
	bool extended = ServerConnection_SupportsExtPlayerList && !Game_UseClassicTabList;
	PlayerListWidget_Create(&screen->PlayerList, &screen->PlayerFont, !extended);

	Widget_Init(&screen->PlayerList);
	Widget_Reposition(&screen->PlayerList);
}


void HUDScreen_OnResize(Screen* elem) {
	HUDScreen* screen = (HUDScreen*)elem;
	chat.OnResize(width, height);
	Widget_Reposition(&screen->Hotbar);

	if (screen->ShowingList) {
		Widget_Reposition(&screen->PlayerList);
	}
}

void HUDScreen_OnNewMap(void* obj) { 
	DisposePlayerList(); 
}

bool HUDScreen_HandlesKeyPress(char key) { 
	return chat.HandlesKeyPress(key); 
}

bool HUDScreen_HandlesKeyDown(Key key) {
	Key playerListKey = game.Mapping(KeyBind.PlayerList);
	bool handles = playerListKey != Key.Tab || !Game_TabAutocomplete || !chat.HandlesAllInput;
	if (key == playerListKey && handles) {
		if (playerList == null && !ServerConnection_IsSinglePlayer) {
			hadPlayerList = true;
			ContextRecreated();
		}
		return true;
	}

	return chat.HandlesKeyDown(key) || hotbar.HandlesKeyDown(key);
}

bool HUDScreen_HandlesKeyUp(Key key) {
	if (key == game.Mapping(KeyBind.PlayerList)) {
		if (playerList != null) {
			hadPlayerList = false;
			playerList.Dispose();
			playerList = null;
			return true;
		}
	}

	return chat.HandlesKeyUp(key) || hotbar.HandlesKeyUp(key);
}

bool HUDScreen_HandlesMouseScroll(float delta) {
	return chat.HandlesMouseScroll(delta);
}

bool HUDScreen_HandlesMouseClick(int mouseX, int mouseY, MouseButton button) {
	if (button != MouseButton_Left || !HandlesAllInput) return false;

	string name;
	if (playerList == null || (name = playerList.GetNameUnder(mouseX, mouseY)) == null)
		return chat.HandlesMouseClick(mouseX, mouseY, button);
	chat.AppendTextToInput(name + " ");
	return true;
}

void HUDScreen_Init(GuiElement* elem) {
	HUDScreen* screen = (HUDScreen*)elem;
	Int32 size = Drawer2D_UseBitmappedChat ? 16 : 11;
	playerFont = new Font(game.FontName, size);
	hotbar = game.Mode.MakeHotbar();
	hotbar.Init();
	chat = new ChatScreen(game, this);
	chat.Init();

	Event_RegisterVoid(&WorldEvents_NewMap,         screen, HUDScreen_OnNewMap);
	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, HUDScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, HUDScreen_ContextRecreated);
}

void HUDScreen_Render(GuiElement* elem, Real64 delta) {
	HUDScreen* screen = (HUDScreen*)elem;
	if (Game_HideGui && chat->HandlesAllInput) {
		Gfx_SetTexturing(true);
		chat.input.Render(delta);
		Gfx_SetTexturing(false);
	}
	if (Game_HideGui) return;
	bool showMinimal = Gui_GetActiveScreen()->BlocksWorld;

	if (!screen->ShowingList && !showMinimal) {
		Gfx_SetTexturing(true);
		DrawCrosshairs();
		Gfx_SetTexturing(false);
	}
	if (chat->HandlesAllInput && !Game_PureClassic) {
		chat.RenderBackground();
	}

	Gfx_SetTexturing(true);
	if (!showMinimal) { Widget_Render(&screen->Hotbar, delta); }
	chat.Render(delta);

	if (screen->ShowingList && Gui_GetActiveScreen() == screen) {
		Widget_Render(&screen->PlayerList, delta);
		/* NOTE: Should usually be caught by KeyUp, but just in case. */
		if (!KeyBind_IsPressed(KeyBind_PlayerList)) {
			Widget_Free(&screen->PlayerList);
			screen->ShowingList = false;
		}
	}
	Gfx_SetTexturing(false);
}

void HUDScreen_Free(GuiElement* elem) {
	HUDScreen* screen = (HUDScreen*)elem;
	Platform_FreeFont(&screen->PlayerFont);
	chat.Dispose();
	HUDScreen_ContextLost(screen);

	Event_UnregisterVoid(&WorldEvents_NewMap,         screen, HUDScreen_OnNewMap);
	Event_UnregisterVoid(&GfxEvents_ContextLost,      screen, HUDScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, screen, HUDScreen_ContextRecreated);
}

Screen* HUDScreen_MakeInstance(void) {
	HUDScreen* screen = &HUDScreen_Instance;
	Platform_MemSet(&screen, 0, sizeof(HUDScreen));
	Screen_Reset(&screen->Base);

	screen->Base.OnResize    = HUDScreen_OnResize;
	screen->Base.Base.Init   = HUDScreen_Init;
	screen->Base.Base.Render = HUDScreen_Render;
	screen->Base.Base.Free   = HUDScreen_Free;
	return &screen->Base;
}

void HUDScreen_Ready(void) {
	GuiElement* elem = &HUDScreen_Instance.Base.Base;
	elem->Init(elem);
}

IGameComponent HUDScreen_MakeComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Ready = HUDScreen_Ready;
	return comp;
}