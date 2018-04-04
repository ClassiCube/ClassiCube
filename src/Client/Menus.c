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

#define FILES_SCREEN_ITEMS 5
#define FILES_SCREEN_BUTTONS (FILES_SCREEN_ITEMS + 3)

typedef struct ListScreen_ {
	Screen_Layout
	FontDesc Font;
	Int32 CurrentIndex;
	Widget_LeftClick TextButtonClick;
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

void Menu_MakeBack(ButtonWidget* widget, Int32 width, STRING_PURE String* text, Int32 y, FontDesc* font, Widget_LeftClick onClick) {
	ButtonWidget_Create(widget, text, width, font, onClick);
	Widget_SetLocation((Widget*)widget, ANCHOR_CENTRE, ANCHOR_MAX, 0, y);
}

void Menu_MakeDefaultBack(ButtonWidget* widget, bool toGame, FontDesc* font, Widget_LeftClick onClick) {
	Int32 width = Game_UseClassicOptions ? 400 : 200;
	if (toGame) {
		String msg = String_FromConst("Back to game");
		Screen_MakeBack(widget, width, &msg, 25, font, onClick);
	} else {
		String msg = String_FromConst("Cancel");
		Screen_MakeBack(widget, width, &msg, 25, font, onClick);
	}
}

void Menu_SwitchOptions(GuiElement* screenElem, GuiElement* widget) {
	Gui_SetNewScreen(OptionsGroupScreen_MakeInstance());
}
void Menu_SwitchPause(GuiElement* screenElem, GuiElement* widget) {
	Gui_SetNewScreen(PauseScreen_MakeInstance());
}

void Menu_RenderBounds(void) {
	/* These were sourced by taking a screenshot of vanilla
	Then using paInt32 to extract the colour components
	Then using wolfram alpha to solve the glblendfunc equation */
	PackedCol topCol    = PACKEDCOL_CONST(24, 24, 24, 105);
	PackedCol bottomCol = PACKEDCOL_CONST(51, 51, 98, 162);
	GfxCommon_Draw2DGradient(0, 0, Game_Width, Game_Height, topCol, bottomCol);
}

Int32 Menu_HandleMouseDown(GuiElement* screen, Widget** widgets, Int32 count, Int32 x, Int32 y, MouseButton btn) {
	Int32 i;
	/* iterate backwards (because last elements rendered are shown over others) */
	for (i = count - 1; i >= 0; i--) {
		Widget* widget = widgets[i];
		if (widget == NULL || !Widget_Contains(widget, x, y)) continue;
		if (widget->Disabled) return i;

		if (widget->MenuClick != NULL && btn == MouseButton_Left) {
			widget->MenuClick(screen, (GuiElement*)widget);
		} else {
			widget->VTABLE->HandlesMouseDown((GuiElement*)widget, x, y, btn);
		}
		return i;
	}
	return -1;
}

Int32 Menu_HandleMouseMove(Widget** widgets, Int32 count, Int32 x, Int32 y) {
	Int32 i;
	for (i = 0; i < count; i++) {
		Widget* widget = widgets[i];
		if (widget != NULL) widget->Active = false;
	}

	for (i = count - 1; i >= 0; i--) {
		Widget* widget = widgets[i];
		if (widget == NULL || !Widget_Contains(widget, x, y)) continue;

		widget->Active = true;
		return i;
	}
	return -1;
}


GuiElementVTABLE ListScreen_VTABLE;
ListScreen ListScreen_Instance;
STRING_REF String ListScreen_UNSAFE_Get(ListScreen* screen, UInt32 index) {
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

void ListScreen_Make(ListScreen* screen, Int32 idx, Int32 x, Int32 y, String* text, Widget_LeftClick onClick) {
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

	Int32 i;
	for (i = 0; i < FILES_SCREEN_ITEMS; i++) {
		String str = ListScreen_UNSAFE_Get(screen, index + i);
		ButtonWidget_SetText(&screen->Buttons[i], &str);
	}

	screen->CurrentIndex = index;
	ListScreen_UpdateArrows(screen);
}

void ListScreen_PageClick(ListScreen* screen, bool forward) {
	Int32 delta = forward ? FILES_SCREEN_ITEMS : -FILES_SCREEN_ITEMS;
	ListScreen_SetCurrentIndex(screen, screen->CurrentIndex + delta);
}

void ListScreen_MoveBackwards(GuiElement* screenElem, GuiElement* widget) {
	ListScreen* screen = (ListScreen*)screenElem;
	ListScreen_PageClick(screen, false);
}

void ListScreen_MoveForwards(GuiElement* screenElem, GuiElement* widget) {
	ListScreen* screen = (ListScreen*)screenElem;
	ListScreen_PageClick(screen, true);
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
		String str = ListScreen_UNSAFE_Get(screen, i);
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
	return Menu_HandleMouseDown(elem, screen->Widgets, Array_Elems(screen->Widgets), x, y, btn) >= 0;
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
	return Menu_HandleMouseDown(elem, screen->WidgetsPtr, screen->WidgetsCount, x, y, btn) >= 0;
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

typedef struct PauseScreen_ {
	MenuScreen Base;
	Widget* Widgets[8];
	ButtonWidget Buttons[8];
} PauseScreen;

GuiElementVTABLE PauseScreen_VTABLE;
PauseScreen PauseScreen_Instance;

void PauseScreen_Make(PauseScreen* screen, Int32 i, Int32 dir, Int32 y, const UInt8* title, Widget_LeftClick onClick) {
	String text = String_FromRaw(title, UInt16_MaxValue);
	ButtonWidget* btn = &screen->Buttons[i];
	ButtonWidget_Create(btn, &text, 300, &screen->Base.TitleFont, onClick);
	Widget_SetLocation((Widget*)btn, ANCHOR_CENTRE, ANCHOR_CENTRE, dir * 160, y);
}

void PauseScreen_MakeClassic(PauseScreen* screen, Int32 i, Int32 y, const UInt8* title, Widget_LeftClick onClick) {
	String text = String_FromRaw(title, UInt16_MaxValue);
	ButtonWidget* btn = &screen->Buttons[i];
	ButtonWidget_Create(btn, &text, 400, &screen->Base.TitleFont, onClick);
	Widget_SetLocation((Widget*)btn, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, y);
}

void PauseScreen_GenLevel(GuiElement* a, GuiElement* b)         { Gui_SetNewScreen(GenLevelScree_MakeInstance()); }
void PauseScreen_ClassicGenLevel(GuiElement* a, GuiElement* b)  { Gui_SetNewScreen(ClassicGenLevelScreen_MakeInstance()); }
void PauseScreen_LoadLevel(GuiElement* a, GuiElement* b)        { Gui_SetNewScreen(LoadLevelScreen_MakeInstance()); }
void PauseScreen_SaveLevel(GuiElement* a, GuiElement* b)        { Gui_SetNewScreen(SaveLevelScreen_MakeInstance()); }
void PauseScreen_TexPack(GuiElement* a, GuiElement* b)          { Gui_SetNewScreen(TexturePackScreen_MakeInstance()); }
void PauseScreen_Hotkeys(GuiElement* a, GuiElement* b)          { Gui_SetNewScreen(HotkeyListScreen_MakeInstance()); }
void PauseScreen_NostalgiaOptions(GuiElement* a, GuiElement* b) { Gui_SetNewScreen(NostalgiaScreen_MakeInstance()); }
void PauseScreen_Game(GuiElement* a, GuiElement* b)             { Gui_SetNewScreen(NULL); }
void PauseScreen_ClassicOptions(GuiElement* a, GuiElement* b)   { Gui_SetNewScreen(ClassicOptionsScreen_MakeInstance()); }
void PauseScreen_QuitGame(GuiElement* a, GuiElement* b) { Window_Close(); }

void PauseScreen_CheckHacksAllowed(void* obj) {
	if (Game_UseClassicOptions) return;
	PauseScreen* screen = (PauseScreen*)obj;
	screen->Buttons[4].Disabled = LocalPlayer_Instance.Hacks.CanAnyHacks; /* select texture pack */
}

void PauseScreen_ContextRecreated(void* obj) {
	PauseScreen* screen = (PauseScreen*)obj;
	Int32 i;
	for (i = 0; i < Array_Elems(screen->Buttons); i++) {
		screen->Widgets[i] = &screen->Buttons[i];
	}
	FontDesc* font = &screen->Base.TitleFont;

	if (Game_UseClassicOptions) {
		PauseScreen_MakeClassic(screen, 0, -100, "Options...",            PauseScreen_ClassicOptions);
		PauseScreen_MakeClassic(screen, 1,  -50, "Generate new level...", PauseScreen_ClassicGenLevel);
		PauseScreen_MakeClassic(screen, 2,    0, "Load level...",         PauseScreen_LoadLevel);
		PauseScreen_MakeClassic(screen, 3,   50, "Save level...",         PauseScreen_SaveLevel);
		PauseScreen_MakeClassic(screen, 4,  150, "Nostalgia options...",  PauseScreen_NostalgiaOptions);

		String back = String_FromConst("Back to game");
		Menu_MakeBack(&screen->Buttons[5], 400, &back, 25, font, PauseScreen_QuitGame);

		/* Disable nostalgia options in classic mode*/
		if (Game_ClassicMode) { screen->Widgets[4] = NULL; }
		screen->Widgets[6] = NULL;
		screen->Widgets[7] = NULL;
	} else {
		PauseScreen_Make(screen, 0, -1, -50, "Options...",             Menu_SwitchOptions);
		PauseScreen_Make(screen, 1,  1, -50, "Generate new level...",  PauseScreen_GenLevel);
		PauseScreen_Make(screen, 2,  1,   0, "Load level...",          PauseScreen_LoadLevel);
		PauseScreen_Make(screen, 3,  1,  50, "Save level...",          PauseScreen_SaveLevel);
		PauseScreen_Make(screen, 4, -1,   0, "Change texture pack...", PauseScreen_TexPack);
		PauseScreen_Make(screen, 5, -1,  50, "Hotkeys...",             PauseScreen_Hotkeys);

		String quitMsg = String_FromConst("Quit game");
		ButtonWidget_Create(&screen->Buttons[6], &quitMsg, 120, font, PauseScreen_QuitGame);
		Widget_SetLocation((Widget*)(&screen->Buttons[6]), ANCHOR_MAX, ANCHOR_MAX, 5, 5);
		Menu_MakeDefaultBack(&screen->Buttons[7], true, font, PauseScreen_QuitGame);
	}

	if (!ServerConnection_IsSinglePlayer) {
		screen->Buttons[1].Disabled = true;
		screen->Buttons[2].Disabled = true;
	}
	PauseScreen_CheckHacksAllowed(obj);
}

void PauseScreen_Init(GuiElement* elem) {
	PauseScreen* screen = (PauseScreen*)elem;
	MenuScreen_Init(elem);
	Platform_MakeFont(&screen->Base.TitleFont, &Game_FontName, 16, FONT_STYLE_BOLD);
	Event_RegisterVoid(&UserEvents_HackPermissionsChanged, screen, PauseScreen_CheckHacksAllowed);
	PauseScreen_ContextRecreated(screen);
}

void PauseScreen_Free(GuiElement* elem) {
	PauseScreen* screen = (PauseScreen*)elem;
	MenuScreen_Free(elem);
	Event_UnregisterVoid(&UserEvents_HackPermissionsChanged, screen, PauseScreen_CheckHacksAllowed);
}

bool PauseScreen_HandlesKeyDown(GuiElement* elem, Key key) {
	if (key == Key_Escape) { Gui_SetNewScreen(NULL); }
	return key < Key_F1 || key > Key_F35;
}

Screen* PauseScreen_MakeInstance(void) {
	PauseScreen* screen = &PauseScreen_Instance;
	MenuScreen_MakeInstance(&screen->Base, screen->Widgets, Array_Elems(screen->Buttons), PauseScreen_ContextRecreated);
	PauseScreen_VTABLE = *screen->Base.VTABLE;
	screen->Base.VTABLE = &PauseScreen_VTABLE;

	screen->Base.VTABLE->Init           = PauseScreen_Init;
	screen->Base.VTABLE->Free           = PauseScreen_Free;
	screen->Base.VTABLE->HandlesKeyDown = PauseScreen_HandlesKeyDown;
	return screen;
}
