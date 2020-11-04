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
#include "Logger.h"
#include "Options.h"
#include "Utils.h"

/* Describes a menu option button */
struct MenuOptionDesc {
	short dir, y;
	const char* name;
	Widget_LeftClick OnClick;
	Button_Get GetValue; Button_Set SetValue;
};
struct SimpleButtonDesc { short x, y; const char* title; Widget_LeftClick onClick; };


/*########################################################################################################################*
*--------------------------------------------------------Menu base--------------------------------------------------------*
*#########################################################################################################################*/
static void Menu_InitButtons(struct ButtonWidget* btns, int width, const struct SimpleButtonDesc* descs, int count) {
	int i;
	for (i = 0; i < count; i++) {
		ButtonWidget_Init(&btns[i], width, descs[i].onClick);
	}
}

static void Menu_LayoutButtons(struct ButtonWidget* btns, const struct SimpleButtonDesc* descs, int count) {
	int i;
	for (i = 0; i < count; i++) {
		Widget_SetLocation(&btns[i], ANCHOR_CENTRE, ANCHOR_CENTRE,  descs[i].x, descs[i].y);
	}
}

static void Menu_SetButtons(struct ButtonWidget* btns, struct FontDesc* font, const struct SimpleButtonDesc* descs, int count) {
	int i;
	for (i = 0; i < count; i++) {
		ButtonWidget_SetConst(&btns[i], descs[i].title, font);
	}
}

static void Menu_InitBack(struct ButtonWidget* btn, Widget_LeftClick onClick) {
	ButtonWidget_Init(btn, Gui.ClassicMenu ? 400 : 200, onClick);
}

static void Menu_LayoutBack(struct ButtonWidget* btn) {
	Widget_SetLocation(btn, ANCHOR_CENTRE, ANCHOR_MAX, 0, 25);
}

CC_NOINLINE static void Menu_MakeTitleFont(struct FontDesc* font) { Drawer2D_MakeFont(font, 16, FONT_FLAGS_BOLD); }
CC_NOINLINE static void Menu_MakeBodyFont(struct FontDesc* font)  { Drawer2D_MakeFont(font, 16, FONT_FLAGS_NONE); }
static void Menu_CloseKeyboard(void* s) { Window_CloseKeyboard(); }

static void Menu_RenderBounds(void) {
	/* These were sourced by taking a screenshot of vanilla
	Then using paint to extract the colour components
	Then using wolfram alpha to solve the glblendfunc equation */
	PackedCol topCol    = PackedCol_Make(24, 24, 24, 105);
	PackedCol bottomCol = PackedCol_Make(51, 51, 98, 162);
	Gfx_Draw2DGradient(0, 0, WindowInfo.Width, WindowInfo.Height, topCol, bottomCol);
}

static int Menu_DoPointerDown(void* screen, int id, int x, int y) {
	struct Screen* s = (struct Screen*)screen;
	struct Widget** widgets = s->widgets;
	int i, count = s->numWidgets;

	/* iterate backwards (because last elements rendered are shown over others) */
	for (i = count - 1; i >= 0; i--) {
		struct Widget* w = widgets[i];
		if (!w || !Widget_Contains(w, x, y)) continue;
		if (w->disabled) return i;

		if (w->MenuClick) {
			w->MenuClick(s, w);
		} else {
			Elem_HandlesPointerDown(w, id, x, y);
		}
		return i;
	}
	return -1;
}
int Menu_PointerDown(void* screen, int id, int x, int y) {
	Menu_DoPointerDown(screen, id, x, y); return true;
}

static int Menu_DoPointerMove(void* screen, int id, int x, int y) {
	struct Screen* s = (struct Screen*)screen;
	struct Widget** widgets = s->widgets;
	int i, count = s->numWidgets;

	/* TODO: id mask */
	for (i = 0; i < count; i++) {
		struct Widget* w = widgets[i];
		if (w) w->active = false;
	}

	for (i = count - 1; i >= 0; i--) {
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


/*########################################################################################################################*
*------------------------------------------------------Menu utilities-----------------------------------------------------*
*#########################################################################################################################*/
static int Menu_Index(void* screen, void* widget) {
	struct Screen* s = (struct Screen*)screen;
	struct Widget** widgets = s->widgets;
	int i;

	struct Widget* w = (struct Widget*)widget;
	for (i = 0; i < s->numWidgets; i++) {
		if (widgets[i] == w) return i;
	}
	return -1;
}

static void Menu_Remove(void* screen, int i) {
	struct Screen* s = (struct Screen*)screen;
	struct Widget** widgets = s->widgets;

	if (widgets[i]) { Elem_Free(widgets[i]); }
	widgets[i] = NULL;
}

static void Menu_BeginGen(int width, int height, int length) {
	World_NewMap();
	World_SetDimensions(width, height, length);
	GeneratingScreen_Show();
}

static int Menu_Int(const cc_string* str)     { int v;   Convert_ParseInt(str, &v);   return v; }
static float Menu_Float(const cc_string* str) { float v; Convert_ParseFloat(str, &v); return v; }
static PackedCol Menu_HexCol(const cc_string* str) { 
	cc_uint8 rgb[3]; 
	PackedCol_TryParseHex(str, rgb); 
	return PackedCol_Make(rgb[0], rgb[1], rgb[2], 255);
}

static void Menu_SwitchOptions(void* a, void* b)        { OptionsGroupScreen_Show(); }
static void Menu_SwitchPause(void* a, void* b)          { PauseScreen_Show(); }
static void Menu_SwitchClassicOptions(void* a, void* b) { ClassicOptionsScreen_Show(); }

static void Menu_SwitchKeysClassic(void* a, void* b)      { ClassicKeyBindingsScreen_Show(); }
static void Menu_SwitchKeysClassicHacks(void* a, void* b) { ClassicHacksKeyBindingsScreen_Show(); }
static void Menu_SwitchKeysNormal(void* a, void* b)       { NormalKeyBindingsScreen_Show(); }
static void Menu_SwitchKeysHacks(void* a, void* b)        { HacksKeyBindingsScreen_Show(); }
static void Menu_SwitchKeysOther(void* a, void* b)        { OtherKeyBindingsScreen_Show(); }
static void Menu_SwitchKeysMouse(void* a, void* b)        { MouseKeyBindingsScreen_Show(); }

static void Menu_SwitchMisc(void* a, void* b)      { MiscOptionsScreen_Show(); }
static void Menu_SwitchChat(void* a, void* b)      { ChatOptionsScreen_Show(); }
static void Menu_SwitchGui(void* a, void* b)       { GuiOptionsScreen_Show(); }
static void Menu_SwitchGfx(void* a, void* b)       { GraphicsOptionsScreen_Show(); }
static void Menu_SwitchHacks(void* a, void* b)     { HacksSettingsScreen_Show(); }
static void Menu_SwitchEnv(void* a, void* b)       { EnvSettingsScreen_Show(); }
static void Menu_SwitchNostalgia(void* a, void* b) { NostalgiaScreen_Show(); }

static void Menu_SwitchGenLevel(void* a, void* b)        { GenLevelScreen_Show(); }
static void Menu_SwitchClassicGenLevel(void* a, void* b) { ClassicGenScreen_Show(); }
static void Menu_SwitchLoadLevel(void* a, void* b)       { LoadLevelScreen_Show(); }
static void Menu_SwitchSaveLevel(void* a, void* b)       { SaveLevelScreen_Show(); }
static void Menu_SwitchTexPacks(void* a, void* b)        { TexturePackScreen_Show(); }
static void Menu_SwitchHotkeys(void* a, void* b)         { HotkeyListScreen_Show(); }
static void Menu_SwitchFont(void* a, void* b)            { FontListScreen_Show(); }


/*########################################################################################################################*
*--------------------------------------------------------ListScreen-------------------------------------------------------*
*#########################################################################################################################*/
struct ListScreen;
#define LIST_SCREEN_ITEMS 5

static struct ListScreen {
	Screen_Body
	struct ButtonWidget btns[LIST_SCREEN_ITEMS];
	struct ButtonWidget left, right, done;
	struct FontDesc font;
	float wheelAcc;
	int currentIndex;
	Widget_LeftClick EntryClick, DoneClick;
	void (*LoadEntries)(struct ListScreen* s);
	void (*UpdateEntry)(struct ListScreen* s, struct ButtonWidget* btn, const cc_string* text);
	const char* titleText;
	struct TextWidget title, page;
	struct StringsBuffer entries;
} ListScreen;

static struct Widget* list_widgets[10] = {
	(struct Widget*)&ListScreen.btns[0], (struct Widget*)&ListScreen.btns[1],
	(struct Widget*)&ListScreen.btns[2], (struct Widget*)&ListScreen.btns[3],
	(struct Widget*)&ListScreen.btns[4], (struct Widget*)&ListScreen.left,
	(struct Widget*)&ListScreen.right,   (struct Widget*)&ListScreen.title,   
	(struct Widget*)&ListScreen.page,    (struct Widget*)&ListScreen.done
};
#define LIST_MAX_VERTICES (8 * BUTTONWIDGET_MAX + 2 * TEXTWIDGET_MAX)
#define LISTSCREEN_EMPTY "-----"

static void ListScreen_Layout(void* screen) {
	struct ListScreen* s = (struct ListScreen*)screen;
	int i;
	for (i = 0; i < LIST_SCREEN_ITEMS; i++) { 
		Widget_SetLocation(&s->btns[i],
			ANCHOR_CENTRE, ANCHOR_CENTRE, 0, (i - 2) * 50);
	}
	Menu_LayoutBack(&s->done);

	Widget_SetLocation(&s->left,  ANCHOR_CENTRE, ANCHOR_CENTRE, -220,    0);
	Widget_SetLocation(&s->right, ANCHOR_CENTRE, ANCHOR_CENTRE,  220,    0);
	Widget_SetLocation(&s->title, ANCHOR_CENTRE, ANCHOR_CENTRE,    0, -155);
	Widget_SetLocation(&s->page,  ANCHOR_CENTRE,    ANCHOR_MAX,    0,   75);
}

static STRING_REF cc_string ListScreen_UNSAFE_Get(struct ListScreen* s, int index) {
	static const cc_string str = String_FromConst(LISTSCREEN_EMPTY);

	if (index >= 0 && index < s->entries.count) {
		return StringsBuffer_UNSAFE_Get(&s->entries, index);
	}
	return str;
}

static void ListScreen_UpdatePage(struct ListScreen* s) {
	cc_string page; char pageBuffer[STRING_SIZE];
	int end, num, pages;

	end = s->entries.count - LIST_SCREEN_ITEMS;
	s->left.disabled  = s->currentIndex <= 0;
	s->right.disabled = s->currentIndex >= end;

	if (Game_ClassicMode) return;
	num   = (s->currentIndex / LIST_SCREEN_ITEMS) + 1;
	pages = Math_CeilDiv(s->entries.count, LIST_SCREEN_ITEMS);
	if (pages == 0) pages = 1;

	String_InitArray(page, pageBuffer);
	String_Format2(&page, "&7Page %i of %i", &num, &pages);
	TextWidget_Set(&s->page, &page, &s->font);
}

static void ListScreen_UpdateEntry(struct ListScreen* s, struct ButtonWidget* button, const cc_string* text) {
	ButtonWidget_Set(button, text, &s->font);
}

static void ListScreen_RedrawEntries(struct ListScreen* s) {
	cc_string str;
	int i;

	for (i = 0; i < LIST_SCREEN_ITEMS; i++) {
		str = ListScreen_UNSAFE_Get(s, s->currentIndex + i);
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

static void ListScreen_QuickSort(int left, int right) {
	struct StringsBuffer* buffer = &ListScreen.entries; 
	cc_uint32* keys = buffer->flagsBuffer; cc_uint32 key;

	while (left < right) {
		int i = left, j = right;
		cc_string pivot = StringsBuffer_UNSAFE_Get(buffer, (i + j) >> 1);
		cc_string strI, strJ;

		/* partition the list */
		while (i <= j) {
			while ((strI = StringsBuffer_UNSAFE_Get(buffer, i), String_Compare(&pivot, &strI)) > 0) i++;
			while ((strJ = StringsBuffer_UNSAFE_Get(buffer, j), String_Compare(&pivot, &strJ)) < 0) j--;
			QuickSort_Swap_Maybe();
		}
		/* recurse into the smaller subset */
		QuickSort_Recurse(ListScreen_QuickSort)
	}
}

CC_NOINLINE static void ListScreen_Sort(struct ListScreen* s) {
	if (s->entries.count) {
		ListScreen_QuickSort(0, s->entries.count - 1);
	}
}

static cc_string ListScreen_UNSAFE_GetCur(struct ListScreen* s, void* widget) {
	int i = Menu_Index(s, widget);
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

static int ListScreen_KeyDown(void* screen, int key) {
	struct ListScreen* s = (struct ListScreen*)screen;
	if (key == KEY_LEFT || key == KEY_PAGEUP) {
		ListScreen_PageClick(s, false);
	} else if (key == KEY_RIGHT || key == KEY_PAGEDOWN) {
		ListScreen_PageClick(s, true);
	}
	return true;
}

static int ListScreen_MouseScroll(void* screen, float delta) {
	struct ListScreen* s = (struct ListScreen*)screen;
	int steps = Utils_AccumulateWheelDelta(&s->wheelAcc, delta);

	if (steps) ListScreen_SetCurrentIndex(s, s->currentIndex - steps);
	return true;
}

static void ListScreen_Init(void* screen) {
	struct ListScreen* s = (struct ListScreen*)screen;
	int i;
	s->widgets    = list_widgets;
	s->numWidgets = Array_Elems(list_widgets);
	s->wheelAcc   = 0.0f;
	s->currentIndex = 0;
	s->maxVertices  = LIST_MAX_VERTICES;

	for (i = 0; i < LIST_SCREEN_ITEMS; i++) { 
		ButtonWidget_Init(&s->btns[i], 300, s->EntryClick);
	}

	ButtonWidget_Init(&s->left,  40, ListScreen_MoveBackwards);
	ButtonWidget_Init(&s->right, 40, ListScreen_MoveForwards);
	TextWidget_Init(&s->title);
	TextWidget_Init(&s->page);
	Menu_InitBack(&s->done, s->DoneClick);
	s->LoadEntries(s);
}

static void ListScreen_Render(void* screen, double delta) {
	Menu_RenderBounds();
	Gfx_SetTexturing(true);
	Screen_Render2Widgets(screen, delta);
	Gfx_SetTexturing(false);
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
	Screen_CreateVb(screen);
	Menu_MakeTitleFont(&s->font);
	ListScreen_RedrawEntries(s);

	ButtonWidget_SetConst(&s->left,  "<",          &s->font);
	ButtonWidget_SetConst(&s->right, ">",          &s->font);
	TextWidget_SetConst(&s->title,   s->titleText, &s->font);
	ButtonWidget_SetConst(&s->done, "Done",        &s->font);
	ListScreen_UpdatePage(s);
}

static const struct ScreenVTABLE ListScreen_VTABLE = {
	ListScreen_Init,    Screen_NullUpdate, ListScreen_Free,  
	ListScreen_Render,  Screen_BuildMesh,
	ListScreen_KeyDown, Screen_TInput,     Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,   Screen_TPointer,   Menu_PointerMove, ListScreen_MouseScroll,
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
static void MenuScreen_Render(void* screen, double delta) {
	Menu_RenderBounds();
	Gfx_SetTexturing(true);
	Screen_RenderWidgets(screen, delta);
	Gfx_SetTexturing(false);
}

static void MenuScreen_Render2(void* screen, double delta) {
	Menu_RenderBounds();
	Gfx_SetTexturing(true);
	Screen_Render2Widgets(screen, delta);
	Gfx_SetTexturing(false);
}


/*########################################################################################################################*
*-------------------------------------------------------PauseScreen-------------------------------------------------------*
*#########################################################################################################################*/
#define PAUSE_MAX_BTNS 6
static struct PauseScreen {
	Screen_Body
	int descsCount;
	const struct SimpleButtonDesc* descs;
	struct ButtonWidget btns[PAUSE_MAX_BTNS], quit, back;
} PauseScreen;

static struct Widget* pause_widgets[PAUSE_MAX_BTNS + 2] = {
	NULL,NULL,NULL,NULL,NULL,NULL,
	(struct Widget*)&PauseScreen.quit, (struct Widget*)&PauseScreen.back
};
#define PAUSE_MAX_VERTICES ((PAUSE_MAX_BTNS + 2) * BUTTONWIDGET_MAX)

static void PauseScreen_Quit(void* a, void* b) { Window_Close(); }
static void PauseScreen_Game(void* a, void* b) { Gui_Remove((struct Screen*)&PauseScreen); }

static void PauseScreen_CheckHacksAllowed(void* screen) {
	struct PauseScreen* s = (struct PauseScreen*)screen;
	if (Gui.ClassicMenu) return;
	s->btns[4].disabled = !LocalPlayer_Instance.Hacks.CanAnyHacks; /* select texture pack */
	s->dirty = true;
}

static void PauseScreen_ContextRecreated(void* screen) {
	struct PauseScreen* s = (struct PauseScreen*)screen;
	struct FontDesc titleFont;

	Screen_CreateVb(screen);
	Menu_MakeTitleFont(&titleFont);
	Menu_SetButtons(s->btns, &titleFont, s->descs, s->descsCount);

	if (!Gui.ClassicMenu) ButtonWidget_SetConst(&s->quit, "Quit game", &titleFont);
	ButtonWidget_SetConst(&s->back, "Back to game", &titleFont);

	if (!Server.IsSinglePlayer) {
		s->btns[1].disabled = true;
		s->btns[2].disabled = true;
	}
	PauseScreen_CheckHacksAllowed(s);
	Font_Free(&titleFont);
}

static void PauseScreen_Layout(void* screen) {
	struct PauseScreen* s = (struct PauseScreen*)screen;
	Menu_LayoutButtons(s->btns, s->descs, s->descsCount);
	Widget_SetLocation(&s->quit, ANCHOR_MAX, ANCHOR_MAX, 5, 5);
	Menu_LayoutBack(&s->back);
}

static void PauseScreen_Init(void* screen) {
	struct PauseScreen* s = (struct PauseScreen*)screen;
	int i, count;
	static const struct SimpleButtonDesc classicDescs[5] = {
		{    0, -100, "Options...",             Menu_SwitchClassicOptions },
		{    0,  -50, "Generate new level...",  Menu_SwitchClassicGenLevel },
		{    0,    0, "Load level...",          Menu_SwitchLoadLevel },
		{    0,   50, "Save level...",          Menu_SwitchSaveLevel },
		{    0,  150, "Nostalgia options...",   Menu_SwitchNostalgia }
	};
	static const struct SimpleButtonDesc modernDescs[6] = {
		{ -160,  -50, "Options...",             Menu_SwitchOptions   },
		{  160,  -50, "Generate new level...",  Menu_SwitchGenLevel  },
		{  160,    0, "Load level...",          Menu_SwitchLoadLevel },
		{  160,   50, "Save level...",          Menu_SwitchSaveLevel },
		{ -160,    0, "Change texture pack...", Menu_SwitchTexPacks  },
		{ -160,   50, "Hotkeys...",             Menu_SwitchHotkeys   }
	};

	s->widgets     = pause_widgets;
	s->numWidgets  = Array_Elems(pause_widgets);
	s->maxVertices = PAUSE_MAX_VERTICES;
	Event_Register_(&UserEvents.HackPermissionsChanged, s, PauseScreen_CheckHacksAllowed);

	if (Gui.ClassicMenu) {
		s->descs = classicDescs; /*400*/
		/* Don't show nostalgia options in classic mode */
		count    = Game_ClassicMode ? 4 : 5;
	} else {
		s->descs = modernDescs; /*300*/
		count    = 6;
	}

	s->descsCount = count;
	for (i = 0; i < count; i++) { s->widgets[i] = (struct Widget*)&s->btns[i]; }
	for (; i < PAUSE_MAX_BTNS; i++) { s->widgets[i] = NULL; }

	Menu_InitButtons(s->btns, Gui.ClassicMenu ? 400 : 300, s->descs, count);
	ButtonWidget_Init(&s->quit, 120, PauseScreen_Quit);
	Menu_InitBack(&s->back, PauseScreen_Game);
}

static void PauseScreen_Free(void* screen) {
	struct PauseScreen* s = (struct PauseScreen*)screen;
	Event_Unregister_(&UserEvents.HackPermissionsChanged, s, PauseScreen_CheckHacksAllowed);
}

static const struct ScreenVTABLE PauseScreen_VTABLE = {
	PauseScreen_Init,   Screen_NullUpdate, PauseScreen_Free, 
	MenuScreen_Render,  Screen_BuildMesh,
	Screen_InputDown,   Screen_TInput,     Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,   Screen_TPointer,   Menu_PointerMove, Screen_TMouseScroll,
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
*--------------------------------------------------OptionsGroupScreen-----------------------------------------------------*
*#########################################################################################################################*/
static struct OptionsGroupScreen {
	Screen_Body
	int selectedI;
	struct FontDesc textFont;
	struct ButtonWidget btns[8];
	struct TextWidget desc;	
	struct ButtonWidget done;	
} OptionsGroupScreen;

static struct Widget* optGroups_widgets[10] = {
	(struct Widget*)&OptionsGroupScreen.btns[0], (struct Widget*)&OptionsGroupScreen.btns[1],
	(struct Widget*)&OptionsGroupScreen.btns[2], (struct Widget*)&OptionsGroupScreen.btns[3],
	(struct Widget*)&OptionsGroupScreen.btns[4], (struct Widget*)&OptionsGroupScreen.btns[5],
	(struct Widget*)&OptionsGroupScreen.btns[6], (struct Widget*)&OptionsGroupScreen.btns[7],
	(struct Widget*)&OptionsGroupScreen.desc,    (struct Widget*)&OptionsGroupScreen.done
};
#define OPTGROUPS_MAX_VERTICES (8 * BUTTONWIDGET_MAX + TEXTWIDGET_MAX + BUTTONWIDGET_MAX)

static const char* const optsGroup_descs[8] = {
	"&eMusic/Sound, view bobbing, and more",
	"&eGui scale, font settings, and more",
	"&eFPS limit, view distance, entity names/shadows",
	"&eSet key bindings, bind keys to act as mouse clicks",
	"&eChat options",
	"&eHacks allowed, jump settings, and more",
	"&eEnv colours, water level, weather, and more",
	"&eSettings for resembling the original classic",
};
static const struct SimpleButtonDesc optsGroup_btns[8] = {
	{ -160, -100, "Misc options...",      Menu_SwitchMisc       },
	{ -160,  -50, "Gui options...",       Menu_SwitchGui        },
	{ -160,    0, "Graphics options...",  Menu_SwitchGfx        },
	{ -160,   50, "Controls...",          Menu_SwitchKeysNormal },
	{  160, -100, "Chat options...",      Menu_SwitchChat       },
	{  160,  -50, "Hacks settings...",    Menu_SwitchHacks      },
	{  160,    0, "Env settings...",      Menu_SwitchEnv        },
	{  160,   50, "Nostalgia options...", Menu_SwitchNostalgia  }
};

static void OptionsGroupScreen_CheckHacksAllowed(void* screen) {
	struct OptionsGroupScreen* s = (struct OptionsGroupScreen*)screen;
	s->btns[6].disabled = !LocalPlayer_Instance.Hacks.CanAnyHacks; /* env settings */
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
	Screen_CreateVb(screen);

	Menu_MakeTitleFont(&titleFont);
	Menu_MakeBodyFont(&s->textFont);

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

	Event_Register_(&UserEvents.HackPermissionsChanged, s, OptionsGroupScreen_CheckHacksAllowed);
	s->widgets     = optGroups_widgets;
	s->numWidgets  = Array_Elems(optGroups_widgets);
	s->selectedI   = -1;
	s->maxVertices = OPTGROUPS_MAX_VERTICES;

	Menu_InitButtons(s->btns, 300, optsGroup_btns, 8);
	TextWidget_Init(&s->desc);
	Menu_InitBack(&s->done, Menu_SwitchPause);
}

static void OptionsGroupScreen_Free(void* screen) {
	struct OptionsGroupScreen* s = (struct OptionsGroupScreen*)screen;
	Event_Unregister_(&UserEvents.HackPermissionsChanged, s, OptionsGroupScreen_CheckHacksAllowed);
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
	Screen_InputDown,          Screen_TInput,     Screen_TKeyPress,               Screen_TText,
	Menu_PointerDown,          Screen_TPointer,   OptionsGroupScreen_PointerMove, Screen_TMouseScroll,
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
	int selectedI;
	cc_bool supressNextPress;
	int barX, barY[2], barWidth, barHeight;
	struct FontDesc titleFont, textFont;
	struct MenuInputWidget input;
	struct ButtonWidget btns[5], cancel;
} EditHotkeyScreen;

static struct Widget* edithotkey_widgets[7] = {
	(struct Widget*)&EditHotkeyScreen.btns[0], (struct Widget*)&EditHotkeyScreen.btns[1],
	(struct Widget*)&EditHotkeyScreen.btns[2], (struct Widget*)&EditHotkeyScreen.btns[3],
	(struct Widget*)&EditHotkeyScreen.btns[4], (struct Widget*)&EditHotkeyScreen.input,
	(struct Widget*)&EditHotkeyScreen.cancel
};
#define EDITHOTKEY_MAX_VERTICES (MENUINPUTWIDGET_MAX + 6 * BUTTONWIDGET_MAX)

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
		String_AppendConst(&text, Input_Names[s->curHotkey.Trigger]);
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
		EditHotkeyScreen_MakeFlags(s->curHotkey.Flags, &text);
	}
	ButtonWidget_Set(&s->btns[1], &text, &s->titleFont);
}

static void EditHotkeyScreen_UpdateLeaveOpen(struct EditHotkeyScreen* s) {
	cc_string text; char textBuffer[STRING_SIZE];
	String_InitArray(text, textBuffer);

	String_AppendConst(&text, "Input stays open: ");
	String_AppendConst(&text, s->curHotkey.StaysOpen ? "ON" : "OFF");
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
	
	s->curHotkey.StaysOpen = !s->curHotkey.StaysOpen;
	EditHotkeyScreen_UpdateLeaveOpen(s);
}

static void EditHotkeyScreen_SaveChanges(void* screen, void* b) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	struct HotkeyData hk = s->origHotkey;

	if (hk.Trigger) {
		Hotkeys_Remove(hk.Trigger, hk.Flags);
		StoredHotkeys_Remove(hk.Trigger, hk.Flags);
	}

	hk = s->curHotkey;
	if (hk.Trigger) {
		cc_string text = s->input.base.text;
		Hotkeys_Add(hk.Trigger, hk.Flags, &text, hk.StaysOpen);
		StoredHotkeys_Add(hk.Trigger, hk.Flags, hk.StaysOpen, &text);
	}
	HotkeyListScreen_Show();
}

static void EditHotkeyScreen_RemoveHotkey(void* screen, void* b) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	struct HotkeyData hk = s->origHotkey;

	if (hk.Trigger) {
		Hotkeys_Remove(hk.Trigger, hk.Flags);
		StoredHotkeys_Remove(hk.Trigger, hk.Flags);
	}
	HotkeyListScreen_Show();
}

static void EditHotkeyScreen_Render(void* screen, double delta) {
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
#ifdef CC_BUILD_TOUCH
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	InputWidget_SetText(&s->input.base, str);
#endif
	return true;
}

static int EditHotkeyScreen_KeyDown(void* screen, int key) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	if (s->selectedI >= 0) {
		if (s->selectedI == 0) {
			s->curHotkey.Trigger = key;
		} else if (s->selectedI == 1) {
			if      (key == KEY_LCTRL  || key == KEY_RCTRL)  s->curHotkey.Flags |= HOTKEY_MOD_CTRL;
			else if (key == KEY_LSHIFT || key == KEY_RSHIFT) s->curHotkey.Flags |= HOTKEY_MOD_SHIFT;
			else if (key == KEY_LALT   || key == KEY_RALT)   s->curHotkey.Flags |= HOTKEY_MOD_ALT;
			else s->curHotkey.Flags = 0;
		}

		s->supressNextPress = true;
		s->selectedI        = -1;

		EditHotkeyScreen_UpdateBaseKey(s);
		EditHotkeyScreen_UpdateModifiers(s);
		return true;
	}
	return Elem_HandlesKeyDown(&s->input.base, key) || Screen_InputDown(s, key);
}

static void EditHotkeyScreen_ContextLost(void* screen) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	Font_Free(&s->titleFont);
	Font_Free(&s->textFont);
	Screen_ContextLost(screen);
}

static void EditHotkeyScreen_ContextRecreated(void* screen) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	cc_bool existed = s->origHotkey.Trigger != KEY_NONE;

	Menu_MakeTitleFont(&s->titleFont);
	Menu_MakeBodyFont(&s->textFont);
	Screen_CreateVb(screen);

	EditHotkeyScreen_UpdateBaseKey(s);
	EditHotkeyScreen_UpdateModifiers(s);
	EditHotkeyScreen_UpdateLeaveOpen(s);

	ButtonWidget_SetConst(&s->btns[3], existed ? "Save changes" : "Add hotkey", &s->titleFont);
	ButtonWidget_SetConst(&s->btns[4], existed ? "Remove hotkey" : "Cancel",    &s->titleFont);
	MenuInputWidget_SetFont(&s->input, &s->textFont);
	ButtonWidget_SetConst(&s->cancel, "Cancel", &s->titleFont);
}

static void EditHotkeyScreen_Update(void* screen, double delta) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	s->input.base.caretAccumulator += delta;
}

static void EditHotkeyScreen_Layout(void* screen) {
	struct EditHotkeyScreen* s = (struct EditHotkeyScreen*)screen;
	s->barWidth  = Display_ScaleX(500);
	s->barX      = Gui_CalcPos(ANCHOR_CENTRE, 0, s->barWidth, WindowInfo.Width);
	s->barHeight = Display_ScaleY(2);

	s->barY[0] = Gui_CalcPos(ANCHOR_CENTRE, Display_ScaleY(-65), 
					s->barHeight, WindowInfo.Height);
	s->barY[1] = Gui_CalcPos(ANCHOR_CENTRE, Display_ScaleY( 45), 
					s->barHeight, WindowInfo.Height);

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
	s->numWidgets  = Array_Elems(edithotkey_widgets);
	s->selectedI   = -1;
	s->maxVertices = EDITHOTKEY_MAX_VERTICES;
	MenuInput_String(desc);

	ButtonWidget_Init(&s->btns[0], 300, EditHotkeyScreen_BaseKey);
	ButtonWidget_Init(&s->btns[1], 300, EditHotkeyScreen_Modifiers);
	ButtonWidget_Init(&s->btns[2], 300, EditHotkeyScreen_LeaveOpen);
	ButtonWidget_Init(&s->btns[3], 300, EditHotkeyScreen_SaveChanges);
	ButtonWidget_Init(&s->btns[4], 300, EditHotkeyScreen_RemoveHotkey);

	if (s->origHotkey.Trigger) {
		text = StringsBuffer_UNSAFE_Get(&HotkeysText, s->origHotkey.TextIndex);
	} else { text = String_Empty; }

	MenuInputWidget_Create(&s->input, 500, &text, &desc);
	Menu_InitBack(&s->cancel, Menu_SwitchHotkeys);
	Window_OpenKeyboard(&text, KEYBOARD_TYPE_TEXT);
}

static const struct ScreenVTABLE EditHotkeyScreen_VTABLE = {
	EditHotkeyScreen_Init,    EditHotkeyScreen_Update, Menu_CloseKeyboard,
	EditHotkeyScreen_Render,  Screen_BuildMesh,
	EditHotkeyScreen_KeyDown, Screen_TInput,     EditHotkeyScreen_KeyPress, EditHotkeyScreen_TextChanged,
	Menu_PointerDown,         Screen_TPointer,   Menu_PointerMove,          Screen_TMouseScroll,
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
	struct MenuInputWidget* selected;
	struct MenuInputWidget inputs[4];
	struct TextWidget labels[4], title;
} GenLevelScreen;

static struct Widget* gen_widgets[12] = {
	(struct Widget*)&GenLevelScreen.inputs[0], (struct Widget*)&GenLevelScreen.inputs[1],
	(struct Widget*)&GenLevelScreen.inputs[2], (struct Widget*)&GenLevelScreen.inputs[3],
	(struct Widget*)&GenLevelScreen.labels[0], (struct Widget*)&GenLevelScreen.labels[1],
	(struct Widget*)&GenLevelScreen.labels[2], (struct Widget*)&GenLevelScreen.labels[3],
	(struct Widget*)&GenLevelScreen.title,     (struct Widget*)&GenLevelScreen.flatgrass,
	(struct Widget*)&GenLevelScreen.vanilla,   (struct Widget*)&GenLevelScreen.cancel
};
#define GEN_MAX_VERTICES (3 * BUTTONWIDGET_MAX + 4 * MENUINPUTWIDGET_MAX + 5 * TEXTWIDGET_MAX)

CC_NOINLINE static int GenLevelScreen_GetInt(struct GenLevelScreen* s, int index) {
	struct MenuInputWidget* input = &s->inputs[index];
	struct MenuInputDesc* desc;
	cc_string text = input->base.text;
	int value;

	desc = &input->desc;
	if (!desc->VTABLE->IsValidValue(desc, &text)) return 0;
	Convert_ParseInt(&text, &value); return value;
}

CC_NOINLINE static int GenLevelScreen_GetSeedInt(struct GenLevelScreen* s, int index) {
	struct MenuInputWidget* input = &s->inputs[index];
	RNGState rnd;

	if (!input->base.text.length) {
		Random_SeedFromCurrentTime(&rnd);
		return Random_Next(&rnd, Int32_MaxValue);
	}
	return GenLevelScreen_GetInt(s, index);
}

static void GenLevelScreen_Gen(void* screen, cc_bool vanilla) {
	struct GenLevelScreen* s = (struct GenLevelScreen*)screen;
	int width  = GenLevelScreen_GetInt(s, 0);
	int height = GenLevelScreen_GetInt(s, 1);
	int length = GenLevelScreen_GetInt(s, 2);
	int seed   = GenLevelScreen_GetSeedInt(s, 3);

	cc_uint64 volume = (cc_uint64)width * height * length;
	if (volume > Int32_MaxValue) {
		Chat_AddRaw("&cThe generated map's volume is too big.");
	} else if (!width || !height || !length) {
		Chat_AddRaw("&cOne of the map dimensions is invalid.");
	} else {
		Gen_Vanilla = vanilla; Gen_Seed = seed;
		Gui_Remove((struct Screen*)s);
		Menu_BeginGen(width, height, length);
	}
}

static void GenLevelScreen_Flatgrass(void* a, void* b) { GenLevelScreen_Gen(a, false); }
static void GenLevelScreen_Notchy(void* a, void* b)    { GenLevelScreen_Gen(a, true);  }

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

	MenuInputWidget_Create(&s->inputs[i], 200, &tmp, &desc);
	s->inputs[i].base.showCaret = false;
	TextWidget_Init(&s->labels[i]);
	s->labels[i].col = PackedCol_Make(224, 224, 224, 255);
}

static int GenLevelScreen_KeyDown(void* screen, int key) {
	struct GenLevelScreen* s = (struct GenLevelScreen*)screen;
	if (s->selected && Elem_HandlesKeyDown(&s->selected->base, key)) return true;
	return Screen_InputDown(s, key);
}

static int GenLevelScreen_KeyPress(void* screen, char keyChar) {
	struct GenLevelScreen* s = (struct GenLevelScreen*)screen;
	if (s->selected) InputWidget_Append(&s->selected->base, keyChar);
	return true;
}

static int GenLevelScreen_TextChanged(void* screen, const cc_string* str) {
#ifdef CC_BUILD_TOUCH
	struct GenLevelScreen* s = (struct GenLevelScreen*)screen;
	if (s->selected) InputWidget_SetText(&s->selected->base, str);
#endif
	return true;
}

static int GenLevelScreen_PointerDown(void* screen, int id, int x, int y) {
	struct GenLevelScreen* s = (struct GenLevelScreen*)screen;
	int i = Menu_DoPointerDown(screen, id, x, y);
	if (i == -1 || i >= 4) return true;

	if (s->selected) s->selected->base.showCaret = false;
	s->selected = (struct MenuInputWidget*)&s->inputs[i];
	s->selected->base.showCaret = true;
	Window_SetKeyboardText(&s->inputs[i].base.text);
	return true;
}

static void GenLevelScreen_ContextLost(void* screen) {
	struct GenLevelScreen* s = (struct GenLevelScreen*)screen;
	Font_Free(&s->textFont);
	Screen_ContextLost(screen);
}

static void GenLevelScreen_ContextRecreated(void* screen) {
	struct GenLevelScreen* s  = (struct GenLevelScreen*)screen;
	struct FontDesc titleFont;
	Menu_MakeTitleFont(&titleFont);
	Menu_MakeBodyFont(&s->textFont);
	Screen_CreateVb(screen);

	MenuInputWidget_SetFont(&s->inputs[0], &s->textFont);
	MenuInputWidget_SetFont(&s->inputs[1], &s->textFont);
	MenuInputWidget_SetFont(&s->inputs[2], &s->textFont);
	MenuInputWidget_SetFont(&s->inputs[3], &s->textFont);

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

static void GenLevelScreen_Update(void* screen, double delta) {
	struct GenLevelScreen* s = (struct GenLevelScreen*)screen;
	if (s->selected) s->selected->base.caretAccumulator += delta;
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
	s->widgets     = gen_widgets;
	s->numWidgets  = Array_Elems(gen_widgets);
	s->selected    = NULL;
	s->maxVertices = GEN_MAX_VERTICES;

	GenLevelScreen_Make(s, 0, World.Width);
	GenLevelScreen_Make(s, 1, World.Height);
	GenLevelScreen_Make(s, 2, World.Length);
	GenLevelScreen_Make(s, 3, 0);

	TextWidget_Init(&s->title);
	ButtonWidget_Init(&s->flatgrass, 200, GenLevelScreen_Flatgrass);
	ButtonWidget_Init(&s->vanilla,   200, GenLevelScreen_Notchy);
	Menu_InitBack(&s->cancel, Menu_SwitchPause);
	Window_OpenKeyboard(&String_Empty, KEYBOARD_TYPE_NUMBER);
}

static const struct ScreenVTABLE GenLevelScreen_VTABLE = {
	GenLevelScreen_Init,        GenLevelScreen_Update, Menu_CloseKeyboard,
	MenuScreen_Render2,         Screen_BuildMesh,
	GenLevelScreen_KeyDown,     Screen_TInput,     GenLevelScreen_KeyPress, GenLevelScreen_TextChanged,
	GenLevelScreen_PointerDown, Screen_TPointer,   Menu_PointerMove,        Screen_TMouseScroll,
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
} ClassicGenScreen;

static struct Widget* classicgen_widgets[4] = {
	(struct Widget*)&ClassicGenScreen.btns[0], (struct Widget*)&ClassicGenScreen.btns[1],
	(struct Widget*)&ClassicGenScreen.btns[2], (struct Widget*)&ClassicGenScreen.cancel
};
#define CLASSICGEN_MAX_VERTICES (4 * BUTTONWIDGET_MAX)

static void ClassicGenScreen_Gen(int size) {
	RNGState rnd; Random_SeedFromCurrentTime(&rnd);
	Gen_Vanilla = true;
	Gen_Seed    = Random_Next(&rnd, Int32_MaxValue);

	Gui_Remove((struct Screen*)&ClassicGenScreen);
	Menu_BeginGen(size, 64, size);
}

static void ClassicGenScreen_Small(void* a, void* b)  { ClassicGenScreen_Gen(128); }
static void ClassicGenScreen_Medium(void* a, void* b) { ClassicGenScreen_Gen(256); }
static void ClassicGenScreen_Huge(void* a, void* b)   { ClassicGenScreen_Gen(512); }

static void ClassicGenScreen_ContextRecreated(void* screen) {
	struct ClassicGenScreen* s = (struct ClassicGenScreen*)screen;
	struct FontDesc titleFont;
	Screen_CreateVb(screen);

	Menu_MakeTitleFont(&titleFont);
	ButtonWidget_SetConst(&s->btns[0], "Small",  &titleFont);
	ButtonWidget_SetConst(&s->btns[1], "Normal", &titleFont);
	ButtonWidget_SetConst(&s->btns[2], "Huge",   &titleFont);
	ButtonWidget_SetConst(&s->cancel,  "Cancel", &titleFont);
	Font_Free(&titleFont);
}

static void ClassicGenScreen_Layout(void* screen) {
	struct ClassicGenScreen* s = (struct ClassicGenScreen*)screen;
	Widget_SetLocation(&s->btns[0], ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -100);
	Widget_SetLocation(&s->btns[1], ANCHOR_CENTRE, ANCHOR_CENTRE, 0,  -50);
	Widget_SetLocation(&s->btns[2], ANCHOR_CENTRE, ANCHOR_CENTRE, 0,    0);
	Menu_LayoutBack(&s->cancel);
}

static void ClassicGenScreen_Init(void* screen) {
	struct ClassicGenScreen* s = (struct ClassicGenScreen*)screen;
	s->widgets     = classicgen_widgets;
	s->numWidgets  = Array_Elems(classicgen_widgets);
	s->maxVertices = CLASSICGEN_MAX_VERTICES;

	ButtonWidget_Init(&s->btns[0], 400, ClassicGenScreen_Small);
	ButtonWidget_Init(&s->btns[1], 400, ClassicGenScreen_Medium);
	ButtonWidget_Init(&s->btns[2], 400, ClassicGenScreen_Huge);
	Menu_InitBack(&s->cancel, Menu_SwitchPause);
}

static const struct ScreenVTABLE ClassicGenScreen_VTABLE = {
	ClassicGenScreen_Init,   Screen_NullUpdate,  Screen_NullFunc,
	MenuScreen_Render2,      Screen_BuildMesh,
	Screen_InputDown,        Screen_TInput,      Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,        Screen_TPointer,    Menu_PointerMove, Screen_TMouseScroll,
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
	struct ButtonWidget save, alt, cancel;
	struct MenuInputWidget input;
	struct TextWidget mcEdit, desc;
} SaveLevelScreen;

static struct Widget* save_widgets[6] = {
	(struct Widget*)&SaveLevelScreen.save,   (struct Widget*)&SaveLevelScreen.alt,
	(struct Widget*)&SaveLevelScreen.mcEdit, (struct Widget*)&SaveLevelScreen.cancel,
	(struct Widget*)&SaveLevelScreen.input,  (struct Widget*)&SaveLevelScreen.desc,
};
#define SAVE_MAX_VERTICES (3 * BUTTONWIDGET_MAX + MENUINPUTWIDGET_MAX + 2 * TEXTWIDGET_MAX)

static void SaveLevelScreen_UpdateSave(struct SaveLevelScreen* s) {
	ButtonWidget_SetConst(&s->save, 
		s->save.optName ? "&cOverwrite existing?" : "Save", &s->titleFont);
}

static void SaveLevelScreen_UpdateAlt(struct SaveLevelScreen* s) {
#ifdef CC_BUILD_WEB
	ButtonWidget_SetConst(&s->alt, "Download (WIP)", &s->titleFont);
#else
	ButtonWidget_SetConst(&s->alt,
		s->alt.optName ? "&cOverwrite existing?" : "Save schematic", &s->titleFont);
#endif
}

static void SaveLevelScreen_RemoveOverwrites(struct SaveLevelScreen* s) {
	if (s->save.optName) {
		s->save.optName = NULL;
		SaveLevelScreen_UpdateSave(s);
	}
	if (s->alt.optName) {
		s->alt.optName = NULL;
		SaveLevelScreen_UpdateAlt(s);
	}
}

#ifdef CC_BUILD_WEB
#include <emscripten.h>
extern int unlink(const char* path);

static void DownloadMap(const cc_string* path) {
	struct Stream s;
	cc_string file;
	char str[NATIVE_STR_LEN];
	cc_uint8* ptr = NULL;
	cc_uint32 len;

	if (Stream_OpenFile(&s, path))         return;
	if (File_Length(s.Meta.File, &len))    goto finished;
	ptr = Mem_TryAlloc(len, 1);
	if (!ptr || Stream_Read(&s, ptr, len)) goto finished;

	/* maps/aaa.schematic -> aaa.cw */
	file = String_UNSAFE_SubstringAt(path, 5);
	file.length = String_LastIndexOf(&file, '.');
	String_AppendConst(&file, ".cw");
	Platform_ConvertString(str, &file);

	EM_ASM_({
		var data = HEAPU8.subarray($1, $1 + $2);
		var blob = new Blob([data], { type: 'application/octet-stream' });
		var name = UTF8ToString($0);
		Module.saveBlob(blob, name);
	}, str, ptr, len);

	Chat_Add1("&eDownloaded map: %s", &file);
finished:
	s.Close(&s);
	/* TODO: Don't free ptr until download is saved?? */
	/* TODO: Make save map dialog prettier */
	Mem_Free(ptr);

	/* Cleanup the schematic file left behind */
	Platform_ConvertString(str, path);
	/* TODO: This doesn't seem to work properly */
	unlink(str);
}
#endif

static void SaveLevelScreen_SaveMap(struct SaveLevelScreen* s, const cc_string* path) {
	static const cc_string cw = String_FromConst(".cw");
	struct Stream stream, compStream;
	struct GZipState state;
	cc_result res;

	res = Stream_CreateFile(&stream, path);
	if (res) { Logger_Warn2(res, "creating", path); return; }
	GZip_MakeStream(&compStream, &state, &stream);

#ifdef CC_BUILD_WEB
	res = Cw_Save(&compStream);
#else
	if (String_CaselessEnds(path, &cw)) {
		res = Cw_Save(&compStream);
	} else {
		res = Schematic_Save(&compStream);
	}
#endif

	if (res) {
		stream.Close(&stream);
		Logger_Warn2(res, "encoding", path); return;
	}

	if ((res = compStream.Close(&compStream))) {
		stream.Close(&stream);
		Logger_Warn2(res, "closing", path); return;
	}

	res = stream.Close(&stream);
	if (res) { Logger_Warn2(res, "closing", path); return; }

#ifdef CC_BUILD_WEB
	if (String_CaselessEnds(path, &cw)) {
		Chat_Add1("&eSaved map to: %s", path);
	} else {
		DownloadMap(path);
	}
#else
	Chat_Add1("&eSaved map to: %s", path);
#endif
	PauseScreen_Show();
}

static void SaveLevelScreen_Save(void* screen, void* widget, const char* ext) {
	cc_string path; char pathBuffer[FILENAME_SIZE];

	struct SaveLevelScreen* s = (struct SaveLevelScreen*)screen;
	struct ButtonWidget* btn  = (struct ButtonWidget*)widget;
	cc_string file = s->input.base.text;

	if (!file.length) {
		TextWidget_SetConst(&s->desc, "&ePlease enter a filename", &s->textFont);
		return;
	}
	String_InitArray(path, pathBuffer);
	String_Format2(&path, "maps/%s%c", &file, ext);

	if (File_Exists(&path) && !btn->optName) {
		btn->optName = "";
		SaveLevelScreen_UpdateSave(s);
		SaveLevelScreen_UpdateAlt(s);
	} else {
		SaveLevelScreen_RemoveOverwrites(s);
		SaveLevelScreen_SaveMap(s, &path);
	}
}
static void SaveLevelScreen_Main(void* a, void* b) { SaveLevelScreen_Save(a, b, ".cw"); }
static void SaveLevelScreen_Alt(void* a, void* b)  { SaveLevelScreen_Save(a, b, ".schematic"); }

static void SaveLevelScreen_Render(void* screen, double delta) {
	PackedCol grey = PackedCol_Make(150, 150, 150, 255);
	int x, y;
	MenuScreen_Render2(screen, delta);

#ifndef CC_BUILD_WEB
	x = WindowInfo.Width / 2; y = WindowInfo.Height / 2;
	Gfx_Draw2DFlat(x - 250, y + 90, 500, 2, grey);
#endif
}

static int SaveLevelScreen_KeyPress(void* screen, char keyChar) {
	struct SaveLevelScreen* s = (struct SaveLevelScreen*)screen;
	SaveLevelScreen_RemoveOverwrites(s);
	InputWidget_Append(&s->input.base, keyChar);
	return true;
}

static int SaveLevelScreen_TextChanged(void* screen, const cc_string* str) {
#ifdef CC_BUILD_TOUCH
	struct SaveLevelScreen* s = (struct SaveLevelScreen*)screen;
	SaveLevelScreen_RemoveOverwrites(s);
	InputWidget_SetText(&s->input.base, str);
#endif
	return true;
}

static int SaveLevelScreen_KeyDown(void* screen, int key) {
	struct SaveLevelScreen* s = (struct SaveLevelScreen*)screen;
	if (Elem_HandlesKeyDown(&s->input.base, key)) {
		SaveLevelScreen_RemoveOverwrites(s);
		return true;
	}
	return Screen_InputDown(s, key);
}

static void SaveLevelScreen_ContextLost(void* screen) {
	struct SaveLevelScreen* s = (struct SaveLevelScreen*)screen;
	Font_Free(&s->titleFont);
	Font_Free(&s->textFont);
	Screen_ContextLost(screen);
}

static void SaveLevelScreen_ContextRecreated(void* screen) {
	struct SaveLevelScreen* s = (struct SaveLevelScreen*)screen;
	Menu_MakeTitleFont(&s->titleFont);
	Menu_MakeBodyFont(&s->textFont);

	Screen_CreateVb(screen);
	SaveLevelScreen_UpdateSave(s);
	SaveLevelScreen_UpdateAlt(s);

#ifndef CC_BUILD_WEB
	TextWidget_SetConst(&s->mcEdit,   "&eCan be imported into MCEdit", &s->textFont);
#endif
	MenuInputWidget_SetFont(&s->input, &s->textFont);
	ButtonWidget_SetConst(&s->cancel, "Cancel",                        &s->titleFont);
}

static void SaveLevelScreen_Update(void* screen, double delta) {
	struct SaveLevelScreen* s = (struct SaveLevelScreen*)screen;
	s->input.base.caretAccumulator += delta;
}

static void SaveLevelScreen_Layout(void* screen) {
	struct SaveLevelScreen* s = (struct SaveLevelScreen*)screen;
	Widget_SetLocation(&s->save,   ANCHOR_CENTRE, ANCHOR_CENTRE,    0,  20);
#ifdef CC_BUILD_WEB
	Widget_SetLocation(&s->alt,    ANCHOR_CENTRE, ANCHOR_CENTRE,    0,  70);
#else
	Widget_SetLocation(&s->alt,    ANCHOR_CENTRE, ANCHOR_CENTRE, -150, 120);
	Widget_SetLocation(&s->mcEdit, ANCHOR_CENTRE, ANCHOR_CENTRE,  110, 120);
#endif

	Menu_LayoutBack(&s->cancel);
	Widget_SetLocation(&s->input,  ANCHOR_CENTRE, ANCHOR_CENTRE,    0, -30);
#ifdef CC_BUILD_WEB
	Widget_SetLocation(&s->desc,   ANCHOR_CENTRE, ANCHOR_CENTRE,    0, 115);
#else
	Widget_SetLocation(&s->desc,   ANCHOR_CENTRE, ANCHOR_CENTRE,    0,  65);
#endif
}

static void SaveLevelScreen_Init(void* screen) {
	struct SaveLevelScreen* s = (struct SaveLevelScreen*)screen;
	struct MenuInputDesc desc;
	
	s->widgets     = save_widgets;
	s->numWidgets  = Array_Elems(save_widgets);
	s->maxVertices = SAVE_MAX_VERTICES;
	MenuInput_Path(desc);
	
	ButtonWidget_Init(&s->save, 300, SaveLevelScreen_Main);
#ifdef CC_BUILD_WEB
	ButtonWidget_Init(&s->alt,  300, SaveLevelScreen_Alt);
	s->widgets[2] = NULL; /* null mcEdit widget */
#else
	ButtonWidget_Init(&s->alt,  200, SaveLevelScreen_Alt);
	TextWidget_Init(&s->mcEdit);
#endif

	Menu_InitBack(&s->cancel, Menu_SwitchPause);
	MenuInputWidget_Create(&s->input, 500, &String_Empty, &desc);
	TextWidget_Init(&s->desc);
	Window_OpenKeyboard(&String_Empty, KEYBOARD_TYPE_TEXT);
}

static const struct ScreenVTABLE SaveLevelScreen_VTABLE = {
	SaveLevelScreen_Init,    SaveLevelScreen_Update, Menu_CloseKeyboard,
	SaveLevelScreen_Render,  Screen_BuildMesh,
	SaveLevelScreen_KeyDown, Screen_TInput,    SaveLevelScreen_KeyPress, SaveLevelScreen_TextChanged,
	Menu_PointerDown,        Screen_TPointer,  Menu_PointerMove,         Screen_TMouseScroll,
	SaveLevelScreen_Layout,  SaveLevelScreen_ContextLost, SaveLevelScreen_ContextRecreated
};
void SaveLevelScreen_Show(void) {
	struct SaveLevelScreen* s = &SaveLevelScreen;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE = &SaveLevelScreen_VTABLE;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_MENU);
}


/*########################################################################################################################*
*---------------------------------------------------TexturePackScreen-----------------------------------------------------*
*#########################################################################################################################*/
static void TexturePackScreen_EntryClick(void* screen, void* widget) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	struct ListScreen* s = (struct ListScreen*)screen;
	cc_string relPath;
	
	relPath = ListScreen_UNSAFE_GetCur(s, widget);
	String_InitArray(path, pathBuffer);
	String_Format1(&path, "texpacks/%s", &relPath);
	if (!File_Exists(&path)) return;
	
	TexturePack_SetDefault(&relPath);
	TexturePack_Url.length = 0;
	TexturePack_ExtractCurrent(true);
}

static void TexturePackScreen_FilterFiles(const cc_string* path, void* obj) {
	static const cc_string zip = String_FromConst(".zip");
	cc_string relPath = *path;
	if (!String_CaselessEnds(path, &zip)) return;

	Utils_UNSAFE_TrimFirstDirectory(&relPath);
	StringsBuffer_Add((struct StringsBuffer*)obj, &relPath);
}

static void TexturePackScreen_LoadEntries(struct ListScreen* s) {
	static const cc_string path = String_FromConst("texpacks");
	Directory_Enum(&path, &s->entries, TexturePackScreen_FilterFiles);
	ListScreen_Sort(s);
}

void TexturePackScreen_Show(void) {
	struct ListScreen* s = &ListScreen;
	s->titleText   = "Select a texture pack zip";
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
	cc_string fontName = ListScreen_UNSAFE_GetCur(s, widget);

	if (String_CaselessEqualsConst(&fontName, LISTSCREEN_EMPTY)) return;
	String_Copy(&Drawer2D_FontName, &fontName);
	Options_Set(OPT_FONT_NAME,      &fontName);
	Event_RaiseVoid(&ChatEvents.FontChanged);
}

static void FontListScreen_UpdateEntry(struct ListScreen* s, struct ButtonWidget* button, const cc_string* text) {
	struct FontDesc font;
	cc_result res;

	if (String_CaselessEqualsConst(text, LISTSCREEN_EMPTY)) {
		ButtonWidget_Set(button, text, &s->font); return;
	}
	res = Font_Make(&font, text, 16, FONT_FLAGS_NONE);

	if (!res) {
		ButtonWidget_Set(button, text, &font);
	} else {
		Logger_SimpleWarn2(res, "making font", text);
		ButtonWidget_Set(button, text, &s->font);
	}
	Font_Free(&font);
}

static void FontListScreen_LoadEntries(struct ListScreen* s) {
	Font_GetNames(&s->entries);
	ListScreen_Sort(s);
	ListScreen_Select(s, &Drawer2D_FontName);
}

void FontListScreen_Show(void) {
	struct ListScreen* s = &ListScreen;
	s->titleText   = "Select a font";
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
	int i, flags = 0;

	text = ListScreen_UNSAFE_GetCur(s, widget);
	if (String_CaselessEqualsConst(&text, LISTSCREEN_EMPTY)) {
		EditHotkeyScreen_Show(original); 
		return;
	}

	String_UNSAFE_Separate(&text, '+', &key, &value);
	if (String_ContainsConst(&value, "Ctrl"))  flags |= HOTKEY_MOD_CTRL;
	if (String_ContainsConst(&value, "Shift")) flags |= HOTKEY_MOD_SHIFT;
	if (String_ContainsConst(&value, "Alt"))   flags |= HOTKEY_MOD_ALT;

	trigger = Utils_ParseEnum(&key, KEY_NONE, Input_Names, INPUT_COUNT);
	for (i = 0; i < HotkeysText.count; i++) {
		h = HotkeysList[i];
		if (h.Trigger == trigger && h.Flags == flags) { original = h; break; }
	}

	EditHotkeyScreen_Show(original);
}

static void HotkeyListScreen_MakeFlags(int flags, cc_string* str) {
	if (flags & HOTKEY_MOD_CTRL)  String_AppendConst(str, " Ctrl");
	if (flags & HOTKEY_MOD_SHIFT) String_AppendConst(str, " Shift");
	if (flags & HOTKEY_MOD_ALT)   String_AppendConst(str, " Alt");
}

static void HotkeyListScreen_LoadEntries(struct ListScreen* s) {
	static const cc_string empty = String_FromConst(LISTSCREEN_EMPTY);
	cc_string text; char textBuffer[STRING_SIZE];
	struct HotkeyData hKey;
	int i;
	String_InitArray(text, textBuffer);

	for (i = 0; i < HotkeysText.count; i++) {
		hKey = HotkeysList[i];
		text.length = 0;
		String_AppendConst(&text, Input_Names[hKey.Trigger]);

		if (hKey.Flags) {
			String_AppendConst(&text, " +");
			HotkeyListScreen_MakeFlags(hKey.Flags, &text);
		}
		StringsBuffer_Add(&s->entries, &text);
	}

	for (i = 0; i < LIST_SCREEN_ITEMS; i++) {
		StringsBuffer_Add(&s->entries, &empty);
	}
}

void HotkeyListScreen_Show(void) {
	struct ListScreen* s = &ListScreen;
	s->titleText   = "Modify hotkeys";
	s->LoadEntries = HotkeyListScreen_LoadEntries;
	s->EntryClick  = HotkeyListScreen_EntryClick;
	s->DoneClick   = Menu_SwitchPause;
	s->UpdateEntry = ListScreen_UpdateEntry;
	ListScreen_Show();
}


/*########################################################################################################################*
*----------------------------------------------------LoadLevelScreen------------------------------------------------------*
*#########################################################################################################################*/
static void LoadLevelScreen_EntryClick(void* screen, void* widget) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	struct ListScreen* s = (struct ListScreen*)screen;
	cc_string relPath;

	relPath = ListScreen_UNSAFE_GetCur(s, widget);
	if (String_CaselessEqualsConst(&relPath, LISTSCREEN_EMPTY)) return;

	String_InitArray(path, pathBuffer);
	String_Format1(&path, "maps/%s", &relPath);
	Map_LoadFrom(&path);
}

static void LoadLevelScreen_FilterFiles(const cc_string* path, void* obj) {
	IMapImporter importer = Map_FindImporter(path);
	cc_string relPath = *path;
	if (!importer) return;

	Utils_UNSAFE_TrimFirstDirectory(&relPath);
	StringsBuffer_Add((struct StringsBuffer*)obj, &relPath);
}

static void LoadLevelScreen_LoadEntries(struct ListScreen* s) {
	static const cc_string path = String_FromConst("maps");
	Directory_Enum(&path, &s->entries, LoadLevelScreen_FilterFiles);
	ListScreen_Sort(s);
}

void LoadLevelScreen_Show(void) {
	struct ListScreen* s = &ListScreen;
	s->titleText   = "Select a level";
	s->LoadEntries = LoadLevelScreen_LoadEntries;
	s->EntryClick  = LoadLevelScreen_EntryClick;
	s->DoneClick   = Menu_SwitchPause;
	s->UpdateEntry = ListScreen_UpdateEntry;
	ListScreen_Show();
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
#define KEYBINDS_MAX_VERTICES ((KEYBINDS_MAX_BTNS + 3) * BUTTONWIDGET_MAX + 2 * TEXTWIDGET_MAX)

static struct Widget* key_widgets[KEYBINDS_MAX_BTNS + 5] = {
	NULL,NULL,NULL,NULL,NULL,NULL,                  NULL,NULL,NULL,NULL,NULL,NULL,
	(struct Widget*)&KeyBindsScreen.title, (struct Widget*)&KeyBindsScreen.msg,
	(struct Widget*)&KeyBindsScreen.back,  (struct Widget*)&KeyBindsScreen.left,
	(struct Widget*)&KeyBindsScreen.right
};

static void KeyBindsScreen_Update(struct KeyBindsScreen* s, int i) {
	cc_string text; char textBuffer[STRING_SIZE];
	String_InitArray(text, textBuffer);

	String_Format2(&text, s->curI == i ? "> %c: %c <" : "%c: %c", 
		s->descs[i], Input_Names[KeyBinds[s->binds[i]]]);
	ButtonWidget_Set(&s->buttons[i], &text, &s->titleFont);
	s->dirty = true;
}

static void KeyBindsScreen_OnBindingClick(void* screen, void* widget) {
	struct KeyBindsScreen* s = (struct KeyBindsScreen*)screen;
	int old     = s->curI;
	s->curI     = Menu_Index(s, widget);
	s->closable = false;

	KeyBindsScreen_Update(s, s->curI);
	/* previously selected a different button for binding */
	if (old >= 0) KeyBindsScreen_Update(s, old);
}

static int KeyBindsScreen_KeyDown(void* screen, int key) {
	struct KeyBindsScreen* s = (struct KeyBindsScreen*)screen;
	KeyBind bind;
	int idx;

	if (s->curI == -1) return Screen_InputDown(s, key);
	bind = s->binds[s->curI];
	if (key == KEY_ESCAPE) key = KeyBind_Defaults[bind];
	KeyBind_Set(bind, key);

	idx         = s->curI;
	s->curI     = -1;
	s->closable = true;
	KeyBindsScreen_Update(s, idx);
	return true;
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

	Screen_CreateVb(screen);
	Menu_MakeTitleFont(&s->titleFont);
	Menu_MakeBodyFont(&textFont);
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
	s->numWidgets  = KEYBINDS_MAX_BTNS + 3;
	s->curI        = -1;
	s->maxVertices = KEYBINDS_MAX_VERTICES;

	for (i = 0; i < s->bindsCount; i++) {
		ButtonWidget_Init(&s->buttons[i], s->btnWidth, KeyBindsScreen_OnBindingClick);
		s->widgets[i] = (struct Widget*)&s->buttons[i];
	}
	for (; i < KEYBINDS_MAX_BTNS; i++) { s->widgets[i] = NULL; }

	TextWidget_Init(&s->title);
	TextWidget_Init(&s->msg);
	Menu_InitBack(&s->back, Gui.ClassicMenu ? Menu_SwitchClassicOptions : Menu_SwitchOptions); 

	ButtonWidget_Init(&s->left,  40, s->leftPage);
	ButtonWidget_Init(&s->right, 40, s->rightPage);
	s->left.disabled  = !s->leftPage;
	s->right.disabled = !s->rightPage;

	if (!s->leftPage && !s->rightPage) return;
	s->numWidgets += 2;
}

static const struct ScreenVTABLE KeyBindsScreen_VTABLE = {
	KeyBindsScreen_Init,    Screen_NullUpdate, Screen_NullFunc,  
	MenuScreen_Render2,     Screen_BuildMesh,
	KeyBindsScreen_KeyDown, Screen_TInput,     Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,       Screen_TPointer,   Menu_PointerMove, Screen_TMouseScroll,
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
*-----------------------------------------------ClassicKeyBindsScreen--------------------------------------------------*
*#########################################################################################################################*/
void ClassicKeyBindingsScreen_Show(void) {
	static const cc_uint8 binds[10]    = { KEYBIND_FORWARD, KEYBIND_BACK, KEYBIND_JUMP, KEYBIND_CHAT, KEYBIND_SET_SPAWN, KEYBIND_LEFT, KEYBIND_RIGHT, KEYBIND_INVENTORY, KEYBIND_FOG, KEYBIND_RESPAWN };
	static const char* const descs[10] = { "Forward", "Back", "Jump", "Chat", "Save loc", "Left", "Right", "Build", "Toggle fog", "Load loc" };
	
	if (Game_ClassicHacks) {
		KeyBindsScreen_Reset(NULL, Menu_SwitchKeysClassicHacks, 260);
	} else {
		KeyBindsScreen_Reset(NULL,                        NULL, 300);
	}
	KeyBindsScreen_SetLayout(-140, -40, 5);
	KeyBindsScreen_Show(Array_Elems(binds), binds, descs, 
						Game_ClassicHacks ? "Normal controls" : "Controls");
}


/*########################################################################################################################*
*--------------------------------------------ClassicHacksKeyBindingsScreen------------------------------------------------*
*#########################################################################################################################*/
void ClassicHacksKeyBindingsScreen_Show(void) {
	static const cc_uint8 binds[6]    = { KEYBIND_SPEED, KEYBIND_NOCLIP, KEYBIND_HALF_SPEED, KEYBIND_FLY, KEYBIND_FLY_UP, KEYBIND_FLY_DOWN };
	static const char* const descs[6] = { "Speed", "Noclip", "Half speed", "Fly", "Fly up", "Fly down" };

	KeyBindsScreen_Reset(Menu_SwitchKeysClassic, NULL, 260);
	KeyBindsScreen_SetLayout(-90, -40, 3);
	KeyBindsScreen_Show(Array_Elems(binds), binds, descs, "Hacks controls");
}


/*########################################################################################################################*
*-----------------------------------------------NormalKeyBindingsScreen---------------------------------------------------*
*#########################################################################################################################*/
void NormalKeyBindingsScreen_Show(void) {
	static const cc_uint8 binds[12]    = { KEYBIND_FORWARD, KEYBIND_BACK, KEYBIND_JUMP, KEYBIND_CHAT, KEYBIND_SET_SPAWN, KEYBIND_PLAYER_LIST, KEYBIND_LEFT, KEYBIND_RIGHT, KEYBIND_INVENTORY, KEYBIND_FOG, KEYBIND_RESPAWN, KEYBIND_SEND_CHAT };
	static const char* const descs[12] = { "Forward", "Back", "Jump", "Chat", "Set spawn", "Player list", "Left", "Right", "Inventory", "Toggle fog", "Respawn", "Send chat" };
	
	KeyBindsScreen_Reset(NULL, Menu_SwitchKeysHacks, 250);
	KeyBindsScreen_SetLayout(-140, 10, 6);
	KeyBindsScreen_Show(Array_Elems(binds), binds, descs, "Normal controls");
}


/*########################################################################################################################*
*------------------------------------------------HacksKeyBindingsScreen---------------------------------------------------*
*#########################################################################################################################*/
void HacksKeyBindingsScreen_Show(void) {
	static const cc_uint8 binds[8]    = { KEYBIND_SPEED, KEYBIND_NOCLIP, KEYBIND_HALF_SPEED, KEYBIND_ZOOM_SCROLL, KEYBIND_FLY, KEYBIND_FLY_UP, KEYBIND_FLY_DOWN, KEYBIND_THIRD_PERSON };
	static const char* const descs[8] = { "Speed", "Noclip", "Half speed", "Scroll zoom", "Fly", "Fly up", "Fly down", "Third person" };
	
	KeyBindsScreen_Reset(Menu_SwitchKeysNormal, Menu_SwitchKeysOther, 260);
	KeyBindsScreen_SetLayout(-40, 10, 4);
	KeyBindsScreen_Show(Array_Elems(binds), binds, descs, "Hacks controls");
}


/*########################################################################################################################*
*------------------------------------------------OtherKeyBindingsScreen---------------------------------------------------*
*#########################################################################################################################*/
void OtherKeyBindingsScreen_Show(void) {
	static const cc_uint8 binds[12]     = { KEYBIND_EXT_INPUT, KEYBIND_HIDE_FPS, KEYBIND_HIDE_GUI, KEYBIND_HOTBAR_SWITCH, KEYBIND_DROP_BLOCK,KEYBIND_SCREENSHOT, KEYBIND_FULLSCREEN, KEYBIND_AXIS_LINES, KEYBIND_AUTOROTATE, KEYBIND_SMOOTH_CAMERA, KEYBIND_IDOVERLAY, KEYBIND_BREAK_LIQUIDS };
	static const char* const descs[12]  = { "Show ext input", "Hide FPS", "Hide gui", "Hotbar switching", "Drop block", "Screenshot", "Fullscreen", "Show axis lines", "Auto-rotate", "Smooth camera", "ID overlay", "Breakable liquids" };
	
	KeyBindsScreen_Reset(Menu_SwitchKeysHacks, Menu_SwitchKeysMouse, 260);
	KeyBindsScreen_SetLayout(-140, 10, 6);
	KeyBindsScreen_Show(Array_Elems(binds), binds, descs, "Other controls");
}


/*########################################################################################################################*
*------------------------------------------------MouseKeyBindingsScreen---------------------------------------------------*
*#########################################################################################################################*/
void MouseKeyBindingsScreen_Show(void) {
	static const cc_uint8 binds[3]    = { KEYBIND_DELETE_BLOCK, KEYBIND_PICK_BLOCK, KEYBIND_PLACE_BLOCK };
	static const char* const descs[3] = { "Delete block", "Pick block", "Place block" };

	KeyBindsScreen_Reset(Menu_SwitchKeysOther, NULL, 260);
	KeyBindsScreen_SetLayout(-40, 10, -1);
	KeyBindsScreen.msgText = "&ePress escape to reset the binding";
	KeyBindsScreen_Show(Array_Elems(binds), binds, descs, "Mouse key bindings");
}


/*########################################################################################################################*
*--------------------------------------------------MenuOptionsScreen------------------------------------------------------*
*#########################################################################################################################*/
struct MenuOptionsScreen;
typedef void (*InitMenuOptions)(struct MenuOptionsScreen* s);
#define MENUOPTS_MAX_OPTS 10
static void MenuOptionsScreen_Layout(void* screen);

static struct MenuOptionsScreen {
	Screen_Body
	struct MenuInputDesc* descs;
	const char** descriptions;
	int activeI, selectedI, descriptionsCount;
	InitMenuOptions DoInit, DoRecreateExtra, OnHacksChanged;
	int numButtons, numCore;
	struct FontDesc titleFont, textFont;
	struct ButtonWidget ok, Default;
	struct MenuInputWidget input;
	struct TextGroupWidget extHelp;
	struct Texture extHelpTextures[5]; /* max lines is 5 */
	struct ButtonWidget buttons[MENUOPTS_MAX_OPTS], done;
	const char* extHelpDesc;
} MenuOptionsScreen_Instance;

static struct Widget* menuOpts_widgets[MENUOPTS_MAX_OPTS + 4] = {
	NULL,NULL,NULL,NULL,NULL,  NULL,NULL,NULL,NULL,NULL,
	(struct Widget*)&MenuOptionsScreen_Instance.done,    (struct Widget*)&MenuOptionsScreen_Instance.ok,
	(struct Widget*)&MenuOptionsScreen_Instance.Default, (struct Widget*)&MenuOptionsScreen_Instance.input
};

static void Menu_GetBool(cc_string* raw, cc_bool v) {
	String_AppendConst(raw, v ? "ON" : "OFF");
}
static cc_bool Menu_SetBool(const cc_string* raw, const char* key) {
	cc_bool isOn = String_CaselessEqualsConst(raw, "ON");
	Options_SetBool(key, isOn); 
	return isOn;
}

static void MenuOptionsScreen_GetFPS(cc_string* raw) {
	String_AppendConst(raw, FpsLimit_Names[Game_FpsLimit]);
}
static void MenuOptionsScreen_SetFPS(const cc_string* v) {
	int method = Utils_ParseEnum(v, FPS_LIMIT_VSYNC, FpsLimit_Names, Array_Elems(FpsLimit_Names));
	Options_Set(OPT_FPS_LIMIT, v);
	Game_SetFpsLimit(method);
}

static void MenuOptionsScreen_Update(struct MenuOptionsScreen* s, int i) {
	cc_string title; char titleBuffer[STRING_SIZE];
	String_InitArray(title, titleBuffer);

	String_AppendConst(&title, s->buttons[i].optName);
	if (s->buttons[i].GetValue) {
		String_AppendConst(&title, ": ");
		s->buttons[i].GetValue(&title);
	}	
	ButtonWidget_Set(&s->buttons[i], &title, &s->titleFont);
}

CC_NOINLINE static void MenuOptionsScreen_Set(struct MenuOptionsScreen* s, int i, const cc_string* text) {
	s->buttons[i].SetValue(text);
	MenuOptionsScreen_Update(s, i);
}

CC_NOINLINE static void MenuOptionsScreen_FreeExtHelp(struct MenuOptionsScreen* s) {
	Elem_Free(&s->extHelp);
	s->extHelp.lines = 0;
}

static void MenuOptionsScreen_LayoutExtHelp(struct MenuOptionsScreen* s) {
	Widget_SetLocation(&s->extHelp, ANCHOR_MIN, ANCHOR_CENTRE_MIN, 0, 100);
	/* If use centre align above, then each line in extended help gets */
	/* centered aligned separately - which is not the desired behaviour. */
	s->extHelp.xOffset = WindowInfo.Width / 2 - s->extHelp.width / 2;
	Widget_Layout(&s->extHelp);
}

static cc_string MenuOptionsScreen_GetDesc(int i) {
	const char* desc = MenuOptionsScreen_Instance.extHelpDesc;
	cc_string descRaw, descLines[5];

	descRaw = String_FromReadonly(desc);
	String_UNSAFE_Split(&descRaw, '\n', descLines, Array_Elems(descLines));
	return descLines[i];
}

static void MenuOptionsScreen_SelectExtHelp(struct MenuOptionsScreen* s, int idx) {
	const char* desc;
	cc_string descRaw, descLines[5];

	MenuOptionsScreen_FreeExtHelp(s);
	if (!s->descriptions || s->activeI >= 0) return;
	desc = s->descriptions[idx];
	if (!desc) return;

	descRaw          = String_FromReadonly(desc);
	s->extHelp.lines = String_UNSAFE_Split(&descRaw, '\n', descLines, Array_Elems(descLines));
	
	s->extHelpDesc = desc;
	TextGroupWidget_RedrawAll(&s->extHelp);
	MenuOptionsScreen_LayoutExtHelp(s);
}

static void MenuOptionsScreen_FreeInput(struct MenuOptionsScreen* s) {
	if (s->activeI == -1) return;
	Elem_Free(&s->input.base);
	Elem_Free(&s->ok);
	Elem_Free(&s->Default);

	s->numWidgets = MENUOPTS_MAX_OPTS + 1;
	Window_CloseKeyboard();
}

static void MenuOptionsScreen_RedrawInput(struct MenuOptionsScreen* s) {
	if (s->activeI == -1) return;
	MenuInputWidget_SetFont(&s->input, &s->textFont);
	ButtonWidget_SetConst(&s->ok,      "OK",            &s->titleFont);
	ButtonWidget_SetConst(&s->Default, "Default value", &s->titleFont);
}

static void MenuOptionsScreen_EnterInput(struct MenuOptionsScreen* s) {
	struct MenuInputDesc* desc = &s->input.desc;
	cc_string text = s->input.base.text;

	if (desc->VTABLE->IsValidValue(desc, &text)) {
		MenuOptionsScreen_Set(s, s->activeI, &text);
	}

	MenuOptionsScreen_SelectExtHelp(s, s->activeI);
	MenuOptionsScreen_FreeInput(s);
	s->activeI = -1;
}

static int MenuOptionsScreen_KeyPress(void* screen, char keyChar) {
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	if (s->activeI >= 0) InputWidget_Append(&s->input.base, keyChar);
	return true;
}

static int MenuOptionsScreen_TextChanged(void* screen, const cc_string* str) {
#ifdef CC_BUILD_TOUCH
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	if (s->activeI >= 0) InputWidget_SetText(&s->input.base, str);
#endif
	return true;
}

static int MenuOptionsScreen_KeyDown(void* screen, int key) {
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	if (s->activeI >= 0) {
		if (Elem_HandlesKeyDown(&s->input.base, key)) return true;

		if (key == KEY_ENTER || key == KEY_KP_ENTER) {
			MenuOptionsScreen_EnterInput(s); return true;
		}
	}
	return Screen_InputDown(s, key);
}

static int MenuOptionsScreen_PointerMove(void* screen, int id, int x, int y) {
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	int i = Menu_DoPointerMove(s, id, x, y);
	if (i == -1 || i == s->selectedI) return true;
	if (!s->descriptions || i >= s->descriptionsCount) return true;

	s->selectedI = i;
	if (s->activeI == -1) MenuOptionsScreen_SelectExtHelp(s, i);
	return true;
}

static void MenuOptionsScreen_InitButtons(struct MenuOptionsScreen* s, const struct MenuOptionDesc* btns, int count, Widget_LeftClick backClick) {
	struct ButtonWidget* btn;
	int i;
	
	for (i = 0; i < count; i++) {
		btn = &s->buttons[i];
		ButtonWidget_Make(btn, 300, btns[i].OnClick,
			ANCHOR_CENTRE, ANCHOR_CENTRE, btns[i].dir * 160, btns[i].y);

		btn->optName  = btns[i].name;
		btn->GetValue = btns[i].GetValue;
		btn->SetValue = btns[i].SetValue;
		s->widgets[i] = (struct Widget*)btn;
	}
	s->numButtons = count;
	Menu_InitBack(&s->done, backClick);
}

static void MenuOptionsScreen_OK(void* screen, void* widget) {
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	MenuOptionsScreen_EnterInput(s);
}

static void MenuOptionsScreen_Default(void* screen, void* widget) {
	cc_string value; char valueBuffer[STRING_SIZE];
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	struct MenuInputDesc* desc  = &s->descs[s->activeI];

	String_InitArray(value, valueBuffer);
	desc->VTABLE->GetDefault(desc, &value);
	InputWidget_SetText(&s->input.base, &value);
}

static void MenuOptionsScreen_Bool(void* screen, void* widget) {
	cc_string value; char valueBuffer[STRING_SIZE];
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	struct ButtonWidget* btn    = (struct ButtonWidget*)widget;
	cc_bool isOn;

	String_InitArray(value, valueBuffer);
	btn->GetValue(&value);

	isOn  = String_CaselessEqualsConst(&value, "ON");
	value = String_FromReadonly(isOn ? "OFF" : "ON");
	MenuOptionsScreen_Set(s, Menu_Index(s, btn), &value);
}

static void MenuOptionsScreen_Enum(void* screen, void* widget) {
	cc_string value; char valueBuffer[STRING_SIZE];
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	struct ButtonWidget* btn    = (struct ButtonWidget*)widget;
	int index;
	struct MenuInputDesc* desc;
	const char* const* names;
	int raw, count;
	
	index = Menu_Index(s, btn);
	String_InitArray(value, valueBuffer);
	btn->GetValue(&value);

	desc  = &s->descs[index];
	names = desc->meta.e.Names;
	count = desc->meta.e.Count;	

	raw   = (Utils_ParseEnum(&value, 0, names, count) + 1) % count;
	value = String_FromReadonly(names[raw]);
	MenuOptionsScreen_Set(s, index, &value);
}

static void MenuOptionsScreen_Input(void* screen, void* widget) {
	cc_string value; char valueBuffer[STRING_SIZE];
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	struct ButtonWidget* btn    = (struct ButtonWidget*)widget;
	struct MenuInputDesc* desc;

	MenuOptionsScreen_FreeExtHelp(s);
	MenuOptionsScreen_FreeInput(s);
	s->activeI = Menu_Index(s, btn);

	String_InitArray(value, valueBuffer);
	btn->GetValue(&value);
	desc = &s->descs[s->activeI];

	MenuInputWidget_Create(&s->input, 400, &value, desc);
	ButtonWidget_Init(&s->ok,          40, MenuOptionsScreen_OK);
	ButtonWidget_Init(&s->Default,    200, MenuOptionsScreen_Default);

	s->numWidgets = MENUOPTS_MAX_OPTS + 1 + 3;
	MenuOptionsScreen_Layout(screen);
	MenuOptionsScreen_RedrawInput(s);

	Window_OpenKeyboard(&value,
		(desc->VTABLE == &IntInput_VTABLE || desc->VTABLE == &FloatInput_VTABLE)
		? KEYBOARD_TYPE_NUMBER : KEYBOARD_TYPE_TEXT);
}

static void MenuOptionsScreen_OnHacksChanged(void* screen) {
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	if (s->OnHacksChanged) s->OnHacksChanged(s);
}

static void MenuOptionsScreen_Init(void* screen) {
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	int i;

	s->widgets    = menuOpts_widgets;
	s->numWidgets = MENUOPTS_MAX_OPTS + 1; /* always have back button */
	/* The various menu options screens might have different number of widgets */
	for (i = 0; i < MENUOPTS_MAX_OPTS; i++) { s->widgets[i] = NULL; }

	s->activeI    = -1;
	s->selectedI  = -1;
	s->DoInit(s);

	TextGroupWidget_Create(&s->extHelp, 5, s->extHelpTextures, MenuOptionsScreen_GetDesc);
	s->extHelp.lines = 0;
	Event_Register_(&UserEvents.HackPermissionsChanged, screen, MenuOptionsScreen_OnHacksChanged);
}
	
#define EXTHELP_PAD 5 /* padding around extended help box */
static void MenuOptionsScreen_Render(void* screen, double delta) {
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	struct TextGroupWidget* w;
	PackedCol tableCol = PackedCol_Make(20, 20, 20, 200);

	MenuScreen_Render(s, delta);
	if (!s->extHelp.lines) return;

	w = &s->extHelp;
	Gfx_Draw2DFlat(w->x - EXTHELP_PAD, w->y - EXTHELP_PAD, 
		w->width + EXTHELP_PAD * 2, w->height + EXTHELP_PAD * 2, tableCol);

	Gfx_SetTexturing(true);
	Elem_Render(&s->extHelp, delta);
	Gfx_SetTexturing(false);
}

static void MenuOptionsScreen_Free(void* screen) {
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	Event_Unregister_(&UserEvents.HackPermissionsChanged, screen, MenuOptionsScreen_OnHacksChanged);
	if (s->activeI >= 0) Window_CloseKeyboard();
}

static void MenuOptionsScreen_Layout(void* screen) {
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	Screen_Layout(s);
	Menu_LayoutBack(&s->done);
	MenuOptionsScreen_LayoutExtHelp(s);

	if (s->activeI == -1) return;
	Widget_SetLocation(&s->input,   ANCHOR_CENTRE, ANCHOR_CENTRE,   0, 110);
	Widget_SetLocation(&s->ok,      ANCHOR_CENTRE, ANCHOR_CENTRE, 240, 110);
	Widget_SetLocation(&s->Default, ANCHOR_CENTRE, ANCHOR_CENTRE,   0, 150);
}

static void MenuOptionsScreen_ContextLost(void* screen) {
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	Font_Free(&s->titleFont);
	Font_Free(&s->textFont);
	Screen_ContextLost(s);
	Elem_Free(&s->extHelp);
}

static void MenuOptionsScreen_ContextRecreated(void* screen) {
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	int i;
	Menu_MakeTitleFont(&s->titleFont);
	Menu_MakeBodyFont(&s->textFont);

	for (i = 0; i < s->numButtons; i++) { 
		if (s->widgets[i]) MenuOptionsScreen_Update(s, i); 
	}

	ButtonWidget_SetConst(&s->done, "Done", &s->titleFont);
	if (s->DoRecreateExtra) s->DoRecreateExtra(s);
	TextGroupWidget_SetFont(&s->extHelp, &s->textFont);
	TextGroupWidget_RedrawAll(&s->extHelp); /* TODO: SetFont should redrawall implicitly */
	MenuOptionsScreen_RedrawInput(s);
}

static void MenuOptionsScreen_BuildMesh(void* screen) { }

static const struct ScreenVTABLE MenuOptionsScreen_VTABLE = {
	MenuOptionsScreen_Init,    Screen_NullUpdate, MenuOptionsScreen_Free, 
	MenuOptionsScreen_Render,  MenuOptionsScreen_BuildMesh,
	MenuOptionsScreen_KeyDown, Screen_TInput,     MenuOptionsScreen_KeyPress,    MenuOptionsScreen_TextChanged,
	Menu_PointerDown,          Screen_TPointer,   MenuOptionsScreen_PointerMove, Screen_TMouseScroll,
	MenuOptionsScreen_Layout, MenuOptionsScreen_ContextLost, MenuOptionsScreen_ContextRecreated
};
void MenuOptionsScreen_Show(struct MenuInputDesc* descs, const char** descriptions, int descsCount, InitMenuOptions init) {
	struct MenuOptionsScreen* s = &MenuOptionsScreen_Instance;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE     = &MenuOptionsScreen_VTABLE;

	s->descs             = descs;
	s->descriptions      = descriptions;
	s->descriptionsCount = descsCount;

	s->DoInit          = init;
	s->DoRecreateExtra = NULL;
	s->OnHacksChanged  = NULL;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_MENU);
}


/*########################################################################################################################*
*---------------------------------------------------ClassicOptionsScreen--------------------------------------------------*
*#########################################################################################################################*/
enum ViewDist { VIEW_TINY, VIEW_SHORT, VIEW_NORMAL, VIEW_FAR, VIEW_COUNT };
static const char* const viewDistNames[VIEW_COUNT] = { "TINY", "SHORT", "NORMAL", "FAR" };

static void ClassicOptionsScreen_GetMusic(cc_string* v) { Menu_GetBool(v, Audio_MusicVolume > 0); }
static void ClassicOptionsScreen_SetMusic(const cc_string* v) {
	Audio_SetMusic(String_CaselessEqualsConst(v, "ON") ? 100 : 0);
	Options_SetInt(OPT_MUSIC_VOLUME, Audio_MusicVolume);
}

static void ClassicOptionsScreen_GetInvert(cc_string* v) { Menu_GetBool(v, Camera.Invert); }
static void ClassicOptionsScreen_SetInvert(const cc_string* v) { Camera.Invert = Menu_SetBool(v, OPT_INVERT_MOUSE); }

static void ClassicOptionsScreen_GetViewDist(cc_string* v) {
	if (Game_ViewDistance >= 512) {
		String_AppendConst(v, viewDistNames[VIEW_FAR]);
	} else if (Game_ViewDistance >= 128) {
		String_AppendConst(v, viewDistNames[VIEW_NORMAL]);
	} else if (Game_ViewDistance >= 32) {
		String_AppendConst(v, viewDistNames[VIEW_SHORT]);
	} else {
		String_AppendConst(v, viewDistNames[VIEW_TINY]);
	}
}
static void ClassicOptionsScreen_SetViewDist(const cc_string* v) {
	int raw  = Utils_ParseEnum(v, 0, viewDistNames, VIEW_COUNT);
	int dist = raw == VIEW_FAR ? 512 : (raw == VIEW_NORMAL ? 128 : (raw == VIEW_SHORT ? 32 : 8));
	Game_UserSetViewDistance(dist);
}

static void ClassicOptionsScreen_GetPhysics(cc_string* v) { Menu_GetBool(v, Physics.Enabled); }
static void ClassicOptionsScreen_SetPhysics(const cc_string* v) {
	Physics_SetEnabled(Menu_SetBool(v, OPT_BLOCK_PHYSICS));
}

static void ClassicOptionsScreen_GetSounds(cc_string* v) { Menu_GetBool(v, Audio_SoundsVolume > 0); }
static void ClassicOptionsScreen_SetSounds(const cc_string* v) {
	Audio_SetSounds(String_CaselessEqualsConst(v, "ON") ? 100 : 0);
	Options_SetInt(OPT_SOUND_VOLUME, Audio_SoundsVolume);
}

static void ClassicOptionsScreen_GetShowFPS(cc_string* v) { Menu_GetBool(v, Gui.ShowFPS); }
static void ClassicOptionsScreen_SetShowFPS(const cc_string* v) { Gui.ShowFPS = Menu_SetBool(v, OPT_SHOW_FPS); }

static void ClassicOptionsScreen_GetViewBob(cc_string* v) { Menu_GetBool(v, Game_ViewBobbing); }
static void ClassicOptionsScreen_SetViewBob(const cc_string* v) { Game_ViewBobbing = Menu_SetBool(v, OPT_VIEW_BOBBING); }

static void ClassicOptionsScreen_GetHacks(cc_string* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.Enabled); }
static void ClassicOptionsScreen_SetHacks(const cc_string* v) {
	LocalPlayer_Instance.Hacks.Enabled = Menu_SetBool(v, OPT_HACKS_ENABLED);
	HacksComp_Update(&LocalPlayer_Instance.Hacks);
}

static void ClassicOptionsScreen_RecreateExtra(struct MenuOptionsScreen* s) {
	ButtonWidget_SetConst(&s->buttons[9], "Controls...", &s->titleFont);
}

static void ClassicOptionsScreen_InitWidgets(struct MenuOptionsScreen* s) {
	static const struct MenuOptionDesc buttons[9] = {
		{ -1, -150, "Music",           MenuOptionsScreen_Bool,
			ClassicOptionsScreen_GetMusic,    ClassicOptionsScreen_SetMusic },
		{ -1, -100, "Invert mouse",    MenuOptionsScreen_Bool,
			ClassicOptionsScreen_GetInvert,   ClassicOptionsScreen_SetInvert },
		{ -1,  -50, "Render distance", MenuOptionsScreen_Enum,
			ClassicOptionsScreen_GetViewDist, ClassicOptionsScreen_SetViewDist },
		{ -1,    0, "Block physics",   MenuOptionsScreen_Bool,
			ClassicOptionsScreen_GetPhysics,  ClassicOptionsScreen_SetPhysics },

		{ 1, -150, "Sound",         MenuOptionsScreen_Bool,
			ClassicOptionsScreen_GetSounds,  ClassicOptionsScreen_SetSounds },
		{ 1, -100, "Show FPS",      MenuOptionsScreen_Bool,
			ClassicOptionsScreen_GetShowFPS, ClassicOptionsScreen_SetShowFPS },
		{ 1,  -50, "View bobbing",  MenuOptionsScreen_Bool,
			ClassicOptionsScreen_GetViewBob, ClassicOptionsScreen_SetViewBob },
		{ 1,    0, "FPS mode",      MenuOptionsScreen_Enum,
			MenuOptionsScreen_GetFPS,        MenuOptionsScreen_SetFPS },
		{ 0,   60, "Hacks enabled", MenuOptionsScreen_Bool,
			ClassicOptionsScreen_GetHacks,   ClassicOptionsScreen_SetHacks }
	};
	s->numCore         = 9 + 1;
	s->DoRecreateExtra = ClassicOptionsScreen_RecreateExtra;

	MenuOptionsScreen_InitButtons(s, buttons, Array_Elems(buttons), Menu_SwitchPause);
	ButtonWidget_Make(&s->buttons[9], 400, Menu_SwitchKeysClassic,
						ANCHOR_CENTRE, ANCHOR_MAX, 0, 95);
	s->widgets[9] = (struct Widget*)&s->buttons[9];

	/* Disable certain options */
	if (!Server.IsSinglePlayer) Menu_Remove(s, 3);
	if (!Game_ClassicHacks)     Menu_Remove(s, 8);
}

void ClassicOptionsScreen_Show(void) {
	static struct MenuInputDesc descs[11];
	MenuInput_Enum(descs[2], viewDistNames,  VIEW_COUNT);
	MenuInput_Enum(descs[7], FpsLimit_Names, FPS_LIMIT_COUNT);

	MenuOptionsScreen_Show(descs, NULL, 0, ClassicOptionsScreen_InitWidgets);
}


/*########################################################################################################################*
*----------------------------------------------------EnvSettingsScreen----------------------------------------------------*
*#########################################################################################################################*/
static void EnvSettingsScreen_GetCloudsCol(cc_string* v) { PackedCol_ToHex(v, Env.CloudsCol); }
static void EnvSettingsScreen_SetCloudsCol(const cc_string* v) { Env_SetCloudsCol(Menu_HexCol(v)); }

static void EnvSettingsScreen_GetSkyCol(cc_string* v) { PackedCol_ToHex(v, Env.SkyCol); }
static void EnvSettingsScreen_SetSkyCol(const cc_string* v) { Env_SetSkyCol(Menu_HexCol(v)); }

static void EnvSettingsScreen_GetFogCol(cc_string* v) { PackedCol_ToHex(v, Env.FogCol); }
static void EnvSettingsScreen_SetFogCol(const cc_string* v) { Env_SetFogCol(Menu_HexCol(v)); }

static void EnvSettingsScreen_GetCloudsSpeed(cc_string* v) { String_AppendFloat(v, Env.CloudsSpeed, 2); }
static void EnvSettingsScreen_SetCloudsSpeed(const cc_string* v) { Env_SetCloudsSpeed(Menu_Float(v)); }

static void EnvSettingsScreen_GetCloudsHeight(cc_string* v) { String_AppendInt(v, Env.CloudsHeight); }
static void EnvSettingsScreen_SetCloudsHeight(const cc_string* v) { Env_SetCloudsHeight(Menu_Int(v)); }

static void EnvSettingsScreen_GetSunCol(cc_string* v) { PackedCol_ToHex(v, Env.SunCol); }
static void EnvSettingsScreen_SetSunCol(const cc_string* v) { Env_SetSunCol(Menu_HexCol(v)); }

static void EnvSettingsScreen_GetShadowCol(cc_string* v) { PackedCol_ToHex(v, Env.ShadowCol); }
static void EnvSettingsScreen_SetShadowCol(const cc_string* v) { Env_SetShadowCol(Menu_HexCol(v)); }

static void EnvSettingsScreen_GetWeather(cc_string* v) { String_AppendConst(v, Weather_Names[Env.Weather]); }
static void EnvSettingsScreen_SetWeather(const cc_string* v) {
	int raw = Utils_ParseEnum(v, 0, Weather_Names, Array_Elems(Weather_Names));
	Env_SetWeather(raw); 
}

static void EnvSettingsScreen_GetWeatherSpeed(cc_string* v) { String_AppendFloat(v, Env.WeatherSpeed, 2); }
static void EnvSettingsScreen_SetWeatherSpeed(const cc_string* v) { Env_SetWeatherSpeed(Menu_Float(v)); }

static void EnvSettingsScreen_GetEdgeHeight(cc_string* v) { String_AppendInt(v, Env.EdgeHeight); }
static void EnvSettingsScreen_SetEdgeHeight(const cc_string* v) { Env_SetEdgeHeight(Menu_Int(v)); }

static void EnvSettingsScreen_InitWidgets(struct MenuOptionsScreen* s) {
	static const struct MenuOptionDesc buttons[10] = {
		{ -1, -150, "Clouds col",    MenuOptionsScreen_Input,
			EnvSettingsScreen_GetCloudsCol,    EnvSettingsScreen_SetCloudsCol },
		{ -1, -100, "Sky col",       MenuOptionsScreen_Input,
			EnvSettingsScreen_GetSkyCol,       EnvSettingsScreen_SetSkyCol },
		{ -1,  -50, "Fog col",       MenuOptionsScreen_Input,
			EnvSettingsScreen_GetFogCol,       EnvSettingsScreen_SetFogCol },
		{ -1,    0, "Clouds speed",  MenuOptionsScreen_Input,
			EnvSettingsScreen_GetCloudsSpeed,  EnvSettingsScreen_SetCloudsSpeed },
		{ -1,   50, "Clouds height", MenuOptionsScreen_Input,
			EnvSettingsScreen_GetCloudsHeight, EnvSettingsScreen_SetCloudsHeight },

		{ 1, -150, "Sunlight col",    MenuOptionsScreen_Input,
			EnvSettingsScreen_GetSunCol,       EnvSettingsScreen_SetSunCol },
		{ 1, -100, "Shadow col",      MenuOptionsScreen_Input,
			EnvSettingsScreen_GetShadowCol,    EnvSettingsScreen_SetShadowCol },
		{ 1,  -50, "Weather",         MenuOptionsScreen_Enum,
			EnvSettingsScreen_GetWeather,      EnvSettingsScreen_SetWeather },
		{ 1,    0, "Rain/Snow speed", MenuOptionsScreen_Input,
			EnvSettingsScreen_GetWeatherSpeed, EnvSettingsScreen_SetWeatherSpeed },
		{ 1,   50, "Water level",     MenuOptionsScreen_Input,
			EnvSettingsScreen_GetEdgeHeight,   EnvSettingsScreen_SetEdgeHeight }
	};

	s->numCore = 10;
	MenuOptionsScreen_InitButtons(s, buttons, Array_Elems(buttons), Menu_SwitchOptions);
}

void EnvSettingsScreen_Show(void) {
	static struct MenuInputDesc descs[11];
	MenuInput_Hex(descs[0],   ENV_DEFAULT_CLOUDS_COL);
	MenuInput_Hex(descs[1],   ENV_DEFAULT_SKY_COL);
	MenuInput_Hex(descs[2],   ENV_DEFAULT_FOG_COL);
	MenuInput_Float(descs[3],      0,  1000, 1);
	MenuInput_Int(descs[4],   -10000, 10000, World.Height + 2);

	MenuInput_Hex(descs[5],   ENV_DEFAULT_SUN_COL);
	MenuInput_Hex(descs[6],   ENV_DEFAULT_SHADOW_COL);
	MenuInput_Enum(descs[7],  Weather_Names, Array_Elems(Weather_Names));
	MenuInput_Float(descs[8],  -100,  100, 1);
	MenuInput_Int(descs[9],   -2048, 2048, World.Height / 2);

	MenuOptionsScreen_Show(descs, NULL, 0, EnvSettingsScreen_InitWidgets);
}


/*########################################################################################################################*
*--------------------------------------------------GraphicsOptionsScreen--------------------------------------------------*
*#########################################################################################################################*/
static void GraphicsOptionsScreen_GetViewDist(cc_string* v) { String_AppendInt(v, Game_ViewDistance); }
static void GraphicsOptionsScreen_SetViewDist(const cc_string* v) { Game_UserSetViewDistance(Menu_Int(v)); }

static void GraphicsOptionsScreen_GetSmooth(cc_string* v) { Menu_GetBool(v, Builder_SmoothLighting); }
static void GraphicsOptionsScreen_SetSmooth(const cc_string* v) {
	Builder_SmoothLighting = Menu_SetBool(v, OPT_SMOOTH_LIGHTING);
	Builder_ApplyActive();
	MapRenderer_Refresh();
}

static void GraphicsOptionsScreen_GetNames(cc_string* v) { String_AppendConst(v, NameMode_Names[Entities.NamesMode]); }
static void GraphicsOptionsScreen_SetNames(const cc_string* v) {
	Entities.NamesMode = Utils_ParseEnum(v, 0, NameMode_Names, NAME_MODE_COUNT);
	Options_Set(OPT_NAMES_MODE, v);
}

static void GraphicsOptionsScreen_GetShadows(cc_string* v) { String_AppendConst(v, ShadowMode_Names[Entities.ShadowsMode]); }
static void GraphicsOptionsScreen_SetShadows(const cc_string* v) {
	Entities.ShadowsMode = Utils_ParseEnum(v, 0, ShadowMode_Names, SHADOW_MODE_COUNT);
	Options_Set(OPT_ENTITY_SHADOW, v);
}

static void GraphicsOptionsScreen_GetMipmaps(cc_string* v) { Menu_GetBool(v, Gfx.Mipmaps); }
static void GraphicsOptionsScreen_SetMipmaps(const cc_string* v) {
	Gfx.Mipmaps = Menu_SetBool(v, OPT_MIPMAPS);
	TexturePack_ExtractCurrent(true);
}

static void GraphicsOptionsScreen_GetCameraMass(cc_string* v) { String_AppendFloat(v, Camera.Mass, 2); }
static void GraphicsOptionsScreen_SetCameraMass(const cc_string* c) {
	Camera.Mass = Menu_Float(c);
	Options_Set(OPT_CAMERA_MASS, c);
}

static void GraphicsOptionsScreen_InitWidgets(struct MenuOptionsScreen* s) {
	static const struct MenuOptionDesc buttons[7] = {
		{ -1, -100, "Camera Mass",       MenuOptionsScreen_Input,
			GraphicsOptionsScreen_GetCameraMass, GraphicsOptionsScreen_SetCameraMass },
		{ -1, -50,  "FPS mode",          MenuOptionsScreen_Enum,
			MenuOptionsScreen_GetFPS,          MenuOptionsScreen_SetFPS },
		{ -1,   0,  "View distance",     MenuOptionsScreen_Input,
			GraphicsOptionsScreen_GetViewDist,   GraphicsOptionsScreen_SetViewDist },
		{ -1,  50,  "Advanced lighting", MenuOptionsScreen_Bool,
			GraphicsOptionsScreen_GetSmooth,     GraphicsOptionsScreen_SetSmooth },

		{ 1, -50, "Names",   MenuOptionsScreen_Enum,
			GraphicsOptionsScreen_GetNames,   GraphicsOptionsScreen_SetNames },
		{ 1,   0, "Shadows", MenuOptionsScreen_Enum,
			GraphicsOptionsScreen_GetShadows, GraphicsOptionsScreen_SetShadows },
		{ 1,  50, "Mipmaps", MenuOptionsScreen_Bool,
			GraphicsOptionsScreen_GetMipmaps, GraphicsOptionsScreen_SetMipmaps }
	};

	s->numCore = 7;
	MenuOptionsScreen_InitButtons(s, buttons, Array_Elems(buttons), Menu_SwitchOptions);
}

void GraphicsOptionsScreen_Show(void) {
	static struct MenuInputDesc descs[8];
	static const char* extDescs[Array_Elems(descs)];

	extDescs[0] = "&eChange the smoothness of the smooth camera.";
	extDescs[1] = \
		"&eVSync: &fNumber of frames rendered is at most the monitor's refresh rate.\n" \
		"&e30/60/120/144 FPS: &fRenders 30/60/120/144 frames at most each second.\n" \
		"&eNoLimit: &fRenders as many frames as possible each second.\n" \
		"&cNoLimit is pointless - it wastefully renders frames that you don't even see!";
	extDescs[3] = "&cNote: &eSmooth lighting is still experimental and can heavily reduce performance.";
	extDescs[4] = \
		"&eNone: &fNo names of players are drawn.\n" \
		"&eHovered: &fName of the targeted player is drawn see-through.\n" \
		"&eAll: &fNames of all other players are drawn normally.\n" \
		"&eAllHovered: &fAll names of players are drawn see-through.\n" \
		"&eAllUnscaled: &fAll names of players are drawn see-through without scaling.";
	extDescs[5] = \
		"&eNone: &fNo entity shadows are drawn.\n" \
		"&eSnapToBlock: &fA square shadow is shown on block you are directly above.\n" \
		"&eCircle: &fA circular shadow is shown across the blocks you are above.\n" \
		"&eCircleAll: &fA circular shadow is shown underneath all entities.";
	
	MenuInput_Float(descs[0], 1, 100, 20);
	MenuInput_Enum(descs[1], FpsLimit_Names, FPS_LIMIT_COUNT);
	MenuInput_Int(descs[2],  8, 4096, 512);
	MenuInput_Enum(descs[4], NameMode_Names,   NAME_MODE_COUNT);
	MenuInput_Enum(descs[5], ShadowMode_Names, SHADOW_MODE_COUNT);

	MenuOptionsScreen_Show(descs, extDescs, Array_Elems(extDescs), GraphicsOptionsScreen_InitWidgets);
}


/*########################################################################################################################*
*----------------------------------------------------ChatOptionsScreen-----------------------------------------------------*
*#########################################################################################################################*/
static void ChatOptionsScreen_SetScale(const cc_string* v, float* target, const char* optKey) {
	*target = Menu_Float(v);
	Options_Set(optKey, v);
	Gui_RefreshChat();
}

static void ChatOptionsScreen_GetChatScale(cc_string* v) { String_AppendFloat(v, Gui.RawChatScale, 1); }
static void ChatOptionsScreen_SetChatScale(const cc_string* v) { ChatOptionsScreen_SetScale(v, &Gui.RawChatScale, OPT_CHAT_SCALE); }

static void ChatOptionsScreen_GetChatlines(cc_string* v) { String_AppendInt(v, Gui.Chatlines); }
static void ChatOptionsScreen_SetChatlines(const cc_string* v) {
	Gui.Chatlines = Menu_Int(v);
	ChatScreen_SetChatlines(Gui.Chatlines);
	Options_Set(OPT_CHATLINES, v);
}

static void ChatOptionsScreen_GetLogging(cc_string* v) { Menu_GetBool(v, Chat_Logging); }
static void ChatOptionsScreen_SetLogging(const cc_string* v) { 
	Chat_Logging = Menu_SetBool(v, OPT_CHAT_LOGGING); 
	if (!Chat_Logging) Chat_DisableLogging();
}

static void ChatOptionsScreen_GetClickable(cc_string* v) { Menu_GetBool(v, Gui.ClickableChat); }
static void ChatOptionsScreen_SetClickable(const cc_string* v) { Gui.ClickableChat = Menu_SetBool(v, OPT_CLICKABLE_CHAT); }

static void ChatOptionsScreen_InitWidgets(struct MenuOptionsScreen* s) {
	static const struct MenuOptionDesc buttons[4] = {
		{ -1,  0, "Chat scale",         MenuOptionsScreen_Input,
			ChatOptionsScreen_GetChatScale, ChatOptionsScreen_SetChatScale },
		{ -1, 50, "Chat lines",         MenuOptionsScreen_Input,
			ChatOptionsScreen_GetChatlines, ChatOptionsScreen_SetChatlines },

		{  1,  0, "Log to disc",        MenuOptionsScreen_Bool,
			ChatOptionsScreen_GetLogging,   ChatOptionsScreen_SetLogging },
		{  1, 50, "Clickable chat",     MenuOptionsScreen_Bool,
			ChatOptionsScreen_GetClickable, ChatOptionsScreen_SetClickable }
	};

	s->numCore = 4;
	MenuOptionsScreen_InitButtons(s, buttons, Array_Elems(buttons), Menu_SwitchOptions);

	/* If MINFILES is defined, chat logging code is not even included at all */
#ifdef CC_BUILD_MINFILES
	s->buttons[2].disabled = true;
#endif
}

void ChatOptionsScreen_Show(void) {
	static struct MenuInputDesc descs[5];
	MenuInput_Float(descs[0], 0.25f, 4.00f, 1);
	MenuInput_Int(descs[1],       0,    30, Gui.DefaultLines);
	MenuOptionsScreen_Show(descs, NULL, 0, ChatOptionsScreen_InitWidgets);
}


/*########################################################################################################################*
*----------------------------------------------------GuiOptionsScreen-----------------------------------------------------*
*#########################################################################################################################*/
static void GuiOptionsScreen_GetShadows(cc_string* v) { Menu_GetBool(v, Drawer2D_BlackTextShadows); }
static void GuiOptionsScreen_SetShadows(const cc_string* v) {
	Drawer2D_BlackTextShadows = Menu_SetBool(v, OPT_BLACK_TEXT);
	Event_RaiseVoid(&ChatEvents.FontChanged);
}

static void GuiOptionsScreen_GetShowFPS(cc_string* v) { Menu_GetBool(v, Gui.ShowFPS); }
static void GuiOptionsScreen_SetShowFPS(const cc_string* v) { Gui.ShowFPS = Menu_SetBool(v, OPT_SHOW_FPS); }

static void GuiOptionsScreen_GetHotbar(cc_string* v) { String_AppendFloat(v, Gui.RawHotbarScale, 1); }
static void GuiOptionsScreen_SetHotbar(const cc_string* v) { ChatOptionsScreen_SetScale(v, &Gui.RawHotbarScale, OPT_HOTBAR_SCALE); }

static void GuiOptionsScreen_GetInventory(cc_string* v) { String_AppendFloat(v, Gui.RawInventoryScale, 1); }
static void GuiOptionsScreen_SetInventory(const cc_string* v) { ChatOptionsScreen_SetScale(v, &Gui.RawInventoryScale, OPT_INVENTORY_SCALE); }

static void GuiOptionsScreen_GetTabAuto(cc_string* v) { Menu_GetBool(v, Gui.TabAutocomplete); }
static void GuiOptionsScreen_SetTabAuto(const cc_string* v) { Gui.TabAutocomplete = Menu_SetBool(v, OPT_TAB_AUTOCOMPLETE); }

static void GuiOptionsScreen_GetUseFont(cc_string* v) { Menu_GetBool(v, !Drawer2D_BitmappedText); }
static void GuiOptionsScreen_SetUseFont(const cc_string* v) {
	Drawer2D_BitmappedText = !Menu_SetBool(v, OPT_USE_CHAT_FONT);
	Event_RaiseVoid(&ChatEvents.FontChanged);
}

static void GuiOptionsScreen_InitWidgets(struct MenuOptionsScreen* s) {
	static const struct MenuOptionDesc buttons[7] = {
		{ -1, -100, "Black text shadows", MenuOptionsScreen_Bool,
			GuiOptionsScreen_GetShadows,   GuiOptionsScreen_SetShadows },
		{ -1,  -50, "Show FPS",           MenuOptionsScreen_Bool,
			GuiOptionsScreen_GetShowFPS,   GuiOptionsScreen_SetShowFPS },
		{ -1,    0, "Hotbar scale",       MenuOptionsScreen_Input,
			GuiOptionsScreen_GetHotbar,    GuiOptionsScreen_SetHotbar },
		{ -1,   50, "Inventory scale",    MenuOptionsScreen_Input,
			GuiOptionsScreen_GetInventory, GuiOptionsScreen_SetInventory },

		{ 1,  -50, "Tab auto-complete",  MenuOptionsScreen_Bool,
			GuiOptionsScreen_GetTabAuto,   GuiOptionsScreen_SetTabAuto },
		{ 1,    0, "Use system font",    MenuOptionsScreen_Bool,
			GuiOptionsScreen_GetUseFont,   GuiOptionsScreen_SetUseFont },
		{ 1,   50, "Select system font", Menu_SwitchFont,
			NULL,                          NULL }
	};

	s->numCore = 7;
	MenuOptionsScreen_InitButtons(s, buttons, Array_Elems(buttons), Menu_SwitchOptions);
}

void GuiOptionsScreen_Show(void) {
	static struct MenuInputDesc descs[8];
	MenuInput_Float(descs[2], 0.25f, 4.00f, 1);
	MenuInput_Float(descs[3], 0.25f, 4.00f, 1);
	MenuOptionsScreen_Show(descs, NULL, 0, GuiOptionsScreen_InitWidgets);
}


/*########################################################################################################################*
*---------------------------------------------------HacksSettingsScreen---------------------------------------------------*
*#########################################################################################################################*/
static void HacksSettingsScreen_GetHacks(cc_string* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.Enabled); }
static void HacksSettingsScreen_SetHacks(const cc_string* v) {
	LocalPlayer_Instance.Hacks.Enabled = Menu_SetBool(v,OPT_HACKS_ENABLED);
	HacksComp_Update(&LocalPlayer_Instance.Hacks);
}

static void HacksSettingsScreen_GetSpeed(cc_string* v) { String_AppendFloat(v, LocalPlayer_Instance.Hacks.SpeedMultiplier, 2); }
static void HacksSettingsScreen_SetSpeed(const cc_string* v) {
	LocalPlayer_Instance.Hacks.SpeedMultiplier = Menu_Float(v);
	Options_Set(OPT_SPEED_FACTOR, v);
}

static void HacksSettingsScreen_GetClipping(cc_string* v) { Menu_GetBool(v, Camera.Clipping); }
static void HacksSettingsScreen_SetClipping(const cc_string* v) {
	Camera.Clipping = Menu_SetBool(v, OPT_CAMERA_CLIPPING);
}

static void HacksSettingsScreen_GetJump(cc_string* v) { String_AppendFloat(v, LocalPlayer_JumpHeight(), 3); }
static void HacksSettingsScreen_SetJump(const cc_string* v) {
	cc_string str; char strBuffer[STRING_SIZE];
	struct PhysicsComp* physics;

	physics = &LocalPlayer_Instance.Physics;
	physics->JumpVel     = PhysicsComp_CalcJumpVelocity(Menu_Float(v));
	physics->UserJumpVel = physics->JumpVel;
	
	String_InitArray(str, strBuffer);
	String_AppendFloat(&str, physics->JumpVel, 8);
	Options_Set(OPT_JUMP_VELOCITY, &str);
}

static void HacksSettingsScreen_GetWOMHacks(cc_string* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.WOMStyleHacks); }
static void HacksSettingsScreen_SetWOMHacks(const cc_string* v) {
	LocalPlayer_Instance.Hacks.WOMStyleHacks = Menu_SetBool(v, OPT_WOM_STYLE_HACKS);
}

static void HacksSettingsScreen_GetFullStep(cc_string* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.FullBlockStep); }
static void HacksSettingsScreen_SetFullStep(const cc_string* v) {
	LocalPlayer_Instance.Hacks.FullBlockStep = Menu_SetBool(v, OPT_FULL_BLOCK_STEP);
}

static void HacksSettingsScreen_GetPushback(cc_string* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.PushbackPlacing); }
static void HacksSettingsScreen_SetPushback(const cc_string* v) {
	LocalPlayer_Instance.Hacks.PushbackPlacing = Menu_SetBool(v, OPT_PUSHBACK_PLACING);
}

static void HacksSettingsScreen_GetLiquids(cc_string* v) { Menu_GetBool(v, Game_BreakableLiquids); }
static void HacksSettingsScreen_SetLiquids(const cc_string* v) {
	Game_BreakableLiquids = Menu_SetBool(v, OPT_MODIFIABLE_LIQUIDS);
}

static void HacksSettingsScreen_GetSlide(cc_string* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.NoclipSlide); }
static void HacksSettingsScreen_SetSlide(const cc_string* v) {
	LocalPlayer_Instance.Hacks.NoclipSlide = Menu_SetBool(v, OPT_NOCLIP_SLIDE);
}

static void HacksSettingsScreen_GetFOV(cc_string* v) { String_AppendInt(v, Game_Fov); }
static void HacksSettingsScreen_SetFOV(const cc_string* v) {
	int fov = Menu_Int(v);
	if (Game_ZoomFov > fov) Game_ZoomFov = fov;
	Game_DefaultFov = fov;

	Options_Set(OPT_FIELD_OF_VIEW, v);
	Game_SetFov(fov);
}

static void HacksSettingsScreen_CheckHacksAllowed(struct MenuOptionsScreen* s) {
	struct Widget** widgets = s->widgets;
	struct LocalPlayer* p   = &LocalPlayer_Instance;
	cc_bool disabled        = !p->Hacks.Enabled;

	widgets[3]->disabled = disabled || !p->Hacks.CanSpeed;
	widgets[4]->disabled = disabled || !p->Hacks.CanSpeed;
	widgets[5]->disabled = disabled || !p->Hacks.CanSpeed;
	widgets[7]->disabled = disabled || !p->Hacks.CanPushbackBlocks;
}

static void HacksSettingsScreen_InitWidgets(struct MenuOptionsScreen* s) {
	static const struct MenuOptionDesc buttons[10] = {
		{ -1, -150, "Hacks enabled",    MenuOptionsScreen_Bool,
			HacksSettingsScreen_GetHacks,    HacksSettingsScreen_SetHacks },
		{ -1, -100, "Speed multiplier", MenuOptionsScreen_Input,
			HacksSettingsScreen_GetSpeed,    HacksSettingsScreen_SetSpeed },
		{ -1,  -50, "Camera clipping",  MenuOptionsScreen_Bool,
			HacksSettingsScreen_GetClipping, HacksSettingsScreen_SetClipping },
		{ -1,    0, "Jump height",      MenuOptionsScreen_Input,
			HacksSettingsScreen_GetJump,     HacksSettingsScreen_SetJump },
		{ -1,   50, "WOM style hacks",  MenuOptionsScreen_Bool,
			HacksSettingsScreen_GetWOMHacks, HacksSettingsScreen_SetWOMHacks },
	
		{ 1, -150, "Full block stepping", MenuOptionsScreen_Bool,
			HacksSettingsScreen_GetFullStep, HacksSettingsScreen_SetFullStep },
		{ 1, -100, "Breakable liquids",   MenuOptionsScreen_Bool,
			HacksSettingsScreen_GetLiquids,  HacksSettingsScreen_SetLiquids },
		{ 1,  -50, "Pushback placing",    MenuOptionsScreen_Bool,
			HacksSettingsScreen_GetPushback, HacksSettingsScreen_SetPushback },
		{ 1,    0, "Noclip slide",        MenuOptionsScreen_Bool,
			HacksSettingsScreen_GetSlide,    HacksSettingsScreen_SetSlide },
		{ 1,   50, "Field of view",       MenuOptionsScreen_Input,
			HacksSettingsScreen_GetFOV,      HacksSettingsScreen_SetFOV },
	};
	s->numCore        = 10;
	s->OnHacksChanged = HacksSettingsScreen_CheckHacksAllowed;

	MenuOptionsScreen_InitButtons(s, buttons, Array_Elems(buttons), Menu_SwitchOptions);
	HacksSettingsScreen_CheckHacksAllowed(s);
}

void HacksSettingsScreen_Show(void) {
	static struct MenuInputDesc descs[11];
	static const char* extDescs[Array_Elems(descs)];

	extDescs[2] = "&eIf &fON&e, then the third person cameras will limit\n&etheir zoom distance if they hit a solid block.";
	extDescs[3] = "&eSets how many blocks high you can jump up.\n&eNote: You jump much higher when holding down the Speed key binding.";
	extDescs[7] = \
		"&eIf &fON&e, placing blocks that intersect your own position cause\n" \
		"&ethe block to be placed, and you to be moved out of the way.\n" \
		"&fThis is mainly useful for quick pillaring/towering.";
	extDescs[8] = "&eIf &fOFF&e, you will immediately stop when in noclip\n&emode and no movement keys are held down.";

	MenuInput_Float(descs[1], 0.1f,   50, 10);
	MenuInput_Float(descs[3], 0.1f, 2048, 1.233f);
	MenuInput_Int(descs[9],      1,  179, 70);

	MenuOptionsScreen_Show(descs, extDescs, Array_Elems(extDescs), HacksSettingsScreen_InitWidgets);
}


/*########################################################################################################################*
*----------------------------------------------------MiscOptionsScreen----------------------------------------------------*
*#########################################################################################################################*/
static void MiscOptionsScreen_GetReach(cc_string* v) { String_AppendFloat(v, LocalPlayer_Instance.ReachDistance, 2); }
static void MiscOptionsScreen_SetReach(const cc_string* v) { LocalPlayer_Instance.ReachDistance = Menu_Float(v); }

static void MiscOptionsScreen_GetMusic(cc_string* v) { String_AppendInt(v, Audio_MusicVolume); }
static void MiscOptionsScreen_SetMusic(const cc_string* v) {
	Options_Set(OPT_MUSIC_VOLUME, v);
	Audio_SetMusic(Menu_Int(v));
}

static void MiscOptionsScreen_GetSounds(cc_string* v) { String_AppendInt(v, Audio_SoundsVolume); }
static void MiscOptionsScreen_SetSounds(const cc_string* v) {
	Options_Set(OPT_SOUND_VOLUME, v);
	Audio_SetSounds(Menu_Int(v));
}

static void MiscOptionsScreen_GetViewBob(cc_string* v) { Menu_GetBool(v, Game_ViewBobbing); }
static void MiscOptionsScreen_SetViewBob(const cc_string* v) { Game_ViewBobbing = Menu_SetBool(v, OPT_VIEW_BOBBING); }

static void MiscOptionsScreen_GetPhysics(cc_string* v) { Menu_GetBool(v, Physics.Enabled); }
static void MiscOptionsScreen_SetPhysics(const cc_string* v) {
	Physics_SetEnabled(Menu_SetBool(v, OPT_BLOCK_PHYSICS));
}

static void MiscOptionsScreen_GetAutoClose(cc_string* v) { Menu_GetBool(v, Options_GetBool(OPT_AUTO_CLOSE_LAUNCHER, false)); }
static void MiscOptionsScreen_SetAutoClose(const cc_string* v) { Menu_SetBool(v, OPT_AUTO_CLOSE_LAUNCHER); }

static void MiscOptionsScreen_GetInvert(cc_string* v) { Menu_GetBool(v, Camera.Invert); }
static void MiscOptionsScreen_SetInvert(const cc_string* v) { Camera.Invert = Menu_SetBool(v, OPT_INVERT_MOUSE); }

static void MiscOptionsScreen_GetSensitivity(cc_string* v) { String_AppendInt(v, Camera.Sensitivity); }
static void MiscOptionsScreen_SetSensitivity(const cc_string* v) {
	Camera.Sensitivity = Menu_Int(v);
	Options_Set(OPT_SENSITIVITY, v);
}

static void MiscSettingsScreen_InitWidgets(struct MenuOptionsScreen* s) {
	static const struct MenuOptionDesc buttons[8] = {
		{ -1, -100, "Reach distance", MenuOptionsScreen_Input,
			MiscOptionsScreen_GetReach,       MiscOptionsScreen_SetReach },
		{ -1,  -50, "Music volume",   MenuOptionsScreen_Input,
			MiscOptionsScreen_GetMusic,       MiscOptionsScreen_SetMusic },
		{ -1,    0, "Sounds volume",  MenuOptionsScreen_Input,
			MiscOptionsScreen_GetSounds,      MiscOptionsScreen_SetSounds },
		{ -1,   50, "View bobbing",   MenuOptionsScreen_Bool,
			MiscOptionsScreen_GetViewBob,     MiscOptionsScreen_SetViewBob },
	
		{ 1, -100, "Block physics",       MenuOptionsScreen_Bool,
			MiscOptionsScreen_GetPhysics,     MiscOptionsScreen_SetPhysics },
		{ 1,  -50, "Auto close launcher", MenuOptionsScreen_Bool,
			MiscOptionsScreen_GetAutoClose,   MiscOptionsScreen_SetAutoClose },
		{ 1,    0, "Invert mouse",        MenuOptionsScreen_Bool,
			MiscOptionsScreen_GetInvert,      MiscOptionsScreen_SetInvert },
		{ 1,   50, "Mouse sensitivity",   MenuOptionsScreen_Input,
			MiscOptionsScreen_GetSensitivity, MiscOptionsScreen_SetSensitivity }
	};
	s->numCore = 8;
	MenuOptionsScreen_InitButtons(s, buttons, Array_Elems(buttons), Menu_SwitchOptions);

	/* Disable certain options */
	if (!Server.IsSinglePlayer) Menu_Remove(s, 0);
	if (!Server.IsSinglePlayer) Menu_Remove(s, 4);
}

void MiscOptionsScreen_Show(void) {
	static struct MenuInputDesc descs[9];
	MenuInput_Float(descs[0], 1, 1024, 5);
	MenuInput_Int(descs[1],   0, 100,  0);
	MenuInput_Int(descs[2],   0, 100,  0);
#ifdef CC_BUILD_WIN
	MenuInput_Int(descs[7],   1, 200, 40);
#else
	MenuInput_Int(descs[7],   1, 200, 30);
#endif

	MenuOptionsScreen_Show(descs, NULL, 0, MiscSettingsScreen_InitWidgets);
}


/*########################################################################################################################*
*-----------------------------------------------------NostalgiaScreen-----------------------------------------------------*
*#########################################################################################################################*/
static void NostalgiaScreen_GetHand(cc_string* v) { Menu_GetBool(v, Models.ClassicArms); }
static void NostalgiaScreen_SetHand(const cc_string* v) { Models.ClassicArms = Menu_SetBool(v, OPT_CLASSIC_ARM_MODEL); }

static void NostalgiaScreen_GetAnim(cc_string* v) { Menu_GetBool(v, !Game_SimpleArmsAnim); }
static void NostalgiaScreen_SetAnim(const cc_string* v) {
	Game_SimpleArmsAnim = String_CaselessEqualsConst(v, "OFF");
	Options_SetBool(OPT_SIMPLE_ARMS_ANIM, Game_SimpleArmsAnim);
}

static void NostalgiaScreen_GetGui(cc_string* v) { Menu_GetBool(v, Gui.ClassicTexture); }
static void NostalgiaScreen_SetGui(const cc_string* v) { Gui.ClassicTexture = Menu_SetBool(v, OPT_CLASSIC_GUI); }

static void NostalgiaScreen_GetList(cc_string* v) { Menu_GetBool(v, Gui.ClassicTabList); }
static void NostalgiaScreen_SetList(const cc_string* v) { Gui.ClassicTabList = Menu_SetBool(v, OPT_CLASSIC_TABLIST); }

static void NostalgiaScreen_GetOpts(cc_string* v) { Menu_GetBool(v, Gui.ClassicMenu); }
static void NostalgiaScreen_SetOpts(const cc_string* v) { Gui.ClassicMenu = Menu_SetBool(v, OPT_CLASSIC_OPTIONS); }

static void NostalgiaScreen_GetCustom(cc_string* v) { Menu_GetBool(v, Game_AllowCustomBlocks); }
static void NostalgiaScreen_SetCustom(const cc_string* v) { Game_AllowCustomBlocks = Menu_SetBool(v, OPT_CUSTOM_BLOCKS); }

static void NostalgiaScreen_GetCPE(cc_string* v) { Menu_GetBool(v, Game_UseCPE); }
static void NostalgiaScreen_SetCPE(const cc_string* v) { Game_UseCPE = Menu_SetBool(v, OPT_CPE); }

static void NostalgiaScreen_GetTexs(cc_string* v) { Menu_GetBool(v, Game_AllowServerTextures); }
static void NostalgiaScreen_SetTexs(const cc_string* v) { Game_AllowServerTextures = Menu_SetBool(v, OPT_SERVER_TEXTURES); }

static void NostalgiaScreen_GetClassicChat(cc_string* v) { Menu_GetBool(v, Gui.ClassicChat); }
static void NostalgiaScreen_SetClassicChat(const cc_string* v) { Gui.ClassicChat = Menu_SetBool(v, OPT_CLASSIC_CHAT); }

static void NostalgiaScreen_SwitchBack(void* a, void* b) {
	if (Gui.ClassicMenu) { Menu_SwitchPause(a, b); } else { Menu_SwitchOptions(a, b); }
}

static struct TextWidget nostalgia_desc;
static void NostalgiaScreen_RecreateExtra(struct MenuOptionsScreen* s) {
	TextWidget_SetConst(&nostalgia_desc, "&eButtons on the right require restarting game", &s->textFont);
}

static void NostalgiaScreen_InitWidgets(struct MenuOptionsScreen* s) {
	static const struct MenuOptionDesc buttons[9] = {
		{ -1, -150, "Classic hand model",   MenuOptionsScreen_Bool,
			NostalgiaScreen_GetHand,   NostalgiaScreen_SetHand },
		{ -1, -100, "Classic walk anim",    MenuOptionsScreen_Bool,
			NostalgiaScreen_GetAnim,   NostalgiaScreen_SetAnim },
		{ -1,  -50, "Classic gui textures", MenuOptionsScreen_Bool,
			NostalgiaScreen_GetGui,    NostalgiaScreen_SetGui },
		{ -1,    0, "Classic player list",  MenuOptionsScreen_Bool,
			NostalgiaScreen_GetList,   NostalgiaScreen_SetList },
		{ -1,   50, "Classic options",      MenuOptionsScreen_Bool,
			NostalgiaScreen_GetOpts,   NostalgiaScreen_SetOpts },
	
		{ 1, -150, "Allow custom blocks", MenuOptionsScreen_Bool,
			NostalgiaScreen_GetCustom, NostalgiaScreen_SetCustom },
		{ 1, -100, "Use CPE",             MenuOptionsScreen_Bool,
			NostalgiaScreen_GetCPE,    NostalgiaScreen_SetCPE },
		{ 1,  -50, "Use server textures", MenuOptionsScreen_Bool,
			NostalgiaScreen_GetTexs,   NostalgiaScreen_SetTexs },
        { 1,    0, "Use classic chat",    MenuOptionsScreen_Bool,
            NostalgiaScreen_GetClassicChat, NostalgiaScreen_SetClassicChat },
	};
	s->numCore         = 9 + 1;
	s->DoRecreateExtra = NostalgiaScreen_RecreateExtra;

	MenuOptionsScreen_InitButtons(s, buttons, Array_Elems(buttons), NostalgiaScreen_SwitchBack);
	TextWidget_Init(&nostalgia_desc);
	Widget_SetLocation(&nostalgia_desc, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 100);
	s->widgets[9] = (struct Widget*)&nostalgia_desc;
}

void NostalgiaScreen_Show(void) {
	MenuOptionsScreen_Show(NULL, NULL, 0, NostalgiaScreen_InitWidgets);
}


/*########################################################################################################################*
*---------------------------------------------------------Overlay---------------------------------------------------------*
*#########################################################################################################################*/
static void Overlay_InitLabels(struct TextWidget* labels) {
	int i;
	TextWidget_Init(&labels[0]);
	for (i = 1; i < 4; i++) {
		TextWidget_Init(&labels[i]);
		labels[i].col = PackedCol_Make(224, 224, 224, 255);
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
static struct Widget* texids_widgets[1] = { (struct Widget*)&TexIdsOverlay.title };

#define TEXIDS_MAX_PER_PAGE (ATLAS2D_TILES_PER_ROW * ATLAS2D_TILES_PER_ROW)
#define TEXIDS_TEXT_VERTICES (10 * 4 + 90 * 8 + 412 * 12) /* '0'-'9' + '10'-'99' + '100'-'511' */
#define TEXIDS_MAX_VERTICES (TEXTWIDGET_MAX + 4 * ATLAS1D_MAX_ATLASES + TEXIDS_TEXT_VERTICES)

static void TexIdsOverlay_Layout(void* screen) {
	struct TexIdsOverlay* s = (struct TexIdsOverlay*)screen;
	int size;

	size = WindowInfo.Height / ATLAS2D_TILES_PER_ROW;
	size = (size / 8) * 8;
	Math_Clamp(size, 8, 40);

	s->xOffset  = Gui_CalcPos(ANCHOR_CENTRE, 0, size * Atlas2D.RowsCount,     WindowInfo.Width);
	s->yOffset  = Gui_CalcPos(ANCHOR_CENTRE, 0, size * ATLAS2D_TILES_PER_ROW, WindowInfo.Height);
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

	Screen_CreateVb(screen);
	Drawer2D_MakeFont(&textFont, 8, FONT_FLAGS_NONE);
	Font_SetPadding(&textFont, 1);
	TextAtlas_Make(&s->idAtlas, &chars, &textFont, &prefix);
	Font_Free(&textFont);
	
	Menu_MakeTitleFont(&titleFont);
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

	tex.uv.U1 = 0.0f; tex.uv.U2 = UV2_Scale;
	tex.Width = size; tex.Height = size;

	for (row = 0; row < Atlas2D.RowsCount; row += ATLAS2D_TILES_PER_ROW) {
		for (i = 0; i < TEXIDS_MAX_PER_PAGE; i++) {

			tex.X = xOffset    + Atlas2D_TileX(i) * size;
			tex.Y = s->yOffset + Atlas2D_TileY(i) * size;

			tex.uv.V1 = Atlas1D_RowId(i + baseLoc) * Atlas1D.InvTileSize;
			tex.uv.V2 = tex.uv.V1      + UV2_Scale * Atlas1D.InvTileSize;
			
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
	
	for (row = 0; row < Atlas2D.RowsCount; row += ATLAS2D_TILES_PER_ROW) {
		idAtlas->tex.Y = s->yOffset + (size - idAtlas->tex.Height);

		for (y = 0; y < ATLAS2D_TILES_PER_ROW; y++) {
			for (x = 0; x < ATLAS2D_TILES_PER_ROW; x++) {
				idAtlas->curX = xOffset + size * x + 3; /* offset text by 3 pixels */
				TextAtlas_AddInt(idAtlas, id++, ptr);
			}
			idAtlas->tex.Y += size;
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
	for (i = 0; i < Atlas1D.Count; i++) {
		Gfx_BindTexture(Atlas1D.TexIds[i]);

		Gfx_DrawVb_IndexedTris_Range(count, offset);
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
	s->numWidgets  = Array_Elems(texids_widgets);
	s->maxVertices = TEXIDS_MAX_VERTICES;

	TextWidget_Init(&s->title);
	Event_Register_(&TextureEvents.AtlasChanged, s, TexIdsOverlay_OnAtlasChanged);
}

static void TexIdsOverlay_Free(void* screen) {
	struct TexIdsOverlay* s = (struct TexIdsOverlay*)screen;
	Event_Unregister_(&TextureEvents.AtlasChanged, s, TexIdsOverlay_OnAtlasChanged);
}

static void TexIdsOverlay_Render(void* screen, double delta) {
	struct TexIdsOverlay* s = (struct TexIdsOverlay*)screen;
	int offset = 0;

	Menu_RenderBounds();
	Gfx_SetTexturing(true);

	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_BindDynamicVb(s->vb);

	offset = Widget_Render2(&s->title, offset);
	offset = TexIdsOverlay_RenderTerrain(s, offset);

	Gfx_BindTexture(s->idAtlas.tex.ID);
	Gfx_DrawVb_IndexedTris_Range(s->textVertices, offset);
	Gfx_SetTexturing(false);
}

static int TexIdsOverlay_KeyDown(void* screen, int key) {
	struct Screen* s = (struct Screen*)screen;
	if (key == KeyBinds[KEYBIND_IDOVERLAY]) { Gui_Remove(s); return true; }
	return false;
}

static const struct ScreenVTABLE TexIdsOverlay_VTABLE = {
	TexIdsOverlay_Init,    Screen_NullUpdate, TexIdsOverlay_Free,
	TexIdsOverlay_Render,  TexIdsOverlay_BuildMesh,
	TexIdsOverlay_KeyDown, Screen_FInput,     Screen_FKeyPress, Screen_FText,
	Menu_PointerDown,      Screen_TPointer,   Menu_PointerMove, Screen_TMouseScroll,
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

static struct Widget* urlwarning_widgets[6] = {
	(struct Widget*)&UrlWarningOverlay.lbls[0], (struct Widget*)&UrlWarningOverlay.lbls[1],
	(struct Widget*)&UrlWarningOverlay.lbls[2], (struct Widget*)&UrlWarningOverlay.lbls[3],
	(struct Widget*)&UrlWarningOverlay.btns[0], (struct Widget*)&UrlWarningOverlay.btns[1]
};
#define URLWARNING_MAX_VERTICES (4 * TEXTWIDGET_MAX + 2 * BUTTONWIDGET_MAX)

static void UrlWarningOverlay_OpenUrl(void* screen, void* b) {
	struct UrlWarningOverlay* s = (struct UrlWarningOverlay*)screen;
	Process_StartOpen(&s->url);
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
	Screen_CreateVb(screen);

	Menu_MakeTitleFont(&titleFont);
	Menu_MakeBodyFont(&textFont);

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
	s->numWidgets  = Array_Elems(urlwarning_widgets);
	s->maxVertices = URLWARNING_MAX_VERTICES;

	Overlay_InitLabels(s->lbls);
	ButtonWidget_Init(&s->btns[0], 160, UrlWarningOverlay_OpenUrl);
	ButtonWidget_Init(&s->btns[1], 160, UrlWarningOverlay_AppendUrl);
}

static const struct ScreenVTABLE UrlWarningOverlay_VTABLE = {
	UrlWarningOverlay_Init,   Screen_NullUpdate,  Screen_NullFunc,  
	MenuScreen_Render2,       Screen_BuildMesh,
	Screen_InputDown,         Screen_TInput,      Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,         Screen_TPointer,    Menu_PointerMove, Screen_TMouseScroll,
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
	cc_bool deny, alwaysDeny;
	cc_uint32 contentLength;
	cc_string url;
	int reqID;
	struct FontDesc textFont;
	struct ButtonWidget btns[4];
	struct TextWidget   lbls[4];
	char _urlBuffer[STRING_SIZE + 1];
} TexPackOverlay;

static struct Widget* texpack_widgets[8] = {
	(struct Widget*)&TexPackOverlay.lbls[0], (struct Widget*)&TexPackOverlay.lbls[1],
	(struct Widget*)&TexPackOverlay.lbls[2], (struct Widget*)&TexPackOverlay.lbls[3],
	(struct Widget*)&TexPackOverlay.btns[0], (struct Widget*)&TexPackOverlay.btns[1],
	(struct Widget*)&TexPackOverlay.btns[2], (struct Widget*)&TexPackOverlay.btns[3]
};
#define TEXPACK_MAX_VERTICES (4 * TEXTWIDGET_MAX + 4 * BUTTONWIDGET_MAX)

static cc_bool TexPackOverlay_IsAlways(void* screen, void* w) { return Menu_Index(screen, w) >= 6; }

static void TexPackOverlay_YesClick(void* screen, void* widget) {
	struct TexPackOverlay* s = (struct TexPackOverlay*)screen;
	TexturePack_Extract(&s->url);
	if (TexPackOverlay_IsAlways(s, widget)) TextureCache_Accept(&s->url);
	Gui_Remove((struct Screen*)s);
}

static void TexPackOverlay_NoClick(void* screen, void* widget) {
	struct TexPackOverlay* s = (struct TexPackOverlay*)screen;
	s->alwaysDeny = TexPackOverlay_IsAlways(s, widget);
	s->deny       = true;
	Gui_Refresh((struct Screen*)s);
}

static void TexPackOverlay_ConfirmNoClick(void* screen, void* b) {
	struct TexPackOverlay* s = (struct TexPackOverlay*)screen;
	if (s->alwaysDeny) TextureCache_Deny(&s->url);
	Gui_Remove((struct Screen*)s);
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
	} else {
		TextWidget_SetConst(&s->lbls[3], "Download size: Determining...", &s->textFont);
	}
}

static void TexPackOverlay_Update(void* screen, double delta) {
	struct TexPackOverlay* s = (struct TexPackOverlay*)screen;
	struct HttpRequest item;
	if (!Http_GetResult(s->reqID, &item)) return;

	s->dirty         = true;
	s->contentLength = item.contentLength;
	TexPackOverlay_UpdateLine3(s);
}

static void TexPackOverlay_ContextLost(void* screen) {
	struct TexPackOverlay* s = (struct TexPackOverlay*)screen;
	Font_Free(&s->textFont);
	Screen_ContextLost(screen);
}

static void TexPackOverlay_ContextRecreated(void* screen) {
	struct TexPackOverlay* s = (struct TexPackOverlay*)screen;
	struct FontDesc titleFont;
	Screen_CreateVb(screen);

	Menu_MakeTitleFont(&titleFont);
	Menu_MakeBodyFont(&s->textFont);

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
	s->numWidgets  = Array_Elems(texpack_widgets);
	s->maxVertices = TEXPACK_MAX_VERTICES;

	s->contentLength = 0;
	s->deny          = false;	
	Overlay_InitLabels(s->lbls);

	ButtonWidget_Init(&s->btns[0], 160, NULL);
	ButtonWidget_Init(&s->btns[1], 160, NULL);
	ButtonWidget_Init(&s->btns[2], 160, NULL);
	ButtonWidget_Init(&s->btns[3], 160, NULL);
}

static const struct ScreenVTABLE TexPackOverlay_VTABLE = {
	TexPackOverlay_Init,   TexPackOverlay_Update, Screen_NullFunc,
	MenuScreen_Render2,    Screen_BuildMesh,
	Screen_InputDown,      Screen_TInput,         Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,      Screen_TPointer,       Menu_PointerMove, Screen_TMouseScroll,
	TexPackOverlay_Layout, TexPackOverlay_ContextLost, TexPackOverlay_ContextRecreated
};
void TexPackOverlay_Show(const cc_string* url) {
	struct TexPackOverlay* s = &TexPackOverlay;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE     = &TexPackOverlay_VTABLE;
	
	String_InitArray(s->url, s->_urlBuffer);
	String_Copy(&s->url, url);

	s->reqID = Http_AsyncGetHeaders(url, true);
	Gui_Add((struct Screen*)s, GUI_PRIORITY_TEXPACK);
}


#ifdef CC_BUILD_TOUCH
/*########################################################################################################################*
*-----------------------------------------------------TouchMoreScreen-----------------------------------------------------*
*#########################################################################################################################*/
static struct TouchMoreScreen {
	Screen_Body
	struct ButtonWidget btns[8];
} TouchMoreScreen;

static struct Widget* touchMore_widgets[8] = {
	(struct Widget*)&TouchMoreScreen.btns[0], (struct Widget*)&TouchMoreScreen.btns[1],
	(struct Widget*)&TouchMoreScreen.btns[2], (struct Widget*)&TouchMoreScreen.btns[3],
	(struct Widget*)&TouchMoreScreen.btns[4], (struct Widget*)&TouchMoreScreen.btns[5],
	(struct Widget*)&TouchMoreScreen.btns[6], (struct Widget*)&TouchMoreScreen.btns[7]
};
#define TOUCHMORE_MAX_VERTICES (8 * BUTTONWIDGET_MAX)

static void TouchMore_Toggle(KeyBind bind) {
	int key = KeyBinds[bind];
	Gui_Remove((struct Screen*)&TouchMoreScreen);
	Input_SetPressed(key, !Input_Pressed[key]);
}

static void TouchMore_Speed(void*  s, void* w) { TouchMore_Toggle(KEYBIND_SPEED); }
static void TouchMore_Fly(void*    s, void* w) { TouchMore_Toggle(KEYBIND_FLY); }
static void TouchMore_Noclip(void* s, void* w) { TouchMore_Toggle(KEYBIND_NOCLIP); }

static void TouchMore_Chat(void* s, void* w) {
	Gui_Remove((struct Screen*)&TouchMoreScreen);
	ChatScreen_OpenInput(&String_Empty);
}
static void TouchMore_Take(void* s, void* w) {
	Gui_Remove((struct Screen*)&TouchMoreScreen);
	Game_ScreenshotRequested = true;
}
static void TouchMore_Menu(void* s, void* w) {
	Gui_Remove((struct Screen*)&TouchMoreScreen);
	PauseScreen_Show();
}
static void TouchMore_Screen(void* s, void* w) {
	Gui_Remove((struct Screen*)&TouchMoreScreen);
	Game_ToggleFullscreen();
}
static void TouchMore_Fog(void* s, void* w) { Game_CycleViewDistance(); }

static const struct SimpleButtonDesc touchMore_btns[8] = {
	{ -160,  -50, "Chat",       TouchMore_Chat   },
	{ -160,    0, "Speed",      TouchMore_Speed  },
	{ -160,   50, "Fly",        TouchMore_Fly    },
	{ -160,  100, "Menu",       TouchMore_Menu   },
	{  160,  -50, "Screenshot", TouchMore_Take   },
	{  160,    0, "Fullscreen", TouchMore_Screen },
	{  160,   50, "Noclip",     TouchMore_Noclip },
	{  160,  100, "Fog",        TouchMore_Fog    }
};

static void TouchMoreScreen_ContextRecreated(void* screen) {
	struct TouchMoreScreen* s = (struct TouchMoreScreen*)screen;
	struct FontDesc titleFont;
	Menu_MakeTitleFont(&titleFont);
	Screen_CreateVb(screen);

	Menu_SetButtons(s->btns, &titleFont, touchMore_btns, 8);
	Font_Free(&titleFont);
}

static void TouchMoreScreen_Layout(void* screen) {
	struct TouchMoreScreen* s = (struct TouchMoreScreen*)screen;
	Menu_LayoutButtons(s->btns, touchMore_btns, 8);
}

static void TouchMoreScreen_Init(void* screen) {
	struct TouchMoreScreen* s = (struct TouchMoreScreen*)screen;
	s->widgets     = touchMore_widgets;
	s->numWidgets  = Array_Elems(touchMore_widgets);
	s->maxVertices = TOUCHMORE_MAX_VERTICES;

	Menu_InitButtons(s->btns, 300, touchMore_btns, 8);
	/* TODO: Close button */
}

static const struct ScreenVTABLE TouchMoreScreen_VTABLE = {
	TouchMoreScreen_Init,   Screen_NullUpdate, Screen_NullFunc,
	MenuScreen_Render2,     Screen_BuildMesh,
	Screen_InputDown,       Screen_TInput,     Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,       Screen_TPointer,   Menu_PointerMove, Screen_TMouseScroll,
	TouchMoreScreen_Layout, Screen_ContextLost, TouchMoreScreen_ContextRecreated
};
void TouchMoreScreen_Show(void) {
	struct TouchMoreScreen* s = &TouchMoreScreen;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE     = &TouchMoreScreen_VTABLE;

	Gui_Add((struct Screen*)s, GUI_PRIORITY_TOUCHMORE);
}
#endif
