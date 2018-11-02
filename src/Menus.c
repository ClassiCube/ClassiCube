#include "Menus.h"
#include "Widgets.h"
#include "Game.h"
#include "Event.h"
#include "GraphicsCommon.h"
#include "Platform.h"
#include "Inventory.h"
#include "Drawer2D.h"
#include "Graphics.h"
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
#include "World.h"
#include "Formats.h"
#include "ErrorHandler.h"
#include "BlockPhysics.h"
#include "MapRenderer.h"
#include "TexturePack.h"
#include "Audio.h"
#include "Screens.h"
#include "Gui.h"
#include "Deflate.h"
#include "Stream.h"

#define MenuBase_Layout Screen_Layout struct Widget** Widgets; int WidgetsCount;
struct Menu { MenuBase_Layout };

#define LIST_SCREEN_ITEMS 5
#define LIST_SCREEN_BUTTONS (LIST_SCREEN_ITEMS + 3)

struct ListScreen;
struct ListScreen {
	MenuBase_Layout
	struct ButtonWidget Buttons[LIST_SCREEN_BUTTONS];
	FontDesc Font;
	float WheelAcc;
	int CurrentIndex;
	Widget_LeftClick EntryClick;
	void (*UpdateEntry)(struct ListScreen* s, struct ButtonWidget* btn, const String* text);
	String TitleText;
	struct TextWidget Title, Page;
	struct Widget* ListWidgets[LIST_SCREEN_BUTTONS + 2];
	StringsBuffer Entries;
};

#define MenuScreen_Layout MenuBase_Layout FontDesc TitleFont, TextFont;
struct MenuScreen { MenuScreen_Layout };

struct PauseScreen {
	MenuScreen_Layout
	struct ButtonWidget Buttons[8];
};

struct OptionsGroupScreen {
	MenuScreen_Layout
	struct ButtonWidget Buttons[8];
	struct TextWidget Desc;
	int SelectedI;
};

struct EditHotkeyScreen {
	MenuScreen_Layout
	struct ButtonWidget Buttons[6];
	struct HotkeyData CurHotkey, OrigHotkey;
	int SelectedI;
	bool SupressNextPress;
	struct MenuInputWidget Input;
};

struct GenLevelScreen {
	MenuScreen_Layout
	struct ButtonWidget Buttons[3];
	struct MenuInputWidget* Selected;
	struct MenuInputWidget Inputs[4];
	struct TextWidget Labels[5];
};

struct ClassicGenScreen {
	MenuScreen_Layout
	struct ButtonWidget Buttons[4];
};

struct KeyBindingsScreen {
	MenuScreen_Layout
	struct ButtonWidget* Buttons;
	int CurI, BindsCount;
	const char** Descs;
	uint8_t* Binds;
	Widget_LeftClick LeftPage, RightPage;
	struct TextWidget Title;
	struct ButtonWidget Back, Left, Right;
};

struct SaveLevelScreen {
	MenuScreen_Layout
	struct ButtonWidget Buttons[3];
	struct MenuInputWidget Input;
	struct TextWidget MCEdit, Desc;
	String TextPath;
	char __TextPathBuffer[FILENAME_SIZE];
};

#define MENUOPTIONS_MAX_DESC 5
struct MenuOptionsScreen {
	MenuScreen_Layout
	struct ButtonWidget* Buttons;
	struct MenuInputValidator* Validators;
	const char** Descriptions;
	const char** DefaultValues;
	int ActiveI, SelectedI, DescriptionsCount;
	struct ButtonWidget OK, Default;
	struct MenuInputWidget Input;
	struct TextGroupWidget ExtHelp;
	struct Texture ExtHelp_Textures[MENUOPTIONS_MAX_DESC];
	char ExtHelp_Buffer[MENUOPTIONS_MAX_DESC * TEXTGROUPWIDGET_LEN];
};

struct TexIdsOverlay {
	MenuScreen_Layout
	struct ButtonWidget* Buttons;
	GfxResourceID DynamicVb;
	int XOffset, YOffset, TileSize, BaseTexLoc;
	struct TextAtlas IdAtlas;
	struct TextWidget Title;
};

struct UrlWarningOverlay {
	MenuScreen_Layout
	struct ButtonWidget Buttons[2];
	struct TextWidget   Labels[4];
	String Url;
	char __UrlBuffer[STRING_SIZE * 4];
};

struct ConfirmDenyOverlay {
	MenuScreen_Layout
	struct ButtonWidget Buttons[2];
	struct TextWidget   Labels[4];
	bool AlwaysDeny;
	String Url;
	char __UrlBuffer[STRING_SIZE];
};

struct TexPackOverlay {
	MenuScreen_Layout
	struct ButtonWidget Buttons[4];
	struct TextWidget Labels[4];
	uint32_t ContentLength;
	String Identifier;
	char __IdentifierBuffer[STRING_SIZE + 4];
};


/*########################################################################################################################*
*--------------------------------------------------------Menu base--------------------------------------------------------*
*#########################################################################################################################*/
static void Menu_Button(void* s, int i, struct ButtonWidget* btn, int width, const String* text, const FontDesc* font, Widget_LeftClick onClick, int horAnchor, int verAnchor, int x, int y) {
	struct Menu* menu = (struct Menu*)s;
	ButtonWidget_Create(btn, width, text, font, onClick);

	menu->Widgets[i] = (struct Widget*)btn;
	Widget_SetLocation(menu->Widgets[i], horAnchor, verAnchor, x, y);
}

static void Menu_Label(void* s, int i, struct TextWidget* label, const String* text, const FontDesc* font, int horAnchor, int verAnchor, int x, int y) {
	struct Menu* menu = (struct Menu*)s;
	TextWidget_Create(label, text, font);

	menu->Widgets[i] = (struct Widget*)label;
	Widget_SetLocation(menu->Widgets[i], horAnchor, verAnchor, x, y);
}

static void Menu_Input(void* s, int i, struct MenuInputWidget* input, int width, const String* text, const FontDesc* font, struct MenuInputValidator* v, int horAnchor, int verAnchor, int x, int y) {
	struct Menu* menu = (struct Menu*)s;
	MenuInputWidget_Create(input, width, 30, text, font, v);

	menu->Widgets[i] = (struct Widget*)input;
	Widget_SetLocation(menu->Widgets[i], horAnchor, verAnchor, x, y);
	input->Base.ShowCaret = true;
}

static void Menu_Back(void* s, int i, struct ButtonWidget* btn, const char* label, const FontDesc* font, Widget_LeftClick onClick) {
	int width = Game_UseClassicOptions ? 400 : 200;
	String msg = String_FromReadonly(label);
	Menu_Button(s, i, btn, width, &msg, font, onClick, ANCHOR_CENTRE, ANCHOR_MAX, 0, 25);
}

static void Menu_ContextLost(void* screen) {
	struct Menu* s = screen;
	struct Widget** widgets = s->Widgets;
	int i;
	if (!widgets) return;
	
	for (i = 0; i < s->WidgetsCount; i++) {
		if (!widgets[i]) continue;
		Elem_Free(widgets[i]);
	}
}

static void Menu_OnResize(void* screen) {
	struct Menu* s = screen;
	struct Widget** widgets = s->Widgets;
	int i;
	if (!widgets) return;
	
	for (i = 0; i < s->WidgetsCount; i++) {
		if (!widgets[i]) continue;
		Widget_Reposition(widgets[i]);
	}
}

static void Menu_Render(void* screen, double delta) {
	struct Menu* s = screen;
	struct Widget** widgets = s->Widgets;
	int i;
	if (!widgets) return;
	
	for (i = 0; i < s->WidgetsCount; i++) {
		if (!widgets[i]) continue;
		Elem_Render(widgets[i], delta);
	}
}

static void Menu_RenderBounds(void) {
	/* These were sourced by taking a screenshot of vanilla
	Then using paint to extract the colour components
	Then using wolfram alpha to solve the glblendfunc equation */
	PackedCol topCol    = PACKEDCOL_CONST(24, 24, 24, 105);
	PackedCol bottomCol = PACKEDCOL_CONST(51, 51, 98, 162);
	GfxCommon_Draw2DGradient(0, 0, Game_Width, Game_Height, topCol, bottomCol);
}

static int Menu_DoMouseDown(void* screen, int x, int y, MouseButton btn) {
	struct Menu* s = screen;
	struct Widget** widgets = s->Widgets;
	int i, count = s->WidgetsCount;

	/* iterate backwards (because last elements rendered are shown over others) */
	for (i = count - 1; i >= 0; i--) {
		struct Widget* w = widgets[i];
		if (!w || !Widget_Contains(w, x, y)) continue;
		if (w->Disabled) return i;

		if (w->MenuClick && btn == MouseButton_Left) {
			w->MenuClick(s, w);
		} else {
			Elem_HandlesMouseDown(w, x, y, btn);
		}
		return i;
	}
	return -1;
}
static bool Menu_MouseDown(void* screen, int x, int y, MouseButton btn) {
	return Menu_DoMouseDown(screen, x, y, btn) >= 0;
}

static int Menu_DoMouseMove(void* screen, int x, int y) {
	struct Menu* s = screen;
	struct Widget** widgets = s->Widgets;
	int i, count = s->WidgetsCount;

	for (i = 0; i < count; i++) {
		struct Widget* w = widgets[i];
		if (w) w->Active = false;
	}

	for (i = count - 1; i >= 0; i--) {
		struct Widget* w = widgets[i];
		if (!w || !Widget_Contains(w, x, y)) continue;

		w->Active = true;
		return i;
	}
	return -1;
}

static bool Menu_MouseMove(void* screen, int x, int y) {
	return Menu_DoMouseMove(screen, x, y) >= 0;
}

static bool Menu_MouseUp(void* screen, int x, int y, MouseButton btn) { return true; }
static bool Menu_KeyPress(void* screen, char keyChar) { return true; }
static bool Menu_KeyUp(void* screen, Key key) { return true; }


/*########################################################################################################################*
*------------------------------------------------------Menu utilities-----------------------------------------------------*
*#########################################################################################################################*/
static int Menu_Index(void* screen, void* widget) {
	struct Menu* s = screen;
	struct Widget** widgets = s->Widgets;
	int i;

	struct Widget* w = (struct Widget*)widget;
	for (i = 0; i < s->WidgetsCount; i++) {
		if (widgets[i] == w) return i;
	}
	return -1;
}

static void Menu_Remove(void* screen, int i) {
	struct Menu* s = screen;
	struct Widget** widgets = s->Widgets;

	if (widgets[i]) { Elem_TryFree(widgets[i]); }
	widgets[i] = NULL;
}

static void Menu_HandleFontChange(struct Screen* s) {
	Event_RaiseVoid(&ChatEvents_FontChanged);
	Elem_Recreate(s);
	Gui_RefreshHud();
	Elem_HandlesMouseMove(s, Mouse_X, Mouse_Y);
}

static int Menu_Int(const String* str)          { int v; Convert_TryParseInt(str, &v); return v; }
static float Menu_Float(const String* str)      { float v; Convert_TryParseFloat(str, &v); return v; }
static PackedCol Menu_HexCol(const String* str) { PackedCol v; PackedCol_TryParseHex(str, &v); return v; }
#define Menu_ReplaceActive(screen) Gui_FreeActive(); Gui_SetActive(screen);

static void Menu_SwitchOptions(void* a, void* b)        { Menu_ReplaceActive(OptionsGroupScreen_MakeInstance()); }
static void Menu_SwitchPause(void* a, void* b)          { Menu_ReplaceActive(PauseScreen_MakeInstance()); }
static void Menu_SwitchClassicOptions(void* a, void* b) { Menu_ReplaceActive(ClassicOptionsScreen_MakeInstance()); }

static void Menu_SwitchKeysClassic(void* a, void* b)      { Menu_ReplaceActive(ClassicKeyBindingsScreen_MakeInstance()); }
static void Menu_SwitchKeysClassicHacks(void* a, void* b) { Menu_ReplaceActive(ClassicHacksKeyBindingsScreen_MakeInstance()); }
static void Menu_SwitchKeysNormal(void* a, void* b)       { Menu_ReplaceActive(NormalKeyBindingsScreen_MakeInstance()); }
static void Menu_SwitchKeysHacks(void* a, void* b)        { Menu_ReplaceActive(HacksKeyBindingsScreen_MakeInstance()); }
static void Menu_SwitchKeysOther(void* a, void* b)        { Menu_ReplaceActive(OtherKeyBindingsScreen_MakeInstance()); }
static void Menu_SwitchKeysMouse(void* a, void* b)        { Menu_ReplaceActive(MouseKeyBindingsScreen_MakeInstance()); }

static void Menu_SwitchMisc(void* a, void* b)      { Menu_ReplaceActive(MiscOptionsScreen_MakeInstance()); }
static void Menu_SwitchGui(void* a, void* b)       { Menu_ReplaceActive(GuiOptionsScreen_MakeInstance()); }
static void Menu_SwitchGfx(void* a, void* b)       { Menu_ReplaceActive(GraphicsOptionsScreen_MakeInstance()); }
static void Menu_SwitchHacks(void* a, void* b)     { Menu_ReplaceActive(HacksSettingsScreen_MakeInstance()); }
static void Menu_SwitchEnv(void* a, void* b)       { Menu_ReplaceActive(EnvSettingsScreen_MakeInstance()); }
static void Menu_SwitchNostalgia(void* a, void* b) { Menu_ReplaceActive(NostalgiaScreen_MakeInstance()); }

static void Menu_SwitchGenLevel(void* a, void* b)        { Menu_ReplaceActive(GenLevelScreen_MakeInstance()); }
static void Menu_SwitchClassicGenLevel(void* a, void* b) { Menu_ReplaceActive(ClassicGenScreen_MakeInstance()); }
static void Menu_SwitchLoadLevel(void* a, void* b)       { Menu_ReplaceActive(LoadLevelScreen_MakeInstance()); }
static void Menu_SwitchSaveLevel(void* a, void* b)       { Menu_ReplaceActive(SaveLevelScreen_MakeInstance()); }
static void Menu_SwitchTexPacks(void* a, void* b)        { Menu_ReplaceActive(TexturePackScreen_MakeInstance()); }
static void Menu_SwitchHotkeys(void* a, void* b)         { Menu_ReplaceActive(HotkeyListScreen_MakeInstance()); }
static void Menu_SwitchFont(void* a, void* b)            { Menu_ReplaceActive(FontListScreen_MakeInstance()); }


/*########################################################################################################################*
*--------------------------------------------------------ListScreen-------------------------------------------------------*
*#########################################################################################################################*/
struct ListScreen ListScreen_Instance;
#define LIST_SCREEN_EMPTY "-----"

STRING_REF String ListScreen_UNSAFE_Get(struct ListScreen* s, int index) {
	static String str = String_FromConst(LIST_SCREEN_EMPTY);

	if (index >= 0 && index < s->Entries.Count) {
		return StringsBuffer_UNSAFE_Get(&s->Entries, index);
	}
	return str;
}

static void ListScreen_MakeText(struct ListScreen* s, int i) {
	String text = ListScreen_UNSAFE_Get(s, s->CurrentIndex + i);
	Menu_Button(s, i, &s->Buttons[i], 300, &String_Empty, &s->Font, 
		s->EntryClick, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, (i - 2) * 50);
	/* needed for font list menu */
	s->UpdateEntry(s, &s->Buttons[i], &text);
}

static void ListScreen_Make(struct ListScreen* s, int i, int x, const String* text, Widget_LeftClick onClick) {
	Menu_Button(s, i, &s->Buttons[i], 40, text, &s->Font, onClick,
		ANCHOR_CENTRE, ANCHOR_CENTRE, x, 0);
}

static void ListScreen_UpdatePage(struct ListScreen* s) {
	int start = LIST_SCREEN_ITEMS, end = s->Entries.Count - LIST_SCREEN_ITEMS;
	s->Buttons[5].Disabled = s->CurrentIndex < start;
	s->Buttons[6].Disabled = s->CurrentIndex >= end;
	if (Game_ClassicMode) return;

	char pageBuffer[STRING_SIZE];
	String page = String_FromArray(pageBuffer);
	int num     = (s->CurrentIndex  / LIST_SCREEN_ITEMS) + 1;
	int pages   = Math_CeilDiv(s->Entries.Count, LIST_SCREEN_ITEMS);
	if (pages == 0) pages = 1;

	String_Format2(&page, "&7Page %i of %i", &num, &pages);
	TextWidget_Set(&s->Page, &page, &s->Font);
}

static void ListScreen_UpdateEntry(struct ListScreen* s, struct ButtonWidget* button, const String* text) {
	ButtonWidget_Set(button, text, &s->Font);
}

static void ListScreen_SetCurrentIndex(struct ListScreen* s, int index) {
	String str;
	int i;

	if (index >= s->Entries.Count) { index = s->Entries.Count - 1; }
	if (index < 0) index = 0;
	
	for (i = 0; i < LIST_SCREEN_ITEMS; i++) {
		str = ListScreen_UNSAFE_Get(s, index + i);
		s->UpdateEntry(s, &s->Buttons[i], &str);
	}

	s->CurrentIndex = index;
	ListScreen_UpdatePage(s);
}

static void ListScreen_PageClick(struct ListScreen* s, bool forward) {
	int delta = forward ? LIST_SCREEN_ITEMS : -LIST_SCREEN_ITEMS;
	ListScreen_SetCurrentIndex(s, s->CurrentIndex + delta);
}

static void ListScreen_MoveBackwards(void* screen, void* b) {
	struct ListScreen* s = screen;
	ListScreen_PageClick(s, false);
}

static void ListScreen_MoveForwards(void* screen, void* b) {
	struct ListScreen* s = screen;
	ListScreen_PageClick(s, true);
}

static void ListScreen_ContextRecreated(void* screen) {
	static String lArrow = String_FromConst("<");
	static String rArrow = String_FromConst(">");

	struct ListScreen* s = screen;
	int i;
	for (i = 0; i < LIST_SCREEN_ITEMS; i++) { ListScreen_MakeText(s, i); }
	
	ListScreen_Make(s, 5, -220, &lArrow, ListScreen_MoveBackwards);	
	ListScreen_Make(s, 6,  220, &rArrow, ListScreen_MoveForwards);

	Menu_Back(s, 7, &s->Buttons[7], "Done", &s->Font, Menu_SwitchPause);

	Menu_Label(s, 8, &s->Title, &s->TitleText, &s->Font,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -155);
	Menu_Label(s, 9, &s->Page,  &String_Empty, &s->Font,
		ANCHOR_CENTRE, ANCHOR_MAX,    0,   75);

	ListScreen_UpdatePage(s);
}

static void ListScreen_QuickSort(int left, int right) {
	StringsBuffer* buffer = &ListScreen_Instance.Entries; 
	uint32_t* keys = buffer->FlagsBuffer; uint32_t key;

	while (left < right) {
		int i = left, j = right;
		String pivot = StringsBuffer_UNSAFE_Get(buffer, (i + j) >> 1);
		String strI, strJ;		

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

static String ListScreen_UNSAFE_GetCur(struct ListScreen* s, struct Widget* w) {
	int i = Menu_Index(s, w);
	return ListScreen_UNSAFE_Get(s, s->CurrentIndex + i);
}

static void ListScreen_Select(struct ListScreen* s, const String* str) {
	String entry;
	int i;

	for (i = 0; i < s->Entries.Count; i++) {
		entry = StringsBuffer_UNSAFE_Get(&s->Entries, i);
		if (!String_CaselessEquals(&entry, str)) continue;

		ListScreen_SetCurrentIndex(s, i);
		return;
	}
}

static void ListScreen_Init(void* screen) {
	struct ListScreen* s = screen;
	s->Widgets      = s->ListWidgets;
	s->WidgetsCount = Array_Elems(s->ListWidgets);
	Drawer2D_MakeFont(&s->Font, 16, FONT_STYLE_BOLD);

	Key_KeyRepeat = true;
	s->WheelAcc   = 0.0f;
	Screen_CommonInit(s);
}

static void ListScreen_Render(void* screen, double delta) {
	Menu_RenderBounds();
	Gfx_SetTexturing(true);
	Menu_Render(screen, delta);
	Gfx_SetTexturing(false);
}

static void ListScreen_Free(void* screen) {
	struct ListScreen* s = screen;
	Font_Free(&s->Font);
	Key_KeyRepeat = false;
	Screen_CommonFree(s);
}

static bool ListScreen_KeyDown(void* screen, Key key) {
	struct ListScreen* s = screen;
	if (key == Key_Escape) {
		Gui_CloseActive();
	} else if (key == Key_Left) {
		ListScreen_PageClick(s, false);
	} else if (key == Key_Right) {
		ListScreen_PageClick(s, true);
	} else {
		return false;
	}
	return true;
}

static bool ListScreen_MouseScroll(void* screen, float delta) {
	struct ListScreen* s = screen;
	int steps = Utils_AccumulateWheelDelta(&s->WheelAcc, delta);

	if (steps) ListScreen_SetCurrentIndex(s, s->CurrentIndex - steps);
	return true;
}

struct ScreenVTABLE ListScreen_VTABLE = {
	ListScreen_Init,    ListScreen_Render, ListScreen_Free, Gui_DefaultRecreate,
	ListScreen_KeyDown, Menu_KeyUp,        Menu_KeyPress,
	Menu_MouseDown,     Menu_MouseUp,      Menu_MouseMove,  ListScreen_MouseScroll,
	Menu_OnResize,      Menu_ContextLost,  ListScreen_ContextRecreated,
};
struct ListScreen* ListScreen_MakeInstance(void) {
	struct ListScreen* s = &ListScreen_Instance;
	StringsBuffer_Clear(&s->Entries);
	s->HandlesAllInput = true;
	s->WidgetsCount    = 0;
	s->CurrentIndex    = 0;

	s->UpdateEntry = ListScreen_UpdateEntry;
	s->VTABLE      = &ListScreen_VTABLE;
	return s;
}


/*########################################################################################################################*
*--------------------------------------------------------MenuScreen-------------------------------------------------------*
*#########################################################################################################################*/
static bool MenuScreen_KeyDown(void* screen, Key key) {
	if (key == Key_Escape) { Gui_CloseActive(); }
	return key < Key_F1 || key > Key_F35;
}
static bool MenuScreen_MouseScroll(void* screen, float delta) { return true; }

static void MenuScreen_Init(void* screen) {
	struct MenuScreen* s = screen;
	if (!s->TitleFont.Handle && !s->TitleFont.Size) {
		Drawer2D_MakeFont(&s->TitleFont, 16, FONT_STYLE_BOLD);
	}
	if (!s->TextFont.Handle && !s->TextFont.Size) {
		Drawer2D_MakeFont(&s->TextFont, 16, FONT_STYLE_NORMAL);
	}

	Screen_CommonInit(s);
}

static void MenuScreen_Render(void* screen, double delta) {
	Menu_RenderBounds();
	Gfx_SetTexturing(true);
	Menu_Render(screen, delta);
	Gfx_SetTexturing(false);
}

static void MenuScreen_Free(void* screen) {
	struct MenuScreen* s = screen;
	Font_Free(&s->TitleFont);
	Font_Free(&s->TextFont);

	Screen_CommonFree(s);
}


/*########################################################################################################################*
*-------------------------------------------------------PauseScreen-------------------------------------------------------*
*#########################################################################################################################*/
struct PauseScreen PauseScreen_Instance;
static void PauseScreen_Make(struct PauseScreen* s, int i, int dir, int y, const char* title, Widget_LeftClick onClick) {
	String text = String_FromReadonly(title);
	Menu_Button(s, i, &s->Buttons[i], 300, &text, &s->TitleFont, onClick,
		ANCHOR_CENTRE, ANCHOR_CENTRE, dir * 160, y);
}

static void PauseScreen_MakeClassic(struct PauseScreen* s, int i, int y, const char* title, Widget_LeftClick onClick) {
	String text = String_FromReadonly(title);
	Menu_Button(s, i, &s->Buttons[i], 400, &text, &s->TitleFont, onClick,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, y);
}

static void PauseScreen_Quit(void* a, void* b) { Window_Close(); }
static void PauseScreen_Game(void* a, void* b) { Gui_CloseActive(); }

static void PauseScreen_CheckHacksAllowed(void* screen) {
	struct PauseScreen* s = screen;
	if (Game_UseClassicOptions) return;
	s->Buttons[4].Disabled = !LocalPlayer_Instance.Hacks.CanAnyHacks; /* select texture pack */
}

static void PauseScreen_ContextRecreated(void* screen) {
	static String quitMsg = String_FromConst("Quit game");
	struct PauseScreen* s = screen;

	if (Game_UseClassicOptions) {
		PauseScreen_MakeClassic(s, 0, -100, "Options...",            Menu_SwitchClassicOptions);
		PauseScreen_MakeClassic(s, 1,  -50, "Generate new level...", Menu_SwitchClassicGenLevel);
		PauseScreen_MakeClassic(s, 2,    0, "Load level...",         Menu_SwitchLoadLevel);
		PauseScreen_MakeClassic(s, 3,   50, "Save level...",         Menu_SwitchSaveLevel);
		PauseScreen_MakeClassic(s, 4,  150, "Nostalgia options...",  Menu_SwitchNostalgia);

		Menu_Back(s, 5, &s->Buttons[5], "Back to game", &s->TitleFont, PauseScreen_Game);

		/* Disable nostalgia options in classic mode */
		if (Game_ClassicMode) Menu_Remove(s, 4);
		s->Widgets[6] = NULL;
		s->Widgets[7] = NULL;
	} else {
		PauseScreen_Make(s, 0, -1, -50, "Options...",             Menu_SwitchOptions);
		PauseScreen_Make(s, 1,  1, -50, "Generate new level...",  Menu_SwitchGenLevel);
		PauseScreen_Make(s, 2,  1,   0, "Load level...",          Menu_SwitchLoadLevel);
		PauseScreen_Make(s, 3,  1,  50, "Save level...",          Menu_SwitchSaveLevel);
		PauseScreen_Make(s, 4, -1,   0, "Change texture pack...", Menu_SwitchTexPacks);
		PauseScreen_Make(s, 5, -1,  50, "Hotkeys...",             Menu_SwitchHotkeys);
		
		Menu_Button(s, 6, &s->Buttons[6], 120, &quitMsg, &s->TitleFont, PauseScreen_Quit,
			ANCHOR_MAX, ANCHOR_MAX, 5, 5);
		Menu_Back(s, 7, &s->Buttons[7], "Back to game",  &s->TitleFont, PauseScreen_Game);
	}

	if (!ServerConnection_IsSinglePlayer) {
		s->Buttons[1].Disabled = true;
		s->Buttons[2].Disabled = true;
	}
	PauseScreen_CheckHacksAllowed(s);
}

static void PauseScreen_Init(void* screen) {
	struct PauseScreen* s = screen;
	MenuScreen_Init(s);
	Event_RegisterVoid(&UserEvents_HackPermissionsChanged, s, PauseScreen_CheckHacksAllowed);
}

static void PauseScreen_Free(void* screen) {
	struct PauseScreen* s = screen;
	MenuScreen_Free(s);
	Event_UnregisterVoid(&UserEvents_HackPermissionsChanged, s, PauseScreen_CheckHacksAllowed);
}

struct ScreenVTABLE PauseScreen_VTABLE = {
	PauseScreen_Init,   MenuScreen_Render,  PauseScreen_Free, Gui_DefaultRecreate,
	MenuScreen_KeyDown, Menu_KeyUp,         Menu_KeyPress,
	Menu_MouseDown,     Menu_MouseUp,       Menu_MouseMove,   MenuScreen_MouseScroll,
	Menu_OnResize,      Menu_ContextLost,   PauseScreen_ContextRecreated,
};
struct Screen* PauseScreen_MakeInstance(void) {
	static struct Widget* widgets[8];
	struct PauseScreen* s = &PauseScreen_Instance;

	s->HandlesAllInput = true;
	s->Widgets         = widgets;
	s->WidgetsCount    = Array_Elems(widgets);

	s->VTABLE = &PauseScreen_VTABLE;
	return (struct Screen*)s;
}


/*########################################################################################################################*
*--------------------------------------------------OptionsGroupScreen-----------------------------------------------------*
*#########################################################################################################################*/
struct OptionsGroupScreen OptionsGroupScreen_Instance;
const char* optsGroup_descs[7] = {
	"&eMusic/Sound, view bobbing, and more",
	"&eChat options, gui scale, font settings, and more",
	"&eFPS limit, view distance, entity names/shadows",
	"&eSet key bindings, bind keys to act as mouse clicks",
	"&eHacks allowed, jump settings, and more",
	"&eEnv colours, water level, weather, and more",
	"&eSettings for resembling the original classic",
};

static void OptionsGroupScreen_CheckHacksAllowed(void* screen) {
	struct OptionsGroupScreen* s = screen;
	s->Buttons[5].Disabled = !LocalPlayer_Instance.Hacks.CanAnyHacks; /* env settings */
}

static void OptionsGroupScreen_Make(struct OptionsGroupScreen* s, int i, int dir, int y, const char* title, Widget_LeftClick onClick) {
	String text = String_FromReadonly(title);
	Menu_Button(s, i, &s->Buttons[i], 300, &text, &s->TitleFont, onClick,
		ANCHOR_CENTRE, ANCHOR_CENTRE, dir * 160, y);
}

static void OptionsGroupScreen_MakeDesc(struct OptionsGroupScreen* s) {
	String text = String_FromReadonly(optsGroup_descs[s->SelectedI]);
	Menu_Label(s, 8, &s->Desc, &text, &s->TextFont,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 100);
}

static void OptionsGroupScreen_ContextRecreated(void* screen) {
	struct OptionsGroupScreen* s = screen;
	OptionsGroupScreen_Make(s, 0, -1, -100, "Misc options...",      Menu_SwitchMisc);
	OptionsGroupScreen_Make(s, 1, -1,  -50, "Gui options...",       Menu_SwitchGui);
	OptionsGroupScreen_Make(s, 2, -1,    0, "Graphics options...",  Menu_SwitchGfx);
	OptionsGroupScreen_Make(s, 3, -1,   50, "Controls...",          Menu_SwitchKeysNormal);
	OptionsGroupScreen_Make(s, 4,  1,  -50, "Hacks settings...",    Menu_SwitchHacks);
	OptionsGroupScreen_Make(s, 5,  1,    0, "Env settings...",      Menu_SwitchEnv);
	OptionsGroupScreen_Make(s, 6,  1,   50, "Nostalgia options...", Menu_SwitchNostalgia);

	Menu_Back(s, 7, &s->Buttons[7], "Done", &s->TitleFont, Menu_SwitchPause);	
	s->Widgets[8] = NULL; /* Description text widget placeholder */

	if (s->SelectedI >= 0) { OptionsGroupScreen_MakeDesc(s); }
	OptionsGroupScreen_CheckHacksAllowed(s);
}

static void OptionsGroupScreen_Init(void* screen) {
	struct OptionsGroupScreen* s = screen;
	MenuScreen_Init(s);
	Event_RegisterVoid(&UserEvents_HackPermissionsChanged, s, OptionsGroupScreen_CheckHacksAllowed);
}

static void OptionsGroupScreen_Free(void* screen) {
	struct OptionsGroupScreen* s = screen;
	MenuScreen_Free(s);
	Event_UnregisterVoid(&UserEvents_HackPermissionsChanged, s, OptionsGroupScreen_CheckHacksAllowed);
}

static bool OptionsGroupScreen_MouseMove(void* screen, int x, int y) {
	struct OptionsGroupScreen* s = screen;
	int i = Menu_DoMouseMove(s, x, y);
	if (i == -1 || i == s->SelectedI) return true;
	if (i >= Array_Elems(optsGroup_descs)) return true;

	s->SelectedI = i;
	Elem_TryFree(&s->Desc);
	OptionsGroupScreen_MakeDesc(s);
	return true;
}

struct ScreenVTABLE OptionsGroupScreen_VTABLE = {
	OptionsGroupScreen_Init, MenuScreen_Render,  OptionsGroupScreen_Free,      Gui_DefaultRecreate,
	MenuScreen_KeyDown,      Menu_KeyUp,         Menu_KeyPress,
	Menu_MouseDown,          Menu_MouseUp,       OptionsGroupScreen_MouseMove, MenuScreen_MouseScroll,
	Menu_OnResize,           Menu_ContextLost,   OptionsGroupScreen_ContextRecreated,
};
struct Screen* OptionsGroupScreen_MakeInstance(void) {
	static struct Widget* widgets[9];
	struct OptionsGroupScreen* s = &OptionsGroupScreen_Instance;
	
	s->HandlesAllInput = true;
	s->Widgets         = widgets;
	s->WidgetsCount    = Array_Elems(widgets);

	s->VTABLE = &OptionsGroupScreen_VTABLE;
	s->SelectedI = -1;
	return (struct Screen*)s;
}


/*########################################################################################################################*
*----------------------------------------------------EditHotkeyScreen-----------------------------------------------------*
*#########################################################################################################################*/
struct EditHotkeyScreen EditHotkeyScreen_Instance;
static void EditHotkeyScreen_Make(struct EditHotkeyScreen* s, int i, int x, int y, const String* text, Widget_LeftClick onClick) {
	Menu_Button(s, i, &s->Buttons[i], 300, text, &s->TitleFont, onClick,
		ANCHOR_CENTRE, ANCHOR_CENTRE, x, y);
}

static void HotkeyListScreen_MakeFlags(int flags, String* str);
static void EditHotkeyScreen_MakeFlags(int flags, String* str) {
	if (flags == 0) String_AppendConst(str, " None");
	HotkeyListScreen_MakeFlags(flags, str);
}

static void EditHotkeyScreen_MakeBaseKey(struct EditHotkeyScreen* s, Widget_LeftClick onClick) {
	char textBuffer[STRING_SIZE];
	String text = String_FromArray(textBuffer);

	String_AppendConst(&text, "Key: ");
	String_AppendConst(&text, Key_Names[s->CurHotkey.Trigger]);
	EditHotkeyScreen_Make(s, 0, 0, -150, &text, onClick);
}

static void EditHotkeyScreen_MakeModifiers(struct EditHotkeyScreen* s, Widget_LeftClick onClick) {
	char textBuffer[STRING_SIZE];
	String text = String_FromArray(textBuffer);

	String_AppendConst(&text, "Modifiers:");
	EditHotkeyScreen_MakeFlags(s->CurHotkey.Flags, &text);
	EditHotkeyScreen_Make(s, 1, 0, -100, &text, onClick);
}

static void EditHotkeyScreen_MakeLeaveOpen(struct EditHotkeyScreen* s, Widget_LeftClick onClick) {
	char textBuffer[STRING_SIZE];
	String text = String_FromArray(textBuffer);

	String_AppendConst(&text, "Input stays open: ");
	String_AppendConst(&text, s->CurHotkey.StaysOpen ? "ON" : "OFF");
	EditHotkeyScreen_Make(s, 2, -100, 10, &text, onClick);
}

static void EditHotkeyScreen_BaseKey(void* screen, void* b) {
	static String msg = String_FromConst("Key: press a key..");
	struct EditHotkeyScreen* s = screen;

	s->SelectedI = 0;
	s->SupressNextPress = true;
	ButtonWidget_Set(&s->Buttons[0], &msg, &s->TitleFont);
}

static void EditHotkeyScreen_Modifiers(void* screen, void* b) {
	static String msg = String_FromConst("Modifiers: press a key..");
	struct EditHotkeyScreen* s = screen;

	s->SelectedI = 1;
	s->SupressNextPress = true;
	ButtonWidget_Set(&s->Buttons[1], &msg, &s->TitleFont);
}

static void EditHotkeyScreen_LeaveOpen(void* screen, void* b) {
	struct EditHotkeyScreen* s = screen;
	/* Reset 'waiting for key..' state of two other buttons */
	if (s->SelectedI == 0) {
		EditHotkeyScreen_MakeBaseKey(s, EditHotkeyScreen_BaseKey);
		s->SupressNextPress = false;
	} else if (s->SelectedI == 1) {
		EditHotkeyScreen_MakeModifiers(s, EditHotkeyScreen_Modifiers);
		s->SupressNextPress = false;
	}

	s->SelectedI = -1;
	s->CurHotkey.StaysOpen = !s->CurHotkey.StaysOpen;
	EditHotkeyScreen_MakeLeaveOpen(s, EditHotkeyScreen_LeaveOpen);
}

static void EditHotkeyScreen_SaveChanges(void* screen, void* b) {
	struct EditHotkeyScreen* s = screen;
	struct HotkeyData hotkey = s->OrigHotkey;

	if (hotkey.Trigger != Key_None) {
		Hotkeys_Remove(hotkey.Trigger, hotkey.Flags);
		Hotkeys_UserRemovedHotkey(hotkey.Trigger, hotkey.Flags);
	}

	hotkey = s->CurHotkey;
	if (hotkey.Trigger != Key_None) {
		String text = s->Input.Base.Text;
		Hotkeys_Add(hotkey.Trigger, hotkey.Flags, &text, hotkey.StaysOpen);
		Hotkeys_UserAddedHotkey(hotkey.Trigger, hotkey.Flags, hotkey.StaysOpen, &text);
	}

	Gui_FreeActive();
	Gui_SetActive(HotkeyListScreen_MakeInstance());
}

static void EditHotkeyScreen_RemoveHotkey(void* screen, void* b) {
	struct EditHotkeyScreen* s = screen;
	struct HotkeyData hotkey = s->OrigHotkey;

	if (hotkey.Trigger != Key_None) {
		Hotkeys_Remove(hotkey.Trigger, hotkey.Flags);
		Hotkeys_UserRemovedHotkey(hotkey.Trigger, hotkey.Flags);
	}

	Gui_FreeActive();
	Gui_SetActive(HotkeyListScreen_MakeInstance());
}

static void EditHotkeyScreen_Init(void* screen) {
	MenuScreen_Init(screen);
	Key_KeyRepeat = true;
}

static void EditHotkeyScreen_Render(void* screen, double delta) {
	MenuScreen_Render(screen, delta);
	int cX = Game_Width / 2, cY = Game_Height / 2;
	PackedCol grey = PACKEDCOL_CONST(150, 150, 150, 255);
	GfxCommon_Draw2DFlat(cX - 250, cY - 65, 500, 2, grey);
	GfxCommon_Draw2DFlat(cX - 250, cY + 45, 500, 2, grey);
}

static void EditHotkeyScreen_Free(void* screen) {
	struct EditHotkeyScreen* s = screen;
	Key_KeyRepeat = false;
	s->SelectedI = -1;
	MenuScreen_Free(s);
}

static bool EditHotkeyScreen_KeyPress(void* screen, char keyChar) {
	struct EditHotkeyScreen* s = screen;
	if (s->SupressNextPress) {
		s->SupressNextPress = false;
		return true;
	}
	return Elem_HandlesKeyPress(&s->Input.Base, keyChar);
}

static bool EditHotkeyScreen_KeyDown(void* screen, Key key) {
	struct EditHotkeyScreen* s = screen;
	if (s->SelectedI >= 0) {
		if (s->SelectedI == 0) {
			s->CurHotkey.Trigger = key;
			EditHotkeyScreen_MakeBaseKey(s, EditHotkeyScreen_BaseKey);
		} else if (s->SelectedI == 1) {
			if      (key == Key_ControlLeft || key == Key_ControlRight) s->CurHotkey.Flags |= HOTKEY_FLAG_CTRL;
			else if (key == Key_ShiftLeft   || key == Key_ShiftRight)   s->CurHotkey.Flags |= HOTKEY_FLAG_SHIFT;
			else if (key == Key_AltLeft     || key == Key_AltRight)     s->CurHotkey.Flags |= HOTKEY_FLAG_ALT;
			else s->CurHotkey.Flags = 0;

			EditHotkeyScreen_MakeModifiers(s, EditHotkeyScreen_Modifiers);
		}

		s->SupressNextPress = true;
		s->SelectedI = -1;
		return true;
	}
	return Elem_HandlesKeyDown(&s->Input.Base, key) || MenuScreen_KeyDown(s, key);
}

static bool EditHotkeyScreen_KeyUp(void* screen, Key key) {
	struct EditHotkeyScreen* s = screen;
	return Elem_HandlesKeyUp(&s->Input.Base, key);
}

static void EditHotkeyScreen_ContextRecreated(void* screen) {
	struct EditHotkeyScreen* s = screen;
	struct MenuInputValidator v = MenuInputValidator_String();
	String text = String_Empty;

	bool existed = s->OrigHotkey.Trigger != Key_None;
	if (existed) {
		text = StringsBuffer_UNSAFE_Get(&HotkeysText, s->OrigHotkey.TextIndex);
	}

	EditHotkeyScreen_MakeBaseKey(s,   EditHotkeyScreen_BaseKey);
	EditHotkeyScreen_MakeModifiers(s, EditHotkeyScreen_Modifiers);
	EditHotkeyScreen_MakeLeaveOpen(s, EditHotkeyScreen_LeaveOpen);

	String addText = String_FromReadonly(existed ? "Save changes" : "Add hotkey");
	EditHotkeyScreen_Make(s, 3, 0, 80, &addText, EditHotkeyScreen_SaveChanges);
	String remText = String_FromReadonly(existed ? "Remove hotkey" : "Cancel");
	EditHotkeyScreen_Make(s, 4, 0, 130, &remText, EditHotkeyScreen_RemoveHotkey);

	Menu_Back(s, 5, &s->Buttons[5], "Cancel", &s->TitleFont, Menu_SwitchHotkeys);

	Menu_Input(s, 6, &s->Input, 500, &text, &s->TextFont, &v,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -35);
}

struct ScreenVTABLE EditHotkeyScreen_VTABLE = {
	EditHotkeyScreen_Init,    EditHotkeyScreen_Render, EditHotkeyScreen_Free,     Gui_DefaultRecreate,
	EditHotkeyScreen_KeyDown, EditHotkeyScreen_KeyUp,  EditHotkeyScreen_KeyPress,
	Menu_MouseDown,           Menu_MouseUp,            Menu_MouseMove,            MenuScreen_MouseScroll,
	Menu_OnResize,            Menu_ContextLost,        EditHotkeyScreen_ContextRecreated,
};
struct Screen* EditHotkeyScreen_MakeInstance(struct HotkeyData original) {
	static struct Widget* widgets[7];
	struct EditHotkeyScreen* s = &EditHotkeyScreen_Instance;

	s->HandlesAllInput = true;
	s->Widgets         = widgets;
	s->WidgetsCount    = Array_Elems(widgets);

	s->VTABLE    = &EditHotkeyScreen_VTABLE;
	s->SelectedI = -1;
	s->OrigHotkey = original;
	s->CurHotkey  = original;
	return (struct Screen*)s;
}


/*########################################################################################################################*
*-----------------------------------------------------GenLevelScreen------------------------------------------------------*
*#########################################################################################################################*/
struct GenLevelScreen GenLevelScreen_Instance;
NOINLINE_ static int GenLevelScreen_GetInt(struct GenLevelScreen* s, int index) {
	struct MenuInputWidget* input = &s->Inputs[index];
	struct MenuInputValidator* v;
	String text = input->Base.Text;
	int value;

	v = &input->Validator;
	if (!v->VTABLE->IsValidValue(v, &text)) return 0;
	Convert_TryParseInt(&text, &value); return value;
}

NOINLINE_ static int GenLevelScreen_GetSeedInt(struct GenLevelScreen* s, int index) {
	struct MenuInputWidget* input = &s->Inputs[index];
	RNGState rnd;

	if (!input->Base.Text.length) {
		Random_InitFromCurrentTime(&rnd);
		return Random_Next(&rnd, Int32_MaxValue);
	}
	return GenLevelScreen_GetInt(s, index);
}

static void GenLevelScreen_Gen(void* screen, bool vanilla) {
	struct GenLevelScreen* s = screen;
	int width  = GenLevelScreen_GetInt(s, 0);
	int height = GenLevelScreen_GetInt(s, 1);
	int length = GenLevelScreen_GetInt(s, 2);
	int seed   = GenLevelScreen_GetSeedInt(s, 3);

	uint64_t volume = (uint64_t)width * height * length;
	if (volume > Int32_MaxValue) {
		Chat_AddRaw("&cThe generated map's volume is too big.");
	} else if (!width || !height || !length) {
		Chat_AddRaw("&cOne of the map dimensions is invalid.");
	} else {
		Gen_SetDimensions(width, height, length); 
		Gen_Vanilla = vanilla; Gen_Seed = seed;

		Gui_FreeActive();
		Gui_SetActive(GeneratingScreen_MakeInstance());
	}
}

static void GenLevelScreen_Flatgrass(void* a, void* b) { GenLevelScreen_Gen(a, false); }
static void GenLevelScreen_Notchy(void* a, void* b)    { GenLevelScreen_Gen(a, true);  }

static void GenLevelScreen_InputClick(void* screen, void* input) {
	struct GenLevelScreen* s = screen;
	if (s->Selected) s->Selected->Base.ShowCaret = false;

	s->Selected = input;
	Elem_HandlesMouseDown(&s->Selected->Base, Mouse_X, Mouse_Y, MouseButton_Left);
	s->Selected->Base.ShowCaret = true;
}

static void GenLevelScreen_Input(struct GenLevelScreen* s, int i, int y, bool seed, String* value) {
	struct MenuInputWidget* input = &s->Inputs[i];
	struct MenuInputValidator v = seed ? MenuInputValidator_Seed() : MenuInputValidator_Int(1, 8192);

	Menu_Input(s, i, input, 200, value, &s->TextFont, &v,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, y);

	input->Base.ShowCaret = false;
	input->Base.MenuClick = GenLevelScreen_InputClick;
	value->length = 0;
}

static void GenLevelScreen_Label(struct GenLevelScreen* s, int i, int x, int y, const char* title) {
	struct TextWidget* label = &s->Labels[i];

	String text = String_FromReadonly(title);
	Menu_Label(s, i + 4, label, &text, &s->TextFont,
		ANCHOR_CENTRE, ANCHOR_CENTRE, x, y);

	label->XOffset = -110 - label->Width / 2;
	Widget_Reposition(label);
	PackedCol col = PACKEDCOL_CONST(224, 224, 224, 255); label->Col = col;
}

static void GenLevelScreen_Init(void* screen) {
	struct GenLevelScreen* s = screen;
	Key_KeyRepeat = true;
	MenuScreen_Init(s);
}

static void GenLevelScreen_Free(void* screen) {
	struct GenLevelScreen* s = screen;
	Key_KeyRepeat = false;
	MenuScreen_Free(s);
}

static bool GenLevelScreen_KeyDown(void* screen, Key key) {
	struct GenLevelScreen* s = screen;
	if (s->Selected && Elem_HandlesKeyDown(&s->Selected->Base, key)) return true;
	return MenuScreen_KeyDown(s, key);
}

static bool GenLevelScreen_KeyUp(void* screen, Key key) {
	struct GenLevelScreen* s = screen;
	return !s->Selected || Elem_HandlesKeyUp(&s->Selected->Base, key);
}

static bool GenLevelScreen_KeyPress(void* screen, char keyChar) {
	struct GenLevelScreen* s = screen;
	return !s->Selected || Elem_HandlesKeyPress(&s->Selected->Base, keyChar);
}

static void GenLevelScreen_ContextRecreated(void* screen) {
	static String title = String_FromConst("Generate new level");
	static String flat  = String_FromConst("Flatgrass");
	static String norm  = String_FromConst("Vanilla");

	struct GenLevelScreen* s = screen;
	char tmpBuffer[STRING_SIZE];
	String tmp = String_FromArray(tmpBuffer);

	String_AppendInt(&tmp, World_Width);
	GenLevelScreen_Input(s, 0, -80, false, &tmp);
	String_AppendInt(&tmp, World_Height);
	GenLevelScreen_Input(s, 1, -40, false, &tmp);
	String_AppendInt(&tmp, World_Length);
	GenLevelScreen_Input(s, 2,   0, false, &tmp);
	GenLevelScreen_Input(s, 3,  40, true,  &tmp);

	GenLevelScreen_Label(s, 0, -150, -80, "Width:");
	GenLevelScreen_Label(s, 1, -150, -40, "Height:");
	GenLevelScreen_Label(s, 2, -150,   0, "Length:");
	GenLevelScreen_Label(s, 3, -140,  40, "Seed:");
	
	Menu_Label(s,   8, &s->Labels[4], &title, &s->TextFont,
		ANCHOR_CENTRE, ANCHOR_CENTRE,    0, -130);
	Menu_Button(s,  9, &s->Buttons[0], 200, &flat, &s->TitleFont, GenLevelScreen_Flatgrass,
		ANCHOR_CENTRE, ANCHOR_CENTRE, -120,  100);
	Menu_Button(s, 10, &s->Buttons[1], 200, &norm, &s->TitleFont, GenLevelScreen_Notchy,
		ANCHOR_CENTRE, ANCHOR_CENTRE,  120,  100);

	Menu_Back(s, 11, &s->Buttons[2], "Cancel", &s->TitleFont, Menu_SwitchPause);
}

struct ScreenVTABLE GenLevelScreen_VTABLE = {
	GenLevelScreen_Init,    MenuScreen_Render,    GenLevelScreen_Free,     Gui_DefaultRecreate,
	GenLevelScreen_KeyDown, GenLevelScreen_KeyUp, GenLevelScreen_KeyPress,
	Menu_MouseDown,         Menu_MouseUp,         Menu_MouseMove,          MenuScreen_MouseScroll,
	Menu_OnResize,          Menu_ContextLost,     GenLevelScreen_ContextRecreated,
};
struct Screen* GenLevelScreen_MakeInstance(void) {
	static struct Widget* widgets[12];
	struct GenLevelScreen* s = &GenLevelScreen_Instance;

	s->HandlesAllInput = true;
	s->Widgets         = widgets;
	s->WidgetsCount    = Array_Elems(widgets);

	s->VTABLE = &GenLevelScreen_VTABLE;
	return (struct Screen*)s;
}


/*########################################################################################################################*
*----------------------------------------------------ClassicGenScreen-----------------------------------------------------*
*#########################################################################################################################*/
struct ClassicGenScreen ClassicGenScreen_Instance;
static void ClassicGenScreen_Gen(int size) {
	RNGState rnd; Random_InitFromCurrentTime(&rnd);
	Gen_SetDimensions(size, 64, size); Gen_Vanilla = true;
	Gen_Seed = Random_Next(&rnd, Int32_MaxValue);

	Gui_FreeActive();
	Gui_SetActive(GeneratingScreen_MakeInstance());
}

static void ClassicGenScreen_Small(void* a, void* b)  { ClassicGenScreen_Gen(128); }
static void ClassicGenScreen_Medium(void* a, void* b) { ClassicGenScreen_Gen(256); }
static void ClassicGenScreen_Huge(void* a, void* b)   { ClassicGenScreen_Gen(512); }

static void ClassicGenScreen_Make(struct ClassicGenScreen* s, int i, int y, const char* title, Widget_LeftClick onClick) {
	String text = String_FromReadonly(title);
	Menu_Button(s, i, &s->Buttons[i], 400, &text, &s->TitleFont, onClick,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, y);
}

static void ClassicGenScreen_ContextRecreated(void* screen) {
	struct ClassicGenScreen* s = screen;
	ClassicGenScreen_Make(s, 0, -100, "Small",  ClassicGenScreen_Small);
	ClassicGenScreen_Make(s, 1,  -50, "Normal", ClassicGenScreen_Medium);
	ClassicGenScreen_Make(s, 2,    0, "Huge",   ClassicGenScreen_Huge);

	Menu_Back(s, 3, &s->Buttons[3], "Cancel", &s->TitleFont, Menu_SwitchPause);
}

struct ScreenVTABLE ClassicGenScreen_VTABLE = {
	MenuScreen_Init,    MenuScreen_Render,  MenuScreen_Free, Gui_DefaultRecreate,
	MenuScreen_KeyDown, Menu_KeyUp,         Menu_KeyPress,
	Menu_MouseDown,     Menu_MouseUp,       Menu_MouseMove,  MenuScreen_MouseScroll,
	Menu_OnResize,      Menu_ContextLost,   ClassicGenScreen_ContextRecreated,
};
struct Screen* ClassicGenScreen_MakeInstance(void) {
	static struct Widget* widgets[4];
	struct ClassicGenScreen* s = &ClassicGenScreen_Instance;

	s->HandlesAllInput = true;
	s->Widgets         = widgets;
	s->WidgetsCount    = Array_Elems(widgets);

	s->VTABLE = &ClassicGenScreen_VTABLE;
	return (struct Screen*)s;
}


/*########################################################################################################################*
*----------------------------------------------------SaveLevelScreen------------------------------------------------------*
*#########################################################################################################################*/
struct SaveLevelScreen SaveLevelScreen_Instance;
static void SaveLevelScreen_RemoveOverwrites(struct SaveLevelScreen* s) {
	static String save  = String_FromConst("Save");
	static String schem = String_FromConst("Save schematic");
	struct ButtonWidget* btn;
		
	btn = &s->Buttons[0];
	if (btn->OptName) {
		btn->OptName = NULL; 
		ButtonWidget_Set(btn, &save,  &s->TitleFont);
	}

	btn = &s->Buttons[1];
	if (btn->OptName) {
		btn->OptName = NULL;
		ButtonWidget_Set(btn, &schem, &s->TitleFont);
	}
}

static void SaveLevelScreen_MakeDesc(struct SaveLevelScreen* s, const String* text) {
	if (s->Widgets[5]) { Elem_TryFree(s->Widgets[5]); }

	Menu_Label(s, 5, &s->Desc, text, &s->TextFont,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 65);
}

static void SaveLevelScreen_Save(void* screen, void* widget, const char* ext) {
	static String overMsg = String_FromConst("&cOverwrite existing?");
	static String saveMsg = String_FromConst("Saving..");
	static String fileMsg = String_FromConst("&ePlease enter a filename");

	char pathBuffer[FILENAME_SIZE];
	String path = String_FromArray(pathBuffer);

	struct SaveLevelScreen* s = screen;
	struct ButtonWidget* btn  = widget;
	String file = s->Input.Base.Text;

	if (!file.length) {
		SaveLevelScreen_MakeDesc(s, &fileMsg); return;
	}
	String_Format3(&path, "maps%r%s%c", &Directory_Separator, &file, ext);

	if (File_Exists(&path) && !btn->OptName) {
		ButtonWidget_Set(btn, &overMsg, &s->TitleFont);
		btn->OptName = "O";
	} else {
		/* NOTE: We don't immediately save here, because otherwise the 'saving...'
		will not be rendered in time because saving is done on the main thread. */
		SaveLevelScreen_MakeDesc(s, &saveMsg);

		String_Copy(&s->TextPath, &path);
		SaveLevelScreen_RemoveOverwrites(s);
	}
}
static void SaveLevelScreen_Classic(void* a, void* b)   { SaveLevelScreen_Save(a, b, ".cw"); }
static void SaveLevelScreen_Schematic(void* a, void* b) { SaveLevelScreen_Save(a, b, ".schematic"); }

static void SaveLevelScreen_Init(void* screen) {
	struct SaveLevelScreen* s = screen;
	String str = String_FromArray(s->__TextPathBuffer);
	s->TextPath = str;

	Key_KeyRepeat = true;
	MenuScreen_Init(s);
}

static void SaveLevelScreen_SaveMap(struct SaveLevelScreen* s) {
	String path = s->TextPath, cw = String_FromConst(".cw");

	struct Stream stream;
	ReturnCode res;

	res = Stream_CreateFile(&stream, &path);
	if (res) { Chat_LogError2(res, "creating", &path); return; }

	struct Stream compStream;
	struct GZipState state;
	GZip_MakeStream(&compStream, &state, &stream);

	if (String_CaselessEnds(&path, &cw)) {
		res = Cw_Save(&compStream);
	} else {
		res = Schematic_Save(&compStream);
	}

	if (res) {
		stream.Close(&stream);
		Chat_LogError2(res, "encoding", &path); return;
	}

	if ((res = compStream.Close(&compStream))) {
		stream.Close(&stream);
		Chat_LogError2(res, "closing", &path); return;
	}

	res = stream.Close(&stream);
	if (res) { Chat_LogError2(res, "closing", &path); return; }

	Chat_Add1("&eSaved map to: %s", &path);
	s->TextPath.length = 0;
	Gui_FreeActive();
	Gui_SetActive(PauseScreen_MakeInstance());
}

static void SaveLevelScreen_Render(void* screen, double delta) {
	struct SaveLevelScreen* s = screen;
	MenuScreen_Render(s, delta);
	int cX = Game_Width / 2, cY = Game_Height / 2;
	PackedCol grey = PACKEDCOL_CONST(150, 150, 150, 255);
	GfxCommon_Draw2DFlat(cX - 250, cY + 90, 500, 2, grey);

	if (!s->TextPath.length) return;
	SaveLevelScreen_SaveMap(s);
}

static void SaveLevelScreen_Free(void* screen) {
	struct SaveLevelScreen* s = screen;
	Key_KeyRepeat = false;
	MenuScreen_Free(s);
}

static bool SaveLevelScreen_KeyPress(void* screen, char keyChar) {
	struct SaveLevelScreen* s = screen;
	SaveLevelScreen_RemoveOverwrites(s);
	return Elem_HandlesKeyPress(&s->Input.Base, keyChar);
}

static bool SaveLevelScreen_KeyDown(void* screen, Key key) {
	struct SaveLevelScreen* s = screen;
	SaveLevelScreen_RemoveOverwrites(s);
	if (Elem_HandlesKeyDown(&s->Input.Base, key)) return true;
	return MenuScreen_KeyDown(s, key);
}

static bool SaveLevelScreen_KeyUp(void* screen, Key key) {
	struct SaveLevelScreen* s = screen;
	return Elem_HandlesKeyUp(&s->Input.Base, key);
}

static void SaveLevelScreen_ContextRecreated(void* screen) {
	static String save   = String_FromConst("Save");
	static String schem  = String_FromConst("Save schematic");
	static String mcEdit = String_FromConst("&eCan be imported into MCEdit");

	struct SaveLevelScreen* s = screen;
	struct MenuInputValidator validator = MenuInputValidator_Path();
	
	Menu_Button(s, 0, &s->Buttons[0], 300, &save,  &s->TitleFont, SaveLevelScreen_Classic,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 20);
	Menu_Button(s, 1, &s->Buttons[1], 200, &schem, &s->TitleFont, SaveLevelScreen_Schematic,
		ANCHOR_CENTRE, ANCHOR_CENTRE, -150, 120);
	Menu_Label(s,  2, &s->MCEdit, &mcEdit,         &s->TextFont,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 110, 120);

	Menu_Back(s,   3, &s->Buttons[2], "Cancel",      &s->TitleFont, Menu_SwitchPause);
	Menu_Input(s,  4, &s->Input, 500, &String_Empty, &s->TextFont, &validator, 
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -30);
	s->Widgets[5] = NULL; /* description widget placeholder */
}

struct ScreenVTABLE SaveLevelScreen_VTABLE = {
	SaveLevelScreen_Init,    SaveLevelScreen_Render, SaveLevelScreen_Free,     Gui_DefaultRecreate,
	SaveLevelScreen_KeyDown, SaveLevelScreen_KeyUp,  SaveLevelScreen_KeyPress,
	Menu_MouseDown,          Menu_MouseUp,           Menu_MouseMove,           MenuScreen_MouseScroll,
	Menu_OnResize,           Menu_ContextLost,       SaveLevelScreen_ContextRecreated,
};
struct Screen* SaveLevelScreen_MakeInstance(void) {
	static struct Widget* widgets[6];
	struct SaveLevelScreen* s = &SaveLevelScreen_Instance;
	
	s->HandlesAllInput = true;
	s->Widgets         = widgets;
	s->WidgetsCount    = Array_Elems(widgets);

	s->VTABLE = &SaveLevelScreen_VTABLE;
	return (struct Screen*)s;
}


/*########################################################################################################################*
*---------------------------------------------------TexturePackScreen-----------------------------------------------------*
*#########################################################################################################################*/
static void TexturePackScreen_EntryClick(void* screen, void* widget) {
	struct ListScreen* s = screen;
	String filename;
	int idx;
	char pathBuffer[FILENAME_SIZE];
	String path = String_FromArray(pathBuffer);

	filename = ListScreen_UNSAFE_GetCur(s, widget);
	String_Format2(&path, "texpacks%r%s", &Directory_Separator, &filename);
	if (!File_Exists(&path)) return;
	
	idx = s->CurrentIndex;
	Game_SetDefaultTexturePack(&filename);
	TexturePack_ExtractDefault();
	Elem_Recreate(s);
	ListScreen_SetCurrentIndex(s, idx);
}

static void TexturePackScreen_FilterFiles(const String* path, void* obj) {
	static String zip = String_FromConst(".zip");
	String file = *path;
	if (!String_CaselessEnds(path, &zip)) return;

	Utils_UNSAFE_GetFilename(&file);
	StringsBuffer_Add((StringsBuffer*)obj, &file);
}

struct Screen* TexturePackScreen_MakeInstance(void) {
	static String title = String_FromConst("Select a texture pack zip");
	static String path  = String_FromConst("texpacks");

	struct ListScreen* s = ListScreen_MakeInstance();
	s->TitleText  = title;
	s->EntryClick = TexturePackScreen_EntryClick;
	
	Directory_Enum(&path, &s->Entries, TexturePackScreen_FilterFiles);
	if (s->Entries.Count) {
		ListScreen_QuickSort(0, s->Entries.Count - 1);
	}
	return (struct Screen*)s;
}


/*########################################################################################################################*
*----------------------------------------------------FontListScreen-------------------------------------------------------*
*#########################################################################################################################*/
static void FontListScreen_EntryClick(void* screen, void* widget) {
	struct ListScreen* s = screen;
	String fontName = ListScreen_UNSAFE_GetCur(s, widget);
	int cur = s->CurrentIndex;

	if (String_CaselessEqualsConst(&fontName, LIST_SCREEN_EMPTY)) return;
	String_Copy(&Game_FontName, &fontName);
	Options_Set(OPT_FONT_NAME,  &fontName);

	/* changing font recreates list menu */
	Menu_HandleFontChange((struct Screen*)s);
	ListScreen_SetCurrentIndex(s, cur);
}

static void FontListScreen_UpdateEntry(struct ListScreen* s, struct ButtonWidget* button, const String* text) {
	FontDesc font = { 0 };
	if (String_CaselessEqualsConst(text, LIST_SCREEN_EMPTY)) {
		ButtonWidget_Set(button, text, &s->Font);
	} else {
		Font_Make(&font, text, 16, FONT_STYLE_NORMAL);
		ButtonWidget_Set(button, text, &font);
		Font_Free(&font);
	}
}

static void FontListScreen_Init(void* screen) {
	struct ListScreen* s = screen;
	ListScreen_Init(s);
	ListScreen_Select(s, &Game_FontName);
}

struct ScreenVTABLE FontListScreen_VTABLE = {
	FontListScreen_Init, ListScreen_Render, ListScreen_Free,     Gui_DefaultRecreate,
	ListScreen_KeyDown,  Menu_KeyUp,        Menu_KeyPress,
	Menu_MouseDown,      Menu_MouseUp,      Menu_MouseMove,      ListScreen_MouseScroll,
	Menu_OnResize,       Menu_ContextLost,  ListScreen_ContextRecreated,
};
struct Screen* FontListScreen_MakeInstance(void) {
	static String title  = String_FromConst("Select a font");
	struct ListScreen* s = ListScreen_MakeInstance();

	s->TitleText   = title;
	s->EntryClick  = FontListScreen_EntryClick;
	s->UpdateEntry = FontListScreen_UpdateEntry;
	s->VTABLE      = &FontListScreen_VTABLE;

	Font_GetNames(&s->Entries);
	if (s->Entries.Count) {
		ListScreen_QuickSort(0, s->Entries.Count - 1);
	}
	return (struct Screen*)s;
}


/*########################################################################################################################*
*---------------------------------------------------HotkeyListScreen------------------------------------------------------*
*#########################################################################################################################*/
/* TODO: Hotkey added event for CPE */
static void HotkeyListScreen_EntryClick(void* screen, void* widget) {
	static String ctrl  = String_FromConst("Ctrl");
	static String shift = String_FromConst("Shift");
	static String alt   = String_FromConst("Alt");

	struct ListScreen* s = screen;
	String text = ListScreen_UNSAFE_GetCur(s, widget);
	struct HotkeyData original = { 0 };

	if (String_CaselessEqualsConst(&text, LIST_SCREEN_EMPTY)) {
		Gui_FreeActive();
		Gui_SetActive(EditHotkeyScreen_MakeInstance(original)); 
		return;
	}

	String key, value;
	int flags = 0;

	String_UNSAFE_Separate(&text, '+', &key, &value);
	if (String_ContainsString(&value, &ctrl))  flags |= HOTKEY_FLAG_CTRL;
	if (String_ContainsString(&value, &shift)) flags |= HOTKEY_FLAG_SHIFT;
	if (String_ContainsString(&value, &alt))   flags |= HOTKEY_FLAG_ALT;

	Key trigger = Utils_ParseEnum(&key, Key_None, Key_Names, Key_Count);
	int i;
	for (i = 0; i < HotkeysText.Count; i++) {
		struct HotkeyData h = HotkeysList[i];
		if (h.Trigger == trigger && h.Flags == flags) { original = h; break; }
	}

	Gui_FreeActive();
	Gui_SetActive(EditHotkeyScreen_MakeInstance(original));
}

static void HotkeyListScreen_MakeFlags(int flags, String* str) {
	if (flags & HOTKEY_FLAG_CTRL)  String_AppendConst(str, " Ctrl");
	if (flags & HOTKEY_FLAG_SHIFT) String_AppendConst(str, " Shift");
	if (flags & HOTKEY_FLAG_ALT)   String_AppendConst(str, " Alt");
}

struct Screen* HotkeyListScreen_MakeInstance(void) {
	static String title = String_FromConst("Modify hotkeys");
	static String empty = String_FromConst(LIST_SCREEN_EMPTY);

	char textBuffer[STRING_SIZE];
	String text = String_FromArray(textBuffer);
	int i;

	struct ListScreen* s = ListScreen_MakeInstance();
	s->TitleText  = title;
	s->EntryClick = HotkeyListScreen_EntryClick;

	for (i = 0; i < HotkeysText.Count; i++) {
		struct HotkeyData hKey = HotkeysList[i];
		text.length = 0;
		String_AppendConst(&text, Key_Names[hKey.Trigger]);

		if (hKey.Flags) {
			String_AppendConst(&text, " +");
			HotkeyListScreen_MakeFlags(hKey.Flags, &text);
		}
		StringsBuffer_Add(&s->Entries, &text);
	}
	
	for (i = 0; i < LIST_SCREEN_ITEMS; i++) {
		StringsBuffer_Add(&s->Entries, &empty);
	}
	return (struct Screen*)s;
}


/*########################################################################################################################*
*----------------------------------------------------LoadLevelScreen------------------------------------------------------*
*#########################################################################################################################*/
static void LoadLevelScreen_FilterFiles(const String* path, void* obj) {
	IMapImporter importer = Map_FindImporter(path);
	String file = *path;
	if (!importer) return;

	Utils_UNSAFE_GetFilename(&file);
	StringsBuffer_Add((StringsBuffer*)obj, &file);
}

void LoadLevelScreen_LoadMap(const String* path) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct LocationUpdate update;
	IMapImporter importer;
	struct Stream stream;
	ReturnCode res;

	World_Reset();
	Event_RaiseVoid(&WorldEvents_NewMap);

	if (World_TextureUrl.length) {
		TexturePack_ExtractDefault();
		World_TextureUrl.length = 0;
	}

	Block_Reset();
	Inventory_SetDefaultMapping();
	
	res = Stream_OpenFile(&stream, path);
	if (res) { Chat_LogError2(res, "opening", path); return; }

	importer = Map_FindImporter(path);
	if ((res = importer(&stream))) {
		World_Reset();
		Chat_LogError2(res, "decoding", path); stream.Close(&stream); return;
	}

	res = stream.Close(&stream);
	if (res) { Chat_LogError2(res, "closing", path); }

	World_SetNewMap(World_Blocks, World_BlocksSize, World_Width, World_Height, World_Length);
	Event_RaiseVoid(&WorldEvents_MapLoaded);

	LocationUpdate_MakePosAndOri(&update, p->Spawn, p->SpawnRotY, p->SpawnHeadX, false);
	p->Base.VTABLE->SetLocation(&p->Base, &update, false);
}

static void LoadLevelScreen_EntryClick(void* screen, void* widget) {
	struct ListScreen* s = screen;
	String filename;
	char pathBuffer[FILENAME_SIZE];
	String path = String_FromArray(pathBuffer);

	filename = ListScreen_UNSAFE_GetCur(s, widget);
	String_Format2(&path, "maps%r%s", &Directory_Separator, &filename);
	if (!File_Exists(&path)) return;
	LoadLevelScreen_LoadMap(&path);
}

struct Screen* LoadLevelScreen_MakeInstance(void) {
	static String title  = String_FromConst("Select a level");
	static String path   = String_FromConst("maps");
	struct ListScreen* s = ListScreen_MakeInstance();

	s->TitleText  = title;
	s->EntryClick = LoadLevelScreen_EntryClick;
	
	Directory_Enum(&path, &s->Entries, LoadLevelScreen_FilterFiles);
	if (s->Entries.Count) {
		ListScreen_QuickSort(0, s->Entries.Count - 1);
	}
	return (struct Screen*)s;
}


/*########################################################################################################################*
*---------------------------------------------------KeyBindingsScreen-----------------------------------------------------*
*#########################################################################################################################*/
struct KeyBindingsScreen KeyBindingsScreen_Instance;
static void KeyBindingsScreen_GetText(struct KeyBindingsScreen* s, int i, String* text) {
	Key key = KeyBind_Get(s->Binds[i]);
	String_Format2(text, "%c: %c", s->Descs[i], Key_Names[key]);
}

static void KeyBindingsScreen_OnBindingClick(void* screen, void* widget) {
	struct KeyBindingsScreen* s = screen;
	struct ButtonWidget* btn    = widget;
	struct ButtonWidget* cur;
	char textBuffer[STRING_SIZE];
	String text = String_FromArray(textBuffer);

	/* previously selected a different button for binding */
	if (s->CurI >= 0) {
		KeyBindingsScreen_GetText(s, s->CurI, &text);
		cur = (struct ButtonWidget*)s->Widgets[s->CurI];
		ButtonWidget_Set(cur, &text, &s->TitleFont);
	}
	s->CurI = Menu_Index(s, btn);

	text.length = 0;
	String_AppendConst(&text, "> ");
	KeyBindingsScreen_GetText(s, s->CurI, &text);
	String_AppendConst(&text, " <");
	ButtonWidget_Set(btn, &text, &s->TitleFont);
}

static int KeyBindingsScreen_MakeWidgets(struct KeyBindingsScreen* s, int y, int arrowsY, int leftLength, const char* title, int btnWidth) {
	static String lArrow = String_FromConst("<");
	static String rArrow = String_FromConst(">");

	int i, origin = y, xOffset = btnWidth / 2 + 5;
	s->CurI = -1;

	char textBuffer[STRING_SIZE];
	String text = String_FromArray(textBuffer);

	for (i = 0; i < s->BindsCount; i++) {
		if (i == leftLength) y = origin; /* reset y for next column */
		int xDir = leftLength == -1 ? 0 : (i < leftLength ? -1 : 1);

		text.length = 0;
		KeyBindingsScreen_GetText(s, i, &text);

		Menu_Button(s, i, &s->Buttons[i], btnWidth, &text, &s->TitleFont, KeyBindingsScreen_OnBindingClick,
			ANCHOR_CENTRE, ANCHOR_CENTRE, xDir * xOffset, y);
		y += 50; /* distance between buttons */
	}

	String titleText = String_FromReadonly(title);
	Menu_Label(s, i, &s->Title, &titleText, &s->TitleFont,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -180); i++;

	Widget_LeftClick backClick = Game_UseClassicOptions ? Menu_SwitchClassicOptions : Menu_SwitchOptions;
	Menu_Back(s, i, &s->Back, "Done", &s->TitleFont, backClick); i++;
	if (!s->LeftPage && !s->RightPage) return i;
	
	Menu_Button(s, i, &s->Left,  40, &lArrow, &s->TitleFont, s->LeftPage,
		ANCHOR_CENTRE, ANCHOR_CENTRE, -btnWidth - 35, arrowsY); i++;
	Menu_Button(s, i, &s->Right, 40, &rArrow, &s->TitleFont, s->RightPage,
		ANCHOR_CENTRE, ANCHOR_CENTRE,  btnWidth + 35, arrowsY); i++;

	s->Left.Disabled  = !s->LeftPage;
	s->Right.Disabled = !s->RightPage;
	return i;
}

static bool KeyBindingsScreen_KeyDown(void* screen, Key key) {
	struct KeyBindingsScreen* s = screen;
	struct ButtonWidget* cur;
	KeyBind bind;
	if (s->CurI == -1) return MenuScreen_KeyDown(s, key);

	bind = s->Binds[s->CurI];
	if (key == Key_Escape) key = KeyBind_GetDefault(bind);
	KeyBind_Set(bind, key);

	char textBuffer[STRING_SIZE];
	String text = String_FromArray(textBuffer);
	KeyBindingsScreen_GetText(s, s->CurI, &text);

	cur = (struct ButtonWidget*)s->Widgets[s->CurI];
	ButtonWidget_Set(cur, &text, &s->TitleFont);
	s->CurI = -1;
	return true;
}

static bool KeyBindingsScreen_MouseDown(void* screen, int x, int y, MouseButton btn) {
	struct KeyBindingsScreen* s = screen;
	if (btn != MouseButton_Right) { return Menu_MouseDown(s, x, y, btn); }
	int i = Menu_DoMouseDown(s, x, y, btn);
	if (i == -1) return false;

	/* Reset a key binding */
	if ((s->CurI == -1 || s->CurI == i) && i < s->BindsCount) {
		s->CurI = i;
		Elem_HandlesKeyDown(s, KeyBind_GetDefault(s->Binds[i]));
	}
	return true;
}

struct ScreenVTABLE KeyBindingsScreen_VTABLE = {
	MenuScreen_Init,             MenuScreen_Render,  MenuScreen_Free, Gui_DefaultRecreate,
	KeyBindingsScreen_KeyDown,   Menu_KeyUp,         Menu_KeyPress,
	KeyBindingsScreen_MouseDown, Menu_MouseUp,       Menu_MouseMove,  MenuScreen_MouseScroll,
	Menu_OnResize,               Menu_ContextLost,   NULL,
};
static struct KeyBindingsScreen* KeyBindingsScreen_Make(int bindsCount, uint8_t* binds, const char** descs, struct ButtonWidget* buttons, struct Widget** widgets, Event_Void_Callback contextRecreated) {
	struct KeyBindingsScreen* s = &KeyBindingsScreen_Instance;
	s->HandlesAllInput = true;
	s->Widgets         = widgets;
	s->WidgetsCount    = bindsCount + 4;

	s->VTABLE = &KeyBindingsScreen_VTABLE;
	s->VTABLE->ContextRecreated = contextRecreated;

	s->BindsCount = bindsCount;
	s->Binds      = binds;
	s->Descs      = descs;
	s->Buttons    = buttons;

	s->CurI      = -1;
	s->LeftPage  = NULL;
	s->RightPage = NULL;
	return s;
}


/*########################################################################################################################*
*-----------------------------------------------ClassicKeyBindingsScreen--------------------------------------------------*
*#########################################################################################################################*/
static void ClassicKeyBindingsScreen_ContextRecreated(void* screen) {
	struct KeyBindingsScreen* s = screen;
	if (Game_ClassicHacks) {
		KeyBindingsScreen_MakeWidgets(s, -140, -40, 5, "Normal controls", 260);
	} else {
		KeyBindingsScreen_MakeWidgets(s, -140, -40, 5, "Controls", 300);
	}
}

struct Screen* ClassicKeyBindingsScreen_MakeInstance(void) {
	static uint8_t binds[10] = { KeyBind_Forward, KeyBind_Back, KeyBind_Jump, KeyBind_Chat, KeyBind_SetSpawn, KeyBind_Left, KeyBind_Right, KeyBind_Inventory, KeyBind_ToggleFog, KeyBind_Respawn };
	static const char* descs[10] = { "Forward", "Back", "Jump", "Chat", "Save loc", "Left", "Right", "Build", "Toggle fog", "Load loc" };
	static struct ButtonWidget buttons[10];
	static struct Widget* widgets[10 + 4];

	struct KeyBindingsScreen* s = KeyBindingsScreen_Make(Array_Elems(binds), binds, descs, buttons, widgets, ClassicKeyBindingsScreen_ContextRecreated);
	if (Game_ClassicHacks) s->RightPage = Menu_SwitchKeysClassicHacks;
	return (struct Screen*)s;
}


/*########################################################################################################################*
*--------------------------------------------ClassicHacksKeyBindingsScreen------------------------------------------------*
*#########################################################################################################################*/
static void ClassicHacksKeyBindingsScreen_ContextRecreated(void* screen) {
	struct KeyBindingsScreen* s = screen;
	KeyBindingsScreen_MakeWidgets(s, -90, -40, 3, "Hacks controls", 260);
}

struct Screen* ClassicHacksKeyBindingsScreen_MakeInstance(void) {
	static uint8_t binds[6] = { KeyBind_Speed, KeyBind_NoClip, KeyBind_HalfSpeed, KeyBind_Fly, KeyBind_FlyUp, KeyBind_FlyDown };
	static const char* descs[6] = { "Speed", "Noclip", "Half speed", "Fly", "Fly up", "Fly down" };
	static struct ButtonWidget buttons[6];
	static struct Widget* widgets[6 + 4];

	struct KeyBindingsScreen* s = KeyBindingsScreen_Make(Array_Elems(binds), binds, descs, buttons, widgets, ClassicHacksKeyBindingsScreen_ContextRecreated);
	s->LeftPage = Menu_SwitchKeysClassic;
	return (struct Screen*)s;
}


/*########################################################################################################################*
*-----------------------------------------------NormalKeyBindingsScreen---------------------------------------------------*
*#########################################################################################################################*/
static void NormalKeyBindingsScreen_ContextRecreated(void* screen) {
	struct KeyBindingsScreen* s = screen;
	KeyBindingsScreen_MakeWidgets(s, -140, 10, 6, "Normal controls", 260);
}

struct Screen* NormalKeyBindingsScreen_MakeInstance(void) {
	static uint8_t binds[12] = { KeyBind_Forward, KeyBind_Back, KeyBind_Jump, KeyBind_Chat, KeyBind_SetSpawn, KeyBind_PlayerList, KeyBind_Left, KeyBind_Right, KeyBind_Inventory, KeyBind_ToggleFog, KeyBind_Respawn, KeyBind_SendChat };
	static const char* descs[12] = { "Forward", "Back", "Jump", "Chat", "Set spawn", "Player list", "Left", "Right", "Inventory", "Toggle fog", "Respawn", "Send chat" };
	static struct ButtonWidget buttons[12];
	static struct Widget* widgets[12 + 4];

	struct KeyBindingsScreen* s = KeyBindingsScreen_Make(Array_Elems(binds), binds, descs, buttons, widgets, NormalKeyBindingsScreen_ContextRecreated);
	s->RightPage = Menu_SwitchKeysHacks;
	return (struct Screen*)s;
}


/*########################################################################################################################*
*------------------------------------------------HacksKeyBindingsScreen---------------------------------------------------*
*#########################################################################################################################*/
static void HacksKeyBindingsScreen_ContextRecreated(void* screen) {
	struct KeyBindingsScreen* s = screen;
	KeyBindingsScreen_MakeWidgets(s, -40, 10, 4, "Hacks controls", 260);
}

struct Screen* HacksKeyBindingsScreen_MakeInstance(void) {
	static uint8_t binds[8] = { KeyBind_Speed, KeyBind_NoClip, KeyBind_HalfSpeed, KeyBind_ZoomScrolling, KeyBind_Fly, KeyBind_FlyUp, KeyBind_FlyDown, KeyBind_ThirdPerson };
	static const char* descs[8] = { "Speed", "Noclip", "Half speed", "Scroll zoom", "Fly", "Fly up", "Fly down", "Third person" };
	static struct ButtonWidget buttons[8];
	static struct Widget* widgets[8 + 4];

	struct KeyBindingsScreen* s = KeyBindingsScreen_Make(Array_Elems(binds), binds, descs, buttons, widgets, HacksKeyBindingsScreen_ContextRecreated);
	s->LeftPage  = Menu_SwitchKeysNormal;
	s->RightPage = Menu_SwitchKeysOther;
	return (struct Screen*)s;
}


/*########################################################################################################################*
*------------------------------------------------OtherKeyBindingsScreen---------------------------------------------------*
*#########################################################################################################################*/
static void OtherKeyBindingsScreen_ContextRecreated(void* screen) {
	struct KeyBindingsScreen* s = screen;
	KeyBindingsScreen_MakeWidgets(s, -140, 10, 6, "Other controls", 260);
}

struct Screen* OtherKeyBindingsScreen_MakeInstance(void) {
	static uint8_t binds[12] = { KeyBind_ExtInput, KeyBind_HideFps, KeyBind_HideGui, KeyBind_HotbarSwitching, KeyBind_DropBlock,KeyBind_Screenshot, KeyBind_Fullscreen, KeyBind_AxisLines, KeyBind_Autorotate, KeyBind_SmoothCamera, KeyBind_IDOverlay, KeyBind_BreakableLiquids };
	static const char* descs[12] = { "Show ext input", "Hide FPS", "Hide gui", "Hotbar switching", "Drop block", "Screenshot", "Fullscreen", "Show axis lines", "Auto-rotate", "Smooth camera", "ID overlay", "Breakable liquids" };
	static struct ButtonWidget buttons[12];
	static struct Widget* widgets[12 + 4];

	struct KeyBindingsScreen* s = KeyBindingsScreen_Make(Array_Elems(binds), binds, descs, buttons, widgets, OtherKeyBindingsScreen_ContextRecreated);
	s->LeftPage  = Menu_SwitchKeysHacks;
	s->RightPage = Menu_SwitchKeysMouse;
	return (struct Screen*)s;
}


/*########################################################################################################################*
*------------------------------------------------MouseKeyBindingsScreen---------------------------------------------------*
*#########################################################################################################################*/
static void MouseKeyBindingsScreen_ContextRecreated(void* screen) {
	static String msg = String_FromConst("&eRight click to remove the key binding");
	struct KeyBindingsScreen* s = screen;
	static struct TextWidget text;

	int i = KeyBindingsScreen_MakeWidgets(s, -40, 10, -1, "Mouse key bindings", 260);
	Menu_Label(s, i, &text, &msg, &s->TextFont,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 100);
}

struct Screen* MouseKeyBindingsScreen_MakeInstance(void) {
	static uint8_t binds[3] = { KeyBind_MouseLeft, KeyBind_MouseMiddle, KeyBind_MouseRight };
	static const char* descs[3] = { "Left", "Middle", "Right" };
	static struct ButtonWidget buttons[3];
	static struct Widget* widgets[3 + 4 + 1];

	struct KeyBindingsScreen* s = KeyBindingsScreen_Make(Array_Elems(binds), binds, descs, buttons, widgets, MouseKeyBindingsScreen_ContextRecreated);
	s->LeftPage = Menu_SwitchKeysOther;
	s->WidgetsCount++; /* Extra text widget for 'right click' message */
	return (struct Screen*)s;
}


/*########################################################################################################################*
*--------------------------------------------------MenuOptionsScreen------------------------------------------------------*
*#########################################################################################################################*/
struct MenuOptionsScreen MenuOptionsScreen_Instance;
static void Menu_GetBool(String* raw, bool v) {
	String_AppendConst(raw, v ? "ON" : "OFF");
}

static bool Menu_SetBool(const String* raw, const char* key) {
	bool isOn = String_CaselessEqualsConst(raw, "ON");
	Options_SetBool(key, isOn); 
	return isOn;
}

static void MenuOptionsScreen_GetFPS(String* raw) {
	String_AppendConst(raw, FpsLimit_Names[Game_FpsLimit]);
}
static void MenuOptionsScreen_SetFPS(const String* v) {
	int method = Utils_ParseEnum(v, FPS_LIMIT_VSYNC, FpsLimit_Names, Array_Elems(FpsLimit_Names));
	Options_Set(OPT_FPS_LIMIT, v);
	Game_SetFpsLimit(method);
}

static void MenuOptionsScreen_Set(struct MenuOptionsScreen* s, int i, const String* text) {
	char titleBuffer[STRING_SIZE];
	String title = String_FromArray(titleBuffer);
	s->Buttons[i].SetValue(text);

	/* need to get btn again here (e.g. changing FPS invalidates all widgets) */
	String_AppendConst(&title, s->Buttons[i].OptName);
	String_AppendConst(&title, ": ");
	s->Buttons[i].GetValue(&title);
	ButtonWidget_Set(&s->Buttons[i], &title, &s->TitleFont);
}

static void MenuOptionsScreen_FreeExtHelp(struct MenuOptionsScreen* s) {
	if (!s->ExtHelp.LinesCount) return;
	Elem_TryFree(&s->ExtHelp);
	s->ExtHelp.LinesCount = 0;
}

static void MenuOptionsScreen_RepositionExtHelp(struct MenuOptionsScreen* s) {
	s->ExtHelp.XOffset = Game_Width  / 2 - s->ExtHelp.Width / 2;
	s->ExtHelp.YOffset = Game_Height / 2 + 100;
	Widget_Reposition(&s->ExtHelp);
}

static void MenuOptionsScreen_SelectExtHelp(struct MenuOptionsScreen* s, int idx) {
	MenuOptionsScreen_FreeExtHelp(s);
	if (!s->Descriptions || s->ActiveI >= 0) return;
	const char* desc = s->Descriptions[idx];
	if (!desc) return;

	String descRaw = String_FromReadonly(desc);
	String descLines[5];
	int count = String_UNSAFE_Split(&descRaw, '|', descLines, Array_Elems(descLines));

	TextGroupWidget_Create(&s->ExtHelp, count, &s->TextFont, &s->TextFont, s->ExtHelp_Textures, s->ExtHelp_Buffer);
	Widget_SetLocation((struct Widget*)(&s->ExtHelp), ANCHOR_MIN, ANCHOR_MIN, 0, 0);
	Elem_Init(&s->ExtHelp);

	int i;
	for (i = 0; i < count; i++) {
		TextGroupWidget_SetText(&s->ExtHelp, i, &descLines[i]);
	}
	MenuOptionsScreen_RepositionExtHelp(s);
}

static void MenuOptionsScreen_FreeInput(struct MenuOptionsScreen* s) {
	int i;
	if (s->ActiveI == -1) return;
	
	for (i = s->WidgetsCount - 3; i < s->WidgetsCount; i++) {
		if (!s->Widgets[i]) continue;
		Elem_TryFree(s->Widgets[i]);
		s->Widgets[i] = NULL;
	}
}

static void MenuOptionsScreen_EnterInput(struct MenuOptionsScreen* s) {
	struct MenuInputValidator* v = &s->Input.Validator;
	String text = s->Input.Base.Text;

	if (v->VTABLE->IsValidValue(v, &text)) {
		MenuOptionsScreen_Set(s, s->ActiveI, &text);
	}

	MenuOptionsScreen_SelectExtHelp(s, s->ActiveI);
	MenuOptionsScreen_FreeInput(s);
	s->ActiveI = -1;
}

static void MenuOptionsScreen_Init(void* screen) {
	struct MenuOptionsScreen* s = screen;
	Key_KeyRepeat = true;
	MenuScreen_Init(s);
	s->SelectedI = -1;
}
	
static void MenuOptionsScreen_Render(void* screen, double delta) {
	struct MenuOptionsScreen* s = screen;
	MenuScreen_Render(s, delta);
	if (!s->ExtHelp.LinesCount) return;

	struct TextGroupWidget* extHelp = &s->ExtHelp;
	int x = extHelp->X - 5, y = extHelp->Y - 5;
	int width = extHelp->Width, height = extHelp->Height;
	PackedCol tableCol = PACKEDCOL_CONST(20, 20, 20, 200);
	GfxCommon_Draw2DFlat(x, y, width + 10, height + 10, tableCol);

	Gfx_SetTexturing(true);
	Elem_Render(&s->ExtHelp, delta);
	Gfx_SetTexturing(false);
}

static void MenuOptionsScreen_Free(void* screen) {
	struct MenuOptionsScreen* s = screen;
	Key_KeyRepeat = false;
	MenuScreen_Free(s);
}

static void MenuOptionsScreen_OnResize(void* screen) {
	struct MenuOptionsScreen* s = screen;
	Menu_OnResize(s);
	if (!s->ExtHelp.LinesCount) return;
	MenuOptionsScreen_RepositionExtHelp(s);
}

static void MenuOptionsScreen_ContextLost(void* screen) {
	struct MenuOptionsScreen* s = screen;
	Menu_ContextLost(s);
	s->ActiveI = -1;
	MenuOptionsScreen_FreeExtHelp(s);
}

static bool MenuOptionsScreen_KeyPress(void* screen, char keyChar) {
	struct MenuOptionsScreen* s = screen;
	if (s->ActiveI == -1) return true;
	return Elem_HandlesKeyPress(&s->Input.Base, keyChar);
}

static bool MenuOptionsScreen_KeyDown(void* screen, Key key) {
	struct MenuOptionsScreen* s = screen;
	if (s->ActiveI >= 0) {
		if (Elem_HandlesKeyDown(&s->Input.Base, key)) return true;

		if (key == Key_Enter || key == Key_KeypadEnter) {
			MenuOptionsScreen_EnterInput(s); return true;
		}
	}
	return MenuScreen_KeyDown(s, key);
}

static bool MenuOptionsScreen_KeyUp(void* screen, Key key) {
	struct MenuOptionsScreen* s = screen;
	if (s->ActiveI == -1) return true;
	return Elem_HandlesKeyUp(&s->Input.Base, key);
}

static bool MenuOptionsScreen_MouseMove(void* screen, int x, int y) {
	struct MenuOptionsScreen* s = screen;
	int i = Menu_DoMouseMove(s, x, y);
	if (i == -1 || i == s->SelectedI) return true;
	if (!s->Descriptions || i >= s->DescriptionsCount) return true;

	s->SelectedI = i;
	if (s->ActiveI == -1) MenuOptionsScreen_SelectExtHelp(s, i);
	return true;
}

static void MenuOptionsScreen_Make(struct MenuOptionsScreen* s, int i, int dir, int y, const char* optName, Widget_LeftClick onClick, Button_Get getter, Button_Set setter) {
	char titleBuffer[STRING_SIZE];
	String title = String_FromArray(titleBuffer);
	String_AppendConst(&title, optName);
	String_AppendConst(&title, ": ");
	getter(&title);

	struct ButtonWidget* btn = &s->Buttons[i];
	Menu_Button(s, i, btn, 300, &title, &s->TitleFont, onClick,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 160 * dir, y);

	btn->OptName  = optName;
	btn->GetValue = getter;
	btn->SetValue = setter;
}

static void MenuOptionsScreen_MakeSimple(struct MenuOptionsScreen* s, int i, int dir, int y, const char* title, Widget_LeftClick onClick) {
	String text = String_FromReadonly(title);
	Menu_Button(s, i, &s->Buttons[i], 300, &text, &s->TitleFont, onClick,
		ANCHOR_CENTRE, ANCHOR_CENTRE, dir * 160, y);
}

static void MenuOptionsScreen_OK(void* screen, void* widget) {
	struct MenuOptionsScreen* s = screen;
	MenuOptionsScreen_EnterInput(s);
}

static void MenuOptionsScreen_Default(void* screen, void* widget) {
	struct MenuOptionsScreen* s = screen;
	String text = String_FromReadonly(s->DefaultValues[s->ActiveI]);
	InputWidget_Clear(&s->Input.Base);
	InputWidget_AppendString(&s->Input.Base, &text);
}

static void MenuOptionsScreen_Bool(void* screen, void* widget) {
	struct MenuOptionsScreen* s = screen;
	struct ButtonWidget* btn    = widget;
	int index = Menu_Index(s, btn);
	MenuOptionsScreen_SelectExtHelp(s, index);

	char valueBuffer[STRING_SIZE];
	String value = String_FromArray(valueBuffer);
	btn->GetValue(&value);

	bool isOn = String_CaselessEqualsConst(&value, "ON");
	String newValue = String_FromReadonly(isOn ? "OFF" : "ON");
	MenuOptionsScreen_Set(s, index, &newValue);
}

static void MenuOptionsScreen_Enum(void* screen, void* widget) {
	struct MenuOptionsScreen* s = screen;
	struct ButtonWidget* btn    = widget;
	int index = Menu_Index(s, btn);
	MenuOptionsScreen_SelectExtHelp(s, index);

	struct MenuInputValidator* v = &s->Validators[index];
	const char** names = v->Meta._Enum.Names;
	int count = v->Meta._Enum.Count;

	char valueBuffer[STRING_SIZE];
	String value = String_FromArray(valueBuffer);
	btn->GetValue(&value);

	int raw = (Utils_ParseEnum(&value, 0, names, count) + 1) % count;
	String newValue = String_FromReadonly(names[raw]);
	MenuOptionsScreen_Set(s, index, &newValue);
}

static void MenuOptionsScreen_Input(void* screen, void* widget) {
	static String okay = String_FromConst("OK");
	static String def  = String_FromConst("Default value");

	struct MenuOptionsScreen* s = screen;
	struct ButtonWidget* btn = widget;
	int i;
	char valueBuffer[STRING_SIZE];
	String value = String_FromArray(valueBuffer);

	s->ActiveI = Menu_Index(s, btn);
	MenuOptionsScreen_FreeExtHelp(s);
	MenuOptionsScreen_FreeInput(s);
	btn->GetValue(&value);

	i = s->WidgetsCount;
	Menu_Input(s,  i - 1, &s->Input,   400, &value, &s->TextFont,  &s->Validators[s->ActiveI],
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 110);
	Menu_Button(s, i - 2, &s->OK,       40, &okay,  &s->TitleFont, MenuOptionsScreen_OK,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 240, 110);
	Menu_Button(s, i - 3, &s->Default, 200, &def,   &s->TitleFont, MenuOptionsScreen_Default,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 150);
}

struct ScreenVTABLE MenuOptionsScreen_VTABLE = {
	MenuOptionsScreen_Init,     MenuOptionsScreen_Render, MenuOptionsScreen_Free,      Gui_DefaultRecreate,
	MenuOptionsScreen_KeyDown,  MenuOptionsScreen_KeyUp,  MenuOptionsScreen_KeyPress,
	Menu_MouseDown,             Menu_MouseUp,             MenuOptionsScreen_MouseMove, MenuScreen_MouseScroll,
	MenuOptionsScreen_OnResize, MenuOptionsScreen_ContextLost, NULL,
};
struct Screen* MenuOptionsScreen_MakeInstance(struct Widget** widgets, int count, struct ButtonWidget* buttons, Event_Void_Callback contextRecreated,
	struct MenuInputValidator* validators, const char** defaultValues, const char** descriptions, int descsCount) {
	struct MenuOptionsScreen* s = &MenuOptionsScreen_Instance;
	s->HandlesAllInput = true;
	s->Widgets         = widgets;
	s->WidgetsCount    = count;

	s->ExtHelp.LinesCount = 0;
	s->VTABLE = &MenuOptionsScreen_VTABLE;
	s->VTABLE->ContextLost      = MenuOptionsScreen_ContextLost;
	s->VTABLE->ContextRecreated = contextRecreated;

	s->Buttons           = buttons;
	s->Validators        = validators;
	s->DefaultValues     = defaultValues;
	s->Descriptions      = descriptions;
	s->DescriptionsCount = descsCount;

	s->ActiveI   = -1;
	s->SelectedI = -1;
	return (struct Screen*)s;
}


/*########################################################################################################################*
*---------------------------------------------------ClassicOptionsScreen--------------------------------------------------*
*#########################################################################################################################*/
enum ViewDist { VIEW_TINY, VIEW_SHORT, VIEW_NORMAL, VIEW_FAR, VIEW_COUNT };
const char* ViewDist_Names[VIEW_COUNT] = { "TINY", "SHORT", "NORMAL", "FAR" };

static void ClassicOptionsScreen_GetMusic(String* v) { Menu_GetBool(v, Game_MusicVolume > 0); }
static void ClassicOptionsScreen_SetMusic(const String* v) {
	Game_MusicVolume = String_CaselessEqualsConst(v, "ON") ? 100 : 0;
	Audio_SetMusic(Game_MusicVolume);
	Options_SetInt(OPT_MUSIC_VOLUME, Game_MusicVolume);
}

static void ClassicOptionsScreen_GetInvert(String* v) { Menu_GetBool(v, Game_InvertMouse); }
static void ClassicOptionsScreen_SetInvert(const String* v) { Game_InvertMouse = Menu_SetBool(v, OPT_INVERT_MOUSE); }

static void ClassicOptionsScreen_GetViewDist(String* v) {
	if (Game_ViewDistance >= 512) {
		String_AppendConst(v, ViewDist_Names[VIEW_FAR]);
	} else if (Game_ViewDistance >= 128) {
		String_AppendConst(v, ViewDist_Names[VIEW_NORMAL]);
	} else if (Game_ViewDistance >= 32) {
		String_AppendConst(v, ViewDist_Names[VIEW_SHORT]);
	} else {
		String_AppendConst(v, ViewDist_Names[VIEW_TINY]);
	}
}
static void ClassicOptionsScreen_SetViewDist(const String* v) {
	int raw  = Utils_ParseEnum(v, 0, ViewDist_Names, VIEW_COUNT);
	int dist = raw == VIEW_FAR ? 512 : (raw == VIEW_NORMAL ? 128 : (raw == VIEW_SHORT ? 32 : 8));
	Game_UserSetViewDistance(dist);
}

static void ClassicOptionsScreen_GetPhysics(String* v) { Menu_GetBool(v, Physics_Enabled); }
static void ClassicOptionsScreen_SetPhysics(const String* v) {
	Physics_SetEnabled(Menu_SetBool(v, OPT_BLOCK_PHYSICS));
}

static void ClassicOptionsScreen_GetSounds(String* v) { Menu_GetBool(v, Game_SoundsVolume > 0); }
static void ClassicOptionsScreen_SetSounds(const String* v) {
	Game_SoundsVolume = String_CaselessEqualsConst(v, "ON") ? 100 : 0;
	Audio_SetSounds(Game_SoundsVolume);
	Options_SetInt(OPT_SOUND_VOLUME, Game_SoundsVolume);
}

static void ClassicOptionsScreen_GetShowFPS(String* v) { Menu_GetBool(v, Game_ShowFPS); }
static void ClassicOptionsScreen_SetShowFPS(const String* v) { Game_ShowFPS = Menu_SetBool(v, OPT_SHOW_FPS); }

static void ClassicOptionsScreen_GetViewBob(String* v) { Menu_GetBool(v, Game_ViewBobbing); }
static void ClassicOptionsScreen_SetViewBob(const String* v) { Game_ViewBobbing = Menu_SetBool(v, OPT_VIEW_BOBBING); }

static void ClassicOptionsScreen_GetHacks(String* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.Enabled); }
static void ClassicOptionsScreen_SetHacks(const String* v) {
	LocalPlayer_Instance.Hacks.Enabled = Menu_SetBool(v, OPT_HACKS_ENABLED);
	LocalPlayer_CheckHacksConsistency();
}

static void ClassicOptionsScreen_ContextRecreated(void* screen) {
	static String title = String_FromConst("Controls...");
	struct MenuOptionsScreen* s = screen;

	MenuOptionsScreen_Make(s, 0, -1, -150, "Music",           MenuOptionsScreen_Bool, 
		ClassicOptionsScreen_GetMusic,    ClassicOptionsScreen_SetMusic);
	MenuOptionsScreen_Make(s, 1, -1, -100, "Invert mouse",    MenuOptionsScreen_Bool, 
		ClassicOptionsScreen_GetInvert,   ClassicOptionsScreen_SetInvert);
	MenuOptionsScreen_Make(s, 2, -1,  -50, "Render distance", MenuOptionsScreen_Enum, 
		ClassicOptionsScreen_GetViewDist, ClassicOptionsScreen_SetViewDist);
	MenuOptionsScreen_Make(s, 3, -1,    0, "Block physics",   MenuOptionsScreen_Bool, 
		ClassicOptionsScreen_GetPhysics,  ClassicOptionsScreen_SetPhysics);

	MenuOptionsScreen_Make(s, 4, 1, -150, "Sound",         MenuOptionsScreen_Bool, 
		ClassicOptionsScreen_GetSounds,  ClassicOptionsScreen_SetSounds);
	MenuOptionsScreen_Make(s, 5, 1, -100, "Show FPS",      MenuOptionsScreen_Bool, 
		ClassicOptionsScreen_GetShowFPS, ClassicOptionsScreen_SetShowFPS);
	MenuOptionsScreen_Make(s, 6, 1,  -50, "View bobbing",  MenuOptionsScreen_Bool, 
		ClassicOptionsScreen_GetViewBob, ClassicOptionsScreen_SetViewBob);
	MenuOptionsScreen_Make(s, 7, 1,    0, "FPS mode",      MenuOptionsScreen_Enum, 
		MenuOptionsScreen_GetFPS,        MenuOptionsScreen_SetFPS);
	MenuOptionsScreen_Make(s, 8, 0,   60, "Hacks enabled", MenuOptionsScreen_Bool, 
		ClassicOptionsScreen_GetHacks,   ClassicOptionsScreen_SetHacks);

	
	Menu_Button(s, 9, &s->Buttons[9], 400, &title, &s->TitleFont, Menu_SwitchKeysClassic,
		ANCHOR_CENTRE, ANCHOR_MAX, 0, 95);
	Menu_Back(s, 10, &s->Buttons[10], "Done", &s->TitleFont, Menu_SwitchPause);

	/* Disable certain options */
	if (!ServerConnection_IsSinglePlayer) Menu_Remove(s, 3);
	if (!Game_ClassicHacks)               Menu_Remove(s, 8);
}

struct Screen* ClassicOptionsScreen_MakeInstance(void) {
	static struct ButtonWidget buttons[11];
	static struct MenuInputValidator validators[Array_Elems(buttons)];
	static struct Widget* widgets[Array_Elems(buttons)];	

	validators[2] = MenuInputValidator_Enum(ViewDist_Names, VIEW_COUNT);
	validators[7] = MenuInputValidator_Enum(FpsLimit_Names, FPS_LIMIT_COUNT);

	return MenuOptionsScreen_MakeInstance(widgets, Array_Elems(widgets), buttons,
		ClassicOptionsScreen_ContextRecreated, validators, NULL, NULL, 0);
}


/*########################################################################################################################*
*----------------------------------------------------EnvSettingsScreen----------------------------------------------------*
*#########################################################################################################################*/
static void EnvSettingsScreen_GetCloudsCol(String* v) { PackedCol_ToHex(v, Env_CloudsCol); }
static void EnvSettingsScreen_SetCloudsCol(const String* v) { Env_SetCloudsCol(Menu_HexCol(v)); }

static void EnvSettingsScreen_GetSkyCol(String* v) { PackedCol_ToHex(v, Env_SkyCol); }
static void EnvSettingsScreen_SetSkyCol(const String* v) { Env_SetSkyCol(Menu_HexCol(v)); }

static void EnvSettingsScreen_GetFogCol(String* v) { PackedCol_ToHex(v, Env_FogCol); }
static void EnvSettingsScreen_SetFogCol(const String* v) { Env_SetFogCol(Menu_HexCol(v)); }

static void EnvSettingsScreen_GetCloudsSpeed(String* v) { String_AppendFloat(v, Env_CloudsSpeed, 2); }
static void EnvSettingsScreen_SetCloudsSpeed(const String* v) { Env_SetCloudsSpeed(Menu_Float(v)); }

static void EnvSettingsScreen_GetCloudsHeight(String* v) { String_AppendInt(v, Env_CloudsHeight); }
static void EnvSettingsScreen_SetCloudsHeight(const String* v) { Env_SetCloudsHeight(Menu_Int(v)); }

static void EnvSettingsScreen_GetSunCol(String* v) { PackedCol_ToHex(v, Env_SunCol); }
static void EnvSettingsScreen_SetSunCol(const String* v) { Env_SetSunCol(Menu_HexCol(v)); }

static void EnvSettingsScreen_GetShadowCol(String* v) { PackedCol_ToHex(v, Env_ShadowCol); }
static void EnvSettingsScreen_SetShadowCol(const String* v) { Env_SetShadowCol(Menu_HexCol(v)); }

static void EnvSettingsScreen_GetWeather(String* v) { String_AppendConst(v, Weather_Names[Env_Weather]); }
static void EnvSettingsScreen_SetWeather(const String* v) {
	int raw = Utils_ParseEnum(v, 0, Weather_Names, Array_Elems(Weather_Names));
	Env_SetWeather(raw); 
}

static void EnvSettingsScreen_GetWeatherSpeed(String* v) { String_AppendFloat(v, Env_WeatherSpeed, 2); }
static void EnvSettingsScreen_SetWeatherSpeed(const String* v) { Env_SetWeatherSpeed(Menu_Float(v)); }

static void EnvSettingsScreen_GetEdgeHeight(String* v) { String_AppendInt(v, Env_EdgeHeight); }
static void EnvSettingsScreen_SetEdgeHeight(const String* v) { Env_SetEdgeHeight(Menu_Int(v)); }

static void EnvSettingsScreen_ContextRecreated(void* screen) {
	struct MenuOptionsScreen* s = screen;
	struct Widget** widgets = s->Widgets;

	MenuOptionsScreen_Make(s, 0, -1, -150, "Clouds col",    MenuOptionsScreen_Input, 
		EnvSettingsScreen_GetCloudsCol,    EnvSettingsScreen_SetCloudsCol);
	MenuOptionsScreen_Make(s, 1, -1, -100, "Sky col",       MenuOptionsScreen_Input,
		EnvSettingsScreen_GetSkyCol,       EnvSettingsScreen_SetSkyCol);
	MenuOptionsScreen_Make(s, 2, -1,  -50, "Fog col",       MenuOptionsScreen_Input,
		EnvSettingsScreen_GetFogCol,       EnvSettingsScreen_SetFogCol);
	MenuOptionsScreen_Make(s, 3, -1,    0, "Clouds speed",  MenuOptionsScreen_Input,
		EnvSettingsScreen_GetCloudsSpeed,  EnvSettingsScreen_SetCloudsSpeed);
	MenuOptionsScreen_Make(s, 4, -1,   50, "Clouds height", MenuOptionsScreen_Input,
		EnvSettingsScreen_GetCloudsHeight, EnvSettingsScreen_SetCloudsHeight);

	MenuOptionsScreen_Make(s, 5, 1, -150, "Sunlight col",    MenuOptionsScreen_Input,
		EnvSettingsScreen_GetSunCol,       EnvSettingsScreen_SetSunCol);
	MenuOptionsScreen_Make(s, 6, 1, -100, "Shadow col",      MenuOptionsScreen_Input,
		EnvSettingsScreen_GetShadowCol,    EnvSettingsScreen_SetShadowCol);
	MenuOptionsScreen_Make(s, 7, 1,  -50, "Weather",         MenuOptionsScreen_Enum,
		EnvSettingsScreen_GetWeather,      EnvSettingsScreen_SetWeather);
	MenuOptionsScreen_Make(s, 8, 1,    0, "Rain/Snow speed", MenuOptionsScreen_Input,
		EnvSettingsScreen_GetWeatherSpeed, EnvSettingsScreen_SetWeatherSpeed);
	MenuOptionsScreen_Make(s, 9, 1, 50, "Water level",       MenuOptionsScreen_Input,
		EnvSettingsScreen_GetEdgeHeight,   EnvSettingsScreen_SetEdgeHeight);

	Menu_Back(s, 10, &s->Buttons[10], "Done", &s->TitleFont, Menu_SwitchOptions);
	widgets[11] = NULL; widgets[12] = NULL; widgets[13] = NULL;
}

struct Screen* EnvSettingsScreen_MakeInstance(void) {
	static struct ButtonWidget buttons[11];
	static struct MenuInputValidator validators[Array_Elems(buttons)];
	static const char* defaultValues[Array_Elems(buttons)];
	static struct Widget* widgets[Array_Elems(buttons) + 3];

	static char cloudHeightBuffer[STRING_INT_CHARS];
	String cloudHeight = String_FromArray(cloudHeightBuffer);
	String_AppendInt(&cloudHeight, World_Height + 2);

	static char edgeHeightBuffer[STRING_INT_CHARS];
	String edgeHeight = String_FromArray(edgeHeightBuffer);
	String_AppendInt(&edgeHeight, World_Height / 2);

	validators[0]    = MenuInputValidator_Hex();
	defaultValues[0] = ENV_DEFAULT_CLOUDSCOL_HEX;
	validators[1]    = MenuInputValidator_Hex();
	defaultValues[1] = ENV_DEFAULT_SKYCOL_HEX;
	validators[2]    = MenuInputValidator_Hex();
	defaultValues[2] = ENV_DEFAULT_FOGCOL_HEX;
	validators[3]    = MenuInputValidator_Float(0.00f, 1000.00f);
	defaultValues[3] = "1";
	validators[4]    = MenuInputValidator_Int(-10000, 10000);
	defaultValues[4] = cloudHeightBuffer;

	validators[5]    = MenuInputValidator_Hex();
	defaultValues[5] = ENV_DEFAULT_SUNCOL_HEX;
	validators[6]    = MenuInputValidator_Hex();
	defaultValues[6] = ENV_DEFAULT_SHADOWCOL_HEX;
	validators[7]    = MenuInputValidator_Enum(Weather_Names, Array_Elems(Weather_Names));
	validators[8]    = MenuInputValidator_Float(-100.00f, 100.00f);
	defaultValues[8] = "1";
	validators[9]    = MenuInputValidator_Int(-2048, 2048);
	defaultValues[9] = edgeHeightBuffer;

	return MenuOptionsScreen_MakeInstance(widgets, Array_Elems(widgets), buttons,
		EnvSettingsScreen_ContextRecreated, validators, defaultValues, NULL, 0);
}


/*########################################################################################################################*
*--------------------------------------------------GraphicsOptionsScreen--------------------------------------------------*
*#########################################################################################################################*/
static void GraphicsOptionsScreen_GetViewDist(String* v) { String_AppendInt(v, Game_ViewDistance); }
static void GraphicsOptionsScreen_SetViewDist(const String* v) { Game_UserSetViewDistance(Menu_Int(v)); }

static void GraphicsOptionsScreen_GetSmooth(String* v) { Menu_GetBool(v, Game_SmoothLighting); }
static void GraphicsOptionsScreen_SetSmooth(const String* v) {
	Game_SmoothLighting = Menu_SetBool(v, OPT_SMOOTH_LIGHTING);
	ChunkUpdater_ApplyMeshBuilder();
	ChunkUpdater_Refresh();
}

static void GraphicsOptionsScreen_GetNames(String* v) { String_AppendConst(v, NameMode_Names[Entities_NameMode]); }
static void GraphicsOptionsScreen_SetNames(const String* v) {
	Entities_NameMode = Utils_ParseEnum(v, 0, NameMode_Names, NAME_MODE_COUNT);
	Options_Set(OPT_NAMES_MODE, v);
}

static void GraphicsOptionsScreen_GetShadows(String* v) { String_AppendConst(v, ShadowMode_Names[Entities_ShadowMode]); }
static void GraphicsOptionsScreen_SetShadows(const String* v) {
	Entities_ShadowMode = Utils_ParseEnum(v, 0, ShadowMode_Names, SHADOW_MODE_COUNT);
	Options_Set(OPT_ENTITY_SHADOW, v);
}

static void GraphicsOptionsScreen_GetMipmaps(String* v) { Menu_GetBool(v, Gfx_Mipmaps); }
static void GraphicsOptionsScreen_SetMipmaps(const String* v) {
	Gfx_Mipmaps = Menu_SetBool(v, OPT_MIPMAPS);
	char urlBuffer[STRING_SIZE];
	String url = String_FromArray(urlBuffer);
	String_Copy(&url, &World_TextureUrl);

	/* always force a reload from cache */
	World_TextureUrl.length = 0;
	String_AppendConst(&World_TextureUrl, "~`#$_^*()@");
	TexturePack_ExtractCurrent(&url);

	String_Copy(&World_TextureUrl, &url);
}

static void GraphicsOptionsScreen_ContextRecreated(void* screen) {
	struct MenuOptionsScreen* s = screen;
	struct Widget** widgets = s->Widgets;

	MenuOptionsScreen_Make(s, 0, -1, -50, "FPS mode",          MenuOptionsScreen_Enum, 
		MenuOptionsScreen_GetFPS,          MenuOptionsScreen_SetFPS);
	MenuOptionsScreen_Make(s, 1, -1,   0, "View distance",     MenuOptionsScreen_Input, 
		GraphicsOptionsScreen_GetViewDist, GraphicsOptionsScreen_SetViewDist);
	MenuOptionsScreen_Make(s, 2, -1,  50, "Advanced lighting", MenuOptionsScreen_Bool,
		GraphicsOptionsScreen_GetSmooth,   GraphicsOptionsScreen_SetSmooth);

	MenuOptionsScreen_Make(s, 3, 1, -50, "Names",   MenuOptionsScreen_Enum, 
		GraphicsOptionsScreen_GetNames,    GraphicsOptionsScreen_SetNames);
	MenuOptionsScreen_Make(s, 4, 1,   0, "Shadows", MenuOptionsScreen_Enum, 
		GraphicsOptionsScreen_GetShadows, GraphicsOptionsScreen_SetShadows);
	MenuOptionsScreen_Make(s, 5, 1,  50, "Mipmaps", MenuOptionsScreen_Bool,
		GraphicsOptionsScreen_GetMipmaps, GraphicsOptionsScreen_SetMipmaps);

	Menu_Back(s, 6, &s->Buttons[6], "Done", &s->TitleFont, Menu_SwitchOptions);
	widgets[7] = NULL; widgets[8] = NULL; widgets[9] = NULL;
}

struct Screen* GraphicsOptionsScreen_MakeInstance(void) {
	static struct ButtonWidget buttons[7];
	static struct MenuInputValidator validators[Array_Elems(buttons)];
	static const char* defaultValues[Array_Elems(buttons)];
	static struct Widget* widgets[Array_Elems(buttons) + 3];
	
	static const char* descs[Array_Elems(buttons)];
	descs[0] = \
		"&eVSync: &fNumber of frames rendered is at most the monitor's refresh rate.|" \
		"&e30/60/120 FPS: &f30/60/120 frames rendered at most each second.|" \
		"&eNoLimit: &fRenders as many frames as possible each second.|" \
		"&cUsing NoLimit mode is discouraged.";
	descs[2] = "&cNote: &eSmooth lighting is still experimental and can heavily reduce performance.";
	descs[3] = \
		"&eNone: &fNo names of players are drawn.|" \
		"&eHovered: &fName of the targeted player is drawn see-through.|" \
		"&eAll: &fNames of all other players are drawn normally.|" \
		"&eAllHovered: &fAll names of players are drawn see-through.|" \
		"&eAllUnscaled: &fAll names of players are drawn see-through without scaling.";
	descs[4] = \
		"&eNone: &fNo entity shadows are drawn.|" \
		"&eSnapToBlock: &fA square shadow is shown on block you are directly above.|" \
		"&eCircle: &fA circular shadow is shown across the blocks you are above.|" \
		"&eCircleAll: &fA circular shadow is shown underneath all entities.";
	
	validators[0]    = MenuInputValidator_Enum(FpsLimit_Names, FPS_LIMIT_COUNT);
	validators[1]    = MenuInputValidator_Int(8, 4096);
	defaultValues[1] = "512";
	validators[3]    = MenuInputValidator_Enum(NameMode_Names,   NAME_MODE_COUNT);
	validators[4]    = MenuInputValidator_Enum(ShadowMode_Names, SHADOW_MODE_COUNT);

	return MenuOptionsScreen_MakeInstance(widgets, Array_Elems(widgets), buttons,
		GraphicsOptionsScreen_ContextRecreated, validators, defaultValues, descs, Array_Elems(descs));
}


/*########################################################################################################################*
*----------------------------------------------------GuiOptionsScreen-----------------------------------------------------*
*#########################################################################################################################*/
static void GuiOptionsScreen_GetShadows(String* v) { Menu_GetBool(v, Drawer2D_BlackTextShadows); }
static void GuiOptionsScreen_SetShadows(const String* v) {
	Drawer2D_BlackTextShadows = Menu_SetBool(v, OPT_BLACK_TEXT);
	Menu_HandleFontChange((struct Screen*)&MenuOptionsScreen_Instance);
}

static void GuiOptionsScreen_GetShowFPS(String* v) { Menu_GetBool(v, Game_ShowFPS); }
static void GuiOptionsScreen_SetShowFPS(const String* v) { Game_ShowFPS = Menu_SetBool(v, OPT_SHOW_FPS); }

static void GuiOptionsScreen_SetScale(const String* v, float* target, const char* optKey) {
	*target = Menu_Float(v);
	Options_Set(optKey, v);
	Gui_RefreshHud();
}

static void GuiOptionsScreen_GetHotbar(String* v) { String_AppendFloat(v, Game_RawHotbarScale, 1); }
static void GuiOptionsScreen_SetHotbar(const String* v) { GuiOptionsScreen_SetScale(v, &Game_RawHotbarScale, OPT_HOTBAR_SCALE); }

static void GuiOptionsScreen_GetInventory(String* v) { String_AppendFloat(v, Game_RawInventoryScale, 1); }
static void GuiOptionsScreen_SetInventory(const String* v) { GuiOptionsScreen_SetScale(v, &Game_RawInventoryScale, OPT_INVENTORY_SCALE); }

static void GuiOptionsScreen_GetTabAuto(String* v) { Menu_GetBool(v, Game_TabAutocomplete); }
static void GuiOptionsScreen_SetTabAuto(const String* v) { Game_TabAutocomplete = Menu_SetBool(v, OPT_TAB_AUTOCOMPLETE); }

static void GuiOptionsScreen_GetClickable(String* v) { Menu_GetBool(v, Game_ClickableChat); }
static void GuiOptionsScreen_SetClickable(const String* v) { Game_ClickableChat = Menu_SetBool(v, OPT_CLICKABLE_CHAT); }

static void GuiOptionsScreen_GetChatScale(String* v) { String_AppendFloat(v, Game_RawChatScale, 1); }
static void GuiOptionsScreen_SetChatScale(const String* v) { GuiOptionsScreen_SetScale(v, &Game_RawChatScale, OPT_CHAT_SCALE); }

static void GuiOptionsScreen_GetChatlines(String* v) { String_AppendInt(v, Game_ChatLines); }
static void GuiOptionsScreen_SetChatlines(const String* v) {
	Game_ChatLines = Menu_Int(v);
	Options_Set(OPT_CHATLINES, v);
	Gui_RefreshHud();
}

static void GuiOptionsScreen_GetUseFont(String* v) { Menu_GetBool(v, !Drawer2D_BitmappedText); }
static void GuiOptionsScreen_SetUseFont(const String* v) {
	Drawer2D_BitmappedText = !Menu_SetBool(v, OPT_USE_CHAT_FONT);
	Menu_HandleFontChange((struct Screen*)&MenuOptionsScreen_Instance);
}

static void GuiOptionsScreen_ContextRecreated(void* screen) {
	struct MenuOptionsScreen* s = screen;
	struct Widget** widgets = s->Widgets;

	MenuOptionsScreen_Make(s, 0, -1, -150, "Black text shadows", MenuOptionsScreen_Bool,
		GuiOptionsScreen_GetShadows,   GuiOptionsScreen_SetShadows);
	MenuOptionsScreen_Make(s, 1, -1, -100, "Show FPS",           MenuOptionsScreen_Bool,
		GuiOptionsScreen_GetShowFPS,   GuiOptionsScreen_SetShowFPS);
	MenuOptionsScreen_Make(s, 2, -1,  -50, "Hotbar scale",       MenuOptionsScreen_Input,
		GuiOptionsScreen_GetHotbar,    GuiOptionsScreen_SetHotbar);
	MenuOptionsScreen_Make(s, 3, -1,    0, "Inventory scale",    MenuOptionsScreen_Input,
		GuiOptionsScreen_GetInventory, GuiOptionsScreen_SetInventory);
	MenuOptionsScreen_Make(s, 4, -1,   50, "Tab auto-complete",  MenuOptionsScreen_Bool,
		GuiOptionsScreen_GetTabAuto,   GuiOptionsScreen_SetTabAuto);

	MenuOptionsScreen_Make(s, 5, 1, -150, "Clickable chat",     MenuOptionsScreen_Bool,
		GuiOptionsScreen_GetClickable, GuiOptionsScreen_SetClickable);
	MenuOptionsScreen_Make(s, 6, 1, -100, "Chat scale",         MenuOptionsScreen_Input,
		GuiOptionsScreen_GetChatScale, GuiOptionsScreen_SetChatScale);
	MenuOptionsScreen_Make(s, 7, 1,  -50, "Chat lines",         MenuOptionsScreen_Input,
		GuiOptionsScreen_GetChatlines, GuiOptionsScreen_SetChatlines);
	MenuOptionsScreen_Make(s, 8, 1,    0, "Use system font",    MenuOptionsScreen_Bool,
		GuiOptionsScreen_GetUseFont,   GuiOptionsScreen_SetUseFont);
	MenuOptionsScreen_MakeSimple(s, 9, 1,  50, "Select system font", Menu_SwitchFont);

	Menu_Back(s, 10, &s->Buttons[10], "Done", &s->TitleFont, Menu_SwitchOptions);
	widgets[11] = NULL; widgets[12] = NULL; widgets[13] = NULL;
}

struct Screen* GuiOptionsScreen_MakeInstance(void) {
	static struct ButtonWidget buttons[11];
	static struct MenuInputValidator validators[Array_Elems(buttons)];
	static const char* defaultValues[Array_Elems(buttons)];
	static struct Widget* widgets[Array_Elems(buttons) + 3];

	validators[2]    = MenuInputValidator_Float(0.25f, 4.00f);
	defaultValues[2] = "1";
	validators[3]    = MenuInputValidator_Float(0.25f, 4.00f);
	defaultValues[3] = "1";
	validators[6]    = MenuInputValidator_Float(0.25f, 4.00f);
	defaultValues[6] = "1";
	validators[7]    = MenuInputValidator_Int(0, 30);
	defaultValues[7] = "10";
	validators[9]    = MenuInputValidator_String();
	defaultValues[9] = "Arial";

	return MenuOptionsScreen_MakeInstance(widgets, Array_Elems(widgets), buttons,
		GuiOptionsScreen_ContextRecreated, validators, defaultValues, NULL, 0);
}


/*########################################################################################################################*
*---------------------------------------------------HacksSettingsScreen---------------------------------------------------*
*#########################################################################################################################*/
static void HacksSettingsScreen_GetHacks(String* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.Enabled); }
static void HacksSettingsScreen_SetHacks(const String* v) {
	LocalPlayer_Instance.Hacks.Enabled = Menu_SetBool(v,OPT_HACKS_ENABLED);
	LocalPlayer_CheckHacksConsistency();
}

static void HacksSettingsScreen_GetSpeed(String* v) { String_AppendFloat(v, LocalPlayer_Instance.Hacks.SpeedMultiplier, 2); }
static void HacksSettingsScreen_SetSpeed(const String* v) {
	LocalPlayer_Instance.Hacks.SpeedMultiplier = Menu_Float(v);
	Options_Set(OPT_SPEED_FACTOR, v);
}

static void HacksSettingsScreen_GetClipping(String* v) { Menu_GetBool(v, Game_CameraClipping); }
static void HacksSettingsScreen_SetClipping(const String* v) {
	Game_CameraClipping = Menu_SetBool(v, OPT_CAMERA_CLIPPING);
}

static void HacksSettingsScreen_GetJump(String* v) { String_AppendFloat(v, LocalPlayer_JumpHeight(), 3); }
static void HacksSettingsScreen_SetJump(const String* v) {
	struct PhysicsComp* physics = &LocalPlayer_Instance.Physics;
	char strBuffer[STRING_SIZE];
	String str = String_FromArray(strBuffer);

	PhysicsComp_CalculateJumpVelocity(physics, Menu_Float(v));
	physics->UserJumpVel = physics->JumpVel;
	
	String_AppendFloat(&str, physics->JumpVel, 8);
	Options_Set(OPT_JUMP_VELOCITY, &str);
}

static void HacksSettingsScreen_GetWOMHacks(String* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.WOMStyleHacks); }
static void HacksSettingsScreen_SetWOMHacks(const String* v) {
	LocalPlayer_Instance.Hacks.WOMStyleHacks = Menu_SetBool(v, OPT_WOM_STYLE_HACKS);
}

static void HacksSettingsScreen_GetFullStep(String* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.FullBlockStep); }
static void HacksSettingsScreen_SetFullStep(const String* v) {
	LocalPlayer_Instance.Hacks.FullBlockStep = Menu_SetBool(v, OPT_FULL_BLOCK_STEP);
}

static void HacksSettingsScreen_GetPushback(String* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.PushbackPlacing); }
static void HacksSettingsScreen_SetPushback(const String* v) {
	LocalPlayer_Instance.Hacks.PushbackPlacing = Menu_SetBool(v, OPT_PUSHBACK_PLACING);
}

static void HacksSettingsScreen_GetLiquids(String* v) { Menu_GetBool(v, Game_BreakableLiquids); }
static void HacksSettingsScreen_SetLiquids(const String* v) {
	Game_BreakableLiquids = Menu_SetBool(v, OPT_MODIFIABLE_LIQUIDS);
}

static void HacksSettingsScreen_GetSlide(String* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.NoclipSlide); }
static void HacksSettingsScreen_SetSlide(const String* v) {
	LocalPlayer_Instance.Hacks.NoclipSlide = Menu_SetBool(v, OPT_NOCLIP_SLIDE);
}

static void HacksSettingsScreen_GetFOV(String* v) { String_AppendInt(v, Game_Fov); }
static void HacksSettingsScreen_SetFOV(const String* v) {
	Game_Fov = Menu_Int(v);
	if (Game_ZoomFov > Game_Fov) Game_ZoomFov = Game_Fov;

	Options_Set(OPT_FIELD_OF_VIEW, v);
	Game_UpdateProjection();
}

static void HacksSettingsScreen_CheckHacksAllowed(void* screen) {
	struct MenuOptionsScreen* s = screen;
	struct Widget** widgets = s->Widgets;
	int i;

	for (i = 0; i < s->WidgetsCount; i++) {
		if (!widgets[i]) continue;
		widgets[i]->Disabled = false;
	}

	struct LocalPlayer* p = &LocalPlayer_Instance;
	bool noGlobalHacks = !p->Hacks.CanAnyHacks || !p->Hacks.Enabled;
	widgets[3]->Disabled = noGlobalHacks || !p->Hacks.CanSpeed;
	widgets[4]->Disabled = noGlobalHacks || !p->Hacks.CanSpeed;
	widgets[5]->Disabled = noGlobalHacks || !p->Hacks.CanSpeed;
	widgets[7]->Disabled = noGlobalHacks || !p->Hacks.CanPushbackBlocks;
}

static void HacksSettingsScreen_ContextLost(void* screen) {
	struct MenuOptionsScreen* s = screen;
	MenuOptionsScreen_ContextLost(s);
	Event_UnregisterVoid(&UserEvents_HackPermissionsChanged, s, HacksSettingsScreen_CheckHacksAllowed);
}

static void HacksSettingsScreen_ContextRecreated(void* screen) {
	struct MenuOptionsScreen* s = screen;
	struct Widget** widgets = s->Widgets;
	Event_RegisterVoid(&UserEvents_HackPermissionsChanged, s, HacksSettingsScreen_CheckHacksAllowed);

	MenuOptionsScreen_Make(s, 0, -1, -150, "Hacks enabled",    MenuOptionsScreen_Bool,
		HacksSettingsScreen_GetHacks,    HacksSettingsScreen_SetHacks);
	MenuOptionsScreen_Make(s, 1, -1, -100, "Speed multiplier", MenuOptionsScreen_Input,
		HacksSettingsScreen_GetSpeed,    HacksSettingsScreen_SetSpeed);
	MenuOptionsScreen_Make(s, 2, -1,  -50, "Camera clipping",  MenuOptionsScreen_Bool,
		HacksSettingsScreen_GetClipping, HacksSettingsScreen_SetClipping);
	MenuOptionsScreen_Make(s, 3, -1,    0, "Jump height",      MenuOptionsScreen_Input,
		HacksSettingsScreen_GetJump,     HacksSettingsScreen_SetJump);
	MenuOptionsScreen_Make(s, 4, -1,   50, "WOM style hacks",  MenuOptionsScreen_Bool,
		HacksSettingsScreen_GetWOMHacks, HacksSettingsScreen_SetWOMHacks);

	MenuOptionsScreen_Make(s, 5, 1, -150, "Full block stepping", MenuOptionsScreen_Bool,
		HacksSettingsScreen_GetFullStep, HacksSettingsScreen_SetFullStep);
	MenuOptionsScreen_Make(s, 6, 1, -100, "Breakable liquids",   MenuOptionsScreen_Bool,
		HacksSettingsScreen_GetLiquids,  HacksSettingsScreen_SetLiquids);
	MenuOptionsScreen_Make(s, 7, 1,  -50, "Pushback placing",    MenuOptionsScreen_Bool,
		HacksSettingsScreen_GetPushback, HacksSettingsScreen_SetPushback);
	MenuOptionsScreen_Make(s, 8, 1,    0, "Noclip slide",        MenuOptionsScreen_Bool,
		HacksSettingsScreen_GetSlide,    HacksSettingsScreen_SetSlide);
	MenuOptionsScreen_Make(s, 9, 1,   50, "Field of view",       MenuOptionsScreen_Input,
		HacksSettingsScreen_GetFOV,      HacksSettingsScreen_SetFOV);

	Menu_Back(s, 10, &s->Buttons[10], "Done", &s->TitleFont, Menu_SwitchOptions);
	widgets[11] = NULL; widgets[12] = NULL; widgets[13] = NULL;
}

struct Screen* HacksSettingsScreen_MakeInstance(void) {
	static struct ButtonWidget buttons[11];
	static struct MenuInputValidator validators[Array_Elems(buttons)];
	static const char* defaultValues[Array_Elems(buttons)];
	static struct Widget* widgets[Array_Elems(buttons) + 3];

	static const char* descs[Array_Elems(buttons)];
	descs[2] = "&eIf &fON&e, then the third person cameras will limit|&etheir zoom distance if they hit a solid block.";
	descs[3] = "&eSets how many blocks high you can jump up.|&eNote: You jump much higher when holding down the Speed key binding.";
	descs[7] = \
		"&eIf &fON&e, placing blocks that intersect your own position cause|" \
		"&ethe block to be placed, and you to be moved out of the way.|" \
		"&fThis is mainly useful for quick pillaring/towering.";
	descs[8] = "&eIf &fOFF&e, you will immediately stop when in noclip|&emode and no movement keys are held down.";
	
	/* TODO: Is this needed because user may not always use . for decimal point? */
	static char jumpHeightBuffer[STRING_INT_CHARS];
	String jumpHeight = String_FromArray(jumpHeightBuffer);
	String_AppendFloat(&jumpHeight, 1.233f, 3);

	validators[1]    = MenuInputValidator_Float(0.10f, 50.00f);
	defaultValues[1] = "10";
	validators[3]    = MenuInputValidator_Float(0.10f, 2048.00f);
	defaultValues[3] = jumpHeightBuffer;
	validators[9]    = MenuInputValidator_Int(1, 150);
	defaultValues[9] = "70";

	struct Screen* s = MenuOptionsScreen_MakeInstance(widgets, Array_Elems(widgets), buttons,
		HacksSettingsScreen_ContextRecreated, validators, defaultValues, descs, Array_Elems(buttons));
	s->VTABLE->ContextLost = HacksSettingsScreen_ContextLost;
	return s;
}


/*########################################################################################################################*
*----------------------------------------------------MiscOptionsScreen----------------------------------------------------*
*#########################################################################################################################*/
static void MiscOptionsScreen_GetReach(String* v) { String_AppendFloat(v, LocalPlayer_Instance.ReachDistance, 2); }
static void MiscOptionsScreen_SetReach(const String* v) { LocalPlayer_Instance.ReachDistance = Menu_Float(v); }

static void MiscOptionsScreen_GetMusic(String* v) { String_AppendInt(v, Game_MusicVolume); }
static void MiscOptionsScreen_SetMusic(const String* v) {
	Game_MusicVolume = Menu_Int(v);
	Options_Set(OPT_MUSIC_VOLUME, v);
	Audio_SetMusic(Game_MusicVolume);
}

static void MiscOptionsScreen_GetSounds(String* v) { String_AppendInt(v, Game_SoundsVolume); }
static void MiscOptionsScreen_SetSounds(const String* v) {
	Game_SoundsVolume = Menu_Int(v);
	Options_Set(OPT_SOUND_VOLUME, v);
	Audio_SetSounds(Game_SoundsVolume);
}

static void MiscOptionsScreen_GetViewBob(String* v) { Menu_GetBool(v, Game_ViewBobbing); }
static void MiscOptionsScreen_SetViewBob(const String* v) { Game_ViewBobbing = Menu_SetBool(v, OPT_VIEW_BOBBING); }

static void MiscOptionsScreen_GetPhysics(String* v) { Menu_GetBool(v, Physics_Enabled); }
static void MiscOptionsScreen_SetPhysics(const String* v) {
	Physics_SetEnabled(Menu_SetBool(v, OPT_BLOCK_PHYSICS));
}

static void MiscOptionsScreen_GetAutoClose(String* v) { Menu_GetBool(v, Options_GetBool(OPT_AUTO_CLOSE_LAUNCHER, false)); }
static void MiscOptionsScreen_SetAutoClose(const String* v) { Menu_SetBool(v, OPT_AUTO_CLOSE_LAUNCHER); }

static void MiscOptionsScreen_GetInvert(String* v) { Menu_GetBool(v, Game_InvertMouse); }
static void MiscOptionsScreen_SetInvert(const String* v) { Game_InvertMouse = Menu_SetBool(v, OPT_INVERT_MOUSE); }

static void MiscOptionsScreen_GetSensitivity(String* v) { String_AppendInt(v, Game_MouseSensitivity); }
static void MiscOptionsScreen_SetSensitivity(const String* v) {
	Game_MouseSensitivity = Menu_Int(v);
	Options_Set(OPT_SENSITIVITY, v);
}

static void MiscOptionsScreen_ContextRecreated(void* screen) {
	struct MenuOptionsScreen* s = screen;
	struct Widget** widgets = s->Widgets;

	MenuOptionsScreen_Make(s, 0, -1, -100, "Reach distance", MenuOptionsScreen_Input,
		MiscOptionsScreen_GetReach,       MiscOptionsScreen_SetReach);
	MenuOptionsScreen_Make(s, 1, -1,  -50, "Music volume",   MenuOptionsScreen_Input,
		MiscOptionsScreen_GetMusic,       MiscOptionsScreen_SetMusic);
	MenuOptionsScreen_Make(s, 2, -1,    0, "Sounds volume",  MenuOptionsScreen_Input,
		MiscOptionsScreen_GetSounds,      MiscOptionsScreen_SetSounds);
	MenuOptionsScreen_Make(s, 3, -1,   50, "View bobbing",   MenuOptionsScreen_Bool,
		MiscOptionsScreen_GetViewBob,     MiscOptionsScreen_SetViewBob);

	MenuOptionsScreen_Make(s, 4, 1, -100, "Block physics",       MenuOptionsScreen_Bool,
		MiscOptionsScreen_GetPhysics,     MiscOptionsScreen_SetPhysics);
	MenuOptionsScreen_Make(s, 5, 1,  -50, "Auto close launcher", MenuOptionsScreen_Bool,
		MiscOptionsScreen_GetAutoClose,   MiscOptionsScreen_SetAutoClose);
	MenuOptionsScreen_Make(s, 6, 1,    0, "Invert mouse",        MenuOptionsScreen_Bool,
		MiscOptionsScreen_GetInvert,      MiscOptionsScreen_SetInvert);
	MenuOptionsScreen_Make(s, 7, 1,   50, "Mouse sensitivity",   MenuOptionsScreen_Input,
		MiscOptionsScreen_GetSensitivity, MiscOptionsScreen_SetSensitivity);

	Menu_Back(s, 8, &s->Buttons[8], "Done", &s->TitleFont, Menu_SwitchOptions);
	widgets[9] = NULL; widgets[10] = NULL; widgets[11] = NULL;

	/* Disable certain options */
	if (!ServerConnection_IsSinglePlayer) Menu_Remove(s, 0);
	if (!ServerConnection_IsSinglePlayer) Menu_Remove(s, 4);
}

struct Screen* MiscOptionsScreen_MakeInstance(void) {
	static struct ButtonWidget buttons[9];
	static struct MenuInputValidator validators[Array_Elems(buttons)];
	static const char* defaultValues[Array_Elems(buttons)];
	static struct Widget* widgets[Array_Elems(buttons) + 3];

	validators[0]    = MenuInputValidator_Float(1.00f, 1024.00f);
	defaultValues[0] = "5";
	validators[1]    = MenuInputValidator_Int(0, 100);
	defaultValues[1] = "0";
	validators[2]    = MenuInputValidator_Int(0, 100);
	defaultValues[2] = "0";
	validators[7]    = MenuInputValidator_Int(1, 200);
	defaultValues[7] = "30";

	return MenuOptionsScreen_MakeInstance(widgets, Array_Elems(widgets), buttons,
		MiscOptionsScreen_ContextRecreated, validators, defaultValues, NULL, 0);
}


/*########################################################################################################################*
*-----------------------------------------------------NostalgiaScreen-----------------------------------------------------*
*#########################################################################################################################*/
static void NostalgiaScreen_GetHand(String* v) { Menu_GetBool(v, Game_ClassicArmModel); }
static void NostalgiaScreen_SetHand(const String* v) { Game_ClassicArmModel = Menu_SetBool(v, OPT_CLASSIC_ARM_MODEL); }

static void NostalgiaScreen_GetAnim(String* v) { Menu_GetBool(v, !Game_SimpleArmsAnim); }
static void NostalgiaScreen_SetAnim(const String* v) {
	Game_SimpleArmsAnim = String_CaselessEqualsConst(v, "OFF");
	Options_SetBool(OPT_SIMPLE_ARMS_ANIM, Game_SimpleArmsAnim);
}

static void NostalgiaScreen_GetGui(String* v) { Menu_GetBool(v, Game_UseClassicGui); }
static void NostalgiaScreen_SetGui(const String* v) { Game_UseClassicGui = Menu_SetBool(v, OPT_CLASSIC_GUI); }

static void NostalgiaScreen_GetList(String* v) { Menu_GetBool(v, Game_UseClassicTabList); }
static void NostalgiaScreen_SetList(const String* v) { Game_UseClassicTabList = Menu_SetBool(v, OPT_CLASSIC_TABLIST); }

static void NostalgiaScreen_GetOpts(String* v) { Menu_GetBool(v, Game_UseClassicOptions); }
static void NostalgiaScreen_SetOpts(const String* v) { Game_UseClassicOptions = Menu_SetBool(v, OPT_CLASSIC_OPTIONS); }

static void NostalgiaScreen_GetCustom(String* v) { Menu_GetBool(v, Game_AllowCustomBlocks); }
static void NostalgiaScreen_SetCustom(const String* v) { Game_AllowCustomBlocks = Menu_SetBool(v, OPT_CUSTOM_BLOCKS); }

static void NostalgiaScreen_GetCPE(String* v) { Menu_GetBool(v, Game_UseCPE); }
static void NostalgiaScreen_SetCPE(const String* v) { Game_UseCPE = Menu_SetBool(v, OPT_CPE); }

static void NostalgiaScreen_GetTexs(String* v) { Menu_GetBool(v, Game_AllowServerTextures); }
static void NostalgiaScreen_SetTexs(const String* v) { Game_AllowServerTextures = Menu_SetBool(v, OPT_SERVER_TEXTURES); }

static void NostalgiaScreen_SwitchBack(void* a, void* b) {
	if (Game_UseClassicOptions) { Menu_SwitchPause(a, b); } else { Menu_SwitchOptions(a, b); }
}

static void NostalgiaScreen_ContextRecreated(void* screen) {
	static String descText = String_FromConst("&eButtons on the right require restarting game");
	struct MenuOptionsScreen* s = screen;
	static struct TextWidget desc;

	MenuOptionsScreen_Make(s, 0, -1, -150, "Classic hand model",   MenuOptionsScreen_Bool,
		NostalgiaScreen_GetHand,   NostalgiaScreen_SetHand);
	MenuOptionsScreen_Make(s, 1, -1, -100, "Classic walk anim",    MenuOptionsScreen_Bool, 
		NostalgiaScreen_GetAnim,   NostalgiaScreen_SetAnim);
	MenuOptionsScreen_Make(s, 2, -1,  -50, "Classic gui textures", MenuOptionsScreen_Bool,
		NostalgiaScreen_GetGui,    NostalgiaScreen_SetGui);
	MenuOptionsScreen_Make(s, 3, -1,    0, "Classic player list",  MenuOptionsScreen_Bool, 
		NostalgiaScreen_GetList,   NostalgiaScreen_SetList);
	MenuOptionsScreen_Make(s, 4, -1,   50, "Classic options",      MenuOptionsScreen_Bool, 
		NostalgiaScreen_GetOpts,   NostalgiaScreen_SetOpts);

	MenuOptionsScreen_Make(s, 5, 1, -150, "Allow custom blocks", MenuOptionsScreen_Bool, 
		NostalgiaScreen_GetCustom, NostalgiaScreen_SetCustom);
	MenuOptionsScreen_Make(s, 6, 1, -100, "Use CPE",             MenuOptionsScreen_Bool, 
		NostalgiaScreen_GetCPE,    NostalgiaScreen_SetCPE);
	MenuOptionsScreen_Make(s, 7, 1,  -50, "Use server textures", MenuOptionsScreen_Bool, 
		NostalgiaScreen_GetTexs,   NostalgiaScreen_SetTexs);

	Menu_Back(s,  8, &s->Buttons[8], "Done", &s->TitleFont, NostalgiaScreen_SwitchBack);
	Menu_Label(s, 9, &desc, &descText, &s->TextFont, 
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 100);
}

struct Screen* NostalgiaScreen_MakeInstance(void) {
	static struct ButtonWidget buttons[9];
	static struct Widget* widgets[Array_Elems(buttons) + 1];

	return MenuOptionsScreen_MakeInstance(widgets, Array_Elems(widgets), buttons,
		NostalgiaScreen_ContextRecreated, NULL, NULL, NULL, 0);
}


/*########################################################################################################################*
*---------------------------------------------------------Overlay---------------------------------------------------------*
*#########################################################################################################################*/
static bool Overlay_KeyDown(void* screen, Key key) { return true; }

static void Overlay_MakeLabels(void* menu, struct TextWidget* labels, const String* lines) {
	struct MenuScreen* s = menu;
	Menu_Label(s, 0, &labels[0], &lines[0], &s->TitleFont,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -120);

	PackedCol col = PACKEDCOL_CONST(224, 224, 224, 255);
	int i;
	for (i = 1; i < 4; i++) {
		if (!lines[i].length) continue;

		Menu_Label(s, i, &labels[i], &lines[i], &s->TextFont,
			ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -70 + 20 * i);
		labels[i].Col = col;
	}
}

static void WarningOverlay_MakeButtons(void* menu, struct ButtonWidget* btns, bool always, Widget_LeftClick yesClick, Widget_LeftClick noClick) {
	static String yes = String_FromConst("Yes");
	static String no  = String_FromConst("No");
	static String alwaysYes = String_FromConst("Always yes");
	static String alwaysNo  = String_FromConst("Always no");

	struct MenuScreen* s = menu;
	Menu_Button(s, 4, &btns[0], 160, &yes, &s->TitleFont, yesClick, 
		ANCHOR_CENTRE, ANCHOR_CENTRE, -110, 30);
	Menu_Button(s, 5, &btns[1], 160, &no,  &s->TitleFont, noClick, 
		ANCHOR_CENTRE, ANCHOR_CENTRE,  110, 30);

	if (!always) return;
	Menu_Button(s, 6, &btns[2], 160, &alwaysYes, &s->TitleFont, yesClick, 
		ANCHOR_CENTRE, ANCHOR_CENTRE, -110, 85);
	Menu_Button(s, 7, &btns[3], 160, &alwaysNo,  &s->TitleFont, noClick, 
		ANCHOR_CENTRE, ANCHOR_CENTRE,  110, 85);
}
bool WarningOverlay_IsAlways(void* screen, void* w) { return Menu_Index(screen, w) >= 6; }


/*########################################################################################################################*
*------------------------------------------------------TexIdsOverlay------------------------------------------------------*
*#########################################################################################################################*/
#define TEXID_OVERLAY_VERTICES_COUNT (ATLAS2D_TILES_PER_ROW * ATLAS2D_TILES_PER_ROW * 4)
struct TexIdsOverlay TexIdsOverlay_Instance;
static void TexIdsOverlay_ContextLost(void* screen) {
	struct TexIdsOverlay* s = screen;
	Menu_ContextLost(s);
	Gfx_DeleteVb(&s->DynamicVb);
	TextAtlas_Free(&s->IdAtlas);
}

static void TexIdsOverlay_ContextRecreated(void* screen) {
	static String chars  = String_FromConst("0123456789");
	static String prefix = String_FromConst("f");
	static String title  = String_FromConst("Texture ID reference sheet");

	struct TexIdsOverlay* s = screen;
	int size = Game_Height / ATLAS2D_TILES_PER_ROW;
	size = (size / 8) * 8;
	Math_Clamp(size, 8, 40);

	s->DynamicVb = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FT2FC4B, TEXID_OVERLAY_VERTICES_COUNT);
	TextAtlas_Make(&s->IdAtlas, &chars, &s->TextFont, &prefix);

	s->XOffset  = Gui_CalcPos(ANCHOR_CENTRE, 0, size * Atlas2D_RowsCount,     Game_Width);
	s->YOffset  = Gui_CalcPos(ANCHOR_CENTRE, 0, size * ATLAS2D_TILES_PER_ROW, Game_Height);
	s->TileSize = size;
	
	Menu_Label(s, 0, &s->Title, &title, &s->TitleFont,
		ANCHOR_CENTRE, ANCHOR_MIN, 0, s->YOffset - 30);
}

static void TexIdsOverlay_RenderTerrain(struct TexIdsOverlay* s) {
	VertexP3fT2fC4b vertices[TEXID_OVERLAY_VERTICES_COUNT];
	int i, size = s->TileSize;

	struct Texture tex;
	tex.U1 = 0.0f; tex.U2 = UV2_Scale;
	tex.Width = size; tex.Height = size;

	for (i = 0; i < ATLAS2D_TILES_PER_ROW * ATLAS2D_TILES_PER_ROW;) {
		VertexP3fT2fC4b* ptr = vertices;
		int texIdx = Atlas1D_Index(i + s->BaseTexLoc), end = i + Atlas1D_TilesPerAtlas;

		for (; i < end; i++) {
			tex.X = s->XOffset + Atlas2D_TileX(i) * size;
			tex.Y = s->YOffset + Atlas2D_TileY(i) * size;

			tex.V1 = Atlas1D_RowId(i + s->BaseTexLoc) * Atlas1D_InvTileSize;
			tex.V2 = tex.V1                    + UV2_Scale * Atlas1D_InvTileSize;

			PackedCol col = PACKEDCOL_WHITE;
			GfxCommon_Make2DQuad(&tex, col, &ptr);
		}

		Gfx_BindTexture(Atlas1D_TexIds[texIdx]);
		int count = (int)(ptr - vertices);
		GfxCommon_UpdateDynamicVb_IndexedTris(s->DynamicVb, vertices, count);
	}
}

static void TexIdsOverlay_RenderTextOverlay(struct TexIdsOverlay* s) {
	int x, y, size = s->TileSize;
	VertexP3fT2fC4b vertices[TEXID_OVERLAY_VERTICES_COUNT];
	VertexP3fT2fC4b* ptr = vertices;

	struct TextAtlas* idAtlas = &s->IdAtlas;
	idAtlas->Tex.Y = (s->YOffset + (size - idAtlas->Tex.Height));

	for (y = 0; y < ATLAS2D_TILES_PER_ROW; y++) {
		for (x = 0; x < ATLAS2D_TILES_PER_ROW; x++) {
			idAtlas->CurX = s->XOffset + size * x + 3; /* offset text by 3 pixels */
			int id = x + y * ATLAS2D_TILES_PER_ROW;
			TextAtlas_AddInt(idAtlas, id + s->BaseTexLoc, &ptr);
		}
		idAtlas->Tex.Y += size;

		if ((y % 4) != 3) continue;
		Gfx_BindTexture(idAtlas->Tex.ID);
		int count = (int)(ptr - vertices);
		GfxCommon_UpdateDynamicVb_IndexedTris(s->DynamicVb, vertices, count);
		ptr = vertices;
	}
}

static void TexIdsOverlay_Init(void* screen) {
	struct TexIdsOverlay* s = screen;
	Drawer2D_MakeFont(&s->TextFont, 8, FONT_STYLE_NORMAL);
	MenuScreen_Init(s);
}

static void TexIdsOverlay_Render(void* screen, double delta) {
	struct TexIdsOverlay* s = screen;
	Menu_RenderBounds();
	Gfx_SetTexturing(true);
	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	Menu_Render(s, delta);

	int rows = Atlas2D_RowsCount, origXOffset = s->XOffset;
	s->BaseTexLoc = 0;
	while (rows > 0) {
		TexIdsOverlay_RenderTerrain(s);
		TexIdsOverlay_RenderTextOverlay(s);
		rows -= ATLAS2D_TILES_PER_ROW;

		s->XOffset    += s->TileSize      * ATLAS2D_TILES_PER_ROW;
		s->BaseTexLoc += ATLAS2D_TILES_PER_ROW * ATLAS2D_TILES_PER_ROW;
	}

	s->XOffset = origXOffset;
	Gfx_SetTexturing(false);
}

static bool TexIdsOverlay_KeyDown(void* screen, Key key) {
	struct TexIdsOverlay* s = screen;
	if (key == KeyBind_Get(KeyBind_IDOverlay) || key == KeyBind_Get(KeyBind_PauseOrExit)) {
		Gui_FreeOverlay(s); return true;
	}

	struct Screen* active = Gui_GetUnderlyingScreen();
	return Elem_HandlesKeyDown(active, key);
}

static bool TexIdsOverlay_KeyPress(void* screen, char keyChar) {
	struct Screen* active = Gui_GetUnderlyingScreen();
	return Elem_HandlesKeyPress(active, keyChar);
}

static bool TexIdsOverlay_KeyUp(void* screen, Key key) {
	struct Screen* active = Gui_GetUnderlyingScreen();
	return Elem_HandlesKeyUp(active, key);
}

struct ScreenVTABLE TexIdsOverlay_VTABLE = {
	TexIdsOverlay_Init,    TexIdsOverlay_Render, MenuScreen_Free,        Gui_DefaultRecreate,
	TexIdsOverlay_KeyDown, TexIdsOverlay_KeyUp,  TexIdsOverlay_KeyPress,
	Menu_MouseDown,        Menu_MouseUp,         Menu_MouseMove,         MenuScreen_MouseScroll,
	Menu_OnResize,         TexIdsOverlay_ContextLost, TexIdsOverlay_ContextRecreated,
};
struct Screen* TexIdsOverlay_MakeInstance(void) {
	static struct Widget* widgets[1];
	struct TexIdsOverlay* s = &TexIdsOverlay_Instance;
	
	s->HandlesAllInput = true;
	s->Widgets         = widgets;
	s->WidgetsCount    = Array_Elems(widgets);

	s->VTABLE = &TexIdsOverlay_VTABLE;
	return (struct Screen*)s;
}


/*########################################################################################################################*
*----------------------------------------------------UrlWarningOverlay----------------------------------------------------*
*#########################################################################################################################*/
struct UrlWarningOverlay UrlWarningOverlay_Instance;
static void UrlWarningOverlay_OpenUrl(void* screen, void* b) {
	struct UrlWarningOverlay* s = screen;
	Gui_FreeOverlay(s);
	Platform_StartShell(&s->Url);
}

static void UrlWarningOverlay_AppendUrl(void* screen, void* b) {
	struct UrlWarningOverlay* s = screen;
	Gui_FreeOverlay(s);
	if (Game_ClickableChat) { HUDScreen_AppendInput(Gui_HUD, &s->Url); }
}

static void UrlWarningOverlay_ContextRecreated(void* screen) {
	struct UrlWarningOverlay* s = screen;
	String lines[4] = {
		String_FromConst("&eAre you sure you want to open this link?"), s->Url,
		String_FromConst("Be careful - links from strangers may be websites that"),
		String_FromConst(" have viruses, or things you may not want to open/see."),
	};

	Overlay_MakeLabels(s, s->Labels, lines);
	WarningOverlay_MakeButtons((struct MenuScreen*)s, s->Buttons, false,
		UrlWarningOverlay_OpenUrl, UrlWarningOverlay_AppendUrl);
}

struct ScreenVTABLE UrlWarningOverlay_VTABLE = {
	MenuScreen_Init, MenuScreen_Render,  MenuScreen_Free, Gui_DefaultRecreate,
	Overlay_KeyDown, Menu_KeyUp,         Menu_KeyPress,
	Menu_MouseDown,  Menu_MouseUp,       Menu_MouseMove,  MenuScreen_MouseScroll,
	Menu_OnResize,   Menu_ContextLost,   UrlWarningOverlay_ContextRecreated,
};
struct Screen* UrlWarningOverlay_MakeInstance(const String* url) {
	static struct Widget* widgets[6];
	struct UrlWarningOverlay* s = &UrlWarningOverlay_Instance;

	s->HandlesAllInput = true;
	s->Widgets         = widgets;
	s->WidgetsCount    = Array_Elems(widgets);

	String dstUrl = String_FromArray(s->__UrlBuffer);
	String_Copy(&dstUrl, url);
	s->Url = dstUrl;

	s->VTABLE = &UrlWarningOverlay_VTABLE;
	return (struct Screen*)s;
}


/*########################################################################################################################*
*----------------------------------------------------ConfirmDenyOverlay---------------------------------------------------*
*#########################################################################################################################*/
struct ConfirmDenyOverlay ConfirmDenyOverlay_Instance;
static void ConfirmDenyOverlay_ConfirmNoClick(void* screen, void* b) {
	struct ConfirmDenyOverlay* s = screen;
	String url;

	Gui_FreeOverlay(s);
	url = s->Url;

	if (s->AlwaysDeny && !TextureCache_HasDenied(&url)) {
		TextureCache_Deny(&url);
	}
}

static void ConfirmDenyOverlay_GoBackClick(void* screen, void* b) {
	struct ConfirmDenyOverlay* s = screen;
	struct Screen* overlay;

	Gui_FreeOverlay(s);
	overlay = TexPackOverlay_MakeInstance(&s->Url);
	Gui_ShowOverlay(overlay, true);
}

static void ConfirmDenyOverlay_ContextRecreated(void* screen) {
	static String imSure = String_FromConst("I'm sure");
	static String goBack = String_FromConst("Go back");
	struct ConfirmDenyOverlay* s = screen;

	String lines[4] = {
		String_FromConst("&eYou might be missing out."),
		String_FromConst("Texture packs can play a vital role in the look and feel of maps."),
		String_Empty,
		String_FromConst("Sure you don't want to download the texture pack?")
	};
	Overlay_MakeLabels(s, s->Labels, lines);

	Menu_Button(s, 4, &s->Buttons[0], 160, &imSure, &s->TitleFont, ConfirmDenyOverlay_ConfirmNoClick,
		ANCHOR_CENTRE, ANCHOR_CENTRE, -110, 30);
	Menu_Button(s, 5, &s->Buttons[1], 160, &goBack, &s->TitleFont, ConfirmDenyOverlay_GoBackClick,
		ANCHOR_CENTRE, ANCHOR_CENTRE,  110, 30);
}

struct ScreenVTABLE ConfirmDenyOverlay_VTABLE = {
	MenuScreen_Init, MenuScreen_Render,  MenuScreen_Free, Gui_DefaultRecreate,
	Overlay_KeyDown, Menu_KeyUp,         Menu_KeyPress,
	Menu_MouseDown,  Menu_MouseUp,       Menu_MouseMove,  MenuScreen_MouseScroll,
	Menu_OnResize,   Menu_ContextLost,   ConfirmDenyOverlay_ContextRecreated,
};
struct Screen* ConfirmDenyOverlay_MakeInstance(const String* url, bool alwaysDeny) {
	static struct Widget* widgets[6];
	struct ConfirmDenyOverlay* s = &ConfirmDenyOverlay_Instance;
	
	s->HandlesAllInput = true;
	s->Widgets         = widgets;
	s->WidgetsCount    = Array_Elems(widgets);

	String dstUrl = String_FromArray(s->__UrlBuffer);
	String_Copy(&dstUrl, url);
	s->Url = dstUrl;
	s->AlwaysDeny = alwaysDeny;

	s->VTABLE = &ConfirmDenyOverlay_VTABLE;
	return (struct Screen*)s;
}


/*########################################################################################################################*
*-----------------------------------------------------TexPackOverlay------------------------------------------------------*
*#########################################################################################################################*/
struct TexPackOverlay TexPackOverlay_Instance;
static void TexPackOverlay_YesClick(void* screen, void* widget) {
	struct TexPackOverlay* s = screen;
	String url;
	bool isAlways;

	Gui_FreeOverlay(s);
	url = String_UNSAFE_SubstringAt(&s->Identifier, 3);

	ServerConnection_DownloadTexturePack(&url);
	isAlways = WarningOverlay_IsAlways(s, widget);
	if (isAlways && !TextureCache_HasAccepted(&url)) {
		TextureCache_Accept(&url);
	}
}

static void TexPackOverlay_NoClick(void* screen, void* widget) {
	struct TexPackOverlay* s = screen;
	String url;
	bool isAlways;

	Gui_FreeOverlay(s);
	url = String_UNSAFE_SubstringAt(&s->Identifier, 3);

	isAlways = WarningOverlay_IsAlways(s, widget);
	struct Screen* overlay = ConfirmDenyOverlay_MakeInstance(&url, isAlways);
	Gui_ShowOverlay(overlay, true);
}

static void TexPackOverlay_Render(void* screen, double delta) {
	struct AsyncRequest item;
	struct TexPackOverlay* s = screen;

	MenuScreen_Render(s, delta);
	if (!AsyncDownloader_Get(&s->Identifier, &item)) return;

	s->ContentLength = item.ResultSize;
	if (!s->ContentLength) return;
	Elem_Recreate(s);
}

static void TexPackOverlay_ContextRecreated(void* screen) {
	static String https = String_FromConst("https://");
	static String http = String_FromConst("http://");
	struct TexPackOverlay* s = screen;
	String url;

	url = String_UNSAFE_SubstringAt(&s->Identifier, 3);
	if (String_CaselessStarts(&url, &https)) {
		url = String_UNSAFE_SubstringAt(&url, https.length);
	}
	if (String_CaselessStarts(&url, &http)) {
		url = String_UNSAFE_SubstringAt(&url, http.length);
	}

	String lines[4] = {
		String_FromConst("Do you want to download the server's texture pack?"),
		String_FromConst("Texture pack url:"),
		url, 
		String_FromConst("Download size: Determining..."),
	};

	if (s->ContentLength) {
		char contentsBuffer[STRING_SIZE];
		String contents = String_FromArray(contentsBuffer);
		float contentLengthMB = s->ContentLength / (1024.0f * 1024.0f);

		String_Format1(&contents, "Download size: %f3 MB", &contentLengthMB);
		lines[3] = contents;
	}

	Overlay_MakeLabels(s, s->Labels, lines);
	WarningOverlay_MakeButtons((struct MenuScreen*)s, s->Buttons, true,
		TexPackOverlay_YesClick, TexPackOverlay_NoClick);
}

struct ScreenVTABLE TexPackOverlay_VTABLE = {
	MenuScreen_Init, TexPackOverlay_Render, MenuScreen_Free, Gui_DefaultRecreate,
	Overlay_KeyDown, Menu_KeyUp,            Menu_KeyPress,
	Menu_MouseDown,  Menu_MouseUp,          Menu_MouseMove,  MenuScreen_MouseScroll,
	Menu_OnResize,   Menu_ContextLost,      TexPackOverlay_ContextRecreated,
};
struct Screen* TexPackOverlay_MakeInstance(const String* url) {
	static struct Widget* widgets[8];
	struct TexPackOverlay* s = &TexPackOverlay_Instance;
	void* overlay;

	/* If we are showing a texture pack overlay, completely free that overlay */
	/* It doesn't matter anymore, because the new texture pack URL will always */
	/* replace/override the old texture pack URL associated with that overlay */
	overlay = &TexPackOverlay_Instance;
	if (Gui_IndexOverlay(overlay) >= 0) { Gui_FreeOverlay(overlay); }
	overlay = &ConfirmDenyOverlay_Instance;
	if (Gui_IndexOverlay(overlay) >= 0) { Gui_FreeOverlay(overlay); }

	s->HandlesAllInput = true;
	s->Widgets         = widgets;
	s->WidgetsCount    = Array_Elems(widgets);

	String identifier = String_FromArray(s->__IdentifierBuffer);
	String_Format1(&identifier, "CL_%s", url);
	s->Identifier    = identifier;
	s->ContentLength = 0;

	AsyncDownloader_GetContentLength(url, true, &identifier);
	s->VTABLE = &TexPackOverlay_VTABLE;
	return (struct Screen*)s;
}
