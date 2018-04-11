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

#define FILES_SCREEN_ITEMS 5
#define FILES_SCREEN_BUTTONS (FILES_SCREEN_ITEMS + 3)

typedef struct ListScreen_ {
	Screen_Layout
	FontDesc Font;
	Int32 CurrentIndex;
	Widget_LeftClick EntryClick;
	String TitleText;
	ButtonWidget Buttons[FILES_SCREEN_BUTTONS];
	TextWidget Title;
	Widget* Widgets[FILES_SCREEN_BUTTONS + 1];
	StringsBuffer Entries; /* NOTE: this is the last member so we can avoid memsetting it to 0 */
} ListScreen;

typedef void (*Menu_ContextRecreated)(GuiElement* elem);
#define MenuScreen_Layout \
Screen_Layout \
Widget** WidgetsPtr; \
Int32 WidgetsCount; \
FontDesc TitleFont, TextFont; \
Menu_ContextRecreated ContextRecreated;

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
		Menu_MakeBack(widget, width, &msg, 25, font, onClick);
	} else {
		String msg = String_FromConst("Cancel");
		Menu_MakeBack(widget, width, &msg, 25, font, onClick);
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

Int32 Menu_Index(Widget** widgets, Int32 widgetsCount, Widget* w) {
	Int32 i;
	for (i = 0; i < widgetsCount; i++) {
		if (widgets[i] == w) return i;
	}
	return -1;
}


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

void ListScreen_MakeText(ListScreen* screen, Int32 idx, Int32 x, Int32 y, String* text) {
	ButtonWidget* widget = &screen->Buttons[idx];
	ButtonWidget_Create(widget, text, 300, &screen->Font, screen->EntryClick);
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
	Menu_MakeDefaultBack(&screen->Buttons[7], false, &screen->Font, Menu_SwitchPause);

	screen->Widgets[0] = (Widget*)(&screen->Title);
	for (i = 0; i < FILES_SCREEN_BUTTONS; i++) {
		screen->Widgets[i + 1] = (Widget*)(&screen->Buttons[i]);
	}
	ListScreen_UpdateArrows(screen);
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

void ListScreen_AddFilename(void* obj, STRING_PURE String* path) {
	/* folder1/folder2/entry.zip --> entry.zip */
	Int32 lastDir = String_LastIndexOf(path, Platform_DirectorySeparator);
	String filename = *path;
	if (lastDir >= 0) {
		filename = String_UNSAFE_SubstringAt(&filename, lastDir + 1);
	}

	StringsBuffer* entries = (StringsBuffer*)obj;
	StringsBuffer_Add(entries, &filename);
}

void ListScreen_MakePath(ListScreen* screen, GuiElement* w, STRING_PURE String* path, const UInt8* dir, STRING_REF String* filename) {
	Int32 idx = Menu_Index(screen->Widgets, Array_Elems(screen->Widgets), (Widget*)w);
	*filename = StringsBuffer_UNSAFE_Get(&screen->Entries, screen->CurrentIndex + idx);

	String_AppendConst(path, dir);
	String_Append(path, Platform_DirectorySeparator);
	String_AppendString(path, filename);
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

ListScreen* ListScreen_MakeInstance(void) {
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
	return screen;
}


/*########################################################################################################################*
*--------------------------------------------------------MenuScreen-------------------------------------------------------*
*#########################################################################################################################*/
GuiElementVTABLE MenuScreen_VTABLE;
Int32 MenuScreen_Index(MenuScreen* screen, Widget* w) {
	return Menu_Index(screen->WidgetsPtr, screen->WidgetsCount, w);
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
	if (key == Key_Escape) { Gui_SetNewScreen(NULL); }
	return key < Key_F1 || key > Key_F35;
}
bool MenuScreen_HandlesKeyPress(GuiElement* elem, UInt8 key) { return true; }
bool MenuScreen_HandlesKeyUp(GuiElement* elem, Key key) { return true; }

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

	if (screen->TitleFont.Handle == NULL) {
		Platform_MakeFont(&screen->TitleFont, &Game_FontName, 16, FONT_STYLE_BOLD);
	}
	if (screen->TextFont.Handle == NULL) {
		Platform_MakeFont(&screen->TextFont, &Game_FontName, 16, FONT_STYLE_NORMAL);
	}

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
	if (screen->TextFont.Handle != NULL) {
		Platform_FreeFont(&screen->TextFont);
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
	ButtonWidget_Create(btn, &text, 300, &screen->TitleFont, onClick);
	Widget_SetLocation((Widget*)btn, ANCHOR_CENTRE, ANCHOR_CENTRE, dir * 160, y);
}

void PauseScreen_MakeClassic(PauseScreen* screen, Int32 i, Int32 y, const UInt8* title, Widget_LeftClick onClick) {	
	ButtonWidget* btn = &screen->Buttons[i];
	screen->Widgets[i] = (Widget*)btn;

	String text = String_FromReadonly(title);
	ButtonWidget_Create(btn, &text, 400, &screen->TitleFont, onClick);
	Widget_SetLocation((Widget*)btn, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, y);
}

void PauseScreen_GenLevel(GuiElement* a, GuiElement* b)         { Gui_SetNewScreen(GenLevelScreen_MakeInstance()); }
void PauseScreen_ClassicGenLevel(GuiElement* a, GuiElement* b)  { Gui_SetNewScreen(ClassicGenScreen_MakeInstance()); }
void PauseScreen_LoadLevel(GuiElement* a, GuiElement* b)        { Gui_SetNewScreen(LoadLevelScreen_MakeInstance()); }
void PauseScreen_SaveLevel(GuiElement* a, GuiElement* b)        { Gui_SetNewScreen(SaveLevelScreen_MakeInstance()); }
void PauseScreen_TexPack(GuiElement* a, GuiElement* b)          { Gui_SetNewScreen(TexturePackScreen_MakeInstance()); }
void PauseScreen_Hotkeys(GuiElement* a, GuiElement* b)          { Gui_SetNewScreen(HotkeyListScreen_MakeInstance()); }
void PauseScreen_NostalgiaOptions(GuiElement* a, GuiElement* b) { Gui_SetNewScreen(NostalgiaScreen_MakeInstance()); }
void PauseScreen_Game(GuiElement* a, GuiElement* b)             { Gui_SetNewScreen(NULL); }
void PauseScreen_ClassicOptions(GuiElement* a, GuiElement* b)   { Gui_SetNewScreen(ClassicOptionsScreen_MakeInstance()); }
void PauseScreen_Quit(GuiElement* a, GuiElement* b) { Window_Close(); }

void PauseScreen_CheckHacksAllowed(void* obj) {
	if (Game_UseClassicOptions) return;
	PauseScreen* screen = (PauseScreen*)obj;
	screen->Buttons[4].Disabled = LocalPlayer_Instance.Hacks.CanAnyHacks; /* select texture pack */
}

void PauseScreen_ContextRecreated(void* obj) {
	PauseScreen* screen = (PauseScreen*)obj;
	FontDesc* font = &screen->TitleFont;

	if (Game_UseClassicOptions) {
		PauseScreen_MakeClassic(screen, 0, -100, "Options...",            PauseScreen_ClassicOptions);
		PauseScreen_MakeClassic(screen, 1,  -50, "Generate new level...", PauseScreen_ClassicGenLevel);
		PauseScreen_MakeClassic(screen, 2,    0, "Load level...",         PauseScreen_LoadLevel);
		PauseScreen_MakeClassic(screen, 3,   50, "Save level...",         PauseScreen_SaveLevel);
		PauseScreen_MakeClassic(screen, 4,  150, "Nostalgia options...",  PauseScreen_NostalgiaOptions);

		String back = String_FromConst("Back to game");
		screen->Widgets[5] = (Widget*)(&screen->Buttons[5]);
		Menu_MakeBack(&screen->Buttons[5], 400, &back, 25, font, PauseScreen_Game);	

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
		screen->Widgets[6] = (Widget*)(&screen->Buttons[6]);
		ButtonWidget_Create(&screen->Buttons[6], &quitMsg, 120, font, PauseScreen_Quit);		
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
	ButtonWidget_Create(btn, &text, 300, &screen->TitleFont, onClick);
	Widget_SetLocation((Widget*)btn, ANCHOR_CENTRE, ANCHOR_CENTRE, dir * 160, y);
}

void OptionsGroupScreen_MakeDesc(OptionsGroupScreen* screen) {
	screen->Widgets[8] = (Widget*)(&screen->Desc);
	String text = String_FromReadonly(optsGroup_descs[screen->SelectedI]);
	TextWidget_Create(&screen->Desc, &text, &screen->TextFont);
	Widget_SetLocation((Widget*)(&screen->Desc), ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 100);
}

void OptionsGroupScreen_Misc(GuiElement* a, GuiElement* b)      { Gui_SetNewScreen(MiscOptionsScreen_MakeInstance()); }
void OptionsGroupScreen_Gui(GuiElement* a, GuiElement* b)       { Gui_SetNewScreen(GuiOptionsScreen_MakeInstance()); }
void OptionsGroupScreen_Gfx(GuiElement* a, GuiElement* b)       { Gui_SetNewScreen(GraphicsOptionsScreen_MakeInstance()); }
void OptionsGroupScreen_Controls(GuiElement* a, GuiElement* b)  { Gui_SetNewScreen(NormalKeyBindingsScreen_MakeInstance()); }
void OptionsGroupScreen_Hacks(GuiElement* a, GuiElement* b)     { Gui_SetNewScreen(HacksSettingsScreen_MakeInstance()); }
void OptionsGroupScreen_Env(GuiElement* a, GuiElement* b)       { Gui_SetNewScreen(EnvSettingsScreen_MakeInstance()); }
void OptionsGroupScreen_Nostalgia(GuiElement* a, GuiElement* b) { Gui_SetNewScreen(NostalgiaScreen_MakeInstance()); }

void OptionsGroupScreen_ContextRecreated(void* obj) {
	OptionsGroupScreen* screen = (OptionsGroupScreen*)obj;

	OptionsGroupScreen_Make(screen, 0, -1, -100, "Misc options...",      OptionsGroupScreen_Misc);
	OptionsGroupScreen_Make(screen, 1, -1,  -50, "Gui options...",       OptionsGroupScreen_Gui);
	OptionsGroupScreen_Make(screen, 2, -1,    0, "Graphics options...",  OptionsGroupScreen_Gfx);
	OptionsGroupScreen_Make(screen, 3, -1,   50, "Controls...",          OptionsGroupScreen_Controls);
	OptionsGroupScreen_Make(screen, 4,  1,  -50, "Hacks settings...",    OptionsGroupScreen_Hacks);
	OptionsGroupScreen_Make(screen, 5,  1,    0, "Env settings...",      OptionsGroupScreen_Env);
	OptionsGroupScreen_Make(screen, 6,  1,   50, "Nostalgia options...", OptionsGroupScreen_Nostalgia);

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
	Elem_Free(&screen->Desc);
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
void DeathScreen_Gen(GuiElement* a, GuiElement* b)  { Gui_SetNewScreen(GenLevelScreen_MakeInstance()); }
void DeathScreen_Load(GuiElement* a, GuiElement* b) { Gui_SetNewScreen(LoadLevelScreen_MakeInstance()); }

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
	ButtonWidget_Create(&screen->Gen, &gen, 400, &screen->TitleFont, DeathScreen_Gen);
	Widget_SetLocation(screen->Widgets[2], ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 25);

	String load = String_FromConst("Load level...");
	screen->Widgets[3] = (Widget*)(&screen->Load);
	ButtonWidget_Create(&screen->Load, &load, 400, &screen->TitleFont, DeathScreen_Load);
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

	ButtonWidget_Create(btn, text, 300, &screen->TitleFont, onClick);
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
	if (hotkey.BaseKey != Key_Unknown) {
		Hotkeys_Remove(hotkey.BaseKey, hotkey.Flags);
		Hotkeys_UserRemovedHotkey(hotkey.BaseKey, hotkey.Flags);
	}

	hotkey = screen->CurHotkey;
	if (hotkey.BaseKey != Key_Unknown) {
		String text = screen->Input.Base.Text;
		Hotkeys_Add(hotkey.BaseKey, hotkey.Flags, &text, hotkey.StaysOpen);
		Hotkeys_UserAddedHotkey(hotkey.BaseKey, hotkey.Flags, hotkey.StaysOpen, &text);
	}
	Gui_SetNewScreen(HotkeyListScreen_MakeInstance());
}

void EditHotkeyScreen_RemoveHotkey(GuiElement* elem, GuiElement* widget) {
	EditHotkeyScreen* screen = (EditHotkeyScreen*)elem;
	HotkeyData hotkey = screen->OrigHotkey;
	if (hotkey.BaseKey != Key_Unknown) {
		Hotkeys_Remove(hotkey.BaseKey, hotkey.Flags);
		Hotkeys_UserRemovedHotkey(hotkey.BaseKey, hotkey.Flags);
	}
	Gui_SetNewScreen(HotkeyListScreen_MakeInstance());
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

	bool existed = screen->OrigHotkey.BaseKey != Key_Unknown;
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
	ButtonWidget_Create(&screen->Buttons[0], &flatgrass, 200, &screen->TitleFont, GenLevelScreen_Flatgrass);
	Widget_SetLocation(screen->Widgets[9], ANCHOR_CENTRE, ANCHOR_CENTRE, -120, 100);

	String vanilla = String_FromConst("Vanilla");
	screen->Widgets[10] = (Widget*)(&screen->Buttons[1]);
	ButtonWidget_Create(&screen->Buttons[1], &vanilla, 200, &screen->TitleFont, GenLevelScreen_Notchy);
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
	ButtonWidget_Create(btn, &text, 400, &screen->TitleFont, onClick);
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
*---------------------------------------------------TexturePackScreen-----------------------------------------------------*
*#########################################################################################################################*/
void TexturePackScreen_EntryClick(GuiElement* screenElem, GuiElement* w) {
	ListScreen* screen = (ListScreen*)screenElem;
	UInt8 pathBuffer[String_BufferSize(FILENAME_SIZE)];
	String path = String_InitAndClearArray(pathBuffer), filename;
	ListScreen_MakePath(screen, w, &path, "texpacks", &filename);
	if (!Platform_FileExists(&path)) return;
	
	Int32 curPage = screen->CurrentIndex;
	Game_SetDefaultTexturePack(&filename);
	TexturePack_ExtractDefault();
	Elem_Recreate(screen);
	ListScreen_SetCurrentIndex(screen, curPage);
}

void TexturePackScreen_SelectEntry(STRING_PURE String* path, void* obj) {
	String zip = String_FromConst(".zip");
	if (!String_CaselessEnds(path, &zip)) return;
	ListScreen_AddFilename(obj, path);
}

Screen* TexturePackScreen_MakeInstance(void) {
	ListScreen* screen = ListScreen_MakeInstance();
	String title = String_FromConst("Select a texture pack zip");
	screen->TitleText = title;
	screen->EntryClick = TexturePackScreen_EntryClick;

	String path = String_FromConst("texpacks");
	Platform_EnumFiles(&path, &screen->Entries, TexturePackScreen_SelectEntry);
	if (screen->Entries.Count > 0) {
		ListScreen_QuickSort(0, screen->Entries.Count - 1);
	}
	return (Screen*)screen;
}


/*########################################################################################################################*
*----------------------------------------------------LoadLevelScreen------------------------------------------------------*
*#########################################################################################################################*/
void LoadLevelScreen_SelectEntry(STRING_PURE String* path, void* obj) {
	String cw  = String_FromConst(".cw");  String lvl = String_FromConst(".lvl");
	String fcm = String_FromConst(".fcm"); String dat = String_FromConst(".dat");

	if (!(String_CaselessEnds(path, &cw) || String_CaselessEnds(path, &lvl) 
		|| String_CaselessEnds(path, &fcm) || String_CaselessEnds(path, &dat))) return;
	ListScreen_AddFilename(obj, path);
}

void LoadLevelScreen_EntryClick(GuiElement* screenElem, GuiElement* w) {
	ListScreen* screen = (ListScreen*)screenElem;
	UInt8 pathBuffer[String_BufferSize(FILENAME_SIZE)];
	String path = String_InitAndClearArray(pathBuffer), filename;
	ListScreen_MakePath(screen, w, &path, "maps", &filename);
	if (!Platform_FileExists(&path)) return;

	void* file;
	ReturnCode code = Platform_FileOpen(&file, &path, true);
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
	String title = String_FromConst("Select a level");
	screen->TitleText = title;
	screen->EntryClick = LoadLevelScreen_EntryClick;

	String path = String_FromConst("maps");
	Platform_EnumFiles(&path, &screen->Entries, LoadLevelScreen_SelectEntry);
	if (screen->Entries.Count > 0) {
		ListScreen_QuickSort(0, screen->Entries.Count - 1);
	}
	return (Screen*)screen;
}