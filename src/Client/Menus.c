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
#include "Random.h"
#include "World.h"
#include "Formats.h"
#include "ErrorHandler.h"
#include "BlockPhysics.h"
#include "MapRenderer.h"
#include "TexturePack.h"
#include "Audio.h"

#define LIST_SCREEN_ITEMS 5
#define LIST_SCREEN_BUTTONS (LIST_SCREEN_ITEMS + 3)

typedef struct ListScreen_ {
	Screen_Layout
	FontDesc Font;
	Int32 CurrentIndex;
	Widget_LeftClick EntryClick;
	String TitleText;
	ButtonWidget Buttons[LIST_SCREEN_BUTTONS];
	TextWidget Title;
	Widget* Widgets[LIST_SCREEN_BUTTONS + 1];
	StringsBuffer Entries; /* NOTE: this is the last member so we can avoid memsetting it to 0 */
} ListScreen;

typedef void (*Menu_ContextFunc)(void* obj);
#define MenuScreen_Layout \
Screen_Layout \
Widget** WidgetsPtr; \
Int32 WidgetsCount; \
FontDesc TitleFont, TextFont; \
Menu_ContextFunc ContextLost; \
Menu_ContextFunc ContextRecreated;

typedef struct MenuScreen_ { MenuScreen_Layout } MenuScreen;

typedef struct PauseScreen_ {
	MenuScreen_Layout
	Widget* Widgets[8];
	ButtonWidget Buttons[8];
} PauseScreen;

typedef struct OptionsGroupScreen_ {
	MenuScreen_Layout
	Widget* Widgets[9];
	ButtonWidget Buttons[8];
	TextWidget Desc;
	Int32 SelectedI;
} OptionsGroupScreen;

typedef struct DeathScreen_ {
	MenuScreen_Layout
	Widget* Widgets[4];
	TextWidget Title, Score;
	ButtonWidget Gen, Load;
} DeathScreen;

typedef struct EditHotkeyScreen_ {
	MenuScreen_Layout
	Widget* Widgets[7];
	HotkeyData CurHotkey, OrigHotkey;
	Int32 SelectedI;
	bool SupressNextPress;
	MenuInputWidget Input;
	ButtonWidget Buttons[6];
} EditHotkeyScreen;

typedef struct GenLevelScreen_ {
	MenuScreen_Layout
	Widget* Widgets[12];
	MenuInputWidget* Selected;
	MenuInputWidget Inputs[4];
	TextWidget Labels[5];
	ButtonWidget Buttons[3];
} GenLevelScreen;

typedef struct ClassicGenScreen_ {
	MenuScreen_Layout
	Widget* Widgets[4];
	ButtonWidget Buttons[4];
} ClassicGenScreen;

typedef struct KeyBindingsScreen_ {
	MenuScreen_Layout
	Int32 CurI, BindsCount;
	const UInt8** Descs;
	UInt8* Binds;
	Widget_LeftClick LeftPage, RightPage;
	ButtonWidget* Buttons;
	TextWidget Title;
	ButtonWidget Back, Left, Right;
} KeyBindingsScreen;

typedef struct SaveLevelScreen_ {
	MenuScreen_Layout
	Widget* Widgets[6];
	MenuInputWidget Input;
	ButtonWidget Buttons[3];
	TextWidget MCEdit, Desc;
	String TextPath;
	UInt8 TextPathBuffer[String_BufferSize(FILENAME_SIZE)];
} SaveLevelScreen;

#define MENUOPTIONS_MAX_DESC 5
typedef struct MenuOptionsScreen_ {
	MenuScreen_Layout
	MenuInputValidator* Validators;
	const UInt8** Descriptions;
	const UInt8** DefaultValues;
	ButtonWidget* Buttons;
	Int32 ActiveI, SelectedI, DescriptionsCount;
	ButtonWidget OK, Default;
	MenuInputWidget Input;
	TextGroupWidget ExtHelp;
	Texture ExtHelp_Textures[MENUOPTIONS_MAX_DESC];
	UInt8 ExtHelp_Buffer[String_BufferSize(MENUOPTIONS_MAX_DESC * TEXTGROUPWIDGET_LEN)];
} MenuOptionsScreen;


/*########################################################################################################################*
*--------------------------------------------------------Menu base--------------------------------------------------------*
*#########################################################################################################################*/
void Menu_MakeBack(ButtonWidget* widget, Int32 width, STRING_PURE String* text, Int32 y, FontDesc* font, Widget_LeftClick onClick) {
	ButtonWidget_Create(widget, width, text, font, onClick);
	Widget_SetLocation((Widget*)widget, ANCHOR_CENTRE, ANCHOR_MAX, 0, y);
}

void Menu_MakeDefaultBack(ButtonWidget* widget, bool toGame, FontDesc* font, Widget_LeftClick onClick) {
	Int32 width = Game_UseClassicOptions ? 400 : 200;
	if (toGame) {
		String msg = String_FromConst("Back to game");
		Menu_MakeBack(widget, width, &msg, 25, font, onClick);
	} else {
		String msg = String_FromConst("Cancel");
		Menu_MakeBack(widget, width, &msg, 25, font, onClick);
	}
}

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

void Menu_RenderBounds(void) {
	/* These were sourced by taking a screenshot of vanilla
	Then using paint to extract the colour components
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


/*########################################################################################################################*
*------------------------------------------------------Menu utilities-----------------------------------------------------*
*#########################################################################################################################*/
Int32 Menu_Index(Widget** widgets, Int32 widgetsCount, Widget* w) {
	Int32 i;
	for (i = 0; i < widgetsCount; i++) {
		if (widgets[i] == w) return i;
	}
	return -1;
}
Int32 Menu_Int32(STRING_PURE String* v) { Int32 value; Convert_TryParseInt32(v, &value); return value; }
Real32 Menu_Real32(STRING_PURE String* v) { Real32 value; Convert_TryParseReal32(v, &value); return value; }
PackedCol Menu_HexCol(STRING_PURE String* v) { PackedCol value; PackedCol_TryParseHex(v, &value); return value; }

void Menu_SwitchOptions(GuiElement* a, GuiElement* b)        { Gui_ReplaceActive(OptionsGroupScreen_MakeInstance()); }
void Menu_SwitchPause(GuiElement* a, GuiElement* b)          { Gui_ReplaceActive(PauseScreen_MakeInstance()); }
void Menu_SwitchClassicOptions(GuiElement* a, GuiElement* b) { Gui_ReplaceActive(ClassicOptionsScreen_MakeInstance()); }

void Menu_SwitchKeysClassic(GuiElement* a, GuiElement* b)      { Gui_ReplaceActive(ClassicKeyBindingsScreen_MakeInstance()); }
void Menu_SwitchKeysClassicHacks(GuiElement* a, GuiElement* b) { Gui_ReplaceActive(ClassicHacksKeyBindingsScreen_MakeInstance()); }
void Menu_SwitchKeysNormal(GuiElement* a, GuiElement* b)       { Gui_ReplaceActive(NormalKeyBindingsScreen_MakeInstance()); }
void Menu_SwitchKeysHacks(GuiElement* a, GuiElement* b)        { Gui_ReplaceActive(HacksKeyBindingsScreen_MakeInstance()); }
void Menu_SwitchKeysOther(GuiElement* a, GuiElement* b)        { Gui_ReplaceActive(OtherKeyBindingsScreen_MakeInstance()); }
void Menu_SwitchKeysMouse(GuiElement* a, GuiElement* b)        { Gui_ReplaceActive(MouseKeyBindingsScreen_MakeInstance()); }

void Menu_SwitchMisc(GuiElement* a, GuiElement* b)      { Gui_ReplaceActive(MiscOptionsScreen_MakeInstance()); }
void Menu_SwitchGui(GuiElement* a, GuiElement* b)       { Gui_ReplaceActive(GuiOptionsScreen_MakeInstance()); }
void Menu_SwitchGfx(GuiElement* a, GuiElement* b)       { Gui_ReplaceActive(GraphicsOptionsScreen_MakeInstance()); }
void Menu_SwitchHacks(GuiElement* a, GuiElement* b)     { Gui_ReplaceActive(HacksSettingsScreen_MakeInstance()); }
void Menu_SwitchEnv(GuiElement* a, GuiElement* b)       { Gui_ReplaceActive(EnvSettingsScreen_MakeInstance()); }
void Menu_SwitchNostalgia(GuiElement* a, GuiElement* b) { Gui_ReplaceActive(NostalgiaScreen_MakeInstance()); }

void Menu_SwitchGenLevel(GuiElement* a, GuiElement* b)         { Gui_ReplaceActive(GenLevelScreen_MakeInstance()); }
void Menu_SwitchClassicGenLevel(GuiElement* a, GuiElement* b)  { Gui_ReplaceActive(ClassicGenScreen_MakeInstance()); }
void Menu_SwitchLoadLevel(GuiElement* a, GuiElement* b)        { Gui_ReplaceActive(LoadLevelScreen_MakeInstance()); }
void Menu_SwitchSaveLevel(GuiElement* a, GuiElement* b)        { Gui_ReplaceActive(SaveLevelScreen_MakeInstance()); }
void Menu_SwitchTexPacks(GuiElement* a, GuiElement* b)         { Gui_ReplaceActive(TexturePackScreen_MakeInstance()); }
void Menu_SwitchHotkeys(GuiElement* a, GuiElement* b)          { Gui_ReplaceActive(HotkeyListScreen_MakeInstance()); }


/*########################################################################################################################*
*--------------------------------------------------------ListScreen-------------------------------------------------------*
*#########################################################################################################################*/
GuiElementVTABLE ListScreen_VTABLE;
ListScreen ListScreen_Instance;
#define LIST_SCREEN_EMPTY "-----"
STRING_REF String ListScreen_UNSAFE_Get(ListScreen* screen, UInt32 index) {
	if (index < screen->Entries.Count) {
		return StringsBuffer_UNSAFE_Get(&screen->Entries, index);
	} else {
		String str = String_FromConst(LIST_SCREEN_EMPTY); return str;
	}
}

void ListScreen_MakeText(ListScreen* screen, Int32 i) {
	ButtonWidget* btn = &screen->Buttons[i];
	screen->Widgets[i] = (Widget*)btn;

	String text = ListScreen_UNSAFE_Get(screen, screen->CurrentIndex + i);
	ButtonWidget_Create(btn, 300, &text, &screen->Font, screen->EntryClick);
	Widget_SetLocation((Widget*)btn, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, (i - 2) * 50);
}

void ListScreen_Make(ListScreen* screen, Int32 i, Int32 x, STRING_PURE String* text, Widget_LeftClick onClick) {
	ButtonWidget* btn = &screen->Buttons[i];
	screen->Widgets[i] = (Widget*)btn;

	ButtonWidget_Create(btn, 40, text, &screen->Font, onClick);
	Widget_SetLocation((Widget*)btn, ANCHOR_CENTRE, ANCHOR_CENTRE, x, 0);
}

void ListScreen_UpdateArrows(ListScreen* screen) {
	Int32 start = LIST_SCREEN_ITEMS, end = screen->Entries.Count - LIST_SCREEN_ITEMS;
	screen->Buttons[5].Disabled = screen->CurrentIndex < start;
	screen->Buttons[6].Disabled = screen->CurrentIndex >= end;
}

void ListScreen_SetCurrentIndex(ListScreen* screen, Int32 index) {
	if (index >= screen->Entries.Count) index -= LIST_SCREEN_ITEMS;
	if (index < 0) index = 0;

	Int32 i;
	for (i = 0; i < LIST_SCREEN_ITEMS; i++) {
		String str = ListScreen_UNSAFE_Get(screen, index + i);
		ButtonWidget_SetText(&screen->Buttons[i], &str);
	}

	screen->CurrentIndex = index;
	ListScreen_UpdateArrows(screen);
}

void ListScreen_PageClick(ListScreen* screen, bool forward) {
	Int32 delta = forward ? LIST_SCREEN_ITEMS : -LIST_SCREEN_ITEMS;
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
	Int32 i;
	for (i = 0; i < LIST_SCREEN_ITEMS; i++) { ListScreen_MakeText(screen, i); }

	String lArrow = String_FromConst("<");
	ListScreen_Make(screen, 5, -220, &lArrow, ListScreen_MoveBackwards);
	String rArrow = String_FromConst(">");
	ListScreen_Make(screen, 6,  220, &rArrow, ListScreen_MoveForwards);

	Menu_MakeDefaultBack(&screen->Buttons[7], false, &screen->Font, Menu_SwitchPause);
	screen->Widgets[7] = (Widget*)(&screen->Buttons[7]);
	ListScreen_UpdateArrows(screen);

	TextWidget_Create(&screen->Title, &screen->TitleText, &screen->Font);
	Widget_SetLocation((Widget*)(&screen->Title), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -155);
	screen->Widgets[8] = (Widget*)(&screen->Title);
}

void ListScreen_QuickSort(Int32 left, Int32 right) {
	StringsBuffer* buffer = &ListScreen_Instance.Entries; 
	UInt32* keys = buffer->FlagsBuffer; UInt32 key;
	while (left < right) {
		Int32 i = left, j = right;
		Int32 pivot = (i + j) / 2;

		/* partition the list */
		while (i <= j) {
			while (StringsBuffer_Compare(buffer, pivot, i) > 0) i++;
			while (StringsBuffer_Compare(buffer, pivot, j) < 0) j--;
			QuickSort_Swap_Maybe();
		}
		/* recurse into the smaller subset */
		QuickSort_Recurse(ListScreen_QuickSort)
	}
}

String ListScreen_UNSAFE_GetCur(ListScreen* screen, GuiElement* w) {
	Int32 idx = Menu_Index(screen->Widgets, Array_Elems(screen->Widgets), (Widget*)w);
	return ListScreen_UNSAFE_Get(screen, screen->CurrentIndex + idx);
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
		Gui_ReplaceActive(NULL);
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

ListScreen* ListScreen_MakeInstance(void) {
	ListScreen* screen = &ListScreen_Instance;
	Platform_MemSet(screen, 0, sizeof(ListScreen) - sizeof(StringsBuffer));
	if (screen->Entries.TextBuffer != NULL) {
		StringsBuffer_Free(&screen->Entries);
	}
	StringsBuffer_Init(&screen->Entries);

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
	return screen;
}


/*########################################################################################################################*
*--------------------------------------------------------MenuScreen-------------------------------------------------------*
*#########################################################################################################################*/
GuiElementVTABLE MenuScreen_VTABLE;
Int32 MenuScreen_Index(MenuScreen* screen, Widget* w) {
	return Menu_Index(screen->WidgetsPtr, screen->WidgetsCount, w);
}

void MenuScreen_Remove(MenuScreen* screen, Int32 i) {
	Widget** widgets = screen->WidgetsPtr;
	if (widgets[i] != NULL) { Elem_TryFree(widgets[i]); }
	widgets[i] = NULL;
}

bool MenuScreen_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	MenuScreen* screen = (MenuScreen*)elem;
	return Menu_HandleMouseDown(elem, screen->WidgetsPtr, screen->WidgetsCount, x, y, btn) >= 0;
}

bool MenuScreen_HandlesMouseMove(GuiElement* elem, Int32 x, Int32 y) {
	MenuScreen* screen = (MenuScreen*)elem;
	return Menu_HandleMouseMove(screen->WidgetsPtr, screen->WidgetsCount, x, y) >= 0;
}
bool MenuScreen_HandlesMouseUp(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) { return true; }
bool MenuScreen_HandlesMouseScroll(GuiElement* elem, Real32 delta) { return true; }

bool MenuScreen_HandlesKeyDown(GuiElement* elem, Key key) {
	if (key == Key_Escape) { Gui_ReplaceActive(NULL); }
	return key < Key_F1 || key > Key_F35;
}
bool MenuScreen_HandlesKeyPress(GuiElement* elem, UInt8 key) { return true; }
bool MenuScreen_HandlesKeyUp(GuiElement* elem, Key key) { return true; }

void MenuScreen_ContextLost(void* obj) {
	MenuScreen* screen = (MenuScreen*)obj;
	Menu_FreeWidgets(screen->WidgetsPtr, screen->WidgetsCount);
}

void MenuScreen_ContextLost_Callback(void* obj) {
	((MenuScreen*)obj)->ContextLost(obj);
}

void MenuScreen_ContextRecreated_Callback(void* obj) {
	((MenuScreen*)obj)->ContextRecreated(obj);
}

void MenuScreen_Init(GuiElement* elem) {
	MenuScreen* screen = (MenuScreen*)elem;

	if (screen->TitleFont.Handle == NULL) {
		Platform_MakeFont(&screen->TitleFont, &Game_FontName, 16, FONT_STYLE_BOLD);
	}
	if (screen->TextFont.Handle == NULL) {
		Platform_MakeFont(&screen->TextFont, &Game_FontName, 16, FONT_STYLE_NORMAL);
	}

	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, MenuScreen_ContextLost_Callback);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, MenuScreen_ContextRecreated_Callback);
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
	screen->ContextLost(screen);

	if (screen->TitleFont.Handle != NULL) {
		Platform_FreeFont(&screen->TitleFont);
	}
	if (screen->TextFont.Handle != NULL) {
		Platform_FreeFont(&screen->TextFont);
	}

	Event_UnregisterVoid(&GfxEvents_ContextLost,      screen, MenuScreen_ContextLost_Callback);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, screen, MenuScreen_ContextRecreated_Callback);
}

void MenuScreen_OnResize(GuiElement* elem) {
	MenuScreen* screen = (MenuScreen*)elem;
	Menu_RepositionWidgets(screen->WidgetsPtr, screen->WidgetsCount);
}

void MenuScreen_MakeInstance(MenuScreen* screen, Widget** widgets, Int32 count, Menu_ContextFunc contextRecreated) {
	Platform_MemSet(screen, 0, sizeof(MenuScreen));
	screen->VTABLE = &MenuScreen_VTABLE;
	Screen_Reset((Screen*)screen);

	screen->VTABLE->HandlesKeyDown     = MenuScreen_HandlesKeyDown;
	screen->VTABLE->HandlesKeyUp       = MenuScreen_HandlesKeyUp;
	screen->VTABLE->HandlesKeyPress    = MenuScreen_HandlesKeyPress;
	screen->VTABLE->HandlesMouseDown   = MenuScreen_HandlesMouseDown;
	screen->VTABLE->HandlesMouseUp     = MenuScreen_HandlesMouseUp;
	screen->VTABLE->HandlesMouseMove   = MenuScreen_HandlesMouseMove;
	screen->VTABLE->HandlesMouseScroll = MenuScreen_HandlesMouseScroll;

	screen->OnResize       = MenuScreen_OnResize;
	screen->VTABLE->Init   = MenuScreen_Init;
	screen->VTABLE->Render = MenuScreen_Render;
	screen->VTABLE->Free   = MenuScreen_Free;

	screen->HandlesAllInput  = true;
	screen->WidgetsPtr       = widgets;
	screen->WidgetsCount     = count;
	screen->ContextLost      = MenuScreen_ContextLost;
	screen->ContextRecreated = contextRecreated;
}


/*########################################################################################################################*
*-------------------------------------------------------PauseScreen-------------------------------------------------------*
*#########################################################################################################################*/
GuiElementVTABLE PauseScreen_VTABLE;
PauseScreen PauseScreen_Instance;
void PauseScreen_Make(PauseScreen* screen, Int32 i, Int32 dir, Int32 y, const UInt8* title, Widget_LeftClick onClick) {	
	ButtonWidget* btn = &screen->Buttons[i];
	screen->Widgets[i] = (Widget*)btn;

	String text = String_FromReadonly(title);
	ButtonWidget_Create(btn, 300, &text, &screen->TitleFont, onClick);
	Widget_SetLocation((Widget*)btn, ANCHOR_CENTRE, ANCHOR_CENTRE, dir * 160, y);
}

void PauseScreen_MakeClassic(PauseScreen* screen, Int32 i, Int32 y, const UInt8* title, Widget_LeftClick onClick) {	
	ButtonWidget* btn = &screen->Buttons[i];
	screen->Widgets[i] = (Widget*)btn;

	String text = String_FromReadonly(title);
	ButtonWidget_Create(btn, 400, &text, &screen->TitleFont, onClick);
	Widget_SetLocation((Widget*)btn, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, y);
}

void PauseScreen_Quit(GuiElement* a, GuiElement* b) { Window_Close(); }
void PauseScreen_Game(GuiElement* a, GuiElement* b) { Gui_ReplaceActive(NULL); }

void PauseScreen_CheckHacksAllowed(void* obj) {
	if (Game_UseClassicOptions) return;
	PauseScreen* screen = (PauseScreen*)obj;
	screen->Buttons[4].Disabled = !LocalPlayer_Instance.Hacks.CanAnyHacks; /* select texture pack */
}

void PauseScreen_ContextRecreated(void* obj) {
	PauseScreen* screen = (PauseScreen*)obj;
	FontDesc* font = &screen->TitleFont;

	if (Game_UseClassicOptions) {
		PauseScreen_MakeClassic(screen, 0, -100, "Options...",            Menu_SwitchClassicOptions);
		PauseScreen_MakeClassic(screen, 1,  -50, "Generate new level...", Menu_SwitchClassicGenLevel);
		PauseScreen_MakeClassic(screen, 2,    0, "Load level...",         Menu_SwitchLoadLevel);
		PauseScreen_MakeClassic(screen, 3,   50, "Save level...",         Menu_SwitchSaveLevel);
		PauseScreen_MakeClassic(screen, 4,  150, "Nostalgia options...",  Menu_SwitchNostalgia);

		String back = String_FromConst("Back to game");
		screen->Widgets[5] = (Widget*)(&screen->Buttons[5]);
		Menu_MakeBack(&screen->Buttons[5], 400, &back, 25, font, PauseScreen_Game);	

		/* Disable nostalgia options in classic mode*/
		if (Game_ClassicMode) MenuScreen_Remove((MenuScreen*)screen, 4);
		screen->Widgets[6] = NULL;
		screen->Widgets[7] = NULL;
	} else {
		PauseScreen_Make(screen, 0, -1, -50, "Options...",             Menu_SwitchOptions);
		PauseScreen_Make(screen, 1,  1, -50, "Generate new level...",  Menu_SwitchGenLevel);
		PauseScreen_Make(screen, 2,  1,   0, "Load level...",          Menu_SwitchLoadLevel);
		PauseScreen_Make(screen, 3,  1,  50, "Save level...",          Menu_SwitchSaveLevel);
		PauseScreen_Make(screen, 4, -1,   0, "Change texture pack...", Menu_SwitchTexPacks);
		PauseScreen_Make(screen, 5, -1,  50, "Hotkeys...",             Menu_SwitchHotkeys);

		String quitMsg = String_FromConst("Quit game");
		screen->Widgets[6] = (Widget*)(&screen->Buttons[6]);
		ButtonWidget_Create(&screen->Buttons[6], 120, &quitMsg, font, PauseScreen_Quit);		
		Widget_SetLocation(screen->Widgets[6], ANCHOR_MAX, ANCHOR_MAX, 5, 5);

		screen->Widgets[7] = (Widget*)(&screen->Buttons[7]);
		Menu_MakeDefaultBack(&screen->Buttons[7], true, font, PauseScreen_Game);
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
	Event_RegisterVoid(&UserEvents_HackPermissionsChanged, screen, PauseScreen_CheckHacksAllowed);
	screen->ContextRecreated(elem);
}

void PauseScreen_Free(GuiElement* elem) {
	PauseScreen* screen = (PauseScreen*)elem;
	MenuScreen_Free(elem);
	Event_UnregisterVoid(&UserEvents_HackPermissionsChanged, screen, PauseScreen_CheckHacksAllowed);
}

Screen* PauseScreen_MakeInstance(void) {
	PauseScreen* screen = &PauseScreen_Instance;
	MenuScreen_MakeInstance((MenuScreen*)screen, screen->Widgets, 
		Array_Elems(screen->Widgets), PauseScreen_ContextRecreated);
	PauseScreen_VTABLE = *screen->VTABLE;
	screen->VTABLE = &PauseScreen_VTABLE;

	screen->VTABLE->Init           = PauseScreen_Init;
	screen->VTABLE->Free           = PauseScreen_Free;
	return (Screen*)screen;
}


/*########################################################################################################################*
*--------------------------------------------------OptionsGroupScreen-----------------------------------------------------*
*#########################################################################################################################*/
GuiElementVTABLE OptionsGroupScreen_VTABLE;
OptionsGroupScreen OptionsGroupScreen_Instance;
const UInt8* optsGroup_descs[7] = {
	"&eMusic/Sound, view bobbing, and more",
	"&eChat options, gui scale, font settings, and more",
	"&eFPS limit, view distance, entity names/shadows",
	"&eSet key bindings, bind keys to act as mouse clicks",
	"&eHacks allowed, jump settings, and more",
	"&eEnv colours, water level, weather, and more",
	"&eSettings for resembling the original classic",
};

void OptionsGroupScreen_CheckHacksAllowed(void* obj) {
	OptionsGroupScreen* screen = (OptionsGroupScreen*)obj;
	screen->Buttons[5].Disabled = !LocalPlayer_Instance.Hacks.CanAnyHacks; /* env settings */
}

void OptionsGroupScreen_Make(OptionsGroupScreen* screen, Int32 i, Int32 dir, Int32 y, const UInt8* title, Widget_LeftClick onClick) {	
	ButtonWidget* btn = &screen->Buttons[i];
	screen->Widgets[i] = (Widget*)btn;

	String text = String_FromReadonly(title);
	ButtonWidget_Create(btn, 300, &text, &screen->TitleFont, onClick);
	Widget_SetLocation((Widget*)btn, ANCHOR_CENTRE, ANCHOR_CENTRE, dir * 160, y);
}

void OptionsGroupScreen_MakeDesc(OptionsGroupScreen* screen) {
	screen->Widgets[8] = (Widget*)(&screen->Desc);
	String text = String_FromReadonly(optsGroup_descs[screen->SelectedI]);
	TextWidget_Create(&screen->Desc, &text, &screen->TextFont);
	Widget_SetLocation((Widget*)(&screen->Desc), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 100);
}

void OptionsGroupScreen_ContextRecreated(void* obj) {
	OptionsGroupScreen* screen = (OptionsGroupScreen*)obj;

	OptionsGroupScreen_Make(screen, 0, -1, -100, "Misc options...",      Menu_SwitchMisc);
	OptionsGroupScreen_Make(screen, 1, -1,  -50, "Gui options...",       Menu_SwitchGui);
	OptionsGroupScreen_Make(screen, 2, -1,    0, "Graphics options...",  Menu_SwitchGfx);
	OptionsGroupScreen_Make(screen, 3, -1,   50, "Controls...",          Menu_SwitchKeysNormal);
	OptionsGroupScreen_Make(screen, 4,  1,  -50, "Hacks settings...",    Menu_SwitchHacks);
	OptionsGroupScreen_Make(screen, 5,  1,    0, "Env settings...",      Menu_SwitchEnv);
	OptionsGroupScreen_Make(screen, 6,  1,   50, "Nostalgia options...", Menu_SwitchNostalgia);

	screen->Widgets[7] = (Widget*)(&screen->Buttons[7]);
	Menu_MakeDefaultBack(&screen->Buttons[7], false, &screen->TitleFont, Menu_SwitchPause);	
	screen->Widgets[8] = NULL; /* Description text widget placeholder */

	if (screen->SelectedI >= 0) { OptionsGroupScreen_MakeDesc(screen); }
	OptionsGroupScreen_CheckHacksAllowed(obj);
}

void OptionsGroupScreen_Init(GuiElement* elem) {
	OptionsGroupScreen* screen = (OptionsGroupScreen*)elem;
	MenuScreen_Init(elem);
	Event_RegisterVoid(&UserEvents_HackPermissionsChanged, screen, OptionsGroupScreen_CheckHacksAllowed);
	screen->ContextRecreated(elem);
}

void OptionsGroupScreen_Free(GuiElement* elem) {
	OptionsGroupScreen* screen = (OptionsGroupScreen*)elem;
	MenuScreen_Free(elem);
	Event_UnregisterVoid(&UserEvents_HackPermissionsChanged, screen, OptionsGroupScreen_CheckHacksAllowed);
}

bool OptionsGroupScreen_HandlesMouseMove(GuiElement* elem, Int32 x, Int32 y) {
	OptionsGroupScreen* screen = (OptionsGroupScreen*)elem;
	Int32 i = Menu_HandleMouseMove(screen->WidgetsPtr, screen->WidgetsCount, x, y);
	if (i == -1 || i == screen->SelectedI) return true;
	if (i >= Array_Elems(optsGroup_descs)) return true;

	screen->SelectedI = i;
	Elem_TryFree(&screen->Desc);
	OptionsGroupScreen_MakeDesc(screen);
	return true;
}

Screen* OptionsGroupScreen_MakeInstance(void) {
	OptionsGroupScreen* screen = &OptionsGroupScreen_Instance;
	MenuScreen_MakeInstance((MenuScreen*)screen, screen->Widgets, 
		Array_Elems(screen->Widgets), OptionsGroupScreen_ContextRecreated);
	OptionsGroupScreen_VTABLE = *screen->VTABLE;
	screen->VTABLE = &OptionsGroupScreen_VTABLE;

	screen->VTABLE->Init = OptionsGroupScreen_Init;
	screen->VTABLE->Free = OptionsGroupScreen_Free;
	screen->VTABLE->HandlesMouseMove = OptionsGroupScreen_HandlesMouseMove;

	screen->SelectedI = -1;
	return (Screen*)screen;
}


/*########################################################################################################################*
*------------------------------------------------------DeathScreen--------------------------------------------------------*
*#########################################################################################################################*/
GuiElementVTABLE DeathScreen_VTABLE;
DeathScreen DeathScreen_Instance;
void DeathScreen_Init(GuiElement* elem) {
	DeathScreen* screen = (DeathScreen*)elem;
	Platform_MakeFont(&screen->TextFont, &Game_FontName, 40, FONT_STYLE_NORMAL);
	MenuScreen_Init(elem);
	screen->ContextRecreated(elem);
}

bool DeathScreen_HandlesKeyDown(GuiElement* elem, Key key) { return true; }

void DeathScreen_ContextRecreated(void* obj) {
	DeathScreen* screen = (DeathScreen*)obj;

	String title = String_FromConst("Game over!");
	screen->Widgets[0] = (Widget*)(&screen->Title);
	TextWidget_Create(&screen->Title, &title, &screen->TextFont);
	Widget_SetLocation(screen->Widgets[0], ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -150);

	String score = String_FromRawArray(Chat_Status[1].Buffer);
	screen->Widgets[1] = (Widget*)(&screen->Score);
	TextWidget_Create(&screen->Score, &score, &screen->TitleFont);
	Widget_SetLocation(screen->Widgets[1], ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -75);

	String gen = String_FromConst("Generate new level...")
	screen->Widgets[2] = (Widget*)(&screen->Gen);
	ButtonWidget_Create(&screen->Gen, 400, &gen, &screen->TitleFont, Menu_SwitchGenLevel);
	Widget_SetLocation(screen->Widgets[2], ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 25);

	String load = String_FromConst("Load level...");
	screen->Widgets[3] = (Widget*)(&screen->Load);
	ButtonWidget_Create(&screen->Load, 400, &load, &screen->TitleFont, Menu_SwitchLoadLevel);
	Widget_SetLocation(screen->Widgets[3], ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 75);
}

Screen* DeathScreen_MakeInstance(void) {
	DeathScreen* screen = &DeathScreen_Instance;
	MenuScreen_MakeInstance((MenuScreen*)screen, screen->Widgets, 
		Array_Elems(screen->Widgets), DeathScreen_ContextRecreated);
	DeathScreen_VTABLE = *screen->VTABLE;
	screen->VTABLE = &DeathScreen_VTABLE;

	screen->VTABLE->Init           = DeathScreen_Init;
	screen->VTABLE->HandlesKeyDown = DeathScreen_HandlesKeyDown;
	return (Screen*)screen;
}


/*########################################################################################################################*
*----------------------------------------------------EditHotkeyScreen-----------------------------------------------------*
*#########################################################################################################################*/
GuiElementVTABLE EditHotkeyScreen_VTABLE;
EditHotkeyScreen EditHotkeyScreen_Instance;
void EditHotkeyScreen_Make(EditHotkeyScreen* screen, Int32 i, Int32 x, Int32 y, STRING_PURE String* text, Widget_LeftClick onClick) {
	ButtonWidget* btn = &screen->Buttons[i];
	screen->Widgets[i] = (Widget*)btn;

	ButtonWidget_Create(btn, 300, text, &screen->TitleFont, onClick);
	Widget_SetLocation((Widget*)btn, ANCHOR_CENTRE, ANCHOR_CENTRE, x, y);
}

void EditHotkeyScreen_MakeFlags(UInt8 flags, STRING_TRANSIENT String* str) {
	if (flags == 0) String_AppendConst(str, " None");
	if (flags & HOTKEYS_FLAG_CTRL)  String_AppendConst(str, " Ctrl");
	if (flags & HOTKEYS_FLAG_SHIFT) String_AppendConst(str, " Shift");
	if (flags & HOTKEYS_FLAG_ALT)   String_AppendConst(str, " Alt");
}

void EditHotkeyScreen_MakeBaseKey(EditHotkeyScreen* screen, Widget_LeftClick onClick) {
	UInt8 textBuffer[String_BufferSize(STRING_SIZE)];
	String text = String_InitAndClearArray(textBuffer);

	String_AppendConst(&text, "Key: ");
	String_AppendConst(&text, Key_Names[screen->CurHotkey.BaseKey]);
	EditHotkeyScreen_Make(screen, 0, 0, -150, &text, onClick);
}

void EditHotkeyScreen_MakeModifiers(EditHotkeyScreen* screen, Widget_LeftClick onClick) {
	UInt8 textBuffer[String_BufferSize(STRING_SIZE)];
	String text = String_InitAndClearArray(textBuffer);

	String_AppendConst(&text, "Modifiers:");
	EditHotkeyScreen_MakeFlags(screen->CurHotkey.Flags, &text);
	EditHotkeyScreen_Make(screen, 1, 0, -100, &text, onClick);
}

void EditHotkeyScreen_MakeLeaveOpen(EditHotkeyScreen* screen, Widget_LeftClick onClick) {
	UInt8 textBuffer[String_BufferSize(STRING_SIZE)];
	String text = String_InitAndClearArray(textBuffer);

	String_AppendConst(&text, "Input stays open: ");
	String_AppendConst(&text, screen->CurHotkey.StaysOpen ? "ON" : "OFF");
	EditHotkeyScreen_Make(screen, 2, -100, 10, &text, onClick);
}

void EditHotkeyScreen_BaseKey(GuiElement* elem, GuiElement* widget) {
	EditHotkeyScreen* screen = (EditHotkeyScreen*)elem;
	screen->SelectedI = 0;
	screen->SupressNextPress = true;
	String msg = String_FromConst("Key: press a key..");
	ButtonWidget_SetText(&screen->Buttons[0], &msg);
}

void EditHotkeyScreen_Modifiers(GuiElement* elem, GuiElement* widget) {
	EditHotkeyScreen* screen = (EditHotkeyScreen*)elem;
	screen->SelectedI = 1;
	screen->SupressNextPress = true;
	String msg = String_FromConst("Modifiers: press a key..");
	ButtonWidget_SetText(&screen->Buttons[1], &msg);
}

void EditHotkeyScreen_LeaveOpen(GuiElement* elem, GuiElement* widget) {
	EditHotkeyScreen* screen = (EditHotkeyScreen*)elem;
	/* Reset 'waiting for key..' state of two other buttons */
	if (screen->SelectedI == 0) {
		EditHotkeyScreen_MakeBaseKey(screen, EditHotkeyScreen_BaseKey);
		screen->SupressNextPress = false;
	} else if (screen->SelectedI == 1) {
		EditHotkeyScreen_MakeModifiers(screen, EditHotkeyScreen_Modifiers);
		screen->SupressNextPress = false;
	}

	screen->SelectedI = -1;
	screen->CurHotkey.StaysOpen = !screen->CurHotkey.StaysOpen;
	EditHotkeyScreen_MakeLeaveOpen(screen, EditHotkeyScreen_LeaveOpen);
}

void EditHotkeyScreen_SaveChanges(GuiElement* elem, GuiElement* widget) {
	EditHotkeyScreen* screen = (EditHotkeyScreen*)elem;
	HotkeyData hotkey = screen->OrigHotkey;
	if (hotkey.BaseKey != Key_None) {
		Hotkeys_Remove(hotkey.BaseKey, hotkey.Flags);
		Hotkeys_UserRemovedHotkey(hotkey.BaseKey, hotkey.Flags);
	}

	hotkey = screen->CurHotkey;
	if (hotkey.BaseKey != Key_None) {
		String text = screen->Input.Base.Text;
		Hotkeys_Add(hotkey.BaseKey, hotkey.Flags, &text, hotkey.StaysOpen);
		Hotkeys_UserAddedHotkey(hotkey.BaseKey, hotkey.Flags, hotkey.StaysOpen, &text);
	}
	Gui_ReplaceActive(HotkeyListScreen_MakeInstance());
}

void EditHotkeyScreen_RemoveHotkey(GuiElement* elem, GuiElement* widget) {
	EditHotkeyScreen* screen = (EditHotkeyScreen*)elem;
	HotkeyData hotkey = screen->OrigHotkey;
	if (hotkey.BaseKey != Key_None) {
		Hotkeys_Remove(hotkey.BaseKey, hotkey.Flags);
		Hotkeys_UserRemovedHotkey(hotkey.BaseKey, hotkey.Flags);
	}
	Gui_ReplaceActive(HotkeyListScreen_MakeInstance());
}

void EditHotkeyScreen_Init(GuiElement* elem) {
	EditHotkeyScreen* screen = (EditHotkeyScreen*)elem;
	MenuScreen_Init(elem);
	Key_KeyRepeat = true;
	screen->ContextRecreated(elem);
}

void EditHotkeyScreen_Render(GuiElement* elem, Real64 delta) {
	MenuScreen_Render(elem, delta);
	Int32 cX = Game_Width / 2, cY = Game_Height / 2;
	PackedCol grey = PACKEDCOL_CONST(150, 150, 150, 255);
	GfxCommon_Draw2DFlat(cX - 250, cY - 65, 500, 2, grey);
	GfxCommon_Draw2DFlat(cX - 250, cY + 45, 500, 2, grey);
}

void EditHotkeyScreen_Free(GuiElement* elem) {
	EditHotkeyScreen* screen = (EditHotkeyScreen*)elem;
	Key_KeyRepeat = false;
	screen->SelectedI = -1;
	MenuScreen_Free(elem);
}

bool EditHotkeyScreen_HandlesKeyPress(GuiElement* elem, UInt8 key) {
	EditHotkeyScreen* screen = (EditHotkeyScreen*)elem;
	if (screen->SupressNextPress) {
		screen->SupressNextPress = false;
		return true;
	}
	return Elem_HandlesKeyPress(&screen->Input.Base, key);
}

bool EditHotkeyScreen_HandlesKeyDown(GuiElement* elem, Key key) {
	EditHotkeyScreen* screen = (EditHotkeyScreen*)elem;
	if (screen->SelectedI >= 0) {
		if (screen->SelectedI == 0) {
			screen->CurHotkey.BaseKey = key;
			EditHotkeyScreen_MakeBaseKey(screen, EditHotkeyScreen_BaseKey);
		} else if (screen->SelectedI == 1) {
			if      (key == Key_ControlLeft || key == Key_ControlRight) screen->CurHotkey.Flags |= HOTKEYS_FLAG_CTRL;
			else if (key == Key_ShiftLeft   || key == Key_ShiftRight)   screen->CurHotkey.Flags |= HOTKEYS_FLAG_SHIFT;
			else if (key == Key_AltLeft     || key == Key_AltRight)     screen->CurHotkey.Flags |= HOTKEYS_FLAG_ALT;
			else screen->CurHotkey.Flags = 0;

			EditHotkeyScreen_MakeModifiers(screen, EditHotkeyScreen_Modifiers);
		}

		screen->SupressNextPress = true;
		screen->SelectedI = -1;
		return true;
	}
	return Elem_HandlesKeyDown(&screen->Input.Base, key) || MenuScreen_HandlesKeyDown(elem, key);
}

bool EditHotkeyScreen_HandlesKeyUp(GuiElement* elem, Key key) {
	EditHotkeyScreen* screen = (EditHotkeyScreen*)elem;
	return Elem_HandlesKeyUp(&screen->Input.Base, key);
}

void EditHotkeyScreen_ContextRecreated(void* obj) {
	EditHotkeyScreen* screen = (EditHotkeyScreen*)obj;
	MenuInputValidator validator = MenuInputValidator_String();
	String text = String_MakeNull();

	bool existed = screen->OrigHotkey.BaseKey != Key_None;
	if (existed) {
		text = StringsBuffer_UNSAFE_Get(&HotkeysText, screen->OrigHotkey.TextIndex);
	}

	EditHotkeyScreen_MakeBaseKey(screen, EditHotkeyScreen_BaseKey);
	EditHotkeyScreen_MakeModifiers(screen, EditHotkeyScreen_Modifiers);
	EditHotkeyScreen_MakeLeaveOpen(screen, EditHotkeyScreen_LeaveOpen);

	String addText = String_FromReadonly(existed ? "Save changes" : "Add hotkey");
	EditHotkeyScreen_Make(screen, 3, 0, 80, &addText, EditHotkeyScreen_SaveChanges);
	String remText = String_FromReadonly(existed ? "Remove hotkey" : "Cancel");
	EditHotkeyScreen_Make(screen, 4, 0, 130, &remText, EditHotkeyScreen_RemoveHotkey);

	screen->Widgets[5] = (Widget*)(&screen->Buttons[5]);
	Menu_MakeDefaultBack(&screen->Buttons[5], false, &screen->TitleFont, Menu_SwitchPause);

	screen->Widgets[6] = (Widget*)(&screen->Input);
	MenuInputWidget_Create(&screen->Input, 500, 30, &text, &screen->TextFont, &validator);
	Widget_SetLocation(screen->Widgets[6], ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -35);
	screen->Input.Base.ShowCaret = true;
}

Screen* EditHotkeyScreen_MakeInstance(HotkeyData original) {
	EditHotkeyScreen* screen = &EditHotkeyScreen_Instance;
	MenuScreen_MakeInstance((MenuScreen*)screen, screen->Widgets, 
		Array_Elems(screen->Widgets), EditHotkeyScreen_ContextRecreated);
	EditHotkeyScreen_VTABLE = *screen->VTABLE;
	screen->VTABLE = &EditHotkeyScreen_VTABLE;

	screen->VTABLE->Init   = EditHotkeyScreen_Init;
	screen->VTABLE->Render = EditHotkeyScreen_Render;
	screen->VTABLE->Free   = EditHotkeyScreen_Free;
	screen->VTABLE->HandlesKeyPress = EditHotkeyScreen_HandlesKeyPress;
	screen->VTABLE->HandlesKeyDown  = EditHotkeyScreen_HandlesKeyDown;
	screen->VTABLE->HandlesKeyUp    = EditHotkeyScreen_HandlesKeyUp;
	
	screen->SelectedI  = -1;
	screen->OrigHotkey = original;
	screen->CurHotkey  = original;
	return (Screen*)screen;
}


/*########################################################################################################################*
*-----------------------------------------------------GenLevelScreen------------------------------------------------------*
*#########################################################################################################################*/
GuiElementVTABLE GenLevelScreen_VTABLE;
GenLevelScreen GenLevelScreen_Instance;
Int32 GenLevelScreen_GetInt(GenLevelScreen* screen, Int32 index) {
	MenuInputWidget* input = &screen->Inputs[index];
	String text = input->Base.Text;

	if (!input->Validator.IsValidValue(&input->Validator, &text)) return 0;
	Int32 value; Convert_TryParseInt32(&text, &value); return value;
}

Int32 GenLevelScreen_GetSeedInt(GenLevelScreen* screen, Int32 index) {
	MenuInputWidget* input = &screen->Inputs[index];
	String text = input->Base.Text;

	if (text.length == 0) {
		Random rnd; Random_InitFromCurrentTime(&rnd);
		return Random_Next(&rnd, Int32_MaxValue);
	}

	if (!input->Validator.IsValidValue(&input->Validator, &text)) return 0;
	Int32 value; Convert_TryParseInt32(&text, &value); return value;
}

void GenLevelScreen_Gen(GenLevelScreen* screen, bool vanilla) {
	Int32 width  = GenLevelScreen_GetInt(screen, 0);
	Int32 height = GenLevelScreen_GetInt(screen, 1);
	Int32 length = GenLevelScreen_GetInt(screen, 2);
	Int32 seed   = GenLevelScreen_GetSeedInt(screen, 3);

	Int64 volume = (Int64)width * height * length;
	if (volume > Int32_MaxValue) {
		String msg = String_FromConst("&cThe generated map's volume is too big.");
		Chat_Add(&msg);
	} else if (width == 0 || height == 0 || length == 0) {
		String msg = String_FromConst("&cOne of the map dimensions is invalid.");
		Chat_Add(&msg);
	} else {
		ServerConnection_BeginGeneration(width, height, length, seed, vanilla);
	}
}

void GenLevelScreen_Flatgrass(GuiElement* elem, GuiElement* widget) {
	GenLevelScreen* screen = (GenLevelScreen*)elem;
	GenLevelScreen_Gen(screen, false);
}

void GenLevelScreen_Notchy(GuiElement* elem, GuiElement* widget) {
	GenLevelScreen* screen = (GenLevelScreen*)elem;
	GenLevelScreen_Gen(screen, true);
}

void GenLevelScreen_InputClick(GuiElement* elem, GuiElement* widget) {
	GenLevelScreen* screen = (GenLevelScreen*)elem;
	if (screen->Selected != NULL) screen->Selected->Base.ShowCaret = false;

	screen->Selected = (MenuInputWidget*)widget;
	Elem_HandlesMouseDown(&screen->Selected->Base, Mouse_X, Mouse_Y, MouseButton_Left);
	screen->Selected->Base.ShowCaret = true;
}

void GenLevelScreen_Input(GenLevelScreen* screen, Int32 i, Int32 y, bool seed, STRING_TRANSIENT String* value) {
	MenuInputWidget* input = &screen->Inputs[i];
	screen->Widgets[i] = (Widget*)input;

	MenuInputValidator validator = seed ? MenuInputValidator_Seed() : MenuInputValidator_Integer(1, 8192);
	MenuInputWidget_Create(input, 200, 30, value, &screen->TextFont, &validator);
	Widget_SetLocation(screen->Widgets[i], ANCHOR_CENTRE, ANCHOR_CENTRE, 0, y);

	input->Base.MenuClick = GenLevelScreen_InputClick;
	String_Clear(value);
}

void GenLevelScreen_Label(GenLevelScreen* screen, Int32 i, Int32 x, Int32 y, const UInt8* title) {	
	TextWidget* label = &screen->Labels[i];	
	screen->Widgets[i + 4] = (Widget*)label;

	String text = String_FromReadonly(title);
	TextWidget_Create(label, &text, &screen->TextFont);
	Widget_SetLocation(screen->Widgets[i + 4], ANCHOR_CENTRE, ANCHOR_CENTRE, x, y);

	label->XOffset = -110 - label->Width / 2;
	Widget_Reposition(label);
	PackedCol col = PACKEDCOL_CONST(224, 224, 224, 255); label->Col = col;
}

void GenLevelScreen_Init(GuiElement* elem) {
	GenLevelScreen* screen = (GenLevelScreen*)elem;
	MenuScreen_Init(elem);
	Key_KeyRepeat = true;
	screen->ContextRecreated(elem);
}

void GenLevelScreen_Free(GuiElement* elem) {
	Key_KeyRepeat = false;
	MenuScreen_Free(elem);
}

bool GenLevelScreen_HandlesKeyDown(GuiElement* elem, Key key) {
	GenLevelScreen* screen = (GenLevelScreen*)elem;
	if (screen->Selected != NULL && Elem_HandlesKeyDown(&screen->Selected->Base, key)) return true;
	return MenuScreen_HandlesKeyDown(elem, key);
}

bool GenLevelScreen_HandlesKeyUp(GuiElement* elem, Key key) {
	GenLevelScreen* screen = (GenLevelScreen*)elem;
	return screen->Selected == NULL || Elem_HandlesKeyUp(&screen->Selected->Base, key);
}

bool GenLevelScreen_HandlesKeyPress(GuiElement* elem, UInt8 key) {
	GenLevelScreen* screen = (GenLevelScreen*)elem;
	return screen->Selected == NULL || Elem_HandlesKeyPress(&screen->Selected->Base, key);
}

void GenLevelScreen_ContextRecreated(void* obj) {
	GenLevelScreen* screen = (GenLevelScreen*)obj;
	UInt8 tmpBuffer[String_BufferSize(STRING_SIZE)];
	String tmp = String_InitAndClearArray(tmpBuffer);

	String_AppendInt32(&tmp, World_Width);
	GenLevelScreen_Input(screen, 0, -80, false, &tmp);
	String_AppendInt32(&tmp, World_Height);
	GenLevelScreen_Input(screen, 1, -40, false, &tmp);
	String_AppendInt32(&tmp, World_Length);
	GenLevelScreen_Input(screen, 2,   0, false, &tmp);
	GenLevelScreen_Input(screen, 3,  40, true,  &tmp);

	GenLevelScreen_Label(screen, 0, -150, -80, "Width:");
	GenLevelScreen_Label(screen, 1, -150, -40, "Height:");
	GenLevelScreen_Label(screen, 2, -150,   0, "Length:");
	GenLevelScreen_Label(screen, 3, -140,  40, "Seed:");

	String gen = String_FromConst("Generate new level");
	screen->Widgets[8] = (Widget*)(&screen->Labels[4]);
	TextWidget_Create(&screen->Labels[4], &gen, &screen->TextFont);
	Widget_SetLocation(screen->Widgets[8], ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -130);

	String flatgrass = String_FromConst("Flatgrass");
	screen->Widgets[9] = (Widget*)(&screen->Buttons[0]);
	ButtonWidget_Create(&screen->Buttons[0], 200, &flatgrass, &screen->TitleFont, GenLevelScreen_Flatgrass);
	Widget_SetLocation(screen->Widgets[9], ANCHOR_CENTRE, ANCHOR_CENTRE, -120, 100);

	String vanilla = String_FromConst("Vanilla");
	screen->Widgets[10] = (Widget*)(&screen->Buttons[1]);
	ButtonWidget_Create(&screen->Buttons[1], 200, &vanilla, &screen->TitleFont, GenLevelScreen_Notchy);
	Widget_SetLocation(screen->Widgets[10], ANCHOR_CENTRE, ANCHOR_CENTRE, 120, 100);

	screen->Widgets[11] = (Widget*)(&screen->Buttons[2]);
	Menu_MakeDefaultBack(&screen->Buttons[2], false, &screen->TitleFont, Menu_SwitchPause);
}

Screen* GenLevelScreen_MakeInstance(void) {
	GenLevelScreen* screen = &GenLevelScreen_Instance;
	MenuScreen_MakeInstance((MenuScreen*)screen, screen->Widgets,
		Array_Elems(screen->Widgets), GenLevelScreen_ContextRecreated);
	GenLevelScreen_VTABLE = *screen->VTABLE;
	screen->VTABLE = &GenLevelScreen_VTABLE;

	screen->VTABLE->Init = GenLevelScreen_Init;
	screen->VTABLE->Free = GenLevelScreen_Free;
	screen->VTABLE->HandlesKeyDown  = GenLevelScreen_HandlesKeyDown;
	screen->VTABLE->HandlesKeyUp    = GenLevelScreen_HandlesKeyUp;
	screen->VTABLE->HandlesKeyPress = GenLevelScreen_HandlesKeyPress;
	return (Screen*)screen;
}


/*########################################################################################################################*
*----------------------------------------------------ClassicGenScreen-----------------------------------------------------*
*#########################################################################################################################*/
GuiElementVTABLE ClassicGenScreen_VTABLE;
ClassicGenScreen ClassicGenScreen_Instance;
void ClassicGenScreen_Gen(Int32 size) {
	Random rnd; Random_InitFromCurrentTime(&rnd);
	Int32 seed = Random_Next(&rnd, Int32_MaxValue);
	ServerConnection_BeginGeneration(size, 64, size, seed, true);
}

void ClassicGenScreen_Small(GuiElement* a, GuiElement* b)  { ClassicGenScreen_Gen(128); }
void ClassicGenScreen_Medium(GuiElement* a, GuiElement* b) { ClassicGenScreen_Gen(256); }
void ClassicGenScreen_Huge(GuiElement* a, GuiElement* b)   { ClassicGenScreen_Gen(512); }

void ClassicGenScreen_Make(ClassicGenScreen* screen, Int32 i, Int32 y, const UInt8* title, Widget_LeftClick onClick) {
	ButtonWidget* btn = &screen->Buttons[i];
	screen->Widgets[i] = (Widget*)btn;

	String text = String_FromReadonly(title);
	ButtonWidget_Create(btn, 400, &text, &screen->TitleFont, onClick);
	Widget_SetLocation((Widget*)btn, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, y);
}

void ClassicGenScreen_Init(GuiElement* elem) {
	ClassicGenScreen* screen = (ClassicGenScreen*)elem;
	MenuScreen_Init(elem);
	screen->ContextRecreated(elem);
}

void ClassicGenScreen_ContextRecreated(void* obj) {
	ClassicGenScreen* screen = (ClassicGenScreen*)obj;
	ClassicGenScreen_Make(screen, 0, -100, "Small",  ClassicGenScreen_Small);
	ClassicGenScreen_Make(screen, 1,  -50, "Normal", ClassicGenScreen_Medium);
	ClassicGenScreen_Make(screen, 2,    0, "Huge",   ClassicGenScreen_Huge);

	screen->Widgets[3] = (Widget*)(&screen->Buttons[3]);
	Menu_MakeDefaultBack(&screen->Buttons[3], false, &screen->TitleFont, Menu_SwitchPause);
}

Screen* ClassicGenScreen_MakeInstance(void) {
	ClassicGenScreen* screen = &ClassicGenScreen_Instance;
	MenuScreen_MakeInstance((MenuScreen*)screen, screen->Widgets, 
		Array_Elems(screen->Widgets), ClassicGenScreen_ContextRecreated);
	ClassicGenScreen_VTABLE = *screen->VTABLE;
	screen->VTABLE = &ClassicGenScreen_VTABLE;

	screen->VTABLE->Init = ClassicGenScreen_Init;
	return (Screen*)screen;
}


/*########################################################################################################################*
*----------------------------------------------------SaveLevelScreen------------------------------------------------------*
*#########################################################################################################################*/
SaveLevelScreen SaveLevelScreen_Instance;
GuiElementVTABLE SaveLevelScreen_VTABLE;
void SaveLevelScreen_RemoveOverwrites(SaveLevelScreen* screen) {
	ButtonWidget* btn = &screen->Buttons[0];
	if (btn->OptName != NULL) {
		btn->OptName = NULL; String save = String_FromConst("Save"); 
		ButtonWidget_SetText(btn, &save);
	}

	btn = &screen->Buttons[1];
	if (btn->OptName != NULL) {
		btn->OptName = NULL; String save = String_FromConst("Save schematic");
		ButtonWidget_SetText(btn, &save);
	}
}

void SaveLevelScreen_MakeDesc(SaveLevelScreen* screen, STRING_PURE String* text) {
	if (screen->Widgets[5] != NULL) { Elem_TryFree(screen->Widgets[5]); }

	TextWidget_Create(&screen->Desc, text, &screen->TextFont);
	Widget_SetLocation((Widget*)(&screen->Desc), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 65);
	screen->Widgets[5] = (Widget*)(&screen->Desc);
}

void SaveLevelScreen_DoSave(GuiElement* screenElem, GuiElement* widget, const UInt8* ext) {
	SaveLevelScreen* screen = (SaveLevelScreen*)screenElem;
	String file = screen->Input.Base.Text;
	if (file.length == 0) {
		String msg = String_FromConst("&ePlease enter a filename")
		SaveLevelScreen_MakeDesc(screen, &msg); return;
	}

	UInt8 pathBuffer[String_BufferSize(FILENAME_SIZE)];
	String path = String_InitAndClearArray(pathBuffer);
	String_Format2(&path, "maps/%s%c", &file, ext);

	ButtonWidget* btn = (ButtonWidget*)widget;
	if (Platform_FileExists(&path) && btn->OptName == NULL) {
		String warnMsg = String_FromConst("&cOverwrite existing?");
		ButtonWidget_SetText(btn, &warnMsg);
		btn->OptName = "O";
	} else {
		/* NOTE: We don't immediately save here, because otherwise the 'saving...'
		will not be rendered in time because saving is done on the main thread. */
		String warnMsg = String_FromConst("Saving..");
		SaveLevelScreen_MakeDesc(screen, &warnMsg);

		String_Set(&screen->TextPath, &path);
		SaveLevelScreen_RemoveOverwrites(screen);
	}
}

void SaveLevelScreen_Classic(GuiElement* screenElem, GuiElement* widget) {
	SaveLevelScreen_DoSave(screenElem, widget, ".cw");
}

void SaveLevelScreen_Schematic(GuiElement* screenElem, GuiElement* widget) {
	SaveLevelScreen_DoSave(screenElem, widget, ".schematic");
}

void SaveLevelScreen_Init(GuiElement* elem) {
	SaveLevelScreen* screen = (SaveLevelScreen*)elem;
	MenuScreen_Init(elem);
	Key_KeyRepeat = true;
	screen->ContextRecreated(elem);
}

void SaveLevelScreen_Render(GuiElement* elem, Real64 delta) {
	MenuScreen_Render(elem, delta);
	Int32 cX = Game_Width / 2, cY = Game_Height / 2;
	PackedCol grey = PACKEDCOL_CONST(150, 150, 150, 255);
	GfxCommon_Draw2DFlat(cX - 250, cY + 90, 500, 2, grey);

	SaveLevelScreen* screen = (SaveLevelScreen*)elem;
	if (screen->TextPath.length == 0) return;
	String path = screen->TextPath;

	void* file;
	ReturnCode code = Platform_FileCreate(&file, &path);
	ErrorHandler_CheckOrFail(code, "Saving map");
	Stream stream; Stream_FromFile(&stream, file, &path);

	String cw = String_FromConst(".cw");
	if (String_CaselessEnds(&path, &cw)) {
		Cw_Save(&stream);
	} else {
		Schematic_Save(&stream);
	}

	UInt8 msgBuffer[String_BufferSize(STRING_SIZE * 2)];
	String msg = String_InitAndClearArray(msgBuffer);
	String_Format1(&msg, "&eSaved map to: %s", &path);
	Chat_Add(&msg);

	Gui_ReplaceActive(PauseScreen_MakeInstance());
	String_Clear(&path);
}

void SaveLevelScreen_Free(GuiElement* elem) {
	Key_KeyRepeat = false;
	MenuScreen_Free(elem);
}

bool SaveLevelScreen_HandlesKeyPress(GuiElement* elem, UInt8 key) {
	SaveLevelScreen* screen = (SaveLevelScreen*)elem;
	SaveLevelScreen_RemoveOverwrites(screen);

	return Elem_HandlesKeyPress(&screen->Input.Base, key);
}

bool SaveLevelScreen_HandlesKeyDown(GuiElement* elem, Key key) {
	SaveLevelScreen* screen = (SaveLevelScreen*)elem;
	SaveLevelScreen_RemoveOverwrites(screen);

	if (Elem_HandlesKeyDown(&screen->Input.Base, key)) return true;
	return MenuScreen_HandlesKeyDown(elem, key);
}

bool SaveLevelScreen_HandlesKeyUp(GuiElement* elem, Key key) {
	SaveLevelScreen* screen = (SaveLevelScreen*)elem;
	return Elem_HandlesKeyUp(&screen->Input.Base, key);
}

void SaveLevelScreen_ContextRecreated(void* obj) {
	SaveLevelScreen* screen = (SaveLevelScreen*)obj;

	String save = String_FromConst("Save");
	ButtonWidget_Create(&screen->Buttons[0], 300, &save, &screen->TitleFont, SaveLevelScreen_Classic);
	Widget_SetLocation((Widget*)(&screen->Buttons[0]), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 20);
	screen->Widgets[0] = (Widget*)(&screen->Buttons[0]);

	String schematic = String_FromConst("Save schematic");
	ButtonWidget_Create(&screen->Buttons[1], 200, &schematic, &screen->TitleFont, SaveLevelScreen_Schematic);
	Widget_SetLocation((Widget*)(&screen->Buttons[1]), ANCHOR_CENTRE, ANCHOR_CENTRE, -150, 120);
	screen->Widgets[1] = (Widget*)(&screen->Buttons[1]);

	String mcEdit = String_FromConst("&eCan be imported into MCEdit");
	TextWidget_Create(&screen->MCEdit, &mcEdit, &screen->TextFont);
	Widget_SetLocation((Widget*)(&screen->MCEdit), ANCHOR_CENTRE, ANCHOR_CENTRE, 110, 120);
	screen->Widgets[2] = (Widget*)(&screen->MCEdit);

	Menu_MakeDefaultBack(&screen->Buttons[2], false, &screen->TitleFont, Menu_SwitchPause);
	screen->Widgets[3] = (Widget*)(&screen->Buttons[2]);

	MenuInputValidator validator = MenuInputValidator_Path();
	String inputText = String_MakeNull();
	MenuInputWidget_Create(&screen->Input, 500, 30, &inputText, &screen->TextFont, &validator);
	Widget_SetLocation((Widget*)(&screen->Input), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -30);
	screen->Input.Base.ShowCaret = true;
	screen->Widgets[4] = (Widget*)(&screen->Input);

	screen->Widgets[5] = NULL; /* description widget placeholder */
}

Screen* SaveLevelScreen_MakeInstance(void) {
	SaveLevelScreen* screen = &SaveLevelScreen_Instance;
	MenuScreen_MakeInstance((MenuScreen*)screen, screen->Widgets, Array_Elems(screen->Widgets), SaveLevelScreen_ContextRecreated);
	SaveLevelScreen_VTABLE = *screen->VTABLE;
	screen->VTABLE = &SaveLevelScreen_VTABLE;

	screen->VTABLE->Init   = SaveLevelScreen_Init;
	screen->VTABLE->Render = SaveLevelScreen_Render;
	screen->VTABLE->Free   = SaveLevelScreen_Free;

	screen->VTABLE->HandlesKeyDown  = SaveLevelScreen_HandlesKeyDown;
	screen->VTABLE->HandlesKeyPress = SaveLevelScreen_HandlesKeyPress;
	screen->VTABLE->HandlesKeyUp    = SaveLevelScreen_HandlesKeyUp;
	return (Screen*)screen;
}


/*########################################################################################################################*
*---------------------------------------------------TexturePackScreen-----------------------------------------------------*
*#########################################################################################################################*/
void TexturePackScreen_EntryClick(GuiElement* screenElem, GuiElement* w) {
	ListScreen* screen = (ListScreen*)screenElem;
	UInt8 pathBuffer[String_BufferSize(FILENAME_SIZE)];
	String path = String_InitAndClearArray(pathBuffer);

	String filename = ListScreen_UNSAFE_GetCur(screen, w);
	String_Format2(&path, "texpacks%r%s", &Platform_DirectorySeparator, &filename);
	if (!Platform_FileExists(&path)) return;
	
	Int32 curPage = screen->CurrentIndex;
	Game_SetDefaultTexturePack(&filename);
	TexturePack_ExtractDefault();
	Elem_Recreate(screen);
	ListScreen_SetCurrentIndex(screen, curPage);
}

void TexturePackScreen_SelectEntry(STRING_PURE String* filename, void* obj) {
	String zip = String_FromConst(".zip");
	if (!String_CaselessEnds(filename, &zip)) return;

	StringsBuffer* entries = (StringsBuffer*)obj;
	StringsBuffer_Add(entries, filename);
}

Screen* TexturePackScreen_MakeInstance(void) {
	ListScreen* screen = ListScreen_MakeInstance();
	String title = String_FromConst("Select a texture pack zip"); screen->TitleText = title;
	screen->EntryClick = TexturePackScreen_EntryClick;

	String path = String_FromConst("texpacks");
	Platform_EnumFiles(&path, &screen->Entries, TexturePackScreen_SelectEntry);
	if (screen->Entries.Count > 0) {
		ListScreen_QuickSort(0, screen->Entries.Count - 1);
	}
	return (Screen*)screen;
}


/*########################################################################################################################*
*---------------------------------------------------HotkeyListScreen------------------------------------------------------*
*#########################################################################################################################*/
/* TODO: Hotkey added event for CPE */
void HotkeyListScreen_EntryClick(GuiElement* screenElem, GuiElement* w) {
	ListScreen* screen = (ListScreen*)screenElem;
	String text = ListScreen_UNSAFE_GetCur(screen, w);
	HotkeyData original = { 0 };

	if (String_CaselessEqualsConst(&text, LIST_SCREEN_EMPTY)) {
		Gui_ReplaceActive(EditHotkeyScreen_MakeInstance(original)); return;
	}

	Int32 sepIndex = String_IndexOf(&text, '|', 0);
	String key = String_UNSAFE_Substring(&text, 0, sepIndex - 1);
	String value = String_UNSAFE_SubstringAt(&text, sepIndex + 1);
	
	String ctrl  = String_FromConst("Ctrl");
	String shift = String_FromConst("Shift");
	String alt   = String_FromConst("Alt");

	UInt8 flags = 0;
	if (String_ContainsString(&value, &ctrl))  flags |= HOTKEYS_FLAG_CTRL;
	if (String_ContainsString(&value, &shift)) flags |= HOTKEYS_FLAG_SHIFT;
	if (String_ContainsString(&value, &alt))   flags |= HOTKEYS_FLAG_ALT;

	Key baseKey = Utils_ParseEnum(&key, Key_None, Key_Names, Key_Count);
	Int32 i;
	for (i = 0; i < HotkeysText.Count; i++) {
		HotkeyData h = HotkeysList[i];
		if (h.BaseKey == baseKey && h.Flags == flags) { original = h; break; }
	}
	Gui_ReplaceActive(EditHotkeyScreen_MakeInstance(original));
}

Screen* HotkeyListScreen_MakeInstance(void) {
	ListScreen* screen = ListScreen_MakeInstance();
	String title = String_FromConst("Modify hotkeys"); screen->TitleText = title;
	screen->EntryClick = HotkeyListScreen_EntryClick;

	UInt8 textBuffer[String_BufferSize(STRING_SIZE)];
	String text = String_InitAndClearArray(textBuffer);
	Int32 i;

	for (i = 0; i < HotkeysText.Count; i++) {
		HotkeyData hKey = HotkeysList[i];
		String_Clear(&text);

		String_AppendConst(&text, Key_Names[hKey.BaseKey]);
		String_AppendConst(&text, " |");
		EditHotkeyScreen_MakeFlags(hKey.Flags, &text);
		StringsBuffer_Add(&screen->Entries, &text);
	}

	String empty = String_FromConst(LIST_SCREEN_EMPTY);
	for (i = 0; i < LIST_SCREEN_ITEMS; i++) {
		StringsBuffer_Add(&screen->Entries, &empty);
	}
	return (Screen*)screen;
}


/*########################################################################################################################*
*----------------------------------------------------LoadLevelScreen------------------------------------------------------*
*#########################################################################################################################*/
void LoadLevelScreen_SelectEntry(STRING_PURE String* filename, void* obj) {
	String cw  = String_FromConst(".cw");  String lvl = String_FromConst(".lvl");
	String fcm = String_FromConst(".fcm"); String dat = String_FromConst(".dat");

	if (!(String_CaselessEnds(filename, &cw) || String_CaselessEnds(filename, &lvl) 
		|| String_CaselessEnds(filename, &fcm) || String_CaselessEnds(filename, &dat))) return;

	StringsBuffer* entries = (StringsBuffer*)obj;
	StringsBuffer_Add(entries, filename);
}

void LoadLevelScreen_EntryClick(GuiElement* screenElem, GuiElement* w) {
	ListScreen* screen = (ListScreen*)screenElem;
	UInt8 pathBuffer[String_BufferSize(FILENAME_SIZE)];
	String path = String_InitAndClearArray(pathBuffer);
	
	String filename = ListScreen_UNSAFE_GetCur(screen, w);
	String_Format2(&path, "maps%r%s", &Platform_DirectorySeparator, &filename);
	if (!Platform_FileExists(&path)) return;

	void* file;
	ReturnCode code = Platform_FileOpen(&file, &path);
	ErrorHandler_CheckOrFail(code, "Failed to open map file");
	Stream stream; Stream_FromFile(&stream, file, &path);

	World_Reset();
	Event_RaiseVoid(&WorldEvents_NewMap);

	if (World_TextureUrl.length > 0) {
		TexturePack_ExtractDefault();
		String_Clear(&World_TextureUrl);
	}
	Block_Reset();
	Inventory_SetDefaultMapping();

	String cw  = String_FromConst(".cw");  String lvl = String_FromConst(".lvl");
	String fcm = String_FromConst(".fcm"); String dat = String_FromConst(".dat");
	if (String_CaselessEnds(&path, &dat)) {
		Dat_Load(&stream);
	} else if (String_CaselessEnds(&path, &fcm)) {
		Fcm_Load(&stream);
	} else if (String_CaselessEnds(&path, &cw)) {
		Cw_Load(&stream);
	} else if (String_CaselessEnds(&path, &lvl)) {
		Lvl_Load(&stream);
	}
	World_SetNewMap(World_Blocks, World_BlocksSize, World_Width, World_Height, World_Length);

	Event_RaiseVoid(&WorldEvents_MapLoaded);
	if (Game_AllowServerTextures && World_TextureUrl.length > 0) {
		ServerConnection_RetrieveTexturePack(&World_TextureUrl);
	}

	LocalPlayer* p = &LocalPlayer_Instance;
	LocationUpdate update; LocationUpdate_MakePosAndOri(&update, p->Spawn, p->SpawnRotY, p->SpawnHeadX, false);
	p->Base.VTABLE->SetLocation(&p->Base, &update, false);
}

Screen* LoadLevelScreen_MakeInstance(void) {
	ListScreen* screen = ListScreen_MakeInstance();
	String title = String_FromConst("Select a level"); screen->TitleText = title;
	screen->EntryClick = LoadLevelScreen_EntryClick;

	String path = String_FromConst("maps");
	Platform_EnumFiles(&path, &screen->Entries, LoadLevelScreen_SelectEntry);
	if (screen->Entries.Count > 0) {
		ListScreen_QuickSort(0, screen->Entries.Count - 1);
	}
	return (Screen*)screen;
}


/*########################################################################################################################*
*---------------------------------------------------KeyBindingsScreen-----------------------------------------------------*
*#########################################################################################################################*/
KeyBindingsScreen KeyBindingsScreen_Instance;
GuiElementVTABLE KeyBindingsScreen_VTABLE;
void KeyBindingsScreen_ButtonText(KeyBindingsScreen* screen, Int32 i, STRING_TRANSIENT String* text) {
	Key key = KeyBind_Get(screen->Binds[i]);
	String_Format2(text, "%c: %c", screen->Descs[i], Key_Names[key]);
}

void KeyBindingsScreen_OnBindingClick(GuiElement* screenElem, GuiElement* widget) {
	KeyBindingsScreen* screen = (KeyBindingsScreen*)screenElem;
	UInt8 textBuffer[String_BufferSize(STRING_SIZE)];
	String text = String_InitAndClearArray(textBuffer);

	if (screen->CurI >= 0) {
		KeyBindingsScreen_ButtonText(screen, screen->CurI, &text);
		ButtonWidget* curButton = (ButtonWidget*)screen->WidgetsPtr[screen->CurI];
		ButtonWidget_SetText(curButton, &text);
	}
	screen->CurI = MenuScreen_Index((MenuScreen*)screen, (Widget*)widget);

	String_Clear(&text);
	String_AppendConst(&text, "> ");
	KeyBindingsScreen_ButtonText(screen, screen->CurI, &text);
	String_AppendConst(&text, " <");
	ButtonWidget_SetText((ButtonWidget*)widget, &text);
}

Int32 KeyBindingsScreen_MakeWidgets(KeyBindingsScreen* screen, Int32 y, Int32 arrowsY, Int32 leftLength, STRING_PURE const UInt8* title, Int32 btnWidth) {
	Int32 i, origin = y, xOffset = btnWidth / 2 + 5;
	screen->CurI = -1;

	Widget** widgets = screen->WidgetsPtr;
	UInt8 textBuffer[String_BufferSize(STRING_SIZE)];
	String text = String_InitAndClearArray(textBuffer);

	for (i = 0; i < screen->BindsCount; i++) {
		if (i == leftLength) y = origin; /* reset y for next column */
		Int32 xDir = leftLength == -1 ? 0 : (i < leftLength ? -1 : 1);

		String_Clear(&text);
		KeyBindingsScreen_ButtonText(screen, i, &text);

		ButtonWidget_Create(&screen->Buttons[i], btnWidth, &text, &screen->TitleFont, KeyBindingsScreen_OnBindingClick);
		widgets[i] = (Widget*)(&screen->Buttons[i]);
		Widget_SetLocation(widgets[i], ANCHOR_CENTRE, ANCHOR_CENTRE, xDir * xOffset, y);
		y += 50; /* distance between buttons */
	}

	String titleText = String_FromReadonly(title);
	TextWidget_Create(&screen->Title, &titleText, &screen->TitleFont);
	Widget_SetLocation((Widget*)(&screen->Title), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -180);
	widgets[i++] = (Widget*)(&screen->Title);

	Widget_LeftClick backClick = Game_UseClassicOptions ? Menu_SwitchClassicOptions : Menu_SwitchOptions;
	Menu_MakeDefaultBack(&screen->Back, false, &screen->TitleFont, backClick);
	widgets[i++] = (Widget*)(&screen->Back);
	if (screen->LeftPage == NULL && screen->RightPage == NULL) return i;

	String lArrow = String_FromConst("<");
	ButtonWidget_Create(&screen->Left, 40, &lArrow, &screen->TitleFont, screen->LeftPage);
	Widget_SetLocation((Widget*)(&screen->Left), ANCHOR_CENTRE, ANCHOR_CENTRE, -btnWidth - 35, arrowsY);
	screen->Left.Disabled = screen->LeftPage == NULL;
	widgets[i++] = (Widget*)(&screen->Left);

	String rArrow = String_FromConst(">");
	ButtonWidget_Create(&screen->Right, 40, &rArrow, &screen->TitleFont, screen->RightPage);
	Widget_SetLocation((Widget*)(&screen->Right), ANCHOR_CENTRE, ANCHOR_CENTRE, btnWidth + 35, arrowsY);
	screen->Right.Disabled = screen->RightPage == NULL;
	widgets[i++] = (Widget*)(&screen->Right);

	return i;
}

void KeyBindingsScreen_Init(GuiElement* elem) {
	KeyBindingsScreen* screen = (KeyBindingsScreen*)elem;
	MenuScreen_Init(elem);
	screen->ContextRecreated(elem);
}

bool KeyBindingsScreen_HandlesKeyDown(GuiElement* elem, Key key) {
	KeyBindingsScreen* screen = (KeyBindingsScreen*)elem;
	if (screen->CurI == -1) return MenuScreen_HandlesKeyDown(elem, key);

	KeyBind bind = screen->Binds[screen->CurI];
	if (key == Key_Escape) key = KeyBind_GetDefault(bind);
	KeyBind_Set(bind, key);

	UInt8 textBuffer[String_BufferSize(STRING_SIZE)];
	String text = String_InitAndClearArray(textBuffer);
	KeyBindingsScreen_ButtonText(screen, screen->CurI, &text);

	ButtonWidget* curButton = (ButtonWidget*)screen->WidgetsPtr[screen->CurI];
	ButtonWidget_SetText(curButton, &text);
	screen->CurI = -1;
	return true;
}

bool KeyBindingsScreen_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	if (btn != MouseButton_Right) {
		return MenuScreen_HandlesMouseDown(elem, x, y, btn);
	}

	KeyBindingsScreen* screen = (KeyBindingsScreen*)elem;
	Int32 i = Menu_HandleMouseDown(elem, screen->WidgetsPtr, screen->WidgetsCount, x, y, btn);
	if (i == -1) return false;

	/* Reset a key binding */
	if ((screen->CurI == -1 || screen->CurI == i) && i < screen->BindsCount) {
		screen->CurI = i;
		Elem_HandlesKeyDown(elem, KeyBind_GetDefault(screen->Binds[i]));
	}
	return true;
}

KeyBindingsScreen* KeyBindingsScreen_Make(Int32 bindsCount, UInt8* binds, const UInt8** descs, ButtonWidget* buttons, Widget** widgets, Menu_ContextFunc contextRecreated) {
	KeyBindingsScreen* screen = &KeyBindingsScreen_Instance;
	MenuScreen_MakeInstance((MenuScreen*)screen, widgets, bindsCount + 4, contextRecreated);
	KeyBindingsScreen_VTABLE = *screen->VTABLE;
	screen->VTABLE = &KeyBindingsScreen_VTABLE;

	screen->VTABLE->Init             = KeyBindingsScreen_Init;
	screen->VTABLE->HandlesKeyDown   = KeyBindingsScreen_HandlesKeyDown;
	screen->VTABLE->HandlesMouseDown = KeyBindingsScreen_HandlesMouseDown;

	screen->BindsCount       = bindsCount;
	screen->Binds            = binds;
	screen->Descs            = descs;
	screen->Buttons          = buttons;
	screen->ContextRecreated = contextRecreated;

	screen->CurI      = -1;
	screen->LeftPage  = NULL;
	screen->RightPage = NULL;
	return screen;
}


/*########################################################################################################################*
*-----------------------------------------------ClassicKeyBindingsScreen--------------------------------------------------*
*#########################################################################################################################*/
void ClassicKeyBindingsScreen_ContextRecreated(void* obj) {
	KeyBindingsScreen* screen = (KeyBindingsScreen*)obj;
	if (Game_ClassicHacks) {
		KeyBindingsScreen_MakeWidgets(screen, -140, -40, 5, "Normal controls", 260);
	} else {
		KeyBindingsScreen_MakeWidgets(screen, -140, -40, 5, "Controls", 300);
	}
}

Screen* ClassicKeyBindingsScreen_MakeInstance(void) {
	static UInt8 binds[10] = { KeyBind_Forward, KeyBind_Back, KeyBind_Jump, KeyBind_Chat, KeyBind_SetSpawn, KeyBind_Left, KeyBind_Right, KeyBind_Inventory, KeyBind_ToggleFog, KeyBind_Respawn };
	static const UInt8* descs[10] = { "Forward", "Back", "Jump", "Chat", "Save loc", "Left", "Right", "Build", "Toggle fog", "Load loc" };
	static ButtonWidget buttons[10];
	static Widget* widgets[10 + 4];

	KeyBindingsScreen* screen = KeyBindingsScreen_Make(Array_Elems(binds), binds, descs, buttons, widgets, ClassicKeyBindingsScreen_ContextRecreated);
	if (Game_ClassicHacks) screen->RightPage = Menu_SwitchKeysClassicHacks;
	return (Screen*)screen;
}


/*########################################################################################################################*
*--------------------------------------------ClassicHacksKeyBindingsScreen------------------------------------------------*
*#########################################################################################################################*/
void ClassicHacksKeyBindingsScreen_ContextRecreated(void* obj) {
	KeyBindingsScreen* screen = (KeyBindingsScreen*)obj;
	KeyBindingsScreen_MakeWidgets(screen, -90, -40, 3, "Hacks controls", 260);
}

Screen* ClassicHacksKeyBindingsScreen_MakeInstance(void) {
	static UInt8 binds[6] = { KeyBind_Speed, KeyBind_NoClip, KeyBind_HalfSpeed, KeyBind_Fly, KeyBind_FlyUp, KeyBind_FlyDown };
	static const UInt8* descs[6] = { "Speed", "Noclip", "Half speed", "Fly", "Fly up", "Fly down" };
	static ButtonWidget buttons[6];
	static Widget* widgets[6 + 4];

	KeyBindingsScreen* screen = KeyBindingsScreen_Make(Array_Elems(binds), binds, descs, buttons, widgets, ClassicHacksKeyBindingsScreen_ContextRecreated);
	screen->LeftPage = Menu_SwitchKeysClassic;
	return (Screen*)screen;
}


/*########################################################################################################################*
*-----------------------------------------------NormalKeyBindingsScreen---------------------------------------------------*
*#########################################################################################################################*/
void NormalKeyBindingsScreen_ContextRecreated(void* obj) {
	KeyBindingsScreen* screen = (KeyBindingsScreen*)obj;
	KeyBindingsScreen_MakeWidgets(screen, -140, 10, 6, "Normal controls", 260);
}

Screen* NormalKeyBindingsScreen_MakeInstance(void) {
	static UInt8 binds[12] = { KeyBind_Forward, KeyBind_Back, KeyBind_Jump, KeyBind_Chat, KeyBind_SetSpawn, KeyBind_PlayerList, KeyBind_Left, KeyBind_Right, KeyBind_Inventory, KeyBind_ToggleFog, KeyBind_Respawn, KeyBind_SendChat };
	static const UInt8* descs[12] = { "Forward", "Back", "Jump", "Chat", "Set spawn", "Player list", "Left", "Right", "Inventory", "Toggle fog", "Respawn", "Send chat" };
	static ButtonWidget buttons[12];
	static Widget* widgets[12 + 4];

	KeyBindingsScreen* screen = KeyBindingsScreen_Make(Array_Elems(binds), binds, descs, buttons, widgets, NormalKeyBindingsScreen_ContextRecreated);
	screen->RightPage = Menu_SwitchKeysHacks;
	return (Screen*)screen;
}


/*########################################################################################################################*
*------------------------------------------------HacksKeyBindingsScreen---------------------------------------------------*
*#########################################################################################################################*/
void HacksKeyBindingsScreen_ContextRecreated(void* obj) {
	KeyBindingsScreen* screen = (KeyBindingsScreen*)obj;
	KeyBindingsScreen_MakeWidgets(screen, -40, 10, 4, "Hacks controls", 260);
}

Screen* HacksKeyBindingsScreen_MakeInstance(void) {
	static UInt8 binds[8] = { KeyBind_Speed, KeyBind_NoClip, KeyBind_HalfSpeed, KeyBind_ZoomScrolling, KeyBind_Fly, KeyBind_FlyUp, KeyBind_FlyDown, KeyBind_ThirdPerson };
	static const UInt8* descs[8] = { "Speed", "Noclip", "Half speed", "Scroll zoom", "Fly", "Fly up", "Fly down", "Third person" };
	static ButtonWidget buttons[8];
	static Widget* widgets[8 + 4];

	KeyBindingsScreen* screen = KeyBindingsScreen_Make(Array_Elems(binds), binds, descs, buttons, widgets, HacksKeyBindingsScreen_ContextRecreated);
	screen->LeftPage  = Menu_SwitchKeysNormal;
	screen->RightPage = Menu_SwitchKeysOther;
	return (Screen*)screen;
}


/*########################################################################################################################*
*------------------------------------------------OtherKeyBindingsScreen---------------------------------------------------*
*#########################################################################################################################*/
void OtherKeyBindingsScreen_ContextRecreated(void* obj) {
	KeyBindingsScreen* screen = (KeyBindingsScreen*)obj;
	KeyBindingsScreen_MakeWidgets(screen, -140, 10, 6, "Other controls", 260);
}

Screen* OtherKeyBindingsScreen_MakeInstance(void) {
	static UInt8 binds[12] = { KeyBind_ExtInput, KeyBind_HideFps, KeyBind_HideGui, KeyBind_HotbarSwitching, KeyBind_DropBlock,KeyBind_Screenshot, KeyBind_Fullscreen, KeyBind_AxisLines, KeyBind_Autorotate, KeyBind_SmoothCamera, KeyBind_IDOverlay, KeyBind_BreakableLiquids };
	static const UInt8* descs[12] = { "Show ext input", "Hide FPS", "Hide gui", "Hotbar switching", "Drop block", "Screenshot", "Fullscreen", "Show axis lines", "Auto-rotate", "Smooth camera", "ID overlay", "Breakable liquids" };
	static ButtonWidget buttons[12];
	static Widget* widgets[12 + 4];

	KeyBindingsScreen* screen = KeyBindingsScreen_Make(Array_Elems(binds), binds, descs, buttons, widgets, OtherKeyBindingsScreen_ContextRecreated);
	screen->LeftPage  = Menu_SwitchKeysHacks;
	screen->RightPage = Menu_SwitchKeysMouse;
	return (Screen*)screen;
}


/*########################################################################################################################*
*------------------------------------------------MouseKeyBindingsScreen---------------------------------------------------*
*#########################################################################################################################*/
void MouseKeyBindingsScreen_ContextRecreated(void* obj) {
	KeyBindingsScreen* screen = (KeyBindingsScreen*)obj;
	Int32 i = KeyBindingsScreen_MakeWidgets(screen, -40, 10, -1, "Mouse key bindings", 260);

	static TextWidget text;
	String msg = String_FromConst("&eRight click to remove the key binding");
	TextWidget_Create(&text, &msg, &screen->TextFont);
	Widget_SetLocation((Widget*)(&text), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 100);
	screen->WidgetsPtr[i] = (Widget*)(&text);
}

Screen* MouseKeyBindingsScreen_MakeInstance(void) {
	static UInt8 binds[3] = { KeyBind_MouseLeft, KeyBind_MouseMiddle, KeyBind_MouseRight };
	static const UInt8* descs[3] = { "Left", "Middle", "Right" };
	static ButtonWidget buttons[3];
	static Widget* widgets[3 + 4 + 1];

	KeyBindingsScreen* screen = KeyBindingsScreen_Make(Array_Elems(binds), binds, descs, buttons, widgets, MouseKeyBindingsScreen_ContextRecreated);
	screen->LeftPage = Menu_SwitchKeysOther;
	screen->WidgetsCount++; /* Extra text widget for 'right click' message */
	return (Screen*)screen;
}


/*########################################################################################################################*
*--------------------------------------------------MenuOptionsScreen------------------------------------------------------*
*#########################################################################################################################*/
MenuOptionsScreen MenuOptionsScreen_Instance;
GuiElementVTABLE MenuOptionsScreen_VTABLE;
void Menu_GetBool(STRING_TRANSIENT String* raw, bool v) {
	String_AppendConst(raw, v ? "ON" : "OFF");
}

bool Menu_SetBool(STRING_PURE String* raw, const UInt8* key) {
	bool isOn = String_CaselessEqualsConst(raw, "ON");
	Options_SetBool(key, isOn); return isOn;
}

void MenuOptionsScreen_GetFPS(STRING_TRANSIENT String* raw) {
	String_AppendConst(raw, FpsLimit_Names[Game_FpsLimit]);
}
void MenuOptionsScreen_SetFPS(STRING_PURE String* raw) {
	UInt32 method = Utils_ParseEnum(raw, FpsLimit_VSync, FpsLimit_Names, Array_Elems(FpsLimit_Names));
	Game_SetFpsLimitMethod(method);

	String value = String_FromReadonly(FpsLimit_Names[method]);
	Options_Set(OPT_FPS_LIMIT, &value);
}

void MenuOptionsScreen_Set(MenuOptionsScreen* screen, Int32 i, STRING_PURE String* text) {
	screen->Buttons[i].SetValue(text);
	/* need to get btn again here (e.g. changing FPS invalidates all widgets) */

	UInt8 titleBuffer[String_BufferSize(STRING_SIZE)];
	String title = String_InitAndClearArray(titleBuffer);
	String_AppendConst(&title, screen->Buttons[i].OptName);
	String_AppendConst(&title, ": ");
	screen->Buttons[i].GetValue(&title);
	ButtonWidget_SetText(&screen->Buttons[i], &title);
}

void MenuOptionsScreen_FreeExtHelp(MenuOptionsScreen* screen) {
	if (screen->ExtHelp.LinesCount == 0) return;
	Elem_TryFree(&screen->ExtHelp);
	screen->ExtHelp.LinesCount = 0;
}

void MenuOptionsScreen_RepositionExtHelp(MenuOptionsScreen* screen) {
	screen->ExtHelp.XOffset = Game_Width / 2 - screen->ExtHelp.Width / 2;
	screen->ExtHelp.YOffset = Game_Height / 2 + 100;
	Widget_Reposition(&screen->ExtHelp);
}

void MenuOptionsScreen_SelectExtHelp(MenuOptionsScreen* screen, Int32 idx) {
	MenuOptionsScreen_FreeExtHelp(screen);
	if (screen->Descriptions == NULL || screen->ActiveI >= 0) return;
	const UInt8* desc = screen->Descriptions[idx];
	if (desc == NULL) return;

	String descRaw = String_FromReadonly(desc);
	String descLines[5];
	UInt32 descLinesCount = Array_Elems(descLines);
	String_UNSAFE_Split(&descRaw, '%', descLines, &descLinesCount);

	TextGroupWidget_Create(&screen->ExtHelp, descLinesCount, &screen->TextFont, &screen->TextFont, screen->ExtHelp_Textures, screen->ExtHelp_Buffer);
	Widget_SetLocation((Widget*)(&screen->ExtHelp), ANCHOR_MIN, ANCHOR_MIN, 0, 0);
	Elem_Init(&screen->ExtHelp);

	Int32 i;
	for (i = 0; i < descLinesCount; i++) {
		TextGroupWidget_SetText(&screen->ExtHelp, i, &descLines[i]);
	}
	MenuOptionsScreen_RepositionExtHelp(screen);
}

void MenuOptionsScreen_FreeInput(MenuOptionsScreen* screen) {
	if (screen->ActiveI == -1) return;

	Int32 i;
	for (i = screen->WidgetsCount - 3; i < screen->WidgetsCount; i++) {
		if (screen->WidgetsPtr[i] == NULL) continue;
		Elem_TryFree(screen->WidgetsPtr[i]);
		screen->WidgetsPtr[i] = NULL;
	}
}

void MenuOptionsScreen_EnterInput(MenuOptionsScreen* screen) {
	String text = screen->Input.Base.Text;
	MenuInputValidator* validator = &screen->Input.Validator;

	if (validator->IsValidValue(validator, &text)) {
		MenuOptionsScreen_Set(screen, screen->ActiveI, &text);
	}

	MenuOptionsScreen_SelectExtHelp(screen, screen->ActiveI);
	MenuOptionsScreen_FreeInput(screen);
	screen->ActiveI = -1;
}

void MenuOptionsScreen_Init(GuiElement* elem) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)elem;
	MenuScreen_Init(elem);
	Key_KeyRepeat = true;
	screen->ContextRecreated(elem);
}
	
void MenuOptionsScreen_Render(GuiElement* elem, Real64 delta) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)elem;
	MenuScreen_Render(elem, delta);
	if (screen->ExtHelp.LinesCount == 0) return;

	TextGroupWidget* extHelp = &screen->ExtHelp;
	Int32 x = extHelp->X - 5, y = extHelp->Y - 5;
	Int32 width = extHelp->Width, height = extHelp->Height;
	PackedCol tableCol = PACKEDCOL_CONST(20, 20, 20, 200);
	GfxCommon_Draw2DFlat(x, y, width + 10, height + 10, tableCol);

	Gfx_SetTexturing(true);
	Elem_Render(&screen->ExtHelp, delta);
	Gfx_SetTexturing(false);
}

void MenuOptionsScreen_Free(GuiElement* elem) {
	MenuScreen_Free(elem);
	Key_KeyRepeat = false;
}

void MenuOptionsScreen_OnResize(GuiElement* elem) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)elem;
	MenuScreen_OnResize(elem);
	if (screen->ExtHelp.LinesCount == 0) return;
	MenuOptionsScreen_RepositionExtHelp(screen);
}

void MenuOptionsScreen_ContextLost(void* obj) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)obj;
	MenuScreen_ContextLost(obj);
	screen->ActiveI = -1;
	MenuOptionsScreen_FreeExtHelp(screen);
}

bool MenuOptionsScreen_HandlesKeyPress(GuiElement* elem, UInt8 key) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)elem;
	if (screen->ActiveI == -1) return true;
	return Elem_HandlesKeyPress(&screen->Input.Base, key);
}

bool MenuOptionsScreen_HandlesKeyDown(GuiElement* elem, Key key) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)elem;
	if (screen->ActiveI >= 0) {
		if (Elem_HandlesKeyDown(&screen->Input.Base, key)) return true;
		if (key == Key_Enter || key == Key_KeypadEnter) {
			MenuOptionsScreen_EnterInput(screen); return true;
		}
	}
	return MenuScreen_HandlesKeyDown(elem, key);
}

bool MenuOptionsScreen_HandlesKeyUp(GuiElement* elem, Key key) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)elem;
	if (screen->ActiveI == -1) return true;
	return Elem_HandlesKeyUp(&screen->Input.Base, key);
}

bool MenuOptionsScreen_HandlesMouseMove(GuiElement* elem, Int32 x, Int32 y) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)elem;
	Int32 i = Menu_HandleMouseMove(screen->WidgetsPtr, screen->WidgetsCount, x, y);
	if (i == -1 || i == screen->SelectedI) return true;
	if (screen->Descriptions == NULL || i >= screen->DescriptionsCount) return true;

	screen->SelectedI = i;
	if (screen->ActiveI == -1) MenuOptionsScreen_SelectExtHelp(screen, i);
	return true;
}

void MenuOptionsScreen_Make(MenuOptionsScreen* screen, Int32 i, Int32 dir, Int32 y, const UInt8* optName, Widget_LeftClick onClick, ButtonWidget_Get getter, ButtonWidget_Set setter) {
	UInt8 titleBuffer[String_BufferSize(STRING_SIZE)];
	String title = String_InitAndClearArray(titleBuffer);
	String_AppendConst(&title, optName);
	String_AppendConst(&title, ": ");
	getter(&title);

	ButtonWidget* btn = &screen->Buttons[i];
	screen->WidgetsPtr[i] = (Widget*)btn;
	ButtonWidget_Create(btn, 300, &title, &screen->TitleFont, onClick);
	Widget_SetLocation((Widget*)btn, ANCHOR_CENTRE, ANCHOR_CENTRE, 160 * dir, y);

	btn->OptName  = optName;
	btn->GetValue = getter;
	btn->SetValue = setter;
}

void MenuOptionsScreen_OK(GuiElement* screenElem, GuiElement* widget) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)screenElem; 
	MenuOptionsScreen_EnterInput(screen);
}

void MenuOptionsScreen_Default(GuiElement* screenElem, GuiElement* widget) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)screenElem;
	String text = String_FromReadonly(screen->DefaultValues[screen->ActiveI]);
	InputWidget_Clear(&screen->Input.Base);
	InputWidget_AppendString(&screen->Input.Base, &text);
}

void MenuOptionsScreen_Bool(GuiElement* screenElem, GuiElement* widget) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)screenElem;
	ButtonWidget* button = (ButtonWidget*)widget;
	Int32 index = MenuScreen_Index((MenuScreen*)screen, (Widget*)widget);
	MenuOptionsScreen_SelectExtHelp(screen, index);

	UInt8 valueBuffer[String_BufferSize(STRING_SIZE)];
	String value = String_InitAndClearArray(valueBuffer);
	button->GetValue(&value);

	bool isOn = String_CaselessEqualsConst(&value, "ON");
	String newValue = String_FromReadonly(isOn ? "OFF" : "ON");
	MenuOptionsScreen_Set(screen, index, &newValue);
}

void MenuOptionsScreen_Enum(GuiElement* screenElem, GuiElement* widget) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)screenElem;
	ButtonWidget* button = (ButtonWidget*)widget;
	Int32 index = MenuScreen_Index((MenuScreen*)screen, (Widget*)widget);
	MenuOptionsScreen_SelectExtHelp(screen, index);

	MenuInputValidator* validator = &screen->Validators[index];
	const UInt8** names = (const UInt8**)validator->Meta_Ptr[0];
	UInt32 count = (UInt32)validator->Meta_Ptr[1];

	UInt8 valueBuffer[String_BufferSize(STRING_SIZE)];
	String value = String_InitAndClearArray(valueBuffer);
	button->GetValue(&value);

	UInt32 raw = (Utils_ParseEnum(&value, 0, names, count) + 1) % count;
	String newValue = String_FromReadonly(names[raw]);
	MenuOptionsScreen_Set(screen, index, &newValue);
}

void MenuOptionsScreen_Input(GuiElement* screenElem, GuiElement* widget) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)screenElem;
	ButtonWidget* button = (ButtonWidget*)widget;
	screen->ActiveI = MenuScreen_Index((MenuScreen*)screen, (Widget*)widget);
	MenuOptionsScreen_FreeExtHelp(screen);

	MenuOptionsScreen_FreeInput(screen);
	UInt8 valueBuffer[String_BufferSize(STRING_SIZE)];
	String value = String_InitAndClearArray(valueBuffer);
	button->GetValue(&value);

	MenuInputValidator* validator = &screen->Validators[screen->ActiveI];
	MenuInputWidget_Create(&screen->Input, 400, 30, &value, &screen->TextFont, validator);
	Widget_SetLocation((Widget*)(&screen->Input), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 110);
	screen->Input.Base.ShowCaret = true;

	String okMsg = String_FromConst("OK");
	ButtonWidget_Create(&screen->OK, 40, &okMsg, &screen->TitleFont, MenuOptionsScreen_OK);
	Widget_SetLocation((Widget*)(&screen->OK), ANCHOR_CENTRE, ANCHOR_CENTRE, 240, 110);

	String defMsg = String_FromConst("Default value");
	ButtonWidget_Create(&screen->Default, 200, &defMsg, &screen->TitleFont, MenuOptionsScreen_Default);
	Widget_SetLocation((Widget*)(&screen->Default), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 150);

	Widget** widgets = screen->WidgetsPtr;
	widgets[screen->WidgetsCount - 1] = (Widget*)(&screen->Input);
	widgets[screen->WidgetsCount - 2] = (Widget*)(&screen->OK);
	widgets[screen->WidgetsCount - 3] = (Widget*)(&screen->Default);
}

Screen* MenuOptionsScreen_MakeInstance(Widget** widgets, Int32 count, ButtonWidget* buttons, Menu_ContextFunc contextRecreated,
	MenuInputValidator* validators, const UInt8** defaultValues, const UInt8** descriptions, Int32 descsCount) {
	MenuOptionsScreen* screen = &MenuOptionsScreen_Instance;
	Platform_MemSet(screen, 0, sizeof(MenuOptionsScreen));
	MenuScreen_MakeInstance((MenuScreen*)screen, widgets, count, contextRecreated);
	MenuOptionsScreen_VTABLE = *screen->VTABLE;
	screen->VTABLE = &MenuOptionsScreen_VTABLE;

	screen->VTABLE->HandlesKeyDown   = MenuOptionsScreen_HandlesKeyDown;
	screen->VTABLE->HandlesKeyUp     = MenuOptionsScreen_HandlesKeyUp;
	screen->VTABLE->HandlesKeyPress  = MenuOptionsScreen_HandlesKeyPress;
	screen->VTABLE->HandlesMouseMove = MenuOptionsScreen_HandlesMouseMove;

	screen->OnResize       = MenuOptionsScreen_OnResize;
	screen->VTABLE->Init   = MenuOptionsScreen_Init;
	screen->VTABLE->Render = MenuOptionsScreen_Render;
	screen->VTABLE->Free   = MenuOptionsScreen_Free;

	screen->ContextLost       = MenuOptionsScreen_ContextLost;
	screen->Buttons           = buttons;
	screen->Validators        = validators;
	screen->DefaultValues     = defaultValues;
	screen->Descriptions      = descriptions;
	screen->DescriptionsCount = descsCount;

	screen->ActiveI = -1;
	return (Screen*)screen;
}


/*########################################################################################################################*
*---------------------------------------------------ClassicOptionsScreen--------------------------------------------------*
*#########################################################################################################################*/
typedef enum ViewDist_ {
	ViewDist_Tiny, ViewDist_Short, ViewDist_Normal, ViewDist_Far, ViewDist_Count,
} ViewDist;
const UInt8* ViewDist_Names[ViewDist_Count] = { "TINY", "SHORT", "NORMAL", "FAR" };

void ClassicOptionsScreen_GetMusic(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_MusicVolume > 0); }
void ClassicOptionsScreen_SetMusic(STRING_PURE String* v) {
	Game_MusicVolume = String_CaselessEqualsConst(v, "ON") ? 100 : 0;
	Audio_SetMusic(Game_MusicVolume);
	Options_SetInt32(OPT_MUSIC_VOLUME, Game_MusicVolume);
}

void ClassicOptionsScreen_GetInvert(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_InvertMouse); }
void ClassicOptionsScreen_SetInvert(STRING_PURE String* v) { Game_InvertMouse = Menu_SetBool(v, OPT_INVERT_MOUSE); }

void ClassicOptionsScreen_GetViewDist(STRING_TRANSIENT String* v) {
	if (Game_ViewDistance >= 512) {
		String_AppendConst(v, ViewDist_Names[ViewDist_Far]);
	} else if (Game_ViewDistance >= 128) {
		String_AppendConst(v, ViewDist_Names[ViewDist_Normal]);
	} else if (Game_ViewDistance >= 32) {
		String_AppendConst(v, ViewDist_Names[ViewDist_Short]);
	} else {
		String_AppendConst(v, ViewDist_Names[ViewDist_Tiny]);
	}
}
void ClassicOptionsScreen_SetViewDist(STRING_PURE String* v) {
	UInt32 raw = Utils_ParseEnum(v, 0, ViewDist_Names, ViewDist_Count);
	Int32 dist = raw == ViewDist_Far ? 512 : (raw == ViewDist_Normal ? 128 : (raw == ViewDist_Short ? 32 : 8));
	Game_SetViewDistance(dist, true);
}

void ClassicOptionsScreen_GetPhysics(STRING_TRANSIENT String* v) { Menu_GetBool(v, Physics_Enabled); }
void ClassicOptionsScreen_SetPhysics(STRING_PURE String* v) {
	Physics_SetEnabled(Menu_SetBool(v, OPT_BLOCK_PHYSICS));
}

void ClassicOptionsScreen_GetSounds(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_SoundsVolume > 0); }
void ClassicOptionsScreen_SetSounds(STRING_PURE String* v) {
	Game_SoundsVolume = String_CaselessEqualsConst(v, "ON") ? 100 : 0;
	Audio_SetSounds(Game_SoundsVolume);
	Options_SetInt32(OPT_SOUND_VOLUME, Game_SoundsVolume);
}

void ClassicOptionsScreen_GetShowFPS(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_ShowFPS); }
void ClassicOptionsScreen_SetShowFPS(STRING_PURE String* v) { Game_ShowFPS = Menu_SetBool(v, OPT_SHOW_FPS); }

void ClassicOptionsScreen_GetViewBob(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_ViewBobbing); }
void ClassicOptionsScreen_SetViewBob(STRING_PURE String* v) { Game_ViewBobbing = Menu_SetBool(v, OPT_VIEW_BOBBING); }

void ClassicOptionsScreen_GetHacks(STRING_TRANSIENT String* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.Enabled); }
void ClassicOptionsScreen_SetHacks(STRING_PURE String* v) {
	LocalPlayer_Instance.Hacks.Enabled = Menu_SetBool(v, OPT_HACKS_ENABLED);
	LocalPlayer_CheckHacksConsistency();
}

void ClassicOptionsScreen_ContextRecreated(void* obj) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)obj;
	Widget** widgets = screen->WidgetsPtr;

	MenuOptionsScreen_Make(screen, 0, -1, -150, "Music",           MenuOptionsScreen_Bool, 
		ClassicOptionsScreen_GetMusic,    ClassicOptionsScreen_SetMusic);
	MenuOptionsScreen_Make(screen, 1, -1, -100, "Invert mouse",    MenuOptionsScreen_Bool, 
		ClassicOptionsScreen_GetInvert,   ClassicOptionsScreen_SetInvert);
	MenuOptionsScreen_Make(screen, 2, -1,  -50, "Render distance", MenuOptionsScreen_Enum, 
		ClassicOptionsScreen_GetViewDist, ClassicOptionsScreen_SetViewDist);
	MenuOptionsScreen_Make(screen, 3, -1,    0, "Block physics",   MenuOptionsScreen_Bool, 
		ClassicOptionsScreen_GetPhysics,  ClassicOptionsScreen_SetPhysics);

	MenuOptionsScreen_Make(screen, 4, 1, -150, "Sound",         MenuOptionsScreen_Bool, 
		ClassicOptionsScreen_GetSounds,  ClassicOptionsScreen_SetSounds);
	MenuOptionsScreen_Make(screen, 5, 1, -100, "Show FPS",      MenuOptionsScreen_Bool, 
		ClassicOptionsScreen_GetShowFPS, ClassicOptionsScreen_SetShowFPS);
	MenuOptionsScreen_Make(screen, 6, 1,  -50, "View bobbing",  MenuOptionsScreen_Bool, 
		ClassicOptionsScreen_GetViewBob, ClassicOptionsScreen_SetViewBob);
	MenuOptionsScreen_Make(screen, 7, 1,    0, "FPS mode",      MenuOptionsScreen_Enum, 
		MenuOptionsScreen_GetFPS,        MenuOptionsScreen_SetFPS);
	MenuOptionsScreen_Make(screen, 8, 0,   60, "Hacks enabled", MenuOptionsScreen_Bool, 
		ClassicOptionsScreen_GetHacks,   ClassicOptionsScreen_SetHacks);

	String controls = String_FromConst("Controls...");
	ButtonWidget_Create(&screen->Buttons[9], 400, &controls, &screen->TitleFont, Menu_SwitchKeysClassic);
	Widget_SetLocation((Widget*)(&screen->Buttons[9]), ANCHOR_CENTRE, ANCHOR_MAX, 0, 95);
	widgets[9] = (Widget*)(&screen->Buttons[9]);

	String done = String_FromConst("Done");
	Menu_MakeBack(&screen->Buttons[10], 400, &done, 25, &screen->TitleFont, Menu_SwitchPause);
	widgets[10] = (Widget*)(&screen->Buttons[10]);

	/* Disable certain options */
	if (!ServerConnection_IsSinglePlayer) MenuScreen_Remove((MenuScreen*)screen, 3);
	if (!Game_ClassicHacks)               MenuScreen_Remove((MenuScreen*)screen, 8);
}

Screen* ClassicOptionsScreen_MakeInstance(void) {
	static ButtonWidget buttons[11];
	static MenuInputValidator validators[Array_Elems(buttons)];
	static Widget* widgets[Array_Elems(buttons)];	

	validators[2] = MenuInputValidator_Enum(ViewDist_Names, ViewDist_Count);
	validators[7] = MenuInputValidator_Enum(FpsLimit_Names, FpsLimit_Count);

	return MenuOptionsScreen_MakeInstance(widgets, Array_Elems(widgets), buttons,
		ClassicOptionsScreen_ContextRecreated, validators, NULL, NULL, 0);
}


/*########################################################################################################################*
*----------------------------------------------------EnvSettingsScreen----------------------------------------------------*
*#########################################################################################################################*/
void EnvSettingsScreen_GetCloudsCol(STRING_TRANSIENT String* v) { PackedCol_ToHex(v, WorldEnv_CloudsCol); }
void EnvSettingsScreen_SetCloudsCol(STRING_PURE String* v) { WorldEnv_SetCloudsCol(Menu_HexCol(v)); }

void EnvSettingsScreen_GetSkyCol(STRING_TRANSIENT String* v) { PackedCol_ToHex(v, WorldEnv_SkyCol); }
void EnvSettingsScreen_SetSkyCol(STRING_PURE String* v) { WorldEnv_SetSkyCol(Menu_HexCol(v)); }

void EnvSettingsScreen_GetFogCol(STRING_TRANSIENT String* v) { PackedCol_ToHex(v, WorldEnv_FogCol); }
void EnvSettingsScreen_SetFogCol(STRING_PURE String* v) { WorldEnv_SetFogCol(Menu_HexCol(v)); }

void EnvSettingsScreen_GetCloudsSpeed(STRING_TRANSIENT String* v) { String_AppendReal32(v, WorldEnv_CloudsSpeed, 2); }
void EnvSettingsScreen_SetCloudsSpeed(STRING_PURE String* v) { WorldEnv_SetCloudsSpeed(Menu_Real32(v)); }

void EnvSettingsScreen_GetCloudsHeight(STRING_TRANSIENT String* v) { String_AppendInt32(v, WorldEnv_CloudsHeight); }
void EnvSettingsScreen_SetCloudsHeight(STRING_PURE String* v) { WorldEnv_SetCloudsHeight(Menu_Int32(v)); }

void EnvSettingsScreen_GetSunCol(STRING_TRANSIENT String* v) { PackedCol_ToHex(v, WorldEnv_SunCol); }
void EnvSettingsScreen_SetSunCol(STRING_PURE String* v) { WorldEnv_SetSunCol(Menu_HexCol(v)); }

void EnvSettingsScreen_GetShadowCol(STRING_TRANSIENT String* v) { PackedCol_ToHex(v, WorldEnv_ShadowCol); }
void EnvSettingsScreen_SetShadowCol(STRING_PURE String* v) { WorldEnv_SetShadowCol(Menu_HexCol(v)); }

void EnvSettingsScreen_GetWeather(STRING_TRANSIENT String* v) { String_AppendConst(v, Weather_Names[WorldEnv_Weather]); }
void EnvSettingsScreen_SetWeather(STRING_PURE String* v) {
	UInt32 raw = Utils_ParseEnum(v, 0, Weather_Names, Array_Elems(Weather_Names));
	WorldEnv_SetWeather(raw); 
}

void EnvSettingsScreen_GetWeatherSpeed(STRING_TRANSIENT String* v) { String_AppendReal32(v, WorldEnv_WeatherSpeed, 2); }
void EnvSettingsScreen_SetWeatherSpeed(STRING_PURE String* v) { WorldEnv_SetWeatherSpeed(Menu_Real32(v)); }

void EnvSettingsScreen_GetEdgeHeight(STRING_TRANSIENT String* v) { String_AppendInt32(v, WorldEnv_EdgeHeight); }
void EnvSettingsScreen_SetEdgeHeight(STRING_PURE String* v) { WorldEnv_SetEdgeHeight(Menu_Int32(v)); }

void EnvSettingsScreen_ContextRecreated(void* obj) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)obj;
	Widget** widgets = screen->WidgetsPtr;

	MenuOptionsScreen_Make(screen, 0, -1, -150, "Clouds col",    MenuOptionsScreen_Input, 
		EnvSettingsScreen_GetCloudsCol,    EnvSettingsScreen_SetCloudsCol);
	MenuOptionsScreen_Make(screen, 1, -1, -100, "Sky col",       MenuOptionsScreen_Input,
		EnvSettingsScreen_GetSkyCol,       EnvSettingsScreen_SetSkyCol);
	MenuOptionsScreen_Make(screen, 2, -1,  -50, "Fog col",       MenuOptionsScreen_Input,
		EnvSettingsScreen_GetFogCol,       EnvSettingsScreen_SetFogCol);
	MenuOptionsScreen_Make(screen, 3, -1,    0, "Clouds speed",  MenuOptionsScreen_Input,
		EnvSettingsScreen_GetCloudsSpeed,  EnvSettingsScreen_SetCloudsSpeed);
	MenuOptionsScreen_Make(screen, 4, -1,   50, "Clouds height", MenuOptionsScreen_Input,
		EnvSettingsScreen_GetCloudsHeight, EnvSettingsScreen_SetCloudsHeight);

	MenuOptionsScreen_Make(screen, 5, 1, -150, "Sunlight col",    MenuOptionsScreen_Input,
		EnvSettingsScreen_GetSunCol,       EnvSettingsScreen_SetSunCol);
	MenuOptionsScreen_Make(screen, 6, 1, -100, "Shadow col",      MenuOptionsScreen_Input,
		EnvSettingsScreen_GetShadowCol,    EnvSettingsScreen_SetShadowCol);
	MenuOptionsScreen_Make(screen, 7, 1,  -50, "Weather",         MenuOptionsScreen_Enum,
		EnvSettingsScreen_GetWeather,      EnvSettingsScreen_SetWeather);
	MenuOptionsScreen_Make(screen, 8, 1,    0, "Rain/Snow speed", MenuOptionsScreen_Input,
		EnvSettingsScreen_GetWeatherSpeed, EnvSettingsScreen_SetWeatherSpeed);
	MenuOptionsScreen_Make(screen, 9, 1, 50, "Water level",       MenuOptionsScreen_Input,
		EnvSettingsScreen_GetEdgeHeight,   EnvSettingsScreen_SetEdgeHeight);

	Menu_MakeDefaultBack(&screen->Buttons[10], false, &screen->TitleFont, Menu_SwitchOptions);
	widgets[10] = (Widget*)(&screen->Buttons[10]);
	widgets[11] = NULL; widgets[12] = NULL; widgets[13] = NULL;
}

Screen* EnvSettingsScreen_MakeInstance(void) {
	static ButtonWidget buttons[11];
	static MenuInputValidator validators[Array_Elems(buttons)];
	static const UInt8* defaultValues[Array_Elems(buttons)];
	static Widget* widgets[Array_Elems(buttons) + 3];

	static UInt8 cloudHeightBuffer[String_BufferSize(STRING_INT_CHARS)];
	String cloudHeight = String_InitAndClearArray(cloudHeightBuffer);
	String_AppendInt32(&cloudHeight, World_Height + 2);

	static UInt8 edgeHeightBuffer[String_BufferSize(STRING_INT_CHARS)];
	String edgeHeight = String_InitAndClearArray(edgeHeightBuffer);
	String_AppendInt32(&edgeHeight, World_Height / 2);

	validators[0]    = MenuInputValidator_Hex();
	defaultValues[0] = WORLDENV_DEFAULT_CLOUDSCOL_HEX;
	validators[1]    = MenuInputValidator_Hex();
	defaultValues[1] = WORLDENV_DEFAULT_SKYCOL_HEX;
	validators[2]    = MenuInputValidator_Hex();
	defaultValues[2] = WORLDENV_DEFAULT_FOGCOL_HEX;
	validators[3]    = MenuInputValidator_Real(0.00f, 1000.00f);
	defaultValues[3] = "1";
	validators[4]    = MenuInputValidator_Integer(-10000, 10000);
	defaultValues[4] = cloudHeightBuffer;

	validators[5]    = MenuInputValidator_Hex();
	defaultValues[5] = WORLDENV_DEFAULT_SUNCOL_HEX;
	validators[6]    = MenuInputValidator_Hex();
	defaultValues[6] = WORLDENV_DEFAULT_SHADOWCOL_HEX;
	validators[7]    = MenuInputValidator_Enum(Weather_Names, Array_Elems(Weather_Names));
	validators[8]    = MenuInputValidator_Real(-100.00f, 100.00f);
	defaultValues[8] = "1";
	validators[9]    = MenuInputValidator_Integer(-2048, 2048);
	defaultValues[9] = edgeHeightBuffer;

	return MenuOptionsScreen_MakeInstance(widgets, Array_Elems(widgets), buttons,
		EnvSettingsScreen_ContextRecreated, validators, defaultValues, NULL, 0);
}


/*########################################################################################################################*
*--------------------------------------------------GraphicsOptionsScreen--------------------------------------------------*
*#########################################################################################################################*/
void GraphicsOptionsScreen_GetViewDist(STRING_TRANSIENT String* v) { String_AppendInt32(v, Game_ViewDistance); }
void GraphicsOptionsScreen_SetViewDist(STRING_PURE String* v) { Game_SetViewDistance(Menu_Int32(v), true); }

void GraphicsOptionsScreen_GetSmooth(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_SmoothLighting); }
void GraphicsOptionsScreen_SetSmooth(STRING_PURE String* v) {
	Game_SmoothLighting = Menu_SetBool(v, OPT_SMOOTH_LIGHTING);
	ChunkUpdater_ApplyMeshBuilder();
	ChunkUpdater_Refresh();
}

void GraphicsOptionsScreen_GetNames(STRING_TRANSIENT String* v) { String_AppendConst(v, NameMode_Names[Entities_NameMode]); }
void GraphicsOptionsScreen_SetNames(STRING_PURE String* v) {
	Entities_NameMode = Utils_ParseEnum(v, 0, NameMode_Names, NAME_MODE_COUNT);
	Options_Set(OPT_NAMES_MODE, v);
}

void GraphicsOptionsScreen_GetShadows(STRING_TRANSIENT String* v) { String_AppendConst(v, ShadowMode_Names[Entities_ShadowMode]); }
void GraphicsOptionsScreen_SetShadows(STRING_PURE String* v) {
	Entities_ShadowMode = Utils_ParseEnum(v, 0, ShadowMode_Names, SHADOW_MODE_COUNT);
	Options_Set(OPT_ENTITY_SHADOW, v);
}

void GraphicsOptionsScreen_GetMipmaps(STRING_TRANSIENT String* v) { Menu_GetBool(v, Gfx_Mipmaps); }
void GraphicsOptionsScreen_SetMipmaps(STRING_PURE String* v) {
	Gfx_Mipmaps = Menu_SetBool(v, OPT_MIPMAPS);
	UInt8 urlBuffer[String_BufferSize(STRING_SIZE)];
	String url = String_InitAndClearArray(urlBuffer);
	String_Set(&url, &World_TextureUrl);

	/* always force a reload from cache */
	String_Clear(&World_TextureUrl);
	String_AppendConst(&World_TextureUrl, "~`#$_^*()@");
	TexturePack_ExtractCurrent(&url);

	String_Set(&World_TextureUrl, &url);
}

void GraphicsOptionsScreen_ContextRecreated(void* obj) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)obj;
	Widget** widgets = screen->WidgetsPtr;

	MenuOptionsScreen_Make(screen, 0, -1, -50, "FPS mode",          MenuOptionsScreen_Enum, 
		MenuOptionsScreen_GetFPS,          MenuOptionsScreen_SetFPS);
	MenuOptionsScreen_Make(screen, 1, -1,   0, "View distance",     MenuOptionsScreen_Input, 
		GraphicsOptionsScreen_GetViewDist, GraphicsOptionsScreen_SetViewDist);
	MenuOptionsScreen_Make(screen, 2, -1,  50, "Advanced lighting", MenuOptionsScreen_Bool,
		GraphicsOptionsScreen_GetSmooth,   GraphicsOptionsScreen_SetSmooth);

	MenuOptionsScreen_Make(screen, 3, 1, -50, "Names",   MenuOptionsScreen_Enum, 
		GraphicsOptionsScreen_GetNames,    GraphicsOptionsScreen_SetNames);
	MenuOptionsScreen_Make(screen, 4, 1,   0, "Shadows", MenuOptionsScreen_Enum, 
		GraphicsOptionsScreen_GetShadows, GraphicsOptionsScreen_SetShadows);
	MenuOptionsScreen_Make(screen, 5, 1,  50, "Mipmaps", MenuOptionsScreen_Bool,
		GraphicsOptionsScreen_GetMipmaps, GraphicsOptionsScreen_SetMipmaps);

	Menu_MakeDefaultBack(&screen->Buttons[6], false, &screen->TitleFont, Menu_SwitchOptions);
	widgets[6] = (Widget*)(&screen->Buttons[6]);
	widgets[7] = NULL; widgets[8] = NULL; widgets[9] = NULL;
}

Screen* GraphicsOptionsScreen_MakeInstance(void) {
	static ButtonWidget buttons[7];
	static MenuInputValidator validators[Array_Elems(buttons)];
	static const UInt8* defaultValues[Array_Elems(buttons)];
	static Widget* widgets[Array_Elems(buttons) + 3];

	validators[0]    = MenuInputValidator_Enum(FpsLimit_Names, FpsLimit_Count);
	validators[1]    = MenuInputValidator_Integer(8, 4096);
	defaultValues[1] = "512";
	validators[3]    = MenuInputValidator_Enum(NameMode_Names,   NAME_MODE_COUNT);
	validators[4]    = MenuInputValidator_Enum(ShadowMode_Names, SHADOW_MODE_COUNT);
	
	static const UInt8* descs[Array_Elems(buttons)];
	descs[0] = \
		"&eVSync: &fNumber of frames rendered is at most the monitor's refresh rate.%" \
		"&e30/60/120 FPS: &f30/60/120 frames rendered at most each second.%" \
		"&eNoLimit: &fRenders as many frames as possible each second.%" \
		"&cUsing NoLimit mode is discouraged.";
	descs[2] = "&cNote: &eSmooth lighting is still experimental and can heavily reduce performance.";
	descs[3] = \
		"&eNone: &fNo names of players are drawn.%" \
		"&eHovered: &fName of the targeted player is drawn see-through.%" \
		"&eAll: &fNames of all other players are drawn normally.%" \
		"&eAllHovered: &fAll names of players are drawn see-through.%" \
		"&eAllUnscaled: &fAll names of players are drawn see-through without scaling.";
	descs[4] = \
		"&eNone: &fNo entity shadows are drawn.%" \
		"&eSnapToBlock: &fA square shadow is shown on block you are directly above.%" \
		"&eCircle: &fA circular shadow is shown across the blocks you are above.%" \
		"&eCircleAll: &fA circular shadow is shown underneath all entities.";

	return MenuOptionsScreen_MakeInstance(widgets, Array_Elems(widgets), buttons,
		GraphicsOptionsScreen_ContextRecreated, validators, defaultValues, descs, Array_Elems(descs));
}


/*########################################################################################################################*
*----------------------------------------------------GuiOptionsScreen-----------------------------------------------------*
*#########################################################################################################################*/
void GuiOptionsScreen_HandleFontChange(void) {
	MenuOptionsScreen* screen = &MenuOptionsScreen_Instance;
	Event_RaiseVoid(&ChatEvents_FontChanged);
	Elem_Recreate(screen);
	Gui_RefreshHud();
	screen->SelectedI = -1;
	Elem_HandlesMouseMove(screen, Mouse_X, Mouse_Y);
}

void GuiOptionsScreen_GetShadows(STRING_TRANSIENT String* v) { Menu_GetBool(v, Drawer2D_BlackTextShadows); }
void GuiOptionsScreen_SetShadows(STRING_PURE String* v) {
	Drawer2D_BlackTextShadows = Menu_SetBool(v, OPT_BLACK_TEXT);
	GuiOptionsScreen_HandleFontChange();
}

void GuiOptionsScreen_GetShowFPS(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_ShowFPS); }
void GuiOptionsScreen_SetShowFPS(STRING_PURE String* v) { Game_ShowFPS = Menu_SetBool(v, OPT_SHOW_FPS); }

void GuiOptionsScreen_SetScale(STRING_PURE String* v, Real32* target, const UInt8* optKey) {
	*target = Menu_Real32(v);
	Options_Set(optKey, v);
	Gui_RefreshHud();
}

void GuiOptionsScreen_GetHotbar(STRING_TRANSIENT String* v) { String_AppendReal32(v, Game_RawHotbarScale, 1); }
void GuiOptionsScreen_SetHotbar(STRING_PURE String* v) { GuiOptionsScreen_SetScale(v, &Game_RawHotbarScale, OPT_HOTBAR_SCALE); }

void GuiOptionsScreen_GetInventory(STRING_TRANSIENT String* v) { String_AppendReal32(v, Game_RawInventoryScale, 1); }
void GuiOptionsScreen_SetInventory(STRING_PURE String* v) { GuiOptionsScreen_SetScale(v, &Game_RawInventoryScale, OPT_INVENTORY_SCALE); }

void GuiOptionsScreen_GetTabAuto(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_TabAutocomplete); }
void GuiOptionsScreen_SetTabAuto(STRING_PURE String* v) { Game_TabAutocomplete = Menu_SetBool(v, OPT_TAB_AUTOCOMPLETE); }

void GuiOptionsScreen_GetClickable(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_ClickableChat); }
void GuiOptionsScreen_SetClickable(STRING_PURE String* v) { Game_ClickableChat = Menu_SetBool(v, OPT_CLICKABLE_CHAT); }

void GuiOptionsScreen_GetChatScale(STRING_TRANSIENT String* v) { String_AppendReal32(v, Game_RawChatScale, 1); }
void GuiOptionsScreen_SetChatScale(STRING_PURE String* v) { GuiOptionsScreen_SetScale(v, &Game_RawChatScale, OPT_CHAT_SCALE); }

void GuiOptionsScreen_GetChatlines(STRING_TRANSIENT String* v) { String_AppendInt32(v, Game_ChatLines); }
void GuiOptionsScreen_SetChatlines(STRING_PURE String* v) {
	Game_ChatLines = Menu_Int32(v);
	Options_Set(OPT_CHATLINES, v);
	Gui_RefreshHud();
}

void GuiOptionsScreen_GetUseFont(STRING_TRANSIENT String* v) { Menu_GetBool(v, !Drawer2D_UseBitmappedChat); }
void GuiOptionsScreen_SetUseFont(STRING_PURE String* v) {
	Drawer2D_UseBitmappedChat = !Menu_SetBool(v, OPT_USE_CHAT_FONT);
	GuiOptionsScreen_HandleFontChange();
}

void GuiOptionsScreen_GetFont(STRING_TRANSIENT String* v) { String_AppendString(v, &Game_FontName); }
void GuiOptionsScreen_SetFont(STRING_PURE String* v) {
	String_Set(&Game_FontName, v);
	Options_Set(OPT_FONT_NAME, v);
	GuiOptionsScreen_HandleFontChange();
}

void GuiOptionsScreen_ContextRecreated(void* obj) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)obj;
	Widget** widgets = screen->WidgetsPtr;

	MenuOptionsScreen_Make(screen, 0, -1, -150, "Black text shadows", MenuOptionsScreen_Bool,
		GuiOptionsScreen_GetShadows,   GuiOptionsScreen_SetShadows);
	MenuOptionsScreen_Make(screen, 1, -1, -100, "Show FPS",           MenuOptionsScreen_Bool,
		GuiOptionsScreen_GetShowFPS,   GuiOptionsScreen_SetShowFPS);
	MenuOptionsScreen_Make(screen, 2, -1,  -50, "Hotbar scale",       MenuOptionsScreen_Input,
		GuiOptionsScreen_GetHotbar,    GuiOptionsScreen_SetHotbar);
	MenuOptionsScreen_Make(screen, 3, -1,    0, "Inventory scale",    MenuOptionsScreen_Input,
		GuiOptionsScreen_GetInventory, GuiOptionsScreen_SetInventory);
	MenuOptionsScreen_Make(screen, 4, -1,   50, "Tab auto-complete",  MenuOptionsScreen_Bool,
		GuiOptionsScreen_GetTabAuto,   GuiOptionsScreen_SetTabAuto);

	MenuOptionsScreen_Make(screen, 5, 1, -150, "Clickable chat",  MenuOptionsScreen_Bool,
		GuiOptionsScreen_GetClickable, GuiOptionsScreen_SetClickable);
	MenuOptionsScreen_Make(screen, 6, 1, -100, "Chat scale",      MenuOptionsScreen_Input,
		GuiOptionsScreen_GetChatScale, GuiOptionsScreen_SetChatScale);
	MenuOptionsScreen_Make(screen, 7, 1,  -50, "Chat lines",      MenuOptionsScreen_Input,
		GuiOptionsScreen_GetChatlines, GuiOptionsScreen_SetChatlines);
	MenuOptionsScreen_Make(screen, 8, 1,    0, "Use system font", MenuOptionsScreen_Bool,
		GuiOptionsScreen_GetUseFont,   GuiOptionsScreen_SetUseFont);
	MenuOptionsScreen_Make(screen, 9, 1,   50, "Font",            MenuOptionsScreen_Input,
		GuiOptionsScreen_GetFont,      GuiOptionsScreen_SetFont);

	Menu_MakeDefaultBack(&screen->Buttons[10], false, &screen->TitleFont, Menu_SwitchOptions);
	widgets[10] = (Widget*)(&screen->Buttons[10]);
	widgets[11] = NULL; widgets[12] = NULL; widgets[13] = NULL;
}

Screen* GuiOptionsScreen_MakeInstance(void) {
	static ButtonWidget buttons[11];
	static MenuInputValidator validators[Array_Elems(buttons)];
	static const UInt8* defaultValues[Array_Elems(buttons)];
	static Widget* widgets[Array_Elems(buttons) + 3];

	validators[2]    = MenuInputValidator_Real(0.25f, 4.00f);
	defaultValues[2] = "1";
	validators[3]    = MenuInputValidator_Real(0.25f, 4.00f);
	defaultValues[3] = "1";
	validators[6]    = MenuInputValidator_Real(0.25f, 4.00f);
	defaultValues[6] = "1";
	validators[7]    = MenuInputValidator_Integer(0, 30);
	defaultValues[7] = "10";
	validators[9]    = MenuInputValidator_String();
	defaultValues[9] = "Arial";

	return MenuOptionsScreen_MakeInstance(widgets, Array_Elems(widgets), buttons,
		GuiOptionsScreen_ContextRecreated, validators, defaultValues, NULL, 0);
}


/*########################################################################################################################*
*---------------------------------------------------HacksSettingsScreen---------------------------------------------------*
*#########################################################################################################################*/
void HacksSettingsScreen_GetHacks(STRING_TRANSIENT String* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.Enabled); }
void HacksSettingsScreen_SetHacks(STRING_PURE String* v) {
	LocalPlayer_Instance.Hacks.Enabled = Menu_SetBool(v,OPT_HACKS_ENABLED);
	LocalPlayer_CheckHacksConsistency();
}

void HacksSettingsScreen_GetSpeed(STRING_TRANSIENT String* v) { String_AppendReal32(v, LocalPlayer_Instance.Hacks.SpeedMultiplier, 2); }
void HacksSettingsScreen_SetSpeed(STRING_PURE String* v) {
	LocalPlayer_Instance.Hacks.SpeedMultiplier = Menu_Real32(v);
	Options_Set(OPT_SPEED_FACTOR, v);
}

void HacksSettingsScreen_GetClipping(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_CameraClipping); }
void HacksSettingsScreen_SetClipping(STRING_PURE String* v) {
	Game_CameraClipping = Menu_SetBool(v, OPT_CAMERA_CLIPPING);
}

void HacksSettingsScreen_GetJump(STRING_TRANSIENT String* v) { String_AppendReal32(v, LocalPlayer_JumpHeight(), 3); }
void HacksSettingsScreen_SetJump(STRING_PURE String* v) {
	PhysicsComp* physics = &LocalPlayer_Instance.Physics;
	PhysicsComp_CalculateJumpVelocity(physics, Menu_Real32(v));
	physics->UserJumpVel = physics->JumpVel;

	UInt8 strBuffer[String_BufferSize(STRING_SIZE)];
	String str = String_InitAndClearArray(strBuffer);
	String_AppendReal32(&str, physics->JumpVel, 8);
	Options_Set(OPT_JUMP_VELOCITY, &str);
}

void HacksSettingsScreen_GetWOMHacks(STRING_TRANSIENT String* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.WOMStyleHacks); }
void HacksSettingsScreen_SetWOMHacks(STRING_PURE String* v) {
	LocalPlayer_Instance.Hacks.WOMStyleHacks = Menu_SetBool(v, OPT_WOM_STYLE_HACKS);
}

void HacksSettingsScreen_GetFullStep(STRING_TRANSIENT String* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.FullBlockStep); }
void HacksSettingsScreen_SetFullStep(STRING_PURE String* v) {
	LocalPlayer_Instance.Hacks.FullBlockStep = Menu_SetBool(v, OPT_FULL_BLOCK_STEP);
}

void HacksSettingsScreen_GetPushback(STRING_TRANSIENT String* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.PushbackPlacing); }
void HacksSettingsScreen_SetPushback(STRING_PURE String* v) {
	LocalPlayer_Instance.Hacks.PushbackPlacing = Menu_SetBool(v, OPT_PUSHBACK_PLACING);
}

void HacksSettingsScreen_GetLiquids(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_BreakableLiquids); }
void HacksSettingsScreen_SetLiquids(STRING_PURE String* v) {
	Game_BreakableLiquids = Menu_SetBool(v, OPT_MODIFIABLE_LIQUIDS);
}

void HacksSettingsScreen_GetSlide(STRING_TRANSIENT String* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.NoclipSlide); }
void HacksSettingsScreen_SetSlide(STRING_PURE String* v) {
	LocalPlayer_Instance.Hacks.NoclipSlide = Menu_SetBool(v, OPT_NOCLIP_SLIDE);
}

void HacksSettingsScreen_GetFOV(STRING_TRANSIENT String* v) { String_AppendInt32(v, Game_Fov); }
void HacksSettingsScreen_SetFOV(STRING_PURE String* v) {
	Game_Fov = Menu_Int32(v);
	if (Game_ZoomFov > Game_Fov) Game_ZoomFov = Game_Fov;

	Options_Set(OPT_FIELD_OF_VIEW, v);
	Game_UpdateProjection();
}

void HacksSettingsScreen_CheckHacksAllowed(void* obj) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)obj;
	Widget** widgets = screen->WidgetsPtr;
	Int32 i;

	for (i = 0; i < screen->WidgetsCount; i++) {
		if (widgets[i] == NULL) continue;
		widgets[i]->Disabled = false;
	}

	LocalPlayer* p = &LocalPlayer_Instance;
	bool noGlobalHacks = !p->Hacks.CanAnyHacks || !p->Hacks.Enabled;
	widgets[3]->Disabled = noGlobalHacks || !p->Hacks.CanSpeed;
	widgets[4]->Disabled = noGlobalHacks || !p->Hacks.CanSpeed;
	widgets[5]->Disabled = noGlobalHacks || !p->Hacks.CanSpeed;
	widgets[7]->Disabled = noGlobalHacks || !p->Hacks.CanPushbackBlocks;
}

void HacksSettingsScreen_ContextLost(void* obj) {
	MenuOptionsScreen_ContextLost(obj);
	Event_UnregisterVoid(&UserEvents_HackPermissionsChanged, obj, HacksSettingsScreen_CheckHacksAllowed);
}

void HacksSettingsScreen_ContextRecreated(void* obj) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)obj;
	Widget** widgets = screen->WidgetsPtr;
	Event_RegisterVoid(&UserEvents_HackPermissionsChanged, obj, HacksSettingsScreen_CheckHacksAllowed);

	MenuOptionsScreen_Make(screen, 0, -1, -150, "Hacks enabled",    MenuOptionsScreen_Bool,
		HacksSettingsScreen_GetHacks,    HacksSettingsScreen_SetHacks);
	MenuOptionsScreen_Make(screen, 1, -1, -100, "Speed multiplier", MenuOptionsScreen_Input,
		HacksSettingsScreen_GetSpeed,    HacksSettingsScreen_SetSpeed);
	MenuOptionsScreen_Make(screen, 2, -1,  -50, "Camera clipping",  MenuOptionsScreen_Bool,
		HacksSettingsScreen_GetClipping, HacksSettingsScreen_SetClipping);
	MenuOptionsScreen_Make(screen, 3, -1,    0, "Jump height",      MenuOptionsScreen_Input,
		HacksSettingsScreen_GetJump,     HacksSettingsScreen_SetJump);
	MenuOptionsScreen_Make(screen, 4, -1,   50, "WOM style hacks",  MenuOptionsScreen_Bool,
		HacksSettingsScreen_GetWOMHacks, HacksSettingsScreen_SetWOMHacks);

	MenuOptionsScreen_Make(screen, 5, 1, -150, "Full block stepping", MenuOptionsScreen_Bool,
		HacksSettingsScreen_GetFullStep, HacksSettingsScreen_SetFullStep);
	MenuOptionsScreen_Make(screen, 6, 1, -100, "Breakable liquids",   MenuOptionsScreen_Bool,
		HacksSettingsScreen_GetLiquids,  HacksSettingsScreen_SetLiquids);
	MenuOptionsScreen_Make(screen, 7, 1,  -50, "Pushback placing",    MenuOptionsScreen_Bool,
		HacksSettingsScreen_GetPushback, HacksSettingsScreen_SetPushback);
	MenuOptionsScreen_Make(screen, 8, 1,    0, "Noclip slide",        MenuOptionsScreen_Bool,
		HacksSettingsScreen_GetSlide,    HacksSettingsScreen_SetSlide);
	MenuOptionsScreen_Make(screen, 9, 1,   50, "Field of view",       MenuOptionsScreen_Input,
		HacksSettingsScreen_GetFOV,      HacksSettingsScreen_SetFOV);

	Menu_MakeDefaultBack(&screen->Buttons[10], false, &screen->TitleFont, Menu_SwitchOptions);
	widgets[10] = (Widget*)(&screen->Buttons[10]);
	widgets[11] = NULL; widgets[12] = NULL; widgets[13] = NULL;
}

Screen* HacksSettingsScreen_MakeInstance(void) {
	static ButtonWidget buttons[11];
	static MenuInputValidator validators[Array_Elems(buttons)];
	static const UInt8* defaultValues[Array_Elems(buttons)];
	static Widget* widgets[Array_Elems(buttons) + 3];

	/* TODO: Is this needed because user may not always use . for decimal point? */
	static UInt8 jumpHeightBuffer[String_BufferSize(STRING_INT_CHARS)];
	String jumpHeight = String_InitAndClearArray(jumpHeightBuffer);
	String_AppendReal32(&jumpHeight, 1.233f, 3);

	validators[1]    = MenuInputValidator_Real(0.10f, 50.00f);
	defaultValues[1] = "10";
	validators[3]    = MenuInputValidator_Real(0.10f, 2048.00f);
	defaultValues[3] = jumpHeightBuffer;
	validators[9]    = MenuInputValidator_Integer(1, 150);
	defaultValues[9] = "70";

	static const UInt8* descs[Array_Elems(buttons)];
	descs[2] = "&eIf &fON&e, then the third person cameras will limit%"   "&etheir zoom distance if they hit a solid block.";
	descs[3] = "&eSets how many blocks high you can jump up.%"   "&eNote: You jump much higher when holding down the Speed key binding.";
	descs[7] = \
		"&eIf &fON&e, placing blocks that intersect your own position cause%" \
		"&ethe block to be placed, and you to be moved out of the way.%" \
		"&fThis is mainly useful for quick pillaring/towering.";
	descs[8] = "&eIf &fOFF&e, you will immediately stop when in noclip%"   "&emode and no movement keys are held down.";

	Screen* screen = MenuOptionsScreen_MakeInstance(widgets, Array_Elems(widgets), buttons,
		HacksSettingsScreen_ContextRecreated, validators, defaultValues, descs, Array_Elems(buttons));
	((MenuOptionsScreen*)screen)->ContextLost = HacksSettingsScreen_ContextLost;
	return screen;
}


/*########################################################################################################################*
*----------------------------------------------------MiscOptionsScreen----------------------------------------------------*
*#########################################################################################################################*/
void MiscOptionsScreen_GetReach(STRING_TRANSIENT String* v) { String_AppendReal32(v, LocalPlayer_Instance.ReachDistance, 2); }
void MiscOptionsScreen_SetReach(STRING_PURE String* v) { LocalPlayer_Instance.ReachDistance = Menu_Real32(v); }

void MiscOptionsScreen_GetMusic(STRING_TRANSIENT String* v) { String_AppendInt32(v, Game_MusicVolume); }
void MiscOptionsScreen_SetMusic(STRING_PURE String* v) {
	Game_MusicVolume = Menu_Int32(v);
	Options_Set(OPT_MUSIC_VOLUME, v);
	Audio_SetMusic(Game_MusicVolume);
}

void MiscOptionsScreen_GetSounds(STRING_TRANSIENT String* v) { String_AppendInt32(v, Game_SoundsVolume); }
void MiscOptionsScreen_SetSounds(STRING_PURE String* v) {
	Game_SoundsVolume = Menu_Int32(v);
	Options_Set(OPT_SOUND_VOLUME, v);
	Audio_SetSounds(Game_SoundsVolume);
}

void MiscOptionsScreen_GetViewBob(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_ViewBobbing); }
void MiscOptionsScreen_SetViewBob(STRING_PURE String* v) { Game_ViewBobbing = Menu_SetBool(v, OPT_VIEW_BOBBING); }

void MiscOptionsScreen_GetPhysics(STRING_TRANSIENT String* v) { Menu_GetBool(v, Physics_Enabled); }
void MiscOptionsScreen_SetPhysics(STRING_PURE String* v) {
	Physics_SetEnabled(Menu_SetBool(v, OPT_BLOCK_PHYSICS));
}

void MiscOptionsScreen_GetAutoClose(STRING_TRANSIENT String* v) { Menu_GetBool(v, Options_GetBool(OPT_AUTO_CLOSE_LAUNCHER, false)); }
void MiscOptionsScreen_SetAutoClose(STRING_PURE String* v) { Menu_SetBool(v, OPT_AUTO_CLOSE_LAUNCHER); }

void MiscOptionsScreen_GetInvert(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_InvertMouse); }
void MiscOptionsScreen_SetInvert(STRING_PURE String* v) { Game_InvertMouse = Menu_SetBool(v, OPT_INVERT_MOUSE); }

void MiscOptionsScreen_GetSensitivity(STRING_TRANSIENT String* v) { String_AppendInt32(v, Game_MouseSensitivity); }
void MiscOptionsScreen_SetSensitivity(STRING_PURE String* v) {
	Game_MouseSensitivity = Menu_Int32(v);
	Options_Set(OPT_SENSITIVITY, v);
}

void MiscOptionsScreen_ContextRecreated(void* obj) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)obj;
	Widget** widgets = screen->WidgetsPtr;

	MenuOptionsScreen_Make(screen, 0, -1, -100, "Reach distance", MenuOptionsScreen_Input,
		MiscOptionsScreen_GetReach,       MiscOptionsScreen_SetReach);
	MenuOptionsScreen_Make(screen, 1, -1,  -50, "Music volume",   MenuOptionsScreen_Input,
		MiscOptionsScreen_GetMusic,       MiscOptionsScreen_SetMusic);
	MenuOptionsScreen_Make(screen, 2, -1,    0, "Sounds volume",  MenuOptionsScreen_Input,
		MiscOptionsScreen_GetSounds,      MiscOptionsScreen_SetSounds);
	MenuOptionsScreen_Make(screen, 3, -1,   50, "View bobbing",   MenuOptionsScreen_Bool,
		MiscOptionsScreen_GetViewBob,     MiscOptionsScreen_SetViewBob);

	MenuOptionsScreen_Make(screen, 4, 1, -100, "Block physics",       MenuOptionsScreen_Bool,
		MiscOptionsScreen_GetPhysics,     MiscOptionsScreen_SetPhysics);
	MenuOptionsScreen_Make(screen, 5, 1,  -50, "Auto close launcher", MenuOptionsScreen_Bool,
		MiscOptionsScreen_GetAutoClose,   MiscOptionsScreen_SetAutoClose);
	MenuOptionsScreen_Make(screen, 6, 1,    0, "Invert mouse",        MenuOptionsScreen_Bool,
		MiscOptionsScreen_GetInvert,      MiscOptionsScreen_SetInvert);
	MenuOptionsScreen_Make(screen, 7, 1,   50, "Mouse sensitivity",   MenuOptionsScreen_Input,
		MiscOptionsScreen_GetSensitivity, MiscOptionsScreen_SetSensitivity);

	Menu_MakeDefaultBack(&screen->Buttons[8], false, &screen->TitleFont, Menu_SwitchOptions);
	widgets[8] = (Widget*)(&screen->Buttons[8]);
	widgets[9] = NULL; widgets[10] = NULL; widgets[11] = NULL;

	/* Disable certain options */
	if (!ServerConnection_IsSinglePlayer) MenuScreen_Remove((MenuScreen*)screen, 0);
	if (!ServerConnection_IsSinglePlayer) MenuScreen_Remove((MenuScreen*)screen, 4);
}

Screen* MiscOptionsScreen_MakeInstance(void) {
	static ButtonWidget buttons[9];
	static MenuInputValidator validators[Array_Elems(buttons)];
	static const UInt8* defaultValues[Array_Elems(buttons)];
	static Widget* widgets[Array_Elems(buttons) + 3];

	validators[0]    = MenuInputValidator_Real(1.00f, 1024.00f);
	defaultValues[0] = "5";
	validators[1]    = MenuInputValidator_Integer(0, 100);
	defaultValues[1] = "0";
	validators[2]    = MenuInputValidator_Integer(0, 100);
	defaultValues[2] = "0";
	validators[7]    = MenuInputValidator_Integer(1, 200);
	defaultValues[7] = "30";

	return MenuOptionsScreen_MakeInstance(widgets, Array_Elems(widgets), buttons,
		MiscOptionsScreen_ContextRecreated, validators, defaultValues, NULL, 0);
}


/*########################################################################################################################*
*-----------------------------------------------------NostalgiaScreen-----------------------------------------------------*
*#########################################################################################################################*/
void NostalgiaScreen_GetHand(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_ClassicArmModel); }
void NostalgiaScreen_SetHand(STRING_PURE String* v) { Game_ClassicArmModel = Menu_SetBool(v, OPT_CLASSIC_ARM_MODEL); }

void NostalgiaScreen_GetAnim(STRING_TRANSIENT String* v) { Menu_GetBool(v, !Game_SimpleArmsAnim); }
void NostalgiaScreen_SetAnim(STRING_PURE String* v) {
	Game_SimpleArmsAnim = String_CaselessEqualsConst(v, "OFF");
	Options_SetBool(OPT_SIMPLE_ARMS_ANIM, Game_SimpleArmsAnim);
}

void NostalgiaScreen_GetGui(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_UseClassicGui); }
void NostalgiaScreen_SetGui(STRING_PURE String* v) { Game_UseClassicGui = Menu_SetBool(v, OPT_USE_CLASSIC_GUI); }

void NostalgiaScreen_GetList(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_UseClassicTabList); }
void NostalgiaScreen_SetList(STRING_PURE String* v) { Game_UseClassicTabList = Menu_SetBool(v, OPT_USE_CLASSIC_TABLIST); }

void NostalgiaScreen_GetOpts(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_UseClassicOptions); }
void NostalgiaScreen_SetOpts(STRING_PURE String* v) { Game_UseClassicOptions = Menu_SetBool(v, OPT_USE_CLASSIC_OPTIONS); }

void NostalgiaScreen_GetCustom(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_AllowCustomBlocks); }
void NostalgiaScreen_SetCustom(STRING_PURE String* v) { Game_AllowCustomBlocks = Menu_SetBool(v, OPT_USE_CUSTOM_BLOCKS); }

void NostalgiaScreen_GetCPE(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_UseCPE); }
void NostalgiaScreen_SetCPE(STRING_PURE String* v) { Game_UseCPE = Menu_SetBool(v, OPT_USE_CPE); }

void NostalgiaScreen_GetTexs(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_AllowServerTextures); }
void NostalgiaScreen_SetTexs(STRING_PURE String* v) { Game_AllowServerTextures = Menu_SetBool(v, OPT_USE_SERVER_TEXTURES); }

void NostalgiaScreen_SwitchBack(GuiElement* a, GuiElement* b) {
	if (Game_UseClassicOptions) { Menu_SwitchPause(a, b); } else { Menu_SwitchOptions(a, b); }
}

void NostalgiaScreen_ContextRecreated(void* obj) {
	MenuOptionsScreen* screen = (MenuOptionsScreen*)obj;
	Widget** widgets = screen->WidgetsPtr;
	static TextWidget desc;

	MenuOptionsScreen_Make(screen, 0, -1, -150, "Classic hand model",   MenuOptionsScreen_Bool,
		NostalgiaScreen_GetHand,   NostalgiaScreen_SetHand);
	MenuOptionsScreen_Make(screen, 1, -1, -100, "Classic walk anim",    MenuOptionsScreen_Bool, 
		NostalgiaScreen_GetAnim,   NostalgiaScreen_SetAnim);
	MenuOptionsScreen_Make(screen, 2, -1,  -50, "Classic gui textures", MenuOptionsScreen_Bool,
		NostalgiaScreen_GetGui,    NostalgiaScreen_SetGui);
	MenuOptionsScreen_Make(screen, 3, -1,    0, "Classic player list",  MenuOptionsScreen_Bool, 
		NostalgiaScreen_GetList,   NostalgiaScreen_SetList);
	MenuOptionsScreen_Make(screen, 4, -1,   50, "Classic options",      MenuOptionsScreen_Bool, 
		NostalgiaScreen_GetOpts,   NostalgiaScreen_SetOpts);

	MenuOptionsScreen_Make(screen, 5, 1, -150, "Allow custom blocks", MenuOptionsScreen_Bool, 
		NostalgiaScreen_GetCustom, NostalgiaScreen_SetCustom);
	MenuOptionsScreen_Make(screen, 6, 1, -100, "Use CPE",             MenuOptionsScreen_Bool, 
		NostalgiaScreen_GetCPE,    NostalgiaScreen_SetCPE);
	MenuOptionsScreen_Make(screen, 7, 1,  -50, "Use server textures", MenuOptionsScreen_Bool, 
		NostalgiaScreen_GetTexs,   NostalgiaScreen_SetTexs);


	Menu_MakeDefaultBack(&screen->Buttons[8], false, &screen->TitleFont, NostalgiaScreen_SwitchBack);
	widgets[8] = (Widget*)(&screen->Buttons[8]);

	String descText = String_FromConst("&eButtons on the right require restarting game");
	TextWidget_Create(&desc, &descText, &screen->TextFont);
	Widget_SetLocation((Widget*)(&desc), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 100);
	widgets[9] = (Widget*)(&desc);
}

Screen* NostalgiaScreen_MakeInstance(void) {
	static ButtonWidget buttons[9];
	static Widget* widgets[Array_Elems(buttons) + 1];

	return MenuOptionsScreen_MakeInstance(widgets, Array_Elems(widgets), buttons,
		NostalgiaScreen_ContextRecreated, NULL, NULL, NULL, 0);
}