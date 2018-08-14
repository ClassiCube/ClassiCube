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
#include "World.h"
#include "Formats.h"
#include "ErrorHandler.h"
#include "BlockPhysics.h"
#include "MapRenderer.h"
#include "TexturePack.h"
#include "Audio.h"
#include "Screens.h"
#include "Gui.h"

#define MenuBase_Layout Screen_Layout struct Widget** Widgets; Int32 WidgetsCount;
struct MenuBase { MenuBase_Layout struct ButtonWidget* Buttons; };

#define LIST_SCREEN_ITEMS 5
#define LIST_SCREEN_BUTTONS (LIST_SCREEN_ITEMS + 3)

struct ListScreen {
	MenuBase_Layout
	struct ButtonWidget Buttons[LIST_SCREEN_BUTTONS];
	struct FontDesc Font;
	Real32 WheelAcc;
	Int32 CurrentIndex;
	Widget_LeftClick EntryClick;
	void (*UpdateEntry)(struct ButtonWidget* button, STRING_PURE String* text);
	String TitleText;
	struct TextWidget Title, Page;
	struct Widget* ListWidgets[LIST_SCREEN_BUTTONS + 2];
	StringsBuffer Entries; /* NOTE: this is the last member so we can avoid memsetting it to 0 */
};

typedef void (*Menu_ContextFunc)(void* obj);
#define MenuScreen_Layout MenuBase_Layout struct FontDesc TitleFont, TextFont; Menu_ContextFunc ContextLost, ContextRecreated;
struct MenuScreen { MenuScreen_Layout };

struct PauseScreen {
	MenuScreen_Layout
	struct ButtonWidget Buttons[8];
};

struct OptionsGroupScreen {
	MenuScreen_Layout
	struct ButtonWidget Buttons[8];
	struct TextWidget Desc;
	Int32 SelectedI;
};

struct EditHotkeyScreen {
	MenuScreen_Layout
	struct ButtonWidget Buttons[6];
	struct HotkeyData CurHotkey, OrigHotkey;
	Int32 SelectedI;
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
	Int32 CurI, BindsCount;
	const UChar** Descs;
	UInt8* Binds;
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
	UChar TextPathBuffer[String_BufferSize(FILENAME_SIZE)];
};

#define MENUOPTIONS_MAX_DESC 5
struct MenuOptionsScreen {
	MenuScreen_Layout
	struct ButtonWidget* Buttons;
	struct MenuInputValidator* Validators;
	const UChar** Descriptions;
	const UChar** DefaultValues;
	Int32 ActiveI, SelectedI, DescriptionsCount;
	struct ButtonWidget OK, Default;
	struct MenuInputWidget Input;
	struct TextGroupWidget ExtHelp;
	struct Texture ExtHelp_Textures[MENUOPTIONS_MAX_DESC];
	UChar ExtHelp_Buffer[String_BufferSize(MENUOPTIONS_MAX_DESC * TEXTGROUPWIDGET_LEN)];
};

struct TexIdsOverlay {
	MenuScreen_Layout
	struct ButtonWidget* Buttons;
	GfxResourceID DynamicVb;
	Int32 XOffset, YOffset, TileSize;
	struct TextAtlas IdAtlas;
	struct TextWidget Title;
};

struct UrlWarningOverlay {
	MenuScreen_Layout
	struct ButtonWidget Buttons[2];
	struct TextWidget Labels[4];
	UChar UrlBuffer[String_BufferSize(STRING_SIZE * 4)];
};

struct ConfirmDenyOverlay {
	MenuScreen_Layout
	struct ButtonWidget Buttons[2];
	struct TextWidget Labels[4];
	bool AlwaysDeny;
	UChar UrlBuffer[String_BufferSize(STRING_SIZE)];
};

struct TexPackOverlay {
	MenuScreen_Layout
	struct ButtonWidget Buttons[4];
	struct TextWidget Labels[4];
	UInt32 ContentLength;
	UChar IdentifierBuffer[String_BufferSize(STRING_SIZE + 4)];
};


/*########################################################################################################################*
*--------------------------------------------------------Menu base--------------------------------------------------------*
*#########################################################################################################################*/
static void Menu_Button(void* elem, Int32 i, struct ButtonWidget* button, Int32 width, String* text, struct FontDesc* font, 
	Widget_LeftClick onClick, UInt8 horAnchor, UInt8 verAnchor, Int32 x, Int32 y) {
	struct MenuBase* screen = (struct MenuBase*)elem;

	ButtonWidget_Create(button, width, text, font, onClick);
	screen->Widgets[i] = (struct Widget*)button;
	Widget_SetLocation(screen->Widgets[i], horAnchor, verAnchor, x, y);
}

static void Menu_Label(void* elem, Int32 i, struct TextWidget* label, String* text, struct FontDesc* font, 
	UInt8 horAnchor, UInt8 verAnchor, Int32 x, Int32 y) {
	struct MenuBase* screen = (struct MenuBase*)elem;

	TextWidget_Create(label, text, font);
	screen->Widgets[i] = (struct Widget*)label;
	Widget_SetLocation(screen->Widgets[i], horAnchor, verAnchor, x, y);
}

static void Menu_Input(void* elem, Int32 i, struct MenuInputWidget* input, Int32 width, String* text, struct FontDesc* font, 
	struct MenuInputValidator* validator, UInt8 horAnchor, UInt8 verAnchor, Int32 x, Int32 y) {
	struct MenuBase* screen = (struct MenuBase*)elem;

	MenuInputWidget_Create(input, width, 30, text, font, validator);
	screen->Widgets[i] = (struct Widget*)input;
	Widget_SetLocation(screen->Widgets[i], horAnchor, verAnchor, x, y);
	input->Base.ShowCaret = true;
}

static void Menu_Back(void* elem, Int32 i, struct ButtonWidget* button, Int32 width, String* text, Int32 y, struct FontDesc* font, Widget_LeftClick onClick) {
	Menu_Button(elem, i, button, width, text, font, onClick, ANCHOR_CENTRE, ANCHOR_MAX, 0, y);
}

static void Menu_DefaultBack(void* elem, Int32 i, struct ButtonWidget* button, bool toGame, struct FontDesc* font, Widget_LeftClick onClick) {
	Int32 width = Game_UseClassicOptions ? 400 : 200;
	String msg = String_FromReadonly(toGame ? "Back to game" : "Cancel");
	Menu_Button(elem, i, button, width, &msg, font, onClick, ANCHOR_CENTRE, ANCHOR_MAX, 0, 25);
}

static void Menu_Free(struct MenuBase* elem) {
	struct Widget** widgets = elem->Widgets;
	if (!widgets) return;

	Int32 i;
	for (i = 0; i < elem->WidgetsCount; i++) {
		if (!widgets[i]) continue;
		Elem_Free(widgets[i]);
	}
}

static void Menu_Reposition(struct MenuBase* elem) {
	struct Widget** widgets = elem->Widgets;
	if (!widgets) return;

	Int32 i;
	for (i = 0; i < elem->WidgetsCount; i++) {
		if (!widgets[i]) continue;
		Widget_Reposition(widgets[i]);
	}
}

static void Menu_Render(struct MenuBase* elem, Real64 delta) {
	struct Widget** widgets = elem->Widgets;
	if (!widgets) return;

	Int32 i;
	for (i = 0; i < elem->WidgetsCount; i++) {
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

static Int32 Menu_HandleMouseDown(struct MenuBase* elem, Int32 x, Int32 y, MouseButton btn) {
	struct Widget** widgets = elem->Widgets;
	Int32 i, count = elem->WidgetsCount;

	/* iterate backwards (because last elements rendered are shown over others) */
	for (i = count - 1; i >= 0; i--) {
		struct Widget* widget = widgets[i];
		if (!widget || !Widget_Contains(widget, x, y)) continue;
		if (widget->Disabled) return i;

		if (widget->MenuClick && btn == MouseButton_Left) {
			widget->MenuClick((struct GuiElem*)elem, (struct GuiElem*)widget);
		} else {
			Elem_HandlesMouseDown(widget, x, y, btn);
		}
		return i;
	}
	return -1;
}

static Int32 Menu_HandleMouseMove(struct MenuBase* elem, Int32 x, Int32 y) {
	struct Widget** widgets = elem->Widgets;
	Int32 i, count = elem->WidgetsCount;

	for (i = 0; i < count; i++) {
		struct Widget* widget = widgets[i];
		if (widget) widget->Active = false;
	}

	for (i = count - 1; i >= 0; i--) {
		struct Widget* widget = widgets[i];
		if (!widget || !Widget_Contains(widget, x, y)) continue;

		widget->Active = true;
		return i;
	}
	return -1;
}


/*########################################################################################################################*
*------------------------------------------------------Menu utilities-----------------------------------------------------*
*#########################################################################################################################*/
static Int32 Menu_Index(struct MenuBase* elem, struct Widget* w) {
	struct Widget** widgets = elem->Widgets;
	Int32 i;

	for (i = 0; i < elem->WidgetsCount; i++) {
		if (widgets[i] == w) return i;
	}
	return -1;
}

static void Menu_HandleFontChange(struct GuiElem* elem) {
	Event_RaiseVoid(&ChatEvents_FontChanged);
	Elem_Recreate(elem);
	Gui_RefreshHud();
	Elem_HandlesMouseMove(elem, Mouse_X, Mouse_Y);
}

static Int32 Menu_Int32(STRING_PURE String* v) { Int32 value; Convert_TryParseInt32(v, &value); return value; }
static Real32 Menu_Real32(STRING_PURE String* v) { Real32 value; Convert_TryParseReal32(v, &value); return value; }
static PackedCol Menu_HexCol(STRING_PURE String* v) { PackedCol value; PackedCol_TryParseHex(v, &value); return value; }

static void Menu_SwitchOptions(struct GuiElem* a, struct GuiElem* b)        { Gui_ReplaceActive(OptionsGroupScreen_MakeInstance()); }
static void Menu_SwitchPause(struct GuiElem* a, struct GuiElem* b)          { Gui_ReplaceActive(PauseScreen_MakeInstance()); }
static void Menu_SwitchClassicOptions(struct GuiElem* a, struct GuiElem* b) { Gui_ReplaceActive(ClassicOptionsScreen_MakeInstance()); }

static void Menu_SwitchKeysClassic(struct GuiElem* a, struct GuiElem* b)      { Gui_ReplaceActive(ClassicKeyBindingsScreen_MakeInstance()); }
static void Menu_SwitchKeysClassicHacks(struct GuiElem* a, struct GuiElem* b) { Gui_ReplaceActive(ClassicHacksKeyBindingsScreen_MakeInstance()); }
static void Menu_SwitchKeysNormal(struct GuiElem* a, struct GuiElem* b)       { Gui_ReplaceActive(NormalKeyBindingsScreen_MakeInstance()); }
static void Menu_SwitchKeysHacks(struct GuiElem* a, struct GuiElem* b)        { Gui_ReplaceActive(HacksKeyBindingsScreen_MakeInstance()); }
static void Menu_SwitchKeysOther(struct GuiElem* a, struct GuiElem* b)        { Gui_ReplaceActive(OtherKeyBindingsScreen_MakeInstance()); }
static void Menu_SwitchKeysMouse(struct GuiElem* a, struct GuiElem* b)        { Gui_ReplaceActive(MouseKeyBindingsScreen_MakeInstance()); }

static void Menu_SwitchMisc(struct GuiElem* a, struct GuiElem* b)      { Gui_ReplaceActive(MiscOptionsScreen_MakeInstance()); }
static void Menu_SwitchGui(struct GuiElem* a, struct GuiElem* b)       { Gui_ReplaceActive(GuiOptionsScreen_MakeInstance()); }
static void Menu_SwitchGfx(struct GuiElem* a, struct GuiElem* b)       { Gui_ReplaceActive(GraphicsOptionsScreen_MakeInstance()); }
static void Menu_SwitchHacks(struct GuiElem* a, struct GuiElem* b)     { Gui_ReplaceActive(HacksSettingsScreen_MakeInstance()); }
static void Menu_SwitchEnv(struct GuiElem* a, struct GuiElem* b)       { Gui_ReplaceActive(EnvSettingsScreen_MakeInstance()); }
static void Menu_SwitchNostalgia(struct GuiElem* a, struct GuiElem* b) { Gui_ReplaceActive(NostalgiaScreen_MakeInstance()); }

static void Menu_SwitchGenLevel(struct GuiElem* a, struct GuiElem* b)        { Gui_ReplaceActive(GenLevelScreen_MakeInstance()); }
static void Menu_SwitchClassicGenLevel(struct GuiElem* a, struct GuiElem* b) { Gui_ReplaceActive(ClassicGenScreen_MakeInstance()); }
static void Menu_SwitchLoadLevel(struct GuiElem* a, struct GuiElem* b)       { Gui_ReplaceActive(LoadLevelScreen_MakeInstance()); }
static void Menu_SwitchSaveLevel(struct GuiElem* a, struct GuiElem* b)       { Gui_ReplaceActive(SaveLevelScreen_MakeInstance()); }
static void Menu_SwitchTexPacks(struct GuiElem* a, struct GuiElem* b)        { Gui_ReplaceActive(TexturePackScreen_MakeInstance()); }
static void Menu_SwitchHotkeys(struct GuiElem* a, struct GuiElem* b)         { Gui_ReplaceActive(HotkeyListScreen_MakeInstance()); }
static void Menu_SwitchFont(struct GuiElem* a, struct GuiElem* b)            { Gui_ReplaceActive(FontListScreen_MakeInstance()); }


/*########################################################################################################################*
*--------------------------------------------------------ListScreen-------------------------------------------------------*
*#########################################################################################################################*/
struct GuiElementVTABLE ListScreen_VTABLE;
struct ListScreen ListScreen_Instance;
#define LIST_SCREEN_EMPTY "-----"

STRING_REF String ListScreen_UNSAFE_Get(struct ListScreen* screen, Int32 index) {
	if (index >= 0 && index < screen->Entries.Count) {
		return StringsBuffer_UNSAFE_Get(&screen->Entries, index);
	} else {
		String str = String_FromConst(LIST_SCREEN_EMPTY); return str;
	}
}

static void ListScreen_MakeText(struct ListScreen* screen, Int32 i) {
	String text = ListScreen_UNSAFE_Get(screen, screen->CurrentIndex + i), empty = String_MakeNull();
	Menu_Button(screen, i, &screen->Buttons[i], 300, &empty, &screen->Font, screen->EntryClick,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, (i - 2) * 50);
	/* needed for font list menu */
	screen->UpdateEntry(&screen->Buttons[i], &text);
}

static void ListScreen_Make(struct ListScreen* screen, Int32 i, Int32 x, STRING_PURE String* text, Widget_LeftClick onClick) {
	Menu_Button(screen, i, &screen->Buttons[i], 40, text, &screen->Font, onClick,
		ANCHOR_CENTRE, ANCHOR_CENTRE, x, 0);
}

static void ListScreen_UpdatePage(struct ListScreen* screen) {
	Int32 start = LIST_SCREEN_ITEMS, end = screen->Entries.Count - LIST_SCREEN_ITEMS;
	screen->Buttons[5].Disabled = screen->CurrentIndex < start;
	screen->Buttons[6].Disabled = screen->CurrentIndex >= end;
	if (Game_ClassicMode) return;

	UInt8 pageBuffer[String_BufferSize(STRING_SIZE)];
	String page = String_InitAndClearArray(pageBuffer);
	Int32 num   = (screen->CurrentIndex  / LIST_SCREEN_ITEMS) + 1;
	Int32 pages = Math_CeilDiv(screen->Entries.Count, LIST_SCREEN_ITEMS);
	if (pages == 0) pages = 1;

	String_Format2(&page, "&7Page %i of %i", &num, &pages);
	TextWidget_SetText(&screen->Page, &page);
}

static void ListScreen_UpdateEntry(struct ButtonWidget* button, STRING_PURE String* text) {
	ButtonWidget_SetText(button, text);
}

static void ListScreen_SetCurrentIndex(struct ListScreen* screen, Int32 index) {
	if (index >= screen->Entries.Count) { index = screen->Entries.Count - 1; }
	if (index < 0) index = 0;

	Int32 i;
	for (i = 0; i < LIST_SCREEN_ITEMS; i++) {
		String str = ListScreen_UNSAFE_Get(screen, index + i);
		screen->UpdateEntry(&screen->Buttons[i], &str);
	}

	screen->CurrentIndex = index;
	ListScreen_UpdatePage(screen);
}

static void ListScreen_PageClick(struct ListScreen* screen, bool forward) {
	Int32 delta = forward ? LIST_SCREEN_ITEMS : -LIST_SCREEN_ITEMS;
	ListScreen_SetCurrentIndex(screen, screen->CurrentIndex + delta);
}

static void ListScreen_MoveBackwards(struct GuiElem* elem, struct GuiElem* widget) {
	struct ListScreen* screen = (struct ListScreen*)elem;
	ListScreen_PageClick(screen, false);
}

static void ListScreen_MoveForwards(struct GuiElem* elem, struct GuiElem* widget) {
	struct ListScreen* screen = (struct ListScreen*)elem;
	ListScreen_PageClick(screen, true);
}

static void ListScreen_ContextLost(void* obj) { Menu_Free((struct MenuBase*)obj); }
static void ListScreen_ContextRecreated(void* obj) {
	struct ListScreen* screen = (struct ListScreen*)obj;
	Int32 i;
	for (i = 0; i < LIST_SCREEN_ITEMS; i++) { ListScreen_MakeText(screen, i); }

	String lArrow = String_FromConst("<");
	ListScreen_Make(screen, 5, -220, &lArrow, ListScreen_MoveBackwards);
	String rArrow = String_FromConst(">");
	ListScreen_Make(screen, 6,  220, &rArrow, ListScreen_MoveForwards);

	Menu_DefaultBack(screen, 7, &screen->Buttons[7], false, &screen->Font, Menu_SwitchPause);

	Menu_Label(screen, 8, &screen->Title, &screen->TitleText, &screen->Font,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -155);

	String empty = String_MakeNull();
	Menu_Label(screen, 9, &screen->Page, &empty, &screen->Font,
		ANCHOR_CENTRE, ANCHOR_MAX, 0, 75);

	ListScreen_UpdatePage(screen);
}

static void ListScreen_QuickSort(Int32 left, Int32 right) {
	StringsBuffer* buffer = &ListScreen_Instance.Entries; 
	UInt32* keys = buffer->FlagsBuffer; UInt32 key;
	while (left < right) {
		Int32 i = left, j = right;
		String strI, strJ, pivot = StringsBuffer_UNSAFE_Get(buffer, (i + j) / 2);

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

static String ListScreen_UNSAFE_GetCur(struct ListScreen* screen, struct GuiElem* w) {
	Int32 i = Menu_Index((struct MenuBase*)screen, (struct Widget*)w);
	return ListScreen_UNSAFE_Get(screen, screen->CurrentIndex + i);
}

static void ListScreen_Select(struct ListScreen* screen, STRING_PURE String* str) {
	Int32 i;
	for (i = 0; i < screen->Entries.Count; i++) {
		String entry = StringsBuffer_UNSAFE_Get(&screen->Entries, i);
		if (!String_CaselessEquals(&entry, str)) continue;

		ListScreen_SetCurrentIndex(screen, i);
		return;
	}
}

static void ListScreen_Init(struct GuiElem* elem) {
	struct ListScreen* screen = (struct ListScreen*)elem;
	screen->Widgets = screen->ListWidgets;
	screen->WidgetsCount = Array_Elems(screen->ListWidgets);

	Font_Make(&screen->Font, &Game_FontName, 16, FONT_STYLE_BOLD);
	Key_KeyRepeat = true;
	screen->WheelAcc = 0.0f;
	ListScreen_ContextRecreated(screen);
	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, ListScreen_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, ListScreen_ContextRecreated);
}

static void ListScreen_Render(struct GuiElem* elem, Real64 delta) {
	Menu_RenderBounds();
	Gfx_SetTexturing(true);
	Menu_Render((struct MenuBase*)elem, delta);
	Gfx_SetTexturing(false);
}

static void ListScreen_Free(struct GuiElem* elem) {
	struct ListScreen* screen = (struct ListScreen*)elem;
	Font_Free(&screen->Font);
	Key_KeyRepeat = false;
	ListScreen_ContextLost(screen);
	Event_UnregisterVoid(&GfxEvents_ContextLost,      screen, ListScreen_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, screen, ListScreen_ContextRecreated);
}

static bool ListScreen_HandlesKeyDown(struct GuiElem* elem, Key key) {
	struct ListScreen* screen = (struct ListScreen*)elem;
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

static bool ListScreen_HandlesMouseMove(struct GuiElem* elem, Int32 x, Int32 y) {
	return Menu_HandleMouseMove((struct MenuBase*)elem, x, y) >= 0;
}

static bool ListScreen_HandlesMouseDown(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn) {
	return Menu_HandleMouseDown((struct MenuBase*)elem, x, y, btn) >= 0;
}

static bool ListScreen_HandlesMouseScroll(struct GuiElem* elem, Real32 delta) {
	struct ListScreen* screen = (struct ListScreen*)elem;
	Int32 steps = Utils_AccumulateWheelDelta(&screen->WheelAcc, delta);
	if (steps) ListScreen_SetCurrentIndex(screen, screen->CurrentIndex - steps);
	return true;
}

static void ListScreen_OnResize(struct GuiElem* elem) {
	Menu_Reposition((struct MenuBase*)elem);
}

struct ListScreen* ListScreen_MakeInstance(void) {
	struct ListScreen* screen = &ListScreen_Instance;
	Mem_Set(screen, 0, sizeof(struct ListScreen) - sizeof(StringsBuffer));
	if (screen->Entries.TextBuffer) StringsBuffer_Free(&screen->Entries);
	StringsBuffer_Init(&screen->Entries);

	screen->VTABLE = &ListScreen_VTABLE;
	Screen_Reset((struct Screen*)screen);
	screen->UpdateEntry = ListScreen_UpdateEntry;
	
	screen->VTABLE->HandlesKeyDown   = ListScreen_HandlesKeyDown;
	screen->VTABLE->HandlesMouseDown = ListScreen_HandlesMouseDown;
	screen->VTABLE->HandlesMouseMove = ListScreen_HandlesMouseMove;
	screen->VTABLE->HandlesMouseScroll = ListScreen_HandlesMouseScroll;

	screen->OnResize       = ListScreen_OnResize;
	screen->VTABLE->Init   = ListScreen_Init;
	screen->VTABLE->Render = ListScreen_Render;
	screen->VTABLE->Free   = ListScreen_Free;
	return screen;
}


/*########################################################################################################################*
*--------------------------------------------------------MenuScreen-------------------------------------------------------*
*#########################################################################################################################*/
struct GuiElementVTABLE MenuScreen_VTABLE;

static void MenuScreen_Remove(struct MenuScreen* screen, Int32 i) {
	struct Widget** widgets = screen->Widgets;
	if (widgets[i]) { Elem_TryFree(widgets[i]); }
	widgets[i] = NULL;
}

static bool MenuScreen_HandlesMouseDown(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn) {
	return Menu_HandleMouseDown((struct MenuBase*)elem, x, y, btn) >= 0;
}
static bool MenuScreen_HandlesMouseMove(struct GuiElem* elem, Int32 x, Int32 y) {
	return Menu_HandleMouseMove((struct MenuBase*)elem, x, y) >= 0;
}

static bool MenuScreen_HandlesMouseUp(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn) { return true; }
static bool MenuScreen_HandlesMouseScroll(struct GuiElem* elem, Real32 delta) { return true; }

static bool MenuScreen_HandlesKeyDown(struct GuiElem* elem, Key key) {
	if (key == Key_Escape) { Gui_ReplaceActive(NULL); }
	return key < Key_F1 || key > Key_F35;
}
static bool MenuScreen_HandlesKeyPress(struct GuiElem* elem, UChar key) { return true; }
static bool MenuScreen_HandlesKeyUp(struct GuiElem* elem, Key key) { return true; }

static void MenuScreen_ContextLost(void* obj) { Menu_Free((struct MenuBase*)obj); }

static void MenuScreen_ContextLost_Callback(void* obj) {
	((struct MenuScreen*)obj)->ContextLost(obj);
}

static void MenuScreen_ContextRecreated_Callback(void* obj) {
	((struct MenuScreen*)obj)->ContextRecreated(obj);
}

static void MenuScreen_Init(struct GuiElem* elem) {
	struct MenuScreen* screen = (struct MenuScreen*)elem;

	if (!screen->TitleFont.Handle) {
		Font_Make(&screen->TitleFont, &Game_FontName, 16, FONT_STYLE_BOLD);
	}
	if (!screen->TextFont.Handle) {
		Font_Make(&screen->TextFont, &Game_FontName, 16, FONT_STYLE_NORMAL);
	}

	Event_RegisterVoid(&GfxEvents_ContextLost,      screen, MenuScreen_ContextLost_Callback);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, screen, MenuScreen_ContextRecreated_Callback);
}

static void MenuScreen_Render(struct GuiElem* elem, Real64 delta) {
	Menu_RenderBounds();
	Gfx_SetTexturing(true);
	Menu_Render((struct MenuBase*)elem, delta);
	Gfx_SetTexturing(false);
}

static void MenuScreen_Free(struct GuiElem* elem) {
	struct MenuScreen* screen = (struct MenuScreen*)elem;
	screen->ContextLost(screen);

	if (screen->TitleFont.Handle) {
		Font_Free(&screen->TitleFont);
	}
	if (screen->TextFont.Handle) {
		Font_Free(&screen->TextFont);
	}

	Event_UnregisterVoid(&GfxEvents_ContextLost,      screen, MenuScreen_ContextLost_Callback);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, screen, MenuScreen_ContextRecreated_Callback);
}

static void MenuScreen_OnResize(struct GuiElem* elem) { Menu_Reposition((struct MenuBase*)elem); }

static void MenuScreen_MakeInstance(struct MenuScreen* screen, struct Widget** widgets, Int32 count, Menu_ContextFunc contextRecreated) {
	Mem_Set(screen, 0, sizeof(struct MenuScreen));
	screen->VTABLE = &MenuScreen_VTABLE;
	Screen_Reset((struct Screen*)screen);

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

	screen->Widgets          = widgets;
	screen->WidgetsCount     = count;
	screen->ContextLost      = MenuScreen_ContextLost;
	screen->ContextRecreated = contextRecreated;
}


/*########################################################################################################################*
*-------------------------------------------------------PauseScreen-------------------------------------------------------*
*#########################################################################################################################*/
struct GuiElementVTABLE PauseScreen_VTABLE;
struct PauseScreen PauseScreen_Instance;
static void PauseScreen_Make(struct PauseScreen* screen, Int32 i, Int32 dir, Int32 y, const UChar* title, Widget_LeftClick onClick) {	
	String text = String_FromReadonly(title);
	Menu_Button(screen, i, &screen->Buttons[i], 300, &text, &screen->TitleFont, onClick,
		ANCHOR_CENTRE, ANCHOR_CENTRE, dir * 160, y);
}

static void PauseScreen_MakeClassic(struct PauseScreen* screen, Int32 i, Int32 y, const UChar* title, Widget_LeftClick onClick) {	
	String text = String_FromReadonly(title);
	Menu_Button(screen, i, &screen->Buttons[i], 400, &text, &screen->TitleFont, onClick,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, y);
}

static void PauseScreen_Quit(struct GuiElem* a, struct GuiElem* b) { Window_Close(); }
static void PauseScreen_Game(struct GuiElem* a, struct GuiElem* b) { Gui_ReplaceActive(NULL); }

static void PauseScreen_CheckHacksAllowed(void* obj) {
	if (Game_UseClassicOptions) return;
	struct PauseScreen* screen = (struct PauseScreen*)obj;
	screen->Buttons[4].Disabled = !LocalPlayer_Instance.Hacks.CanAnyHacks; /* select texture pack */
}

static void PauseScreen_ContextRecreated(void* obj) {
	struct PauseScreen* screen = (struct PauseScreen*)obj;
	struct FontDesc* font = &screen->TitleFont;

	if (Game_UseClassicOptions) {
		PauseScreen_MakeClassic(screen, 0, -100, "Options...",            Menu_SwitchClassicOptions);
		PauseScreen_MakeClassic(screen, 1,  -50, "Generate new level...", Menu_SwitchClassicGenLevel);
		PauseScreen_MakeClassic(screen, 2,    0, "Load level...",         Menu_SwitchLoadLevel);
		PauseScreen_MakeClassic(screen, 3,   50, "Save level...",         Menu_SwitchSaveLevel);
		PauseScreen_MakeClassic(screen, 4,  150, "Nostalgia options...",  Menu_SwitchNostalgia);

		String back = String_FromConst("Back to game");
		Menu_Back(screen, 5, &screen->Buttons[5], 400, &back, 25, font, PauseScreen_Game);	

		/* Disable nostalgia options in classic mode*/
		if (Game_ClassicMode) MenuScreen_Remove((struct MenuScreen*)screen, 4);
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
		Menu_Button(screen, 6, &screen->Buttons[6], 120, &quitMsg, font, PauseScreen_Quit,
			ANCHOR_MAX, ANCHOR_MAX, 5, 5);

		Menu_DefaultBack(screen, 7, &screen->Buttons[7], true, font, PauseScreen_Game);
	}

	if (!ServerConnection_IsSinglePlayer) {
		screen->Buttons[1].Disabled = true;
		screen->Buttons[2].Disabled = true;
	}
	PauseScreen_CheckHacksAllowed(obj);
}

static void PauseScreen_Init(struct GuiElem* elem) {
	struct PauseScreen* screen = (struct PauseScreen*)elem;
	MenuScreen_Init(elem);
	Event_RegisterVoid(&UserEvents_HackPermissionsChanged, screen, PauseScreen_CheckHacksAllowed);
	screen->ContextRecreated(elem);
}

static void PauseScreen_Free(struct GuiElem* elem) {
	struct PauseScreen* screen = (struct PauseScreen*)elem;
	MenuScreen_Free(elem);
	Event_UnregisterVoid(&UserEvents_HackPermissionsChanged, screen, PauseScreen_CheckHacksAllowed);
}

struct Screen* PauseScreen_MakeInstance(void) {
	static struct Widget* widgets[8];
	struct PauseScreen* screen = &PauseScreen_Instance;
	MenuScreen_MakeInstance((struct MenuScreen*)screen, widgets, 
		Array_Elems(widgets), PauseScreen_ContextRecreated);
	PauseScreen_VTABLE = *screen->VTABLE;
	screen->VTABLE = &PauseScreen_VTABLE;

	screen->VTABLE->Init           = PauseScreen_Init;
	screen->VTABLE->Free           = PauseScreen_Free;
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*--------------------------------------------------OptionsGroupScreen-----------------------------------------------------*
*#########################################################################################################################*/
struct GuiElementVTABLE OptionsGroupScreen_VTABLE;
struct OptionsGroupScreen OptionsGroupScreen_Instance;
const UChar* optsGroup_descs[7] = {
	"&eMusic/Sound, view bobbing, and more",
	"&eChat options, gui scale, font settings, and more",
	"&eFPS limit, view distance, entity names/shadows",
	"&eSet key bindings, bind keys to act as mouse clicks",
	"&eHacks allowed, jump settings, and more",
	"&eEnv colours, water level, weather, and more",
	"&eSettings for resembling the original classic",
};

static void OptionsGroupScreen_CheckHacksAllowed(void* obj) {
	struct OptionsGroupScreen* screen = (struct OptionsGroupScreen*)obj;
	screen->Buttons[5].Disabled = !LocalPlayer_Instance.Hacks.CanAnyHacks; /* env settings */
}

static void OptionsGroupScreen_Make(struct OptionsGroupScreen* screen, Int32 i, Int32 dir, Int32 y, const UChar* title, Widget_LeftClick onClick) {	
	String text = String_FromReadonly(title);
	Menu_Button(screen, i, &screen->Buttons[i], 300, &text, &screen->TitleFont, onClick,
		ANCHOR_CENTRE, ANCHOR_CENTRE, dir * 160, y);
}

static void OptionsGroupScreen_MakeDesc(struct OptionsGroupScreen* screen) {
	String text = String_FromReadonly(optsGroup_descs[screen->SelectedI]);
	Menu_Label(screen, 8, &screen->Desc, &text, &screen->TextFont,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 100);
}

static void OptionsGroupScreen_ContextRecreated(void* obj) {
	struct OptionsGroupScreen* screen = (struct OptionsGroupScreen*)obj;

	OptionsGroupScreen_Make(screen, 0, -1, -100, "Misc options...",      Menu_SwitchMisc);
	OptionsGroupScreen_Make(screen, 1, -1,  -50, "Gui options...",       Menu_SwitchGui);
	OptionsGroupScreen_Make(screen, 2, -1,    0, "Graphics options...",  Menu_SwitchGfx);
	OptionsGroupScreen_Make(screen, 3, -1,   50, "Controls...",          Menu_SwitchKeysNormal);
	OptionsGroupScreen_Make(screen, 4,  1,  -50, "Hacks settings...",    Menu_SwitchHacks);
	OptionsGroupScreen_Make(screen, 5,  1,    0, "Env settings...",      Menu_SwitchEnv);
	OptionsGroupScreen_Make(screen, 6,  1,   50, "Nostalgia options...", Menu_SwitchNostalgia);

	Menu_DefaultBack(screen, 7, &screen->Buttons[7], false, &screen->TitleFont, Menu_SwitchPause);	
	screen->Widgets[8] = NULL; /* Description text widget placeholder */

	if (screen->SelectedI >= 0) { OptionsGroupScreen_MakeDesc(screen); }
	OptionsGroupScreen_CheckHacksAllowed(obj);
}

static void OptionsGroupScreen_Init(struct GuiElem* elem) {
	struct OptionsGroupScreen* screen = (struct OptionsGroupScreen*)elem;
	MenuScreen_Init(elem);
	Event_RegisterVoid(&UserEvents_HackPermissionsChanged, screen, OptionsGroupScreen_CheckHacksAllowed);
	screen->ContextRecreated(elem);
}

static void OptionsGroupScreen_Free(struct GuiElem* elem) {
	struct OptionsGroupScreen* screen = (struct OptionsGroupScreen*)elem;
	MenuScreen_Free(elem);
	Event_UnregisterVoid(&UserEvents_HackPermissionsChanged, screen, OptionsGroupScreen_CheckHacksAllowed);
}

static bool OptionsGroupScreen_HandlesMouseMove(struct GuiElem* elem, Int32 x, Int32 y) {
	struct OptionsGroupScreen* screen = (struct OptionsGroupScreen*)elem;
	Int32 i = Menu_HandleMouseMove((struct MenuBase*)elem, x, y);
	if (i == -1 || i == screen->SelectedI) return true;
	if (i >= Array_Elems(optsGroup_descs)) return true;

	screen->SelectedI = i;
	Elem_TryFree(&screen->Desc);
	OptionsGroupScreen_MakeDesc(screen);
	return true;
}

struct Screen* OptionsGroupScreen_MakeInstance(void) {
	static struct Widget* widgets[9];
	struct OptionsGroupScreen* screen = &OptionsGroupScreen_Instance;
	MenuScreen_MakeInstance((struct MenuScreen*)screen, widgets, 
		Array_Elems(widgets), OptionsGroupScreen_ContextRecreated);
	OptionsGroupScreen_VTABLE = *screen->VTABLE;
	screen->VTABLE = &OptionsGroupScreen_VTABLE;

	screen->VTABLE->Init = OptionsGroupScreen_Init;
	screen->VTABLE->Free = OptionsGroupScreen_Free;
	screen->VTABLE->HandlesMouseMove = OptionsGroupScreen_HandlesMouseMove;

	screen->SelectedI = -1;
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*----------------------------------------------------EditHotkeyScreen-----------------------------------------------------*
*#########################################################################################################################*/
struct GuiElementVTABLE EditHotkeyScreen_VTABLE;
struct EditHotkeyScreen EditHotkeyScreen_Instance;
static void EditHotkeyScreen_Make(struct EditHotkeyScreen* screen, Int32 i, Int32 x, Int32 y, STRING_PURE String* text, Widget_LeftClick onClick) {
	Menu_Button(screen, i, &screen->Buttons[i], 300, text, &screen->TitleFont, onClick,
		ANCHOR_CENTRE, ANCHOR_CENTRE, x, y);
}

static void HotkeyListScreen_MakeFlags(UInt8 flags, STRING_TRANSIENT String* str);
static void EditHotkeyScreen_MakeFlags(UInt8 flags, STRING_TRANSIENT String* str) {
	if (flags == 0) String_AppendConst(str, " None");
	HotkeyListScreen_MakeFlags(flags, str);
}

static void EditHotkeyScreen_MakeBaseKey(struct EditHotkeyScreen* screen, Widget_LeftClick onClick) {
	UChar textBuffer[String_BufferSize(STRING_SIZE)];
	String text = String_InitAndClearArray(textBuffer);

	String_AppendConst(&text, "Key: ");
	String_AppendConst(&text, Key_Names[screen->CurHotkey.Trigger]);
	EditHotkeyScreen_Make(screen, 0, 0, -150, &text, onClick);
}

static void EditHotkeyScreen_MakeModifiers(struct EditHotkeyScreen* screen, Widget_LeftClick onClick) {
	UChar textBuffer[String_BufferSize(STRING_SIZE)];
	String text = String_InitAndClearArray(textBuffer);

	String_AppendConst(&text, "Modifiers:");
	EditHotkeyScreen_MakeFlags(screen->CurHotkey.Flags, &text);
	EditHotkeyScreen_Make(screen, 1, 0, -100, &text, onClick);
}

static void EditHotkeyScreen_MakeLeaveOpen(struct EditHotkeyScreen* screen, Widget_LeftClick onClick) {
	UChar textBuffer[String_BufferSize(STRING_SIZE)];
	String text = String_InitAndClearArray(textBuffer);

	String_AppendConst(&text, "Input stays open: ");
	String_AppendConst(&text, screen->CurHotkey.StaysOpen ? "ON" : "OFF");
	EditHotkeyScreen_Make(screen, 2, -100, 10, &text, onClick);
}

static void EditHotkeyScreen_BaseKey(struct GuiElem* elem, struct GuiElem* widget) {
	struct EditHotkeyScreen* screen = (struct EditHotkeyScreen*)elem;
	screen->SelectedI = 0;
	screen->SupressNextPress = true;
	String msg = String_FromConst("Key: press a key..");
	ButtonWidget_SetText(&screen->Buttons[0], &msg);
}

static void EditHotkeyScreen_Modifiers(struct GuiElem* elem, struct GuiElem* widget) {
	struct EditHotkeyScreen* screen = (struct EditHotkeyScreen*)elem;
	screen->SelectedI = 1;
	screen->SupressNextPress = true;
	String msg = String_FromConst("Modifiers: press a key..");
	ButtonWidget_SetText(&screen->Buttons[1], &msg);
}

static void EditHotkeyScreen_LeaveOpen(struct GuiElem* elem, struct GuiElem* widget) {
	struct EditHotkeyScreen* screen = (struct EditHotkeyScreen*)elem;
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

static void EditHotkeyScreen_SaveChanges(struct GuiElem* elem, struct GuiElem* widget) {
	struct EditHotkeyScreen* screen = (struct EditHotkeyScreen*)elem;
	struct HotkeyData hotkey = screen->OrigHotkey;
	if (hotkey.Trigger != Key_None) {
		Hotkeys_Remove(hotkey.Trigger, hotkey.Flags);
		Hotkeys_UserRemovedHotkey(hotkey.Trigger, hotkey.Flags);
	}

	hotkey = screen->CurHotkey;
	if (hotkey.Trigger != Key_None) {
		String text = screen->Input.Base.Text;
		Hotkeys_Add(hotkey.Trigger, hotkey.Flags, &text, hotkey.StaysOpen);
		Hotkeys_UserAddedHotkey(hotkey.Trigger, hotkey.Flags, hotkey.StaysOpen, &text);
	}
	Gui_ReplaceActive(HotkeyListScreen_MakeInstance());
}

static void EditHotkeyScreen_RemoveHotkey(struct GuiElem* elem, struct GuiElem* widget) {
	struct EditHotkeyScreen* screen = (struct EditHotkeyScreen*)elem;
	struct HotkeyData hotkey = screen->OrigHotkey;
	if (hotkey.Trigger != Key_None) {
		Hotkeys_Remove(hotkey.Trigger, hotkey.Flags);
		Hotkeys_UserRemovedHotkey(hotkey.Trigger, hotkey.Flags);
	}
	Gui_ReplaceActive(HotkeyListScreen_MakeInstance());
}

static void EditHotkeyScreen_Init(struct GuiElem* elem) {
	struct EditHotkeyScreen* screen = (struct EditHotkeyScreen*)elem;
	MenuScreen_Init(elem);
	Key_KeyRepeat = true;
	screen->ContextRecreated(elem);
}

static void EditHotkeyScreen_Render(struct GuiElem* elem, Real64 delta) {
	MenuScreen_Render(elem, delta);
	Int32 cX = Game_Width / 2, cY = Game_Height / 2;
	PackedCol grey = PACKEDCOL_CONST(150, 150, 150, 255);
	GfxCommon_Draw2DFlat(cX - 250, cY - 65, 500, 2, grey);
	GfxCommon_Draw2DFlat(cX - 250, cY + 45, 500, 2, grey);
}

static void EditHotkeyScreen_Free(struct GuiElem* elem) {
	struct EditHotkeyScreen* screen = (struct EditHotkeyScreen*)elem;
	Key_KeyRepeat = false;
	screen->SelectedI = -1;
	MenuScreen_Free(elem);
}

static bool EditHotkeyScreen_HandlesKeyPress(struct GuiElem* elem, UChar key) {
	struct EditHotkeyScreen* screen = (struct EditHotkeyScreen*)elem;
	if (screen->SupressNextPress) {
		screen->SupressNextPress = false;
		return true;
	}
	return Elem_HandlesKeyPress(&screen->Input.Base, key);
}

static bool EditHotkeyScreen_HandlesKeyDown(struct GuiElem* elem, Key key) {
	struct EditHotkeyScreen* screen = (struct EditHotkeyScreen*)elem;
	if (screen->SelectedI >= 0) {
		if (screen->SelectedI == 0) {
			screen->CurHotkey.Trigger = key;
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

static bool EditHotkeyScreen_HandlesKeyUp(struct GuiElem* elem, Key key) {
	struct EditHotkeyScreen* screen = (struct EditHotkeyScreen*)elem;
	return Elem_HandlesKeyUp(&screen->Input.Base, key);
}

static void EditHotkeyScreen_ContextRecreated(void* obj) {
	struct EditHotkeyScreen* screen = (struct EditHotkeyScreen*)obj;
	struct MenuInputValidator validator = MenuInputValidator_String();
	String text = String_MakeNull();

	bool existed = screen->OrigHotkey.Trigger != Key_None;
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

	Menu_DefaultBack(screen, 5, &screen->Buttons[5], false, &screen->TitleFont, Menu_SwitchHotkeys);

	Menu_Input(screen, 6, &screen->Input, 500, &text, &screen->TextFont, &validator,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -35);
}

struct Screen* EditHotkeyScreen_MakeInstance(struct HotkeyData original) {
	static struct Widget* widgets[7];
	struct EditHotkeyScreen* screen = &EditHotkeyScreen_Instance;
	MenuScreen_MakeInstance((struct MenuScreen*)screen, widgets, 
		Array_Elems(widgets), EditHotkeyScreen_ContextRecreated);
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
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*-----------------------------------------------------GenLevelScreen------------------------------------------------------*
*#########################################################################################################################*/
struct GuiElementVTABLE GenLevelScreen_VTABLE;
struct GenLevelScreen GenLevelScreen_Instance;
Int32 GenLevelScreen_GetInt(struct GenLevelScreen* screen, Int32 index) {
	struct MenuInputWidget* input = &screen->Inputs[index];
	String text = input->Base.Text;

	if (!input->Validator.IsValidValue(&input->Validator, &text)) return 0;
	Int32 value; Convert_TryParseInt32(&text, &value); return value;
}

Int32 GenLevelScreen_GetSeedInt(struct GenLevelScreen* screen, Int32 index) {
	struct MenuInputWidget* input = &screen->Inputs[index];
	String text = input->Base.Text;

	if (!text.length) {
		Random rnd; Random_InitFromCurrentTime(&rnd);
		return Random_Next(&rnd, Int32_MaxValue);
	}

	if (!input->Validator.IsValidValue(&input->Validator, &text)) return 0;
	Int32 value; Convert_TryParseInt32(&text, &value); return value;
}

static void GenLevelScreen_Gen(struct GenLevelScreen* screen, bool vanilla) {
	Int32 width  = GenLevelScreen_GetInt(screen, 0);
	Int32 height = GenLevelScreen_GetInt(screen, 1);
	Int32 length = GenLevelScreen_GetInt(screen, 2);
	Int32 seed   = GenLevelScreen_GetSeedInt(screen, 3);

	Int64 volume = (Int64)width * height * length;
	if (volume > Int32_MaxValue) {
		String msg = String_FromConst("&cThe generated map's volume is too big.");
		Chat_Add(&msg);
	} else if (!width || !height || !length) {
		String msg = String_FromConst("&cOne of the map dimensions is invalid.");
		Chat_Add(&msg);
	} else {
		Gen_SetDimensions(width, height, length); 
		Gen_Vanilla = vanilla; Gen_Seed = seed;
		Gui_ReplaceActive(GeneratingScreen_MakeInstance());
	}
}

static void GenLevelScreen_Flatgrass(struct GuiElem* elem, struct GuiElem* widget) {
	struct GenLevelScreen* screen = (struct GenLevelScreen*)elem;
	GenLevelScreen_Gen(screen, false);
}

static void GenLevelScreen_Notchy(struct GuiElem* elem, struct GuiElem* widget) {
	struct GenLevelScreen* screen = (struct GenLevelScreen*)elem;
	GenLevelScreen_Gen(screen, true);
}

static void GenLevelScreen_InputClick(struct GuiElem* elem, struct GuiElem* widget) {
	struct GenLevelScreen* screen = (struct GenLevelScreen*)elem;
	if (screen->Selected) screen->Selected->Base.ShowCaret = false;

	screen->Selected = (struct MenuInputWidget*)widget;
	Elem_HandlesMouseDown(&screen->Selected->Base, Mouse_X, Mouse_Y, MouseButton_Left);
	screen->Selected->Base.ShowCaret = true;
}

static void GenLevelScreen_Input(struct GenLevelScreen* screen, Int32 i, Int32 y, bool seed, STRING_TRANSIENT String* value) {
	struct MenuInputWidget* input = &screen->Inputs[i];
	struct MenuInputValidator validator = seed ? MenuInputValidator_Seed() : MenuInputValidator_Integer(1, 8192);

	Menu_Input(screen, i, input, 200, value, &screen->TextFont, &validator,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, y);

	input->Base.ShowCaret = false;
	input->Base.MenuClick = GenLevelScreen_InputClick;
	String_Clear(value);
}

static void GenLevelScreen_Label(struct GenLevelScreen* screen, Int32 i, Int32 x, Int32 y, const UChar* title) {	
	struct TextWidget* label = &screen->Labels[i];

	String text = String_FromReadonly(title);
	Menu_Label(screen, i + 4, label, &text, &screen->TextFont,
		ANCHOR_CENTRE, ANCHOR_CENTRE, x, y);

	label->XOffset = -110 - label->Width / 2;
	Widget_Reposition(label);
	PackedCol col = PACKEDCOL_CONST(224, 224, 224, 255); label->Col = col;
}

static void GenLevelScreen_Init(struct GuiElem* elem) {
	struct GenLevelScreen* screen = (struct GenLevelScreen*)elem;
	MenuScreen_Init(elem);
	Key_KeyRepeat = true;
	screen->ContextRecreated(elem);
}

static void GenLevelScreen_Free(struct GuiElem* elem) {
	Key_KeyRepeat = false;
	MenuScreen_Free(elem);
}

static bool GenLevelScreen_HandlesKeyDown(struct GuiElem* elem, Key key) {
	struct GenLevelScreen* screen = (struct GenLevelScreen*)elem;
	if (screen->Selected && Elem_HandlesKeyDown(&screen->Selected->Base, key)) return true;
	return MenuScreen_HandlesKeyDown(elem, key);
}

static bool GenLevelScreen_HandlesKeyUp(struct GuiElem* elem, Key key) {
	struct GenLevelScreen* screen = (struct GenLevelScreen*)elem;
	return !screen->Selected || Elem_HandlesKeyUp(&screen->Selected->Base, key);
}

static bool GenLevelScreen_HandlesKeyPress(struct GuiElem* elem, UChar key) {
	struct GenLevelScreen* screen = (struct GenLevelScreen*)elem;
	return !screen->Selected || Elem_HandlesKeyPress(&screen->Selected->Base, key);
}

static void GenLevelScreen_ContextRecreated(void* obj) {
	struct GenLevelScreen* screen = (struct GenLevelScreen*)obj;
	UChar tmpBuffer[String_BufferSize(STRING_SIZE)];
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
	Menu_Label(screen, 8, &screen->Labels[4], &gen, &screen->TextFont,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -130);

	String flatgrass = String_FromConst("Flatgrass");
	Menu_Button(screen, 9, &screen->Buttons[0], 200, &flatgrass, &screen->TitleFont, GenLevelScreen_Flatgrass,
		ANCHOR_CENTRE, ANCHOR_CENTRE, -120, 100);

	String vanilla = String_FromConst("Vanilla");
	Menu_Button(screen, 10, &screen->Buttons[1], 200, &vanilla, &screen->TitleFont, GenLevelScreen_Notchy,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 120, 100);

	Menu_DefaultBack(screen, 11, &screen->Buttons[2], false, &screen->TitleFont, Menu_SwitchPause);
}

struct Screen* GenLevelScreen_MakeInstance(void) {
	static struct Widget* widgets[12];
	struct GenLevelScreen* screen = &GenLevelScreen_Instance;
	MenuScreen_MakeInstance((struct MenuScreen*)screen, widgets,
		Array_Elems(widgets), GenLevelScreen_ContextRecreated);
	GenLevelScreen_VTABLE = *screen->VTABLE;
	screen->VTABLE = &GenLevelScreen_VTABLE;

	screen->VTABLE->Init = GenLevelScreen_Init;
	screen->VTABLE->Free = GenLevelScreen_Free;
	screen->VTABLE->HandlesKeyDown  = GenLevelScreen_HandlesKeyDown;
	screen->VTABLE->HandlesKeyUp    = GenLevelScreen_HandlesKeyUp;
	screen->VTABLE->HandlesKeyPress = GenLevelScreen_HandlesKeyPress;
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*----------------------------------------------------ClassicGenScreen-----------------------------------------------------*
*#########################################################################################################################*/
struct GuiElementVTABLE ClassicGenScreen_VTABLE;
struct ClassicGenScreen ClassicGenScreen_Instance;
static void ClassicGenScreen_Gen(Int32 size) {
	Random rnd; Random_InitFromCurrentTime(&rnd);
	Gen_SetDimensions(size, 64, size); Gen_Vanilla = true;
	Gen_Seed = Random_Next(&rnd, Int32_MaxValue);
	Gui_ReplaceActive(GeneratingScreen_MakeInstance());
}

static void ClassicGenScreen_Small(struct GuiElem* a, struct GuiElem* b)  { ClassicGenScreen_Gen(128); }
static void ClassicGenScreen_Medium(struct GuiElem* a, struct GuiElem* b) { ClassicGenScreen_Gen(256); }
static void ClassicGenScreen_Huge(struct GuiElem* a, struct GuiElem* b)   { ClassicGenScreen_Gen(512); }

static void ClassicGenScreen_Make(struct ClassicGenScreen* screen, Int32 i, Int32 y, const UChar* title, Widget_LeftClick onClick) {
	String text = String_FromReadonly(title);
	Menu_Button(screen, i, &screen->Buttons[i], 400, &text, &screen->TitleFont, onClick,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, y);
}

static void ClassicGenScreen_Init(struct GuiElem* elem) {
	struct ClassicGenScreen* screen = (struct ClassicGenScreen*)elem;
	MenuScreen_Init(elem);
	screen->ContextRecreated(elem);
}

static void ClassicGenScreen_ContextRecreated(void* obj) {
	struct ClassicGenScreen* screen = (struct ClassicGenScreen*)obj;
	ClassicGenScreen_Make(screen, 0, -100, "Small",  ClassicGenScreen_Small);
	ClassicGenScreen_Make(screen, 1,  -50, "Normal", ClassicGenScreen_Medium);
	ClassicGenScreen_Make(screen, 2,    0, "Huge",   ClassicGenScreen_Huge);

	Menu_DefaultBack(screen, 3, &screen->Buttons[3], false, &screen->TitleFont, Menu_SwitchPause);
}

struct Screen* ClassicGenScreen_MakeInstance(void) {
	static struct Widget* widgets[4];
	struct ClassicGenScreen* screen = &ClassicGenScreen_Instance;
	MenuScreen_MakeInstance((struct MenuScreen*)screen, widgets, 
		Array_Elems(widgets), ClassicGenScreen_ContextRecreated);
	ClassicGenScreen_VTABLE = *screen->VTABLE;
	screen->VTABLE = &ClassicGenScreen_VTABLE;

	screen->VTABLE->Init = ClassicGenScreen_Init;
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*----------------------------------------------------SaveLevelScreen------------------------------------------------------*
*#########################################################################################################################*/
struct SaveLevelScreen SaveLevelScreen_Instance;
struct GuiElementVTABLE SaveLevelScreen_VTABLE;
static void SaveLevelScreen_RemoveOverwrites(struct SaveLevelScreen* screen) {
	struct ButtonWidget* btn = &screen->Buttons[0];
	if (btn->OptName) {
		btn->OptName = NULL; String save = String_FromConst("Save"); 
		ButtonWidget_SetText(btn, &save);
	}

	btn = &screen->Buttons[1];
	if (btn->OptName) {
		btn->OptName = NULL; String save = String_FromConst("Save schematic");
		ButtonWidget_SetText(btn, &save);
	}
}

static void SaveLevelScreen_MakeDesc(struct SaveLevelScreen* screen, STRING_PURE String* text) {
	if (screen->Widgets[5]) { Elem_TryFree(screen->Widgets[5]); }

	Menu_Label(screen, 5, &screen->Desc, text, &screen->TextFont,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 65);
}

static void SaveLevelScreen_DoSave(struct GuiElem* elem, struct GuiElem* widget, const UChar* ext) {
	struct SaveLevelScreen* screen = (struct SaveLevelScreen*)elem;
	String file = screen->Input.Base.Text;
	if (!file.length) {
		String msg = String_FromConst("&ePlease enter a filename")
		SaveLevelScreen_MakeDesc(screen, &msg); return;
	}

	UChar pathBuffer[String_BufferSize(FILENAME_SIZE)];
	String path = String_InitAndClearArray(pathBuffer);
	String_Format2(&path, "maps/%s%c", &file, ext);

	struct ButtonWidget* btn = (struct ButtonWidget*)widget;
	if (File_Exists(&path) && !btn->OptName) {
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

static void SaveLevelScreen_Classic(struct GuiElem* elem, struct GuiElem* widget) {
	SaveLevelScreen_DoSave(elem, widget, ".cw");
}

static void SaveLevelScreen_Schematic(struct GuiElem* elem, struct GuiElem* widget) {
	SaveLevelScreen_DoSave(elem, widget, ".schematic");
}

static void SaveLevelScreen_Init(struct GuiElem* elem) {
	struct SaveLevelScreen* screen = (struct SaveLevelScreen*)elem;
	screen->TextPath = String_InitAndClearArray(screen->TextPathBuffer);
	MenuScreen_Init(elem);
	Key_KeyRepeat = true;
	screen->ContextRecreated(elem);
}

static void SaveLevelScreen_Render(struct GuiElem* elem, Real64 delta) {
	MenuScreen_Render(elem, delta);
	Int32 cX = Game_Width / 2, cY = Game_Height / 2;
	PackedCol grey = PACKEDCOL_CONST(150, 150, 150, 255);
	GfxCommon_Draw2DFlat(cX - 250, cY + 90, 500, 2, grey);

	struct SaveLevelScreen* screen = (struct SaveLevelScreen*)elem;
	if (!screen->TextPath.length) return;
	String path = screen->TextPath;

	void* file; ReturnCode result = File_Create(&file, &path);
	ErrorHandler_CheckOrFail(result, "Saving map - opening file");
	struct Stream stream; Stream_FromFile(&stream, file, &path);
	{
		String cw = String_FromConst(".cw");
		if (String_CaselessEnds(&path, &cw)) {
			Cw_Save(&stream);
		} else {
			Schematic_Save(&stream);
		}
	}
	result = stream.Close(&stream);
	ErrorHandler_CheckOrFail(result, "Saving map - closing file");

	UChar msgBuffer[String_BufferSize(STRING_SIZE * 2)];
	String msg = String_InitAndClearArray(msgBuffer);
	String_Format1(&msg, "&eSaved map to: %s", &path);
	Chat_Add(&msg);

	Gui_ReplaceActive(PauseScreen_MakeInstance());
	String_Clear(&path);
}

static void SaveLevelScreen_Free(struct GuiElem* elem) {
	Key_KeyRepeat = false;
	MenuScreen_Free(elem);
}

static bool SaveLevelScreen_HandlesKeyPress(struct GuiElem* elem, UChar key) {
	struct SaveLevelScreen* screen = (struct SaveLevelScreen*)elem;
	SaveLevelScreen_RemoveOverwrites(screen);

	return Elem_HandlesKeyPress(&screen->Input.Base, key);
}

static bool SaveLevelScreen_HandlesKeyDown(struct GuiElem* elem, Key key) {
	struct SaveLevelScreen* screen = (struct SaveLevelScreen*)elem;
	SaveLevelScreen_RemoveOverwrites(screen);

	if (Elem_HandlesKeyDown(&screen->Input.Base, key)) return true;
	return MenuScreen_HandlesKeyDown(elem, key);
}

static bool SaveLevelScreen_HandlesKeyUp(struct GuiElem* elem, Key key) {
	struct SaveLevelScreen* screen = (struct SaveLevelScreen*)elem;
	return Elem_HandlesKeyUp(&screen->Input.Base, key);
}

static void SaveLevelScreen_ContextRecreated(void* obj) {
	struct SaveLevelScreen* screen = (struct SaveLevelScreen*)obj;

	String save = String_FromConst("Save");
	Menu_Button(screen, 0, &screen->Buttons[0], 300, &save, &screen->TitleFont, SaveLevelScreen_Classic,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 20);

	String schematic = String_FromConst("Save schematic");
	Menu_Button(screen, 1, &screen->Buttons[1], 200, &schematic, &screen->TitleFont, SaveLevelScreen_Schematic,
		ANCHOR_CENTRE, ANCHOR_CENTRE, -150, 120);

	String mcEdit = String_FromConst("&eCan be imported into MCEdit");
	Menu_Label(screen, 2, &screen->MCEdit, &mcEdit, &screen->TextFont,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 110, 120);

	Menu_DefaultBack(screen, 3, &screen->Buttons[2], false, &screen->TitleFont, Menu_SwitchPause);

	struct MenuInputValidator validator = MenuInputValidator_Path();
	String empty = String_MakeNull();
	Menu_Input(screen, 4, &screen->Input, 500, &empty, &screen->TextFont, &validator,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -30);

	screen->Widgets[5] = NULL; /* description widget placeholder */
}

struct Screen* SaveLevelScreen_MakeInstance(void) {
	static struct Widget* widgets[6];
	struct SaveLevelScreen* screen = &SaveLevelScreen_Instance;
	MenuScreen_MakeInstance((struct MenuScreen*)screen, widgets, 
		Array_Elems(widgets), SaveLevelScreen_ContextRecreated);
	SaveLevelScreen_VTABLE = *screen->VTABLE;
	screen->VTABLE = &SaveLevelScreen_VTABLE;

	screen->VTABLE->Init   = SaveLevelScreen_Init;
	screen->VTABLE->Render = SaveLevelScreen_Render;
	screen->VTABLE->Free   = SaveLevelScreen_Free;

	screen->VTABLE->HandlesKeyDown  = SaveLevelScreen_HandlesKeyDown;
	screen->VTABLE->HandlesKeyPress = SaveLevelScreen_HandlesKeyPress;
	screen->VTABLE->HandlesKeyUp    = SaveLevelScreen_HandlesKeyUp;
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*---------------------------------------------------TexturePackScreen-----------------------------------------------------*
*#########################################################################################################################*/
static void TexturePackScreen_EntryClick(struct GuiElem* elem, struct GuiElem* w) {
	struct ListScreen* screen = (struct ListScreen*)elem;
	UChar pathBuffer[String_BufferSize(FILENAME_SIZE)];
	String path = String_InitAndClearArray(pathBuffer);

	String filename = ListScreen_UNSAFE_GetCur(screen, w);
	String_Format2(&path, "texpacks%r%s", &Directory_Separator, &filename);
	if (!File_Exists(&path)) return;
	
	Int32 curPage = screen->CurrentIndex;
	Game_SetDefaultTexturePack(&filename);
	TexturePack_ExtractDefault();
	Elem_Recreate(screen);
	ListScreen_SetCurrentIndex(screen, curPage);
}

static void TexturePackScreen_SelectEntry(STRING_PURE String* filename, void* obj) {
	String zip = String_FromConst(".zip");
	if (!String_CaselessEnds(filename, &zip)) return;

	StringsBuffer* entries = (StringsBuffer*)obj;
	StringsBuffer_Add(entries, filename);
}

struct Screen* TexturePackScreen_MakeInstance(void) {
	struct ListScreen* screen = ListScreen_MakeInstance();
	String title = String_FromConst("Select a texture pack zip"); screen->TitleText = title;
	screen->EntryClick = TexturePackScreen_EntryClick;

	String path = String_FromConst("texpacks");
	Directory_Enum(&path, &screen->Entries, TexturePackScreen_SelectEntry);
	if (screen->Entries.Count) {
		ListScreen_QuickSort(0, screen->Entries.Count - 1);
	}
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*----------------------------------------------------FontListScreen-------------------------------------------------------*
*#########################################################################################################################*/
static void FontListScreen_EntryClick(struct GuiElem* elem, struct GuiElem* w) {
	struct ListScreen* screen = (struct ListScreen*)elem;
	String fontName = ListScreen_UNSAFE_GetCur(screen, w);
	if (String_CaselessEqualsConst(&fontName, LIST_SCREEN_EMPTY)) return;

	String_Set(&Game_FontName, &fontName);
	Options_Set(OPT_FONT_NAME, &fontName);

	Int32 cur = screen->CurrentIndex;
	Menu_HandleFontChange(elem);
	ListScreen_SetCurrentIndex(screen, cur);
}

static void FontListScreen_UpdateEntry(struct ButtonWidget* button, STRING_PURE String* text) {
	struct FontDesc tmp = { 0 }, font = button->Font;
	Font_Make(&tmp, text, 16, FONT_STYLE_NORMAL);
	{
		button->Font = tmp;
		ButtonWidget_SetText(button, text);
		button->Font = font;
	}
	Font_Free(&tmp);
}

static void FontListScreen_Init(struct GuiElem* elem) {
	struct ListScreen* screen = (struct ListScreen*)elem;
	ListScreen_Init(elem);
	ListScreen_Select(screen, &Game_FontName);
}

struct Screen* FontListScreen_MakeInstance(void) {
	struct ListScreen* screen = ListScreen_MakeInstance();
	String title = String_FromConst("Select a font"); screen->TitleText = title;
	screen->EntryClick   = FontListScreen_EntryClick;
	screen->UpdateEntry  = FontListScreen_UpdateEntry;
	screen->VTABLE->Init = FontListScreen_Init;

	Font_GetNames(&screen->Entries);
	if (screen->Entries.Count) {
		ListScreen_QuickSort(0, screen->Entries.Count - 1);
	}
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*---------------------------------------------------HotkeyListScreen------------------------------------------------------*
*#########################################################################################################################*/
/* TODO: Hotkey added event for CPE */
static void HotkeyListScreen_EntryClick(struct GuiElem* elem, struct GuiElem* w) {
	struct ListScreen* screen = (struct ListScreen*)elem;
	String text = ListScreen_UNSAFE_GetCur(screen, w);
	struct HotkeyData original = { 0 };

	if (String_CaselessEqualsConst(&text, LIST_SCREEN_EMPTY)) {
		Gui_ReplaceActive(EditHotkeyScreen_MakeInstance(original)); return;
	}

	Int32 sepIndex = String_IndexOf(&text, '+', 0);
	String key = text, value;
	UInt8 flags = 0;

	if (sepIndex >= 0) {
		key   = String_UNSAFE_Substring(&text, 0, sepIndex - 1);
		value = String_UNSAFE_SubstringAt(&text, sepIndex + 1);

		String ctrl  = String_FromConst("Ctrl");
		String shift = String_FromConst("Shift");
		String alt   = String_FromConst("Alt");

		if (String_ContainsString(&value, &ctrl))  flags |= HOTKEYS_FLAG_CTRL;
		if (String_ContainsString(&value, &shift)) flags |= HOTKEYS_FLAG_SHIFT;
		if (String_ContainsString(&value, &alt))   flags |= HOTKEYS_FLAG_ALT;
	}

	Key baseKey = Utils_ParseEnum(&key, Key_None, Key_Names, Key_Count);
	Int32 i;
	for (i = 0; i < HotkeysText.Count; i++) {
		struct HotkeyData h = HotkeysList[i];
		if (h.Trigger == baseKey && h.Flags == flags) { original = h; break; }
	}
	Gui_ReplaceActive(EditHotkeyScreen_MakeInstance(original));
}

static void HotkeyListScreen_MakeFlags(UInt8 flags, STRING_TRANSIENT String* str) {
	if (flags & HOTKEYS_FLAG_CTRL)  String_AppendConst(str, " Ctrl");
	if (flags & HOTKEYS_FLAG_SHIFT) String_AppendConst(str, " Shift");
	if (flags & HOTKEYS_FLAG_ALT)   String_AppendConst(str, " Alt");
}

struct Screen* HotkeyListScreen_MakeInstance(void) {
	struct ListScreen* screen = ListScreen_MakeInstance();
	String title = String_FromConst("Modify hotkeys"); screen->TitleText = title;
	screen->EntryClick = HotkeyListScreen_EntryClick;

	UChar textBuffer[String_BufferSize(STRING_SIZE)];
	String text = String_InitAndClearArray(textBuffer);
	Int32 i;

	for (i = 0; i < HotkeysText.Count; i++) {
		struct HotkeyData hKey = HotkeysList[i];
		String_Clear(&text);
		String_AppendConst(&text, Key_Names[hKey.Trigger]);

		if (hKey.Flags) {
			String_AppendConst(&text, " +");
			HotkeyListScreen_MakeFlags(hKey.Flags, &text);
		}
		StringsBuffer_Add(&screen->Entries, &text);
	}

	String empty = String_FromConst(LIST_SCREEN_EMPTY);
	for (i = 0; i < LIST_SCREEN_ITEMS; i++) {
		StringsBuffer_Add(&screen->Entries, &empty);
	}
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*----------------------------------------------------LoadLevelScreen------------------------------------------------------*
*#########################################################################################################################*/
static void LoadLevelScreen_SelectEntry(STRING_PURE String* filename, void* obj) {
	String cw = String_FromConst(".cw");  String lvl = String_FromConst(".lvl");
	String fcm = String_FromConst(".fcm"); String dat = String_FromConst(".dat");

	if (!(String_CaselessEnds(filename, &cw) || String_CaselessEnds(filename, &lvl)
		|| String_CaselessEnds(filename, &fcm) || String_CaselessEnds(filename, &dat))) return;

	StringsBuffer* entries = (StringsBuffer*)obj;
	StringsBuffer_Add(entries, filename);
}

void LoadLevelScreen_LoadMap(STRING_PURE String* path) {
	World_Reset();
	Event_RaiseVoid(&WorldEvents_NewMap);

	if (World_TextureUrl.length) {
		TexturePack_ExtractDefault();
		String_Clear(&World_TextureUrl);
	}

	Block_Reset();
	Inventory_SetDefaultMapping();

	void* file; ReturnCode result = File_Open(&file, path);
	ErrorHandler_CheckOrFail(result, "Loading map - open file");
	struct Stream stream; Stream_FromFile(&stream, file, path);
	{
		String cw = String_FromConst(".cw");   String lvl = String_FromConst(".lvl");
		String fcm = String_FromConst(".fcm"); String dat = String_FromConst(".dat");

		if (String_CaselessEnds(path, &dat)) {
			result = Dat_Load(&stream);
		} else if (String_CaselessEnds(path, &fcm)) {
			result = Fcm_Load(&stream);
		} else if (String_CaselessEnds(path, &cw)) {
			result = Cw_Load(&stream);
		} else if (String_CaselessEnds(path, &lvl)) {
			result = Lvl_Load(&stream);
		}
		ErrorHandler_CheckOrFail(result, "Loading map - reading data");
	}
	result = stream.Close(&stream);
	ErrorHandler_CheckOrFail(result, "Loading map - close file");

	World_SetNewMap(World_Blocks, World_BlocksSize, World_Width, World_Height, World_Length);
	Event_RaiseVoid(&WorldEvents_MapLoaded);

	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct LocationUpdate update; LocationUpdate_MakePosAndOri(&update, p->Spawn, p->SpawnRotY, p->SpawnHeadX, false);
	p->Base.VTABLE->SetLocation(&p->Base, &update, false);
}

static void LoadLevelScreen_EntryClick(struct GuiElem* elem, struct GuiElem* w) {
	struct ListScreen* screen = (struct ListScreen*)elem;
	UChar pathBuffer[String_BufferSize(FILENAME_SIZE)];
	String path = String_InitAndClearArray(pathBuffer);

	String filename = ListScreen_UNSAFE_GetCur(screen, w);
	String_Format2(&path, "maps%r%s", &Directory_Separator, &filename);
	if (!File_Exists(&path)) return;
	LoadLevelScreen_LoadMap(&path);
}

struct Screen* LoadLevelScreen_MakeInstance(void) {
	struct ListScreen* screen = ListScreen_MakeInstance();
	String title = String_FromConst("Select a level"); screen->TitleText = title;
	screen->EntryClick = LoadLevelScreen_EntryClick;

	String path = String_FromConst("maps");
	Directory_Enum(&path, &screen->Entries, LoadLevelScreen_SelectEntry);
	if (screen->Entries.Count) {
		ListScreen_QuickSort(0, screen->Entries.Count - 1);
	}
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*---------------------------------------------------KeyBindingsScreen-----------------------------------------------------*
*#########################################################################################################################*/
struct KeyBindingsScreen KeyBindingsScreen_Instance;
struct GuiElementVTABLE KeyBindingsScreen_VTABLE;
static void KeyBindingsScreen_ButtonText(struct KeyBindingsScreen* screen, Int32 i, STRING_TRANSIENT String* text) {
	Key key = KeyBind_Get(screen->Binds[i]);
	String_Format2(text, "%c: %c", screen->Descs[i], Key_Names[key]);
}

static void KeyBindingsScreen_OnBindingClick(struct GuiElem* elem, struct GuiElem* widget) {
	struct KeyBindingsScreen* screen = (struct KeyBindingsScreen*)elem;
	UChar textBuffer[String_BufferSize(STRING_SIZE)];
	String text = String_InitAndClearArray(textBuffer);

	if (screen->CurI >= 0) {
		KeyBindingsScreen_ButtonText(screen, screen->CurI, &text);
		struct ButtonWidget* curButton = (struct ButtonWidget*)screen->Widgets[screen->CurI];
		ButtonWidget_SetText(curButton, &text);
	}
	screen->CurI = Menu_Index((struct MenuBase*)screen, (struct Widget*)widget);

	String_Clear(&text);
	String_AppendConst(&text, "> ");
	KeyBindingsScreen_ButtonText(screen, screen->CurI, &text);
	String_AppendConst(&text, " <");
	ButtonWidget_SetText((struct ButtonWidget*)widget, &text);
}

static Int32 KeyBindingsScreen_MakeWidgets(struct KeyBindingsScreen* screen, Int32 y, Int32 arrowsY, Int32 leftLength, STRING_PURE const UChar* title, Int32 btnWidth) {
	Int32 i, origin = y, xOffset = btnWidth / 2 + 5;
	screen->CurI = -1;

	struct Widget** widgets = screen->Widgets;
	UChar textBuffer[String_BufferSize(STRING_SIZE)];
	String text = String_InitAndClearArray(textBuffer);

	for (i = 0; i < screen->BindsCount; i++) {
		if (i == leftLength) y = origin; /* reset y for next column */
		Int32 xDir = leftLength == -1 ? 0 : (i < leftLength ? -1 : 1);

		String_Clear(&text);
		KeyBindingsScreen_ButtonText(screen, i, &text);

		Menu_Button(screen, i, &screen->Buttons[i], btnWidth, &text, &screen->TitleFont, KeyBindingsScreen_OnBindingClick,
			ANCHOR_CENTRE, ANCHOR_CENTRE, xDir * xOffset, y);
		y += 50; /* distance between buttons */
	}

	String titleText = String_FromReadonly(title);
	Menu_Label(screen, i, &screen->Title, &titleText, &screen->TitleFont,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -180); i++;

	Widget_LeftClick backClick = Game_UseClassicOptions ? Menu_SwitchClassicOptions : Menu_SwitchOptions;
	Menu_DefaultBack(screen, i, &screen->Back, false, &screen->TitleFont, backClick); i++;
	if (!screen->LeftPage && !screen->RightPage) return i;

	String lArrow = String_FromConst("<");
	Menu_Button(screen, i, &screen->Left, 40, &lArrow, &screen->TitleFont, screen->LeftPage,
		ANCHOR_CENTRE, ANCHOR_CENTRE, -btnWidth - 35, arrowsY); i++;
	screen->Left.Disabled = !screen->LeftPage;

	String rArrow = String_FromConst(">");
	Menu_Button(screen, i, &screen->Right, 40, &rArrow, &screen->TitleFont, screen->RightPage,
		ANCHOR_CENTRE, ANCHOR_CENTRE, btnWidth + 35, arrowsY); i++;
	screen->Right.Disabled = !screen->RightPage;

	return i;
}

static void KeyBindingsScreen_Init(struct GuiElem* elem) {
	struct KeyBindingsScreen* screen = (struct KeyBindingsScreen*)elem;
	MenuScreen_Init(elem);
	screen->ContextRecreated(elem);
}

static bool KeyBindingsScreen_HandlesKeyDown(struct GuiElem* elem, Key key) {
	struct KeyBindingsScreen* screen = (struct KeyBindingsScreen*)elem;
	if (screen->CurI == -1) return MenuScreen_HandlesKeyDown(elem, key);

	KeyBind bind = screen->Binds[screen->CurI];
	if (key == Key_Escape) key = KeyBind_GetDefault(bind);
	KeyBind_Set(bind, key);

	UChar textBuffer[String_BufferSize(STRING_SIZE)];
	String text = String_InitAndClearArray(textBuffer);
	KeyBindingsScreen_ButtonText(screen, screen->CurI, &text);

	struct ButtonWidget* curButton = (struct ButtonWidget*)screen->Widgets[screen->CurI];
	ButtonWidget_SetText(curButton, &text);
	screen->CurI = -1;
	return true;
}

static bool KeyBindingsScreen_HandlesMouseDown(struct GuiElem* elem, Int32 x, Int32 y, MouseButton btn) {
	if (btn != MouseButton_Right) {
		return MenuScreen_HandlesMouseDown(elem, x, y, btn);
	}

	struct KeyBindingsScreen* screen = (struct KeyBindingsScreen*)elem;
	Int32 i = Menu_HandleMouseDown((struct MenuBase*)elem, x, y, btn);
	if (i == -1) return false;

	/* Reset a key binding */
	if ((screen->CurI == -1 || screen->CurI == i) && i < screen->BindsCount) {
		screen->CurI = i;
		Elem_HandlesKeyDown(elem, KeyBind_GetDefault(screen->Binds[i]));
	}
	return true;
}

static struct KeyBindingsScreen* KeyBindingsScreen_Make(Int32 bindsCount, UInt8* binds, const UChar** descs, struct ButtonWidget* buttons, struct Widget** widgets, Menu_ContextFunc contextRecreated) {
	struct KeyBindingsScreen* screen = &KeyBindingsScreen_Instance;
	MenuScreen_MakeInstance((struct MenuScreen*)screen, widgets, bindsCount + 4, contextRecreated);
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
static void ClassicKeyBindingsScreen_ContextRecreated(void* obj) {
	struct KeyBindingsScreen* screen = (struct KeyBindingsScreen*)obj;
	if (Game_ClassicHacks) {
		KeyBindingsScreen_MakeWidgets(screen, -140, -40, 5, "Normal controls", 260);
	} else {
		KeyBindingsScreen_MakeWidgets(screen, -140, -40, 5, "Controls", 300);
	}
}

struct Screen* ClassicKeyBindingsScreen_MakeInstance(void) {
	static UInt8 binds[10] = { KeyBind_Forward, KeyBind_Back, KeyBind_Jump, KeyBind_Chat, KeyBind_SetSpawn, KeyBind_Left, KeyBind_Right, KeyBind_Inventory, KeyBind_ToggleFog, KeyBind_Respawn };
	static const UChar* descs[10] = { "Forward", "Back", "Jump", "Chat", "Save loc", "Left", "Right", "Build", "Toggle fog", "Load loc" };
	static struct ButtonWidget buttons[10];
	static struct Widget* widgets[10 + 4];

	struct KeyBindingsScreen* screen = KeyBindingsScreen_Make(Array_Elems(binds), binds, descs, buttons, widgets, ClassicKeyBindingsScreen_ContextRecreated);
	if (Game_ClassicHacks) screen->RightPage = Menu_SwitchKeysClassicHacks;
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*--------------------------------------------ClassicHacksKeyBindingsScreen------------------------------------------------*
*#########################################################################################################################*/
static void ClassicHacksKeyBindingsScreen_ContextRecreated(void* obj) {
	struct KeyBindingsScreen* screen = (struct KeyBindingsScreen*)obj;
	KeyBindingsScreen_MakeWidgets(screen, -90, -40, 3, "Hacks controls", 260);
}

struct Screen* ClassicHacksKeyBindingsScreen_MakeInstance(void) {
	static UInt8 binds[6] = { KeyBind_Speed, KeyBind_NoClip, KeyBind_HalfSpeed, KeyBind_Fly, KeyBind_FlyUp, KeyBind_FlyDown };
	static const UChar* descs[6] = { "Speed", "Noclip", "Half speed", "Fly", "Fly up", "Fly down" };
	static struct ButtonWidget buttons[6];
	static struct Widget* widgets[6 + 4];

	struct KeyBindingsScreen* screen = KeyBindingsScreen_Make(Array_Elems(binds), binds, descs, buttons, widgets, ClassicHacksKeyBindingsScreen_ContextRecreated);
	screen->LeftPage = Menu_SwitchKeysClassic;
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*-----------------------------------------------NormalKeyBindingsScreen---------------------------------------------------*
*#########################################################################################################################*/
static void NormalKeyBindingsScreen_ContextRecreated(void* obj) {
	struct KeyBindingsScreen* screen = (struct KeyBindingsScreen*)obj;
	KeyBindingsScreen_MakeWidgets(screen, -140, 10, 6, "Normal controls", 260);
}

struct Screen* NormalKeyBindingsScreen_MakeInstance(void) {
	static UInt8 binds[12] = { KeyBind_Forward, KeyBind_Back, KeyBind_Jump, KeyBind_Chat, KeyBind_SetSpawn, KeyBind_PlayerList, KeyBind_Left, KeyBind_Right, KeyBind_Inventory, KeyBind_ToggleFog, KeyBind_Respawn, KeyBind_SendChat };
	static const UChar* descs[12] = { "Forward", "Back", "Jump", "Chat", "Set spawn", "Player list", "Left", "Right", "Inventory", "Toggle fog", "Respawn", "Send chat" };
	static struct ButtonWidget buttons[12];
	static struct Widget* widgets[12 + 4];

	struct KeyBindingsScreen* screen = KeyBindingsScreen_Make(Array_Elems(binds), binds, descs, buttons, widgets, NormalKeyBindingsScreen_ContextRecreated);
	screen->RightPage = Menu_SwitchKeysHacks;
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*------------------------------------------------HacksKeyBindingsScreen---------------------------------------------------*
*#########################################################################################################################*/
static void HacksKeyBindingsScreen_ContextRecreated(void* obj) {
	struct KeyBindingsScreen* screen = (struct KeyBindingsScreen*)obj;
	KeyBindingsScreen_MakeWidgets(screen, -40, 10, 4, "Hacks controls", 260);
}

struct Screen* HacksKeyBindingsScreen_MakeInstance(void) {
	static UInt8 binds[8] = { KeyBind_Speed, KeyBind_NoClip, KeyBind_HalfSpeed, KeyBind_ZoomScrolling, KeyBind_Fly, KeyBind_FlyUp, KeyBind_FlyDown, KeyBind_ThirdPerson };
	static const UChar* descs[8] = { "Speed", "Noclip", "Half speed", "Scroll zoom", "Fly", "Fly up", "Fly down", "Third person" };
	static struct ButtonWidget buttons[8];
	static struct Widget* widgets[8 + 4];

	struct KeyBindingsScreen* screen = KeyBindingsScreen_Make(Array_Elems(binds), binds, descs, buttons, widgets, HacksKeyBindingsScreen_ContextRecreated);
	screen->LeftPage  = Menu_SwitchKeysNormal;
	screen->RightPage = Menu_SwitchKeysOther;
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*------------------------------------------------OtherKeyBindingsScreen---------------------------------------------------*
*#########################################################################################################################*/
static void OtherKeyBindingsScreen_ContextRecreated(void* obj) {
	struct KeyBindingsScreen* screen = (struct KeyBindingsScreen*)obj;
	KeyBindingsScreen_MakeWidgets(screen, -140, 10, 6, "Other controls", 260);
}

struct Screen* OtherKeyBindingsScreen_MakeInstance(void) {
	static UInt8 binds[12] = { KeyBind_ExtInput, KeyBind_HideFps, KeyBind_HideGui, KeyBind_HotbarSwitching, KeyBind_DropBlock,KeyBind_Screenshot, KeyBind_Fullscreen, KeyBind_AxisLines, KeyBind_Autorotate, KeyBind_SmoothCamera, KeyBind_IDOverlay, KeyBind_BreakableLiquids };
	static const UChar* descs[12] = { "Show ext input", "Hide FPS", "Hide gui", "Hotbar switching", "Drop block", "Screenshot", "Fullscreen", "Show axis lines", "Auto-rotate", "Smooth camera", "ID overlay", "Breakable liquids" };
	static struct ButtonWidget buttons[12];
	static struct Widget* widgets[12 + 4];

	struct KeyBindingsScreen* screen = KeyBindingsScreen_Make(Array_Elems(binds), binds, descs, buttons, widgets, OtherKeyBindingsScreen_ContextRecreated);
	screen->LeftPage  = Menu_SwitchKeysHacks;
	screen->RightPage = Menu_SwitchKeysMouse;
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*------------------------------------------------MouseKeyBindingsScreen---------------------------------------------------*
*#########################################################################################################################*/
static void MouseKeyBindingsScreen_ContextRecreated(void* obj) {
	struct KeyBindingsScreen* screen = (struct KeyBindingsScreen*)obj;
	Int32 i = KeyBindingsScreen_MakeWidgets(screen, -40, 10, -1, "Mouse key bindings", 260);

	static struct TextWidget text;
	String msg = String_FromConst("&eRight click to remove the key binding");
	Menu_Label(screen, i, &text, &msg, &screen->TextFont,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 100);
}

struct Screen* MouseKeyBindingsScreen_MakeInstance(void) {
	static UInt8 binds[3] = { KeyBind_MouseLeft, KeyBind_MouseMiddle, KeyBind_MouseRight };
	static const UChar* descs[3] = { "Left", "Middle", "Right" };
	static struct ButtonWidget buttons[3];
	static struct Widget* widgets[3 + 4 + 1];

	struct KeyBindingsScreen* screen = KeyBindingsScreen_Make(Array_Elems(binds), binds, descs, buttons, widgets, MouseKeyBindingsScreen_ContextRecreated);
	screen->LeftPage = Menu_SwitchKeysOther;
	screen->WidgetsCount++; /* Extra text widget for 'right click' message */
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*--------------------------------------------------MenuOptionsScreen------------------------------------------------------*
*#########################################################################################################################*/
struct MenuOptionsScreen MenuOptionsScreen_Instance;
struct GuiElementVTABLE MenuOptionsScreen_VTABLE;
static void Menu_GetBool(STRING_TRANSIENT String* raw, bool v) {
	String_AppendConst(raw, v ? "ON" : "OFF");
}

static bool Menu_SetBool(STRING_PURE String* raw, const UChar* key) {
	bool isOn = String_CaselessEqualsConst(raw, "ON");
	Options_SetBool(key, isOn); return isOn;
}

static void MenuOptionsScreen_GetFPS(STRING_TRANSIENT String* raw) {
	String_AppendConst(raw, FpsLimit_Names[Game_FpsLimit]);
}
static void MenuOptionsScreen_SetFPS(STRING_PURE String* raw) {
	UInt32 method = Utils_ParseEnum(raw, FpsLimit_VSync, FpsLimit_Names, Array_Elems(FpsLimit_Names));
	Game_SetFpsLimitMethod(method);

	String value = String_FromReadonly(FpsLimit_Names[method]);
	Options_Set(OPT_FPS_LIMIT, &value);
}

static void MenuOptionsScreen_Set(struct MenuOptionsScreen* screen, Int32 i, STRING_PURE String* text) {
	screen->Buttons[i].SetValue(text);
	/* need to get btn again here (e.g. changing FPS invalidates all widgets) */

	UChar titleBuffer[String_BufferSize(STRING_SIZE)];
	String title = String_InitAndClearArray(titleBuffer);
	String_AppendConst(&title, screen->Buttons[i].OptName);
	String_AppendConst(&title, ": ");
	screen->Buttons[i].GetValue(&title);
	ButtonWidget_SetText(&screen->Buttons[i], &title);
}

static void MenuOptionsScreen_FreeExtHelp(struct MenuOptionsScreen* screen) {
	if (!screen->ExtHelp.LinesCount) return;
	Elem_TryFree(&screen->ExtHelp);
	screen->ExtHelp.LinesCount = 0;
}

static void MenuOptionsScreen_RepositionExtHelp(struct MenuOptionsScreen* screen) {
	screen->ExtHelp.XOffset = Game_Width / 2 - screen->ExtHelp.Width / 2;
	screen->ExtHelp.YOffset = Game_Height / 2 + 100;
	Widget_Reposition(&screen->ExtHelp);
}

static void MenuOptionsScreen_SelectExtHelp(struct MenuOptionsScreen* screen, Int32 idx) {
	MenuOptionsScreen_FreeExtHelp(screen);
	if (!screen->Descriptions || screen->ActiveI >= 0) return;
	const UChar* desc = screen->Descriptions[idx];
	if (!desc) return;

	String descRaw = String_FromReadonly(desc);
	String descLines[5];
	Int32 descLinesCount = Array_Elems(descLines);
	String_UNSAFE_Split(&descRaw, '%', descLines, &descLinesCount);

	TextGroupWidget_Create(&screen->ExtHelp, descLinesCount, &screen->TextFont, &screen->TextFont, screen->ExtHelp_Textures, screen->ExtHelp_Buffer);
	Widget_SetLocation((struct Widget*)(&screen->ExtHelp), ANCHOR_MIN, ANCHOR_MIN, 0, 0);
	Elem_Init(&screen->ExtHelp);

	Int32 i;
	for (i = 0; i < descLinesCount; i++) {
		TextGroupWidget_SetText(&screen->ExtHelp, i, &descLines[i]);
	}
	MenuOptionsScreen_RepositionExtHelp(screen);
}

static void MenuOptionsScreen_FreeInput(struct MenuOptionsScreen* screen) {
	if (screen->ActiveI == -1) return;

	Int32 i;
	for (i = screen->WidgetsCount - 3; i < screen->WidgetsCount; i++) {
		if (!screen->Widgets[i]) continue;
		Elem_TryFree(screen->Widgets[i]);
		screen->Widgets[i] = NULL;
	}
}

static void MenuOptionsScreen_EnterInput(struct MenuOptionsScreen* screen) {
	String text = screen->Input.Base.Text;
	struct MenuInputValidator* validator = &screen->Input.Validator;

	if (validator->IsValidValue(validator, &text)) {
		MenuOptionsScreen_Set(screen, screen->ActiveI, &text);
	}

	MenuOptionsScreen_SelectExtHelp(screen, screen->ActiveI);
	MenuOptionsScreen_FreeInput(screen);
	screen->ActiveI = -1;
}

static void MenuOptionsScreen_Init(struct GuiElem* elem) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)elem;
	MenuScreen_Init(elem);
	Key_KeyRepeat = true;
	screen->SelectedI = -1;
	screen->ContextRecreated(elem);
}
	
static void MenuOptionsScreen_Render(struct GuiElem* elem, Real64 delta) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)elem;
	MenuScreen_Render(elem, delta);
	if (!screen->ExtHelp.LinesCount) return;

	struct TextGroupWidget* extHelp = &screen->ExtHelp;
	Int32 x = extHelp->X - 5, y = extHelp->Y - 5;
	Int32 width = extHelp->Width, height = extHelp->Height;
	PackedCol tableCol = PACKEDCOL_CONST(20, 20, 20, 200);
	GfxCommon_Draw2DFlat(x, y, width + 10, height + 10, tableCol);

	Gfx_SetTexturing(true);
	Elem_Render(&screen->ExtHelp, delta);
	Gfx_SetTexturing(false);
}

static void MenuOptionsScreen_Free(struct GuiElem* elem) {
	MenuScreen_Free(elem);
	Key_KeyRepeat = false;
}

static void MenuOptionsScreen_OnResize(struct GuiElem* elem) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)elem;
	MenuScreen_OnResize(elem);
	if (!screen->ExtHelp.LinesCount) return;
	MenuOptionsScreen_RepositionExtHelp(screen);
}

static void MenuOptionsScreen_ContextLost(void* obj) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)obj;
	MenuScreen_ContextLost(obj);
	screen->ActiveI = -1;
	MenuOptionsScreen_FreeExtHelp(screen);
}

static bool MenuOptionsScreen_HandlesKeyPress(struct GuiElem* elem, UChar key) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)elem;
	if (screen->ActiveI == -1) return true;
	return Elem_HandlesKeyPress(&screen->Input.Base, key);
}

static bool MenuOptionsScreen_HandlesKeyDown(struct GuiElem* elem, Key key) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)elem;
	if (screen->ActiveI >= 0) {
		if (Elem_HandlesKeyDown(&screen->Input.Base, key)) return true;
		if (key == Key_Enter || key == Key_KeypadEnter) {
			MenuOptionsScreen_EnterInput(screen); return true;
		}
	}
	return MenuScreen_HandlesKeyDown(elem, key);
}

static bool MenuOptionsScreen_HandlesKeyUp(struct GuiElem* elem, Key key) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)elem;
	if (screen->ActiveI == -1) return true;
	return Elem_HandlesKeyUp(&screen->Input.Base, key);
}

static bool MenuOptionsScreen_HandlesMouseMove(struct GuiElem* elem, Int32 x, Int32 y) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)elem;
	Int32 i = Menu_HandleMouseMove((struct MenuBase*)elem, x, y);
	if (i == -1 || i == screen->SelectedI) return true;
	if (!screen->Descriptions || i >= screen->DescriptionsCount) return true;

	screen->SelectedI = i;
	if (screen->ActiveI == -1) MenuOptionsScreen_SelectExtHelp(screen, i);
	return true;
}

static void MenuOptionsScreen_Make(struct MenuOptionsScreen* screen, Int32 i, Int32 dir, Int32 y, const UChar* optName, Widget_LeftClick onClick, ButtonWidget_Get getter, ButtonWidget_Set setter) {
	UChar titleBuffer[String_BufferSize(STRING_SIZE)];
	String title = String_InitAndClearArray(titleBuffer);
	String_AppendConst(&title, optName);
	String_AppendConst(&title, ": ");
	getter(&title);

	struct ButtonWidget* btn = &screen->Buttons[i];
	Menu_Button(screen, i, btn, 300, &title, &screen->TitleFont, onClick,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 160 * dir, y);

	btn->OptName  = optName;
	btn->GetValue = getter;
	btn->SetValue = setter;
}

static void MenuOptionsScreen_OK(struct GuiElem* elem, struct GuiElem* widget) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)elem; 
	MenuOptionsScreen_EnterInput(screen);
}

static void MenuOptionsScreen_Default(struct GuiElem* elem, struct GuiElem* widget) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)elem;
	String text = String_FromReadonly(screen->DefaultValues[screen->ActiveI]);
	InputWidget_Clear(&screen->Input.Base);
	InputWidget_AppendString(&screen->Input.Base, &text);
}

static void MenuOptionsScreen_Bool(struct GuiElem* elem, struct GuiElem* widget) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)elem;
	struct ButtonWidget* button = (struct ButtonWidget*)widget;
	Int32 index = Menu_Index((struct MenuBase*)screen, (struct Widget*)widget);
	MenuOptionsScreen_SelectExtHelp(screen, index);

	UChar valueBuffer[String_BufferSize(STRING_SIZE)];
	String value = String_InitAndClearArray(valueBuffer);
	button->GetValue(&value);

	bool isOn = String_CaselessEqualsConst(&value, "ON");
	String newValue = String_FromReadonly(isOn ? "OFF" : "ON");
	MenuOptionsScreen_Set(screen, index, &newValue);
}

static void MenuOptionsScreen_Enum(struct GuiElem* elem, struct GuiElem* widget) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)elem;
	struct ButtonWidget* button = (struct ButtonWidget*)widget;
	Int32 index = Menu_Index((struct MenuBase*)screen, (struct Widget*)widget);
	MenuOptionsScreen_SelectExtHelp(screen, index);

	struct MenuInputValidator* validator = &screen->Validators[index];
	const UChar** names = (const UChar**)validator->Meta_Ptr[0];
	UInt32 count = (UInt32)validator->Meta_Ptr[1];

	UChar valueBuffer[String_BufferSize(STRING_SIZE)];
	String value = String_InitAndClearArray(valueBuffer);
	button->GetValue(&value);

	UInt32 raw = (Utils_ParseEnum(&value, 0, names, count) + 1) % count;
	String newValue = String_FromReadonly(names[raw]);
	MenuOptionsScreen_Set(screen, index, &newValue);
}

static void MenuOptionsScreen_Input(struct GuiElem* elem, struct GuiElem* widget) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)elem;
	struct ButtonWidget* button = (struct ButtonWidget*)widget;
	screen->ActiveI = Menu_Index((struct MenuBase*)screen, (struct Widget*)widget);
	MenuOptionsScreen_FreeExtHelp(screen);
	MenuOptionsScreen_FreeInput(screen);

	UChar valueBuffer[String_BufferSize(STRING_SIZE)];
	String value = String_InitAndClearArray(valueBuffer);
	button->GetValue(&value);
	Int32 count = screen->WidgetsCount;

	struct MenuInputValidator* validator = &screen->Validators[screen->ActiveI];
	Menu_Input(screen, count - 1, &screen->Input, 400, &value, &screen->TextFont, validator,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 110);

	String okMsg = String_FromConst("OK");
	Menu_Button(screen, count - 2, &screen->OK, 40, &okMsg, &screen->TitleFont, MenuOptionsScreen_OK,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 240, 110);

	String defMsg = String_FromConst("Default value");
	Menu_Button(screen, count - 3, &screen->Default, 200, &defMsg, &screen->TitleFont, MenuOptionsScreen_Default,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 150);
}

struct Screen* MenuOptionsScreen_MakeInstance(struct Widget** widgets, Int32 count, struct ButtonWidget* buttons, Menu_ContextFunc contextRecreated,
	struct MenuInputValidator* validators, const UChar** defaultValues, const UChar** descriptions, Int32 descsCount) {
	struct MenuOptionsScreen* screen = &MenuOptionsScreen_Instance;
	Mem_Set(screen, 0, sizeof(struct MenuOptionsScreen));
	MenuScreen_MakeInstance((struct MenuScreen*)screen, widgets, count, contextRecreated);
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
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*---------------------------------------------------ClassicOptionsScreen--------------------------------------------------*
*#########################################################################################################################*/
typedef enum ViewDist_ {
	ViewDist_Tiny, ViewDist_Short, ViewDist_Normal, ViewDist_Far, ViewDist_Count,
} ViewDist;
const UChar* ViewDist_Names[ViewDist_Count] = { "TINY", "SHORT", "NORMAL", "FAR" };

static void ClassicOptionsScreen_GetMusic(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_MusicVolume > 0); }
static void ClassicOptionsScreen_SetMusic(STRING_PURE String* v) {
	Game_MusicVolume = String_CaselessEqualsConst(v, "ON") ? 100 : 0;
	Audio_SetMusic(Game_MusicVolume);
	Options_SetInt32(OPT_MUSIC_VOLUME, Game_MusicVolume);
}

static void ClassicOptionsScreen_GetInvert(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_InvertMouse); }
static void ClassicOptionsScreen_SetInvert(STRING_PURE String* v) { Game_InvertMouse = Menu_SetBool(v, OPT_INVERT_MOUSE); }

static void ClassicOptionsScreen_GetViewDist(STRING_TRANSIENT String* v) {
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
static void ClassicOptionsScreen_SetViewDist(STRING_PURE String* v) {
	UInt32 raw = Utils_ParseEnum(v, 0, ViewDist_Names, ViewDist_Count);
	Int32 dist = raw == ViewDist_Far ? 512 : (raw == ViewDist_Normal ? 128 : (raw == ViewDist_Short ? 32 : 8));
	Game_SetViewDistance(dist, true);
}

static void ClassicOptionsScreen_GetPhysics(STRING_TRANSIENT String* v) { Menu_GetBool(v, Physics_Enabled); }
static void ClassicOptionsScreen_SetPhysics(STRING_PURE String* v) {
	Physics_SetEnabled(Menu_SetBool(v, OPT_BLOCK_PHYSICS));
}

static void ClassicOptionsScreen_GetSounds(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_SoundsVolume > 0); }
static void ClassicOptionsScreen_SetSounds(STRING_PURE String* v) {
	Game_SoundsVolume = String_CaselessEqualsConst(v, "ON") ? 100 : 0;
	Audio_SetSounds(Game_SoundsVolume);
	Options_SetInt32(OPT_SOUND_VOLUME, Game_SoundsVolume);
}

static void ClassicOptionsScreen_GetShowFPS(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_ShowFPS); }
static void ClassicOptionsScreen_SetShowFPS(STRING_PURE String* v) { Game_ShowFPS = Menu_SetBool(v, OPT_SHOW_FPS); }

static void ClassicOptionsScreen_GetViewBob(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_ViewBobbing); }
static void ClassicOptionsScreen_SetViewBob(STRING_PURE String* v) { Game_ViewBobbing = Menu_SetBool(v, OPT_VIEW_BOBBING); }

static void ClassicOptionsScreen_GetHacks(STRING_TRANSIENT String* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.Enabled); }
static void ClassicOptionsScreen_SetHacks(STRING_PURE String* v) {
	LocalPlayer_Instance.Hacks.Enabled = Menu_SetBool(v, OPT_HACKS_ENABLED);
	LocalPlayer_CheckHacksConsistency();
}

static void ClassicOptionsScreen_ContextRecreated(void* obj) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)obj;

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
	Menu_Button(screen, 9, &screen->Buttons[9], 400, &controls, &screen->TitleFont, Menu_SwitchKeysClassic,
		ANCHOR_CENTRE, ANCHOR_MAX, 0, 95);

	String done = String_FromConst("Done");
	Menu_Back(screen, 10, &screen->Buttons[10], 400, &done, 25, &screen->TitleFont, Menu_SwitchPause);

	/* Disable certain options */
	if (!ServerConnection_IsSinglePlayer) MenuScreen_Remove((struct MenuScreen*)screen, 3);
	if (!Game_ClassicHacks)               MenuScreen_Remove((struct MenuScreen*)screen, 8);
}

struct Screen* ClassicOptionsScreen_MakeInstance(void) {
	static struct ButtonWidget buttons[11];
	static struct MenuInputValidator validators[Array_Elems(buttons)];
	static struct Widget* widgets[Array_Elems(buttons)];	

	validators[2] = MenuInputValidator_Enum(ViewDist_Names, ViewDist_Count);
	validators[7] = MenuInputValidator_Enum(FpsLimit_Names, FpsLimit_Count);

	return MenuOptionsScreen_MakeInstance(widgets, Array_Elems(widgets), buttons,
		ClassicOptionsScreen_ContextRecreated, validators, NULL, NULL, 0);
}


/*########################################################################################################################*
*----------------------------------------------------EnvSettingsScreen----------------------------------------------------*
*#########################################################################################################################*/
static void EnvSettingsScreen_GetCloudsCol(STRING_TRANSIENT String* v) { PackedCol_ToHex(v, WorldEnv_CloudsCol); }
static void EnvSettingsScreen_SetCloudsCol(STRING_PURE String* v) { WorldEnv_SetCloudsCol(Menu_HexCol(v)); }

static void EnvSettingsScreen_GetSkyCol(STRING_TRANSIENT String* v) { PackedCol_ToHex(v, WorldEnv_SkyCol); }
static void EnvSettingsScreen_SetSkyCol(STRING_PURE String* v) { WorldEnv_SetSkyCol(Menu_HexCol(v)); }

static void EnvSettingsScreen_GetFogCol(STRING_TRANSIENT String* v) { PackedCol_ToHex(v, WorldEnv_FogCol); }
static void EnvSettingsScreen_SetFogCol(STRING_PURE String* v) { WorldEnv_SetFogCol(Menu_HexCol(v)); }

static void EnvSettingsScreen_GetCloudsSpeed(STRING_TRANSIENT String* v) { String_AppendReal32(v, WorldEnv_CloudsSpeed, 2); }
static void EnvSettingsScreen_SetCloudsSpeed(STRING_PURE String* v) { WorldEnv_SetCloudsSpeed(Menu_Real32(v)); }

static void EnvSettingsScreen_GetCloudsHeight(STRING_TRANSIENT String* v) { String_AppendInt32(v, WorldEnv_CloudsHeight); }
static void EnvSettingsScreen_SetCloudsHeight(STRING_PURE String* v) { WorldEnv_SetCloudsHeight(Menu_Int32(v)); }

static void EnvSettingsScreen_GetSunCol(STRING_TRANSIENT String* v) { PackedCol_ToHex(v, WorldEnv_SunCol); }
static void EnvSettingsScreen_SetSunCol(STRING_PURE String* v) { WorldEnv_SetSunCol(Menu_HexCol(v)); }

static void EnvSettingsScreen_GetShadowCol(STRING_TRANSIENT String* v) { PackedCol_ToHex(v, WorldEnv_ShadowCol); }
static void EnvSettingsScreen_SetShadowCol(STRING_PURE String* v) { WorldEnv_SetShadowCol(Menu_HexCol(v)); }

static void EnvSettingsScreen_GetWeather(STRING_TRANSIENT String* v) { String_AppendConst(v, Weather_Names[WorldEnv_Weather]); }
static void EnvSettingsScreen_SetWeather(STRING_PURE String* v) {
	UInt32 raw = Utils_ParseEnum(v, 0, Weather_Names, Array_Elems(Weather_Names));
	WorldEnv_SetWeather(raw); 
}

static void EnvSettingsScreen_GetWeatherSpeed(STRING_TRANSIENT String* v) { String_AppendReal32(v, WorldEnv_WeatherSpeed, 2); }
static void EnvSettingsScreen_SetWeatherSpeed(STRING_PURE String* v) { WorldEnv_SetWeatherSpeed(Menu_Real32(v)); }

static void EnvSettingsScreen_GetEdgeHeight(STRING_TRANSIENT String* v) { String_AppendInt32(v, WorldEnv_EdgeHeight); }
static void EnvSettingsScreen_SetEdgeHeight(STRING_PURE String* v) { WorldEnv_SetEdgeHeight(Menu_Int32(v)); }

static void EnvSettingsScreen_ContextRecreated(void* obj) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)obj;
	struct Widget** widgets = screen->Widgets;

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

	Menu_DefaultBack(screen, 10, &screen->Buttons[10], false, &screen->TitleFont, Menu_SwitchOptions);
	widgets[11] = NULL; widgets[12] = NULL; widgets[13] = NULL;
}

struct Screen* EnvSettingsScreen_MakeInstance(void) {
	static struct ButtonWidget buttons[11];
	static struct MenuInputValidator validators[Array_Elems(buttons)];
	static const UChar* defaultValues[Array_Elems(buttons)];
	static struct Widget* widgets[Array_Elems(buttons) + 3];

	static UChar cloudHeightBuffer[String_BufferSize(STRING_INT_CHARS)];
	String cloudHeight = String_InitAndClearArray(cloudHeightBuffer);
	String_AppendInt32(&cloudHeight, World_Height + 2);

	static UChar edgeHeightBuffer[String_BufferSize(STRING_INT_CHARS)];
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
static void GraphicsOptionsScreen_GetViewDist(STRING_TRANSIENT String* v) { String_AppendInt32(v, Game_ViewDistance); }
static void GraphicsOptionsScreen_SetViewDist(STRING_PURE String* v) { Game_SetViewDistance(Menu_Int32(v), true); }

static void GraphicsOptionsScreen_GetSmooth(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_SmoothLighting); }
static void GraphicsOptionsScreen_SetSmooth(STRING_PURE String* v) {
	Game_SmoothLighting = Menu_SetBool(v, OPT_SMOOTH_LIGHTING);
	ChunkUpdater_ApplyMeshBuilder();
	ChunkUpdater_Refresh();
}

static void GraphicsOptionsScreen_GetNames(STRING_TRANSIENT String* v) { String_AppendConst(v, NameMode_Names[Entities_NameMode]); }
static void GraphicsOptionsScreen_SetNames(STRING_PURE String* v) {
	Entities_NameMode = Utils_ParseEnum(v, 0, NameMode_Names, NAME_MODE_COUNT);
	Options_Set(OPT_NAMES_MODE, v);
}

static void GraphicsOptionsScreen_GetShadows(STRING_TRANSIENT String* v) { String_AppendConst(v, ShadowMode_Names[Entities_ShadowMode]); }
static void GraphicsOptionsScreen_SetShadows(STRING_PURE String* v) {
	Entities_ShadowMode = Utils_ParseEnum(v, 0, ShadowMode_Names, SHADOW_MODE_COUNT);
	Options_Set(OPT_ENTITY_SHADOW, v);
}

static void GraphicsOptionsScreen_GetMipmaps(STRING_TRANSIENT String* v) { Menu_GetBool(v, Gfx_Mipmaps); }
static void GraphicsOptionsScreen_SetMipmaps(STRING_PURE String* v) {
	Gfx_Mipmaps = Menu_SetBool(v, OPT_MIPMAPS);
	UChar urlBuffer[String_BufferSize(STRING_SIZE)];
	String url = String_InitAndClearArray(urlBuffer);
	String_Set(&url, &World_TextureUrl);

	/* always force a reload from cache */
	String_Clear(&World_TextureUrl);
	String_AppendConst(&World_TextureUrl, "~`#$_^*()@");
	TexturePack_ExtractCurrent(&url);

	String_Set(&World_TextureUrl, &url);
}

static void GraphicsOptionsScreen_ContextRecreated(void* obj) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)obj;
	struct Widget** widgets = screen->Widgets;

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

	Menu_DefaultBack(screen, 6, &screen->Buttons[6], false, &screen->TitleFont, Menu_SwitchOptions);
	widgets[7] = NULL; widgets[8] = NULL; widgets[9] = NULL;
}

struct Screen* GraphicsOptionsScreen_MakeInstance(void) {
	static struct ButtonWidget buttons[7];
	static struct MenuInputValidator validators[Array_Elems(buttons)];
	static const UChar* defaultValues[Array_Elems(buttons)];
	static struct Widget* widgets[Array_Elems(buttons) + 3];

	validators[0]    = MenuInputValidator_Enum(FpsLimit_Names, FpsLimit_Count);
	validators[1]    = MenuInputValidator_Integer(8, 4096);
	defaultValues[1] = "512";
	validators[3]    = MenuInputValidator_Enum(NameMode_Names,   NAME_MODE_COUNT);
	validators[4]    = MenuInputValidator_Enum(ShadowMode_Names, SHADOW_MODE_COUNT);
	
	static const UChar* descs[Array_Elems(buttons)];
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
static void GuiOptionsScreen_GetShadows(STRING_TRANSIENT String* v) { Menu_GetBool(v, Drawer2D_BlackTextShadows); }
static void GuiOptionsScreen_SetShadows(STRING_PURE String* v) {
	Drawer2D_BlackTextShadows = Menu_SetBool(v, OPT_BLACK_TEXT);
	Menu_HandleFontChange((struct GuiElem*)&MenuOptionsScreen_Instance);
}

static void GuiOptionsScreen_GetShowFPS(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_ShowFPS); }
static void GuiOptionsScreen_SetShowFPS(STRING_PURE String* v) { Game_ShowFPS = Menu_SetBool(v, OPT_SHOW_FPS); }

static void GuiOptionsScreen_SetScale(STRING_PURE String* v, Real32* target, const UChar* optKey) {
	*target = Menu_Real32(v);
	Options_Set(optKey, v);
	Gui_RefreshHud();
}

static void GuiOptionsScreen_GetHotbar(STRING_TRANSIENT String* v) { String_AppendReal32(v, Game_RawHotbarScale, 1); }
static void GuiOptionsScreen_SetHotbar(STRING_PURE String* v) { GuiOptionsScreen_SetScale(v, &Game_RawHotbarScale, OPT_HOTBAR_SCALE); }

static void GuiOptionsScreen_GetInventory(STRING_TRANSIENT String* v) { String_AppendReal32(v, Game_RawInventoryScale, 1); }
static void GuiOptionsScreen_SetInventory(STRING_PURE String* v) { GuiOptionsScreen_SetScale(v, &Game_RawInventoryScale, OPT_INVENTORY_SCALE); }

static void GuiOptionsScreen_GetTabAuto(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_TabAutocomplete); }
static void GuiOptionsScreen_SetTabAuto(STRING_PURE String* v) { Game_TabAutocomplete = Menu_SetBool(v, OPT_TAB_AUTOCOMPLETE); }

static void GuiOptionsScreen_GetClickable(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_ClickableChat); }
static void GuiOptionsScreen_SetClickable(STRING_PURE String* v) { Game_ClickableChat = Menu_SetBool(v, OPT_CLICKABLE_CHAT); }

static void GuiOptionsScreen_GetChatScale(STRING_TRANSIENT String* v) { String_AppendReal32(v, Game_RawChatScale, 1); }
static void GuiOptionsScreen_SetChatScale(STRING_PURE String* v) { GuiOptionsScreen_SetScale(v, &Game_RawChatScale, OPT_CHAT_SCALE); }

static void GuiOptionsScreen_GetChatlines(STRING_TRANSIENT String* v) { String_AppendInt32(v, Game_ChatLines); }
static void GuiOptionsScreen_SetChatlines(STRING_PURE String* v) {
	Game_ChatLines = Menu_Int32(v);
	Options_Set(OPT_CHATLINES, v);
	Gui_RefreshHud();
}

static void GuiOptionsScreen_GetUseFont(STRING_TRANSIENT String* v) { Menu_GetBool(v, !Drawer2D_UseBitmappedChat); }
static void GuiOptionsScreen_SetUseFont(STRING_PURE String* v) {
	Drawer2D_UseBitmappedChat = !Menu_SetBool(v, OPT_USE_CHAT_FONT);
	Menu_HandleFontChange((struct GuiElem*)&MenuOptionsScreen_Instance);
}

static void GuiOptionsScreen_GetFont(STRING_TRANSIENT String* v) { String_AppendString(v, &Game_FontName); }
static void GuiOptionsScreen_SetFont(STRING_PURE String* v) {
	String_Set(&Game_FontName, v);
	Options_Set(OPT_FONT_NAME, v);
	Menu_HandleFontChange((struct GuiElem*)&MenuOptionsScreen_Instance);
}

static void GuiOptionsScreen_ContextRecreated(void* obj) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)obj;
	struct Widget** widgets = screen->Widgets;

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

	MenuOptionsScreen_Make(screen, 5, 1, -150, "Clickable chat",     MenuOptionsScreen_Bool,
		GuiOptionsScreen_GetClickable, GuiOptionsScreen_SetClickable);
	MenuOptionsScreen_Make(screen, 6, 1, -100, "Chat scale",         MenuOptionsScreen_Input,
		GuiOptionsScreen_GetChatScale, GuiOptionsScreen_SetChatScale);
	MenuOptionsScreen_Make(screen, 7, 1,  -50, "Chat lines",         MenuOptionsScreen_Input,
		GuiOptionsScreen_GetChatlines, GuiOptionsScreen_SetChatlines);
	MenuOptionsScreen_Make(screen, 8, 1,    0, "Use system font",    MenuOptionsScreen_Bool,
		GuiOptionsScreen_GetUseFont,   GuiOptionsScreen_SetUseFont);
	OptionsGroupScreen_Make(screen, 9, 1,  50, "Select system font", Menu_SwitchFont);

	Menu_DefaultBack(screen, 10, &screen->Buttons[10], false, &screen->TitleFont, Menu_SwitchOptions);
	widgets[11] = NULL; widgets[12] = NULL; widgets[13] = NULL;
}

struct Screen* GuiOptionsScreen_MakeInstance(void) {
	static struct ButtonWidget buttons[11];
	static struct MenuInputValidator validators[Array_Elems(buttons)];
	static const UChar* defaultValues[Array_Elems(buttons)];
	static struct Widget* widgets[Array_Elems(buttons) + 3];

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
static void HacksSettingsScreen_GetHacks(STRING_TRANSIENT String* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.Enabled); }
static void HacksSettingsScreen_SetHacks(STRING_PURE String* v) {
	LocalPlayer_Instance.Hacks.Enabled = Menu_SetBool(v,OPT_HACKS_ENABLED);
	LocalPlayer_CheckHacksConsistency();
}

static void HacksSettingsScreen_GetSpeed(STRING_TRANSIENT String* v) { String_AppendReal32(v, LocalPlayer_Instance.Hacks.SpeedMultiplier, 2); }
static void HacksSettingsScreen_SetSpeed(STRING_PURE String* v) {
	LocalPlayer_Instance.Hacks.SpeedMultiplier = Menu_Real32(v);
	Options_Set(OPT_SPEED_FACTOR, v);
}

static void HacksSettingsScreen_GetClipping(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_CameraClipping); }
static void HacksSettingsScreen_SetClipping(STRING_PURE String* v) {
	Game_CameraClipping = Menu_SetBool(v, OPT_CAMERA_CLIPPING);
}

static void HacksSettingsScreen_GetJump(STRING_TRANSIENT String* v) { String_AppendReal32(v, LocalPlayer_JumpHeight(), 3); }
static void HacksSettingsScreen_SetJump(STRING_PURE String* v) {
	struct PhysicsComp* physics = &LocalPlayer_Instance.Physics;
	PhysicsComp_CalculateJumpVelocity(physics, Menu_Real32(v));
	physics->UserJumpVel = physics->JumpVel;

	UChar strBuffer[String_BufferSize(STRING_SIZE)];
	String str = String_InitAndClearArray(strBuffer);
	String_AppendReal32(&str, physics->JumpVel, 8);
	Options_Set(OPT_JUMP_VELOCITY, &str);
}

static void HacksSettingsScreen_GetWOMHacks(STRING_TRANSIENT String* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.WOMStyleHacks); }
static void HacksSettingsScreen_SetWOMHacks(STRING_PURE String* v) {
	LocalPlayer_Instance.Hacks.WOMStyleHacks = Menu_SetBool(v, OPT_WOM_STYLE_HACKS);
}

static void HacksSettingsScreen_GetFullStep(STRING_TRANSIENT String* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.FullBlockStep); }
static void HacksSettingsScreen_SetFullStep(STRING_PURE String* v) {
	LocalPlayer_Instance.Hacks.FullBlockStep = Menu_SetBool(v, OPT_FULL_BLOCK_STEP);
}

static void HacksSettingsScreen_GetPushback(STRING_TRANSIENT String* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.PushbackPlacing); }
static void HacksSettingsScreen_SetPushback(STRING_PURE String* v) {
	LocalPlayer_Instance.Hacks.PushbackPlacing = Menu_SetBool(v, OPT_PUSHBACK_PLACING);
}

static void HacksSettingsScreen_GetLiquids(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_BreakableLiquids); }
static void HacksSettingsScreen_SetLiquids(STRING_PURE String* v) {
	Game_BreakableLiquids = Menu_SetBool(v, OPT_MODIFIABLE_LIQUIDS);
}

static void HacksSettingsScreen_GetSlide(STRING_TRANSIENT String* v) { Menu_GetBool(v, LocalPlayer_Instance.Hacks.NoclipSlide); }
static void HacksSettingsScreen_SetSlide(STRING_PURE String* v) {
	LocalPlayer_Instance.Hacks.NoclipSlide = Menu_SetBool(v, OPT_NOCLIP_SLIDE);
}

static void HacksSettingsScreen_GetFOV(STRING_TRANSIENT String* v) { String_AppendInt32(v, Game_Fov); }
static void HacksSettingsScreen_SetFOV(STRING_PURE String* v) {
	Game_Fov = Menu_Int32(v);
	if (Game_ZoomFov > Game_Fov) Game_ZoomFov = Game_Fov;

	Options_Set(OPT_FIELD_OF_VIEW, v);
	Game_UpdateProjection();
}

static void HacksSettingsScreen_CheckHacksAllowed(void* obj) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)obj;
	struct Widget** widgets = screen->Widgets;
	Int32 i;

	for (i = 0; i < screen->WidgetsCount; i++) {
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

static void HacksSettingsScreen_ContextLost(void* obj) {
	MenuOptionsScreen_ContextLost(obj);
	Event_UnregisterVoid(&UserEvents_HackPermissionsChanged, obj, HacksSettingsScreen_CheckHacksAllowed);
}

static void HacksSettingsScreen_ContextRecreated(void* obj) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)obj;
	struct Widget** widgets = screen->Widgets;
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

	Menu_DefaultBack(screen, 10, &screen->Buttons[10], false, &screen->TitleFont, Menu_SwitchOptions);
	widgets[11] = NULL; widgets[12] = NULL; widgets[13] = NULL;
}

struct Screen* HacksSettingsScreen_MakeInstance(void) {
	static struct ButtonWidget buttons[11];
	static struct MenuInputValidator validators[Array_Elems(buttons)];
	static const UChar* defaultValues[Array_Elems(buttons)];
	static struct Widget* widgets[Array_Elems(buttons) + 3];

	/* TODO: Is this needed because user may not always use . for decimal point? */
	static UChar jumpHeightBuffer[String_BufferSize(STRING_INT_CHARS)];
	String jumpHeight = String_InitAndClearArray(jumpHeightBuffer);
	String_AppendReal32(&jumpHeight, 1.233f, 3);

	validators[1]    = MenuInputValidator_Real(0.10f, 50.00f);
	defaultValues[1] = "10";
	validators[3]    = MenuInputValidator_Real(0.10f, 2048.00f);
	defaultValues[3] = jumpHeightBuffer;
	validators[9]    = MenuInputValidator_Integer(1, 150);
	defaultValues[9] = "70";

	static const UChar* descs[Array_Elems(buttons)];
	descs[2] = "&eIf &fON&e, then the third person cameras will limit%"   "&etheir zoom distance if they hit a solid block.";
	descs[3] = "&eSets how many blocks high you can jump up.%"   "&eNote: You jump much higher when holding down the Speed key binding.";
	descs[7] = \
		"&eIf &fON&e, placing blocks that intersect your own position cause%" \
		"&ethe block to be placed, and you to be moved out of the way.%" \
		"&fThis is mainly useful for quick pillaring/towering.";
	descs[8] = "&eIf &fOFF&e, you will immediately stop when in noclip%"   "&emode and no movement keys are held down.";

	struct Screen* screen = MenuOptionsScreen_MakeInstance(widgets, Array_Elems(widgets), buttons,
		HacksSettingsScreen_ContextRecreated, validators, defaultValues, descs, Array_Elems(buttons));
	((struct MenuOptionsScreen*)screen)->ContextLost = HacksSettingsScreen_ContextLost;
	return screen;
}


/*########################################################################################################################*
*----------------------------------------------------MiscOptionsScreen----------------------------------------------------*
*#########################################################################################################################*/
static void MiscOptionsScreen_GetReach(STRING_TRANSIENT String* v) { String_AppendReal32(v, LocalPlayer_Instance.ReachDistance, 2); }
static void MiscOptionsScreen_SetReach(STRING_PURE String* v) { LocalPlayer_Instance.ReachDistance = Menu_Real32(v); }

static void MiscOptionsScreen_GetMusic(STRING_TRANSIENT String* v) { String_AppendInt32(v, Game_MusicVolume); }
static void MiscOptionsScreen_SetMusic(STRING_PURE String* v) {
	Game_MusicVolume = Menu_Int32(v);
	Options_Set(OPT_MUSIC_VOLUME, v);
	Audio_SetMusic(Game_MusicVolume);
}

static void MiscOptionsScreen_GetSounds(STRING_TRANSIENT String* v) { String_AppendInt32(v, Game_SoundsVolume); }
static void MiscOptionsScreen_SetSounds(STRING_PURE String* v) {
	Game_SoundsVolume = Menu_Int32(v);
	Options_Set(OPT_SOUND_VOLUME, v);
	Audio_SetSounds(Game_SoundsVolume);
}

static void MiscOptionsScreen_GetViewBob(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_ViewBobbing); }
static void MiscOptionsScreen_SetViewBob(STRING_PURE String* v) { Game_ViewBobbing = Menu_SetBool(v, OPT_VIEW_BOBBING); }

static void MiscOptionsScreen_GetPhysics(STRING_TRANSIENT String* v) { Menu_GetBool(v, Physics_Enabled); }
static void MiscOptionsScreen_SetPhysics(STRING_PURE String* v) {
	Physics_SetEnabled(Menu_SetBool(v, OPT_BLOCK_PHYSICS));
}

static void MiscOptionsScreen_GetAutoClose(STRING_TRANSIENT String* v) { Menu_GetBool(v, Options_GetBool(OPT_AUTO_CLOSE_LAUNCHER, false)); }
static void MiscOptionsScreen_SetAutoClose(STRING_PURE String* v) { Menu_SetBool(v, OPT_AUTO_CLOSE_LAUNCHER); }

static void MiscOptionsScreen_GetInvert(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_InvertMouse); }
static void MiscOptionsScreen_SetInvert(STRING_PURE String* v) { Game_InvertMouse = Menu_SetBool(v, OPT_INVERT_MOUSE); }

static void MiscOptionsScreen_GetSensitivity(STRING_TRANSIENT String* v) { String_AppendInt32(v, Game_MouseSensitivity); }
static void MiscOptionsScreen_SetSensitivity(STRING_PURE String* v) {
	Game_MouseSensitivity = Menu_Int32(v);
	Options_Set(OPT_SENSITIVITY, v);
}

static void MiscOptionsScreen_ContextRecreated(void* obj) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)obj;
	struct Widget** widgets = screen->Widgets;

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

	Menu_DefaultBack(screen, 8, &screen->Buttons[8], false, &screen->TitleFont, Menu_SwitchOptions);
	widgets[9] = NULL; widgets[10] = NULL; widgets[11] = NULL;

	/* Disable certain options */
	if (!ServerConnection_IsSinglePlayer) MenuScreen_Remove((struct MenuScreen*)screen, 0);
	if (!ServerConnection_IsSinglePlayer) MenuScreen_Remove((struct MenuScreen*)screen, 4);
}

struct Screen* MiscOptionsScreen_MakeInstance(void) {
	static struct ButtonWidget buttons[9];
	static struct MenuInputValidator validators[Array_Elems(buttons)];
	static const UChar* defaultValues[Array_Elems(buttons)];
	static struct Widget* widgets[Array_Elems(buttons) + 3];

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
static void NostalgiaScreen_GetHand(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_ClassicArmModel); }
static void NostalgiaScreen_SetHand(STRING_PURE String* v) { Game_ClassicArmModel = Menu_SetBool(v, OPT_CLASSIC_ARM_MODEL); }

static void NostalgiaScreen_GetAnim(STRING_TRANSIENT String* v) { Menu_GetBool(v, !Game_SimpleArmsAnim); }
static void NostalgiaScreen_SetAnim(STRING_PURE String* v) {
	Game_SimpleArmsAnim = String_CaselessEqualsConst(v, "OFF");
	Options_SetBool(OPT_SIMPLE_ARMS_ANIM, Game_SimpleArmsAnim);
}

static void NostalgiaScreen_GetGui(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_UseClassicGui); }
static void NostalgiaScreen_SetGui(STRING_PURE String* v) { Game_UseClassicGui = Menu_SetBool(v, OPT_USE_CLASSIC_GUI); }

static void NostalgiaScreen_GetList(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_UseClassicTabList); }
static void NostalgiaScreen_SetList(STRING_PURE String* v) { Game_UseClassicTabList = Menu_SetBool(v, OPT_USE_CLASSIC_TABLIST); }

static void NostalgiaScreen_GetOpts(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_UseClassicOptions); }
static void NostalgiaScreen_SetOpts(STRING_PURE String* v) { Game_UseClassicOptions = Menu_SetBool(v, OPT_USE_CLASSIC_OPTIONS); }

static void NostalgiaScreen_GetCustom(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_AllowCustomBlocks); }
static void NostalgiaScreen_SetCustom(STRING_PURE String* v) { Game_AllowCustomBlocks = Menu_SetBool(v, OPT_USE_CUSTOM_BLOCKS); }

static void NostalgiaScreen_GetCPE(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_UseCPE); }
static void NostalgiaScreen_SetCPE(STRING_PURE String* v) { Game_UseCPE = Menu_SetBool(v, OPT_USE_CPE); }

static void NostalgiaScreen_GetTexs(STRING_TRANSIENT String* v) { Menu_GetBool(v, Game_AllowServerTextures); }
static void NostalgiaScreen_SetTexs(STRING_PURE String* v) { Game_AllowServerTextures = Menu_SetBool(v, OPT_USE_SERVER_TEXTURES); }

static void NostalgiaScreen_SwitchBack(struct GuiElem* a, struct GuiElem* b) {
	if (Game_UseClassicOptions) { Menu_SwitchPause(a, b); } else { Menu_SwitchOptions(a, b); }
}

static void NostalgiaScreen_ContextRecreated(void* obj) {
	struct MenuOptionsScreen* screen = (struct MenuOptionsScreen*)obj;
	struct Widget** widgets = screen->Widgets;
	static struct TextWidget desc;

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


	Menu_DefaultBack(screen, 8, &screen->Buttons[8], false, &screen->TitleFont, NostalgiaScreen_SwitchBack);

	String descText = String_FromConst("&eButtons on the right require restarting game");
	Menu_Label(screen, 9, &desc, &descText, &screen->TextFont,
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
static void Overlay_Init(struct GuiElem* elem) {
	MenuScreen_Init(elem);
	if (Gfx_LostContext) return;
	struct MenuScreen* screen = (struct MenuScreen*)elem;
	screen->ContextRecreated(elem);
}

static bool Overlay_HandlesKeyDown(struct GuiElem* elem, Key key) { return true; }

static void Overlay_MakeLabels(struct MenuScreen* screen, struct TextWidget* labels, STRING_PURE String* lines) {
	Menu_Label(screen, 0, &labels[0], &lines[0], &screen->TitleFont, 
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -120);

	PackedCol col = PACKEDCOL_CONST(224, 224, 224, 255);
	Int32 i;
	for (i = 1; i < 4; i++) {
		if (!lines[i].length) continue;

		Menu_Label(screen, i, &labels[i], &lines[i], &screen->TextFont,
			ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -70 + 20 * i);
		labels[i].Col = col;
	}
}

static void Overlay_UseVTABLE(struct MenuScreen* screen, struct GuiElementVTABLE* vtable) {
	*vtable = *screen->VTABLE;
	screen->VTABLE = vtable;

	screen->VTABLE->Init = Overlay_Init;
	screen->VTABLE->HandlesKeyDown = Overlay_HandlesKeyDown;
}


/*########################################################################################################################*
*------------------------------------------------------TexIdsOverlay------------------------------------------------------*
*#########################################################################################################################*/
#define TEXID_OVERLAY_VERTICES_COUNT (ATLAS2D_TILES_PER_ROW * ATLAS2D_ROWS_COUNT * 4)
struct GuiElementVTABLE TexIdsOverlay_VTABLE;
struct TexIdsOverlay TexIdsOverlay_Instance;
static void TexIdsOverlay_ContextLost(void* obj) {
	struct TexIdsOverlay* screen = (struct TexIdsOverlay*)obj;
	MenuScreen_ContextLost(obj);
	Gfx_DeleteVb(&screen->DynamicVb);
	TextAtlas_Free(&screen->IdAtlas);
}

static void TexIdsOverlay_ContextRecreated(void* obj) {
	struct TexIdsOverlay* screen = (struct TexIdsOverlay*)obj;
	screen->DynamicVb = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FT2FC4B, TEXID_OVERLAY_VERTICES_COUNT);

	String chars = String_FromConst("0123456789");
	String prefix = String_FromConst("f");
	TextAtlas_Make(&screen->IdAtlas, &chars, &screen->TextFont, &prefix);

	Int32 size = Game_Height / ATLAS2D_TILES_PER_ROW;
	size = (size / 8) * 8;
	Math_Clamp(size, 8, 40);

	screen->XOffset = Gui_CalcPos(ANCHOR_CENTRE, 0, size * ATLAS2D_ROWS_COUNT,   Game_Width);
	screen->YOffset = Gui_CalcPos(ANCHOR_CENTRE, 0, size * ATLAS2D_TILES_PER_ROW, Game_Height);
	screen->TileSize = size;

	String title = String_FromConst("Texture ID reference sheet");
	Menu_Label(screen, 0, &screen->Title, &title, &screen->TitleFont,
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, screen->YOffset - 30);
}

static void TexIdsOverlay_RenderTerrain(struct TexIdsOverlay* screen) {
	VertexP3fT2fC4b vertices[TEXID_OVERLAY_VERTICES_COUNT];
	Int32 elemsPerAtlas = Atlas1D_TilesPerAtlas, i;
	for (i = 0; i < ATLAS2D_TILES_PER_ROW * ATLAS2D_ROWS_COUNT;) {
		VertexP3fT2fC4b* ptr = vertices;
		Int32 j, ignored, size = screen->TileSize;

		for (j = 0; j < elemsPerAtlas; j++) {
			struct TextureRec rec = Atlas1D_TexRec(i + j, 1, &ignored);
			Int32 x = (i + j) % ATLAS2D_TILES_PER_ROW;
			Int32 y = (i + j) / ATLAS2D_TILES_PER_ROW;

			struct Texture tex;
			Texture_FromRec(&tex, NULL, screen->XOffset + x * size,
				screen->YOffset + y * size, size, size, rec);

			PackedCol col = PACKEDCOL_WHITE;
			GfxCommon_Make2DQuad(&tex, col, &ptr);
		}

		Gfx_BindTexture(Atlas1D_TexIds[i / elemsPerAtlas]);
		i += elemsPerAtlas;
		Int32 count = (Int32)(ptr - vertices);
		GfxCommon_UpdateDynamicVb_IndexedTris(screen->DynamicVb, vertices, count);
	}
}

static void TexIdsOverlay_RenderTextOverlay(struct TexIdsOverlay* screen) {
	Int32 x, y, size = screen->TileSize;
	VertexP3fT2fC4b vertices[TEXID_OVERLAY_VERTICES_COUNT];
	VertexP3fT2fC4b* ptr = vertices;

	struct TextAtlas* idAtlas = &screen->IdAtlas;
	idAtlas->Tex.Y = (screen->YOffset + (size - idAtlas->Tex.Height));
	for (y = 0; y < ATLAS2D_ROWS_COUNT; y++) {
		for (x = 0; x < ATLAS2D_TILES_PER_ROW; x++) {
			idAtlas->CurX = screen->XOffset + size * x + 3; /* offset text by 3 pixels */
			Int32 id = x + y * ATLAS2D_TILES_PER_ROW;
			TextAtlas_AddInt(idAtlas, id, &ptr);
		}
		idAtlas->Tex.Y += size;

		if ((y % 4) != 3) continue;
		Gfx_BindTexture(idAtlas->Tex.ID);
		Int32 count = (Int32)(ptr - vertices);
		GfxCommon_UpdateDynamicVb_IndexedTris(screen->DynamicVb, vertices, count);
		ptr = vertices;
	}
}

static void TexIdsOverlay_Init(struct GuiElem* elem) {
	struct MenuScreen* screen = (struct MenuScreen*)elem;
	Font_Make(&screen->TextFont, &Game_FontName, 8, FONT_STYLE_NORMAL);
	Overlay_Init(elem);
}

static void TexIdsOverlay_Render(struct GuiElem* elem, Real64 delta) {
	struct TexIdsOverlay* screen = (struct TexIdsOverlay*)elem;
	Menu_RenderBounds();
	Gfx_SetTexturing(true);
	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	Menu_Render((struct MenuBase*)elem, delta);
	TexIdsOverlay_RenderTerrain(screen);
	TexIdsOverlay_RenderTextOverlay(screen);
	Gfx_SetTexturing(false);
}

static bool TexIdsOverlay_HandlesKeyDown(struct GuiElem* elem, Key key) {
	if (key == KeyBind_Get(KeyBind_IDOverlay) || key == KeyBind_Get(KeyBind_PauseOrExit)) {
		Gui_FreeOverlay((struct Screen*)elem);
		return true;
	}
	struct Screen* screen = Gui_GetUnderlyingScreen();
	return Elem_HandlesKeyDown(screen, key);
}

static bool TexIdsOverlay_HandlesKeyPress(struct GuiElem* elem, UChar key) {
	struct Screen* screen = Gui_GetUnderlyingScreen();
	return Elem_HandlesKeyPress(screen, key);
}

static bool TexIdsOverlay_HandlesKeyUp(struct GuiElem* elem, Key key) {
	struct Screen* screen = Gui_GetUnderlyingScreen();
	return Elem_HandlesKeyUp(screen, key);
}

struct Screen* TexIdsOverlay_MakeInstance(void) {
	static struct Widget* widgets[1];
	struct TexIdsOverlay* screen = &TexIdsOverlay_Instance;
	MenuScreen_MakeInstance((struct MenuScreen*)screen, widgets,
		Array_Elems(widgets), TexIdsOverlay_ContextRecreated);
	screen->ContextLost = TexIdsOverlay_ContextLost;

	TexIdsOverlay_VTABLE = *screen->VTABLE;
	screen->VTABLE = &TexIdsOverlay_VTABLE;

	screen->VTABLE->Init   = TexIdsOverlay_Init;
	screen->VTABLE->Render = TexIdsOverlay_Render;
	screen->VTABLE->HandlesKeyDown  = TexIdsOverlay_HandlesKeyDown;
	screen->VTABLE->HandlesKeyPress = TexIdsOverlay_HandlesKeyPress;
	screen->VTABLE->HandlesKeyUp    = TexIdsOverlay_HandlesKeyUp;
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*-----------------------------------------------------WarningOverlay------------------------------------------------------*
*#########################################################################################################################*/
#define WarningOverlay_MakeLeft(buttonI, widgetI, titleText, yPos) \
	Menu_Button(screen, widgetI, &buttons[buttonI], 160, titleText, &screen->TitleFont, yesClick,\
		ANCHOR_CENTRE, ANCHOR_CENTRE, -110, yPos);

#define WarningOverlay_MakeRight(buttonI, widgetI, titleText, yPos) \
	Menu_Button(screen, widgetI, &buttons[buttonI], 160, titleText, &screen->TitleFont, noClick,\
		ANCHOR_CENTRE, ANCHOR_CENTRE, 110, yPos);

static void WarningOverlay_MakeButtons(struct MenuScreen* screen, struct ButtonWidget* buttons, bool always, 
	Widget_LeftClick yesClick, Widget_LeftClick noClick) {

	String yes = String_FromConst("Yes"); 
	WarningOverlay_MakeLeft(0, 4, &yes, 30);
	String no = String_FromConst("No"); 
	WarningOverlay_MakeRight(1, 5, &no, 30);

	if (!always) return;

	String alwaysYes = String_FromConst("Always yes"); 
	WarningOverlay_MakeLeft(2, 6, &alwaysYes, 85);
	String alwaysNo = String_FromConst("Always no");  
	WarningOverlay_MakeRight(3, 7, &alwaysNo, 85);
}

bool WarningOverlay_IsAlways(struct GuiElem* screen, struct Widget* w) {
	return Menu_Index((struct MenuBase*)screen, w) >= 6;
}
struct GuiElementVTABLE WarningOverlay_VTABLE;


/*########################################################################################################################*
*----------------------------------------------------UrlWarningOverlay----------------------------------------------------*
*#########################################################################################################################*/
struct UrlWarningOverlay UrlWarningOverlay_Instance;
static void UrlWarningOverlay_OpenUrl(struct GuiElem* elem, struct GuiElem* widget) {
	Gui_FreeOverlay((struct Screen*)elem);
	struct UrlWarningOverlay* screen = (struct UrlWarningOverlay*)elem;
	String url = String_FromRawArray(screen->UrlBuffer);
	Platform_StartShell(&url);
}

static void UrlWarningOverlay_AppendUrl(struct GuiElem* elem, struct GuiElem* widget) {
	Gui_FreeOverlay((struct Screen*)elem);
	if (Game_ClickableChat) {
		struct UrlWarningOverlay* screen = (struct UrlWarningOverlay*)elem;
		String url = String_FromRawArray(screen->UrlBuffer);
		HUDScreen_AppendInput(Gui_HUD, &url);
	}
}

static void UrlWarningOverlay_ContextRecreated(void* obj) {
	struct UrlWarningOverlay* screen = (struct UrlWarningOverlay*)obj;
	String lines[4];
	lines[0] = String_FromReadonly("&eAre you sure you want to open this link?");
	lines[1] = String_FromRawArray(screen->UrlBuffer);
	lines[2] = String_FromReadonly("Be careful - links from strangers may be websites that");
	lines[3] = String_FromReadonly(" have viruses, or things you may not want to open/see.");
	Overlay_MakeLabels((struct MenuScreen*)screen, screen->Labels, lines);

	WarningOverlay_MakeButtons((struct MenuScreen*)screen, screen->Buttons, false,
		UrlWarningOverlay_OpenUrl, UrlWarningOverlay_AppendUrl);
}

struct Screen* UrlWarningOverlay_MakeInstance(STRING_PURE String* url) {
	static struct Widget* widgets[6];
	struct UrlWarningOverlay* screen = &UrlWarningOverlay_Instance;
	MenuScreen_MakeInstance((struct MenuScreen*)screen, widgets,
		Array_Elems(widgets), UrlWarningOverlay_ContextRecreated);

	String dstUrl = String_InitAndClearArray(screen->UrlBuffer);
	String_Set(&dstUrl, url);

	Overlay_UseVTABLE((struct MenuScreen*)screen, &WarningOverlay_VTABLE);
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*----------------------------------------------------ConfirmDenyOverlay---------------------------------------------------*
*#########################################################################################################################*/
struct ConfirmDenyOverlay ConfirmDenyOverlay_Instance;
static void ConfirmDenyOverlay_ConfirmNoClick(struct GuiElem* elem, struct GuiElem* widget) {
	Gui_FreeOverlay((struct Screen*)elem);
	struct ConfirmDenyOverlay* screen = (struct ConfirmDenyOverlay*)elem;
	String url = String_FromRawArray(screen->UrlBuffer);

	if (screen->AlwaysDeny && !TextureCache_HasDenied(&url)) {
		TextureCache_Deny(&url);
	}
}

static void ConfirmDenyOverlay_GoBackClick(struct GuiElem* elem, struct GuiElem* widget) {
	Gui_FreeOverlay((struct Screen*)elem);
	struct ConfirmDenyOverlay* screen = (struct ConfirmDenyOverlay*)elem;
	String url = String_FromRawArray(screen->UrlBuffer);

	struct Screen* overlay = TexPackOverlay_MakeInstance(&url);
	Gui_ShowOverlay(overlay, true);
}

static void ConfirmDenyOverlay_ContextRecreated(void* obj) {
	struct ConfirmDenyOverlay* screen = (struct ConfirmDenyOverlay*)obj;
	String lines[4];
	lines[0] = String_FromReadonly("&eYou might be missing out.");
	lines[1] = String_FromReadonly("Texture packs can play a vital role in the look and feel of maps.");
	lines[2] = String_MakeNull();
	lines[3] = String_FromReadonly("Sure you don't want to download the texture pack?");

	Overlay_MakeLabels((struct MenuScreen*)screen, screen->Labels, lines);
	struct ButtonWidget* buttons = screen->Buttons;
	Widget_LeftClick yesClick = ConfirmDenyOverlay_ConfirmNoClick;
	Widget_LeftClick noClick  = ConfirmDenyOverlay_GoBackClick;

	String imSure = String_FromConst("I'm sure");
	WarningOverlay_MakeLeft(0, 4, &imSure, 30);
	String goBack = String_FromConst("Go back");
	WarningOverlay_MakeRight(1, 5, &goBack, 30);
}

struct Screen* ConfirmDenyOverlay_MakeInstance(STRING_PURE String* url, bool alwaysDeny) {
	static struct Widget* widgets[6];
	struct ConfirmDenyOverlay* screen = &ConfirmDenyOverlay_Instance;
	MenuScreen_MakeInstance((struct MenuScreen*)screen, widgets,
		Array_Elems(widgets), ConfirmDenyOverlay_ContextRecreated);

	String dstUrl = String_InitAndClearArray(screen->UrlBuffer);
	String_Set(&dstUrl, url);
	screen->AlwaysDeny = alwaysDeny;

	Overlay_UseVTABLE((struct MenuScreen*)screen, &WarningOverlay_VTABLE);
	return (struct Screen*)screen;
}


/*########################################################################################################################*
*-----------------------------------------------------TexPackOverlay------------------------------------------------------*
*#########################################################################################################################*/
struct TexPackOverlay TexPackOverlay_Instance;
struct GuiElementVTABLE TexPackOverlay_VTABLE;
static void TexPackOverlay_YesClick(struct GuiElem* elem, struct GuiElem* widget) {
	Gui_FreeOverlay((struct Screen*)elem);
	struct TexPackOverlay* screen = (struct TexPackOverlay*)elem;
	String url = String_FromRawArray(screen->IdentifierBuffer);
	url = String_UNSAFE_SubstringAt(&url, 3);

	ServerConnection_DownloadTexturePack(&url);
	bool isAlways = WarningOverlay_IsAlways(elem, (struct Widget*)widget);
	if (isAlways && !TextureCache_HasAccepted(&url)) {
		TextureCache_Accept(&url);
	}
}

static void TexPackOverlay_NoClick(struct GuiElem* elem, struct GuiElem* widget) {
	Gui_FreeOverlay((struct Screen*)elem);
	struct TexPackOverlay* screen = (struct TexPackOverlay*)elem;
	String url = String_FromRawArray(screen->IdentifierBuffer);
	url = String_UNSAFE_SubstringAt(&url, 3);

	bool isAlways = WarningOverlay_IsAlways(elem, (struct Widget*)widget);
	struct Screen* overlay = ConfirmDenyOverlay_MakeInstance(&url, isAlways);
	Gui_ShowOverlay(overlay, true);
}

static void TexPackOverlay_Render(struct GuiElem* elem, Real64 delta) {
	struct TexPackOverlay* screen = (struct TexPackOverlay*)elem;
	String identifier = String_FromRawArray(screen->IdentifierBuffer);

	MenuScreen_Render(elem, delta);
	struct AsyncRequest item;
	if (!AsyncDownloader_Get(&identifier, &item)) return;

	screen->ContentLength = item.ResultSize;
	if (!screen->ContentLength) return;

	screen->ContextLost(elem);
	screen->ContextRecreated(elem);
}

static void TexPackOverlay_ContextRecreated(void* obj) {
	struct TexPackOverlay* screen = (struct TexPackOverlay*)obj;
	String url = String_FromRawArray(screen->IdentifierBuffer);
	url = String_UNSAFE_SubstringAt(&url, 3);

	String https = String_FromConst("https://");
	if (String_CaselessStarts(&url, &https)) {
		url = String_UNSAFE_SubstringAt(&url, https.length);
	}

	String http = String_FromConst("http://");
	if (String_CaselessStarts(&url, &http)) {
		url = String_UNSAFE_SubstringAt(&url, http.length);
	}

	String lines[4];
	lines[0] = String_FromReadonly("Do you want to download the server's texture pack?");
	lines[1] = String_FromReadonly("Texture pack url:");
	lines[2] = url;

	if (!screen->ContentLength) {
		lines[3] = String_FromReadonly("Download size: Determining...");
	} else {
		UChar contentsBuffer[String_BufferSize(STRING_SIZE)];
		lines[3] = String_InitAndClearArray(contentsBuffer);
		Real32 contentLengthMB = screen->ContentLength / (1024.0f * 1024.0f);
		String_Format1(&lines[3], "Download size: %f3 MB", &contentLengthMB);
	}

	Overlay_MakeLabels((struct MenuScreen*)screen, screen->Labels, lines);
	WarningOverlay_MakeButtons((struct MenuScreen*)screen, screen->Buttons, true,
		TexPackOverlay_YesClick, TexPackOverlay_NoClick);
}

struct Screen* TexPackOverlay_MakeInstance(STRING_PURE String* url) {
	/* If we are showing a texture pack overlay, completely free that overlay */
	/* - it doesn't matter anymore, because the new texture pack URL will always */
	/* replace/override the old texture pack URL associated with that overlay */
	struct Screen* elem = (struct Screen*)(&TexPackOverlay_Instance);
	if (Gui_IndexOverlay(elem) >= 0) { Gui_FreeOverlay(elem); }
	elem = (struct Screen*)(&ConfirmDenyOverlay_Instance);
	if (Gui_IndexOverlay(elem) >= 0) { Gui_FreeOverlay(elem); }

	static struct Widget* widgets[8];
	struct TexPackOverlay* screen = &TexPackOverlay_Instance;
	MenuScreen_MakeInstance((struct MenuScreen*)screen, widgets,
		Array_Elems(widgets), TexPackOverlay_ContextRecreated);

	String identifier = String_InitAndClearArray(screen->IdentifierBuffer);
	String_AppendConst(&identifier, "CL_");
	String_AppendString(&identifier, url);
	screen->ContentLength = 0;

	AsyncDownloader_GetContentLength(url, true, &identifier);
	Overlay_UseVTABLE((struct MenuScreen*)screen, &TexPackOverlay_VTABLE);
	screen->VTABLE->Render = TexPackOverlay_Render;
	return (struct Screen*)screen;
}
