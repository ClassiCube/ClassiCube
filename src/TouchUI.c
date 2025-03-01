#include "Screens.h"
#ifdef CC_BUILD_TOUCH
#include "Widgets.h"
#include "Game.h"
#include "Event.h"
#include "Platform.h"
#include "Inventory.h"
#include "Drawer2D.h"
#include "Graphics.h"
#include "Funcs.h"
#include "TexturePack.h"
#include "Model.h"
#include "Generator.h"
#include "Server.h"
#include "Chat.h"
#include "ExtMath.h"
#include "Window.h"
#include "Camera.h"
#include "Http.h"
#include "Block.h"
#include "Menus.h"
#include "World.h"
#include "Input.h"
#include "Utils.h"
#include "Options.h"
#include "InputHandler.h"


/* Enumeration of on-screen buttons for touch GUI */
#define ONSCREEN_BTN_CHAT      (1 << 0)
#define ONSCREEN_BTN_LIST      (1 << 1)
#define ONSCREEN_BTN_SPAWN     (1 << 2)
#define ONSCREEN_BTN_SETSPAWN  (1 << 3)
#define ONSCREEN_BTN_FLY       (1 << 4)
#define ONSCREEN_BTN_NOCLIP    (1 << 5)
#define ONSCREEN_BTN_SPEED     (1 << 6)
#define ONSCREEN_BTN_HALFSPEED (1 << 7)
#define ONSCREEN_BTN_CAMERA    (1 << 8)
#define ONSCREEN_BTN_DELETE    (1 << 9)
#define ONSCREEN_BTN_PICK      (1 << 10)
#define ONSCREEN_BTN_PLACE     (1 << 11)
#define ONSCREEN_BTN_SWITCH    (1 << 12)
#define ONSCREEN_MAX_BTNS 13

static int GetOnscreenButtons(void) {
	#define DEFAULT_SP_ONSCREEN (ONSCREEN_BTN_FLY | ONSCREEN_BTN_SPEED)
	#define DEFAULT_MP_ONSCREEN (ONSCREEN_BTN_FLY | ONSCREEN_BTN_SPEED | ONSCREEN_BTN_CHAT)
	
	return Options_GetInt(OPT_TOUCH_BUTTONS, 0, Int32_MaxValue,
							Server.IsSinglePlayer ? DEFAULT_SP_ONSCREEN : DEFAULT_MP_ONSCREEN);
}

static int GetOnscreenHAligns(void) {
	return Options_GetInt(OPT_TOUCH_HALIGN, 0, Int32_MaxValue, 0);
}

/*########################################################################################################################*
*---------------------------------------------------TouchControlsScreen---------------------------------------------------*
*#########################################################################################################################*/
#define ONSCREEN_PAGE_BTNS 4
#define ONSCREEN_NUM_PAGES 4
static struct TouchOnscreenScreen {
	Screen_Body
	struct ButtonWidget back, left, right;
	struct ButtonWidget btns[ONSCREEN_PAGE_BTNS];
	struct FontDesc font;
	int page;
} TouchOnscreenScreen;

static struct Widget* touchOnscreen_widgets[3 + ONSCREEN_PAGE_BTNS];

static const char* const touchOnscreen[ONSCREEN_PAGE_BTNS * ONSCREEN_NUM_PAGES] = {
	"Chat", "Tablist", "Spawn", "Set spawn",
	"Fly",  "Noclip",  "Speed", "Half speed",
	"Third person", "Delete", "Pick", "Place",
	"Switch hotbar", "---", "---", "---",
};

static void TouchOnscreen_UpdateButton(struct TouchOnscreenScreen* s, struct ButtonWidget* btn) {
	PackedCol grey = PackedCol_Make(0x7F, 0x7F, 0x7F, 0xFF);
	int buttons = GetOnscreenButtons();
	int haligns = GetOnscreenHAligns();
	const char* label;
	char buffer[64];
	cc_string str;
	int bit;

	bit        = 1 << btn->meta.val;
	btn->color = (buttons & bit) ? PACKEDCOL_WHITE : grey;
	
	String_InitArray(str, buffer);
	label = touchOnscreen[btn->meta.val];
	
	if ((buttons & bit) && (haligns & bit)) {
		String_Format1(&str, "%c: Left aligned",  label);
	} else if (buttons & bit) {
		String_Format1(&str, "%c: Right aligned", label);
	} else {
		String_AppendConst(&str, label);
	}
	ButtonWidget_Set(btn, &str, &s->font);
}

static void TouchOnscreen_UpdateAll(struct TouchOnscreenScreen* s) {
	int i;
	for (i = 0; i < ONSCREEN_PAGE_BTNS; i++) 
	{
		TouchOnscreen_UpdateButton(s, &s->btns[i]);
	}
}

static void TouchOnscreen_Any(void* screen, void* w) {
	struct TouchOnscreenScreen* s = (struct TouchOnscreenScreen*)screen;
	struct ButtonWidget* btn      = (struct ButtonWidget*)w;
	int buttons = GetOnscreenButtons();
	int haligns = GetOnscreenHAligns();
	int bit = 1 << btn->meta.val;
	
	if ((buttons & bit) & (haligns & bit)) {
		buttons &= ~bit;
		haligns &= ~bit;
	} else if (buttons & bit) {
		haligns |= bit;
	} else {
		buttons |= bit;
	}

	Options_SetInt(OPT_TOUCH_BUTTONS, buttons);
	Options_SetInt(OPT_TOUCH_HALIGN,  haligns);
	TouchOnscreen_UpdateButton(s, btn);
	TouchScreen_Refresh();
}
static void TouchOnscreen_More(void* s, void* w) { TouchCtrlsScreen_Show(); }

static void TouchOnscreen_Left(void* screen,  void* b);
static void TouchOnscreen_Right(void* screen, void* b);

static void TouchOnscreen_RemakeWidgets(struct TouchOnscreenScreen* s) {
	int i;
	int offset    = s->page * ONSCREEN_PAGE_BTNS;
	s->widgets    = touchOnscreen_widgets;
	s->numWidgets = 0;
	s->maxWidgets = Array_Elems(touchOnscreen_widgets);

	for (i = 0; i < ONSCREEN_PAGE_BTNS; i++) 
	{
		ButtonWidget_Add(s, &s->btns[i], 300, TouchOnscreen_Any);
		s->btns[i].meta.val = i + offset;
	}
	ButtonWidget_Add(s, &s->back, 400, TouchOnscreen_More);
	ButtonWidget_Add(s, &s->left,  40, TouchOnscreen_Left);
	ButtonWidget_Add(s, &s->right, 40, TouchOnscreen_Right);

	Widget_SetDisabled(&s->left,  s->page == 0);
	Widget_SetDisabled(&s->right, s->page == ONSCREEN_NUM_PAGES - 1);
}

static void TouchOnscreen_Left(void* screen, void* b) {
	struct TouchOnscreenScreen* s = (struct TouchOnscreenScreen*)screen;
	s->page--;
	TouchOnscreen_RemakeWidgets(s);
	Gui_Refresh((struct Screen*)s);
	TouchOnscreen_UpdateAll(s);
}

static void TouchOnscreen_Right(void* screen, void* b) {
	struct TouchOnscreenScreen* s = (struct TouchOnscreenScreen*)screen;
	s->page++;
	TouchOnscreen_RemakeWidgets(s);
	Gui_Refresh((struct Screen*)s);
	TouchOnscreen_UpdateAll(s);
}

static void TouchOnscreenScreen_ContextRecreated(void* screen) {
	struct TouchOnscreenScreen* s = (struct TouchOnscreenScreen*)screen;
	Screen_UpdateVb(screen);
	TouchOnscreen_UpdateAll(s);
	ButtonWidget_SetConst(&s->back,  "Done", &s->font);
	ButtonWidget_SetConst(&s->left,  "<",    &s->font);
	ButtonWidget_SetConst(&s->right, ">",    &s->font);
}

static void TouchOnscreenScreen_Layout(void* screen) {
	struct TouchOnscreenScreen* s = (struct TouchOnscreenScreen*)screen;
	int i;
	for (i = 0; i < ONSCREEN_PAGE_BTNS; i++) 
	{
		Widget_SetLocation(&s->btns[i], ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -75 + i * 50);
	}
	
	Menu_LayoutBack(&s->back);
	Widget_SetLocation(&s->left,  ANCHOR_CENTRE, ANCHOR_CENTRE, -220, 0);
	Widget_SetLocation(&s->right, ANCHOR_CENTRE, ANCHOR_CENTRE,  220, 0);
}

static void TouchOnscreenScreen_Init(void* screen) {
	struct TouchOnscreenScreen* s = (struct TouchOnscreenScreen*)screen;
	s->page = 0;
	Gui_MakeTitleFont(&s->font);
	TouchOnscreen_RemakeWidgets(s);
	TouchOnscreen_UpdateAll(screen);

	s->maxVertices = Screen_CalcDefaultMaxVertices(s);
}

static void TouchOnscreenScreen_Free(void* screen) {
	struct TouchOnscreenScreen* s = (struct TouchOnscreenScreen*)screen;
	Font_Free(&s->font);
}

static const struct ScreenVTABLE TouchOnscreenScreen_VTABLE = {
	TouchOnscreenScreen_Init,   Screen_NullUpdate, TouchOnscreenScreen_Free,
	MenuScreen_Render2,         Screen_BuildMesh,
	Menu_InputDown,             Screen_InputUp,    Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,           Screen_PointerUp,  Menu_PointerMove, Screen_TMouseScroll,
	TouchOnscreenScreen_Layout, Screen_ContextLost, TouchOnscreenScreen_ContextRecreated
};
void TouchOnscreenScreen_Show(void) {
	struct TouchOnscreenScreen* s = &TouchOnscreenScreen;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE     = &TouchOnscreenScreen_VTABLE;

	Gui_Add((struct Screen*)s, GUI_PRIORITY_TOUCHMORE);
}


/*########################################################################################################################*
*---------------------------------------------------TouchControlsScreen---------------------------------------------------*
*#########################################################################################################################*/
#define TOUCHCTRLS_BTNS 5
static struct TouchCtrlsScreen {
	Screen_Body
	struct ButtonWidget back;
	struct ButtonWidget btns[TOUCHCTRLS_BTNS];
	struct FontDesc font;
} TouchCtrlsScreen;

static struct Widget* touchCtrls_widgets[TOUCHCTRLS_BTNS + 1];

static const char* GetTapDesc(int mode) {
	if (mode == INPUT_MODE_PLACE)  return "Tap: Place";
	if (mode == INPUT_MODE_DELETE) return "Tap: Delete";
	return "Tap: None";
}
static void TouchCtrls_UpdateTapText(void* screen) {
	struct TouchCtrlsScreen* s = (struct TouchCtrlsScreen*)screen;
	ButtonWidget_SetConst(&s->btns[0], GetTapDesc(Input_TapMode),  &s->font);
	s->dirty = true;
}

static const char* GetHoldDesc(int mode) {
	if (mode == INPUT_MODE_PLACE)  return "Hold: Place";
	if (mode == INPUT_MODE_DELETE) return "Hold: Delete";
	return "Hold: None";
}
static void TouchCtrls_UpdateHoldText(void* screen) {
	struct TouchCtrlsScreen* s = (struct TouchCtrlsScreen*)screen;
	ButtonWidget_SetConst(&s->btns[1], GetHoldDesc(Input_HoldMode), &s->font);
	s->dirty = true;
}

static void TouchCtrls_UpdateSensitivity(void* screen) {
	cc_string value; char valueBuffer[STRING_SIZE];
	struct TouchCtrlsScreen* s = (struct TouchCtrlsScreen*)screen;
	String_InitArray(value, valueBuffer);

	String_Format1(&value, "Sensitivity: %i", &Camera.Sensitivity);
	ButtonWidget_Set(&s->btns[2], &value, &s->font);
	s->dirty = true;
}

static void TouchCtrls_UpdateScale(void* screen) {
	cc_string value; char valueBuffer[STRING_SIZE];
	struct TouchCtrlsScreen* s = (struct TouchCtrlsScreen*)screen;
	String_InitArray(value, valueBuffer);

	String_AppendConst(&value, "Scale: ");
	String_AppendFloat(&value, Gui.RawTouchScale, 1);
	ButtonWidget_Set(&s->btns[3], &value, &s->font);
	s->dirty = true;
}

static void TouchCtrls_More(void* s,     void* w) { TouchMoreScreen_Show(); }
static void TouchCtrls_Onscreen(void* s, void* w) { TouchOnscreenScreen_Show(); }

static void TouchCtrls_Tap(void* s, void* w) {
	Input_TapMode  = (Input_TapMode  + 1) % INPUT_MODE_COUNT;
	TouchCtrls_UpdateTapText(s);
}
static void TouchCtrls_Hold(void* s, void* w) {
	Input_HoldMode = (Input_HoldMode + 1) % INPUT_MODE_COUNT;
	TouchCtrls_UpdateHoldText(s);
}

static void TouchCtrls_SensitivityDone(const cc_string* value, cc_bool valid) {
	int sensitivity;
	if (!valid) return;
	
	Convert_ParseInt(value, &sensitivity);
	Camera.Sensitivity = sensitivity;
	Options_Set(OPT_SENSITIVITY, value);
	TouchCtrls_UpdateSensitivity(&TouchCtrlsScreen);
}

static void TouchCtrls_Sensitivity(void* screen, void* w) {
	struct TouchCtrlsScreen* s = (struct TouchCtrlsScreen*)screen;
	static struct MenuInputDesc desc;
	cc_string value; char valueBuffer[STRING_SIZE];
	String_InitArray(value, valueBuffer);

	MenuInput_Int(desc, 1, 200, 30);
	String_AppendInt(&value, Camera.Sensitivity);
	MenuInputOverlay_Show(&desc, &value, TouchCtrls_SensitivityDone, true);
	/* Fix Sensitivity button getting stuck as 'active' */
	/* (input overlay swallows subsequent pointer events) */
	s->btns[2].active = 0;
}

static void TouchCtrls_ScaleDone(const cc_string* value, cc_bool valid) {
	if (!valid) return;
	Convert_ParseFloat(value, &Gui.RawTouchScale);
	Options_Set(OPT_TOUCH_SCALE, value);
	
	TouchCtrls_UpdateScale(&TouchCtrlsScreen);
	Gui_LayoutAll();
}

static void TouchCtrls_Scale(void* screen, void* w) {
	struct TouchCtrlsScreen* s = (struct TouchCtrlsScreen*)screen;
	static struct MenuInputDesc desc;
	cc_string value; char valueBuffer[STRING_SIZE];
	String_InitArray(value, valueBuffer);

	MenuInput_Float(desc, 0.25f, 5.0f, 1.0f);
	String_AppendFloat(&value, Gui.RawTouchScale, 1);
	MenuInputOverlay_Show(&desc, &value, TouchCtrls_ScaleDone, true);
	s->btns[3].active = 0;
}

static const struct SimpleButtonDesc touchCtrls_btns[5] = {
	{ -102,  -50, "",     TouchCtrls_Tap  },
	{  102,  -50, "",     TouchCtrls_Hold },
	{ -102,    0, "",     TouchCtrls_Sensitivity },
	{  102,    0, "",     TouchCtrls_Scale },
	{    0,   50, "On-screen controls", TouchCtrls_Onscreen }
};

static void TouchCtrlsScreen_ContextLost(void* screen) {
	struct TouchCtrlsScreen* s = (struct TouchCtrlsScreen*)screen;
	Font_Free(&s->font);
	Screen_ContextLost(screen);
}

static void TouchCtrlsScreen_ContextRecreated(void* screen) {
	struct TouchCtrlsScreen* s = (struct TouchCtrlsScreen*)screen;
	Gui_MakeTitleFont(&s->font);
	Screen_UpdateVb(screen);
	Menu_SetButtons(s->btns, &s->font, touchCtrls_btns, TOUCHCTRLS_BTNS);
	ButtonWidget_SetConst(&s->back, "Done", &s->font);

	TouchCtrls_UpdateTapText(s);
	TouchCtrls_UpdateHoldText(s);
	TouchCtrls_UpdateSensitivity(s);
	TouchCtrls_UpdateScale(s);
}

static void TouchCtrlsScreen_Layout(void* screen) {
	struct TouchCtrlsScreen* s = (struct TouchCtrlsScreen*)screen;
	Menu_LayoutButtons(s->btns, touchCtrls_btns, TOUCHCTRLS_BTNS);
	Menu_LayoutBack(&s->back);
}

static void TouchCtrlsScreen_Init(void* screen) {
	struct TouchCtrlsScreen* s = (struct TouchCtrlsScreen*)screen;
	s->widgets     = touchCtrls_widgets;
	s->numWidgets  = 0;
	s->maxWidgets  = Array_Elems(touchCtrls_widgets);

	Menu_AddButtons(s,  s->btns,     195, touchCtrls_btns,     4);
	Menu_AddButtons(s,  s->btns + 4, 400, touchCtrls_btns + 4, 1);
	ButtonWidget_Add(s, &s->back,    400, TouchCtrls_More);

	s->maxVertices = Screen_CalcDefaultMaxVertices(s);
}

static const struct ScreenVTABLE TouchCtrlsScreen_VTABLE = {
	TouchCtrlsScreen_Init,   Screen_NullUpdate, Screen_NullFunc,
	MenuScreen_Render2,      Screen_BuildMesh,
	Menu_InputDown,          Screen_InputUp,    Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,        Screen_PointerUp,  Menu_PointerMove, Screen_TMouseScroll,
	TouchCtrlsScreen_Layout, TouchCtrlsScreen_ContextLost, TouchCtrlsScreen_ContextRecreated
};
void TouchCtrlsScreen_Show(void) {
	struct TouchCtrlsScreen* s = &TouchCtrlsScreen;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE     = &TouchCtrlsScreen_VTABLE;

	Gui_Add((struct Screen*)s, GUI_PRIORITY_TOUCHMORE);
}


/*########################################################################################################################*
*-----------------------------------------------------TouchMoreScreen-----------------------------------------------------*
*#########################################################################################################################*/
#define TOUCHMORE_BTNS 6
static struct TouchMoreScreen {
	Screen_Body
	struct ButtonWidget back;
	struct ButtonWidget btns[TOUCHMORE_BTNS];
} TouchMoreScreen;

static struct Widget* touchMore_widgets[TOUCHMORE_BTNS + 1];

static void TouchMore_Take(void* s, void* w) {
	Gui_Remove((struct Screen*)&TouchMoreScreen);
	Game_ScreenshotRequested = true;
}
static void TouchMore_Screen(void* s, void* w) {
	Gui_Remove((struct Screen*)&TouchMoreScreen);
	Game_ToggleFullscreen();
}
static void TouchMore_Ctrls(void* s, void* w) { TouchCtrlsScreen_Show(); }
static void TouchMore_Menu(void* s, void* w) {
	Gui_Remove((struct Screen*)&TouchMoreScreen);
	Gui_ShowPauseMenu();
}
static void TouchMore_Game(void* s, void* w) { 
	Gui_Remove((struct Screen*)&TouchMoreScreen);
}
static void TouchMore_Chat(void* s, void* w) {
	Gui_Remove((struct Screen*)&TouchMoreScreen);
	ChatScreen_OpenInput(&String_Empty);
}
static void TouchMore_Fog(void* s, void* w) { Game_CycleViewDistance(); }

static const struct SimpleButtonDesc touchMore_btns[TOUCHMORE_BTNS] = {
	{ -102, -50, "Screenshot", TouchMore_Take   },
	{ -102,   0, "Fullscreen", TouchMore_Screen },
	{  102, -50, "Chat",       TouchMore_Chat   },
	{  102,   0, "Fog",        TouchMore_Fog    },
	{    0,  50, "Controls",   TouchMore_Ctrls  },
	{    0, 100, "Main menu",  TouchMore_Menu   }
};

static void TouchMoreScreen_ContextRecreated(void* screen) {
	struct TouchMoreScreen* s = (struct TouchMoreScreen*)screen;
	struct FontDesc titleFont;
	Gui_MakeTitleFont(&titleFont);
	Screen_UpdateVb(screen);

	Menu_SetButtons(s->btns, &titleFont, touchMore_btns, TOUCHMORE_BTNS);
	ButtonWidget_SetConst(&s->back, "Back to game", &titleFont);
	Font_Free(&titleFont);
}

static void TouchMoreScreen_Layout(void* screen) {
	struct TouchMoreScreen* s = (struct TouchMoreScreen*)screen;
	Menu_LayoutButtons(s->btns, touchMore_btns, TOUCHMORE_BTNS);
	Menu_LayoutBack(&s->back);
}

static void TouchMoreScreen_Init(void* screen) {
	struct TouchMoreScreen* s = (struct TouchMoreScreen*)screen;
	s->widgets     = touchMore_widgets;
	s->numWidgets  = 0;
	s->maxWidgets  = Array_Elems(touchMore_widgets);

	Menu_AddButtons(s,  s->btns,     195, touchMore_btns,     4);
	Menu_AddButtons(s,  s->btns + 4, 400, touchMore_btns + 4, 2);
	ButtonWidget_Add(s, &s->back,    400, TouchMore_Game);

	s->maxVertices = Screen_CalcDefaultMaxVertices(s);
}

static const struct ScreenVTABLE TouchMoreScreen_VTABLE = {
	TouchMoreScreen_Init,   Screen_NullUpdate, Screen_NullFunc,
	MenuScreen_Render2,     Screen_BuildMesh,
	Menu_InputDown,         Screen_InputUp,    Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,       Screen_PointerUp,  Menu_PointerMove, Screen_TMouseScroll,
	TouchMoreScreen_Layout, Screen_ContextLost, TouchMoreScreen_ContextRecreated
};
void TouchMoreScreen_Show(void) {
	struct TouchMoreScreen* s = &TouchMoreScreen;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE     = &TouchMoreScreen_VTABLE;

	Gui_Add((struct Screen*)s, GUI_PRIORITY_TOUCHMORE);
}


/*########################################################################################################################*
*--------------------------------------------------------TouchScreen------------------------------------------------------*
*#########################################################################################################################*/
#define TOUCH_EXTRA_BTNS 2
#define TOUCH_MAX_BTNS (ONSCREEN_MAX_BTNS + TOUCH_EXTRA_BTNS + 1)
struct TouchButtonDesc {
	const char* text;
	cc_uint8 bind, x, y;
	Widget_LeftClick OnClick;
	cc_bool* enabled;
};

static struct TouchScreen {
	Screen_Body
	int numOnscreen, numBtns;
	struct FontDesc font;
	struct ThumbstickWidget thumbstick;
	struct ButtonWidget onscreen[ONSCREEN_MAX_BTNS];
	struct ButtonWidget btns[TOUCH_EXTRA_BTNS], more;
} TouchScreen;

static struct Widget* touch_widgets[ONSCREEN_MAX_BTNS + TOUCH_EXTRA_BTNS + 2] = {
	NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL, NULL,
	NULL,NULL, (struct Widget*)&TouchScreen.thumbstick, (struct Widget*)&TouchScreen.more
};
#define TOUCH_MAX_VERTICES (THUMBSTICKWIDGET_MAX + TOUCH_MAX_BTNS * BUTTONWIDGET_MAX)

static void TouchScreen_ChatClick(void* s,     void* w) { ChatScreen_OpenInput(&String_Empty); }
static void TouchScreen_MoreClick(void* s,     void* w) { TouchMoreScreen_Show(); }
static void TouchScreen_SwitchClick(void* s,   void* w) { Inventory_SwitchHotbar(); }

static void TouchScreen_TabClick(void* s, void* w) {
	struct Screen* tablist = Gui_GetScreen(GUI_PRIORITY_TABLIST);
	if (tablist) {
		Gui_Remove(tablist);
	} else {
		TabListOverlay_Show(true);
	}
}

static void TouchScreen_BindClick(void* screen, void* widget) {
	struct ButtonWidget* btn     = (struct ButtonWidget*)widget;
	struct TouchButtonDesc* desc = (struct TouchButtonDesc*)btn->meta.ptr;

	if (!Bind_OnTriggered[desc->bind]) return;
	Bind_OnTriggered[desc->bind](0, &TouchDevice);
}

static struct TouchButtonDesc onscreenDescs[ONSCREEN_MAX_BTNS] = {
	{ "Chat",      0,                 0,0, TouchScreen_ChatClick },
	{ "Tablist",   0,                 0,0, TouchScreen_TabClick },
	{ "Respawn",   BIND_RESPAWN,      0,0, TouchScreen_BindClick, &LocalPlayer_Instances[0].Hacks.CanRespawn },
	{ "Set spawn", BIND_SET_SPAWN,    0,0, TouchScreen_BindClick, &LocalPlayer_Instances[0].Hacks.CanRespawn },
	{ "Fly",       BIND_FLY,          0,0, TouchScreen_BindClick, &LocalPlayer_Instances[0].Hacks.CanFly     },
	{ "Noclip",    BIND_NOCLIP,       0,0, TouchScreen_BindClick, &LocalPlayer_Instances[0].Hacks.CanNoclip  },
	{ "Speed",     BIND_SPEED,        0,0, TouchScreen_BindClick, &LocalPlayer_Instances[0].Hacks.CanSpeed   },
	{ "\xabSpeed", BIND_HALF_SPEED,   0,0, TouchScreen_BindClick, &LocalPlayer_Instances[0].Hacks.CanSpeed   },
	{ "Camera",    BIND_THIRD_PERSON, 0,0, TouchScreen_BindClick, &LocalPlayer_Instances[0].Hacks.CanUseThirdPerson },
	{ "Delete",    BIND_DELETE_BLOCK, 0,0, TouchScreen_BindClick },
	{ "Pick",      BIND_PICK_BLOCK,   0,0, TouchScreen_BindClick },
	{ "Place",     BIND_PLACE_BLOCK,  0,0, TouchScreen_BindClick },
	{ "Hotbar",    0,                 0,0, TouchScreen_SwitchClick }
};
static struct TouchButtonDesc normDescs[1] = {
	{ "\x1E", BIND_JUMP,     50,  10, TouchScreen_BindClick }
};
static struct TouchButtonDesc hackDescs[2] = {
	{ "\x1E", BIND_FLY_UP,   50,  70, TouchScreen_BindClick },
	{ "\x1F", BIND_FLY_DOWN, 50,  10, TouchScreen_BindClick }
};

#define TOUCHSCREEN_BTN_COLOR PackedCol_Make(255, 255, 255, 200)
static void TouchScreen_InitButtons(struct TouchScreen* s) {
	struct HacksComp* hacks = &Entities.CurPlayer->Hacks;
	struct TouchButtonDesc* descs;
	struct TouchButtonDesc* desc;
	int haligns = GetOnscreenHAligns();
	int buttons = GetOnscreenButtons();
	int i, j;
	for (i = 0; i < ONSCREEN_MAX_BTNS + TOUCH_EXTRA_BTNS; i++) s->widgets[i] = NULL;

	for (i = 0, j = 0; i < ONSCREEN_MAX_BTNS; i++) 
	{
		if (!(buttons & (1 << i))) continue;
		desc    = &onscreenDescs[i];
		desc->x = (haligns & (1 << i)) ? ANCHOR_MIN : ANCHOR_MAX;

		ButtonWidget_Init(&s->onscreen[j], 100, desc->OnClick);
		if (desc->enabled) Widget_SetDisabled(&s->onscreen[j], !(*desc->enabled));

		s->onscreen[j].meta.ptr = desc;
		s->onscreen[j].color    = TOUCHSCREEN_BTN_COLOR;
		s->widgets[j]           = (struct Widget*)&s->onscreen[j];
		j++;
	}

	s->numOnscreen = j;
	if (hacks->Flying || hacks->Noclip) {
		descs      = hackDescs;
		s->numBtns = Array_Elems(hackDescs);
	} else {
		descs      = normDescs;
		s->numBtns = Array_Elems(normDescs);
	}

	for (i = 0; i < s->numBtns; i++) 
	{
		s->widgets[i + ONSCREEN_MAX_BTNS] = (struct Widget*)&s->btns[i];
		ButtonWidget_Init(&s->btns[i], 60, descs[i].OnClick);
		s->btns[i].color = TOUCHSCREEN_BTN_COLOR;
		s->btns[i].meta.ptr = &descs[i];
	}
}

void TouchScreen_Refresh(void) {
	struct TouchScreen* s = &TouchScreen;
	/* InitButtons changes number of widgets, hence */
	/* must destroy graphics resources BEFORE that */
	Screen_ContextLost(s);
	TouchScreen_InitButtons(s);
	Gui_Refresh((struct Screen*)s);
}
static void TouchScreen_HacksChanged(void* s) { TouchScreen_Refresh(); }

static void TouchScreen_ContextLost(void* screen) {
	struct TouchScreen* s = (struct TouchScreen*)screen;
	Font_Free(&s->font);
	Screen_ContextLost(screen);
}

static void TouchScreen_ContextRecreated(void* screen) {
	struct TouchScreen* s = (struct TouchScreen*)screen;
	struct TouchButtonDesc* desc;
	int i;
	Screen_UpdateVb(screen);
	Gui_MakeTitleFont(&s->font);

	for (i = 0; i < s->numOnscreen; i++) 
	{
		desc = (struct TouchButtonDesc*)s->onscreen[i].meta.ptr;
		ButtonWidget_SetConst(&s->onscreen[i], desc->text, &s->font);
	}
	for (i = 0; i < s->numBtns; i++) 
	{
		desc = (struct TouchButtonDesc*)s->btns[i].meta.ptr;
		ButtonWidget_SetConst(&s->btns[i], desc->text, &s->font);
	}
	ButtonWidget_SetConst(&s->more, "...", &s->font);
}

static void TouchScreen_Render(void* screen, float delta) {
	if (Gui.InputGrab) return;
	Screen_Render2Widgets(screen, delta);
}

static int TouchScreen_PointerDown(void* screen, int id, int x, int y) {
	struct TouchScreen* s = (struct TouchScreen*)screen;
	struct Widget* w;
	int i;
	//Chat_Add1("POINTER DOWN: %i", &id);
	if (Gui.InputGrab) return false;

	i = Screen_DoPointerDown(screen, id, x, y);
	if (i < ONSCREEN_MAX_BTNS) return i >= 0;

	/* Clicking on other buttons then */
	w = s->widgets[i];
	w->active |= id;

	/* Clicking on jump or fly buttons should still move camera */
	for (i = 0; i < s->numBtns; i++) 
	{
		if (w == (struct Widget*)&s->btns[i]) return TOUCH_TYPE_GUI | TOUCH_TYPE_CAMERA;
	}
	return TOUCH_TYPE_GUI;
}

static void TouchScreen_PointerUp(void* screen, int id, int x, int y) {
	struct TouchScreen* s = (struct TouchScreen*)screen;
	struct TouchButtonDesc* desc;
	int i;
	//Chat_Add1("POINTER UP: %i", &id);
	s->thumbstick.active &= ~id;
	s->more.active       &= ~id;

	for (i = 0; i < s->numBtns; i++) 
	{
		if (!(s->btns[i].active & id)) continue;
	 	desc = (struct TouchButtonDesc*)s->btns[i].meta.ptr;

		if (desc->bind) {
			Bind_OnReleased[desc->bind](0, &TouchDevice);
		}
		s->btns[i].active &= ~id;
		return;
	}
}

static void TouchScreen_LayoutOnscreen(struct TouchScreen* s, cc_uint8 alignment) {
	struct TouchButtonDesc* desc;
	int i, x, y;
	cc_uint8 halign;

	for (i = 0, x = 10, y = 10; i < s->numOnscreen; i++) 
	{
		desc   = (struct TouchButtonDesc*)s->onscreen[i].meta.ptr;
		halign = desc->x;
		if (halign != alignment) continue;
		
		Widget_SetLocation(&s->onscreen[i], halign, ANCHOR_MIN, x, y);
		if (s->onscreen[i].y + s->onscreen[i].height <= s->btns[0].y){ y += 40; continue; }

		// overflowed onto jump/fly buttons, move to next column
		y = 10;
		x += 110;
		Widget_SetLocation(&s->onscreen[i], halign, ANCHOR_MIN, x, y);
	}
}

static void TouchScreen_Layout(void* screen) {
	struct TouchScreen* s = (struct TouchScreen*)screen;
	struct TouchButtonDesc* desc;
	float scale = Gui.RawTouchScale;
	int i, height;

	/* Need to align these relative to the hotbar */
	height = HUDScreen_LayoutHotbar();

	for (i = 0; i < s->numBtns; i++) 
	{
		desc = (struct TouchButtonDesc*)s->btns[i].meta.ptr;
		Widget_SetLocation(&s->btns[i], ANCHOR_MAX, ANCHOR_MAX, desc->x, desc->y);
		s->btns[i].yOffset += height;

		/* TODO: Maybe move scaling to be part of button instead */
		s->btns[i].minWidth  = Display_ScaleX(60 * scale);
		s->btns[i].minHeight = Display_ScaleY(60 * scale);
		Widget_Layout(&s->btns[i]);
	}

	TouchScreen_LayoutOnscreen(s, ANCHOR_MIN);
	TouchScreen_LayoutOnscreen(s, ANCHOR_MAX);
	Widget_SetLocation(&s->more, ANCHOR_CENTRE, ANCHOR_MIN, 0, 10);

	Widget_SetLocation(&s->thumbstick, ANCHOR_MIN, ANCHOR_MAX, 30, 5);
	s->thumbstick.yOffset += height;
	s->thumbstick.scale = scale;
	Widget_Layout(&s->thumbstick);
}

static void TouchScreen_GetMovement(struct LocalPlayer* p, float* xMoving, float* zMoving) {
	ThumbstickWidget_GetMovement(&TouchScreen.thumbstick, xMoving, zMoving);
}
static struct LocalPlayerInput touchInput = { TouchScreen_GetMovement };

static void TouchScreen_Init(void* screen) {
	struct TouchScreen* s = (struct TouchScreen*)screen;

	s->widgets     = touch_widgets;
	s->numWidgets  = Array_Elems(touch_widgets);
	s->maxVertices = TOUCH_MAX_VERTICES;
	Event_Register_(&UserEvents.HacksStateChanged, screen, TouchScreen_HacksChanged);
	Event_Register_(&UserEvents.HackPermsChanged,  screen, TouchScreen_HacksChanged);

	TouchScreen_InitButtons(s);
	ButtonWidget_Init(&s->more, 40, TouchScreen_MoreClick);
	s->more.color = TOUCHSCREEN_BTN_COLOR;
	ThumbstickWidget_Init(&s->thumbstick);

	LocalPlayerInput_Add(&touchInput);
}

static void TouchScreen_Free(void* s) {
	Event_Unregister_(&UserEvents.HacksStateChanged, s, TouchScreen_HacksChanged);
	Event_Unregister_(&UserEvents.HackPermsChanged,  s, TouchScreen_HacksChanged);
	LocalPlayerInput_Remove(&touchInput);
}

static const struct ScreenVTABLE TouchScreen_VTABLE = {
	TouchScreen_Init,        Screen_NullUpdate,     TouchScreen_Free,
	TouchScreen_Render,      Screen_BuildMesh,
	Screen_FInput,           Screen_InputUp,        Screen_FKeyPress, Screen_FText,
	TouchScreen_PointerDown, TouchScreen_PointerUp, Screen_FPointer,  Screen_FMouseScroll,
	TouchScreen_Layout,      TouchScreen_ContextLost, TouchScreen_ContextRecreated
};
void TouchScreen_Show(void) {
	struct TouchScreen* s = &TouchScreen;
	s->VTABLE = &TouchScreen_VTABLE;

	if (!Gui.TouchUI) return;
	Gui_Add((struct Screen*)s, GUI_PRIORITY_TOUCH);
}
#endif
