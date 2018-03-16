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

#define Widget_HandlesKeyDown(widget, key) (widget)->Base.Base.HandlesKeyDown(&((widget)->Base.Base), key)
#define Widget_HandlesKeyUp(widget, key)   (widget)->Base.Base.HandlesKeyUp(&((widget)->Base.Base), key)
#define Widget_HandlesMouseDown(widget, x, y, btn) (widget)->Base.Base.HandlesMouseDown(&((widget)->Base.Base), x, y, btn)
#define Widget_HandlesMouseUp(widget, x, y, btn)   (widget)->Base.Base.HandlesMouseUp(&((widget)->Base.Base), x, y, btn)
#define Widget_HandlesMouseMove(widget, x, y)      (widget)->Base.Base.HandlesMouseMove(&((widget)->Base.Base), x, y)
#define Widget_HandlesMouseScroll(widget, delta)   (widget)->Base.Base.HandlesMouseScroll(&((widget)->Base.Base), delta)


typedef struct ClickableScreen_ {
	GuiElement* Elem;
	Widget** Widgets;
	UInt32 WidgetsCount;
	Int32 LastX, LastY;
	void (*OnWidgetSelected)(GuiElement* elem, Widget* widget);
} ClickableScreen;

typedef struct InventoryScreen_ {
	Screen Base;
	FontDesc Font;
	TableWidget Table;
	bool ReleasedInv;
} InventoryScreen;

typedef struct StatusScreen_ {
	Screen Base;
	FontDesc Font;
	TextWidget Status, HackStates;
	TextAtlas PosAtlas;
	Real64 Accumulator;
	Int32 Frames, FPS;
	bool Speed, HalfSpeed, Noclip, Fly, CanSpeed;
	Int32 LastFov;
} StatusScreen;

typedef struct HUDScreen_ {
	Screen Base;
	Screen* Chat;
	HotbarWidget Hotbar;
	PlayerListWidget PlayerList;
	FontDesc PlayerFont;
	bool ShowingList, WasShowingList;
} HUDScreen;

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

typedef struct GeneratingMapScreen_ {
	LoadingScreen Base;
	String LastState;
	UInt8 LastStateBuffer[String_BufferSize(STRING_SIZE)];
} GeneratingMapScreen;


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

	if (key == KeyBind_Get(KeyBind_PauseOrExit)) {
		Gui_SetNewScreen(NULL);
	} else if (key == KeyBind_Get(KeyBind_Inventory) && screen->ReleasedInv) {
		Gui_SetNewScreen(NULL);
	} else if (key == Key_Enter && table->SelectedIndex != -1) {
		Inventory_SetSelectedBlock(table->Elements[table->SelectedIndex]);
		Gui_SetNewScreen(NULL);
	} else if (Widget_HandlesKeyDown(table, key)) {
	} else {
		HUDScreen* hud = (HUDScreen*)Gui_HUD;
		return Widget_HandlesKeyDown(&hud->Hotbar, key);
	}
	return true;
}

bool InventoryScreen_HandlesKeyUp(GuiElement* elem, Key key) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	if (key == KeyBind_Get(KeyBind_Inventory)) {
		screen->ReleasedInv = true; return true;
	}

	HUDScreen* hud = (HUDScreen*)Gui_HUD;
	return Widget_HandlesKeyUp(&hud->Hotbar, key);
}

bool InventoryScreen_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	TableWidget* table = &screen->Table;
	HUDScreen* hud = (HUDScreen*)Gui_HUD;
	if (table->Scroll.DraggingMouse || Widget_HandlesMouseDown(&hud->Hotbar, x, y, btn)) return true;

	bool handled = Widget_HandlesMouseDown(table, x, y, btn);
	if ((!handled || table->PendingClose) && btn == MouseButton_Left) {
		bool hotbar = Key_IsControlPressed();
		if (!hotbar) Gui_SetNewScreen(NULL);
	}
	return true;
}

bool InventoryScreen_HandlesMouseUp(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	TableWidget* table = &screen->Table;
	return Widget_HandlesMouseUp(table, x, y, btn);
}

bool InventoryScreen_HandlesMouseMove(GuiElement* elem, Int32 x, Int32 y) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	TableWidget* table = &screen->Table;
	return Widget_HandlesMouseMove(table, x, y);
}

bool InventoryScreen_HandlesMouseScroll(GuiElement* elem, Real32 delta) {
	InventoryScreen* screen = (InventoryScreen*)elem;
	TableWidget* table = &screen->Table;

	bool hotbar = Key_IsAltPressed() || Key_IsControlPressed() || Key_IsShiftPressed();
	if (hotbar) return false;
	return Widget_HandlesMouseScroll(table, delta);
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

		Int32 ping = PingList_AveragePingMilliseconds();
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
	Int32 count = (Int32)(ptr - vertices) / sizeof(VertexP3fT2fC4b);
	GfxCommon_UpdateDynamicVb_IndexedTris(ModelCache_Vb, vertices, count);
}

bool StatusScreen_HacksChanged(StatusScreen* screen) {
	HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	return hacks->Speeding != screen->Speed || hacks->HalfSpeeding != screen->HalfSpeed || hacks->Flying != screen->Fly
		|| hacks->Noclip != screen->Noclip  || Game_Fov != screen->LastFov || hacks->CanSpeed != screen->CanSpeed;
}

void StatusScreen_UpdateHackState(StatusScreen* screen) {
	HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	screen->Speed = hacks->Speeding; screen->HalfSpeed = hacks->HalfSpeeding; screen->Fly = hacks->Flying;
	screen->Noclip = hacks->Noclip; screen->LastFov = Game_Fov; screen->CanSpeed = hacks->CanSpeed;

	UInt8 statusBuffer[String_BufferSize(STRING_SIZE * 2)];
	String status = String_InitAndClearArray(statusBuffer);

	if (Game_Fov != Game_DefaultFov) {
		String_AppendConst(&status, "Zoom fov ");
		String_AppendInt32(&status, Game_Fov);
		String_AppendConst(&status, "  ");
	}
	if (hacks->Flying) String_AppendConst(&status, "Fly ON   ");

	bool speeding = (hacks->Speeding || hacks->HalfSpeeding) && (hacks->CanSpeed || hacks->BaseHorSpeed > 1);
	if (speeding) String_AppendConst(&status, "Speed ON   ");
	if (hacks->Noclip) String_AppendConst(&status, "Noclip ON   ");

	TextWidget_SetText(&screen->HackStates, &status);
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
	StatusScreen_UpdateHackState(screen);
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
		if (StatusScreen_HacksChanged(screen)) { StatusScreen_UpdateHackState(screen); }
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


FilesScreen FilesScreen_Instance;
String FilesScreen_Get(FilesScreen* screen, UInt32 index) {
	if (index < screen->Entries.Count) {
		return StringsBuffer_UNSAFE_Get(&screen->Entries, index);
	} else {
		String str = String_FromConst("-----"); return str;
	}
}

void FilesScreen_MakeText(FilesScreen* screen, Int32 idx, Int32 x, Int32 y, String* text) {
	ButtonWidget* widget = &screen->Buttons[idx];
	ButtonWidget_Create(widget, text, 300, &screen->Font, screen->TextButtonClick);
	Widget_SetLocation(&widget->Base, ANCHOR_CENTRE, ANCHOR_CENTRE, x, y);
}

void FilesScreen_Make(FilesScreen* screen, Int32 idx, Int32 x, Int32 y, String* text, Gui_MouseHandler onClick) {
	ButtonWidget* widget = &screen->Buttons[idx];
	ButtonWidget_Create(widget, text, 40, &screen->Font, onClick);
	Widget_SetLocation(&widget->Base, ANCHOR_CENTRE, ANCHOR_CENTRE, x, y);
}

void FilesScreen_UpdateArrows(FilesScreen* screen) {
	Int32 start = FILES_SCREEN_ITEMS, end = screen->Entries.Count - FILES_SCREEN_ITEMS;
	screen->Buttons[5].Base.Disabled = screen->CurrentIndex < start;
	screen->Buttons[6].Base.Disabled = screen->CurrentIndex >= end;
}

void FilesScreen_SetCurrentIndex(FilesScreen* screen, Int32 index) {
	if (index >= screen->Entries.Count) index -= FILES_SCREEN_ITEMS;
	if (index < 0) index = 0;

	UInt32 i;
	for (i = 0; i < FILES_SCREEN_ITEMS; i++) {
		String str = FilesScreen_Get(screen, index + i);
		ButtonWidget_SetText(&screen->Buttons[i], &str);
	}

	screen->CurrentIndex = index;
	FilesScreen_UpdateArrows(screen);
}

void FilesScreen_PageClick(FilesScreen* screen, bool forward) {
	Int32 delta = forward ? FILES_SCREEN_ITEMS : -FILES_SCREEN_ITEMS;
	FilesScreen_SetCurrentIndex(screen, screen->CurrentIndex + delta);
}

bool FilesScreen_MoveBackwards(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	FilesScreen* screen = (FilesScreen*)elem;
	LeftOnly(FilesScreen_PageClick(screen, false));
}

bool FilesScreen_MoveForwards(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	FilesScreen* screen = (FilesScreen*)elem;
	LeftOnly(FilesScreen_PageClick(screen, true));
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
		String str = FilesScreen_Get(screen, i);
		FilesScreen_MakeText(screen, i, 0, 50 * (Int32)i - 100, &str);
	}

	String lArrow = String_FromConst("<");
	FilesScreen_Make(screen, 5, -220, 0, &lArrow, FilesScreen_MoveBackwards);
	String rArrow = String_FromConst(">");
	FilesScreen_Make(screen, 6,  220, 0, &rArrow, FilesScreen_MoveForwards);
	Screen_MakeDefaultBack(&screen->Buttons[7], false, &screen->Font, Screen_SwitchPause);

	screen->Widgets[0] = &screen->Title.Base;
	for (i = 0; i < FILES_SCREEN_BUTTONS; i++) {
		screen->Widgets[i + 1] = &screen->Buttons[i].Base;
	}
	ClickableScreen_Init(&screen->Clickable, &screen->Base.Base, screen->Widgets, Array_NumElements(screen->Widgets));
	FilesScreen_UpdateArrows(screen);
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
		FilesScreen_PageClick(screen, false);
	} else if (key == Key_Right) {
		FilesScreen_PageClick(screen, true);
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
	elem = screen->Chat; elem->OnResize(elem);

	Widget_Reposition(&screen->Hotbar);
	if (screen->ShowingList) {
		Widget_Reposition(&screen->PlayerList);
	}
}

void HUDScreen_OnNewMap(void* obj) {
	HUDScreen* screen = (HUDScreen*)obj;
	HUDScreen_FreePlayerList(screen);
}

bool HUDScreen_HandlesKeyPress(GuiElement* elem, UInt8 key) {
	HUDScreen* screen = (HUDScreen*)elem;
	elem = &screen->Chat->Base;
	return elem->HandlesKeyPress(elem, key); 
}

bool HUDScreen_HandlesKeyDown(GuiElement* elem, Key key) {
	HUDScreen* screen = (HUDScreen*)elem;
	Key playerListKey = KeyBind_Get(KeyBind_PlayerList);
	bool handles = playerListKey != Key_Tab || !Game_TabAutocomplete || !screen->Chat->HandlesAllInput;

	if (key == playerListKey && handles) {
		if (!screen->ShowingList && !ServerConnection_IsSinglePlayer) {
			screen->WasShowingList = true;
			HUDScreen_ContextRecreated(screen);
		}
		return true;
	}

	elem = &screen->Chat->Base;
	return elem->HandlesKeyDown(elem, key) || Widget_HandlesKeyDown(&screen->Hotbar, key);
}

bool HUDScreen_HandlesKeyUp(GuiElement* elem, Key key) {
	HUDScreen* screen = (HUDScreen*)elem;
	if (key == KeyBind_Get(KeyBind_PlayerList)) {
		if (screen->ShowingList) {
			screen->ShowingList = false;
			screen->WasShowingList = false;
			Widget_Free(&screen->PlayerList);
			return true;
		}
	}

	elem = &screen->Chat->Base;
	return elem->HandlesKeyUp(elem, key) || Widget_HandlesKeyUp(&screen->Hotbar, key);
}

bool HUDScreen_HandlesMouseScroll(GuiElement* elem, Real32 delta) {
	HUDScreen* screen = (HUDScreen*)elem;
	elem = &screen->Chat->Base;
	return elem->HandlesMouseScroll(elem, delta);
}

bool HUDScreen_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	HUDScreen* screen = (HUDScreen*)elem;
	if (btn != MouseButton_Left || !screen->Base.HandlesAllInput) return false;

	elem = &screen->Chat->Base;
	if (!screen->ShowingList) { return elem->HandlesMouseDown(elem, x, y, btn); }

	UInt8 nameBuffer[String_BufferSize(STRING_SIZE)];
	String name = String_InitAndClearArray(nameBuffer);
	PlayerListWidget_GetNameUnder(&screen->PlayerList, x, y, &name);
	if (name.length == 0) { return elem->HandlesMouseDown(elem, x, y, btn); }

	chat.AppendTextToInput(name + " ");
	return true;
}

void HUDScreen_Init(GuiElement* elem) {
	HUDScreen* screen = (HUDScreen*)elem;
	UInt16 size = Drawer2D_UseBitmappedChat ? 16 : 11;
	Platform_MakeFont(&screen->PlayerFont, &Game_FontName, size, FONT_STYLE_NORMAL);

	HotbarWidget_Create(&screen->Hotbar);
	Widget_Init(&screen->Hotbar);
	chat = new ChatScreen(game, this);
	chat.Init();

	Event_RegisterVoid(&WorldEvents_NewMap,         screen, HUDScreen_OnNewMap);
	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, HUDScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, HUDScreen_ContextRecreated);
}

void HUDScreen_Render(GuiElement* elem, Real64 delta) {
	HUDScreen* screen = (HUDScreen*)elem;
	Screen* chat = screen->Chat;
	if (Game_HideGui && chat->HandlesAllInput) {
		Gfx_SetTexturing(true);
		chat.input.Render(delta);
		Gfx_SetTexturing(false);
	}
	if (Game_HideGui) return;
	bool showMinimal = Gui_GetActiveScreen()->BlocksWorld;

	if (!screen->ShowingList && !showMinimal) {
		Gfx_SetTexturing(true);
		HUDScreen_DrawCrosshairs();
		Gfx_SetTexturing(false);
	}
	if (chat->HandlesAllInput && !Game_PureClassic) {
		chat.RenderBackground();
	}

	Gfx_SetTexturing(true);
	if (!showMinimal) { Widget_Render(&screen->Hotbar, delta); }
	elem = &screen->Chat->Base;
	elem->Render(elem, delta);

	if (screen->ShowingList && Gui_GetActiveScreen() == screen) {
		screen->PlayerList.Base.Active = screen->Chat->HandlesAllInput;
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
	elem = &screen->Chat->Base; elem->Free(elem);
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