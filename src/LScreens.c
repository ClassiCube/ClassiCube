#include "LScreens.h"
#ifndef CC_BUILD_WEB
#include "String.h"
#include "LWidgets.h"
#include "LWeb.h"
#include "Launcher.h"
#include "Gui.h"
#include "Drawer2D.h"
#include "ExtMath.h"
#include "Platform.h"
#include "Stream.h"
#include "Funcs.h"
#include "Resources.h"
#include "Logger.h"
#include "Window.h"
#include "Input.h"
#include "Options.h"
#include "Utils.h"
#include "LBackend.h"
#include "Http.h"
#define LAYOUTS static const struct LLayout

/*########################################################################################################################*
*---------------------------------------------------------Screen base-----------------------------------------------------*
*#########################################################################################################################*/
static void LScreen_NullFunc(struct LScreen* s) { }
CC_NOINLINE static int LScreen_IndexOf(struct LScreen* s, void* w) {
	int i;
	for (i = 0; i < s->numWidgets; i++) {
		if (s->widgets[i] == w) return i;
	}
	return -1;
}

static void LScreen_DoLayout(struct LScreen* s) {
	int i;
	for (i = 0; i < s->numWidgets; i++) 
	{
		LBackend_LayoutWidget(s->widgets[i]);
	}
}

static void LScreen_Tick(struct LScreen* s) {
	struct LWidget* w = s->selectedWidget;
	if (w && w->VTABLE->Tick) w->VTABLE->Tick(w);
}

void LScreen_SelectWidget(struct LScreen* s, int idx, struct LWidget* w, cc_bool was) {
	if (!w) return;
	w->selected       = true;
	s->selectedWidget = w;
	if (w->VTABLE->OnSelect) w->VTABLE->OnSelect(w, idx, was);
}

void LScreen_UnselectWidget(struct LScreen* s, int idx, struct LWidget* w) {
	if (!w) return;
	w->selected       = false;
	s->selectedWidget = NULL;
	if (w->VTABLE->OnUnselect) w->VTABLE->OnUnselect(w, idx);
}

static void LScreen_HandleTab(struct LScreen* s) {
	struct LWidget* w;
	int dir   = Key_IsShiftPressed() ? -1 : 1;
	int index = 0, i, j;

	if (s->selectedWidget) {
		index = LScreen_IndexOf(s, s->selectedWidget) + dir;
	}

	for (j = 0; j < s->numWidgets; j++) {
		i = (index + j * dir) % s->numWidgets;
		if (i < 0) i += s->numWidgets;

		w = s->widgets[i];
		if (!w->tabSelectable) continue;

		LScreen_UnselectWidget(s, 0, s->selectedWidget);
		LScreen_SelectWidget(s, 0, w, false);
		return;
	}
}

static void LScreen_KeyDown(struct LScreen* s, int key, cc_bool was) {
	if (key == KEY_TAB) {
		LScreen_HandleTab(s);
	} else if (key == KEY_ENTER || key == KEY_KP_ENTER) {
		/* Shouldn't multi click when holding down Enter */
		if (was) return;

		if (s->selectedWidget && s->selectedWidget->OnClick) {
			s->selectedWidget->OnClick(s->selectedWidget);
		} else if (s->hoveredWidget && s->hoveredWidget->OnClick) {
			s->hoveredWidget->OnClick(s->hoveredWidget);
		} else if (s->onEnterWidget) {
			s->onEnterWidget->OnClick(s->onEnterWidget);
		}
	} else if (s->selectedWidget) {
		if (!s->selectedWidget->VTABLE->KeyDown) return;
		s->selectedWidget->VTABLE->KeyDown(s->selectedWidget, key, was);
	}
}

static void LScreen_MouseUp(struct LScreen* s, int idx) { }
static void LScreen_MouseWheel(struct LScreen* s, float delta) { }

static void LScreen_DrawBackground(struct LScreen* s, struct Context2D* ctx) {
	if (!s->title) {
		Launcher_DrawBackground(ctx, 0, 0, ctx->width, ctx->height);
		return;
	}
	Launcher_DrawBackgroundAll(ctx);
	LBackend_DrawLogo(ctx, s->title);
}

CC_NOINLINE static void LScreen_Reset(struct LScreen* s) {
	int i;

	s->Init = NULL; /* screens should always override this */
	s->Show = LScreen_NullFunc;
	s->Free = LScreen_NullFunc;
	s->Layout     = LScreen_DoLayout;
	s->Tick       = LScreen_Tick;
	s->KeyDown    = LScreen_KeyDown;
	s->MouseUp    = LScreen_MouseUp;
	s->MouseWheel = LScreen_MouseWheel;
	s->DrawBackground = LScreen_DrawBackground;
	s->ResetArea      = Launcher_DrawBackground;

	/* reset all widgets mouse state */
	for (i = 0; i < s->numWidgets; i++) { 
		s->widgets[i]->hovered  = false;
		s->widgets[i]->selected = false;
	}

	s->onEnterWidget  = NULL;
	s->hoveredWidget  = NULL;
	s->selectedWidget = NULL;
}

static void SwitchToChooseMode(void* w)    { ChooseModeScreen_SetActive(false); }
static void SwitchToColours(void* w)       { ColoursScreen_SetActive(); }
static void SwitchToDirectConnect(void* w) { DirectConnectScreen_SetActive(); }
static void SwitchToMain(void* w)          { MainScreen_SetActive(); }
static void SwitchToSettings(void* w)      { SettingsScreen_SetActive(); }
static void SwitchToThemes(void* w)        { ThemesScreen_SetActive(); }
static void SwitchToUpdates(void* w)       { UpdatesScreen_SetActive(); }


/*########################################################################################################################*
*-------------------------------------------------------ChooseModeScreen--------------------------------------------------*
*#########################################################################################################################*/
static struct ChooseModeScreen {
	LScreen_Layout
	struct LLine seps[2];
	struct LButton btnEnhanced, btnClassicHax, btnClassic, btnBack;
	struct LLabel  lblHelp, lblEnhanced[2], lblClassicHax[2], lblClassic[2];
	cc_bool firstTime;
} ChooseModeScreen;

static struct LWidget* chooseMode_widgets[] = {
	(struct LWidget*)&ChooseModeScreen.seps[0],      (struct LWidget*)&ChooseModeScreen.seps[1],
	(struct LWidget*)&ChooseModeScreen.btnEnhanced,  (struct LWidget*)&ChooseModeScreen.lblEnhanced[0],  (struct LWidget*)&ChooseModeScreen.lblEnhanced[1],
	(struct LWidget*)&ChooseModeScreen.btnClassicHax,(struct LWidget*)&ChooseModeScreen.lblClassicHax[0],(struct LWidget*)&ChooseModeScreen.lblClassicHax[1],
	(struct LWidget*)&ChooseModeScreen.btnClassic,   (struct LWidget*)&ChooseModeScreen.lblClassic[0],   (struct LWidget*)&ChooseModeScreen.lblClassic[1],
	NULL
};

LAYOUTS mode_seps0[] = { { ANCHOR_CENTRE, -5 }, { ANCHOR_CENTRE, -85 } };
LAYOUTS mode_seps1[] = { { ANCHOR_CENTRE, -5 }, { ANCHOR_CENTRE, -15 } };

LAYOUTS mode_btnEnhanced[]    = { { ANCHOR_CENTRE_MIN, -250 }, { ANCHOR_CENTRE, -120      } };
LAYOUTS mode_lblEnhanced0[]   = { { ANCHOR_CENTRE_MIN,  -85 }, { ANCHOR_CENTRE, -120 - 12 } };
LAYOUTS mode_lblEnhanced1[]   = { { ANCHOR_CENTRE_MIN,  -85 }, { ANCHOR_CENTRE, -120 + 12 } };
LAYOUTS mode_btnClassicHax[]  = { { ANCHOR_CENTRE_MIN, -250 }, { ANCHOR_CENTRE,  -50      } };
LAYOUTS mode_lblClassicHax0[] = { { ANCHOR_CENTRE_MIN,  -85 }, { ANCHOR_CENTRE,  -50 - 12 } };
LAYOUTS mode_lblClassicHax1[] = { { ANCHOR_CENTRE_MIN,  -85 }, { ANCHOR_CENTRE,  -50 + 12 } };
LAYOUTS mode_btnClassic[]     = { { ANCHOR_CENTRE_MIN, -250 }, { ANCHOR_CENTRE,   20      } };
LAYOUTS mode_lblClassic0[]    = { { ANCHOR_CENTRE_MIN,  -85 }, { ANCHOR_CENTRE,   20 - 12 } };
LAYOUTS mode_lblClassic1[]    = { { ANCHOR_CENTRE_MIN,  -85 }, { ANCHOR_CENTRE,   20 + 12 } };

LAYOUTS mode_lblHelp[] = { { ANCHOR_CENTRE, 0 }, { ANCHOR_CENTRE, 160 } };
LAYOUTS mode_btnBack[] = { { ANCHOR_CENTRE, 0 }, { ANCHOR_CENTRE, 170 } };


CC_NOINLINE static void ChooseMode_Click(cc_bool classic, cc_bool classicHacks) {
	Options_SetBool(OPT_CLASSIC_MODE, classic);
	if (classic) Options_SetBool(OPT_CLASSIC_HACKS, classicHacks);

	Options_SetBool(OPT_CUSTOM_BLOCKS,   !classic);
	Options_SetBool(OPT_CPE,             !classic);
	Options_SetBool(OPT_SERVER_TEXTURES, !classic);
	Options_SetBool(OPT_CLASSIC_TABLIST, classic);
	Options_SetBool(OPT_CLASSIC_OPTIONS, classic);

	Options_SaveIfChanged();
	Launcher_LoadTheme();
	LBackend_UpdateLogoFont();
	MainScreen_SetActive();
}

static void UseModeEnhanced(void* w)   { ChooseMode_Click(false, false); }
static void UseModeClassicHax(void* w) { ChooseMode_Click(true,  true);  }
static void UseModeClassic(void* w)    { ChooseMode_Click(true,  false); }

static void ChooseModeScreen_Init(struct LScreen* s_) {
	struct ChooseModeScreen* s = (struct ChooseModeScreen*)s_;
	s->widgets     = chooseMode_widgets;
	s->numWidgets  = Array_Elems(chooseMode_widgets);

	LLine_Init(  &s->seps[0], 490, mode_seps0);
	LLine_Init(  &s->seps[1], 490, mode_seps1);

	LButton_Init(&s->btnEnhanced, 145, 35, "Enhanced",                        mode_btnEnhanced);
	LLabel_Init( &s->lblEnhanced[0], "&eEnables custom blocks, changing env", mode_lblEnhanced0);
	LLabel_Init( &s->lblEnhanced[1], "&esettings, longer messages, and more", mode_lblEnhanced1);

	LButton_Init(&s->btnClassicHax, 145, 35, "Classic +hax",                     mode_btnClassicHax);
	LLabel_Init( &s->lblClassicHax[0], "&eSame as Classic mode, except that",    mode_lblClassicHax0);
	LLabel_Init( &s->lblClassicHax[1], "&ehacks (noclip/fly/speed) are enabled", mode_lblClassicHax1);

	LButton_Init(&s->btnClassic, 145, 35, "Classic",                        mode_btnClassic);
	LLabel_Init( &s->lblClassic[0], "&eOnly uses blocks and features from", mode_lblClassic0);
	LLabel_Init( &s->lblClassic[1], "&ethe original minecraft classic",     mode_lblClassic1);

	LLabel_Init( &s->lblHelp, "&eClick &fEnhanced &eif you're not sure which mode to choose.", mode_lblHelp);
	LButton_Init(&s->btnBack, 80, 35, "Back",                                                  mode_btnBack);

	s->btnEnhanced.OnClick   = UseModeEnhanced;
	s->btnClassicHax.OnClick = UseModeClassicHax;
	s->btnClassic.OnClick    = UseModeClassic;
	s->btnBack.OnClick       = SwitchToSettings;
}

static void ChooseModeScreen_Show(struct LScreen* s_) {
	struct ChooseModeScreen* s = (struct ChooseModeScreen*)s_;

	s->widgets[11] = s->firstTime ?
		(struct LWidget*)&ChooseModeScreen.lblHelp :
		(struct LWidget*)&ChooseModeScreen.btnBack;
}

void ChooseModeScreen_SetActive(cc_bool firstTime) {
	struct ChooseModeScreen* s = &ChooseModeScreen;
	LScreen_Reset((struct LScreen*)s);
	s->Init      = ChooseModeScreen_Init;
	s->Show      = ChooseModeScreen_Show;
	s->firstTime = firstTime;

	s->title         = "Choose mode";
	s->onEnterWidget = (struct LWidget*)&s->btnEnhanced;
	Launcher_SetScreen((struct LScreen*)s);
}


/*########################################################################################################################*
*---------------------------------------------------------ColoursScreen---------------------------------------------------*
*#########################################################################################################################*/
static struct ColoursScreen {
	LScreen_Layout
	struct LButton btnBack;
	struct LLabel lblNames[5], lblRGB[3];
	struct LInput iptColours[5 * 3];
	struct LCheckbox cbClassic;
	float colourAcc;
} ColoursScreen;

static struct LWidget* colours_widgets[] = {
	(struct LWidget*)&ColoursScreen.iptColours[ 0], (struct LWidget*)&ColoursScreen.iptColours[ 1], (struct LWidget*)&ColoursScreen.iptColours[ 2],
	(struct LWidget*)&ColoursScreen.iptColours[ 3], (struct LWidget*)&ColoursScreen.iptColours[ 4], (struct LWidget*)&ColoursScreen.iptColours[ 5],
	(struct LWidget*)&ColoursScreen.iptColours[ 6], (struct LWidget*)&ColoursScreen.iptColours[ 7], (struct LWidget*)&ColoursScreen.iptColours[ 8],
	(struct LWidget*)&ColoursScreen.iptColours[ 9], (struct LWidget*)&ColoursScreen.iptColours[10], (struct LWidget*)&ColoursScreen.iptColours[11],
	(struct LWidget*)&ColoursScreen.iptColours[12], (struct LWidget*)&ColoursScreen.iptColours[13], (struct LWidget*)&ColoursScreen.iptColours[14],
	(struct LWidget*)&ColoursScreen.lblNames[0],    (struct LWidget*)&ColoursScreen.lblNames[1],
	(struct LWidget*)&ColoursScreen.lblNames[2],    (struct LWidget*)&ColoursScreen.lblNames[3],
	(struct LWidget*)&ColoursScreen.lblNames[4],
	(struct LWidget*)&ColoursScreen.lblRGB[0],      (struct LWidget*)&ColoursScreen.lblRGB[1], (struct LWidget*)&ColoursScreen.lblRGB[2],
	(struct LWidget*)&ColoursScreen.btnBack,        (struct LWidget*)&ColoursScreen.cbClassic
};

#define IptColor_Layout(xx, yy) { { ANCHOR_CENTRE, xx }, { ANCHOR_CENTRE, yy } }
LAYOUTS clr_iptColours[15][2] = {
	IptColor_Layout(30, -100), IptColor_Layout(95, -100), IptColor_Layout(160, -100),
	IptColor_Layout(30,  -60), IptColor_Layout(95,  -60), IptColor_Layout(160,  -60),
	IptColor_Layout(30,  -20), IptColor_Layout(95,  -20), IptColor_Layout(160,  -20),
	IptColor_Layout(30,   20), IptColor_Layout(95,   20), IptColor_Layout(160,   20),
	IptColor_Layout(30,   60), IptColor_Layout(95,   60), IptColor_Layout(160,   60),
};
LAYOUTS clr_lblNames0[] = { { ANCHOR_CENTRE_MAX, 10 }, { ANCHOR_CENTRE, -100 } };
LAYOUTS clr_lblNames1[] = { { ANCHOR_CENTRE_MAX, 10 }, { ANCHOR_CENTRE,  -60 } };
LAYOUTS clr_lblNames2[] = { { ANCHOR_CENTRE_MAX, 10 }, { ANCHOR_CENTRE,  -20 } };
LAYOUTS clr_lblNames3[] = { { ANCHOR_CENTRE_MAX, 10 }, { ANCHOR_CENTRE,   20 } };
LAYOUTS clr_lblNames4[] = { { ANCHOR_CENTRE_MAX, 10 }, { ANCHOR_CENTRE,   60 } };

LAYOUTS clr_lblRGB0[]   = { { ANCHOR_CENTRE,  30 }, { ANCHOR_CENTRE, -130 } };
LAYOUTS clr_lblRGB1[]   = { { ANCHOR_CENTRE,  95 }, { ANCHOR_CENTRE, -130 } };
LAYOUTS clr_lblRGB2[]   = { { ANCHOR_CENTRE, 160 }, { ANCHOR_CENTRE, -130 } };
LAYOUTS clr_cbClassic[] = { { ANCHOR_CENTRE, -16 }, { ANCHOR_CENTRE,  130 } };
LAYOUTS clr_btnBack[]   = { { ANCHOR_CENTRE,   0 }, { ANCHOR_CENTRE,  170 } };


CC_NOINLINE static void ColoursScreen_Set(struct LInput* w, cc_uint8 value) {
	cc_string tmp; char tmpBuffer[STRING_SIZE];
	String_InitArray(tmp, tmpBuffer);

	String_AppendInt(&tmp, value);
	LInput_SetText(w, &tmp);
}

CC_NOINLINE static void ColoursScreen_Update(struct ColoursScreen* s, int i, BitmapCol color) {
	ColoursScreen_Set(&s->iptColours[i + 0], BitmapCol_R(color));
	ColoursScreen_Set(&s->iptColours[i + 1], BitmapCol_G(color));
	ColoursScreen_Set(&s->iptColours[i + 2], BitmapCol_B(color));
}

CC_NOINLINE static void ColoursScreen_UpdateAll(struct ColoursScreen* s) {
	ColoursScreen_Update(s,  0, Launcher_Theme.BackgroundColor);
	ColoursScreen_Update(s,  3, Launcher_Theme.ButtonBorderColor);
	ColoursScreen_Update(s,  6, Launcher_Theme.ButtonHighlightColor);
	ColoursScreen_Update(s,  9, Launcher_Theme.ButtonForeColor);
	ColoursScreen_Update(s, 12, Launcher_Theme.ButtonForeActiveColor);
}

static void ColoursScreen_TextChanged(struct LInput* w) {
	struct ColoursScreen* s = &ColoursScreen;
	int index = LScreen_IndexOf((struct LScreen*)s, w);
	BitmapCol* color;
	cc_uint8 r, g, b;

	if (index < 3)       color = &Launcher_Theme.BackgroundColor;
	else if (index < 6)  color = &Launcher_Theme.ButtonBorderColor;
	else if (index < 9)  color = &Launcher_Theme.ButtonHighlightColor;
	else if (index < 12) color = &Launcher_Theme.ButtonForeColor;
	else                 color = &Launcher_Theme.ButtonForeActiveColor;

	/* if index of G input, changes to index of R input */
	index = (index / 3) * 3;
	if (!Convert_ParseUInt8(&s->iptColours[index + 0].text, &r)) return;
	if (!Convert_ParseUInt8(&s->iptColours[index + 1].text, &g)) return;
	if (!Convert_ParseUInt8(&s->iptColours[index + 2].text, &b)) return;

	*color = BitmapColor_RGB(r, g, b);
	Launcher_SaveTheme();
	LBackend_ThemeChanged();
}

static void ColoursScreen_AdjustSelected(struct LScreen* s, int delta) {
	struct LInput* w;
	int index, newVal;
	cc_uint8 value;

	if (!s->selectedWidget) return;
	index = LScreen_IndexOf(s, s->selectedWidget);
	if (index >= 15) return;

	w = (struct LInput*)s->selectedWidget;
	if (!Convert_ParseUInt8(&w->text, &value)) return;
	newVal = value + delta;

	Math_Clamp(newVal, 0, 255);
	ColoursScreen_Set(w, newVal);
	ColoursScreen_TextChanged(w);
}

static void ColoursScreen_MouseWheel(struct LScreen* s_, float delta) {
	struct ColoursScreen* s = (struct ColoursScreen*)s_;
	int steps = Utils_AccumulateWheelDelta(&s->colourAcc, delta);
	ColoursScreen_AdjustSelected(s_, steps);
}

static void ColoursScreen_KeyDown(struct LScreen* s, int key, cc_bool was) {
	if (key == KEY_LEFT) {
		ColoursScreen_AdjustSelected(s, -1);
	} else if (key == KEY_RIGHT) {
		ColoursScreen_AdjustSelected(s, +1);
	} else if (key == KEY_UP) {
		ColoursScreen_AdjustSelected(s, +10);
	} else if (key == KEY_DOWN) {
		ColoursScreen_AdjustSelected(s, -10);
	} else {
		LScreen_KeyDown(s, key, was);
	}
}

static void ColoursScreen_ToggleBG(struct LCheckbox* w) {
	Launcher_Theme.ClassicBackground = w->value;
	Launcher_SaveTheme();
	LBackend_ThemeChanged();
}

static void ColoursScreen_Init(struct LScreen* s_) {
	struct ColoursScreen* s = (struct ColoursScreen*)s_;
	int i;
	s->widgets    = colours_widgets;
	s->numWidgets = Array_Elems(colours_widgets);

	for (i = 0; i < 5 * 3; i++) {
		s->iptColours[i].inputType   = KEYBOARD_TYPE_INTEGER;
		s->iptColours[i].TextChanged = ColoursScreen_TextChanged;
		LInput_Init(&s->iptColours[i], 55, NULL, clr_iptColours[i]);
	}

	LLabel_Init( &s->lblNames[0], "Background",       clr_lblNames0);
	LLabel_Init( &s->lblNames[1], "Button border",    clr_lblNames1);
	LLabel_Init( &s->lblNames[2], "Button highlight", clr_lblNames2);
	LLabel_Init( &s->lblNames[3], "Button",           clr_lblNames3);
	LLabel_Init( &s->lblNames[4], "Active button",    clr_lblNames4);

	LLabel_Init( &s->lblRGB[0], "Red",        clr_lblRGB0);
	LLabel_Init( &s->lblRGB[1], "Green",      clr_lblRGB1);
	LLabel_Init( &s->lblRGB[2], "Blue",       clr_lblRGB2);
	LButton_Init(&s->btnBack, 80, 35, "Back", clr_btnBack);

	LCheckbox_Init(&s->cbClassic, "Classic style", clr_cbClassic);
	s->cbClassic.ValueChanged = ColoursScreen_ToggleBG;
	s->btnBack.OnClick = SwitchToThemes;
}

static void ColoursScreen_Show(struct LScreen* s_) {
	struct ColoursScreen* s = (struct ColoursScreen*)s_;
	s->colourAcc = 0;

	LCheckbox_Set(&s->cbClassic, Launcher_Theme.ClassicBackground);
	ColoursScreen_UpdateAll(s);
}

void ColoursScreen_SetActive(void) {
	struct ColoursScreen* s = &ColoursScreen;
	LScreen_Reset((struct LScreen*)s);
	s->Init       = ColoursScreen_Init;
	s->Show       = ColoursScreen_Show;
	s->KeyDown    = ColoursScreen_KeyDown;
	s->MouseWheel = ColoursScreen_MouseWheel;

	s->title      = "Custom theme";
	Launcher_SetScreen((struct LScreen*)s);
}


/*########################################################################################################################*
*------------------------------------------------------DirectConnectScreen------------------------------------------------*
*#########################################################################################################################*/
static struct DirectConnectScreen {
	LScreen_Layout
	struct LButton btnConnect, btnBack;
	struct LInput iptUsername, iptAddress, iptMppass;
	struct LLabel lblStatus;
} DirectConnectScreen;

static struct LWidget* directConnect_widgets[] = {
	(struct LWidget*)&DirectConnectScreen.iptUsername,  (struct LWidget*)&DirectConnectScreen.iptAddress,
	(struct LWidget*)&DirectConnectScreen.iptMppass,    (struct LWidget*)&DirectConnectScreen.btnConnect,
	(struct LWidget*)&DirectConnectScreen.btnBack,      (struct LWidget*)&DirectConnectScreen.lblStatus
};

LAYOUTS dc_iptUsername[] = { { ANCHOR_CENTRE_MIN, -165 }, { ANCHOR_CENTRE, -120 } };
LAYOUTS dc_iptAddress[]  = { { ANCHOR_CENTRE_MIN, -165 }, { ANCHOR_CENTRE,  -75 } };
LAYOUTS dc_iptMppass[]   = { { ANCHOR_CENTRE_MIN, -165 }, { ANCHOR_CENTRE,  -30 } };

LAYOUTS dc_btnConnect[]  = { { ANCHOR_CENTRE, -110 }, { ANCHOR_CENTRE, 20 } };
LAYOUTS dc_btnBack[]     = { { ANCHOR_CENTRE,  125 }, { ANCHOR_CENTRE, 20 } };
LAYOUTS dc_lblStatus[]   = { { ANCHOR_CENTRE,    0 }, { ANCHOR_CENTRE, 70 } };


static void DirectConnectScreen_UrlFilter(cc_string* str) {
	static const cc_string prefix = String_FromConst("mc://");
	cc_string parts[6];
	if (!String_CaselessStarts(str, &prefix)) return;
	/* mc://[ip:port]/[username]/[mppass] */
	if (String_UNSAFE_Split(str, '/', parts, 6) != 5) return;
	
	LInput_SetString(&DirectConnectScreen.iptAddress,  &parts[2]);
	LInput_SetString(&DirectConnectScreen.iptUsername, &parts[3]);
	LInput_SetString(&DirectConnectScreen.iptMppass,   &parts[4]);
	str->length = 0;
}

static void DirectConnectScreen_Load(struct DirectConnectScreen* s) {
	cc_string addr; char addrBuffer[STRING_SIZE];
	cc_string mppass; char mppassBuffer[STRING_SIZE];
	cc_string user, ip, port;
	Options_Reload();

	Options_UNSAFE_Get("launcher-dc-username", &user);
	Options_UNSAFE_Get("launcher-dc-ip",       &ip);
	Options_UNSAFE_Get("launcher-dc-port",     &port);
	
	String_InitArray(mppass, mppassBuffer);
	Options_GetSecure("launcher-dc-mppass", &mppass);
	String_InitArray(addr, addrBuffer);
	String_Format2(&addr, "%s:%s", &ip, &port);

	/* don't want just ':' for address */
	if (addr.length == 1) addr.length = 0;
	
	LInput_SetText(&s->iptUsername, &user);
	LInput_SetText(&s->iptAddress,  &addr);
	LInput_SetText(&s->iptMppass,   &mppass);
}

static void DirectConnectScreen_StartClient(void* w) {
	static const cc_string defMppass = String_FromConst("(none)");
	static const cc_string defPort   = String_FromConst("25565");
	const cc_string* user   = &DirectConnectScreen.iptUsername.text;
	const cc_string* addr   = &DirectConnectScreen.iptAddress.text;
	const cc_string* mppass = &DirectConnectScreen.iptMppass.text;
	struct LLabel* status   = &DirectConnectScreen.lblStatus;

	cc_string ip, port;
	cc_uint16 raw_port;

	int index = String_LastIndexOf(addr, ':');
	if (index == 0 || index == addr->length - 1) {
		LLabel_SetConst(status, "&cInvalid address"); return;
	}

	/* support either "[IP]" or "[IP]:[PORT]" */
	if (index == -1) {
		ip   = *addr;
		port = defPort;
	} else {
		ip   = String_UNSAFE_Substring(addr, 0, index);
		port = String_UNSAFE_SubstringAt(addr, index + 1);
	}

	if (!user->length) {
		LLabel_SetConst(status, "&cUsername required"); return;
	}
	if (!Socket_ValidAddress(&ip)) {
		LLabel_SetConst(status, "&cInvalid ip"); return;
	}
	if (!Convert_ParseUInt16(&port, &raw_port)) {
		LLabel_SetConst(status, "&cInvalid port"); return;
	}
	if (!mppass->length) mppass = &defMppass;

	Options_PauseSaving();
	Options_Set("launcher-dc-username", user);
	Options_Set("launcher-dc-ip",       &ip);
	Options_Set("launcher-dc-port",     &port);
	Options_SetSecure("launcher-dc-mppass", mppass);

	LLabel_SetConst(status, "");
	Launcher_StartGame(user, mppass, &ip, &port, &String_Empty);
}

static void DirectConnectScreen_Init(struct LScreen* s_) {
	struct DirectConnectScreen* s = (struct DirectConnectScreen*)s_;
	s->widgets    = directConnect_widgets;
	s->numWidgets = Array_Elems(directConnect_widgets);

	LInput_Init(&s->iptUsername, 330, "Username..",               dc_iptUsername);
	LInput_Init(&s->iptAddress,  330, "IP address:Port number..", dc_iptAddress);
	LInput_Init(&s->iptMppass,   330, "Mppass..",                 dc_iptMppass);

	LButton_Init(&s->btnConnect, 110, 35, "Connect", dc_btnConnect);
	LButton_Init(&s->btnBack,     80, 35, "Back",    dc_btnBack);
	LLabel_Init( &s->lblStatus,  "",                 dc_lblStatus);

	s->iptUsername.ClipboardFilter = DirectConnectScreen_UrlFilter;
	s->iptAddress.ClipboardFilter  = DirectConnectScreen_UrlFilter;
	s->iptMppass.ClipboardFilter   = DirectConnectScreen_UrlFilter;

	s->btnConnect.OnClick = DirectConnectScreen_StartClient;
	s->btnBack.OnClick    = SwitchToMain;
	/* Init input text from options */
	DirectConnectScreen_Load(s);
}

void DirectConnectScreen_SetActive(void) {
	struct DirectConnectScreen* s = &DirectConnectScreen;
	LScreen_Reset((struct LScreen*)s);
	s->Init = DirectConnectScreen_Init;

	s->title         = "Direct connect";
	s->onEnterWidget = (struct LWidget*)&s->btnConnect;
	Launcher_SetScreen((struct LScreen*)s);
}


/*########################################################################################################################*
*----------------------------------------------------------MFAScreen------------------------------------------------------*
*#########################################################################################################################*/
static struct MFAScreen {
	LScreen_Layout
	struct LInput iptCode;
	struct LButton btnSignIn, btnCancel;
	struct LLabel  lblTitle;
} MFAScreen;

static struct LWidget* mfa_widgets[] = {
	(struct LWidget*)&MFAScreen.lblTitle,  (struct LWidget*)&MFAScreen.iptCode,
	(struct LWidget*)&MFAScreen.btnSignIn, (struct LWidget*)&MFAScreen.btnCancel
};

LAYOUTS mfa_lblTitle[]  = { { ANCHOR_CENTRE,   0 }, { ANCHOR_CENTRE, -115 } };
LAYOUTS mfa_iptCode[]   = { { ANCHOR_CENTRE,   0 }, { ANCHOR_CENTRE,  -75 } };
LAYOUTS mfa_btnSignIn[] = { { ANCHOR_CENTRE, -90 }, { ANCHOR_CENTRE,  -25 } };
LAYOUTS mfa_btnCancel[] = { { ANCHOR_CENTRE,  90 }, { ANCHOR_CENTRE,  -25 } };


static void MainScreen_DoLogin(void);
static void MFAScreen_SignIn(void* w) {
	MainScreen_SetActive();
	MainScreen_DoLogin();
}
static void MFAScreen_Cancel(void* w) {
	LInput_ClearText(&MFAScreen.iptCode);
	MainScreen_SetActive();
}

static void MFAScreen_Init(struct LScreen* s_) {
	struct MFAScreen* s = (struct MFAScreen*)s_;
	s->widgets    = mfa_widgets;
	s->numWidgets = Array_Elems(mfa_widgets);
	
	LLabel_Init( &s->lblTitle,  "",                  mfa_lblTitle);
	LInput_Init( &s->iptCode,   280, "Login code..", mfa_iptCode);
	LButton_Init(&s->btnSignIn, 100, 35, "Sign in",  mfa_btnSignIn);
	LButton_Init(&s->btnCancel, 100, 35, "Cancel",   mfa_btnCancel);

	s->btnSignIn.OnClick = MFAScreen_SignIn;
	s->btnCancel.OnClick = MFAScreen_Cancel;
	s->iptCode.inputType = KEYBOARD_TYPE_INTEGER;
}

static void MFAScreen_Show(struct LScreen* s_) {
	struct MFAScreen* s = (struct MFAScreen*)s_;
	LLabel_SetConst(&s->lblTitle, s->iptCode.text.length ?
		"&cWrong code entered  (Check emails)" :
		"&cLogin code required (Check emails)");
}

void MFAScreen_SetActive(void) {
	struct MFAScreen* s = &MFAScreen;
	LScreen_Reset((struct LScreen*)s);
	s->Init = MFAScreen_Init;
	s->Show = MFAScreen_Show;

	s->title         = "Enter login code";
	s->onEnterWidget = (struct LWidget*)&s->btnSignIn;
	Launcher_SetScreen((struct LScreen*)s);
}


/*########################################################################################################################*
*----------------------------------------------------------MainScreen-----------------------------------------------------*
*#########################################################################################################################*/
static struct MainScreen {
	LScreen_Layout
	struct LButton btnLogin, btnResume, btnDirect, btnSPlayer, btnRegister, btnOptions, btnUpdates;
	struct LInput iptUsername, iptPassword;
	struct LLabel lblStatus, lblUpdate;
	cc_bool signingIn;
} MainScreen;

static struct LWidget* main_widgets[] = {
	(struct LWidget*)&MainScreen.iptUsername, (struct LWidget*)&MainScreen.iptPassword,
	(struct LWidget*)&MainScreen.btnLogin,    (struct LWidget*)&MainScreen.btnResume,
	(struct LWidget*)&MainScreen.lblStatus,   (struct LWidget*)&MainScreen.btnDirect,
	(struct LWidget*)&MainScreen.btnSPlayer,  (struct LWidget*)&MainScreen.lblUpdate,
	(struct LWidget*)&MainScreen.btnRegister, (struct LWidget*)&MainScreen.btnOptions,
	(struct LWidget*)&MainScreen.btnUpdates
};

LAYOUTS main_iptUsername[] = { { ANCHOR_CENTRE_MIN, -140 }, { ANCHOR_CENTRE, -120 } };
LAYOUTS main_iptPassword[] = { { ANCHOR_CENTRE_MIN, -140 }, { ANCHOR_CENTRE,  -75 } };

LAYOUTS main_btnLogin[]  = { { ANCHOR_CENTRE, -90 }, { ANCHOR_CENTRE, -25 } };
LAYOUTS main_lblStatus[] = { { ANCHOR_CENTRE,   0 }, { ANCHOR_CENTRE,  20 } };

LAYOUTS main_btnResume[]  = { { ANCHOR_CENTRE, 90 }, { ANCHOR_CENTRE, -25 } };
LAYOUTS main_btnDirect[]  = { { ANCHOR_CENTRE,  0 }, { ANCHOR_CENTRE,  60 } };
LAYOUTS main_btnSPlayer[] = { { ANCHOR_CENTRE,  0 }, { ANCHOR_CENTRE, 110 } };

LAYOUTS main_lblUpdate[]   = { { ANCHOR_MAX,   10 }, { ANCHOR_MAX, 45 } };
LAYOUTS main_btnRegister[] = { { ANCHOR_MIN,    6 }, { ANCHOR_MAX,  6 } };
LAYOUTS main_btnOptions[]  = { { ANCHOR_CENTRE, 0 }, { ANCHOR_MAX,  6 } };
LAYOUTS main_btnUpdates[]  = { { ANCHOR_MAX,    6 }, { ANCHOR_MAX,  6 } };


struct ResumeInfo {
	cc_string user, ip, port, server, mppass;
	char _userBuffer[STRING_SIZE], _serverBuffer[STRING_SIZE];
	char _ipBuffer[16], _portBuffer[16], _mppassBuffer[STRING_SIZE];
	cc_bool valid;
};

CC_NOINLINE static void MainScreen_GetResume(struct ResumeInfo* info, cc_bool full) {
	String_InitArray(info->server, info->_serverBuffer);
	Options_Get(ROPT_SERVER,       &info->server, "");
	String_InitArray(info->user,   info->_userBuffer);
	Options_Get(ROPT_USER,         &info->user, "");

	String_InitArray(info->ip,   info->_ipBuffer);
	Options_Get(ROPT_IP,         &info->ip, "");
	String_InitArray(info->port, info->_portBuffer);
	Options_Get(ROPT_PORT,       &info->port, "");

	if (!full) return;
	String_InitArray(info->mppass, info->_mppassBuffer);
	Options_GetSecure(ROPT_MPPASS, &info->mppass);

	info->valid = 
		info->user.length && info->mppass.length &&
		info->ip.length   && info->port.length;
}

CC_NOINLINE static void MainScreen_Error(struct HttpRequest* req, const char* action) {
	cc_string str; char strBuffer[STRING_SIZE];
	struct MainScreen* s = &MainScreen;
	String_InitArray(str, strBuffer);

	Launcher_DisplayHttpError(req, action, &str);
	LLabel_SetText(&s->lblStatus, &str);
	s->signingIn = false;
}

static void MainScreen_DoLogin(void) {
	struct MainScreen* s = &MainScreen;
	cc_string* user = &s->iptUsername.text;
	cc_string* pass = &s->iptPassword.text;

	if (!user->length) {
		LLabel_SetConst(&s->lblStatus, "&cUsername required"); return;
	}
	if (!pass->length) {
		LLabel_SetConst(&s->lblStatus, "&cPassword required"); return;
	}

	if (GetTokenTask.Base.working) return;
	Options_Set(LOPT_USERNAME, user);
	Options_SetSecure(LOPT_PASSWORD, pass);

	GetTokenTask_Run();
	LLabel_SetConst(&s->lblStatus, "&eSigning in..");
	s->signingIn = true;
}
static void MainScreen_Login(void* w) { MainScreen_DoLogin(); }

static void MainScreen_Register(void* w) {
	static const cc_string regUrl = String_FromConst(REGISTERNEW_URL);
	cc_result res = Process_StartOpen(&regUrl);
	if (res) Logger_SimpleWarn(res, "opening register webpage in browser");
}

static void MainScreen_Resume(void* w) {
	struct ResumeInfo info;
	MainScreen_GetResume(&info, true);

	if (!info.valid) return;
	Launcher_StartGame(&info.user, &info.mppass, &info.ip, &info.port, &info.server);
}

static void MainScreen_Singleplayer(void* w) {
	static const cc_string defUser = String_FromConst(DEFAULT_USERNAME);
	const cc_string* user = &MainScreen.iptUsername.text;

	if (!user->length) user = &defUser;
	Launcher_StartGame(user, &String_Empty, &String_Empty, &String_Empty, &String_Empty);
}

static void MainScreen_ResumeHover(void* w) {
	cc_string str; char strBuffer[STRING_SIZE];
	struct MainScreen* s = &MainScreen;
	struct ResumeInfo info;
	if (s->signingIn) return;

	MainScreen_GetResume(&info, false);
	if (!info.user.length) return;
	String_InitArray(str, strBuffer);

	if (info.server.length && String_Equals(&info.user,	&s->iptUsername.text)) {
		String_Format1(&str, "&eResume to %s", &info.server);
	} else if (info.server.length) {
		String_Format2(&str, "&eResume as %s to %s", &info.user, &info.server);
	} else {
		String_Format3(&str, "&eResume as %s to %s:%s", &info.user, &info.ip, &info.port);
	}
	LLabel_SetText(&s->lblStatus, &str);
}

static void MainScreen_ResumeUnhover(void* w) {
	struct MainScreen* s = &MainScreen;
	if (s->signingIn) return;

	LLabel_SetConst(&s->lblStatus, "");
}

static void MainScreen_Init(struct LScreen* s_) {
	cc_string user, pass; char passBuffer[STRING_SIZE];
	struct MainScreen* s = (struct MainScreen*)s_;
	s->widgets           = main_widgets;
	s->numWidgets        = Array_Elems(main_widgets);

	s->iptPassword.inputType = KEYBOARD_TYPE_PASSWORD;
	s->lblUpdate.small       = true;

	LInput_Init( &s->iptUsername, 280, "Username..",  main_iptUsername);
	LInput_Init( &s->iptPassword, 280, "Password..",  main_iptPassword);
	LButton_Init(&s->btnLogin,    100, 35, "Sign in", main_btnLogin);
	LButton_Init(&s->btnResume,   100, 35, "Resume",  main_btnResume);

	LLabel_Init( &s->lblStatus,  "",                        main_lblStatus);
	LButton_Init(&s->btnDirect,  200, 35, "Direct connect", main_btnDirect);
	LButton_Init(&s->btnSPlayer, 200, 35, "Singleplayer",   main_btnSPlayer);

	LLabel_Init( &s->lblUpdate,   "&eChecking..",      main_lblUpdate);
	LButton_Init(&s->btnRegister, 100, 35, "Register", main_btnRegister);
	LButton_Init(&s->btnOptions,  100, 35, "Options",  main_btnOptions);
	LButton_Init(&s->btnUpdates,  100, 35, "Updates",  main_btnUpdates);
	
	s->btnLogin.OnClick    = MainScreen_Login;
	s->btnResume.OnClick   = MainScreen_Resume;
	s->btnDirect.OnClick   = SwitchToDirectConnect;
	s->btnSPlayer.OnClick  = MainScreen_Singleplayer;
	s->btnRegister.OnClick = MainScreen_Register;
	s->btnOptions.OnClick  = SwitchToSettings;
	s->btnUpdates.OnClick  = SwitchToUpdates;

	s->btnResume.OnHover   = MainScreen_ResumeHover;
	s->btnResume.OnUnhover = MainScreen_ResumeUnhover;
	
	String_InitArray(pass, passBuffer);
	Options_UNSAFE_Get(LOPT_USERNAME, &user);
	Options_GetSecure(LOPT_PASSWORD, &pass);

	LInput_SetText(&s->iptUsername, &user);
	LInput_SetText(&s->iptPassword, &pass);

	/* Auto sign in when automatically joining a server */
	if (!Launcher_AutoHash.length)    return;
	if (!user.length || !pass.length) return;
	MainScreen_DoLogin();
}

static void MainScreen_Free(struct LScreen* s_) {
	struct MainScreen* s = (struct MainScreen*)s_;
	/* status should reset when user goes to another menu */
	LLabel_SetConst(&s->lblStatus, "");
}

CC_NOINLINE static cc_uint32 MainScreen_GetVersion(const cc_string* version) {
	cc_uint8 raw[4] = { 0, 0, 0, 0 };
	cc_string parts[4];
	int i, count;
	
	/* 1.0.1 -> { 1, 0, 1, 0 } */
	count = String_UNSAFE_Split(version, '.', parts, 4);
	for (i = 0; i < count; i++) {
		Convert_ParseUInt8(&parts[i], &raw[i]);
	}
	return Stream_GetU32_BE(raw);
}

static void MainScreen_TickCheckUpdates(struct MainScreen* s) {
	static const cc_string currentStr = String_FromConst(GAME_APP_VER);
	cc_uint32 latest, current;

	if (!CheckUpdateTask.Base.working)   return;
	LWebTask_Tick(&CheckUpdateTask.Base, NULL);
	if (!CheckUpdateTask.Base.completed) return;

	if (CheckUpdateTask.Base.success) {
		latest  = MainScreen_GetVersion(&CheckUpdateTask.latestRelease);
		current = MainScreen_GetVersion(&currentStr);
		LLabel_SetConst(&s->lblUpdate, latest > current ? "&aNew release" : "&eUp to date");
	} else {
		LLabel_SetConst(&s->lblUpdate, "&cCheck failed");
	}
}

static void MainScreen_LoginPhase2(struct MainScreen* s, const cc_string* user) {
	/* website returns case correct username */
	if (!String_Equals(&s->iptUsername.text, user)) {
		LInput_SetText(&s->iptUsername, user);
	}
	String_Copy(&Launcher_Username, user);

	FetchServersTask_Run();
	LLabel_SetConst(&s->lblStatus, "&eRetrieving servers list..");
}

static void MainScreen_SignInError(struct HttpRequest* req) {
	MainScreen_Error(req, "signing in");
}

static void MainScreen_TickGetToken(struct MainScreen* s) {
	if (!GetTokenTask.Base.working)   return;
	LWebTask_Tick(&GetTokenTask.Base, MainScreen_SignInError);

	if (!GetTokenTask.Base.completed) return;
	if (!GetTokenTask.Base.success)   return;

	if (!GetTokenTask.error && String_CaselessEquals(&GetTokenTask.username, &s->iptUsername.text)) {
		/* Already logged in, go straight to fetching servers */
		MainScreen_LoginPhase2(s, &GetTokenTask.username);
	} else {
		SignInTask_Run(&s->iptUsername.text, &s->iptPassword.text,
						&MFAScreen.iptCode.text);
	}
}

static void MainScreen_TickSignIn(struct MainScreen* s) {
	if (!SignInTask.Base.working)   return;
	LWebTask_Tick(&SignInTask.Base, MainScreen_SignInError);
	if (!SignInTask.Base.completed) return;

	if (SignInTask.needMFA) { MFAScreen_SetActive(); return; }

	if (SignInTask.error) {
		LLabel_SetConst(&s->lblStatus, SignInTask.error);
	} else if (SignInTask.Base.success) {
		MainScreen_LoginPhase2(s, &SignInTask.username);
	}
}

static void MainScreen_ServersError(struct HttpRequest* req) {
	MainScreen_Error(req, "retrieving servers list");
}

static void MainScreen_TickFetchServers(struct MainScreen* s) {
	if (!FetchServersTask.Base.working)   return;
	LWebTask_Tick(&FetchServersTask.Base, MainScreen_ServersError);

	if (!FetchServersTask.Base.completed) return;
	if (!FetchServersTask.Base.success)   return;

	s->signingIn = false;
	if (Launcher_AutoHash.length) {
		Launcher_ConnectToServer(&Launcher_AutoHash);
		Launcher_AutoHash.length = 0;
	} else {
		ServersScreen_SetActive();
	}
}

static void MainScreen_Tick(struct LScreen* s_) {
	struct MainScreen* s = (struct MainScreen*)s_;
	LScreen_Tick(s_);

	MainScreen_TickCheckUpdates(s);
	MainScreen_TickGetToken(s);
	MainScreen_TickSignIn(s);
	MainScreen_TickFetchServers(s);
}

void MainScreen_SetActive(void) {
	struct MainScreen* s = &MainScreen;
	LScreen_Reset((struct LScreen*)s);
	s->Init = MainScreen_Init;
	s->Free = MainScreen_Free;
	s->Tick = MainScreen_Tick;\

	s->title         = "ClassiCube";
	s->onEnterWidget = (struct LWidget*)&s->btnLogin;
	Launcher_SetScreen((struct LScreen*)s);
}


/*########################################################################################################################*
*----------------------------------------------------CheckResourcesScreen-------------------------------------------------*
*#########################################################################################################################*/
static struct CheckResourcesScreen {
	LScreen_Layout
	struct LLabel  lblLine1, lblLine2, lblStatus;
	struct LButton btnYes, btnNo;
} CheckResourcesScreen;

static struct LWidget* checkResources_widgets[] = {
	(struct LWidget*)&CheckResourcesScreen.lblLine1,  (struct LWidget*)&CheckResourcesScreen.lblLine2,
	(struct LWidget*)&CheckResourcesScreen.lblStatus, (struct LWidget*)&CheckResourcesScreen.btnYes,
	(struct LWidget*)&CheckResourcesScreen.btnNo
};

LAYOUTS cres_lblLine1[]  = { { ANCHOR_CENTRE, 0 }, { ANCHOR_CENTRE, -50 } };
LAYOUTS cres_lblLine2[]  = { { ANCHOR_CENTRE, 0 }, { ANCHOR_CENTRE, -30 } };
LAYOUTS cres_lblStatus[] = { { ANCHOR_CENTRE, 0 }, { ANCHOR_CENTRE,  10 } };

LAYOUTS cres_btnYes[] = { { ANCHOR_CENTRE, -70 }, { ANCHOR_CENTRE, 45 } };
LAYOUTS cres_btnNo[]  = { { ANCHOR_CENTRE,  70 }, { ANCHOR_CENTRE, 45 } };


static void CheckResourcesScreen_Yes(void*  w) { FetchResourcesScreen_SetActive(); }
static void CheckResourcesScreen_Next(void* w) {
	Http_ClearPending();
	if (Options_LoadResult != ReturnCode_FileNotFound) {
		MainScreen_SetActive();
	} else {
		ChooseModeScreen_SetActive(true);
	}
}

static void CheckResourcesScreen_Init(struct LScreen* s_) {
	struct CheckResourcesScreen* s = (struct CheckResourcesScreen*)s_;
	s->widgets    = checkResources_widgets;
	s->numWidgets = Array_Elems(checkResources_widgets);
	s->lblStatus.small = true;

	LLabel_Init( &s->lblLine1,  "Some required resources weren't found", cres_lblLine1);
	LLabel_Init( &s->lblLine2,  "Okay to download?", cres_lblLine2);
	LLabel_Init( &s->lblStatus, "",                  cres_lblStatus);

	LButton_Init(&s->btnYes, 70, 35, "Yes", cres_btnYes);
	LButton_Init(&s->btnNo,  70, 35, "No",  cres_btnNo);
	s->btnYes.OnClick = CheckResourcesScreen_Yes;
	s->btnNo.OnClick  = CheckResourcesScreen_Next;
}

static void CheckResourcesScreen_Show(struct LScreen* s_) {
	cc_string str; char buffer[STRING_SIZE];
	struct CheckResourcesScreen* s = (struct CheckResourcesScreen*)s_;
	float size = Resources_Size / 1024.0f;

	String_InitArray(str, buffer);
	String_Format1(&str, "&eDownload size: %f2 megabytes", &size);
	LLabel_SetText(&s->lblStatus, &str);
}

#define RESOURCES_FORE_COLOR BitmapColor_RGB(120,  85, 151)
static void CheckResourcesScreen_ResetArea(struct Context2D* ctx, int x, int y, int width, int height) {
	Gradient_Noise(ctx, RESOURCES_FORE_COLOR, 4, x, y, width, height);
}

static void CheckResourcesScreen_DrawBackground(struct LScreen* s, struct Context2D* ctx) {
	int x, y, width, height;
	BitmapCol color = BitmapColor_Scale(Launcher_Theme.BackgroundColor, 0.2f);
	Context2D_Clear(ctx, color, 0, 0, ctx->width, ctx->height);
	width  = Display_ScaleX(380);
	height = Display_ScaleY(140);

	x = Gui_CalcPos(ANCHOR_CENTRE, 0, width,  ctx->width);
	y = Gui_CalcPos(ANCHOR_CENTRE, 0, height, ctx->height);
	CheckResourcesScreen_ResetArea(ctx, x, y, width, height);
}

void CheckResourcesScreen_SetActive(void) {
	struct CheckResourcesScreen* s = &CheckResourcesScreen;
	LScreen_Reset((struct LScreen*)s);
	s->Init   = CheckResourcesScreen_Init;
	s->Show   = CheckResourcesScreen_Show;
	s->DrawBackground = CheckResourcesScreen_DrawBackground;
	s->ResetArea      = CheckResourcesScreen_ResetArea;
	s->onEnterWidget  = (struct LWidget*)&s->btnYes;
	Launcher_SetScreen((struct LScreen*)s);
}


/*########################################################################################################################*
*----------------------------------------------------FetchResourcesScreen-------------------------------------------------*
*#########################################################################################################################*/
static struct FetchResourcesScreen {
	LScreen_Layout
	struct LLabel  lblStatus;
	struct LButton btnCancel;
	struct LSlider sdrProgress;
} FetchResourcesScreen;

static struct LWidget* fetchResources_widgets[] = {
	(struct LWidget*)&FetchResourcesScreen.lblStatus,  (struct LWidget*)&FetchResourcesScreen.btnCancel,
	(struct LWidget*)&FetchResourcesScreen.sdrProgress
};

LAYOUTS fres_lblStatus[]   = { { ANCHOR_CENTRE, 0 }, { ANCHOR_CENTRE, -10 } };
LAYOUTS fres_btnCancel[]   = { { ANCHOR_CENTRE, 0 }, { ANCHOR_CENTRE,  45 } };
LAYOUTS fres_sdrProgress[] = { { ANCHOR_CENTRE, 0 }, { ANCHOR_CENTRE,  15 } };


static void FetchResourcesScreen_Init(struct LScreen* s_) {
	struct FetchResourcesScreen* s = (struct FetchResourcesScreen*)s_;
	s->widgets    = fetchResources_widgets;
	s->numWidgets = Array_Elems(fetchResources_widgets);
	s->lblStatus.small = true;

	LLabel_Init( &s->lblStatus,   "",                                  fres_lblStatus);
	LButton_Init(&s->btnCancel,   120, 35, "Cancel",                   fres_btnCancel);
	LSlider_Init(&s->sdrProgress, 200, 12, BitmapColor_RGB(0, 220, 0), fres_sdrProgress);

	s->btnCancel.OnClick = CheckResourcesScreen_Next;
}

static void FetchResourcesScreen_Error(struct HttpRequest* req) {
	cc_string str; char buffer[STRING_SIZE];
	String_InitArray(str, buffer);

	Launcher_DisplayHttpError(req, "downloading resources", &str);
	LLabel_SetText(&FetchResourcesScreen.lblStatus, &str);
}

static void FetchResourcesScreen_Show(struct LScreen* s_) {
	Fetcher_ErrorCallback = FetchResourcesScreen_Error;
	Fetcher_Run(); 
}

static void FetchResourcesScreen_UpdateStatus(struct FetchResourcesScreen* s, int reqID) {
	cc_string str; char strBuffer[STRING_SIZE];
	const char* name;
	int count;

	name = Fetcher_RequestName(reqID);
	if (!name) return;

	String_InitArray(str, strBuffer);
	count = Fetcher_Downloaded + 1;
	String_Format3(&str, "&eFetching %c.. (%i/%i)", name, &count, &Resources_Count);

	if (String_Equals(&str, &s->lblStatus.text)) return;
	LLabel_SetText(&s->lblStatus, &str);
}

static void FetchResourcesScreen_UpdateProgress(struct FetchResourcesScreen* s) {
	int reqID, progress;

	if (!Http_GetCurrent(&reqID, &progress)) return;
	FetchResourcesScreen_UpdateStatus(s, reqID);
	/* making request still, haven't started download yet */
	if (progress < 0 || progress > 100) return;

	LSlider_SetProgress(&s->sdrProgress, progress);
}

static void FetchResourcesScreen_Tick(struct LScreen* s_) {
	struct FetchResourcesScreen* s = (struct FetchResourcesScreen*)s_;
	if (!Fetcher_Working) return;

	FetchResourcesScreen_UpdateProgress(s);
	Fetcher_Update();

	if (!Fetcher_Completed) return;
	if (Fetcher_Failed)     return;

	Launcher_TryLoadTexturePack();
	CheckResourcesScreen_Next(NULL);
}

void FetchResourcesScreen_SetActive(void) {
	struct FetchResourcesScreen* s = &FetchResourcesScreen;
	LScreen_Reset((struct LScreen*)s);
	s->Init = FetchResourcesScreen_Init;
	s->Show = FetchResourcesScreen_Show;
	s->Tick = FetchResourcesScreen_Tick;
	s->DrawBackground = CheckResourcesScreen_DrawBackground;
	s->ResetArea      = CheckResourcesScreen_ResetArea;
	Launcher_SetScreen((struct LScreen*)s);
}


/*########################################################################################################################*
*--------------------------------------------------------ServersScreen----------------------------------------------------*
*#########################################################################################################################*/
static struct ServersScreen {
	LScreen_Layout
	struct LInput iptSearch, iptHash;
	struct LButton btnBack, btnConnect, btnRefresh;
	struct LTable table;
	struct FontDesc rowFont;
	float tableAcc;
} ServersScreen;

static struct LWidget* servers_widgets[] = {
	(struct LWidget*)&ServersScreen.iptSearch,  (struct LWidget*)&ServersScreen.iptHash,
	(struct LWidget*)&ServersScreen.btnBack,    (struct LWidget*)&ServersScreen.btnConnect,
	(struct LWidget*)&ServersScreen.btnRefresh, (struct LWidget*)&ServersScreen.table
};

LAYOUTS srv_iptSearch[] = { { ANCHOR_MIN, 10 }, { ANCHOR_MIN, 10 } };
LAYOUTS srv_iptHash[]   = { { ANCHOR_MIN, 10 }, { ANCHOR_MAX, 10 } };
LAYOUTS srv_table[5]    = { { ANCHOR_MIN, 10 }, { ANCHOR_MIN | LLAYOUT_EXTRA, 50 }, { LLAYOUT_WIDTH, 0 }, { LLAYOUT_HEIGHT, 50 } };

LAYOUTS srv_btnBack[]    = { { ANCHOR_MAX,  10 }, { ANCHOR_MIN, 10 } };
LAYOUTS srv_btnConnect[] = { { ANCHOR_MAX,  10 }, { ANCHOR_MAX, 10 } };
LAYOUTS srv_btnRefresh[] = { { ANCHOR_MAX, 135 }, { ANCHOR_MIN, 10 } };
	

static void ServersScreen_Connect(void* w) {
	struct LTable* table = &ServersScreen.table;
	cc_string* hash      = &ServersScreen.iptHash.text;

	if (!hash->length && table->rowsCount) { 
		hash = &LTable_Get(table->topRow)->hash; 
	}
	Launcher_ConnectToServer(hash);
}

static void ServersScreen_Refresh(void* w) {
	struct LButton* btn;
	if (FetchServersTask.Base.working) return;

	FetchServersTask_Run();
	btn = &ServersScreen.btnRefresh;
	LButton_SetConst(btn, "&eWorking..");
}

static void ServersScreen_HashFilter(cc_string* str) {
	int lastIndex;
	if (!str->length) return;

	/* Server url look like http://www.classicube.net/server/play/aaaaa/ */
	/* Trim it to only be the aaaaa */
	if (str->buffer[str->length - 1] == '/') str->length--;

	/* Trim the URL parts before the hash */
	lastIndex = String_LastIndexOf(str, '/');
	if (lastIndex == -1) return;
	*str = String_UNSAFE_SubstringAt(str, lastIndex + 1);
}

static void ServersScreen_SearchChanged(struct LInput* w) {
	struct ServersScreen* s = &ServersScreen;
	LTable_ApplyFilter(&s->table);
	LBackend_MarkDirty(&s->table);
}

static void ServersScreen_HashChanged(struct LInput* w) {
	struct ServersScreen* s = &ServersScreen;
	LTable_ShowSelected(&s->table);
	LBackend_MarkDirty(&s->table);
}

static void ServersScreen_OnSelectedChanged(void) {
	struct ServersScreen* s = &ServersScreen;
	LBackend_MarkDirty(&s->iptHash);
	LBackend_MarkDirty(&s->table);
}

static void ServersScreen_ReloadServers(struct ServersScreen* s) {
	int i;
	LTable_Sort(&s->table);

	for (i = 0; i < FetchServersTask.numServers; i++) {
		FetchFlagsTask_Add(&FetchServersTask.servers[i]);
	}
}

static void ServersScreen_Init(struct LScreen* s_) {
	struct ServersScreen* s = (struct ServersScreen*)s_;
	s->widgets    = servers_widgets;
	s->numWidgets = Array_Elems(servers_widgets);

	LInput_Init( &s->iptSearch, 370, "Search servers..",               srv_iptSearch);
	LInput_Init( &s->iptHash,   475, "classicube.net/server/play/...", srv_iptHash);

	LButton_Init(&s->btnBack,    110, 30, "Back",    srv_btnBack);
	LButton_Init(&s->btnConnect, 130, 30, "Connect", srv_btnConnect);
	LButton_Init(&s->btnRefresh, 110, 30, "Refresh", srv_btnRefresh);

	s->btnBack.OnClick    = SwitchToMain;
	s->btnConnect.OnClick = ServersScreen_Connect;
	s->btnRefresh.OnClick = ServersScreen_Refresh;

	s->iptSearch.skipsEnter    = true;
	s->iptSearch.TextChanged   = ServersScreen_SearchChanged;
	s->iptHash.TextChanged     = ServersScreen_HashChanged;
	s->iptHash.ClipboardFilter = ServersScreen_HashFilter;

	LTable_Init(&s->table, srv_table);
	s->table.filter       = &s->iptSearch.text;
	s->table.selectedHash = &s->iptHash.text;
	s->table.OnSelectedChanged = ServersScreen_OnSelectedChanged;
}

static void ServersScreen_Show(struct LScreen* s_) {
	struct ServersScreen* s = (struct ServersScreen*)s_;
	LTable_Reset(&s->table);
	LInput_ClearText(&s->iptHash);
	LInput_ClearText(&s->iptSearch);

	ServersScreen_ReloadServers(s);
	/* This is so typing on keyboard by default searchs server list */
	/* But don't do that when it would cause on-screen keyboard to show */
	if (WindowInfo.SoftKeyboard) return;
	LScreen_SelectWidget(s_, 0, (struct LWidget*)&s->iptSearch, false);
}

static void ServersScreen_Tick(struct LScreen* s_) {
	struct ServersScreen* s = (struct ServersScreen*)s_;
	int count;
	LScreen_Tick(s_);

	count = FetchFlagsTask.count;
	LWebTask_Tick(&FetchFlagsTask.Base, NULL);
	if (count != FetchFlagsTask.count) LBackend_TableFlagAdded(&s->table);

	if (!FetchServersTask.Base.working) return;
	LWebTask_Tick(&FetchServersTask.Base, NULL);
	if (!FetchServersTask.Base.completed) return;

	if (FetchServersTask.Base.success) {
		ServersScreen_ReloadServers(s);
		LBackend_MarkDirty(&s->table);
	}

	LButton_SetConst(&s->btnRefresh, 
				FetchServersTask.Base.success ? "Refresh" : "&cFailed");
}

static void ServersScreen_MouseWheel(struct LScreen* s_, float delta) {
	struct ServersScreen* s = (struct ServersScreen*)s_;
	s->table.VTABLE->MouseWheel(&s->table, delta);
}

static void ServersScreen_KeyDown(struct LScreen* s_, int key, cc_bool was) {
	struct ServersScreen* s = (struct ServersScreen*)s_;
	if (!LTable_HandlesKey(key)) {
		LScreen_KeyDown(s_, key, was);
	} else {
		s->table.VTABLE->KeyDown(&s->table, key, was);
	}
}

static void ServersScreen_MouseUp(struct LScreen* s_, int idx) {
	struct ServersScreen* s = (struct ServersScreen*)s_;
	s->table.VTABLE->OnUnselect(&s->table, idx);
}

void ServersScreen_SetActive(void) {
	struct ServersScreen* s = &ServersScreen;
	LScreen_Reset((struct LScreen*)s);
	s->tableAcc = 0.0f;

	s->Init       = ServersScreen_Init;
	s->Show       = ServersScreen_Show;
	s->Tick       = ServersScreen_Tick;
	s->MouseWheel = ServersScreen_MouseWheel;
	s->KeyDown    = ServersScreen_KeyDown;
	s->MouseUp    = ServersScreen_MouseUp;
	s->onEnterWidget = (struct LWidget*)&s->btnConnect;
	Launcher_SetScreen((struct LScreen*)s);
}


/*########################################################################################################################*
*--------------------------------------------------------SettingsScreen---------------------------------------------------*
*#########################################################################################################################*/
static struct SettingsScreen {
	LScreen_Layout
	struct LButton btnMode, btnColours, btnBack;
	struct LLabel  lblMode, lblColours;
	struct LCheckbox cbExtra, cbEmpty, cbScale;
	struct LLine sep;
} SettingsScreen;

static struct LWidget* settings_widgets[] = {
	(struct LWidget*)&SettingsScreen.sep,
	(struct LWidget*)&SettingsScreen.btnMode,    (struct LWidget*)&SettingsScreen.lblMode,
	(struct LWidget*)&SettingsScreen.btnColours, (struct LWidget*)&SettingsScreen.lblColours,
	(struct LWidget*)&SettingsScreen.cbExtra,    (struct LWidget*)&SettingsScreen.cbEmpty,
	(struct LWidget*)&SettingsScreen.btnBack,    (struct LWidget*)&SettingsScreen.cbScale
};
static struct LWidget* settings_classic[] = {
	(struct LWidget*)&SettingsScreen.sep,
	(struct LWidget*)&SettingsScreen.btnMode,    (struct LWidget*)&SettingsScreen.lblMode,
	(struct LWidget*)&SettingsScreen.cbExtra,    (struct LWidget*)&SettingsScreen.cbEmpty,
	(struct LWidget*)&SettingsScreen.btnBack,    (struct LWidget*)&SettingsScreen.cbScale
};

LAYOUTS set_btnMode[]    = { { ANCHOR_CENTRE,     -135 }, { ANCHOR_CENTRE,  -70 } };
LAYOUTS set_lblMode[]    = { { ANCHOR_CENTRE_MIN,  -70 }, { ANCHOR_CENTRE,  -70 } };
LAYOUTS set_btnColours[] = { { ANCHOR_CENTRE,     -135 }, { ANCHOR_CENTRE,  -20 } };
LAYOUTS set_lblColours[] = { { ANCHOR_CENTRE_MIN,  -70 }, { ANCHOR_CENTRE,  -20 } };

LAYOUTS set_sep[]     = { { ANCHOR_CENTRE,        0 }, { ANCHOR_CENTRE,  15 } };
LAYOUTS set_cbExtra[] = { { ANCHOR_CENTRE_MIN, -190 }, { ANCHOR_CENTRE,  44 } };
LAYOUTS set_cbEmpty[] = { { ANCHOR_CENTRE_MIN, -190 }, { ANCHOR_CENTRE,  84 } };
LAYOUTS set_cbScale[] = { { ANCHOR_CENTRE_MIN, -190 }, { ANCHOR_CENTRE, 124 } };
LAYOUTS set_btnBack[] = { { ANCHOR_CENTRE,        0 }, { ANCHOR_CENTRE, 170 } };


#if defined CC_BUILD_MOBILE
static void SettingsScreen_LockOrientation(struct LCheckbox* w) {
	Options_SetBool(OPT_LANDSCAPE_MODE, w->value);
	Window_LockLandscapeOrientation(w->value);
	LBackend_Redraw();
}
#else
static void SettingsScreen_AutoClose(struct LCheckbox* w) {
	Options_SetBool(LOPT_AUTO_CLOSE, w->value);
}
#endif
static void SettingsScreen_ShowEmpty(struct LCheckbox* w) {
	Launcher_ShowEmptyServers = w->value;
	Options_SetBool(LOPT_SHOW_EMPTY, w->value);
}

static void SettingsScreen_DPIScaling(struct LCheckbox* w) {
#if defined CC_BUILD_WIN
	DisplayInfo.DPIScaling = w->value;
	Options_SetBool(OPT_DPI_SCALING, w->value);
	Window_ShowDialog("Restart required", "You must restart ClassiCube before display scaling takes effect");
#else
	Window_ShowDialog("Restart required", "Display scaling is currently only supported on Windows");
#endif
}

static void SettingsScreen_Init(struct LScreen* s_) {
	struct SettingsScreen* s = (struct SettingsScreen*)s_;
	LLine_Init(  &s->sep, 380, set_sep);

	LButton_Init(&s->btnMode, 110, 35, "Mode", set_btnMode);
	LLabel_Init( &s->lblMode, "&eChange the enabled features", set_lblMode);

	LButton_Init(&s->btnColours, 110, 35, "Theme", set_btnColours);
	LLabel_Init( &s->lblColours, "&eChange how the launcher looks", set_lblColours);

#if defined CC_BUILD_MOBILE
	LCheckbox_Init(&s->cbExtra, "Force landscape", set_cbExtra);
	s->cbExtra.ValueChanged = SettingsScreen_LockOrientation;
#else
	LCheckbox_Init(&s->cbExtra, "Close this after game starts", set_cbExtra);
	s->cbExtra.ValueChanged = SettingsScreen_AutoClose;
#endif

	LCheckbox_Init(&s->cbEmpty, "Show empty servers in list", set_cbEmpty);
	s->cbEmpty.ValueChanged = SettingsScreen_ShowEmpty;
	LButton_Init(  &s->btnBack, 80, 35, "Back", set_btnBack);


	LCheckbox_Init(&s->cbScale, "Use display scaling", set_cbScale);
	s->cbScale.ValueChanged = SettingsScreen_DPIScaling;

	s->btnMode.OnClick    = SwitchToChooseMode;
	s->btnColours.OnClick = SwitchToThemes;
	s->btnBack.OnClick    = SwitchToMain;
}

static void SettingsScreen_Show(struct LScreen* s_) {
	struct SettingsScreen* s = (struct SettingsScreen*)s_;

	if (Options_GetBool(OPT_CLASSIC_MODE, false)) {
		s->widgets    = settings_classic;
		s->numWidgets = Array_Elems(settings_classic);
	} else {
		s->widgets    = settings_widgets;
		s->numWidgets = Array_Elems(settings_widgets);
	}

#if defined CC_BUILD_MOBILE
	LCheckbox_Set(&s->cbExtra, Options_GetBool(OPT_LANDSCAPE_MODE, false));
#else
	LCheckbox_Set(&s->cbExtra, Options_GetBool(LOPT_AUTO_CLOSE, false));
#endif
	LCheckbox_Set(&s->cbEmpty, Launcher_ShowEmptyServers);
	LCheckbox_Set(&s->cbScale, DisplayInfo.DPIScaling);
}

void SettingsScreen_SetActive(void) {
	struct SettingsScreen* s = &SettingsScreen;
	LScreen_Reset((struct LScreen*)s);
	s->Init   = SettingsScreen_Init;
	s->Show   = SettingsScreen_Show;

	s->title  = "Options";
	Launcher_SetScreen((struct LScreen*)s);
}


/*########################################################################################################################*
*----------------------------------------------------------ThemesScreen----------------------------------------------------*
*#########################################################################################################################*/
static struct ThemesScreen {
	LScreen_Layout
	struct LButton btnModern, btnClassic, btnNordic;
	struct LButton btnCustom, btnBack;
} ThemesScreen;

static struct LWidget* themes_widgets[] = {
	(struct LWidget*)&ThemesScreen.btnModern,  (struct LWidget*)&ThemesScreen.btnClassic,
	(struct LWidget*)&ThemesScreen.btnNordic, (struct LWidget*)&ThemesScreen.btnCustom,
	(struct LWidget*)&ThemesScreen.btnBack
};

LAYOUTS the_btnModern[]  = { { ANCHOR_CENTRE, 0 }, { ANCHOR_CENTRE, -120 } };
LAYOUTS the_btnClassic[] = { { ANCHOR_CENTRE, 0 }, { ANCHOR_CENTRE,  -70 } };
LAYOUTS the_btnNordic[]  = { { ANCHOR_CENTRE, 0 }, { ANCHOR_CENTRE,  -20 } };
LAYOUTS the_btnCustom[]  = { { ANCHOR_CENTRE, 0 }, { ANCHOR_CENTRE,  120 } };
LAYOUTS the_btnBack[]    = { { ANCHOR_CENTRE, 0 }, { ANCHOR_CENTRE,  170 } };


static void ThemesScreen_Set(const struct LauncherTheme* theme) {
	Launcher_Theme = *theme;
	Launcher_SaveTheme();
	LBackend_ThemeChanged();
}

static void ThemesScreen_Modern(void* w) {
	ThemesScreen_Set(&Launcher_ModernTheme);
}
static void ThemesScreen_Classic(void* w) {
	ThemesScreen_Set(&Launcher_ClassicTheme);
}
static void ThemesScreen_Nordic(void* w) {
	ThemesScreen_Set(&Launcher_NordicTheme);
}

static void ThemesScreen_Init(struct LScreen* s_) {
	struct ThemesScreen* s = (struct ThemesScreen*)s_;
	s->widgets    = themes_widgets;
	s->numWidgets = Array_Elems(themes_widgets);

	LButton_Init(&s->btnModern,  200, 35, "Modern",  the_btnModern);
	LButton_Init(&s->btnClassic, 200, 35, "Classic", the_btnClassic);
	LButton_Init(&s->btnNordic,  200, 35, "Nordic",  the_btnNordic);
	LButton_Init(&s->btnCustom,  200, 35, "Custom",  the_btnCustom);
	LButton_Init(&s->btnBack,     80, 35, "Back",    the_btnBack);

	s->btnModern.OnClick  = ThemesScreen_Modern;
	s->btnClassic.OnClick = ThemesScreen_Classic;
	s->btnNordic.OnClick  = ThemesScreen_Nordic;
	s->btnCustom.OnClick  = SwitchToColours;
	s->btnBack.OnClick    = SwitchToSettings;
}

void ThemesScreen_SetActive(void) {
	struct ThemesScreen* s = &ThemesScreen;
	LScreen_Reset((struct LScreen*)s);
	s->Init   = ThemesScreen_Init;

	s->title  = "Select theme";
	Launcher_SetScreen((struct LScreen*)s);
}


/*########################################################################################################################*
*--------------------------------------------------------UpdatesScreen----------------------------------------------------*
*#########################################################################################################################*/
static struct UpdatesScreen {
	LScreen_Layout
	struct LLine seps[2];
	struct LButton btnRel[2], btnDev[2], btnBack;
	struct LLabel  lblYour, lblRel, lblDev, lblInfo, lblStatus;
	int buildProgress, buildIndex;
	cc_bool pendingFetch, release;
} UpdatesScreen;

static struct LWidget* updates_widgets[] = {
	(struct LWidget*)&UpdatesScreen.seps[0],   (struct LWidget*)&UpdatesScreen.seps[1],
	(struct LWidget*)&UpdatesScreen.lblYour,
	(struct LWidget*)&UpdatesScreen.lblRel,    (struct LWidget*)&UpdatesScreen.lblDev,
	(struct LWidget*)&UpdatesScreen.lblStatus, (struct LWidget*)&UpdatesScreen.btnBack,
	(struct LWidget*)&UpdatesScreen.lblInfo,
	(struct LWidget*)&UpdatesScreen.btnRel[0], (struct LWidget*)&UpdatesScreen.btnDev[0],
	(struct LWidget*)&UpdatesScreen.btnRel[1], (struct LWidget*)&UpdatesScreen.btnDev[1],
};

LAYOUTS upd_lblYour[] = { { ANCHOR_CENTRE, -5 }, { ANCHOR_CENTRE, -120 } };
LAYOUTS upd_seps0[]   = { { ANCHOR_CENTRE,  0 }, { ANCHOR_CENTRE, -100 } };
LAYOUTS upd_seps1[]   = { { ANCHOR_CENTRE,  0 }, { ANCHOR_CENTRE,   -5 } };

LAYOUTS upd_lblRel[]    = { { ANCHOR_CENTRE, -20 }, { ANCHOR_CENTRE, -75 } };
LAYOUTS upd_lblDev[]    = { { ANCHOR_CENTRE, -30 }, { ANCHOR_CENTRE,  20 } };
LAYOUTS upd_lblInfo[]   = { { ANCHOR_CENTRE,   0 }, { ANCHOR_CENTRE, 105 } };
LAYOUTS upd_lblStatus[] = { { ANCHOR_CENTRE,   0 }, { ANCHOR_CENTRE, 130 } };
LAYOUTS upd_btnBack[]   = { { ANCHOR_CENTRE,   0 }, { ANCHOR_CENTRE, 170 } };

/* Update button layouts when 1 build */
LAYOUTS upd_btnRel0_1[] = { { ANCHOR_CENTRE,   0 }, { ANCHOR_CENTRE, -40 } };
LAYOUTS upd_btnDev0_1[] = { { ANCHOR_CENTRE,   0 }, { ANCHOR_CENTRE,  55 } };
/* Update button layouts when 2 builds */
LAYOUTS upd_btnRel0_2[] = { { ANCHOR_CENTRE, -80 }, { ANCHOR_CENTRE, -40 } };
LAYOUTS upd_btnRel1_2[] = { { ANCHOR_CENTRE,  80 }, { ANCHOR_CENTRE, -40 } };
LAYOUTS upd_btnDev0_2[] = { { ANCHOR_CENTRE, -80 }, { ANCHOR_CENTRE,  55 } };
LAYOUTS upd_btnDev1_2[] = { { ANCHOR_CENTRE,  80 }, { ANCHOR_CENTRE,  55 } };


CC_NOINLINE static void UpdatesScreen_FormatTime(cc_string* str, int delta) {
	const char* span;
	int unit, value = Math_AbsI(delta);

	if (value < SECS_PER_MIN) {
		span = "second";  unit = 1;
	} else if (value < SECS_PER_HOUR) {
		span = "minute";  unit = SECS_PER_MIN;
	} else if (value < SECS_PER_DAY) {
		span = "hour";    unit = SECS_PER_HOUR;
	} else {
		span = "day";     unit = SECS_PER_DAY;
	}

	value /= unit;
	String_AppendInt(str, value);
	String_Append(str, ' ');
	String_AppendConst(str, span);

	if (value > 1) String_Append(str, 's');
	String_AppendConst(str, delta >= 0 ? " ago" : " in the future");
}

static void UpdatesScreen_Format(struct LLabel* lbl, const char* prefix, cc_uint64 timestamp) {
	cc_string str; char buffer[STRING_SIZE];
	TimeMS now;
	int delta;

	String_InitArray(str, buffer);
	String_AppendConst(&str, prefix);

	if (!timestamp) {
		String_AppendConst(&str, "&cCheck failed");
	} else {
		now   = DateTime_CurrentUTC_MS() - UNIX_EPOCH;
		/* must divide as cc_uint64, int delta overflows after 26 days */
		delta = (int)((now / 1000) - timestamp);
		UpdatesScreen_FormatTime(&str, delta);
	}
	LLabel_SetText(lbl, &str);
}

static void UpdatesScreen_FormatBoth(struct UpdatesScreen* s) {
	UpdatesScreen_Format(&s->lblRel, "Latest release: ",   CheckUpdateTask.relTimestamp);
	UpdatesScreen_Format(&s->lblDev, "Latest dev build: ", CheckUpdateTask.devTimestamp);
}

static void UpdatesScreen_UpdateHeader(struct UpdatesScreen* s, cc_string* str) {
	const char* message;
	if ( s->release) message = "&eFetching latest release ";
	if (!s->release) message = "&eFetching latest dev build ";

	String_Format2(str, "%c%c", message, Updater_Info.builds[s->buildIndex].name);
}

static void UpdatesScreen_DoFetch(struct UpdatesScreen* s) {
	cc_string str; char strBuffer[STRING_SIZE];
	cc_uint64 time;
	
	time = s->release ? CheckUpdateTask.relTimestamp : CheckUpdateTask.devTimestamp;
	if (!time || FetchUpdateTask.Base.working) return;
	FetchUpdateTask_Run(s->release, s->buildIndex);

	s->pendingFetch  = false;
	s->buildProgress = -1;
	String_InitArray(str, strBuffer);

	UpdatesScreen_UpdateHeader(s, &str);
	String_AppendConst(&str, "..");
	LLabel_SetText(&s->lblStatus, &str);
}

static void UpdatesScreen_Get(cc_bool release, int buildIndex) {
	struct UpdatesScreen* s = &UpdatesScreen;
	/* This code is deliberately split up to handle this particular case: */
	/*  The user clicked this button before CheckUpdateTask completed, */
	/*  therefore update fetching would not actually occur. (as timestamp is 0) */
	/*  By storing requested build, it can be fetched later upon completion. */
	s->pendingFetch = true;
	s->release      = release;
	s->buildIndex   = buildIndex;
	UpdatesScreen_DoFetch(s);
}

static void UpdatesScreen_CheckTick(struct UpdatesScreen* s) {
	if (!CheckUpdateTask.Base.working) return;
	LWebTask_Tick(&CheckUpdateTask.Base, NULL);

	if (!CheckUpdateTask.Base.completed) return;
	UpdatesScreen_FormatBoth(s);
	if (s->pendingFetch) UpdatesScreen_DoFetch(s);
}

static void UpdatesScreen_UpdateProgress(struct UpdatesScreen* s, struct LWebTask* task) {
	cc_string str; char strBuffer[STRING_SIZE];
	int progress = Http_CheckProgress(task->reqID);
	if (progress == s->buildProgress) return;

	s->buildProgress = progress;
	if (progress < 0 || progress > 100) return;
	String_InitArray(str, strBuffer);

	UpdatesScreen_UpdateHeader(s, &str);
	String_Format1(&str, " &a%i%%", &s->buildProgress);
	LLabel_SetText(&s->lblStatus, &str);
}

static void FetchUpdatesError(struct HttpRequest* req) {
	cc_string str; char strBuffer[STRING_SIZE];
	String_InitArray(str, strBuffer);

	Launcher_DisplayHttpError(req, "fetching update", &str);
	LLabel_SetText(&UpdatesScreen.lblStatus, &str);
}

static void UpdatesScreen_FetchTick(struct UpdatesScreen* s) {
	if (!FetchUpdateTask.Base.working) return;

	LWebTask_Tick(&FetchUpdateTask.Base, FetchUpdatesError);
	UpdatesScreen_UpdateProgress(s, &FetchUpdateTask.Base);
	if (!FetchUpdateTask.Base.completed) return;

	if (!FetchUpdateTask.Base.success) return;
	/* FetchUpdateTask handles saving the updated file for us */
	Launcher_ShouldExit   = true;
	Launcher_ShouldUpdate = true;
}

static void UpdatesScreen_Rel_0(void* w) { UpdatesScreen_Get(true,  0); }
static void UpdatesScreen_Rel_1(void* w) { UpdatesScreen_Get(true,  1); }
static void UpdatesScreen_Dev_0(void* w) { UpdatesScreen_Get(false, 0); }
static void UpdatesScreen_Dev_1(void* w) { UpdatesScreen_Get(false, 1); }

static void UpdatesScreen_Init(struct LScreen* s_) {
	struct UpdatesScreen* s = (struct UpdatesScreen*)s_;
	int builds    = Updater_Info.numBuilds;
	s->widgets    = updates_widgets;
	s->numWidgets = Array_Elems(updates_widgets);

	if (Updater_Info.numBuilds < 2) s->numWidgets -= 2;
	if (Updater_Info.numBuilds < 1) s->numWidgets -= 2;

	LLabel_Init(&s->lblYour, "Your build: (unknown)", upd_lblYour);
	LLine_Init( &s->seps[0],   320,                   upd_seps0);
	LLine_Init( &s->seps[1],   320,                   upd_seps1);

	LLabel_Init( &s->lblRel, "Latest release: Checking..",   upd_lblRel);
	LLabel_Init( &s->lblDev, "Latest dev build: Checking..", upd_lblDev);
	LLabel_Init( &s->lblStatus, "",           upd_lblStatus);
	LButton_Init(&s->btnBack, 80, 35, "Back", upd_btnBack);

	if (builds >= 1) {
		LButton_Init(&s->btnRel[0], 130, 35, Updater_Info.builds[0].name, 
							builds == 1 ? upd_btnRel0_1 : upd_btnRel0_2);
		LButton_Init(&s->btnDev[0], 130, 35, Updater_Info.builds[0].name, 
							builds == 1 ? upd_btnDev0_1 : upd_btnDev0_2);
	}
	if (builds >= 2) {
		LButton_Init(&s->btnRel[1], 130, 35, Updater_Info.builds[1].name, upd_btnRel1_2);
		LButton_Init(&s->btnDev[1], 130, 35, Updater_Info.builds[1].name, upd_btnDev1_2);
	}
	LLabel_Init(&s->lblInfo, Updater_Info.info, upd_lblInfo);

	s->btnRel[0].OnClick = UpdatesScreen_Rel_0;
	s->btnRel[1].OnClick = UpdatesScreen_Rel_1;
	s->btnDev[0].OnClick = UpdatesScreen_Dev_0;
	s->btnDev[1].OnClick = UpdatesScreen_Dev_1;
	s->btnBack.OnClick   = SwitchToMain;
}

static void UpdatesScreen_Show(struct LScreen* s_) {
	struct UpdatesScreen* s = (struct UpdatesScreen*)s_;
	cc_uint64 buildTime;
	cc_result res;

	/* Initially fill out with update check result from main menu */
	if (CheckUpdateTask.Base.completed && CheckUpdateTask.Base.success) {
		UpdatesScreen_FormatBoth(s);
	}
	CheckUpdateTask_Run();
	s->pendingFetch = false;

	res = Updater_GetBuildTime(&buildTime);
	if (res) { Logger_SysWarn(res, "getting build time"); return; }
	UpdatesScreen_Format(&s->lblYour, "Your build: ", buildTime);
}

static void UpdatesScreen_Tick(struct LScreen* s_) {
	struct UpdatesScreen* s = (struct UpdatesScreen*)s_;
	LScreen_Tick(s_);

	UpdatesScreen_FetchTick(s);
	UpdatesScreen_CheckTick(s);
}

/* Aborts fetch if it is in progress */
static void UpdatesScreen_Free(struct LScreen* s_) {
	struct UpdatesScreen* s = (struct UpdatesScreen*)s_;
	s->buildProgress = -1;

	FetchUpdateTask.Base.working = false;
	s->lblStatus.text.length     = 0;
}

void UpdatesScreen_SetActive(void) {
	struct UpdatesScreen* s = &UpdatesScreen;
	LScreen_Reset((struct LScreen*)s);
	s->Init   = UpdatesScreen_Init;
	s->Show   = UpdatesScreen_Show;
	s->Tick   = UpdatesScreen_Tick;
	s->Free   = UpdatesScreen_Free;

	s->title  = "Update game";
	Launcher_SetScreen((struct LScreen*)s);
}
#endif
