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

typedef void (*Button_GetValue)(struct ButtonWidget* btn, cc_string* raw);
typedef void (*Button_SetValue)(struct ButtonWidget* btn, const cc_string* raw);
/* Describes a menu option button */
struct MenuOptionDesc {
	short dir, y;
	const char* name;
	Widget_LeftClick OnClick;
	Button_Get GetValue; Button_Set SetValue;
	void* meta;
};

/* Describes a boolean menu option button */
struct MenuOptionBoolMeta {
	Button_GetValue GetValue;
	Button_SetValue SetValue;
}

/* Describes a hex color menu option button */
struct MenuOptionHexMeta {
	Button_GetValue GetValue;
	Button_SetValue SetValue;
	PackedCol defaultValue;
}

/* Describes a enumeration menu option button */
struct MenuOptionEnumMeta {
	Button_GetValue GetValue;
	Button_SetValue SetValue;
	const char* const names;
	int count;
}

/* Describes a float menu option button */
struct MenuOptionFloat {
	Button_GetValue GetValue;
	Button_SetValue SetValue;
	struct MenuInputDesc desc;
};

/* Describes a int menu option button */
struct MenuOptionInt {
	Button_GetValue GetValue;
	Button_SetValue SetValue;
	struct MenuInputDesc desc;
};

/*########################################################################################################################*
*------------------------------------------------------Menu utilities-----------------------------------------------------*
*#########################################################################################################################*/
static void Menu_Remove(void* screen, int i) {
	struct Screen* s = (struct Screen*)screen;
	struct Widget** widgets = s->widgets;

	if (widgets[i]) { Elem_Free(widgets[i]); }
	widgets[i] = NULL;
}

static int Menu_Int(const cc_string* str)     { int v;   Convert_ParseInt(str, &v);   return v; }
static float Menu_Float(const cc_string* str) { float v; Convert_ParseFloat(str, &v); return v; }
static PackedCol Menu_HexCol(const cc_string* str) { 
	cc_uint8 rgb[3]; 
	PackedCol_TryParseHex(str, rgb); 
	return PackedCol_Make(rgb[0], rgb[1], rgb[2], 255);
}

static void Menu_SwitchBindsClassic(void* a, void* b) { ClassicBindingsScreen_Show(); }
static void Menu_SwitchNostalgia(void* a, void* b)    { NostalgiaMenuScreen_Show(); }
static void Menu_SwitchFont(void* a, void* b)         { FontListScreen_Show(); }

static void Menu_SwitchOptions(void* a, void* b)   { OptionsGroupScreen_Show(); }
static void Menu_SwitchPause(void* a, void* b)     { Gui_ShowPauseMenu(); }


/*########################################################################################################################*
*--------------------------------------------------MenuOptionsScreen------------------------------------------------------*
*#########################################################################################################################*/
struct MenuOptionsScreen;
typedef void (*InitMenuOptions)(struct MenuOptionsScreen* s);
#define MENUOPTS_MAX_OPTS 11
static void MenuOptionsScreen_Layout(void* screen);

static struct MenuOptionsScreen {
	Screen_Body
	const char* descriptions[MENUOPTS_MAX_OPTS + 1];
	struct ButtonWidget* activeBtn;
	InitMenuOptions DoInit, DoRecreateExtra, OnHacksChanged, OnLightingModeServerChanged;
	int numButtons;
	struct FontDesc titleFont, textFont;
	struct TextGroupWidget extHelp;
	struct Texture extHelpTextures[5]; /* max lines is 5 */
	struct ButtonWidget buttons[MENUOPTS_MAX_OPTS], done;
	const char* extHelpDesc;
} MenuOptionsScreen_Instance;

union MenuOptionMeta {
	MenuOptionBoolMeta b;
	MenuOptionEnumMeta e;
	MenuOptionHexMeta  h;
	MenuOptionFloatMeta f;
	MenuOptionIntMeta i;
};

static union MenuOptionMeta menuOpts_meta[MENUOPTS_MAX_OPTS];
static struct Widget* menuOpts_widgets[MENUOPTS_MAX_OPTS + 1];

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

static void MenuOptionsScreen_Update(struct MenuOptionsScreen* s, struct ButtonWidget* btn) {
	cc_string title; char titleBuffer[STRING_SIZE];
	String_InitArray(title, titleBuffer);

	String_AppendConst(&title, btn->optName);
	if (btn->GetValue) {
		String_AppendConst(&title, ": ");
		btn->GetValue(&title);
	}	
	ButtonWidget_Set(btn, &title, &s->titleFont);
}

CC_NOINLINE static void MenuOptionsScreen_Set(struct MenuOptionsScreen* s, 
											struct ButtonWidget* btn, const cc_string* text) {
	btn->SetValue(text);
	MenuOptionsScreen_Update(s, btn);
}

CC_NOINLINE static void MenuOptionsScreen_FreeExtHelp(struct MenuOptionsScreen* s) {
	Elem_Free(&s->extHelp);
	s->extHelp.lines = 0;
}

static void MenuOptionsScreen_LayoutExtHelp(struct MenuOptionsScreen* s) {
	Widget_SetLocation(&s->extHelp, ANCHOR_MIN, ANCHOR_CENTRE_MIN, 0, 100);
	/* If use centre align above, then each line in extended help gets */
	/* centered aligned separately - which is not the desired behaviour. */
	s->extHelp.xOffset = Window_UI.Width / 2 - s->extHelp.width / 2;
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
	if (s->activeBtn) return;
	desc = s->descriptions[idx];
	if (!desc) return;

	if (!s->widgets[idx]) return;
	if (s->widgets[idx]->flags & WIDGET_FLAG_DISABLED) return;

	descRaw          = String_FromReadonly(desc);
	s->extHelp.lines = String_UNSAFE_Split(&descRaw, '\n', descLines, Array_Elems(descLines));
	
	s->extHelpDesc = desc;
	TextGroupWidget_RedrawAll(&s->extHelp);
	MenuOptionsScreen_LayoutExtHelp(s);
}

static void MenuOptionsScreen_OnDone(const cc_string* value, cc_bool valid) {
	struct MenuOptionsScreen* s = &MenuOptionsScreen_Instance;
	if (valid) {
		MenuOptionsScreen_Set(s, s->activeBtn, value);
		/* Marking screen as dirty fixes changed option widget appearing wrong */
		/*  for a few frames (e.g. Chatlines options changed from '12' to '1') */
		s->dirty = true;
	}

	if (s->selectedI >= 0) MenuOptionsScreen_SelectExtHelp(s, s->selectedI);
	s->activeBtn = NULL;
}

static int MenuOptionsScreen_PointerMove(void* screen, int id, int x, int y) {
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	int i = Menu_DoPointerMove(s, id, x, y);
	if (i == -1 || i == s->selectedI) return true;

	s->selectedI = i;
	if (!s->activeBtn) MenuOptionsScreen_SelectExtHelp(s, i);
	return true;
}

static void MenuOptionsScreen_BeginButtons(struct MenuOptionsScreen* s) {
	s->numButtons = 0;
}

static int MenuOptionsScreen_Add(struct MenuOptionsScreen* s, const char* name, Widget_LeftClick onClick) {
	struct ButtonWidget* btn;
	int i = s->numButtons;
	
	btn = &s->buttons[i];
	ButtonWidget_Add(s, btn, 300, onClick);

	btn->optName  = name;
	btn->meta.ptr = &menuOpts_meta[i];
	s->widgets[i] = (struct Widget*)btn;
	
	s->numButtons++;
	return i;
}

static void MenuOptionsScreen_EndButtons(struct MenuOptionsScreen* s, Widget_LeftClick backClick) {
	struct ButtonWidget* btn;
	int i;
	
	for (i = 0; i < s->numButtons; i++) {
		btn = &s->buttons[i];
		Widget_SetLocation(btn, ANCHOR_CENTRE, ANCHOR_CENTRE, i * 160, i * 160);
	}
	s->numButtons = count;
	ButtonWidget_Add(s, &s->done, 400, backClick);
}

static void MenuOptionsScreen_AddButtons(struct MenuOptionsScreen* s, const struct MenuOptionDesc* btns, int count, Widget_LeftClick backClick) {
	struct ButtonWidget* btn;
	int i;
	
	for (i = 0; i < count; i++) {
		btn = &s->buttons[i];
		ButtonWidget_Add(s, btn,  300, btns[i].OnClick);
		Widget_SetLocation(btn, ANCHOR_CENTRE, ANCHOR_CENTRE, btns[i].dir * 160, btns[i].y);

		btn->optName  = btns[i].name;
		btn->GetValue = btns[i].GetValue;
		btn->SetValue = btns[i].SetValue;
		btn->meta.ptr = &menuOpts_descs[i];
		s->widgets[i] = (struct Widget*)btn;
	}
	s->numButtons = count;
	ButtonWidget_Add(s, &s->done, 400, backClick);
}

static void MenuOptionsScreen_BoolClick(void* screen, void* widget) {
	cc_string value; char valueBuffer[STRING_SIZE];
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	struct ButtonWidget* btn    = (struct ButtonWidget*)widget;
	struct MenuOptionBoolMeta* meta = (struct MenuOptionBoolMeta*)btn->meta.ptr;
	cc_bool isOn;

	String_InitArray(value, valueBuffer);
	meta->GetValue(&value);

	isOn  = String_CaselessEqualsConst(&value, "ON");
	value = String_FromReadonly(isOn ? "OFF" : "ON");
	MenuOptionsScreen_Set(s, btn, &value);
}

static void MenuOptionsScreen_AddBool(struct MenuOptionsScreen* s, const char* name, 
									Button_GetValue getValue, Button_SetValue setValue) {
	int i = MenuOptionsScreen_Add(s, name, MenuOptionsScreen_BoolClick);
	struct MenuOptionBoolMeta* meta = &menuOpts_meta[i].b;
	
	meta->GetValue = getValue;
	meta->SetValue = setValue;
}

static void MenuOptionsScreen_Enum(void* screen, void* widget) {
	cc_string value; char valueBuffer[STRING_SIZE];
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	struct ButtonWidget* btn    = (struct ButtonWidget*)widget;
	
	struct MenuOptionEnumMeta* meta = (struct MenuOptionEnumMeta*)btn->meta.ptr;
	const char* const* names = meta->names;
	int raw, count = meta->count;
	
	String_InitArray(value, valueBuffer);
	meta->GetValue(&value);

	raw   = (Utils_ParseEnum(&value, 0, names, count) + 1) % count;
	value = String_FromReadonly(names[raw]);
	MenuOptionsScreen_Set(s, btn, &value);
}

static void MenuOptionsScreen_AddEnum(struct MenuOptionsScreen* s, const char* name,
									const char* const names, int namesCount,
									Button_GetValue getValue, Button_SetValue setValue) {
	int i = MenuOptionsScreen_Add(s, name, MenuOptionsScreen_EnumClick);
	struct MenuOptionEnumMeta* meta = &menuOpts_meta[i].e;
	
	meta->GetValue = getValue;
	meta->SetValue = setValue;
	meta->names    = names;
	meta->count    = namesCount;
}


static void MenuInputOverlay_CheckStillValid(struct MenuOptionsScreen* s) {
	if (!s->activeBtn) return;
	
	if (s->activeBtn->flags & WIDGET_FLAG_DISABLED) {
		/* source button is disabled now, so close open input overlay */
		MenuInputOverlay_Close(false);
	}
}

static void MenuOptionsScreen_Input(void* screen, void* widget) {
	cc_string value; char valueBuffer[STRING_SIZE];
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	struct ButtonWidget* btn    = (struct ButtonWidget*)widget;
	struct MenuInputDesc* desc;

	MenuOptionsScreen_FreeExtHelp(s);
	s->activeBtn = btn;

	String_InitArray(value, valueBuffer);
	btn->GetValue(&value);
	desc = (struct MenuInputDesc*)btn->meta.ptr;
	MenuInputOverlay_Show(desc, &value, MenuOptionsScreen_OnDone, Gui_TouchUI);
}

static void MenuOptionsScreen_OnHacksChanged(void* screen) {
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	if (s->OnHacksChanged) s->OnHacksChanged(s);
	s->dirty = true;
}
static void MenuOptionsScreen_OnLightingModeServerChanged(void* screen, cc_uint8 oldMode, cc_bool fromServer) {
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	/* This event only actually matters if it's from the server */
	if (fromServer) {
		if (s->OnLightingModeServerChanged) s->OnLightingModeServerChanged(s);
		s->dirty = true;
	}
}

static void MenuOptionsScreen_Init(void* screen) {
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	int i;

	s->widgets    = menuOpts_widgets;
	s->numWidgets = 0;
	s->maxWidgets = MENUOPTS_MAX_OPTS + 1; /* always have back button */

	/* The various menu options screens might have different number of widgets */
	for (i = 0; i < MENUOPTS_MAX_OPTS; i++) { 
		s->widgets[i]      = NULL;
		s->descriptions[i] = NULL;
	}

	s->activeBtn   = NULL;
	s->selectedI   = -1;
	s->DoInit(s);

	TextGroupWidget_Create(&s->extHelp, 5, s->extHelpTextures, MenuOptionsScreen_GetDesc);
	s->extHelp.lines = 0;
	Event_Register_(&UserEvents.HackPermsChanged, screen, MenuOptionsScreen_OnHacksChanged);
	Event_Register_(&WorldEvents.LightingModeChanged, screen, MenuOptionsScreen_OnLightingModeServerChanged);
	
	s->maxVertices = Screen_CalcDefaultMaxVertices(s);
}
	
#define EXTHELP_PAD 5 /* padding around extended help box */
static void MenuOptionsScreen_Render(void* screen, float delta) {
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	struct TextGroupWidget* w;
	PackedCol tableColor = PackedCol_Make(20, 20, 20, 200);

	MenuScreen_Render2(s, delta);
	if (!s->extHelp.lines) return;

	w = &s->extHelp;
	Gfx_Draw2DFlat(w->x - EXTHELP_PAD, w->y - EXTHELP_PAD, 
		w->width + EXTHELP_PAD * 2, w->height + EXTHELP_PAD * 2, tableColor);

	Elem_Render(&s->extHelp, delta);
}

static void MenuOptionsScreen_Free(void* screen) {
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	Event_Unregister_(&UserEvents.HackPermsChanged, screen, MenuOptionsScreen_OnHacksChanged);
	Event_Unregister_(&WorldEvents.LightingModeChanged, screen, MenuOptionsScreen_OnLightingModeServerChanged);
}

static void MenuOptionsScreen_Layout(void* screen) {
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	Screen_Layout(s);
	Menu_LayoutBack(&s->done);
	MenuOptionsScreen_LayoutExtHelp(s);
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
	Gui_MakeTitleFont(&s->titleFont);
	Gui_MakeBodyFont(&s->textFont);
	Screen_UpdateVb(screen);

	for (i = 0; i < s->numButtons; i++) 
	{ 
		if (s->widgets[i]) MenuOptionsScreen_Update(s, &s->buttons[i]); 
	}

	ButtonWidget_SetConst(&s->done, "Done", &s->titleFont);
	if (s->DoRecreateExtra) s->DoRecreateExtra(s);
	TextGroupWidget_SetFont(&s->extHelp, &s->textFont);
	TextGroupWidget_RedrawAll(&s->extHelp); /* TODO: SetFont should redrawall implicitly */
}

static const struct ScreenVTABLE MenuOptionsScreen_VTABLE = {
	MenuOptionsScreen_Init,   Screen_NullUpdate, MenuOptionsScreen_Free, 
	MenuOptionsScreen_Render, Screen_BuildMesh,
	Menu_InputDown,           Screen_InputUp,    Screen_TKeyPress, Screen_TText,
	Menu_PointerDown,         Screen_PointerUp,  MenuOptionsScreen_PointerMove, Screen_TMouseScroll,
	MenuOptionsScreen_Layout, MenuOptionsScreen_ContextLost, MenuOptionsScreen_ContextRecreated
};
void MenuOptionsScreen_Show(InitMenuOptions init) {
	struct MenuOptionsScreen* s = &MenuOptionsScreen_Instance;
	s->grabsInput = true;
	s->closable   = true;
	s->VTABLE     = &MenuOptionsScreen_VTABLE;

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

static void ClassicOptionsScreen_GetAnaglyph(cc_string* v) { Menu_GetBool(v, Game_Anaglyph3D); }
static void ClassicOptionsScreen_SetAnaglyph(const cc_string* v) {
	Game_Anaglyph3D = Menu_SetBool(v, OPT_ANAGLYPH3D);
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

static void ClassicOptionsScreen_GetHacks(cc_string* v) { Menu_GetBool(v, Entities.CurPlayer->Hacks.Enabled); }
static void ClassicOptionsScreen_SetHacks(const cc_string* v) {
	Entities.CurPlayer->Hacks.Enabled = Menu_SetBool(v, OPT_HACKS_ENABLED);
	HacksComp_Update(&Entities.CurPlayer->Hacks);
}

static void ClassicOptionsScreen_RecreateExtra(struct MenuOptionsScreen* s) {
	ButtonWidget_SetConst(&s->buttons[9], "Controls...", &s->titleFont);
}

static void ClassicOptionsScreen_InitWidgets(struct MenuOptionsScreen* s) {
	s->DoRecreateExtra = ClassicOptionsScreen_RecreateExtra;
	
	MenuOptionsScreen_BeginButtons(s);
	{
		MenuOptionsScreen_AddBool(s, "Music",
			ClassicOptionsScreen_GetMusic,    ClassicOptionsScreen_SetMusic);
		MenuOptionsScreen_AddBool(s, "Invert mouse",
			ClassicOptionsScreen_GetInvert,   ClassicOptionsScreen_SetInvert);
		MenuOptionsScreen_AddEnum(s, "Render distance", viewDistNames, VIEW_COUNT,
			ClassicOptionsScreen_GetViewDist, ClassicOptionsScreen_SetViewDist);
		MenuOptionsScreen_AddBool(s, "3d anaglyph",
			ClassicOptionsScreen_GetAnaglyph, ClassicOptionsScreen_SetAnaglyph);
		
		MenuOptionsScreen_AddBool(s, "Sound",
			ClassicOptionsScreen_GetSounds,   ClassicOptionsScreen_SetSounds);
		MenuOptionsScreen_AddBool(s, "Show FPS",
			ClassicOptionsScreen_GetShowFPS,  ClassicOptionsScreen_SetShowFPS);
		MenuOptionsScreen_AddBool(s, "View bobbing",
			ClassicOptionsScreen_GetViewBob,  ClassicOptionsScreen_SetViewBob);
		MenuOptionsScreen_AddEnum(s, "FPS mode", FpsLimit_Names, FPS_LIMIT_COUNT,
			MenuOptionsScreen_GetFPS,         MenuOptionsScreen_SetFPS);
		MenuOptionsScreen_AddBool(s, "Hacks enabled",
			ClassicOptionsScreen_GetHacks,   ClassicOptionsScreen_SetHacks);
	}
	MenuOptionsScreen_EndButtons(s, Menu_SwitchPause);

	ButtonWidget_Add(s, &s->buttons[9], 400, Menu_SwitchBindsClassic);
	Widget_SetLocation(&s->buttons[9],  ANCHOR_CENTRE, ANCHOR_MAX, 0, 95);

	/* Disable certain options */
	if (!Server.IsSinglePlayer) Menu_Remove(s, 3);
	if (!Game_ClassicHacks)     Menu_Remove(s, 8);
}

void ClassicOptionsScreen_Show(void) {
	MenuOptionsScreen_Show(ClassicOptionsScreen_InitWidgets);
}


/*########################################################################################################################*
*----------------------------------------------------EnvSettingsScreen----------------------------------------------------*
*#########################################################################################################################*/
static void EnvSettingsScreen_GetCloudsColor(cc_string* v) { PackedCol_ToHex(v, Env.CloudsCol); }
static void EnvSettingsScreen_SetCloudsColor(const cc_string* v) { Env_SetCloudsCol(Menu_HexCol(v)); }

static void EnvSettingsScreen_GetSkyColor(cc_string* v) { PackedCol_ToHex(v, Env.SkyCol); }
static void EnvSettingsScreen_SetSkyColor(const cc_string* v) { Env_SetSkyCol(Menu_HexCol(v)); }

static void EnvSettingsScreen_GetFogColor(cc_string* v) { PackedCol_ToHex(v, Env.FogCol); }
static void EnvSettingsScreen_SetFogColor(const cc_string* v) { Env_SetFogCol(Menu_HexCol(v)); }

static void EnvSettingsScreen_GetCloudsSpeed(cc_string* v) { String_AppendFloat(v, Env.CloudsSpeed, 2); }
static void EnvSettingsScreen_SetCloudsSpeed(const cc_string* v) { Env_SetCloudsSpeed(Menu_Float(v)); }

static void EnvSettingsScreen_GetCloudsHeight(cc_string* v) { String_AppendInt(v, Env.CloudsHeight); }
static void EnvSettingsScreen_SetCloudsHeight(const cc_string* v) { Env_SetCloudsHeight(Menu_Int(v)); }

static void EnvSettingsScreen_GetSunColor(cc_string* v) { PackedCol_ToHex(v, Env.SunCol); }
static void EnvSettingsScreen_SetSunColor(const cc_string* v) { Env_SetSunCol(Menu_HexCol(v)); }

static void EnvSettingsScreen_GetShadowColor(cc_string* v) { PackedCol_ToHex(v, Env.ShadowCol); }
static void EnvSettingsScreen_SetShadowColor(const cc_string* v) { Env_SetShadowCol(Menu_HexCol(v)); }

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
	static const struct MenuOptionDesc buttons[] = {
		{ -1, -150, "Clouds color",  MenuOptionsScreen_Input,
			EnvSettingsScreen_GetCloudsColor,  EnvSettingsScreen_SetCloudsColor },
		{ -1, -100, "Sky color",     MenuOptionsScreen_Input,
			EnvSettingsScreen_GetSkyColor,     EnvSettingsScreen_SetSkyColor },
		{ -1,  -50, "Fog color",     MenuOptionsScreen_Input,
			EnvSettingsScreen_GetFogColor,     EnvSettingsScreen_SetFogColor },
		{ -1,    0, "Clouds speed",  MenuOptionsScreen_Input,
			EnvSettingsScreen_GetCloudsSpeed,  EnvSettingsScreen_SetCloudsSpeed },
		{ -1,   50, "Clouds height", MenuOptionsScreen_Input,
			EnvSettingsScreen_GetCloudsHeight, EnvSettingsScreen_SetCloudsHeight },

		{ 1, -150, "Sunlight color",  MenuOptionsScreen_Input,
			EnvSettingsScreen_GetSunColor,     EnvSettingsScreen_SetSunColor },
		{ 1, -100, "Shadow color",    MenuOptionsScreen_Input,
			EnvSettingsScreen_GetShadowColor,  EnvSettingsScreen_SetShadowColor },
		{ 1,  -50, "Weather",         MenuOptionsScreen_Enum,
			EnvSettingsScreen_GetWeather,      EnvSettingsScreen_SetWeather },
		{ 1,    0, "Rain/Snow speed", MenuOptionsScreen_Input,
			EnvSettingsScreen_GetWeatherSpeed, EnvSettingsScreen_SetWeatherSpeed },
		{ 1,   50, "Water level",     MenuOptionsScreen_Input,
			EnvSettingsScreen_GetEdgeHeight,   EnvSettingsScreen_SetEdgeHeight }
	};
	MenuOptionsScreen_AddButtons(s, buttons, Array_Elems(buttons), Menu_SwitchOptions);
}

void EnvSettingsScreen_Show(void) {
	MenuInput_Hex(menuOpts_descs[0],   ENV_DEFAULT_CLOUDS_COLOR);
	MenuInput_Hex(menuOpts_descs[1],   ENV_DEFAULT_SKY_COLOR);
	MenuInput_Hex(menuOpts_descs[2],   ENV_DEFAULT_FOG_COLOR);
	MenuInput_Float(menuOpts_descs[3],      0,  1000, 1);
	MenuInput_Int(menuOpts_descs[4],   -10000, 10000, World.Height + 2);

	MenuInput_Hex(menuOpts_descs[5],   ENV_DEFAULT_SUN_COLOR);
	MenuInput_Hex(menuOpts_descs[6],   ENV_DEFAULT_SHADOW_COLOR);
	MenuInput_Enum(menuOpts_descs[7],  Weather_Names, Array_Elems(Weather_Names));
	MenuInput_Float(menuOpts_descs[8],  -100,  100, 1);
	MenuInput_Int(menuOpts_descs[9],   -2048, 2048, World.Height / 2);

	MenuOptionsScreen_Show(EnvSettingsScreen_InitWidgets);
}


/*########################################################################################################################*
*--------------------------------------------------GraphicsOptionsScreen--------------------------------------------------*
*#########################################################################################################################*/
static void GraphicsOptionsScreen_CheckLightingModeAllowed(struct MenuOptionsScreen* s) {
	Widget_SetDisabled(s->widgets[4], Lighting_ModeLockedByServer);
}

static void GraphicsOptionsScreen_GetViewDist(cc_string* v) { String_AppendInt(v, Game_ViewDistance); }
static void GraphicsOptionsScreen_SetViewDist(const cc_string* v) { Game_UserSetViewDistance(Menu_Int(v)); }

static void GraphicsOptionsScreen_GetSmooth(cc_string* v) { Menu_GetBool(v, Builder_SmoothLighting); }
static void GraphicsOptionsScreen_SetSmooth(const cc_string* v) {
	Builder_SmoothLighting = Menu_SetBool(v, OPT_SMOOTH_LIGHTING);
	Builder_ApplyActive();
	MapRenderer_Refresh();
}

static void GraphicsOptionsScreen_GetLighting(cc_string* v) { String_AppendConst(v, LightingMode_Names[Lighting_Mode]); }
static void GraphicsOptionsScreen_SetLighting(const cc_string* v) {
	cc_uint8 mode = Utils_ParseEnum(v, 0, LightingMode_Names, LIGHTING_MODE_COUNT);
	Options_Set(OPT_LIGHTING_MODE, v);

	Lighting_ModeSetByServer = false;
	Lighting_SetMode(mode, false);
}

static void GraphicsOptionsScreen_GetCamera(cc_string* v) { Menu_GetBool(v, Camera.Smooth); }
static void GraphicsOptionsScreen_SetCamera(const cc_string* v) { Camera.Smooth = Menu_SetBool(v, OPT_CAMERA_SMOOTH); }

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
	static const struct MenuOptionDesc buttons[] = {
		{ -1, -150, "Camera Mass",       MenuOptionsScreen_Input,
			GraphicsOptionsScreen_GetCameraMass, GraphicsOptionsScreen_SetCameraMass },
		{ -1, -100, "FPS mode",          MenuOptionsScreen_Enum,
			MenuOptionsScreen_GetFPS,          MenuOptionsScreen_SetFPS },
		{ -1,  -50, "View distance",     MenuOptionsScreen_Input,
			GraphicsOptionsScreen_GetViewDist,   GraphicsOptionsScreen_SetViewDist },
		{ -1,    0, "Smooth lighting", MenuOptionsScreen_Bool,
			GraphicsOptionsScreen_GetSmooth,     GraphicsOptionsScreen_SetSmooth },
		{ -1,  50,  "Lighting mode", MenuOptionsScreen_Enum,
			GraphicsOptionsScreen_GetLighting,   GraphicsOptionsScreen_SetLighting },
		{ 1, -150, "Smooth camera", MenuOptionsScreen_Bool,
			GraphicsOptionsScreen_GetCamera,   GraphicsOptionsScreen_SetCamera },
		{ 1, -100, "Names",   MenuOptionsScreen_Enum,
			GraphicsOptionsScreen_GetNames,   GraphicsOptionsScreen_SetNames },
		{ 1,  -50, "Shadows", MenuOptionsScreen_Enum,
			GraphicsOptionsScreen_GetShadows, GraphicsOptionsScreen_SetShadows },
#ifdef CC_BUILD_N64
		{ 1,    0,  "Filtering", MenuOptionsScreen_Bool,
			GraphicsOptionsScreen_GetMipmaps, GraphicsOptionsScreen_SetMipmaps },
#else
		{ 1,    0,  "Mipmaps", MenuOptionsScreen_Bool,
			GraphicsOptionsScreen_GetMipmaps, GraphicsOptionsScreen_SetMipmaps },
#endif
		{ 1,   50,  "3D anaglyph", MenuOptionsScreen_Bool,
			ClassicOptionsScreen_GetAnaglyph, ClassicOptionsScreen_SetAnaglyph }
	};

	s->OnLightingModeServerChanged = GraphicsOptionsScreen_CheckLightingModeAllowed;

	MenuOptionsScreen_AddButtons(s, buttons, Array_Elems(buttons), Menu_SwitchOptions);
	GraphicsOptionsScreen_CheckLightingModeAllowed(s);

	s->descriptions[0] = "&eChange the smoothness of the smooth camera.";
	s->descriptions[1] = \
		"&eVSync: &fNumber of frames rendered is at most the monitor's refresh rate.\n" \
		"&e30/60/120/144 FPS: &fRenders 30/60/120/144 frames at most each second.\n" \
		"&eNoLimit: &fRenders as many frames as possible each second.\n" \
		"&cNoLimit is pointless - it wastefully renders frames that you don't even see!";
	s->descriptions[3] = \
		"&eSmooth lighting smooths lighting and adds a minor glow to bright blocks.\n" \
		"&cNote: &eThis setting may reduce performance.";
	s->descriptions[4] = \
		"&eClassic: &fTwo levels of light, sun and shadow.\n" \
		"    Good for performance.\n" \
		"&eFancy: &fBright blocks cast a much wider range of light\n" \
		"    May heavily reduce performance.\n" \
		"&cNote: &eIn multiplayer, this option may be changed or locked by the server.";
	s->descriptions[6] = \
		"&eNone: &fNo names of players are drawn.\n" \
		"&eHovered: &fName of the targeted player is drawn see-through.\n" \
		"&eAll: &fNames of all other players are drawn normally.\n" \
		"&eAllHovered: &fAll names of players are drawn see-through.\n" \
		"&eAllUnscaled: &fAll names of players are drawn see-through without scaling.";
	s->descriptions[7] = \
		"&eNone: &fNo entity shadows are drawn.\n" \
		"&eSnapToBlock: &fA square shadow is shown on block you are directly above.\n" \
		"&eCircle: &fA circular shadow is shown across the blocks you are above.\n" \
		"&eCircleAll: &fA circular shadow is shown underneath all entities.";
}

void GraphicsOptionsScreen_Show(void) {
	MenuInput_Float(menuOpts_descs[0], 1, 100, 20);
	MenuInput_Enum(menuOpts_descs[1], FpsLimit_Names, FPS_LIMIT_COUNT);
	MenuInput_Int(menuOpts_descs[2],  8, 4096, 512);
	MenuInput_Enum(menuOpts_descs[4], LightingMode_Names, LIGHTING_MODE_COUNT)
	MenuInput_Enum(menuOpts_descs[6], NameMode_Names,     NAME_MODE_COUNT);
	MenuInput_Enum(menuOpts_descs[7], ShadowMode_Names,   SHADOW_MODE_COUNT);

	MenuOptionsScreen_Show(GraphicsOptionsScreen_InitWidgets);
}


/*########################################################################################################################*
*----------------------------------------------------ChatOptionsScreen-----------------------------------------------------*
*#########################################################################################################################*/
static void ChatOptionsScreen_SetScale(const cc_string* v, float* target, const char* optKey) {
	*target = Menu_Float(v);
	Options_Set(optKey, v);
	Gui_LayoutAll();
}

static void ChatOptionsScreen_GetAutoScaleChat(cc_string* v) { Menu_GetBool(v, Gui.AutoScaleChat); }
static void ChatOptionsScreen_SetAutoScaleChat(const cc_string* v) {
	Gui.AutoScaleChat = Menu_SetBool(v, OPT_CHAT_AUTO_SCALE);
	Gui_LayoutAll();
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
	static const struct MenuOptionDesc buttons[] = {
		{ -1,  0, "Chat scale",         MenuOptionsScreen_Input,
			ChatOptionsScreen_GetChatScale, ChatOptionsScreen_SetChatScale },
		{ -1, 50, "Chat lines",         MenuOptionsScreen_Input,
			ChatOptionsScreen_GetChatlines, ChatOptionsScreen_SetChatlines },

		{  1,  0, "Log to disk",        MenuOptionsScreen_Bool,
			ChatOptionsScreen_GetLogging,   ChatOptionsScreen_SetLogging },
		{  1, 50, "Clickable chat",     MenuOptionsScreen_Bool,
			ChatOptionsScreen_GetClickable, ChatOptionsScreen_SetClickable },

		{ -1,-50, "Scale with window",         MenuOptionsScreen_Bool,
			ChatOptionsScreen_GetAutoScaleChat, ChatOptionsScreen_SetAutoScaleChat }
	};
	MenuOptionsScreen_AddButtons(s, buttons, Array_Elems(buttons), Menu_SwitchOptions);
}

void ChatOptionsScreen_Show(void) {
	MenuInput_Float(menuOpts_descs[0], 0.25f, 4.00f, 1);
	MenuInput_Int(menuOpts_descs[1],       0,    30, Gui.DefaultLines);

	MenuOptionsScreen_Show(ChatOptionsScreen_InitWidgets);
}


/*########################################################################################################################*
*----------------------------------------------------GuiOptionsScreen-----------------------------------------------------*
*#########################################################################################################################*/
static void GuiOptionsScreen_GetShadows(cc_string* v) { Menu_GetBool(v, Drawer2D.BlackTextShadows); }
static void GuiOptionsScreen_SetShadows(const cc_string* v) {
	Drawer2D.BlackTextShadows = Menu_SetBool(v, OPT_BLACK_TEXT);
	Event_RaiseVoid(&ChatEvents.FontChanged);
}

static void GuiOptionsScreen_GetShowFPS(cc_string* v) { Menu_GetBool(v, Gui.ShowFPS); }
static void GuiOptionsScreen_SetShowFPS(const cc_string* v) { Gui.ShowFPS = Menu_SetBool(v, OPT_SHOW_FPS); }

static void GuiOptionsScreen_GetHotbar(cc_string* v) { String_AppendFloat(v, Gui.RawHotbarScale, 1); }
static void GuiOptionsScreen_SetHotbar(const cc_string* v) { ChatOptionsScreen_SetScale(v, &Gui.RawHotbarScale, OPT_HOTBAR_SCALE); }

static void GuiOptionsScreen_GetInventory(cc_string* v) { String_AppendFloat(v, Gui.RawInventoryScale, 1); }
static void GuiOptionsScreen_SetInventory(const cc_string* v) { ChatOptionsScreen_SetScale(v, &Gui.RawInventoryScale, OPT_INVENTORY_SCALE); }

static void GuiOptionsScreen_GetCrosshair(cc_string* v) { String_AppendFloat(v, Gui.RawCrosshairScale, 1); }
static void GuiOptionsScreen_SetCrosshair(const cc_string* v) { ChatOptionsScreen_SetScale(v, &Gui.RawCrosshairScale, OPT_CROSSHAIR_SCALE); }

static void GuiOptionsScreen_GetTabAuto(cc_string* v) { Menu_GetBool(v, Gui.TabAutocomplete); }
static void GuiOptionsScreen_SetTabAuto(const cc_string* v) { Gui.TabAutocomplete = Menu_SetBool(v, OPT_TAB_AUTOCOMPLETE); }

static void GuiOptionsScreen_GetUseFont(cc_string* v) { Menu_GetBool(v, !Drawer2D.BitmappedText); }
static void GuiOptionsScreen_SetUseFont(const cc_string* v) {
	Drawer2D.BitmappedText = !Menu_SetBool(v, OPT_USE_CHAT_FONT);
	Event_RaiseVoid(&ChatEvents.FontChanged);
}

static void GuiOptionsScreen_InitWidgets(struct MenuOptionsScreen* s) {
	static const struct MenuOptionDesc buttons[] = {
		
		{ -1,  -100, "Show FPS",           MenuOptionsScreen_Bool,
			GuiOptionsScreen_GetShowFPS,   GuiOptionsScreen_SetShowFPS },
		{ -1,  -50, "Hotbar scale",       MenuOptionsScreen_Input,
			GuiOptionsScreen_GetHotbar,    GuiOptionsScreen_SetHotbar },
		{ -1,    0, "Inventory scale",    MenuOptionsScreen_Input,
			GuiOptionsScreen_GetInventory, GuiOptionsScreen_SetInventory },
		{ -1,   50, "Crosshair scale",    MenuOptionsScreen_Input,
			GuiOptionsScreen_GetCrosshair, GuiOptionsScreen_SetCrosshair },

		{ 1, -100, "Black text shadows", MenuOptionsScreen_Bool,
			GuiOptionsScreen_GetShadows,   GuiOptionsScreen_SetShadows },
		{ 1,  -50, "Tab auto-complete",  MenuOptionsScreen_Bool,
			GuiOptionsScreen_GetTabAuto,   GuiOptionsScreen_SetTabAuto },
		{ 1,    0, "Use system font",    MenuOptionsScreen_Bool,
			GuiOptionsScreen_GetUseFont,   GuiOptionsScreen_SetUseFont },
		{ 1,   50, "Select system font", Menu_SwitchFont,
			NULL,                          NULL }
	};
	MenuOptionsScreen_AddButtons(s, buttons, Array_Elems(buttons), Menu_SwitchOptions);
}

void GuiOptionsScreen_Show(void) {
	MenuInput_Float(menuOpts_descs[1], 0.25f, 4.00f, 1);
	MenuInput_Float(menuOpts_descs[2], 0.25f, 4.00f, 1);
	MenuInput_Float(menuOpts_descs[3], 0.25f, 4.00f, 1);

	MenuOptionsScreen_Show(GuiOptionsScreen_InitWidgets);
}


/*########################################################################################################################*
*---------------------------------------------------HacksSettingsScreen---------------------------------------------------*
*#########################################################################################################################*/
static void HacksSettingsScreen_GetHacks(cc_string* v) { Menu_GetBool(v, Entities.CurPlayer->Hacks.Enabled); }
static void HacksSettingsScreen_SetHacks(const cc_string* v) {
	Entities.CurPlayer->Hacks.Enabled = Menu_SetBool(v,OPT_HACKS_ENABLED);
	HacksComp_Update(&Entities.CurPlayer->Hacks);
}

static void HacksSettingsScreen_GetSpeed(cc_string* v) { String_AppendFloat(v, Entities.CurPlayer->Hacks.SpeedMultiplier, 2); }
static void HacksSettingsScreen_SetSpeed(const cc_string* v) {
	Entities.CurPlayer->Hacks.SpeedMultiplier = Menu_Float(v);
	Options_Set(OPT_SPEED_FACTOR, v);
}

static void HacksSettingsScreen_GetClipping(cc_string* v) { Menu_GetBool(v, Camera.Clipping); }
static void HacksSettingsScreen_SetClipping(const cc_string* v) {
	Camera.Clipping = Menu_SetBool(v, OPT_CAMERA_CLIPPING);
}

static void HacksSettingsScreen_GetJump(cc_string* v) { 
	String_AppendFloat(v, LocalPlayer_JumpHeight(Entities.CurPlayer), 3); 
}

static void HacksSettingsScreen_SetJump(const cc_string* v) {
	cc_string str; char strBuffer[STRING_SIZE];
	struct PhysicsComp* physics;

	physics = &Entities.CurPlayer->Physics;
	physics->JumpVel     = PhysicsComp_CalcJumpVelocity(Menu_Float(v));
	physics->UserJumpVel = physics->JumpVel;
	
	String_InitArray(str, strBuffer);
	String_AppendFloat(&str, physics->JumpVel, 8);
	Options_Set(OPT_JUMP_VELOCITY, &str);
}

static void HacksSettingsScreen_GetWOMHacks(cc_string* v) { Menu_GetBool(v, Entities.CurPlayer->Hacks.WOMStyleHacks); }
static void HacksSettingsScreen_SetWOMHacks(const cc_string* v) {
	Entities.CurPlayer->Hacks.WOMStyleHacks = Menu_SetBool(v, OPT_WOM_STYLE_HACKS);
}

static void HacksSettingsScreen_GetFullStep(cc_string* v) { Menu_GetBool(v, Entities.CurPlayer->Hacks.FullBlockStep); }
static void HacksSettingsScreen_SetFullStep(const cc_string* v) {
	Entities.CurPlayer->Hacks.FullBlockStep = Menu_SetBool(v, OPT_FULL_BLOCK_STEP);
}

static void HacksSettingsScreen_GetPushback(cc_string* v) { Menu_GetBool(v, Entities.CurPlayer->Hacks.PushbackPlacing); }
static void HacksSettingsScreen_SetPushback(const cc_string* v) {
	Entities.CurPlayer->Hacks.PushbackPlacing = Menu_SetBool(v, OPT_PUSHBACK_PLACING);
}

static void HacksSettingsScreen_GetLiquids(cc_string* v) { Menu_GetBool(v, Game_BreakableLiquids); }
static void HacksSettingsScreen_SetLiquids(const cc_string* v) {
	Game_BreakableLiquids = Menu_SetBool(v, OPT_MODIFIABLE_LIQUIDS);
}

static void HacksSettingsScreen_GetSlide(cc_string* v) { Menu_GetBool(v, Entities.CurPlayer->Hacks.NoclipSlide); }
static void HacksSettingsScreen_SetSlide(const cc_string* v) {
	Entities.CurPlayer->Hacks.NoclipSlide = Menu_SetBool(v, OPT_NOCLIP_SLIDE);
}

static void HacksSettingsScreen_GetFOV(cc_string* v) { String_AppendInt(v, Camera.Fov); }
static void HacksSettingsScreen_SetFOV(const cc_string* v) {
	int fov = Menu_Int(v);
	if (Camera.ZoomFov > fov) Camera.ZoomFov = fov;
	Camera.DefaultFov = fov;

	Options_Set(OPT_FIELD_OF_VIEW, v);
	Camera_SetFov(fov);
}

static void HacksSettingsScreen_CheckHacksAllowed(struct MenuOptionsScreen* s) {
	struct Widget** widgets = s->widgets;
	struct LocalPlayer* p   = Entities.CurPlayer;
	cc_bool disabled        = !p->Hacks.Enabled;

	Widget_SetDisabled(widgets[3], disabled || !p->Hacks.CanSpeed);
	Widget_SetDisabled(widgets[4], disabled || !p->Hacks.CanSpeed);
	Widget_SetDisabled(widgets[5], disabled || !p->Hacks.CanSpeed);
	Widget_SetDisabled(widgets[7], disabled || !p->Hacks.CanPushbackBlocks);
	MenuInputOverlay_CheckStillValid(s);
}

static void HacksSettingsScreen_InitWidgets(struct MenuOptionsScreen* s) {
	MenuOptionsScreen_BeginButtons(s);
	{
		MenuOptionsScreen_AddBool(s, "Hacks enabled",
			HacksSettingsScreen_GetHacks,    HacksSettingsScreen_SetHacks);
		{ -1, -100, "Speed multiplier", MenuOptionsScreen_Input,
			HacksSettingsScreen_GetSpeed,    HacksSettingsScreen_SetSpeed },
		MenuOptionsScreen_AddBool(s, "Camera clipping",
			HacksSettingsScreen_GetClipping, HacksSettingsScreen_SetClipping);
		{ -1,    0, "Jump height",      MenuOptionsScreen_Input,
			HacksSettingsScreen_GetJump,     HacksSettingsScreen_SetJump },
		{ -1,   50, "WoM style hacks",  MenuOptionsScreen_Bool,
			HacksSettingsScreen_GetWOMHacks, HacksSettingsScreen_SetWOMHacks },
		
		MenuOptionsScreen_AddBool(s, "Sound",
			ClassicOptionsScreen_GetSounds,   ClassicOptionsScreen_SetSounds);
		MenuOptionsScreen_AddBool(s, "Show FPS",
			ClassicOptionsScreen_GetShowFPS,  ClassicOptionsScreen_SetShowFPS);
		MenuOptionsScreen_AddBool(s, "View bobbing",
			ClassicOptionsScreen_GetViewBob,  ClassicOptionsScreen_SetViewBob);
		MenuOptionsScreen_AddEnum(s, "FPS mode", FpsLimit_Names, FPS_LIMIT_COUNT,
			MenuOptionsScreen_GetFPS,         MenuOptionsScreen_SetFPS);
		MenuOptionsScreen_AddBool(s, "Hacks enabled",
			ClassicOptionsScreen_GetHacks,   ClassicOptionsScreen_SetHacks);
	}
	MenuOptionsScreen_EndButtons(s, Menu_SwitchOptions);
	
	static const struct MenuOptionDesc buttons[] = {
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
	s->OnHacksChanged = HacksSettingsScreen_CheckHacksAllowed;
	HacksSettingsScreen_CheckHacksAllowed(s);

	s->descriptions[2] = "&eIf &fON&e, then the third person cameras will limit\n&etheir zoom distance if they hit a solid block.";
	s->descriptions[3] = "&eSets how many blocks high you can jump up.\n&eNote: You jump much higher when holding down the Speed key binding.";
	s->descriptions[4] = \
		"&eIf &fON&e, gives you a triple jump which increases speed massively,\n" \
		"&ealong with older noclip style. This is based on the \"World of Minecraft\"\n" \
		"&eclassic client mod, which popularized hacks conventions and controls\n" \
		"&ebefore ClassiCube was created.";
	s->descriptions[7] = \
		"&eIf &fON&e, placing blocks that intersect your own position cause\n" \
		"&ethe block to be placed, and you to be moved out of the way.\n" \
		"&fThis is mainly useful for quick pillaring/towering.";
	s->descriptions[8] = "&eIf &fOFF&e, you will immediately stop when in noclip\n&emode and no movement keys are held down.";
}

void HacksSettingsScreen_Show(void) {
	MenuInput_Float(menuOpts_descs[1], 0.1f,   50, 10);
	MenuInput_Float(menuOpts_descs[3], 0.1f, 2048, 1.233f);
	MenuInput_Int(menuOpts_descs[9],      1,  179, 70);

	MenuOptionsScreen_Show(HacksSettingsScreen_InitWidgets);
}


/*########################################################################################################################*
*----------------------------------------------------MiscOptionsScreen----------------------------------------------------*
*#########################################################################################################################*/
static void MiscOptionsScreen_GetReach(cc_string* v) { String_AppendFloat(v, Entities.CurPlayer->ReachDistance, 2); }
static void MiscOptionsScreen_SetReach(const cc_string* v) { Entities.CurPlayer->ReachDistance = Menu_Float(v); }

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

static void MiscOptionsScreen_GetInvert(cc_string* v) { Menu_GetBool(v, Camera.Invert); }
static void MiscOptionsScreen_SetInvert(const cc_string* v) { Camera.Invert = Menu_SetBool(v, OPT_INVERT_MOUSE); }

static void MiscOptionsScreen_GetSensitivity(cc_string* v) { String_AppendInt(v, Camera.Sensitivity); }
static void MiscOptionsScreen_SetSensitivity(const cc_string* v) {
	Camera.Sensitivity = Menu_Int(v);
	Options_Set(OPT_SENSITIVITY, v);
}

static void MiscSettingsScreen_InitWidgets(struct MenuOptionsScreen* s) {
	static const struct MenuOptionDesc buttons[] = {
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
		{ 1,    0, "Invert mouse",        MenuOptionsScreen_Bool,
			MiscOptionsScreen_GetInvert,      MiscOptionsScreen_SetInvert },
		{ 1,   50, "Mouse sensitivity",   MenuOptionsScreen_Input,
			MiscOptionsScreen_GetSensitivity, MiscOptionsScreen_SetSensitivity }
	};
	MenuOptionsScreen_AddButtons(s, buttons, Array_Elems(buttons), Menu_SwitchOptions);

	/* Disable certain options */
	if (!Server.IsSinglePlayer) Menu_Remove(s, 0);
	if (!Server.IsSinglePlayer) Menu_Remove(s, 4);
}

void MiscOptionsScreen_Show(void) {
	MenuInput_Float(menuOpts_descs[0], 1, 1024, 5);
	MenuInput_Int(menuOpts_descs[1],   0, 100,  DEFAULT_MUSIC_VOLUME);
	MenuInput_Int(menuOpts_descs[2],   0, 100,  DEFAULT_SOUNDS_VOLUME);
#ifdef CC_BUILD_WIN
	MenuInput_Int(menuOpts_descs[6],   1, 200, 40);
#else
	MenuInput_Int(menuOpts_descs[6],   1, 200, 30);
#endif

	MenuOptionsScreen_Show(MiscSettingsScreen_InitWidgets);
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


/*########################################################################################################################*
*------------------------------------------------NostalgiaAppearanceScreen------------------------------------------------*
*#########################################################################################################################*/
static void NostalgiaScreen_GetHand(cc_string* v) { Menu_GetBool(v, Models.ClassicArms); }
static void NostalgiaScreen_SetHand(const cc_string* v) { Models.ClassicArms = Menu_SetBool(v, OPT_CLASSIC_ARM_MODEL); }

static void NostalgiaScreen_GetAnim(cc_string* v) { Menu_GetBool(v, !Game_SimpleArmsAnim); }
static void NostalgiaScreen_SetAnim(const cc_string* v) {
	Game_SimpleArmsAnim = String_CaselessEqualsConst(v, "OFF");
	Options_SetBool(OPT_SIMPLE_ARMS_ANIM, Game_SimpleArmsAnim);
}

static void NostalgiaScreen_GetClassicChat(cc_string* v) { Menu_GetBool(v, Gui.ClassicChat); }
static void NostalgiaScreen_SetClassicChat(const cc_string* v) { Gui.ClassicChat = Menu_SetBool(v, OPT_CLASSIC_CHAT); }

static void NostalgiaScreen_GetClassicInv(cc_string* v) { Menu_GetBool(v, Gui.ClassicInventory); }
static void NostalgiaScreen_SetClassicInv(const cc_string* v) { Gui.ClassicInventory = Menu_SetBool(v, OPT_CLASSIC_INVENTORY); }

static void NostalgiaScreen_GetGui(cc_string* v) { Menu_GetBool(v, Gui.ClassicTexture); }
static void NostalgiaScreen_SetGui(const cc_string* v) { Gui.ClassicTexture = Menu_SetBool(v, OPT_CLASSIC_GUI); }

static void NostalgiaScreen_GetList(cc_string* v) { Menu_GetBool(v, Gui.ClassicTabList); }
static void NostalgiaScreen_SetList(const cc_string* v) { Gui.ClassicTabList = Menu_SetBool(v, OPT_CLASSIC_TABLIST); }

static void NostalgiaScreen_GetOpts(cc_string* v) { Menu_GetBool(v, Gui.ClassicMenu); }
static void NostalgiaScreen_SetOpts(const cc_string* v) { Gui.ClassicMenu = Menu_SetBool(v, OPT_CLASSIC_OPTIONS); }

static void NostalgiaAppearanceScreen_InitWidgets(struct MenuOptionsScreen* s) {
	static const struct MenuOptionDesc buttons[] = {
		{ -1, -100, "Classic hand model",   MenuOptionsScreen_Bool,
			NostalgiaScreen_GetHand,   NostalgiaScreen_SetHand },
		{ -1,  -50, "Classic walk anim",    MenuOptionsScreen_Bool,
			NostalgiaScreen_GetAnim,   NostalgiaScreen_SetAnim },
        { -1,    0, "Classic chat",    MenuOptionsScreen_Bool,
            NostalgiaScreen_GetClassicChat, NostalgiaScreen_SetClassicChat },
		{ -1,  50, "Classic inventory", MenuOptionsScreen_Bool,
			NostalgiaScreen_GetClassicInv,  NostalgiaScreen_SetClassicInv },
		{  1,  -50, "Classic GUI textures", MenuOptionsScreen_Bool,
			NostalgiaScreen_GetGui,    NostalgiaScreen_SetGui },
		{  1,    0, "Classic player list",  MenuOptionsScreen_Bool,
			NostalgiaScreen_GetList,   NostalgiaScreen_SetList },
		{  1,   50, "Classic options",      MenuOptionsScreen_Bool,
			NostalgiaScreen_GetOpts,   NostalgiaScreen_SetOpts },
	};

	MenuOptionsScreen_AddButtons(s, buttons, Array_Elems(buttons), Menu_SwitchNostalgia);
}

void NostalgiaAppearanceScreen_Show(void) {
	MenuOptionsScreen_Show(NostalgiaAppearanceScreen_InitWidgets);
}


/*########################################################################################################################*
*----------------------------------------------NostalgiaFunctionalityScreen-----------------------------------------------*
*#########################################################################################################################*/
static void NostalgiaScreen_UpdateVersionDisabled(void) {
	struct ButtonWidget* gameVerBtn = &MenuOptionsScreen_Instance.buttons[3];
	Widget_SetDisabled(gameVerBtn, Game_Version.HasCPE);
}

static void NostalgiaScreen_GetTexs(cc_string* v) { Menu_GetBool(v, Game_AllowServerTextures); }
static void NostalgiaScreen_SetTexs(const cc_string* v) { Game_AllowServerTextures = Menu_SetBool(v, OPT_SERVER_TEXTURES); }

static void NostalgiaScreen_GetCustom(cc_string* v) { Menu_GetBool(v, Game_AllowCustomBlocks); }
static void NostalgiaScreen_SetCustom(const cc_string* v) { Game_AllowCustomBlocks = Menu_SetBool(v, OPT_CUSTOM_BLOCKS); }

static void NostalgiaScreen_GetCPE(cc_string* v) { Menu_GetBool(v, Game_Version.HasCPE); }
static void NostalgiaScreen_SetCPE(const cc_string* v) {
	Menu_SetBool(v, OPT_CPE); 
	GameVersion_Load();
	NostalgiaScreen_UpdateVersionDisabled();
}

static void NostalgiaScreen_Version(void* screen, void* widget) {
	struct MenuOptionsScreen* s = (struct MenuOptionsScreen*)screen;
	int ver = Game_Version.Version - 1;
	if (ver < VERSION_0017) ver = VERSION_0030;

	Options_SetInt(OPT_GAME_VERSION, ver);
	GameVersion_Load();
	MenuOptionsScreen_Update(s, widget);
}

static void NostalgiaScreen_GetVersion(cc_string* v) { String_AppendConst(v, Game_Version.Name); }
static void NostalgiaScreen_SetVersion(const cc_string* v) { }

static struct TextWidget nostalgia_desc;
static void NostalgiaScreen_RecreateExtra(struct MenuOptionsScreen* s) {
	TextWidget_SetConst(&nostalgia_desc, "&eRequires restarting game to take full effect", &s->textFont);
}

static void NostalgiaFunctionalityScreen_InitWidgets(struct MenuOptionsScreen* s) {
	static const struct MenuOptionDesc buttons[] = {
		{ -1, -50, "Use server textures",  MenuOptionsScreen_Bool,
			NostalgiaScreen_GetTexs,    NostalgiaScreen_SetTexs },
		{ -1,   0, "Allow custom blocks",  MenuOptionsScreen_Bool,
			NostalgiaScreen_GetCustom,  NostalgiaScreen_SetCustom },

		{  1, -50, "Non-classic features", MenuOptionsScreen_Bool,
			NostalgiaScreen_GetCPE,     NostalgiaScreen_SetCPE },
		{  1,   0, "Game version",         NostalgiaScreen_Version,
			NostalgiaScreen_GetVersion, NostalgiaScreen_SetVersion }
	};
	s->DoRecreateExtra = NostalgiaScreen_RecreateExtra;

	MenuOptionsScreen_AddButtons(s, buttons, Array_Elems(buttons), Menu_SwitchNostalgia);
	TextWidget_Add(s, &nostalgia_desc);
	Widget_SetLocation(&nostalgia_desc, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 100);

	NostalgiaScreen_UpdateVersionDisabled();
	s->descriptions[3] = \
		"&eNote that support for versions earlier than 0.30 is incomplete.\n" \
		"\n" \
		"&cNote that some servers only support 0.30 game version";
}

void NostalgiaFunctionalityScreen_Show(void) {
	MenuOptionsScreen_Show(NostalgiaFunctionalityScreen_InitWidgets);
}

