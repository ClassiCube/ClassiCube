#include "Menus.h"
#include "Widgets.h"
#include "Game.h"
#include "Event.h"
#include "Platform.h"
#include "Inventory.h"
#include "Drawer2D.h"
#include "Graphics.h"
#include "Funcs.h"
#include "Model.h"
#include "Generator.h"
#include "Server.h"
#include "Chat.h"
#include "ExtMath.h"
#include "Window.h"
#include "Camera.h"
#include "Http.h"
#include "Block.h"
#include "World.h"
#include "Formats.h"
#include "BlockPhysics.h"
#include "MapRenderer.h"
#include "TexturePack.h"
#include "Audio.h"
#include "Screens.h"
#include "Gui.h"
#include "Deflate.h"
#include "Stream.h"
#include "Builder.h"
#include "Lighting.h"
#include "Logger.h"
#include "Options.h"
#include "Input.h"
#include "Utils.h"
#include "Errors.h"
#include "SystemFonts.h"
#include "Lighting.h"
#include "InputHandler.h"
#include "Protocol.h"

/*########################################################################################################################*
*--------------------------------------------------------Menu base--------------------------------------------------------*
*#########################################################################################################################*/
void Menu_AddButtons(void* screen, struct ButtonWidget* btns, int width, const struct SimpleButtonDesc* descs, int count) {
	int i;
	for (i = 0; i < count; i++) {
		ButtonWidget_Add(screen, &btns[i], width, descs[i].onClick);
	}
}

void Menu_LayoutButtons(struct ButtonWidget* btns, const struct SimpleButtonDesc* descs, int count) {
	int i;
	for (i = 0; i < count; i++) {
		Widget_SetLocation(&btns[i], ANCHOR_CENTRE, ANCHOR_CENTRE,  descs[i].x, descs[i].y);
	}
}

void Menu_SetButtons(struct ButtonWidget* btns, struct FontDesc* font, const struct SimpleButtonDesc* descs, int count) {
	int i;
	for (i = 0; i < count; i++) {
		ButtonWidget_SetConst(&btns[i], descs[i].title, font);
	}
}

void Menu_LayoutBack(struct ButtonWidget* btn) {
	Widget_SetLocation(btn, ANCHOR_CENTRE, ANCHOR_MAX, 0, 25);
}
static void Menu_CloseKeyboard(void* s) { OnscreenKeyboard_Close(); }

static void Menu_RenderBounds(void) {
	/* These were sourced by taking a screenshot of vanilla
	Then using paint to extract the color components
	Then using wolfram alpha to solve the glblendfunc equation */
	PackedCol topCol    = PackedCol_Make(24, 24, 24, 105);
	PackedCol bottomCol = PackedCol_Make(51, 51, 98, 162);
	Gfx_Draw2DGradient(0, 0, Window_UI.Width, Window_UI.Height, topCol, bottomCol);
}


static void Menu_UnselectAll(struct Screen* s) {
	int i;
	/* TODO: pointer ID mask */

	for (i = 0; i < s->numWidgets; i++) 
	{
		struct Widget* w = s->widgets[i];
		if (w) w->active = false;
	}
}

int Menu_PointerDown(void* screen, int id, int x, int y) {
	Screen_DoPointerDown(screen, id, x, y); 
	return TOUCH_TYPE_GUI;
}

int Menu_DoPointerMove(void* screen, int id, int x, int y) {
	struct Screen* s = (struct Screen*)screen;
	struct Widget** widgets;
	int i, count;

	Menu_UnselectAll(s);
	widgets = s->widgets;
	count   = s->numWidgets;

	for (i = count - 1; i >= 0; i--) 
	{
		struct Widget* w = widgets[i];
		if (!w || !Widget_Contains(w, x, y)) continue;

		w->active = true;
		return i;
	}
	return -1;
}

int Menu_PointerMove(void* screen, int id, int x, int y) {
	Menu_DoPointerMove(screen, id, x, y); return true;
}

static cc_bool Menu_IsSelectable(struct Widget* w) {
	if (!w || (w->flags & WIDGET_FLAG_DISABLED)) return false;
	if (!(w->flags & WIDGET_FLAG_SELECTABLE))    return false;

	return true;
}

static void Menu_SelectWidget(struct Screen* s, int i) {
	struct Widget* w= s->widgets[i];

	Menu_UnselectAll(s);
	s->selectedI = i;
	w->active    = true;
}

static int Menu_CycleSelected(struct Screen* s, int dir) {
	struct Widget* w;
	int index = s->selectedI + dir;
	int i, j;

	for (j = 0; j < s->numWidgets; j++) 
	{
		i = (index + j * dir) % s->numWidgets;
		if (i < 0) i += s->numWidgets;

		w = s->widgets[i];
		if (!Menu_IsSelectable(w)) continue;

		Menu_SelectWidget(s, i);
		return true;
	}
	return false;
}

static int Menu_InputSelected(struct Screen* s, int key, struct InputDevice* device) {
	struct Widget* w;
	if (s->selectedI < 0) return false;

	w = s->widgets[s->selectedI];
	if (!Menu_IsSelectable(w)) return false;

	if (w->MenuClick && InputDevice_IsEnter(key, device)) {
		w->MenuClick(s, w);
		return true;
	}
	return Elem_HandlesKeyDown(w, key, device);
}

static int Menu_DoInputDown(void* screen, int key, struct InputDevice* device) {
	struct Screen* s = (struct Screen*)screen;
	
	if (key == device->upButton) {
		return Menu_CycleSelected(s, -1);
	} else if (key == device->downButton) {
		return Menu_CycleSelected(s, +1);
	} else {
		return Menu_InputSelected(s, key, device);
	}
}

int Menu_InputDown(void* screen, int key, struct InputDevice* device) {
	Menu_DoInputDown(screen, key, device);
	return Screen_InputDown(screen, key, device);
}


/*########################################################################################################################*
*------------------------------------------------------Menu utilities-----------------------------------------------------*
*#########################################################################################################################*/
#ifndef CC_DISABLE_UI
static void Menu_BeginGen(int width, int height, int length) {
	World_NewMap();
	World_SetDimensions(width, height, length);

	Gen_Start();
	GeneratingScreen_Show();
}

static void Menu_SwitchOptions(void* a, void* b)        { OptionsGroupScreen_Show(); }
static void Menu_SwitchPause(void* a, void* b)          { Gui_ShowPauseMenu(); }
static void Menu_SwitchClassicOptions(void* a, void* b) { ClassicOptionsScreen_Show(); }

static void Menu_SwitchBindsClassic(void* a, void* b)      { ClassicBindingsScreen_Show(); }
static void Menu_SwitchBindsClassicHacks(void* a, void* b) { ClassicHacksBindingsScreen_Show(); }
static void Menu_SwitchBindsNormal(void* a, void* b)       { NormalBindingsScreen_Show(); }
static void Menu_SwitchBindsHacks(void* a, void* b)        { HacksBindingsScreen_Show(); }
static void Menu_SwitchBindsOther(void* a, void* b)        { OtherBindingsScreen_Show(); }
static void Menu_SwitchBindsMouse(void* a, void* b)        { MouseBindingsScreen_Show(); }
static void Menu_SwitchBindsHotbar(void* a, void* b)       { HotbarBindingsScreen_Show(); }
static void SwitchBindsMain(void* s, void* w);

static void Menu_SwitchMisc(void* a, void* b)      { MiscOptionsScreen_Show(); }
static void Menu_SwitchChat(void* a, void* b)      { ChatOptionsScreen_Show(); }
static void Menu_SwitchGui(void* a, void* b)       { GuiOptionsScreen_Show(); }
static void Menu_SwitchGfx(void* a, void* b)       { GraphicsOptionsScreen_Show(); }
static void Menu_SwitchHacks(void* a, void* b)     { HacksSettingsScreen_Show(); }
static void Menu_SwitchEnv(void* a, void* b)       { EnvSettingsScreen_Show(); }
static void Menu_SwitchNostalgia(void* a, void* b) { NostalgiaMenuScreen_Show(); }

static void Menu_SwitchGenLevel(void* a, void* b)        { GenLevelScreen_Show(); }
static void Menu_SwitchClassicGenLevel(void* a, void* b) { ClassicGenScreen_Show(); }
static void Menu_SwitchLoadLevel(void* a, void* b)       { LoadLevelScreen_Show(); }
static void Menu_SwitchSaveLevel(void* a, void* b)       { SaveLevelScreen_Show(); }
static void Menu_SwitchTexPacks(void* a, void* b)        { TexturePackScreen_Show(); }
static void Menu_SwitchHotkeys(void* a, void* b)         { HotkeyListScreen_Show(); }


/*########################################################################################################################*
*--------------------------------------------------------ListScreen-------------------------------------------------------*
*#########################################################################################################################*/
struct ListScreen;
#define LIST_SCREEN_ITEMS 5

static struct ListScreen {
	Screen_Body
	struct ButtonWidget btns[LIST_SCREEN_ITEMS];
	struct ButtonWidget left, right, done, action;
	struct FontDesc font;
	int currentIndex;
	Widget_LeftClick EntryClick, DoneClick, ActionClick;
	const char* actionText;
	void (*LoadEntries)(struct ListScreen* s);
	void (*UpdateEntry)(struct ListScreen* s, struct ButtonWidget* btn, const cc_string* text);
	const char* titleText;
	struct TextWidget title;
	struct StringsBuffer entries;
} ListScreen;

static struct Widget* list_widgets[LIST_SCREEN_ITEMS + 4 + 1];
#define LISTSCREEN_EMPTY "-"

static void ListScreen_Layout(void* screen) {
	struct ListScreen* s = (struct ListScreen*)screen;
	int i;

	for (i = 0; i < LIST_SCREEN_ITEMS; i++) { 
		Widget_SetLocation(&s->btns[i],
			ANCHOR_CENTRE, ANCHOR_CENTRE, 0, (i - 2) * 50);
	}

	if (Input_TouchMode) {
		Widget_SetLocation(&s->done,   ANCHOR_CENTRE_MIN, ANCHOR_MAX, -150, 25);
		Widget_SetLocation(&s->action, ANCHOR_CENTRE_MAX, ANCHOR_MAX, -150, 25);
	} else {
		Widget_SetLocation(&s->done,   ANCHOR_CENTRE, ANCHOR_MAX, 0, 25);
		Widget_SetLocation(&s->action, ANCHOR_CENTRE, ANCHOR_MAX, 0, 70);
	}

	Widget_SetLocation(&s->left,  ANCHOR_CENTRE, ANCHOR_CENTRE, -220,    0);
	Widget_SetLocation(&s->right, ANCHOR_CENTRE, ANCHOR_CENTRE,  220,    0);
	Widget_SetLocation(&s->title, ANCHOR_CENTRE, ANCHOR_CENTRE,    0, -155);
}

static STRING_REF cc_string ListScreen_UNSAFE_Get(struct ListScreen* s, int index) {
	static const cc_string str = String_FromConst(LISTSCREEN_EMPTY);

	if (index >= 0 && index < s->entries.count) {
		return StringsBuffer_UNSAFE_Get(&s->entries, index);
	}
	return str;
}

static void ListScreen_UpdateTitle(struct ListScreen* s) {
	cc_string str; char strBuffer[STRING_SIZE];
	int num, pages;
	String_InitArray(str, strBuffer);
	String_AppendConst(&str, s->titleText);

	if (!Game_ClassicMode) {
		num   = (s->currentIndex / LIST_SCREEN_ITEMS) + 1;
		pages = Math_CeilDiv(s->entries.count, LIST_SCREEN_ITEMS);

		if (pages == 0) pages = 1;
		String_Format2(&str, " &7(page %i/%i)", &num, &pages);
	}
	TextWidget_Set(&s->title, &str, &s->font);
}

static void ListScreen_UpdatePage(struct ListScreen* s) {
	int end = s->entries.count - LIST_SCREEN_ITEMS;
	Widget_SetDisabled(&s->left,  s->currentIndex <= 0);
	Widget_SetDisabled(&s->right, s->currentIndex >= end);
	ListScreen_UpdateTitle(s);
}

static void ListScreen_UpdateEntry(struct ListScreen* s, struct ButtonWidget* button, const cc_string* text) {
	ButtonWidget_Set(button, text, &s->font);
}

static void ListScreen_RedrawEntries(struct ListScreen* s) {
	cc_string str;
	int i;
	for (i = 0; i < LIST_SCREEN_ITEMS; i++) {
		str = ListScreen_UNSAFE_Get(s, s->currentIndex + i);
		
		Widget_SetDisabled(&s->btns[i], String_CaselessEqualsConst(&str, LISTSCREEN_EMPTY));
		s->UpdateEntry(s, &s->btns[i], &str);
	}
}

static void ListScreen_SetCurrentIndex(struct ListScreen* s, int index) {
	if (index >= s->entries.count) { index = s->entries.count - 1; }
	if (index < 0) index = 0;

	s->currentIndex = index;
	ListScreen_RedrawEntries(s);
	ListScreen_UpdatePage(s);
}

static void ListScreen_PageClick(struct ListScreen* s, cc_bool forward) {
	int delta = forward ? LIST_SCREEN_ITEMS : -LIST_SCREEN_ITEMS;
	ListScreen_SetCurrentIndex(s, s->currentIndex + delta);
}

static void ListScreen_MoveBackwards(void* screen, void* b) {
	struct ListScreen* s = (struct ListScreen*)screen;
	ListScreen_PageClick(s, false);
}

static void ListScreen_MoveForwards(void* screen, void* b) {
	struct ListScreen* s = (struct ListScreen*)screen;
	ListScreen_PageClick(s, true);
}

static cc_string ListScreen_UNSAFE_GetCur(struct ListScreen* s, void* widget) {
	struct ButtonWidget* btn = (struct ButtonWidget*)widget;
	int i = btn->meta.val;
	return ListScreen_UNSAFE_Get(s, s->currentIndex + i);
}

static void ListScreen_Select(struct ListScreen* s, const cc_string* str) {
	cc_string entry;
	int i;

	for (i = 0; i < s->entries.count; i++) {
		entry = StringsBuffer_UNSAFE_Get(&s->entries, i);
		if (!String_CaselessEquals(&entry, str)) continue;

		s->currentIndex = i;
		return;
	}
}

static int ListScreen_KeyDown(void* screen, int key, struct InputDevice* device) {
	struct ListScreen* s = (struct ListScreen*)screen;

	if (key == device->leftButton         || key == device->pageUpButton) {
		ListScreen_PageClick(s, false);
	} else if (key == device->rightButton || key == device->pageDownButton) {
		ListScreen_PageClick(s, true);
	} else if (key == CCWHEEL_UP) {
		ListScreen_SetCurrentIndex(s, s->currentIndex - 1);
	} else if (key == CCWHEEL_DOWN) {
		ListScreen_SetCurrentIndex(s, s->currentIndex + 1);
	} else {
		Menu_InputDown(screen, key, device);
	}
	return true;
}

static void ListScreen_Init(void* screen) {
	struct ListScreen* s = (struct ListScreen*)screen;
	int i, width;
	s->widgets    = list_widgets;
	s->numWidgets = 0;
	s->maxWidgets = Array_Elems(list_widgets);
	s->currentIndex = 0;

	for (i = 0; i < LIST_SCREEN_ITEMS; i++) 
	{
		ButtonWidget_Add(s, &s->btns[i], 300, s->EntryClick);
		s->btns[i].meta.val = i;
	}
	width = Input_TouchMode ? 140 : 400;
	ButtonWidget_Add(s, &s->action, width, s->ActionClick);

	ButtonWidget_Add(s, &s->left,  40, ListScreen_MoveBackwards);
	ButtonWidget_Add(s, &s->right, 40, ListScreen_MoveForwards);
	TextWidget_Add(s,   &s->title);
	ButtonWidget_Add(s, &s->done,  width, s->DoneClick);

	s->maxVertices = Screen_CalcDefaultMaxVertices(screen);
	s->LoadEntries(s);
}

static void ListScreen_Render(void* screen, float delta) {
	Menu_RenderBounds();
	Screen_Render2Widgets(screen, delta);
}

static void ListScreen_Free(void* screen) {
	struct ListScreen* s = (struct ListScreen*)screen;
	StringsBuffer_Clear(&s->entries);
}

static void ListScreen_ContextLost(void* screen) {
	struct ListScreen* s = (struct ListScreen*)screen;
	Screen_ContextLost(screen);
	Font_Free(&s->font);
}

static void ListScreen_ContextRecreated(void* screen) {
	struct ListScreen* s = (struct ListScreen*)screen;
	Screen_UpdateVb(screen);
	Gui_MakeTitleFont(&s->font);
	ListScreen_RedrawEntries(s);

	ButtonWidget_SetConst(&s->left,  "<",   &s->font);
	ButtonWidget_SetConst(&s->right, ">",   &s->font);
	ButtonWidget_SetConst(&s->done, "Done", &s->font);
	ListScreen_UpdatePage(s);

	ButtonWidget_SetConst(&s->action, s->actionText, &s->font);
}

static void ListScreen_Reload(struct ListScreen* s) {
	ListScreen_Free(s);
	s->LoadEntries(s);
	ListScreen_SetCurrentIndex(s, s->currentIndex);
}

static const struct ScreenVTABLE ListScreen_VTABLE = {
	ListScreen_Init,    Screen_NullUpdate, ListScreen_Free,  
	ListScreen_Render,  Screen_BuildMesh,
	ListScreen_KeyDown, Screen_InputUp,    Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,   Screen_PointerUp,  Menu_PointerMove, Screen_TMouseScroll,
	ListScreen_Layout,  ListScreen_ContextLost,  ListScreen_ContextRecreated
};
void ListScreen_Show(void) {
	struct ListScreen* s = &ListScreen;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE     = &ListScreen_VTABLE;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_MENU);
}


/*########################################################################################################################*
*--------------------------------------------------------MenuScreen-------------------------------------------------------*
*#########################################################################################################################*/
void MenuScreen_Render2(void* screen, float delta) {
	Menu_RenderBounds();
	Screen_Render2Widgets(screen, delta);
}


/*########################################################################################################################*
*-----------------------------------------------------PauseScreenBase-----------------------------------------------------*
*#########################################################################################################################*/
#define PAUSE_MAX_BTNS 6
static struct PauseScreen {
	Screen_Body
	int descsCount;
	const struct SimpleButtonDesc* descs;
	struct ButtonWidget btns[PAUSE_MAX_BTNS], quit, back;
	struct TextWidget title;
} PauseScreen;

static void PauseScreenBase_Quit(void* a, void* b) { 
	Window_RequestClose(); 
}

static void PauseScreenBase_Game(void* a, void* b) { 
	Gui_Remove((struct Screen*)&PauseScreen); 
}

static void PauseScreenBase_ContextRecreated(struct PauseScreen* s, struct FontDesc* titleFont) {
	Screen_UpdateVb(s);
	Gui_MakeTitleFont(titleFont);
	Menu_SetButtons(s->btns, titleFont, s->descs, s->descsCount);
	ButtonWidget_SetConst(&s->back, "Back to game", titleFont);
	TextWidget_SetConst(&s->title,  "Game menu", titleFont);
}

static void PauseScreenBase_AddWidgets(struct PauseScreen* s, int width) {
	TextWidget_Add(s,   &s->title);
	Menu_AddButtons(s,  s->btns, width, s->descs, s->descsCount);
	ButtonWidget_Add(s, &s->back, 400, PauseScreenBase_Game);
}


/*########################################################################################################################*
*-------------------------------------------------------PauseScreen-------------------------------------------------------*
*#########################################################################################################################*/
static struct Widget* pause_widgets[1 + 6 + 2];

static void PauseScreen_CheckHacksAllowed(void* screen) {
	struct PauseScreen* s = (struct PauseScreen*)screen;
	if (Gui.ClassicMenu) return;

	Widget_SetDisabled(&s->btns[1],
			!Entities.CurPlayer->Hacks.CanAnyHacks); /* select texture pack */
	s->dirty = true;
}

static void PauseScreen_ContextRecreated(void* screen) {
	struct PauseScreen* s = (struct PauseScreen*)screen;
	struct FontDesc titleFont;
	PauseScreenBase_ContextRecreated(s, &titleFont);

	ButtonWidget_SetConst(&s->quit, "Quit game", &titleFont);
	PauseScreen_CheckHacksAllowed(s);
	Font_Free(&titleFont);
}

static void PauseScreen_Layout(void* screen) {
	struct PauseScreen* s = (struct PauseScreen*)screen;
	Menu_LayoutButtons(s->btns, s->descs, s->descsCount);
	Menu_LayoutBack(&s->back);
	Widget_SetLocation(&s->quit,  ANCHOR_MAX,    ANCHOR_MAX,    5,    5);
	Widget_SetLocation(&s->title, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -100);
}

static void PauseScreen_Init(void* screen) {
	struct PauseScreen* s = (struct PauseScreen*)screen;
	static const struct SimpleButtonDesc descs[] = {
		{ -160,  -50, "Options...",             Menu_SwitchOptions   },
		{ -160,    0, "Change texture pack...", Menu_SwitchTexPacks  },
		{ -160,   50, "Hotkeys...",             Menu_SwitchHotkeys   },
		{  160,  -50, "Generate new level...",  Menu_SwitchGenLevel  },
		{  160,    0, "Load level...",          Menu_SwitchLoadLevel },
		{  160,   50, "Save level...",          Menu_SwitchSaveLevel }
	};
	s->widgets     = pause_widgets;
	s->numWidgets  = 0;
	s->maxWidgets  = Array_Elems(pause_widgets);
	Event_Register_(&UserEvents.HackPermsChanged, s, PauseScreen_CheckHacksAllowed);

	s->descs      = descs;
	s->descsCount = Array_Elems(descs);
	PauseScreenBase_AddWidgets(s, 300);
	ButtonWidget_Add(s, &s->quit, 120, PauseScreenBase_Quit);
	s->maxVertices = Screen_CalcDefaultMaxVertices(s);

	if (Server.IsSinglePlayer) return;
	s->btns[3].flags = WIDGET_FLAG_DISABLED;
	s->btns[4].flags = WIDGET_FLAG_DISABLED;
}

static void PauseScreen_Free(void* screen) {
	Event_Unregister_(&UserEvents.HackPermsChanged, screen, PauseScreen_CheckHacksAllowed);
}

static const struct ScreenVTABLE PauseScreen_VTABLE = {
	PauseScreen_Init,   Screen_NullUpdate, PauseScreen_Free, 
	MenuScreen_Render2, Screen_BuildMesh,
	Menu_InputDown,     Screen_InputUp,    Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,   Screen_PointerUp,  Menu_PointerMove, Screen_TMouseScroll,
	PauseScreen_Layout, Screen_ContextLost, PauseScreen_ContextRecreated
};
void PauseScreen_Show(void) {
	struct PauseScreen* s = &PauseScreen;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE     = &PauseScreen_VTABLE;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_MENU);
}


/*########################################################################################################################*
*----------------------------------------------------ClassicPauseScreen---------------------------------------------------*
*#########################################################################################################################*/
static struct Widget* classicPause_widgets[1 + 5 + 1];

static void ClassicPauseScreen_ContextRecreated(void* screen) {
	struct PauseScreen* s = (struct PauseScreen*)screen;
	struct FontDesc titleFont;
	PauseScreenBase_ContextRecreated(s, &titleFont);
	Font_Free(&titleFont);
}

static void ClassicPauseScreen_Layout(void* screen) {
	struct PauseScreen* s = (struct PauseScreen*)screen;
	Menu_LayoutButtons(s->btns, s->descs, s->descsCount);
	Widget_SetLocation(&s->back,  ANCHOR_CENTRE, ANCHOR_MAX, 0, Game_ClassicMode ? 80 : 25);
	Widget_SetLocation(&s->title, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -150);
}

static void ClassicPauseScreen_Init(void* screen) {
	struct PauseScreen* s = (struct PauseScreen*)screen;
	static const struct SimpleButtonDesc descs[] = {
		{    0, -100, "Options...",             Menu_SwitchClassicOptions },
		{    0,  -50, "Generate new level...",  Menu_SwitchClassicGenLevel },
		{    0,    0, "Save level..",           Menu_SwitchSaveLevel },
		{    0,   50, "Load level..",           Menu_SwitchLoadLevel },
		{    0,  100, "Nostalgia options...",   Menu_SwitchNostalgia }
	};
	s->widgets    = classicPause_widgets;
	s->numWidgets = 0;
	s->maxWidgets = Array_Elems(classicPause_widgets);
	s->descs      = descs;

	/* Don't show nostalgia options in classic mode */
	s->descsCount = Game_ClassicMode ? 4 : 5;
	PauseScreenBase_AddWidgets(s, 400);
	s->maxVertices = Screen_CalcDefaultMaxVertices(s);

	if (Server.IsSinglePlayer) return;
	s->btns[1].flags = WIDGET_FLAG_DISABLED;
	s->btns[3].flags = WIDGET_FLAG_DISABLED;

	if (Game_PureClassic) s->btns[2].flags = WIDGET_FLAG_DISABLED;
}

static const struct ScreenVTABLE ClassicPauseScreen_VTABLE = {
	ClassicPauseScreen_Init,   Screen_NullUpdate, Screen_NullFunc, 
	MenuScreen_Render2,        Screen_BuildMesh,
	Menu_InputDown,            Screen_InputUp,    Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,          Screen_PointerUp,  Menu_PointerMove, Screen_TMouseScroll,
	ClassicPauseScreen_Layout, Screen_ContextLost, ClassicPauseScreen_ContextRecreated
};
void ClassicPauseScreen_Show(void) {
	struct PauseScreen* s = &PauseScreen;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE     = &ClassicPauseScreen_VTABLE;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_MENU);
}


/*########################################################################################################################*
*--------------------------------------------------OptionsGroupScreen-----------------------------------------------------*
*#########################################################################################################################*/
static struct OptionsGroupScreen {
	Screen_Body
	struct FontDesc textFont;
	struct ButtonWidget btns[8];
	struct TextWidget desc;
	struct ButtonWidget done;
} OptionsGroupScreen;

static struct Widget* optGroups_widgets[8 + 2];

static const char* const optsGroup_descs[8] = {
	"&eMusic/Sound, view bobbing, and more",
	"&eGui scale, font settings, and more",
	"&eFPS limit, view distance, entity names/shadows",
	"&eSet key and mouse bindings",
	"&eChat options",
	"&eHacks allowed, jump settings, and more",
	"&eEnv colours, water level, weather, and more",
	"&eSettings for resembling the original classic",
};
static const struct SimpleButtonDesc optsGroup_btns[8] = {
	{ -160, -100, "Misc options...",      Menu_SwitchMisc        },
	{ -160,  -50, "Gui options...",       Menu_SwitchGui         },
	{ -160,    0, "Graphics options...",  Menu_SwitchGfx         },
	{ -160,   50, "Controls...",          SwitchBindsMain        },
	{  160, -100, "Chat options...",      Menu_SwitchChat        },
	{  160,  -50, "Hacks settings...",    Menu_SwitchHacks       },
	{  160,    0, "Env settings...",      Menu_SwitchEnv         },
	{  160,   50, "Nostalgia options...", Menu_SwitchNostalgia   }
};

static void OptionsGroupScreen_CheckHacksAllowed(void* screen) {
	struct OptionsGroupScreen* s = (struct OptionsGroupScreen*)screen;
	Widget_SetDisabled(&s->btns[6],
			!Entities.CurPlayer->Hacks.CanAnyHacks); /* env settings */
	s->dirty = true;
}

CC_NOINLINE static void OptionsGroupScreen_UpdateDesc(struct OptionsGroupScreen* s) {
	TextWidget_SetConst(&s->desc, optsGroup_descs[s->selectedI], &s->textFont);
}

static void OptionsGroupScreen_ContextLost(void* screen) {
	struct OptionsGroupScreen* s = (struct OptionsGroupScreen*)screen;
	Font_Free(&s->textFont);
	Screen_ContextLost(screen);
}

static void OptionsGroupScreen_ContextRecreated(void* screen) {
	struct OptionsGroupScreen* s = (struct OptionsGroupScreen*)screen;
	struct FontDesc titleFont;
	Screen_UpdateVb(screen);

	Gui_MakeTitleFont(&titleFont);
	Gui_MakeBodyFont(&s->textFont);

	Menu_SetButtons(s->btns, &titleFont, optsGroup_btns, 8);
	ButtonWidget_SetConst(&s->done, "Done", &titleFont);

	if (s->selectedI >= 0) OptionsGroupScreen_UpdateDesc(s);
	OptionsGroupScreen_CheckHacksAllowed(s);
	Font_Free(&titleFont);
}

static void OptionsGroupScreen_Layout(void* screen) {
	struct OptionsGroupScreen* s = (struct OptionsGroupScreen*)screen;
	Menu_LayoutButtons(s->btns, optsGroup_btns, 8);
	Widget_SetLocation(&s->desc, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 100);
	Menu_LayoutBack(&s->done);
}

static void OptionsGroupScreen_Init(void* screen) {
	struct OptionsGroupScreen* s = (struct OptionsGroupScreen*)screen;

	Event_Register_(&UserEvents.HackPermsChanged, s, OptionsGroupScreen_CheckHacksAllowed);
	s->widgets     = optGroups_widgets;
	s->numWidgets  = 0;
	s->maxWidgets  = Array_Elems(optGroups_widgets);
	s->selectedI   = -1;

	Menu_AddButtons(s,  s->btns, 300, optsGroup_btns, 8);
	TextWidget_Add(s,   &s->desc);
	ButtonWidget_Add(s, &s->done, 400, Menu_SwitchPause);

	s->maxVertices = Screen_CalcDefaultMaxVertices(s);
}

static void OptionsGroupScreen_Free(void* screen) {
	struct OptionsGroupScreen* s = (struct OptionsGroupScreen*)screen;
	Event_Unregister_(&UserEvents.HackPermsChanged, s, OptionsGroupScreen_CheckHacksAllowed);
}

static int OptionsGroupScreen_PointerMove(void* screen, int id, int x, int y) {
	struct OptionsGroupScreen* s = (struct OptionsGroupScreen*)screen;
	int i = Menu_DoPointerMove(s, id, x, y);
	if (i == -1 || i == s->selectedI) return true;
	if (i >= Array_Elems(optsGroup_descs)) return true;

	s->selectedI = i;
	OptionsGroupScreen_UpdateDesc(s);
	return true;
}

static const struct ScreenVTABLE OptionsGroupScreen_VTABLE = {
	OptionsGroupScreen_Init,   Screen_NullUpdate, OptionsGroupScreen_Free,
	MenuScreen_Render2,        Screen_BuildMesh,
	Menu_InputDown,            Screen_InputUp,    Screen_TKeyPress,               Screen_TText,
	Menu_PointerDown,          Screen_PointerUp,  OptionsGroupScreen_PointerMove, Screen_TMouseScroll,
	OptionsGroupScreen_Layout, OptionsGroupScreen_ContextLost, OptionsGroupScreen_ContextRecreated
};
void OptionsGroupScreen_Show(void) {
	struct OptionsGroupScreen* s = &OptionsGroupScreen;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE     = &OptionsGroupScreen_VTABLE;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_MENU);
}


/*########################################################################################################################*
*----------------------------------------------------EditHotkeyScreen-----------------------------------------------------*
*#########################################################################################################################*/
static struct EditHotkeyScreen {
	Screen_Body
	struct HotkeyData curHotkey, origHotkey;
	cc_bool supressNextPress;
	int barX, barY[2], barWidth, barHeight;
	struct FontDesc titleFont, textFont;
	struct TextInputWidget input;
	struct ButtonWidget btns[5], cancel;
} EditHotkeyScreen;

static struct Widget* edithotkey_widgets[1 + 5 + 1];

static void HotkeyListScreen_MakeFlags(int flags, cc_string* str);
static void EditHotkeyScreen_MakeFlags(int flags, cc_string* str) {
	if (flags == 0) String_AppendConst(str, " None");
	HotkeyListScreen_MakeFlags(flags, str);
}

static void EditHotkeyScreen_UpdateBaseKey(struct EditHotkeyScreen* s) {
	cc_string text; char textBuffer[STRING_SIZE];
	String_InitArray(text, textBuffer);

	if (s->selectedI == 0) {
		String_AppendConst(&text, "Key: press a key..");
	} else {
		String_AppendConst(&text, "Key: ");
		String_AppendConst(&text, Input_DisplayNames[s->curHotkey.trigger]);
	}
	ButtonWidget_Set(&s->btns[0], &text, &s->titleFont);
}

static void EditHotkeyScreen_UpdateModifiers(struct EditHotkeyScreen* s) {
	cc_string text; char textBuffer[STRING_SIZE];
	String_InitArray(text, textBuffer);

	if (s->selectedI == 1) {
		String_AppendConst(&text, "Modifiers: press a key..");
	} else {
		String_AppendConst(&text, "Modifiers:");
		EditHotkeyScreen_MakeFlags(s->curHotkey.mods, &text);
	}
	ButtonWidget_Set(&s->btns[1], &text, &s->titleFont);
}

static void EditHotkeyScreen_UpdateLeaveOpen(struct EditHotkeyScreen* s) {
	cc_string text; char textBuffer[STRING_SIZE];
	String_InitArray(text, textBuffer);

	String_AppendConst(&text, "Input stays open: ");
	String_AppendConst(&text, 
		(s->curHotkey.flags & HOTKEY_FLAG_STAYS_OPEN) ? "ON" : "OFF");
	ButtonWidget_Set(&s->btns[2], &text, &s->titleFont);
}

static void EditHotkeyScreen_BaseKey(void* screen, void* b) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	s->selectedI        = 0;
	s->supressNextPress = true;
	EditHotkeyScreen_UpdateBaseKey(s);
}

static void EditHotkeyScreen_Modifiers(void* screen, void* b) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	s->selectedI        = 1;
	s->supressNextPress = true;
	EditHotkeyScreen_UpdateModifiers(s);
}

static void EditHotkeyScreen_LeaveOpen(void* screen, void* b) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	/* Reset 'waiting for key..' state of two other buttons */
	if (s->selectedI >= 0) {
		s->selectedI        = -1;
		s->supressNextPress = false;
		EditHotkeyScreen_UpdateBaseKey(s);
		EditHotkeyScreen_UpdateModifiers(s);
	}
	
	/* Toggle Input Stays Open flag */
	s->curHotkey.flags ^= HOTKEY_FLAG_STAYS_OPEN;
	EditHotkeyScreen_UpdateLeaveOpen(s);
}

static void EditHotkeyScreen_SaveChanges(void* screen, void* b) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	struct HotkeyData hk = s->origHotkey;

	if (hk.trigger) {
		Hotkeys_Remove(hk.trigger, hk.mods);
		StoredHotkeys_Remove(hk.trigger, hk.mods);
	}
	hk = s->curHotkey;

	if (hk.trigger) {
		cc_string text    = s->input.base.text;
		cc_bool staysOpen = hk.flags & HOTKEY_FLAG_STAYS_OPEN;

		Hotkeys_Add(hk.trigger, hk.mods, &text, hk.flags);
		StoredHotkeys_Add(hk.trigger, hk.mods, staysOpen, &text);
	}
	HotkeyListScreen_Show();
}

static void EditHotkeyScreen_RemoveHotkey(void* screen, void* b) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	struct HotkeyData hk = s->origHotkey;

	if (hk.trigger) {
		Hotkeys_Remove(hk.trigger, hk.mods);
		StoredHotkeys_Remove(hk.trigger, hk.mods);
	}
	HotkeyListScreen_Show();
}

static void EditHotkeyScreen_Render(void* screen, float delta) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	PackedCol grey = PackedCol_Make(150, 150, 150, 255);

	MenuScreen_Render2(screen, delta);
	Gfx_Draw2DFlat(s->barX, s->barY[0], s->barWidth, s->barHeight, grey);
	Gfx_Draw2DFlat(s->barX, s->barY[1], s->barWidth, s->barHeight, grey);
}

static int EditHotkeyScreen_KeyPress(void* screen, char keyChar) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	if (s->supressNextPress) {
		s->supressNextPress = false;
	} else {
		InputWidget_Append(&s->input.base, keyChar);
	}
	return true;
}

static int EditHotkeyScreen_TextChanged(void* screen, const cc_string* str) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	InputWidget_SetText(&s->input.base, str);
	return true;
}

static int EditHotkeyScreen_KeyDown(void* screen, int key, struct InputDevice* device) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	if (s->selectedI >= 0) {
		if (s->selectedI == 0) {
			s->curHotkey.trigger = key;
		} else if (s->selectedI == 1) {
			if      (key == CCKEY_LCTRL  || key == CCKEY_RCTRL)  s->curHotkey.mods |= HOTKEY_MOD_CTRL;
			else if (key == CCKEY_LSHIFT || key == CCKEY_RSHIFT) s->curHotkey.mods |= HOTKEY_MOD_SHIFT;
			else if (key == CCKEY_LALT   || key == CCKEY_RALT)   s->curHotkey.mods |= HOTKEY_MOD_ALT;
			else s->curHotkey.mods = 0;
		}

		s->supressNextPress = true;
		s->selectedI        = -1;

		EditHotkeyScreen_UpdateBaseKey(s);
		EditHotkeyScreen_UpdateModifiers(s);
		return true;
	}
	return Elem_HandlesKeyDown(&s->input.base, key, device) || Screen_InputDown(s, key, device);
}

static void EditHotkeyScreen_ContextLost(void* screen) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	Font_Free(&s->titleFont);
	Font_Free(&s->textFont);
	Screen_ContextLost(screen);
}

static void EditHotkeyScreen_ContextRecreated(void* screen) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	cc_bool existed = s->origHotkey.trigger != INPUT_NONE;

	Gui_MakeTitleFont(&s->titleFont);
	Gui_MakeBodyFont(&s->textFont);
	Screen_UpdateVb(screen);

	EditHotkeyScreen_UpdateBaseKey(s);
	EditHotkeyScreen_UpdateModifiers(s);
	EditHotkeyScreen_UpdateLeaveOpen(s);

	ButtonWidget_SetConst(&s->btns[3], existed ? "Save changes" : "Add hotkey", &s->titleFont);
	ButtonWidget_SetConst(&s->btns[4], existed ? "Remove hotkey" : "Cancel",    &s->titleFont);
	TextInputWidget_SetFont(&s->input, &s->textFont);
	ButtonWidget_SetConst(&s->cancel, "Cancel", &s->titleFont);
}

static void EditHotkeyScreen_Update(void* screen, float delta) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	s->input.base.caretAccumulator += delta;
}

static void EditHotkeyScreen_Layout(void* screen) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	s->barWidth  = Display_ScaleX(500);
	s->barX      = Gui_CalcPos(ANCHOR_CENTRE, 0, s->barWidth, Window_UI.Width);
	s->barHeight = Display_ScaleY(2);

	s->barY[0] = Gui_CalcPos(ANCHOR_CENTRE, Display_ScaleY(-65), 
					s->barHeight, Window_UI.Height);
	s->barY[1] = Gui_CalcPos(ANCHOR_CENTRE, Display_ScaleY( 45), 
					s->barHeight, Window_UI.Height);

	Widget_SetLocation(&s->btns[0], ANCHOR_CENTRE, ANCHOR_CENTRE,    0, -150);
	Widget_SetLocation(&s->btns[1], ANCHOR_CENTRE, ANCHOR_CENTRE,    0, -100);
	Widget_SetLocation(&s->btns[2], ANCHOR_CENTRE, ANCHOR_CENTRE, -100,   10);
	Widget_SetLocation(&s->btns[3], ANCHOR_CENTRE, ANCHOR_CENTRE,    0,   80);
	Widget_SetLocation(&s->btns[4], ANCHOR_CENTRE, ANCHOR_CENTRE,    0,  130);
	Widget_SetLocation(&s->input,   ANCHOR_CENTRE, ANCHOR_CENTRE,    0,  -35);
	Menu_LayoutBack(&s->cancel);
}

static void EditHotkeyScreen_Init(void* screen) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	struct MenuInputDesc desc;
	cc_string text;

	s->widgets     = edithotkey_widgets;
	s->numWidgets  = 0;
	s->maxWidgets  = Array_Elems(edithotkey_widgets);
	
	s->selectedI   = -1;
	MenuInput_String(desc);

	ButtonWidget_Add(s, &s->btns[0], 300, EditHotkeyScreen_BaseKey);
	ButtonWidget_Add(s, &s->btns[1], 300, EditHotkeyScreen_Modifiers);
	ButtonWidget_Add(s, &s->btns[2], 300, EditHotkeyScreen_LeaveOpen);
	ButtonWidget_Add(s, &s->btns[3], 300, EditHotkeyScreen_SaveChanges);
	ButtonWidget_Add(s, &s->btns[4], 300, EditHotkeyScreen_RemoveHotkey);

	if (s->origHotkey.trigger) {
		text = StringsBuffer_UNSAFE_Get(&HotkeysText, s->origHotkey.textIndex);
	} else { text = String_Empty; }

	TextInputWidget_Add(s, &s->input, 500, &text, &desc);
	ButtonWidget_Add(s,    &s->cancel, 400, Menu_SwitchHotkeys);
	s->input.onscreenPlaceholder = "Hotkey text";

	s->maxVertices = Screen_CalcDefaultMaxVertices(s);
}

static const struct ScreenVTABLE EditHotkeyScreen_VTABLE = {
	EditHotkeyScreen_Init,    EditHotkeyScreen_Update, Menu_CloseKeyboard,
	EditHotkeyScreen_Render,  Screen_BuildMesh,
	EditHotkeyScreen_KeyDown, Screen_InputUp,    EditHotkeyScreen_KeyPress, EditHotkeyScreen_TextChanged,
	Menu_PointerDown,         Screen_PointerUp,  Menu_PointerMove,          Screen_TMouseScroll,
	EditHotkeyScreen_Layout,  EditHotkeyScreen_ContextLost, EditHotkeyScreen_ContextRecreated
};
void EditHotkeyScreen_Show(struct HotkeyData original) {
	struct EditHotkeyScreen* s = &EditHotkeyScreen;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE     = &EditHotkeyScreen_VTABLE;
	s->origHotkey = original;
	s->curHotkey  = original;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_MENU);
}


/*########################################################################################################################*
*-----------------------------------------------------GenLevelScreen------------------------------------------------------*
*#########################################################################################################################*/
static struct GenLevelScreen {
	Screen_Body
	struct FontDesc textFont;
	struct ButtonWidget flatgrass, vanilla, cancel;
	struct TextInputWidget inputs[4];
	struct TextWidget labels[4], title;
} GenLevelScreen;
#define GENLEVEL_NUM_INPUTS 4

static struct Widget* gen_widgets[2 * GENLEVEL_NUM_INPUTS + 4];

CC_NOINLINE static int GenLevelScreen_GetInt(struct GenLevelScreen* s, int index) {
	struct TextInputWidget* input = &s->inputs[index];
	struct MenuInputDesc* desc;
	cc_string text = input->base.text;
	int value;

	desc = &input->desc;
	if (!desc->VTABLE->IsValidValue(desc, &text)) return 0;
	Convert_ParseInt(&text, &value); return value;
}

CC_NOINLINE static int GenLevelScreen_GetSeedInt(struct GenLevelScreen* s, int index) {
	struct TextInputWidget* input = &s->inputs[index];
	RNGState rnd;

	if (!input->base.text.length) {
		Random_SeedFromCurrentTime(&rnd);
		return Random_Next(&rnd, Int32_MaxValue);
	}
	return GenLevelScreen_GetInt(s, index);
}

static void GenLevelScreen_Gen(void* screen, const struct MapGenerator* gen) {
	struct GenLevelScreen* s = (struct GenLevelScreen*)screen;
	int width  = GenLevelScreen_GetInt(s, 0);
	int height = GenLevelScreen_GetInt(s, 1);
	int length = GenLevelScreen_GetInt(s, 2);
	int seed   = GenLevelScreen_GetSeedInt(s, 3);

	if (!World_CheckVolume(width, height, length)) {
		Chat_AddRaw("&cThe generated map's volume is too big.");
	} else if (!width || !height || !length) {
		Chat_AddRaw("&cOne of the map dimensions is invalid.");
	} else {
		Gen_Active  = gen;
		Gen_Seed    = seed;
		Gui_Remove((struct Screen*)s);
		Menu_BeginGen(width, height, length);
	}
}

static void GenLevelScreen_Flatgrass(void* a, void* b) { GenLevelScreen_Gen(a, &FlatgrassGen); }
static void GenLevelScreen_Notchy(void* a, void* b)    { GenLevelScreen_Gen(a, &NotchyGen);    }

static void GenLevelScreen_Make(struct GenLevelScreen* s, int i, int def) {
	cc_string tmp; char tmpBuffer[STRING_SIZE];
	struct MenuInputDesc desc;

	if (i == 3) {
		MenuInput_Seed(desc);
	} else {
		MenuInput_Int(desc, 1, 8192, def);
	}

	String_InitArray(tmp, tmpBuffer);
	desc.VTABLE->GetDefault(&desc, &tmp);

	TextWidget_Add(s, &s->labels[i]);
	s->labels[i].color = PackedCol_Make(224, 224, 224, 255);
	
	/* TODO placeholder */
	TextInputWidget_Add(s, &s->inputs[i], 200, &tmp, &desc);
	s->inputs[i].base.showCaret = false;
	s->inputs[i].onscreenType = KEYBOARD_TYPE_INTEGER;
	s->inputs[i].base.meta.val = 10000;
}
#define GenLevelScreen_IsInput(w) (w)->meta.val == 10000

static struct TextInputWidget* GenLevelScreen_SelectedInput(struct GenLevelScreen* s) {
	if (s->selectedI >= 0 && GenLevelScreen_IsInput(s->widgets[s->selectedI])) {
		return (struct TextInputWidget*)s->widgets[s->selectedI];
	}
	return NULL;
}

static int GenLevelScreen_KeyPress(void* screen, char keyChar) {
	struct GenLevelScreen* s = (struct GenLevelScreen*)screen;
	struct TextInputWidget* selected = GenLevelScreen_SelectedInput(s);

	if (selected) InputWidget_Append(&selected->base, keyChar);
	return true;
}

static int GenLevelScreen_TextChanged(void* screen, const cc_string* str) {
	struct GenLevelScreen* s = (struct GenLevelScreen*)screen;
	struct TextInputWidget* selected = GenLevelScreen_SelectedInput(s);

	if (selected) InputWidget_SetText(&selected->base, str);
	return true;
}

static int GenLevelScreen_PointerDown(void* screen, int id, int x, int y) {
	struct GenLevelScreen* s = (struct GenLevelScreen*)screen;
	struct TextInputWidget* selected;
	s->selectedI = Screen_DoPointerDown(screen, id, x, y);

	selected = GenLevelScreen_SelectedInput(s);
	if (selected) OnscreenKeyboard_SetText(&selected->base.text);
	return TOUCH_TYPE_GUI;
}

static void GenLevelScreen_ContextLost(void* screen) {
	struct GenLevelScreen* s = (struct GenLevelScreen*)screen;
	Font_Free(&s->textFont);
	Screen_ContextLost(screen);
}

static void GenLevelScreen_ContextRecreated(void* screen) {
	struct GenLevelScreen* s  = (struct GenLevelScreen*)screen;
	struct FontDesc titleFont;
	Gui_MakeTitleFont(&titleFont);
	Gui_MakeBodyFont(&s->textFont);
	Screen_UpdateVb(screen);

	TextInputWidget_SetFont(&s->inputs[0], &s->textFont);
	TextInputWidget_SetFont(&s->inputs[1], &s->textFont);
	TextInputWidget_SetFont(&s->inputs[2], &s->textFont);
	TextInputWidget_SetFont(&s->inputs[3], &s->textFont);

	TextWidget_SetConst(&s->labels[0], "Width:",  &s->textFont);
	TextWidget_SetConst(&s->labels[1], "Height:", &s->textFont);
	TextWidget_SetConst(&s->labels[2], "Length:", &s->textFont);
	TextWidget_SetConst(&s->labels[3], "Seed:",   &s->textFont);
	
	TextWidget_SetConst(&s->title,       "Generate new level", &s->textFont);
	ButtonWidget_SetConst(&s->flatgrass, "Flatgrass",          &titleFont);
	ButtonWidget_SetConst(&s->vanilla,   "Vanilla",            &titleFont);
	ButtonWidget_SetConst(&s->cancel,    "Cancel",             &titleFont);
	Font_Free(&titleFont);
}

static void GenLevelScreen_Update(void* screen, float delta) {
	struct GenLevelScreen* s = (struct GenLevelScreen*)screen;
	struct TextInputWidget* selected = GenLevelScreen_SelectedInput(s);
	int i;
	
	for (i = 0; i < GENLEVEL_NUM_INPUTS; i++)
	{
		s->inputs[i].base.showCaret = &s->inputs[i] == selected;
	}
	if (selected) selected->base.caretAccumulator += delta;
}

static void GenLevelScreen_Layout(void* screen) {
	struct GenLevelScreen* s = (struct GenLevelScreen*)screen;
	int i, y;
	for (i = 0; i < 4; i++) {
		y = (i - 2) * 40;
		Widget_SetLocation(&s->inputs[i], ANCHOR_CENTRE,     ANCHOR_CENTRE,   0, y);
		Widget_SetLocation(&s->labels[i], ANCHOR_CENTRE_MAX, ANCHOR_CENTRE, 110, y);
	}

	Widget_SetLocation(&s->title,     ANCHOR_CENTRE, ANCHOR_CENTRE,    0, -130);
	Widget_SetLocation(&s->flatgrass, ANCHOR_CENTRE, ANCHOR_CENTRE, -120,  100);
	Widget_SetLocation(&s->vanilla,   ANCHOR_CENTRE, ANCHOR_CENTRE,  120,  100);
	Menu_LayoutBack(&s->cancel);
}

static void GenLevelScreen_Init(void* screen) {
	struct GenLevelScreen* s = (struct GenLevelScreen*)screen;
	s->widgets    = gen_widgets;
	s->numWidgets = 0;
	s->maxWidgets = Array_Elems(gen_widgets);
	s->selectedI  = -1;

	GenLevelScreen_Make(s, 0, World.Width);
	GenLevelScreen_Make(s, 1, World.Height);
	GenLevelScreen_Make(s, 2, World.Length);
	GenLevelScreen_Make(s, 3, 0);

	TextWidget_Add(s,   &s->title);
	ButtonWidget_Add(s, &s->flatgrass, 200, GenLevelScreen_Flatgrass);
	ButtonWidget_Add(s, &s->vanilla,   200, GenLevelScreen_Notchy);
	ButtonWidget_Add(s, &s->cancel,    400, Menu_SwitchPause);

	s->maxVertices = Screen_CalcDefaultMaxVertices(s);
}

static const struct ScreenVTABLE GenLevelScreen_VTABLE = {
	GenLevelScreen_Init,        GenLevelScreen_Update, Menu_CloseKeyboard,
	MenuScreen_Render2,         Screen_BuildMesh,
	Menu_InputDown,             Screen_InputUp,    GenLevelScreen_KeyPress, GenLevelScreen_TextChanged,
	GenLevelScreen_PointerDown, Screen_PointerUp,  Menu_PointerMove,        Screen_TMouseScroll,
	GenLevelScreen_Layout,      GenLevelScreen_ContextLost, GenLevelScreen_ContextRecreated
};
void GenLevelScreen_Show(void) {	
	struct GenLevelScreen* s = &GenLevelScreen;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE     = &GenLevelScreen_VTABLE;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_MENU);
}


/*########################################################################################################################*
*----------------------------------------------------ClassicGenScreen-----------------------------------------------------*
*#########################################################################################################################*/
static struct ClassicGenScreen {
	Screen_Body
	struct ButtonWidget btns[3], cancel;
	struct TextWidget title;
} ClassicGenScreen;

static struct Widget* classicgen_widgets[1 + 3 + 1];

static void ClassicGenScreen_Gen(int size) {
	RNGState rnd; Random_SeedFromCurrentTime(&rnd);
	Gen_Active = &NotchyGen;
	Gen_Seed   = Random_Next(&rnd, Int32_MaxValue);

	Gui_Remove((struct Screen*)&ClassicGenScreen);
	Menu_BeginGen(size, 64, size);
}

static void ClassicGenScreen_Small(void* a, void* b)  { ClassicGenScreen_Gen(128); }
static void ClassicGenScreen_Medium(void* a, void* b) { ClassicGenScreen_Gen(256); }
static void ClassicGenScreen_Huge(void* a, void* b)   { ClassicGenScreen_Gen(512); }

static void ClassicGenScreen_ContextRecreated(void* screen) {
	struct ClassicGenScreen* s = (struct ClassicGenScreen*)screen;
	struct FontDesc titleFont;

	Screen_UpdateVb(screen);
	Gui_MakeTitleFont(&titleFont);
	TextWidget_SetConst(&s->title, "Generate new level", &titleFont);

	ButtonWidget_SetConst(&s->btns[0], "Small",  &titleFont);
	ButtonWidget_SetConst(&s->btns[1], "Normal", &titleFont);
	ButtonWidget_SetConst(&s->btns[2], "Huge",   &titleFont);
	ButtonWidget_SetConst(&s->cancel,  "Cancel", &titleFont);
	Font_Free(&titleFont);
}

static void ClassicGenScreen_Layout(void* screen) {
	struct ClassicGenScreen* s = (struct ClassicGenScreen*)screen;
	Widget_SetLocation(&s->title,   ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -150);
	Widget_SetLocation(&s->btns[0], ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -100);
	Widget_SetLocation(&s->btns[1], ANCHOR_CENTRE, ANCHOR_CENTRE, 0,  -50);
	Widget_SetLocation(&s->btns[2], ANCHOR_CENTRE, ANCHOR_CENTRE, 0,    0);
	Widget_SetLocation(&s->cancel,  ANCHOR_CENTRE, ANCHOR_MAX,    0,   80);
}

static void ClassicGenScreen_Init(void* screen) {
	struct ClassicGenScreen* s = (struct ClassicGenScreen*)screen;
	s->widgets     = classicgen_widgets;
	s->numWidgets  = 0;
	s->maxWidgets  = Array_Elems(classicgen_widgets);

	TextWidget_Add(s,   &s->title);
	ButtonWidget_Add(s, &s->btns[0], 400, ClassicGenScreen_Small);
	ButtonWidget_Add(s, &s->btns[1], 400, ClassicGenScreen_Medium);
	ButtonWidget_Add(s, &s->btns[2], 400, ClassicGenScreen_Huge);
	ButtonWidget_Add(s, &s->cancel,  400, Menu_SwitchPause);

	s->maxVertices = Screen_CalcDefaultMaxVertices(s);
}

static const struct ScreenVTABLE ClassicGenScreen_VTABLE = {
	ClassicGenScreen_Init,   Screen_NullUpdate,  Screen_NullFunc,
	MenuScreen_Render2,      Screen_BuildMesh,
	Menu_InputDown,          Screen_InputUp,     Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,        Screen_PointerUp,   Menu_PointerMove, Screen_TMouseScroll,
	ClassicGenScreen_Layout, Screen_ContextLost, ClassicGenScreen_ContextRecreated
};
void ClassicGenScreen_Show(void) {
	struct ClassicGenScreen* s = &ClassicGenScreen;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE     = &ClassicGenScreen_VTABLE;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_MENU);
}


/*########################################################################################################################*
*----------------------------------------------------SaveLevelScreen------------------------------------------------------*
*#########################################################################################################################*/
static struct SaveLevelScreen {
	Screen_Body
	struct FontDesc titleFont, textFont;
	struct ButtonWidget save, file, cancel;
	struct TextInputWidget input;
	struct TextWidget desc;
} SaveLevelScreen;

static struct Widget* save_widgets[3 + 1 + 1];

static void SaveLevelScreen_UpdateSave(struct SaveLevelScreen* s) {
	ButtonWidget_SetConst(&s->save, 
		s->save.optName ? "&cOverwrite existing?" : "Save", &s->titleFont);
}

static void SaveLevelScreen_RemoveOverwrites(struct SaveLevelScreen* s) {
	if (s->save.optName) {
		s->save.optName = NULL;
		SaveLevelScreen_UpdateSave(s);
	}
}

static cc_result DoSaveMap(const cc_string* path, struct GZipState* state) {
	static const cc_string schematic = String_FromConst(".schematic");
	static const cc_string mine      = String_FromConst(".mine");
	struct Stream stream, compStream;
	cc_result res;

	res = Stream_CreateFile(&stream, path);
	if (res) { Logger_SysWarn2(res, "creating", path); return res; }
	GZip_MakeStream(&compStream, state, &stream);

	if (String_CaselessEnds(path, &schematic)) {
		res = Schematic_Save(&compStream);
	} else if (String_CaselessEnds(path, &mine)) {
		res = Dat_Save(&compStream);
	} else {
		res = Cw_Save(&compStream);
	}

	if (res) {
		stream.Close(&stream);
		Logger_SysWarn2(res, "encoding", path); return res;
	}

	if ((res = compStream.Close(&compStream))) {
		stream.Close(&stream);
		Logger_SysWarn2(res, "closing", path); return res;
	}

	res = stream.Close(&stream);
	if (res) { Logger_SysWarn2(res, "closing", path); return res; }
	return 0;
}

static cc_result SaveLevelScreen_SaveMap(const cc_string* path) {
	struct GZipState* state;
	cc_result res;

	state = Mem_TryAlloc(1, sizeof(struct GZipState));
	res   = ERR_OUT_OF_MEMORY;
	if (!state) { Logger_SysWarn(res, "allocating temp memory"); return res; }

	res = DoSaveMap(path, state);
	Mem_Free(state);
	if (res) return res;

	World.LastSave = Game.Time;
	Gui_ShowPauseMenu();
	return 0;
}

static void SaveLevelScreen_Save(void* screen, void* widget) { 
	struct SaveLevelScreen* s = (struct SaveLevelScreen*)screen;
	struct ButtonWidget* btn  = (struct ButtonWidget*)widget;
	cc_string path; char pathBuffer[FILENAME_SIZE];
	cc_string file = s->input.base.text;
	cc_filepath str;
	cc_result res;

	if (!file.length) {
		TextWidget_SetConst(&s->desc, "&ePlease enter a filename", &s->textFont);
		return;
	}

	String_InitArray(path, pathBuffer);
	String_Format1(&path, "maps/%s.cw", &file);
	String_Copy(&World.Name, &file);

	Platform_EncodePath(&str, &path);
	if (File_Exists(&str) && !btn->optName) {
		btn->optName = "";
		SaveLevelScreen_UpdateSave(s);
		return;
	}
		
	SaveLevelScreen_RemoveOverwrites(s);
	if ((res = SaveLevelScreen_SaveMap(&path))) return;
	Chat_Add1("&eSaved map to: %s", &path);
	CPE_SendNotifyAction(NOTIFY_ACTION_LEVEL_SAVED, 0);
}

static void SaveLevelScreen_UploadCallback(const cc_string* path) {
	cc_result res = SaveLevelScreen_SaveMap(path);
	if (!res) {
		Chat_Add1("&eSaved map to: %s", path);
		CPE_SendNotifyAction(NOTIFY_ACTION_LEVEL_SAVED, 0);
	}
}

static void SaveLevelScreen_File(void* screen, void* b) {
	static const char* const titles[] = {
		"ClassiCube map", "Minecraft schematic", "Minecraft classic map", NULL
	};
	static const char* const filters[] = {
		".cw", ".schematic", ".mine", NULL
	};
	struct SaveLevelScreen* s = (struct SaveLevelScreen*)screen;
	struct SaveFileDialogArgs args;
	cc_result res;

	args.filters     = filters;
	args.titles      = titles;
	args.defaultName = s->input.base.text;
	args.Callback    = SaveLevelScreen_UploadCallback;

	res = Window_SaveFileDialog(&args);
	if (res == SFD_ERR_NEED_DEFAULT_NAME) {
		TextWidget_SetConst(&s->desc, "&ePlease enter a filename", &s->textFont);
	} else if (res) {
		Logger_SimpleWarn(res, "showing save file dialog");
	}
}

static int SaveLevelScreen_KeyPress(void* screen, char keyChar) {
	struct SaveLevelScreen* s = (struct SaveLevelScreen*)screen;
	SaveLevelScreen_RemoveOverwrites(s);
	InputWidget_Append(&s->input.base, keyChar);
	return true;
}

static int SaveLevelScreen_TextChanged(void* screen, const cc_string* str) {
	struct SaveLevelScreen* s = (struct SaveLevelScreen*)screen;
	SaveLevelScreen_RemoveOverwrites(s);
	InputWidget_SetText(&s->input.base, str);
	return true;
}

static int SaveLevelScreen_KeyDown(void* screen, int key, struct InputDevice* device) {
	struct SaveLevelScreen* s = (struct SaveLevelScreen*)screen;
	int handled;
	
	handled = Menu_DoInputDown(s, key, device);
	/* Pressing Enter triggers save */
	if (!handled && InputDevice_IsEnter(key, device))
		SaveLevelScreen_Save(s, &s->save);

	if (key != CCMOUSE_L) SaveLevelScreen_RemoveOverwrites(s);
	return Screen_InputDown(s, key, device);
}

static void SaveLevelScreen_ContextLost(void* screen) {
	struct SaveLevelScreen* s = (struct SaveLevelScreen*)screen;
	Font_Free(&s->titleFont);
	Font_Free(&s->textFont);
	Screen_ContextLost(screen);
}

static void SaveLevelScreen_ContextRecreated(void* screen) {
	struct SaveLevelScreen* s = (struct SaveLevelScreen*)screen;
	Gui_MakeTitleFont(&s->titleFont);
	Gui_MakeBodyFont(&s->textFont);

	Screen_UpdateVb(screen);
	SaveLevelScreen_UpdateSave(s);

	TextInputWidget_SetFont(&s->input,                &s->textFont);
	ButtonWidget_SetConst(&s->cancel, "Cancel",       &s->titleFont);
#ifdef CC_BUILD_WEB
	ButtonWidget_SetConst(&s->file,   "Download",     &s->titleFont);
#else
	ButtonWidget_SetConst(&s->file,   "Save file...", &s->titleFont);
#endif
}

static void SaveLevelScreen_Update(void* screen, float delta) {
	struct SaveLevelScreen* s = (struct SaveLevelScreen*)screen;
	s->input.base.caretAccumulator += delta;
}

static void SaveLevelScreen_Layout(void* screen) {
	struct SaveLevelScreen* s = (struct SaveLevelScreen*)screen;
	Widget_SetLocation(&s->input, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -30);
	Widget_SetLocation(&s->save,  ANCHOR_CENTRE, ANCHOR_CENTRE, 0,  20);
	Widget_SetLocation(&s->desc,  ANCHOR_CENTRE, ANCHOR_CENTRE, 0,  65);

	Widget_SetLocation(&s->file, ANCHOR_CENTRE, ANCHOR_MAX, 0, 70);
	Menu_LayoutBack(&s->cancel);
}

static void SaveLevelScreen_Init(void* screen) {
	struct SaveLevelScreen* s = (struct SaveLevelScreen*)screen;
	struct MenuInputDesc desc;
	
	s->widgets     = save_widgets;
	s->numWidgets  = 0;
	s->maxWidgets  = Array_Elems(save_widgets);
	MenuInput_Path(desc);
	
	ButtonWidget_Add(s, &s->save, 400, SaveLevelScreen_Save);
	ButtonWidget_Add(s, &s->file, 400, SaveLevelScreen_File);

	ButtonWidget_Add(s, &s->cancel, 400, Menu_SwitchPause);
	TextInputWidget_Add(s, &s->input, 400, &World.Name, &desc);	
	Menu_SelectWidget((struct Screen*)s, 3); /* s->input */

	TextWidget_Add(s, &s->desc);
	s->input.onscreenPlaceholder = "Map name";

	s->maxVertices = Screen_CalcDefaultMaxVertices(s);
}

static const struct ScreenVTABLE SaveLevelScreen_VTABLE = {
	SaveLevelScreen_Init,    SaveLevelScreen_Update, Menu_CloseKeyboard,
	MenuScreen_Render2,      Screen_BuildMesh,
	SaveLevelScreen_KeyDown, Screen_InputUp,   SaveLevelScreen_KeyPress, SaveLevelScreen_TextChanged,
	Menu_PointerDown,        Screen_PointerUp, Menu_PointerMove,         Screen_TMouseScroll,
	SaveLevelScreen_Layout,  SaveLevelScreen_ContextLost, SaveLevelScreen_ContextRecreated
};
void SaveLevelScreen_Show(void) {
	struct SaveLevelScreen* s = &SaveLevelScreen;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE = &SaveLevelScreen_VTABLE;

	Gui_Add((struct Screen*)s, GUI_PRIORITY_MENU);
	TextInputWidget_OpenKeyboard(&s->input, NULL);
}


/*########################################################################################################################*
*---------------------------------------------------TexturePackScreen-----------------------------------------------------*
*#########################################################################################################################*/
static void TexturePackScreen_EntryClick(void* screen, void* widget) {
	struct ListScreen* s = (struct ListScreen*)screen;
	cc_string file = ListScreen_UNSAFE_GetCur(s, widget);
	cc_result res;

	TexturePack_SetDefault(&file);
	TexturePack_Url.length = 0;
	res = TexturePack_ExtractCurrent(true);

	CPE_SendNotifyAction(NOTIFY_ACTION_TEXTURE_PACK_CHANGED, 0);

	/* FileNotFound error may be because user deleted .zips from disc */
	if (res != ReturnCode_FileNotFound) return;
	Chat_AddRaw("&eReloading texture pack list as it may be out of date");
	ListScreen_Reload(s);
}

static void TexturePackScreen_FilterFiles(const cc_string* path, void* obj, int isDirectory) {
	static const cc_string zip = String_FromConst(".zip");
	cc_string relPath = *path;

	if (isDirectory) {
		Directory_Enum(path, obj, TexturePackScreen_FilterFiles);
	} else if (String_CaselessEnds(path, &zip)) {
		Utils_UNSAFE_TrimFirstDirectory(&relPath);
		StringsBuffer_Add((struct StringsBuffer*)obj, &relPath);
	}
}

static void TexturePackScreen_LoadEntries(struct ListScreen* s) {
	static const cc_string path = String_FromConst("texpacks");
	Directory_Enum(&path, &s->entries, TexturePackScreen_FilterFiles);
	StringsBuffer_Sort(&s->entries);
}

static void TexturePackScreen_UploadCallback(const cc_string* path) {
#ifdef CC_BUILD_WEB
	cc_string relPath = *path;
	Utils_UNSAFE_GetFilename(&relPath);

	ListScreen_Reload(&ListScreen);
	TexturePack_SetDefault(&relPath);
#else
	String_Copy(&TexturePack_Path, path);
#endif
	TexturePack_ExtractCurrent(true);
}

static void TexturePackScreen_ActionFunc(void* s, void* w) {
	static const char* const filters[] = { 
		".zip", NULL 
	};
	static struct OpenFileDialogArgs args = {
		"Texture packs", filters,
		TexturePackScreen_UploadCallback,
		OFD_UPLOAD_PERSIST, "texpacks"
	};

	cc_result res = Window_OpenFileDialog(&args);
	if (res) Logger_SimpleWarn(res, "showing open file dialog");
}

void TexturePackScreen_Show(void) {
	struct ListScreen* s = &ListScreen;
	s->titleText   = "Select a texture pack";
#ifdef CC_BUILD_WEB
	s->actionText = "Upload";
#else
	s->actionText = "Load file...";
#endif

	s->ActionClick = TexturePackScreen_ActionFunc;
	s->LoadEntries = TexturePackScreen_LoadEntries;
	s->EntryClick  = TexturePackScreen_EntryClick;
	s->DoneClick   = Menu_SwitchPause;
	s->UpdateEntry = ListScreen_UpdateEntry;
	ListScreen_Show();
}


/*########################################################################################################################*
*----------------------------------------------------FontListScreen-------------------------------------------------------*
*#########################################################################################################################*/
static void FontListScreen_EntryClick(void* screen, void* widget) {
	struct ListScreen* s = (struct ListScreen*)screen;
	cc_string fontName   = ListScreen_UNSAFE_GetCur(s, widget);

	Options_Set(OPT_FONT_NAME, &fontName);
	SysFont_SetDefault(&fontName);
}

static void FontListScreen_UpdateEntry(struct ListScreen* s, struct ButtonWidget* button, const cc_string* text) {
	struct FontDesc font;
	cc_result res;

	if (String_CaselessEqualsConst(text, LISTSCREEN_EMPTY)) {
		ButtonWidget_Set(button, text, &s->font); return;
	}
	res = SysFont_Make(&font, text, 16, FONT_FLAGS_NONE);

	if (!res) {
		ButtonWidget_Set(button, text, &font);
	} else {
		Logger_SimpleWarn2(res, "making font", text);
		ButtonWidget_Set(button, text, &s->font);
	}
	Font_Free(&font);
}

static void FontListScreen_LoadEntries(struct ListScreen* s) {
	SysFonts_GetNames(&s->entries);
	ListScreen_Select(s, SysFonts_UNSAFE_GetDefault());
}

static void FontListScreen_RegisterCallback(const cc_string* path) {
	Chat_Add1("Loaded font from %s", path);
}

static void FontListScreen_UploadCallback(const cc_string* path) { 
	cc_result res = SysFonts_Register(path, FontListScreen_RegisterCallback);

	if (res) {
		Logger_SimpleWarn2(res, "loading font from", path);
	} else {
		SysFonts_SaveCache();
	}
}

static void FontListScreen_ActionFunc(void* s, void* w) {
	static const char* const filters[] = {
		".ttf", ".otf", NULL
	};
	static struct OpenFileDialogArgs args = {
		"Font files", filters,
		FontListScreen_UploadCallback,
		OFD_UPLOAD_DELETE, "tmp"
	};

	cc_result res = Window_OpenFileDialog(&args);
	if (res) Logger_SimpleWarn(res, "showing open file dialog");
}

void FontListScreen_Show(void) {
	struct ListScreen* s = &ListScreen;
	s->titleText   = "Select a font";
	s->actionText  = "Load font...";
	s->ActionClick = FontListScreen_ActionFunc;

	s->LoadEntries = FontListScreen_LoadEntries;
	s->EntryClick  = FontListScreen_EntryClick;
	s->DoneClick   = Menu_SwitchGui;
	s->UpdateEntry = FontListScreen_UpdateEntry;
	ListScreen_Show();
}


/*########################################################################################################################*
*---------------------------------------------------HotkeyListScreen------------------------------------------------------*
*#########################################################################################################################*/
/* TODO: Hotkey added event for CPE */
static void HotkeyListScreen_EntryClick(void* screen, void* widget) {
	struct ListScreen* s = (struct ListScreen*)screen;
	struct HotkeyData h, original = { 0 };
	cc_string text, key, value;
	int trigger;
	int i, mods = 0;

	text = ListScreen_UNSAFE_GetCur(s, widget);

	String_UNSAFE_Separate(&text, '+', &key, &value);
	if (String_ContainsConst(&value, "Ctrl"))  mods |= HOTKEY_MOD_CTRL;
	if (String_ContainsConst(&value, "Shift")) mods |= HOTKEY_MOD_SHIFT;
	if (String_ContainsConst(&value, "Alt"))   mods |= HOTKEY_MOD_ALT;

	trigger = Utils_ParseEnum(&key, INPUT_NONE, Input_DisplayNames, INPUT_COUNT);
	for (i = 0; i < HotkeysText.count; i++) {
		h = HotkeysList[i];
		if (h.trigger == trigger && h.mods == mods) { original = h; break; }
	}

	EditHotkeyScreen_Show(original);
}

static void HotkeyListScreen_MakeFlags(int flags, cc_string* str) {
	if (flags & HOTKEY_MOD_CTRL)  String_AppendConst(str, " Ctrl");
	if (flags & HOTKEY_MOD_SHIFT) String_AppendConst(str, " Shift");
	if (flags & HOTKEY_MOD_ALT)   String_AppendConst(str, " Alt");
}

static void HotkeyListScreen_LoadEntries(struct ListScreen* s) {
	cc_string text; char textBuffer[STRING_SIZE];
	struct HotkeyData hKey;
	int i;
	String_InitArray(text, textBuffer);

	for (i = 0; i < HotkeysText.count; i++) {
		hKey = HotkeysList[i];
		text.length = 0;
		String_AppendConst(&text, Input_DisplayNames[hKey.trigger]);

		if (hKey.mods) {
			String_AppendConst(&text, " +");
			HotkeyListScreen_MakeFlags(hKey.mods, &text);
		}
		StringsBuffer_Add(&s->entries, &text);
	}
	StringsBuffer_Sort(&s->entries);
}

static void HotkeyListScreen_UpdateEntry(struct ListScreen* s, struct ButtonWidget* button, const cc_string* text) {
	if (text->length) ButtonWidget_Set(button, text, &s->font);
}

static void HotkeyListScreen_ActionFunc(void* s, void* w) {
	struct HotkeyData original = { 0 };
	EditHotkeyScreen_Show(original);
}

void HotkeyListScreen_Show(void) {
	struct ListScreen* s = &ListScreen;
	s->titleText   = "Modify hotkeys";
	s->actionText  = "New hotkey...";
	
	s->ActionClick = HotkeyListScreen_ActionFunc;
	s->LoadEntries = HotkeyListScreen_LoadEntries;
	s->EntryClick  = HotkeyListScreen_EntryClick;
	s->DoneClick   = Menu_SwitchPause;
	s->UpdateEntry = HotkeyListScreen_UpdateEntry;
	ListScreen_Show();
}


/*########################################################################################################################*
*----------------------------------------------------LoadLevelScreen------------------------------------------------------*
*#########################################################################################################################*/
static void LoadLevelScreen_EntryClick(void* screen, void* widget) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	struct ListScreen* s = (struct ListScreen*)screen;
	cc_result res;

	cc_string relPath = ListScreen_UNSAFE_GetCur(s, widget);
	String_InitArray(path, pathBuffer);
	String_Format1(&path, "maps/%s", &relPath);
	res = Map_LoadFrom(&path);

	/* FileNotFound error may be because user deleted maps from disc */
	if (res != ReturnCode_FileNotFound) return;
	Chat_AddRaw("&eReloading level list as it may be out of date");
	ListScreen_Reload(s);
}

static void LoadLevelScreen_FilterFiles(const cc_string* path, void* obj, int isDirectory) {
	struct MapImporter* imp = MapImporter_Find(path);
	cc_string relPath = *path;

	if (isDirectory) {
		Directory_Enum(path, obj, LoadLevelScreen_FilterFiles);
	} else if (imp) {
		Utils_UNSAFE_TrimFirstDirectory(&relPath);
		StringsBuffer_Add((struct StringsBuffer*)obj, &relPath);
	}
}

static void LoadLevelScreen_LoadEntries(struct ListScreen* s) {
	static const cc_string path = String_FromConst("maps");
	Directory_Enum(&path, &s->entries, LoadLevelScreen_FilterFiles);
	StringsBuffer_Sort(&s->entries);
}

static void LoadLevelScreen_UploadCallback(const cc_string* path) { Map_LoadFrom(path); }
static void LoadLevelScreen_ActionFunc(void* s, void* w) {
	static const char* const filters[] = { 
		".cw", ".dat", ".lvl", ".mine", ".fcm", ".mclevel", NULL 
	}; /* TODO not hardcode list */
	static struct OpenFileDialogArgs args = {
		"Classic map files", filters,
		LoadLevelScreen_UploadCallback,
		OFD_UPLOAD_DELETE, "tmp"
	};

	cc_result res = Window_OpenFileDialog(&args);
	if (res) Logger_SimpleWarn(res, "showing open file dialog");
}

void LoadLevelScreen_Show(void) {
	struct ListScreen* s = &ListScreen;
	s->titleText   = "Load level";
#ifdef CC_BUILD_WEB
	s->actionText = "Upload";
#else
	s->actionText = "Load file...";
#endif
	
	s->ActionClick = LoadLevelScreen_ActionFunc;
	s->LoadEntries = LoadLevelScreen_LoadEntries;
	s->EntryClick  = LoadLevelScreen_EntryClick;
	s->DoneClick   = Menu_SwitchPause;
	s->UpdateEntry = ListScreen_UpdateEntry;
	ListScreen_Show();
}


/*########################################################################################################################*
*----------------------------------------------------BindSourcesScreen----------------------------------------------------*
*#########################################################################################################################*/
static struct BindsSourceScreen {
	Screen_Body
	struct ButtonWidget btns[2], cancel;
} BindsSourceScreen;
static struct InputDevice* bind_device;

static struct Widget* bindsSource_widgets[3];

static void BindsSourceScreen_ModeNormal(void* screen, void* b) {
	bind_device = &NormDevice;
	NormalBindingsScreen_Show();
}

static void BindsSourceScreen_ModeGamepad(void* screen, void* b) {
	bind_device = &Gamepad_Devices[0].base;
	NormalBindingsScreen_Show();
}

static void BindsSourceScreen_ContextRecreated(void* screen) {
	struct BindsSourceScreen* s = (struct BindsSourceScreen*)screen;
	struct FontDesc font;
	Gui_MakeTitleFont(&font);
	Screen_UpdateVb(screen);

	ButtonWidget_SetConst(&s->btns[0], "Keyboard/Mouse",     &font);
	ButtonWidget_SetConst(&s->btns[1], "Gamepad/Controller", &font);
	ButtonWidget_SetConst(&s->cancel,  "Cancel",             &font);
	Font_Free(&font);
}

static void BindsSourceScreen_Layout(void* screen) {
	struct BindsSourceScreen* s = (struct BindsSourceScreen*)screen;
	Widget_SetLocation(&s->btns[0], ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -25);
	Widget_SetLocation(&s->btns[1], ANCHOR_CENTRE, ANCHOR_CENTRE, 0,  25);
	Menu_LayoutBack(&s->cancel);
}

static void BindsSourceScreen_Init(void* screen) {
	struct BindsSourceScreen* s = (struct BindsSourceScreen*)screen;

	s->widgets     = bindsSource_widgets;
	s->numWidgets  = 0;
	s->maxWidgets  = Array_Elems(bindsSource_widgets);
	s->selectedI   = -1;

	ButtonWidget_Add(s, &s->btns[0], 300, BindsSourceScreen_ModeNormal);
	ButtonWidget_Add(s, &s->btns[1], 300, BindsSourceScreen_ModeGamepad);
	ButtonWidget_Add(s, &s->cancel,  400, Menu_SwitchPause);

	s->maxVertices = Screen_CalcDefaultMaxVertices(s);
}

static const struct ScreenVTABLE BindsSourceScreen_VTABLE = {
	BindsSourceScreen_Init,    Screen_NullUpdate, Screen_NullFunc,
	MenuScreen_Render2,        Screen_BuildMesh,
	Menu_InputDown,            Screen_InputUp,    Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,          Screen_PointerUp,  Menu_PointerMove, Screen_TMouseScroll,
	BindsSourceScreen_Layout,  Screen_ContextLost, BindsSourceScreen_ContextRecreated
};
void BindsSourceScreen_Show(void) {
	struct BindsSourceScreen* s = &BindsSourceScreen;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE     = &BindsSourceScreen_VTABLE;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_MENU);
}

static void SwitchBindsMain(void* s, void* w) {
	if (Input.Sources == (INPUT_SOURCE_NORMAL | INPUT_SOURCE_GAMEPAD)) {
		/* User needs to decide whether to configure mouse/keyboard or gamepad */
		BindsSourceScreen_Show();
	} else if (Input.Sources == INPUT_SOURCE_GAMEPAD) {
		bind_device = &Gamepad_Devices[0].base;
		NormalBindingsScreen_Show();
	} else {
		bind_device = &NormDevice;
		NormalBindingsScreen_Show();
	}
}


/*########################################################################################################################*
*---------------------------------------------------KeyBindsScreen-----------------------------------------------------*
*#########################################################################################################################*/
struct KeyBindsScreen;
typedef void (*InitKeyBindings)(struct KeyBindsScreen* s);
#define KEYBINDS_MAX_BTNS 12

static struct KeyBindsScreen {
	Screen_Body	
	int curI, bindsCount;
	const char* const* descs;
	const cc_uint8* binds;
	Widget_LeftClick leftPage, rightPage;
	int btnWidth, topY, arrowsY, leftLen;
	const char* titleText;
	const char* msgText;
	struct FontDesc titleFont;
	struct TextWidget title, msg;
	struct ButtonWidget back, left, right;
	struct ButtonWidget buttons[KEYBINDS_MAX_BTNS];
} KeyBindsScreen;

static struct Widget* key_widgets[KEYBINDS_MAX_BTNS + 5];

static void KeyBindsScreen_Update(struct KeyBindsScreen* s, int i) {
	cc_string text; char textBuffer[STRING_SIZE];
	BindMapping curBind; 

	String_InitArray(text, textBuffer);
	curBind = bind_device->currentBinds[s->binds[i]];

	String_Format4(&text, s->curI == i ? "> %c: %c%c%c <" : "%c: %c%c%c", 
		s->descs[i],
		Input_DisplayNames[curBind.button1],
		curBind.button2 ? " + " : "",
		curBind.button2 ? Input_DisplayNames[curBind.button2] : "");
		
	ButtonWidget_Set(&s->buttons[i], &text, &s->titleFont);
	s->dirty = true;
}

static cc_bool KeyBindsScreen_TriggerBinding(int key, struct InputDevice* device) {
	struct KeyBindsScreen* s = &KeyBindsScreen;
	InputBind bind;
	int idx;
	if (device->type != bind_device->type) return false;
	
	Input.DownHook = NULL;
	if (s->curI == -1) return false;
	bind = s->binds[s->curI];
	
	if (key == device->escapeButton) {
		InputBind_Reset(bind, bind_device);
	} else {
		InputBind_Set(bind, key, bind_device);
	}

	idx         = s->curI;
	s->curI     = -1;
	s->closable = true;
	KeyBindsScreen_Update(s, idx);
	return true;
}

static void KeyBindsScreen_OnBindingClick(void* screen, void* widget) {
	struct KeyBindsScreen* s = (struct KeyBindsScreen*)screen;
	struct ButtonWidget* btn = (struct ButtonWidget*)widget;
	int old;
	
	Input.DownHook = NULL;
	old         = s->curI;
	s->curI     = (int)btn->meta.val;
	s->closable = false;

	KeyBindsScreen_Update(s, s->curI);
	/* previously selected a different button for binding */
	if (old >= 0) KeyBindsScreen_Update(s, old);
	Input.DownHook = KeyBindsScreen_TriggerBinding;
}

static void KeyBindsScreen_ContextLost(void* screen) {
	struct KeyBindsScreen* s = (struct KeyBindsScreen*)screen;
	Font_Free(&s->titleFont);
	Screen_ContextLost(screen);
}

static void KeyBindsScreen_ContextRecreated(void* screen) {
	struct KeyBindsScreen* s = (struct KeyBindsScreen*)screen;
	struct FontDesc textFont;
	int i;

	Screen_UpdateVb(screen);
	Gui_MakeTitleFont(&s->titleFont);
	Gui_MakeBodyFont(&textFont);
	for (i = 0; i < s->bindsCount; i++) { KeyBindsScreen_Update(s, i); }

	TextWidget_SetConst(&s->title, s->titleText, &s->titleFont);
	TextWidget_SetConst(&s->msg,   s->msgText,   &textFont);
	ButtonWidget_SetConst(&s->back, "Done",      &s->titleFont);

	Font_Free(&textFont);
	if (!s->leftPage && !s->rightPage) return;	
	ButtonWidget_SetConst(&s->left,  "<", &s->titleFont);
	ButtonWidget_SetConst(&s->right, ">", &s->titleFont);
}

static void KeyBindsScreen_Layout(void* screen) {
	struct KeyBindsScreen* s = (struct KeyBindsScreen*)screen;
	int i, x, y, xDir, leftLen;
	x = s->btnWidth / 2 + 5;
	y = s->topY;
	
	leftLen = s->leftLen;
	for (i = 0; i < s->bindsCount; i++) {
		if (i == leftLen) y = s->topY; /* reset y for next column */
		xDir = leftLen == -1 ? 0 : (i < leftLen ? -1 : 1);

		Widget_SetLocation(&s->buttons[i], ANCHOR_CENTRE, ANCHOR_CENTRE, x * xDir, y);
		y += 50; /* distance between buttons */
	}

	Widget_SetLocation(&s->title, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -180);
	Widget_SetLocation(&s->msg,   ANCHOR_CENTRE, ANCHOR_CENTRE, 0,  100);
	Menu_LayoutBack(&s->back);
	
	Widget_SetLocation(&s->left,  ANCHOR_CENTRE, ANCHOR_CENTRE, -s->btnWidth - 35, s->arrowsY);
	Widget_SetLocation(&s->right, ANCHOR_CENTRE, ANCHOR_CENTRE,  s->btnWidth + 35, s->arrowsY);
}

static void KeyBindsScreen_Init(void* screen) {
	struct KeyBindsScreen* s = (struct KeyBindsScreen*)screen;
	int i;
	s->widgets     = key_widgets;
	s->numWidgets  = 0;
	s->maxWidgets  = Array_Elems(key_widgets);
	s->curI        = -1;

	for (i = 0; i < s->bindsCount; i++) 
	{
		ButtonWidget_Add(s, &s->buttons[i], s->btnWidth, KeyBindsScreen_OnBindingClick);
		s->widgets[i] = (struct Widget*)&s->buttons[i];
		s->buttons[i].meta.val = i;
	}

	TextWidget_Add(s,   &s->title);
	TextWidget_Add(s,   &s->msg);
	ButtonWidget_Add(s, &s->back, 400, Gui.ClassicMenu ? Menu_SwitchClassicOptions : Menu_SwitchOptions);

	if (s->leftPage || s->rightPage) {
		ButtonWidget_Add(s, &s->left,  40, s->leftPage);
		ButtonWidget_Add(s, &s->right, 40, s->rightPage);
		Widget_SetDisabled(&s->left,   !s->leftPage);
		Widget_SetDisabled(&s->right,  !s->rightPage);
	}

	s->maxVertices = Screen_CalcDefaultMaxVertices(s);
}

static void KeyBindsScreen_Free(void* screen) {
	Input.DownHook = NULL;
}

static const struct ScreenVTABLE KeyBindsScreen_VTABLE = {
	KeyBindsScreen_Init,    Screen_NullUpdate, KeyBindsScreen_Free,  
	MenuScreen_Render2,     Screen_BuildMesh,
	Menu_InputDown,         Screen_InputUp,    Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,       Screen_PointerUp,  Menu_PointerMove, Screen_TMouseScroll,
	KeyBindsScreen_Layout,  KeyBindsScreen_ContextLost, KeyBindsScreen_ContextRecreated
};

static void KeyBindsScreen_Reset(Widget_LeftClick left, Widget_LeftClick right, int btnWidth) {
	struct KeyBindsScreen* s = &KeyBindsScreen;
	s->leftPage  = left;
	s->rightPage = right;
	s->btnWidth  = btnWidth;
	s->msgText   = "";
}
static void KeyBindsScreen_SetLayout(int topY, int arrowsY, int leftLen) {
	struct KeyBindsScreen* s = &KeyBindsScreen;
	s->topY    = topY;
	s->arrowsY = arrowsY;
	s->leftLen = leftLen;
}
static void KeyBindsScreen_Show(int bindsCount, const cc_uint8* binds, const char* const* descs, const char* title) {
	struct KeyBindsScreen* s = &KeyBindsScreen;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE     = &KeyBindsScreen_VTABLE;

	s->titleText  = title;
	s->bindsCount = bindsCount;
	s->binds      = binds;
	s->descs      = descs;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_MENU);
}


/*########################################################################################################################*
*------------------------------------------------ClassicBindingsScreen----------------------------------------------------*
*#########################################################################################################################*/
void ClassicBindingsScreen_Show(void) {
	static const cc_uint8 binds[]    = { BIND_FORWARD, BIND_BACK, BIND_JUMP, BIND_CHAT, BIND_SET_SPAWN, BIND_LEFT, BIND_RIGHT, BIND_INVENTORY, BIND_FOG, BIND_RESPAWN };
	static const char* const descs[] = { "Forward", "Back", "Jump", "Chat", "Save location", "Left", "Right", "Build", "Toggle fog", "Load location" };
	bind_device = Input.Sources == INPUT_SOURCE_GAMEPAD ? &Gamepad_Devices[0].base : &NormDevice;

	if (Game_ClassicHacks) {
		KeyBindsScreen_Reset(NULL, Menu_SwitchBindsClassicHacks, 260);
	} else {
		KeyBindsScreen_Reset(NULL,                         NULL, 300);
	}
	KeyBindsScreen_SetLayout(-140, -40, 5);
	KeyBindsScreen_Show(Array_Elems(binds), binds, descs, 
						Game_ClassicHacks ? "Normal controls" : "Controls");
}


/*########################################################################################################################*
*----------------------------------------------ClassicHacksBindingsScreen-------------------------------------------------*
*#########################################################################################################################*/
void ClassicHacksBindingsScreen_Show(void) {
	static const cc_uint8 binds[6]    = { BIND_SPEED, BIND_NOCLIP, BIND_HALF_SPEED, BIND_FLY, BIND_FLY_UP, BIND_FLY_DOWN };
	static const char* const descs[6] = { "Speed", "Noclip", "Half speed", "Fly", "Fly up", "Fly down" };
	bind_device = Input.Sources == INPUT_SOURCE_GAMEPAD ? &Gamepad_Devices[0].base : &NormDevice;

	KeyBindsScreen_Reset(Menu_SwitchBindsClassic, NULL, 260);
	KeyBindsScreen_SetLayout(-90, -40, 3);
	KeyBindsScreen_Show(Array_Elems(binds), binds, descs, "Hacks controls");
}


/*########################################################################################################################*
*-------------------------------------------------NormalBindingsScreen----------------------------------------------------*
*#########################################################################################################################*/
void NormalBindingsScreen_Show(void) {
	static const cc_uint8 binds[]    = { BIND_FORWARD, BIND_BACK, BIND_JUMP, BIND_CHAT, BIND_SET_SPAWN, BIND_TABLIST, BIND_LEFT, BIND_RIGHT, BIND_INVENTORY, BIND_FOG, BIND_RESPAWN, BIND_SEND_CHAT };
	static const char* const descs[] = { "Forward", "Back", "Jump", "Chat", "Set spawn", "Player list", "Left", "Right", "Inventory", "Toggle fog", "Respawn", "Send chat" };
	
	KeyBindsScreen_Reset(NULL, Menu_SwitchBindsHacks, 250);
	KeyBindsScreen_SetLayout(-140, 10, 6);
	KeyBindsScreen_Show(Array_Elems(binds), binds, descs, "Normal controls");
}


/*########################################################################################################################*
*--------------------------------------------------HacksBindingsScreen----------------------------------------------------*
*#########################################################################################################################*/
void HacksBindingsScreen_Show(void) {
	static const cc_uint8 binds[]    = { BIND_SPEED, BIND_NOCLIP, BIND_HALF_SPEED, BIND_ZOOM_SCROLL, BIND_FLY, BIND_FLY_UP, BIND_FLY_DOWN, BIND_THIRD_PERSON };
	static const char* const descs[] = { "Speed", "Noclip", "Half speed", "Scroll zoom", "Fly", "Fly up", "Fly down", "Third person" };
	
	KeyBindsScreen_Reset(Menu_SwitchBindsNormal, Menu_SwitchBindsOther, 260);
	KeyBindsScreen_SetLayout(-40, 10, 4);
	KeyBindsScreen_Show(Array_Elems(binds), binds, descs, "Hacks controls");
}


/*########################################################################################################################*
*--------------------------------------------------OtherBindingsScreen----------------------------------------------------*
*#########################################################################################################################*/
void OtherBindingsScreen_Show(void) {
	static const cc_uint8 binds[]     = { BIND_EXT_INPUT, BIND_HIDE_FPS, BIND_HIDE_GUI, BIND_HOTBAR_SWITCH, BIND_DROP_BLOCK,BIND_SCREENSHOT, BIND_FULLSCREEN, BIND_AXIS_LINES, BIND_AUTOROTATE, BIND_SMOOTH_CAMERA, BIND_IDOVERLAY, BIND_BREAK_LIQUIDS };
	static const char* const descs[]  = { "Show ext input", "Hide FPS", "Hide gui", "Hotbar switching", "Drop block", "Screenshot", "Fullscreen", "Show axis lines", "Auto-rotate", "Smooth camera", "ID overlay", "Breakable liquids" };
	
	KeyBindsScreen_Reset(Menu_SwitchBindsHacks, Menu_SwitchBindsMouse, 260);
	KeyBindsScreen_SetLayout(-140, 10, 6);
	KeyBindsScreen_Show(Array_Elems(binds), binds, descs, "Other controls");
}


/*########################################################################################################################*
*--------------------------------------------------MouseBindingsScreen----------------------------------------------------*
*#########################################################################################################################*/
void MouseBindingsScreen_Show(void) {
	static const cc_uint8 binds[]    = { BIND_DELETE_BLOCK, BIND_PICK_BLOCK, BIND_PLACE_BLOCK, BIND_LOOK_UP, BIND_LOOK_DOWN, BIND_LOOK_LEFT, BIND_LOOK_RIGHT };
	static const char* const descs[] = { "Delete block", "Pick block", "Place block", "Look Up", "Look Down", "Look Left", "Look Right" };

	KeyBindsScreen_Reset(Menu_SwitchBindsOther, Menu_SwitchBindsHotbar, 260);
	KeyBindsScreen_SetLayout(-140, 10, 3);
	KeyBindsScreen.msgText = "&ePress escape to reset the binding";
	KeyBindsScreen_Show(Array_Elems(binds), binds, descs, "Mouse key bindings");
}


/*########################################################################################################################*
*-------------------------------------------------HotbarBindingsScreen----------------------------------------------------*
*#########################################################################################################################*/
void HotbarBindingsScreen_Show(void) {
	static const cc_uint8 binds[] = { BIND_HOTBAR_1,BIND_HOTBAR_2,BIND_HOTBAR_3, BIND_HOTBAR_4,BIND_HOTBAR_5,BIND_HOTBAR_6, BIND_HOTBAR_7,BIND_HOTBAR_8,BIND_HOTBAR_9,
										BIND_HOTBAR_LEFT, BIND_HOTBAR_RIGHT };
	static const char* const descs[] = { "Slot #1","Slot #2","Slot #3", "Slot #4","Slot #5","Slot #6", "Slot #7","Slot #8","Slot #9", "Slot left","Slot right" };

	KeyBindsScreen_Reset(Menu_SwitchBindsMouse, NULL, 260);
	KeyBindsScreen_SetLayout(-140, 10, 6);
	KeyBindsScreen_Show(Array_Elems(binds), binds, descs, "Hotbar controls");
}



/*########################################################################################################################*
*--------------------------------------------------MenuInputOverlay-------------------------------------------------------*
*#########################################################################################################################*/
static struct MenuInputOverlay {
	Screen_Body
	cc_bool screenMode;
	struct FontDesc textFont;
	struct ButtonWidget ok, Default;
	struct TextInputWidget input;
	struct MenuInputDesc* desc;
	MenuInputDone onDone;
	cc_string value; char valueBuffer[STRING_SIZE];
} MenuInputOverlay;

static struct Widget* menuInput_widgets[2 + 1];

void MenuInputOverlay_Close(cc_bool valid) {
	struct MenuInputOverlay* s = (struct MenuInputOverlay*)&MenuInputOverlay;
	Gui_Remove((struct Screen*)s);
	
	if (s->onDone) s->onDone(&s->input.base.text, valid);
	s->onDone = NULL;
}

static void MenuInputOverlay_EnterInput(struct MenuInputOverlay* s) {
	cc_bool valid = s->desc->VTABLE->IsValidValue(s->desc, &s->input.base.text);
	MenuInputOverlay_Close(valid);
}

static int MenuInputOverlay_KeyPress(void* screen, char keyChar) {
	struct MenuInputOverlay* s = (struct MenuInputOverlay*)screen;
	InputWidget_Append(&s->input.base, keyChar);
	return true;
}

static int MenuInputOverlay_TextChanged(void* screen, const cc_string* str) {
	struct MenuInputOverlay* s = (struct MenuInputOverlay*)screen;
	InputWidget_SetText(&s->input.base, str);
	return true;
}

static int MenuInputOverlay_KeyDown(void* screen, int key, struct InputDevice* device) {
	struct MenuInputOverlay* s = (struct MenuInputOverlay*)screen;

	int handled = Menu_DoInputDown(s, key, device);
	/* Pressing Enter triggers OK click */
	if (!handled && InputDevice_IsEnter(key, device))
		MenuInputOverlay_EnterInput(s);

	return Screen_InputDown(s, key, device);
}

static int MenuInputOverlay_PointerDown(void* screen, int id, int x, int y) {
	struct MenuInputOverlay* s = (struct MenuInputOverlay*)screen;
	return Screen_DoPointerDown(screen, id, x, y) >= 0 || s->screenMode;
}

static int MenuInputOverlay_PointerMove(void* screen, int id, int x, int y) {
	struct MenuInputOverlay* s = (struct MenuInputOverlay*)screen;
	return Menu_DoPointerMove(screen, id, x, y) >= 0 || s->screenMode;
}

static void MenuInputOverlay_OK(void* screen, void* widget) {
	struct MenuInputOverlay* s = (struct MenuInputOverlay*)screen;
	MenuInputOverlay_EnterInput(s);
}

static void MenuInputOverlay_Default(void* screen, void* widget) {
	cc_string value; char valueBuffer[STRING_SIZE];
	struct MenuInputOverlay* s = (struct MenuInputOverlay*)screen;

	String_InitArray(value, valueBuffer);
	s->desc->VTABLE->GetDefault(s->desc, &value);
	InputWidget_SetText(&s->input.base, &value);
}

static void MenuInputOverlay_Init(void* screen) {
	struct MenuInputOverlay* s = (struct MenuInputOverlay*)screen;
	s->widgets     = menuInput_widgets;
	s->numWidgets  = 0;
	s->maxWidgets  = Array_Elems(menuInput_widgets);

	ButtonWidget_Add(s,    &s->ok, Input_TouchMode ? 200 : 40, MenuInputOverlay_OK);
	ButtonWidget_Add(s,    &s->Default,              200, MenuInputOverlay_Default);
	TextInputWidget_Add(s, &s->input,                400, &s->value, s->desc);
	Menu_SelectWidget((struct Screen*)s, 2); /* s->input */

	if (s->desc->VTABLE == &IntInput_VTABLE) {
		s->input.onscreenType = KEYBOARD_TYPE_INTEGER;
	} else if (s->desc->VTABLE == &FloatInput_VTABLE) {
		s->input.onscreenType = KEYBOARD_TYPE_NUMBER;
	}

	s->maxVertices = Screen_CalcDefaultMaxVertices(s);
}

static void MenuInputOverlay_Update(void* screen, float delta) {
	struct MenuInputOverlay* s = (struct MenuInputOverlay*)screen;
	s->input.base.caretAccumulator += delta;
}

static void MenuInputOverlay_Render(void* screen, float delta) {
	struct MenuInputOverlay* s = (struct MenuInputOverlay*)screen;
	if (s->screenMode) Menu_RenderBounds();

	Screen_Render2Widgets(screen, delta);
}

static void MenuInputOverlay_Free(void* screen) {
	OnscreenKeyboard_Close();
}

static void MenuInputOverlay_Layout(void* screen) {
	struct MenuInputOverlay* s = (struct MenuInputOverlay*)screen;
	if (!Input_TouchMode) {
		Widget_SetLocation(&s->input,   ANCHOR_CENTRE, ANCHOR_CENTRE,    0, 110);
		Widget_SetLocation(&s->ok,      ANCHOR_CENTRE, ANCHOR_CENTRE,  240, 110);
		Widget_SetLocation(&s->Default, ANCHOR_CENTRE, ANCHOR_CENTRE,    0, 150);
	} else if (Window_Main.SoftKeyboard == SOFT_KEYBOARD_SHIFT) {
		Widget_SetLocation(&s->input,   ANCHOR_CENTRE, ANCHOR_MAX,       0,  65);
		Widget_SetLocation(&s->ok,      ANCHOR_CENTRE, ANCHOR_MAX,     120,  25);
		Widget_SetLocation(&s->Default, ANCHOR_CENTRE, ANCHOR_MAX,    -120,  25);
	} else {
		Widget_SetLocation(&s->input,   ANCHOR_CENTRE, ANCHOR_CENTRE,    0, 110);
		Widget_SetLocation(&s->ok,      ANCHOR_CENTRE, ANCHOR_CENTRE,  120, 150);
		Widget_SetLocation(&s->Default, ANCHOR_CENTRE, ANCHOR_CENTRE, -120, 150);
	}
}

static void MenuInputOverlay_ContextLost(void* screen) {
	struct MenuInputOverlay* s = (struct MenuInputOverlay*)screen;
	Font_Free(&s->textFont);
	Screen_ContextLost(s);
}

static void MenuInputOverlay_ContextRecreated(void* screen) {
	struct MenuInputOverlay* s = (struct MenuInputOverlay*)screen;
	struct FontDesc font;
	Gui_MakeTitleFont(&font);
	Gui_MakeBodyFont(&s->textFont);
	Screen_UpdateVb(s);

	TextInputWidget_SetFont(&s->input, &s->textFont);
	ButtonWidget_SetConst(&s->ok,      "OK",            &font);
	ButtonWidget_SetConst(&s->Default, "Default value", &font);
	Font_Free(&font);
}

static const struct ScreenVTABLE MenuInputOverlay_VTABLE = {
	MenuInputOverlay_Init,        MenuInputOverlay_Update, MenuInputOverlay_Free,
	MenuInputOverlay_Render,      Screen_BuildMesh,
	MenuInputOverlay_KeyDown,     Screen_InputUp,   MenuInputOverlay_KeyPress,    MenuInputOverlay_TextChanged,
	MenuInputOverlay_PointerDown, Screen_PointerUp, MenuInputOverlay_PointerMove, Screen_TMouseScroll,
	MenuInputOverlay_Layout,      MenuInputOverlay_ContextLost, MenuInputOverlay_ContextRecreated
};
void MenuInputOverlay_Show(struct MenuInputDesc* desc, const cc_string* value, MenuInputDone onDone, cc_bool screenMode) {
	struct MenuInputOverlay* s = &MenuInputOverlay;
	s->grabsInput = true;
	s->closable   = true;
	s->desc       = desc;
	s->onDone     = onDone;
	s->screenMode = screenMode;
	s->VTABLE     = &MenuInputOverlay_VTABLE;

	String_InitArray(s->value, s->valueBuffer);
	String_Copy(&s->value, value);
	Gui_Add((struct Screen*)s, GUI_PRIORITY_MENUINPUT);
}


/*########################################################################################################################*
*---------------------------------------------------------Overlay---------------------------------------------------------*
*#########################################################################################################################*/
static void Overlay_AddLabels(void* screen, struct TextWidget* labels) {
	int i;
	TextWidget_Add(screen, &labels[0]);
	for (i = 1; i < 4; i++) 
	{
		TextWidget_Add(screen, &labels[i]);
		labels[i].color = PackedCol_Make(224, 224, 224, 255);
	}
}

static void Overlay_LayoutLabels(struct TextWidget* labels) {
	int i;
	Widget_SetLocation(&labels[0],     ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -120);
	for (i = 1; i < 4; i++) {
		Widget_SetLocation(&labels[i], ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -70 + 20 * i);
	}
}

static void Overlay_LayoutMainButtons(struct ButtonWidget* btns) {
	Widget_SetLocation(&btns[0], ANCHOR_CENTRE, ANCHOR_CENTRE, -110, 30);
	Widget_SetLocation(&btns[1], ANCHOR_CENTRE, ANCHOR_CENTRE,  110, 30);
}


/*########################################################################################################################*
*------------------------------------------------------TexIdsOverlay------------------------------------------------------*
*#########################################################################################################################*/
static struct TexIdsOverlay {
	Screen_Body
	int xOffset, yOffset, tileSize, textVertices;
	struct TextAtlas idAtlas;
	struct TextWidget title;
} TexIdsOverlay;
static struct Widget* texids_widgets[1];

#define TEXIDS_MAX_ROWS_PER_PAGE 16
#define TEXIDS_MAX_PER_PAGE      (TEXIDS_MAX_ROWS_PER_PAGE * ATLAS2D_TILES_PER_ROW)
#define TEXIDS_TEXT_VERTICES (10 * 4 + 90 * 8 + 412 * 12) /* '0'-'9' + '10'-'99' + '100'-'511' */
#define TEXIDS_MAX_VERTICES (TEXTWIDGET_MAX + 4 * ATLAS1D_MAX_ATLASES + TEXIDS_TEXT_VERTICES)

static void TexIdsOverlay_Layout(void* screen) {
	struct TexIdsOverlay* s = (struct TexIdsOverlay*)screen;
	int size;

	size = Window_UI.Height / ATLAS2D_TILES_PER_ROW;
	size = (size / 8) * 8;
	Math_Clamp(size, 8, 40);

	s->xOffset  = Gui_CalcPos(ANCHOR_CENTRE, 0, size * Atlas2D.RowsCount,     Window_UI.Width);
	s->yOffset  = Gui_CalcPos(ANCHOR_CENTRE, 0, size * ATLAS2D_TILES_PER_ROW, Window_UI.Height);
	s->tileSize = size;

	/* Can't use vertical centreing here */
	Widget_SetLocation(&s->title, ANCHOR_CENTRE, ANCHOR_MIN, 0, 0);
	s->title.yOffset = s->yOffset - Display_ScaleY(30);
	Widget_Layout(&s->title);
}

static void TexIdsOverlay_ContextLost(void* screen) {
	struct TexIdsOverlay* s = (struct TexIdsOverlay*)screen;
	Screen_ContextLost(s);
	TextAtlas_Free(&s->idAtlas);
}

static void TexIdsOverlay_ContextRecreated(void* screen) {
	static const cc_string chars  = String_FromConst("0123456789");
	static const cc_string prefix = String_FromConst("f");
	struct TexIdsOverlay* s = (struct TexIdsOverlay*)screen;
	struct FontDesc textFont, titleFont;

	Screen_UpdateVb(screen);
	Font_Make(&textFont, 8, FONT_FLAGS_PADDING);
	Font_SetPadding(&textFont, 1);
	TextAtlas_Make(&s->idAtlas, &chars, &textFont, &prefix);
	Font_Free(&textFont);
	
	Gui_MakeTitleFont(&titleFont);
	TextWidget_SetConst(&s->title, "Texture ID reference sheet", &titleFont);
	Font_Free(&titleFont);
}

static void TexIdsOverlay_BuildTerrain(struct TexIdsOverlay* s, struct VertexTextured** ptr) {
	struct Texture tex;
	int baseLoc, xOffset;
	int i, row, size;

	size    = s->tileSize;
	baseLoc = 0;
	xOffset = s->xOffset;

	tex.uv.u1 = 0.0f; tex.uv.u2 = UV2_Scale;
	tex.width = size; tex.height = size;

	for (row = 0; row < Atlas2D.RowsCount; row += TEXIDS_MAX_ROWS_PER_PAGE) {
		for (i = 0; i < TEXIDS_MAX_PER_PAGE; i++) {

			tex.x = xOffset    + Atlas2D_TileX(i) * size;
			tex.y = s->yOffset + Atlas2D_TileY(i) * size;

			tex.uv.v1 = Atlas1D_RowId(i + baseLoc) * Atlas1D.InvTileSize;
			tex.uv.v2 = tex.uv.v1      + UV2_Scale * Atlas1D.InvTileSize;
			
			Gfx_Make2DQuad(&tex, PACKEDCOL_WHITE, ptr);
		}

		baseLoc += TEXIDS_MAX_PER_PAGE;
		xOffset += size * ATLAS2D_TILES_PER_ROW;
	}
}

static void TexIdsOverlay_BuildText(struct TexIdsOverlay* s, struct VertexTextured** ptr) {
	struct TextAtlas* idAtlas;
	struct VertexTextured* beg;
	int xOffset, size, row;
	int x, y, id = 0;

	size    = s->tileSize;
	xOffset = s->xOffset;
	idAtlas = &s->idAtlas;
	beg     = *ptr;
	
	for (row = 0; row < Atlas2D.RowsCount; row += TEXIDS_MAX_ROWS_PER_PAGE) {
		idAtlas->tex.y = s->yOffset + (size - idAtlas->tex.height);

		for (y = 0; y < ATLAS2D_TILES_PER_ROW; y++) {
			for (x = 0; x < ATLAS2D_TILES_PER_ROW; x++) {
				idAtlas->curX = xOffset + size * x + 3; /* offset text by 3 pixels */
				TextAtlas_AddInt(idAtlas, id++, ptr);
			}
			idAtlas->tex.y += size;
		}
		xOffset += size * ATLAS2D_TILES_PER_ROW;
	}	
	s->textVertices = (int)(*ptr - beg);
}

static void TexIdsOverlay_BuildMesh(void* screen) {
	struct TexIdsOverlay* s = (struct TexIdsOverlay*)screen;
	struct VertexTextured* data;
	struct VertexTextured** ptr;

	data = Screen_LockVb(s);
	ptr  = &data;

	Widget_BuildMesh(&s->title, ptr);
	TexIdsOverlay_BuildTerrain(s, ptr);
	TexIdsOverlay_BuildText(s, ptr);
	Gfx_UnlockDynamicVb(s->vb);
}

static int TexIdsOverlay_RenderTerrain(struct TexIdsOverlay* s, int offset) {
	int i, count = Atlas1D.TilesPerAtlas * 4;
	for (i = 0; i < Atlas1D.Count; i++) 
	{
		Atlas1D_Bind(i);

		Gfx_DrawVb_IndexedTris_Range(count, offset, DRAW_HINT_SPRITE);
		offset += count;
	}
	return offset;
}

static void TexIdsOverlay_OnAtlasChanged(void* screen) {
	struct TexIdsOverlay* s = (struct TexIdsOverlay*)screen;
	s->dirty = true;
	/* Atlas may have 256 or 512 textures, which changes s->xOffset */
	/* This can resize the position of the 'pages', so just re-layout */
	TexIdsOverlay_Layout(screen);
}

static void TexIdsOverlay_Init(void* screen) {
	struct TexIdsOverlay* s = (struct TexIdsOverlay*)screen;
	s->widgets     = texids_widgets;
	s->numWidgets  = 0;
	s->maxWidgets  = Array_Elems(texids_widgets);
	s->maxVertices = TEXIDS_MAX_VERTICES;

	TextWidget_Add(s, &s->title);
	Event_Register_(&TextureEvents.AtlasChanged, s, TexIdsOverlay_OnAtlasChanged);
}

static void TexIdsOverlay_Free(void* screen) {
	struct TexIdsOverlay* s = (struct TexIdsOverlay*)screen;
	Event_Unregister_(&TextureEvents.AtlasChanged, s, TexIdsOverlay_OnAtlasChanged);
}

static void TexIdsOverlay_Render(void* screen, float delta) {
	struct TexIdsOverlay* s = (struct TexIdsOverlay*)screen;
	int offset = 0;
	Menu_RenderBounds();

	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_BindDynamicVb(s->vb);

	offset = Widget_Render2(&s->title, offset);
	offset = TexIdsOverlay_RenderTerrain(s, offset);

	Gfx_BindTexture(s->idAtlas.tex.ID);
	Gfx_DrawVb_IndexedTris_Range(s->textVertices, offset, DRAW_HINT_SPRITE);
}

static int TexIdsOverlay_KeyDown(void* screen, int key, struct InputDevice* device) {
	return false;
}

static const struct ScreenVTABLE TexIdsOverlay_VTABLE = {
	TexIdsOverlay_Init,    Screen_NullUpdate, TexIdsOverlay_Free,
	TexIdsOverlay_Render,  TexIdsOverlay_BuildMesh,
	TexIdsOverlay_KeyDown, Screen_InputUp,    Screen_FKeyPress, Screen_FText,
	Menu_PointerDown,      Screen_PointerUp,  Menu_PointerMove, Screen_TMouseScroll,
	TexIdsOverlay_Layout,  TexIdsOverlay_ContextLost, TexIdsOverlay_ContextRecreated
};
void TexIdsOverlay_Show(void) {
	struct TexIdsOverlay* s = &TexIdsOverlay;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE     = &TexIdsOverlay_VTABLE;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_TEXIDS);
}


/*########################################################################################################################*
*----------------------------------------------------UrlWarningOverlay----------------------------------------------------*
*#########################################################################################################################*/
static struct UrlWarningOverlay {
	Screen_Body
	cc_string url;
	struct ButtonWidget btns[2];
	struct TextWidget   lbls[4];
	char _urlBuffer[STRING_SIZE * 4];
} UrlWarningOverlay;

static struct Widget* urlwarning_widgets[4 + 2];

static void UrlWarningOverlay_OpenUrl(void* screen, void* b) {
	struct UrlWarningOverlay* s = (struct UrlWarningOverlay*)screen;
	cc_result res = Process_StartOpen(&s->url);
	if (res) Logger_SimpleWarn2(res, "opening url in browser", &s->url);
	Gui_Remove((struct Screen*)s);
}

static void UrlWarningOverlay_AppendUrl(void* screen, void* b) {
	struct UrlWarningOverlay* s = (struct UrlWarningOverlay*)screen;
	if (Gui.ClickableChat) ChatScreen_AppendInput(&s->url);
	Gui_Remove((struct Screen*)s);
}

static void UrlWarningOverlay_ContextRecreated(void* screen) {
	struct UrlWarningOverlay* s = (struct UrlWarningOverlay*)screen;
	struct FontDesc titleFont, textFont;
	Screen_UpdateVb(screen);

	Gui_MakeTitleFont(&titleFont);
	Gui_MakeBodyFont(&textFont);

	TextWidget_SetConst(&s->lbls[0], "&eAre you sure you want to open this link?", &titleFont);
	TextWidget_Set(&s->lbls[1],      &s->url,                                      &textFont);
	TextWidget_SetConst(&s->lbls[2], "Be careful - links from strangers may be websites that", &textFont);
	TextWidget_SetConst(&s->lbls[3], " have viruses, or things you may not want to open/see.", &textFont);

	ButtonWidget_SetConst(&s->btns[0], "Yes", &titleFont);
	ButtonWidget_SetConst(&s->btns[1], "No",  &titleFont);
	Font_Free(&titleFont);
	Font_Free(&textFont);
}

static void UrlWarningOverlay_Layout(void* screen) {
	struct UrlWarningOverlay* s = (struct UrlWarningOverlay*)screen;
	Overlay_LayoutLabels(s->lbls);
	Overlay_LayoutMainButtons(s->btns);
}

static void UrlWarningOverlay_Init(void* screen) {
	struct UrlWarningOverlay* s = (struct UrlWarningOverlay*)screen;
	s->widgets     = urlwarning_widgets;
	s->numWidgets  = 0;
	s->maxWidgets  = Array_Elems(urlwarning_widgets);

	Overlay_AddLabels(s, s->lbls);
	ButtonWidget_Add(s, &s->btns[0], 160, UrlWarningOverlay_OpenUrl);
	ButtonWidget_Add(s, &s->btns[1], 160, UrlWarningOverlay_AppendUrl);

	s->maxVertices = Screen_CalcDefaultMaxVertices(s);
}

static const struct ScreenVTABLE UrlWarningOverlay_VTABLE = {
	UrlWarningOverlay_Init,   Screen_NullUpdate,  Screen_NullFunc,  
	MenuScreen_Render2,       Screen_BuildMesh,
	Menu_InputDown,           Screen_InputUp,     Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,         Screen_PointerUp,   Menu_PointerMove, Screen_TMouseScroll,
	UrlWarningOverlay_Layout, Screen_ContextLost, UrlWarningOverlay_ContextRecreated
};
void UrlWarningOverlay_Show(const cc_string* url) {
	struct UrlWarningOverlay* s = &UrlWarningOverlay;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE     = &UrlWarningOverlay_VTABLE;

	String_InitArray(s->url, s->_urlBuffer);
	String_Copy(&s->url, url);
	Gui_Add((struct Screen*)s, GUI_PRIORITY_URLWARNING);
}


/*########################################################################################################################*
*-----------------------------------------------------TexPackOverlay------------------------------------------------------*
*#########################################################################################################################*/
static struct TexPackOverlay {
	Screen_Body
	cc_bool deny, alwaysDeny, gotContent;
	cc_uint32 contentLength;
	cc_string url;
	int reqID;
	struct FontDesc textFont;
	struct ButtonWidget btns[4];
	struct TextWidget   lbls[4];
	char _urlBuffer[URL_MAX_SIZE];
} TexPackOverlay;

static struct Widget* texpack_widgets[4 + 4];

static cc_bool TexPackOverlay_IsAlways(void* screen, void* w) { 
	struct ButtonWidget* btn = (struct ButtonWidget*)w;
	return btn->meta.val != 0;
}

static void TexPackOverlay_YesClick(void* screen, void* widget) {
	struct TexPackOverlay* s = (struct TexPackOverlay*)screen;
	TexturePack_Extract(&s->url);
	if (TexPackOverlay_IsAlways(s, widget)) TextureUrls_Accept(&s->url);
	Gui_Remove((struct Screen*)s);

	if (TexPackOverlay_IsAlways(s, widget)) CPE_SendNotifyAction(NOTIFY_ACTION_TEXTURE_PROMPT_RESPONDED, 3);
	else CPE_SendNotifyAction(NOTIFY_ACTION_TEXTURE_PROMPT_RESPONDED, 2);
}

static void TexPackOverlay_NoClick(void* screen, void* widget) {
	struct TexPackOverlay* s = (struct TexPackOverlay*)screen;
	s->alwaysDeny = TexPackOverlay_IsAlways(s, widget);
	s->deny       = true;
	Gui_Refresh((struct Screen*)s);
}

static void TexPackOverlay_ConfirmNoClick(void* screen, void* b) {
	struct TexPackOverlay* s = (struct TexPackOverlay*)screen;
	if (s->alwaysDeny) TextureUrls_Deny(&s->url);
	Gui_Remove((struct Screen*)s);

	if (s->alwaysDeny) CPE_SendNotifyAction(NOTIFY_ACTION_TEXTURE_PROMPT_RESPONDED, 0);
	else CPE_SendNotifyAction(NOTIFY_ACTION_TEXTURE_PROMPT_RESPONDED, 1);
}

static void TexPackOverlay_GoBackClick(void* screen, void* b) {
	struct TexPackOverlay* s = (struct TexPackOverlay*)screen;
	s->deny = false;
	Gui_Refresh((struct Screen*)s);
}

static void TexPackOverlay_UpdateLine2(struct TexPackOverlay* s) {
	static const cc_string https = String_FromConst("https://");
	static const cc_string http  = String_FromConst("http://");
	cc_string url = String_Empty;

	if (!s->deny) {
		url = s->url;
		if (String_CaselessStarts(&url, &https)) {
			url = String_UNSAFE_SubstringAt(&url, https.length);
		}
		if (String_CaselessStarts(&url, &http)) {
			url = String_UNSAFE_SubstringAt(&url, http.length);
		}
	}
	TextWidget_Set(&s->lbls[2], &url, &s->textFont);
}

static void TexPackOverlay_UpdateLine3(struct TexPackOverlay* s) {
	cc_string contents; char contentsBuffer[STRING_SIZE];
	float contentLengthMB;

	if (s->deny) {
		TextWidget_SetConst(&s->lbls[3], "Sure you don't want to download the texture pack?", &s->textFont);
	} else if (s->contentLength) {
		String_InitArray(contents, contentsBuffer);
		contentLengthMB = s->contentLength / (1024.0f * 1024.0f);
		String_Format1(&contents, "Download size: %f3 MB", &contentLengthMB);
		TextWidget_Set(&s->lbls[3], &contents, &s->textFont);
	} else if (s->gotContent) {
		TextWidget_SetConst(&s->lbls[3], "Download size: Unknown", &s->textFont);
	} else {
		TextWidget_SetConst(&s->lbls[3], "Download size: Determining...", &s->textFont);
	}
}

static void TexPackOverlay_Update(void* screen, float delta) {
	struct TexPackOverlay* s = (struct TexPackOverlay*)screen;
	struct HttpRequest item;
	if (!Http_GetResult(s->reqID, &item)) return;

	s->dirty         = true;
	s->gotContent    = true;
	s->contentLength = item.contentLength;

	TexPackOverlay_UpdateLine3(s);
	HttpRequest_Free(&item);
}

static void TexPackOverlay_ContextLost(void* screen) {
	struct TexPackOverlay* s = (struct TexPackOverlay*)screen;
	Font_Free(&s->textFont);
	Screen_ContextLost(screen);
}

static void TexPackOverlay_ContextRecreated(void* screen) {
	struct TexPackOverlay* s = (struct TexPackOverlay*)screen;
	struct FontDesc titleFont;
	Screen_UpdateVb(screen);

	Gui_MakeTitleFont(&titleFont);
	Gui_MakeBodyFont(&s->textFont);

	TextWidget_SetConst(&s->lbls[0], s->deny  ? "&eYou might be missing out." 
		: "Do you want to download the server's texture pack?", &titleFont);
	TextWidget_SetConst(&s->lbls[1], !s->deny ? "Texture pack url:"
		: "Texture packs can play a vital role in the look and feel of maps.", &s->textFont);
	TexPackOverlay_UpdateLine2(s);
	TexPackOverlay_UpdateLine3(s);

	ButtonWidget_SetConst(&s->btns[0], s->deny ? "I'm sure" : "Yes", &titleFont);
	ButtonWidget_SetConst(&s->btns[1], s->deny ? "Go back"  : "No",  &titleFont);
	s->btns[0].MenuClick = s->deny ? TexPackOverlay_ConfirmNoClick : TexPackOverlay_YesClick;
	s->btns[1].MenuClick = s->deny ? TexPackOverlay_GoBackClick    : TexPackOverlay_NoClick;

	if (!s->deny) {
		ButtonWidget_SetConst(&s->btns[2], "Always yes", &titleFont);
		ButtonWidget_SetConst(&s->btns[3], "Always no",  &titleFont);
		s->btns[2].MenuClick = TexPackOverlay_YesClick;
		s->btns[3].MenuClick = TexPackOverlay_NoClick;
		s->btns[2].meta.val  = 1;
		s->btns[3].meta.val  = 1;
	}

	s->numWidgets = s->deny ? 6 : 8;
	Font_Free(&titleFont);
}

static void TexPackOverlay_Layout(void* screen) {
	struct TexPackOverlay* s = (struct TexPackOverlay*)screen;
	Overlay_LayoutLabels(s->lbls);
	Overlay_LayoutMainButtons(s->btns);
	Widget_SetLocation(&s->btns[2], ANCHOR_CENTRE, ANCHOR_CENTRE, -110, 85);
	Widget_SetLocation(&s->btns[3], ANCHOR_CENTRE, ANCHOR_CENTRE,  110, 85);
}

static void TexPackOverlay_Init(void* screen) {
	struct TexPackOverlay* s = (struct TexPackOverlay*)screen;
	s->widgets     = texpack_widgets;
	s->numWidgets  = 0;
	s->maxWidgets  = Array_Elems(texpack_widgets);

	s->contentLength = 0;
	s->gotContent    = false;
	s->deny          = false;
	Overlay_AddLabels(s, s->lbls);

	ButtonWidget_Add(s, &s->btns[0], 160, NULL);
	ButtonWidget_Add(s, &s->btns[1], 160, NULL);
	ButtonWidget_Add(s, &s->btns[2], 160, NULL);
	ButtonWidget_Add(s, &s->btns[3], 160, NULL);

	s->maxVertices = Screen_CalcDefaultMaxVertices(s);
}

static const struct ScreenVTABLE TexPackOverlay_VTABLE = {
	TexPackOverlay_Init,   TexPackOverlay_Update, Screen_NullFunc,
	MenuScreen_Render2,    Screen_BuildMesh,
	Menu_InputDown,        Screen_InputUp,        Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,      Screen_PointerUp,      Menu_PointerMove, Screen_TMouseScroll,
	TexPackOverlay_Layout, TexPackOverlay_ContextLost, TexPackOverlay_ContextRecreated
};
void TexPackOverlay_Show(const cc_string* url) {
	struct TexPackOverlay* s = &TexPackOverlay;
	s->grabsInput = true;
	/* Too easy to accidentally ESC this important dialog */
	/* s->closable= true; */
	s->VTABLE     = &TexPackOverlay_VTABLE;
	
	String_InitArray(s->url, s->_urlBuffer);
	String_Copy(&s->url, url);

	s->reqID = Http_AsyncGetHeaders(url, HTTP_FLAG_PRIORITY);
	Gui_Add((struct Screen*)s, GUI_PRIORITY_TEXPACK);
}


/*########################################################################################################################*
*--------------------------------------------------NostalgiaMenuScreen-----------------------------------------------------*
*#########################################################################################################################*/
static struct NostalgiaMenuScreen {
	Screen_Body
	struct ButtonWidget btnA, btnF, done;
	struct TextWidget title;
} NostalgiaMenuScreen;

static struct Widget* nostalgiaMenu_widgets[4];

static void NostalgiaMenuScreen_Appearance(void* a, void* b)    { NostalgiaAppearanceScreen_Show(); }
static void NostalgiaMenuScreen_Functionality(void* a, void* b) { NostalgiaFunctionalityScreen_Show(); }

static void NostalgiaMenuScreen_SwitchBack(void* a, void* b) {
	if (Gui.ClassicMenu) { Menu_SwitchPause(a, b); } else { Menu_SwitchOptions(a, b); }
}

static void NostalgiaMenuScreen_ContextRecreated(void* screen) {
	struct NostalgiaMenuScreen* s = (struct NostalgiaMenuScreen*)screen;
	struct FontDesc titleFont;
	Screen_UpdateVb(screen);
	Gui_MakeTitleFont(&titleFont);

	TextWidget_SetConst(&s->title, "Nostalgia options", &titleFont);
	ButtonWidget_SetConst(&s->btnA, "Appearance",       &titleFont);
	ButtonWidget_SetConst(&s->btnF, "Functionality",    &titleFont);
	ButtonWidget_SetConst(&s->done,    "Done",          &titleFont);

	Font_Free(&titleFont);
}

static void NostalgiaMenuScreen_Layout(void* screen) {
	struct NostalgiaMenuScreen* s = (struct NostalgiaMenuScreen*)screen;
	Widget_SetLocation(&s->title, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -100);
	Widget_SetLocation(&s->btnA,  ANCHOR_CENTRE, ANCHOR_CENTRE, 0,  -25);
	Widget_SetLocation(&s->btnF,  ANCHOR_CENTRE, ANCHOR_CENTRE, 0,   25);
	Menu_LayoutBack(&s->done);
}

static void NostalgiaMenuScreen_Init(void* screen) {
	struct NostalgiaMenuScreen* s = (struct NostalgiaMenuScreen*)screen;

	s->widgets     = nostalgiaMenu_widgets;
	s->numWidgets  = 0;
	s->maxWidgets  = Array_Elems(nostalgiaMenu_widgets);

	ButtonWidget_Add(s, &s->btnA, 400, NostalgiaMenuScreen_Appearance);
	ButtonWidget_Add(s, &s->btnF, 400, NostalgiaMenuScreen_Functionality);
	ButtonWidget_Add(s, &s->done, 400, NostalgiaMenuScreen_SwitchBack);
	TextWidget_Add(s,   &s->title);

	s->maxVertices = Screen_CalcDefaultMaxVertices(s);
}

static const struct ScreenVTABLE NostalgiaMenuScreen_VTABLE = {
	NostalgiaMenuScreen_Init,  Screen_NullUpdate, Screen_NullFunc,
	MenuScreen_Render2,        Screen_BuildMesh,
	Menu_InputDown,            Screen_InputUp,    Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,          Screen_PointerUp,  Menu_PointerMove, Screen_TMouseScroll,
	NostalgiaMenuScreen_Layout, Screen_ContextLost, NostalgiaMenuScreen_ContextRecreated
};
void NostalgiaMenuScreen_Show(void) {
	struct NostalgiaMenuScreen* s = &NostalgiaMenuScreen;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE     = &NostalgiaMenuScreen_VTABLE;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_MENU);
}
#else
void TexIdsOverlay_Show(void) { }
void UrlWarningOverlay_Show(const cc_string* url) { }
#endif
