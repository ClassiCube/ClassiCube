#include "Menus.h"
#include "Widgets.h"
#include "Game.h"
#include "Event.h"
#include "GraphicsCommon.h"
#include "Platform.h"
#include "Inventory.h"
#include "Drawer2D.h"
#include "GraphicsAPI.h"
#include "Funcs.h"
#include "TerrainAtlas.h"
#include "ModelCache.h"
#include "MapGenerator.h"
#include "ServerConnection.h"
#include "Chat.h"
#include "ExtMath.h"
#include "Window.h"
#include "Camera.h"
#include "AsyncDownloader.h"
#include "Block.h"

#define LeftOnly(func) { if (btn == MouseButton_Left) { func; } return true; }
#define FILES_SCREEN_ITEMS 5
#define FILES_SCREEN_BUTTONS (FILES_SCREEN_ITEMS + 3)

typedef struct ListScreen_ {
	Screen_Layout
	FontDesc Font;
	Int32 CurrentIndex;
	ButtonWidget_Click TextButtonClick;
	String TitleText;
	ButtonWidget Buttons[FILES_SCREEN_BUTTONS];
	TextWidget Title;
	Widget* Widgets[FILES_SCREEN_BUTTONS + 1];
	StringsBuffer Entries; /* NOTE: this is the last member so we can avoid memsetting it to 0 */
} ListScreen;

typedef void (*Menu_ContextRecreated)(GuiElement* elem);
typedef struct MenuScreen_ {
	Screen_Layout
	Widget** WidgetsPtr;
	Int32 WidgetsCount;
	FontDesc TitleFont, RegularFont;
	Menu_ContextRecreated ContextRecreated;
} MenuScreen;


void Menu_FreeWidgets(Widget** widgets, Int32 widgetsCount) {
	if (widgets == NULL) return;
	Int32 i;
	for (i = 0; i < widgetsCount; i++) {
		if (widgets[i] == NULL) continue;
		widgets[i]->VTABLE->Free((GuiElement*)widgets[i]);
	}
}

void Menu_RepositionWidgets(Widget** widgets, Int32 widgetsCount) {
	if (widgets == NULL) return;
	Int32 i;
	for (i = 0; i < widgetsCount; i++) {
		if (widgets[i] == NULL) continue;
		widgets[i]->Reposition((GuiElement*)widgets[i]);
	}
}

void Menu_RenderWidgets(Widget** widgets, Int32 widgetsCount, Real64 delta) {
	if (widgets == NULL) return;

	Int32 i;
	for (i = 0; i < widgetsCount; i++) {
		if (widgets[i] == NULL) continue;
		widgets[i]->VTABLE->Render((GuiElement*)widgets[i], delta);
	}
}

void Menu_MakeBack(ButtonWidget* widget, Int32 width, STRING_PURE String* text, Int32 y, FontDesc* font, ButtonWidget_Click onClick) {
	ButtonWidget_Create(widget, text, width, font, onClick);
	Widget_SetLocation((Widget*)widget, ANCHOR_CENTRE, ANCHOR_MAX, 0, y);
}

void Menu_MakeDefaultBack(ButtonWidget* widget, bool toGame, FontDesc* font, ButtonWidget_Click onClick) {
	Int32 width = Game_UseClassicOptions ? 400 : 200;
	if (toGame) {
		String msg = String_FromConst("Back to game");
		Screen_MakeBack(widget, width, &msg, 25, font, onClick);
	} else {
		String msg = String_FromConst("Cancel");
		Screen_MakeBack(widget, width, &msg, 25, font, onClick);
	}
}

bool Menu_SwitchOptions(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	LeftOnly(Gui_SetNewScreen(OptionsGroupScreen_MakeInstance()));
}
bool Menu_SwitchPause(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	LeftOnly(Gui_SetNewScreen(PauseScreen_MakeInstance()));
}

void Menu_RenderBounds(void) {
	/* These were sourced by taking a screenshot of vanilla
	Then using paint to extract the colour components
	Then using wolfram alpha to solve the glblendfunc equation */
	PackedCol topCol    = PACKEDCOL_CONST(24, 24, 24, 105);
	PackedCol bottomCol = PACKEDCOL_CONST(51, 51, 98, 162);
	GfxCommon_Draw2DGradient(0, 0, Game_Width, Game_Height, topCol, bottomCol);
}

bool Menu_HandleMouseDown(Widget** widgets, Int32 count, Int32 x, Int32 y, MouseButton btn) {
	Int32 i;
	/* iterate backwards (because last elements rendered are shown over others) */
	for (i = count - 1; i >= 0; i--) {
		Widget* widget = widgets[i];
		if (widget != NULL && Widget_Contains(widget, x, y)) {
			if (!widget->Disabled) {
				widget->VTABLE->HandlesMouseDown((GuiElement*)widget, x, y, btn);
			}
			return true;
		}
	}
	return false;
}

Int32 Menu_HandleMouseMove(Widget** widgets, Int32 count, Int32 x, Int32 y) {
	Int32 i;
	for (i = 0; i < count; i++) {
		Widget* widget = widgets[i];
		if (widget != NULL) widget->Active = false;
	}

	for (i = count - 1; i >= 0; i--) {
		Widget* widget = widgets[i];
		if (widget != NULL && Widget_Contains(widget, x, y)) {
			widget->Active = true;
			return i;
		}
	}
	return -1;
}


GuiElementVTABLE ListScreen_VTABLE;
ListScreen ListScreen_Instance;
String ListScreen_Get(ListScreen* screen, UInt32 index) {
	if (index < screen->Entries.Count) {
		return StringsBuffer_UNSAFE_Get(&screen->Entries, index);
	} else {
		String str = String_FromConst("-----"); return str;
	}
}

void ListScreen_MakeText(ListScreen* screen, Int32 idx, Int32 x, Int32 y, String* text) {
	ButtonWidget* widget = &screen->Buttons[idx];
	ButtonWidget_Create(widget, text, 300, &screen->Font, screen->TextButtonClick);
	Widget_SetLocation((Widget*)widget, ANCHOR_CENTRE, ANCHOR_CENTRE, x, y);
}

void ListScreen_Make(ListScreen* screen, Int32 idx, Int32 x, Int32 y, String* text, ButtonWidget_Click onClick) {
	ButtonWidget* widget = &screen->Buttons[idx];
	ButtonWidget_Create(widget, text, 40, &screen->Font, onClick);
	Widget_SetLocation((Widget*)widget, ANCHOR_CENTRE, ANCHOR_CENTRE, x, y);
}

void ListScreen_UpdateArrows(ListScreen* screen) {
	Int32 start = FILES_SCREEN_ITEMS, end = screen->Entries.Count - FILES_SCREEN_ITEMS;
	screen->Buttons[5].Disabled = screen->CurrentIndex < start;
	screen->Buttons[6].Disabled = screen->CurrentIndex >= end;
}

void ListScreen_SetCurrentIndex(ListScreen* screen, Int32 index) {
	if (index >= screen->Entries.Count) index -= FILES_SCREEN_ITEMS;
	if (index < 0) index = 0;

	UInt32 i;
	for (i = 0; i < FILES_SCREEN_ITEMS; i++) {
		String str = ListScreen_Get(screen, index + i);
		ButtonWidget_SetText(&screen->Buttons[i], &str);
	}

	screen->CurrentIndex = index;
	ListScreen_UpdateArrows(screen);
}

void ListScreen_PageClick(ListScreen* screen, bool forward) {
	Int32 delta = forward ? FILES_SCREEN_ITEMS : -FILES_SCREEN_ITEMS;
	ListScreen_SetCurrentIndex(screen, screen->CurrentIndex + delta);
}

bool ListScreen_MoveBackwards(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	ListScreen* screen = (ListScreen*)elem;
	LeftOnly(ListScreen_PageClick(screen, false));
}

bool ListScreen_MoveForwards(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	ListScreen* screen = (ListScreen*)elem;
	LeftOnly(ListScreen_PageClick(screen, true));
}

void ListScreen_ContextLost(void* obj) {
	ListScreen* screen = (ListScreen*)obj;
	Menu_FreeWidgets(screen->Widgets, Array_Elems(screen->Widgets));
}

void ListScreen_ContextRecreated(void* obj) {
	ListScreen* screen = (ListScreen*)obj;
	TextWidget_Create(&screen->Title, &screen->TitleText, &screen->Font);
	Widget_SetLocation((Widget*)(&screen->Title), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -155);

	UInt32 i;
	for (i = 0; i < FILES_SCREEN_ITEMS; i++) {
		String str = ListScreen_Get(screen, i);
		ListScreen_MakeText(screen, i, 0, 50 * (Int32)i - 100, &str);
	}

	String lArrow = String_FromConst("<");
	ListScreen_Make(screen, 5, -220, 0, &lArrow, ListScreen_MoveBackwards);
	String rArrow = String_FromConst(">");
	ListScreen_Make(screen, 6,  220, 0, &rArrow, ListScreen_MoveForwards);
	Screen_MakeDefaultBack(&screen->Buttons[7], false, &screen->Font, Menu_SwitchPause);

	screen->Widgets[0] = (Widget*)(&screen->Title);
	for (i = 0; i < FILES_SCREEN_BUTTONS; i++) {
		screen->Widgets[i + 1] = (Widget*)(&screen->Buttons[i]);
	}
	ListScreen_UpdateArrows(screen);
}

void ListScreen_Init(GuiElement* elem) {
	ListScreen* screen = (ListScreen*)elem;
	Platform_MakeFont(&screen->Font, &Game_FontName, 16, FONT_STYLE_BOLD);
	ListScreen_ContextRecreated(screen);
	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, ListScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, ListScreen_ContextRecreated);
}

void ListScreen_Render(GuiElement* elem, Real64 delta) {
	ListScreen* screen = (ListScreen*)elem;
	Menu_RenderBounds();
	Gfx_SetTexturing(true);
	Menu_RenderWidgets(screen->Widgets, Array_Elems(screen->Widgets), delta);
	Gfx_SetTexturing(false);
}

void ListScreen_Free(GuiElement* elem) {
	ListScreen* screen = (ListScreen*)elem;
	Platform_FreeFont(&screen->Font);
	ListScreen_ContextLost(screen);
	Event_UnregisterVoid(&GfxEvents_ContextLost,      screen, ListScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, screen, ListScreen_ContextRecreated);
}

bool ListScreen_HandlesKeyDown(GuiElement* elem, Key key) {
	ListScreen* screen = (ListScreen*)elem;
	if (key == Key_Escape) {
		Gui_SetNewScreen(NULL);
	} else if (key == Key_Left) {
		ListScreen_PageClick(screen, false);
	} else if (key == Key_Right) {
		ListScreen_PageClick(screen, true);
	} else {
		return false;
	}
	return true;
}

bool ListScreen_HandlesMouseMove(GuiElement* elem, Int32 x, Int32 y) {
	ListScreen* screen = (ListScreen*)elem;
	return Menu_HandleMouseMove(screen->Widgets, Array_Elems(screen->Widgets), x, y) >= 0;
}

bool ListScreen_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	ListScreen* screen = (ListScreen*)elem;
	return Menu_HandleMouseDown(screen->Widgets, Array_Elems(screen->Widgets), x, y, btn);
}

void ListScreen_OnResize(GuiElement* elem) {
	ListScreen* screen = (ListScreen*)elem;
	Menu_RepositionWidgets(screen->Widgets, Array_Elems(screen->Widgets));
}

Screen* ListScreen_MakeInstance(void) {
	ListScreen* screen = &ListScreen_Instance;
	Platform_MemSet(screen, 0, sizeof(ListScreen) - sizeof(StringsBuffer));
	StringsBuffer_UNSAFE_Reset(&screen->Entries);
	screen->VTABLE = &ListScreen_VTABLE;
	Screen_Reset((Screen*)screen);
	
	screen->VTABLE->HandlesKeyDown   = ListScreen_HandlesKeyDown;
	screen->VTABLE->HandlesMouseDown = ListScreen_HandlesMouseDown;
	screen->VTABLE->HandlesMouseMove = ListScreen_HandlesMouseMove;

	screen->OnResize       = ListScreen_OnResize;
	screen->VTABLE->Init   = ListScreen_Init;
	screen->VTABLE->Render = ListScreen_Render;
	screen->VTABLE->Free   = ListScreen_Free;
	screen->HandlesAllInput = true;
	return (Screen*)screen;
}


GuiElementVTABLE MenuScreen_VTABLE;
Int32 MenuScreen_Index(MenuScreen* screen, Widget* w) {
	Int32 i;
	for (i = 0; i < screen->WidgetsCount; i++) {
		if (screen->WidgetsPtr[i] == w) return i;
	}
	return -1;
}

bool MenuScreen_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	MenuScreen* screen = (MenuScreen*)elem;
	return Menu_HandleMouseDown(screen->WidgetsPtr, screen->WidgetsCount, x, y, btn);
}

bool MenuScreen_HandlesMouseMove(GuiElement* elem, Int32 x, Int32 y) {
	MenuScreen* screen = (MenuScreen*)elem;
	return Menu_HandleMouseMove(screen->WidgetsPtr, screen->WidgetsCount, x, y) >= 0;
}

bool MenuScreen_TrueMouse(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) { return true; }
bool MenuScreen_TrueMouseScroll(GuiElement* elem, Real32 delta) { return true; }
bool MenuScreen_TrueKeyPress(GuiElement* elem, UInt8 key)       { return true; }
bool MenuScreen_TrueKey(GuiElement* elem, Key key)              { return true; }

void MenuScreen_ContextLost(void* obj) {
	MenuScreen* screen = (MenuScreen*)obj;
	Menu_FreeWidgets(screen->WidgetsPtr, screen->WidgetsCount);
}

void MenuScreen_ContextRecreated(void* obj) {
	MenuScreen* screen = (MenuScreen*)obj;
	screen->ContextRecreated((GuiElement*)screen);
}

void MenuScreen_Init(GuiElement* elem) {
	MenuScreen* screen = (MenuScreen*)elem;
	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, MenuScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, MenuScreen_ContextRecreated);
}

void MenuScreen_Render(GuiElement* elem, Real64 delta) {
	MenuScreen* screen = (MenuScreen*)elem;
	Menu_RenderBounds();
	Gfx_SetTexturing(true);
	Menu_RenderWidgets(screen->WidgetsPtr, screen->WidgetsCount, delta);
	Gfx_SetTexturing(false);
}

void MenuScreen_Free(GuiElement* elem) {
	MenuScreen* screen = (MenuScreen*)elem;
	MenuScreen_ContextLost(screen);

	if (screen->TitleFont.Handle != NULL) {
		Platform_FreeFont(&screen->TitleFont);
	}
	if (screen->RegularFont.Handle != NULL) {
		Platform_FreeFont(&screen->RegularFont);
	}

	Event_UnregisterVoid(&GfxEvents_ContextLost,      screen, MenuScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, screen, MenuScreen_ContextRecreated);
}

void MenuScreen_OnResize(GuiElement* elem) {
	MenuScreen* screen = (MenuScreen*)elem;
	Menu_RepositionWidgets(screen->WidgetsPtr, screen->WidgetsCount);
}

void MenuScreen_MakeInstance(MenuScreen* screen, Widget** widgets, Int32 count, Menu_ContextRecreated contextRecreated) {
	ListScreen* screen = &ListScreen_Instance;
	Platform_MemSet(screen, 0, sizeof(MenuScreen));
	screen->VTABLE = &MenuScreen_VTABLE;
	Screen_Reset((Screen*)screen);

	screen->VTABLE->HandlesKeyDown     = MenuScreen_TrueKey;
	screen->VTABLE->HandlesKeyUp       = MenuScreen_TrueKey;
	screen->VTABLE->HandlesKeyPress    = MenuScreen_TrueKeyPress;
	screen->VTABLE->HandlesMouseDown   = MenuScreen_HandlesMouseDown;
	screen->VTABLE->HandlesMouseUp     = MenuScreen_TrueMouse;
	screen->VTABLE->HandlesMouseMove   = MenuScreen_HandlesMouseMove;
	screen->VTABLE->HandlesMouseScroll = MenuScreen_TrueMouseScroll;

	screen->OnResize        = MenuScreen_OnResize;
	screen->VTABLE->Init    = MenuScreen_Init;
	screen->VTABLE->Render  = MenuScreen_Render;
	screen->VTABLE->Free    = MenuScreen_Free;

	screen->HandlesAllInput  = true;
	screen->WidgetsPtr       = widgets;
	screen->WidgetsCount     = count;
	screen->ContextRecreated = contextRecreated;
}