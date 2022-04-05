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

CC_NOINLINE static struct LWidget* LScreen_WidgetAt(struct LScreen* s, int idx) {
	struct LWidget* w;
	int i = 0, x = Pointers[idx].x, y = Pointers[idx].y;

	for (i = 0; i < s->numWidgets; i++) {
		w = s->widgets[i];
		if (Gui_Contains(w->x, w->y, w->width, w->height, x, y)) return w;
	}
	return NULL;
}

static void LScreen_Draw(struct LScreen* s) {
	int i;
	for (i = 0; i < s->numWidgets; i++) {
		LWidget_Draw(s->widgets[i]);
	}
	Launcher_MarkAllDirty();
}

static void LScreen_Tick(struct LScreen* s) {
	struct LWidget* w = s->selectedWidget;
	if (w && w->VTABLE->Tick) w->VTABLE->Tick(w);
}

static void LScreen_HoverWidget(struct LScreen* s,   struct LWidget* w) { }
static void LScreen_UnhoverWidget(struct LScreen* s, struct LWidget* w) { }

CC_NOINLINE static void LScreen_SelectWidget(struct LScreen* s, int idx, struct LWidget* w, cc_bool was) {
	if (!w) return;
	w->selected       = true;
	s->selectedWidget = w;
	if (w->VTABLE->OnSelect) w->VTABLE->OnSelect(w, idx, was);
}

CC_NOINLINE static void LScreen_UnselectWidget(struct LScreen* s, int idx, struct LWidget* w) {
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
			s->selectedWidget->OnClick(s->selectedWidget, 0);
		} else if (s->hoveredWidget && s->hoveredWidget->OnClick) {
			s->hoveredWidget->OnClick(s->hoveredWidget,   0);
		} else if (s->onEnterWidget) {
			s->onEnterWidget->OnClick(s->onEnterWidget,   0);
		}
	} else if (s->selectedWidget) {
		if (!s->selectedWidget->VTABLE->KeyDown) return;
		s->selectedWidget->VTABLE->KeyDown(s->selectedWidget, key, was);
	}
}

static void LScreen_KeyPress(struct LScreen* s, char key) {
	if (!s->selectedWidget) return;
	if (!s->selectedWidget->VTABLE->KeyPress) return;
	s->selectedWidget->VTABLE->KeyPress(s->selectedWidget, key);
}

static void LScreen_TextChanged(struct LScreen* s, const cc_string* str) {
	if (!s->selectedWidget) return;
	if (!s->selectedWidget->VTABLE->TextChanged) return;
	s->selectedWidget->VTABLE->TextChanged(s->selectedWidget, str);
}

static void LScreen_MouseDown(struct LScreen* s, int idx) {
	struct LWidget* over = LScreen_WidgetAt(s, idx);
	struct LWidget* prev = s->selectedWidget;

	if (prev && over != prev) LScreen_UnselectWidget(s, idx, prev);
	if (over) LScreen_SelectWidget(s, idx, over, over == prev);
}

static void LScreen_MouseUp(struct LScreen* s, int idx) {
	struct LWidget* over = LScreen_WidgetAt(s, idx);
	struct LWidget* prev = s->selectedWidget;

	/* if user moves mouse away, it doesn't count */
	if (over != prev) {
		LScreen_UnselectWidget(s, idx, prev);
	} else if (over && over->OnClick) {
		over->OnClick(over, idx);
	}
}

static void LScreen_MouseMove(struct LScreen* s, int idx) {
	struct LWidget* over = LScreen_WidgetAt(s, idx);
	struct LWidget* prev = s->hoveredWidget;
	cc_bool overSame = prev == over;

	if (prev && !overSame) {
		prev->hovered    = false;
		s->hoveredWidget = NULL;
		s->UnhoverWidget(s, prev);

		if (prev->VTABLE->MouseLeft) prev->VTABLE->MouseLeft(prev);
	}

	if (over) {
		over->hovered    = true;
		s->hoveredWidget = over;
		s->HoverWidget(s, over);

		if (!over->VTABLE->MouseMove) return;
		over->VTABLE->MouseMove(over, idx, overSame);
	}
}
static void LScreen_MouseWheel(struct LScreen* s, float delta) { }

static void LScreen_DrawBackground(struct LScreen* s, struct Bitmap* bmp) {
	cc_string title_fore, title_back;
	struct DrawTextArgs args;
	int x;

	if (!s->title_fore) {
		Launcher_DrawBackground(bmp, 0, 0, bmp->width, bmp->height);
		return;
	}
	title_fore = String_FromReadonly(s->title_fore);
	title_back = String_FromReadonly(s->title_back);

	Launcher_DrawBackgroundAll(bmp);
	DrawTextArgs_Make(&args, &title_fore, &Launcher_LogoFont, false);
	x = WindowInfo.Width / 2 - Drawer2D_TextWidth(&args) / 2;

	args.text = title_back;
	Drawer2D_DrawText(bmp, &args, x + Display_ScaleX(4), Display_ScaleY(4));
	args.text = title_fore;
	Drawer2D_DrawText(bmp, &args, x,                     0);
}

CC_NOINLINE static void LScreen_Reset(struct LScreen* s) {
	int i;

	s->Init = NULL; /* screens should always override this */
	s->Show = LScreen_NullFunc;
	s->Free = LScreen_NullFunc;
	s->Draw       = LScreen_Draw;
	s->Tick       = LScreen_Tick;
	s->KeyDown    = LScreen_KeyDown;
	s->KeyPress   = LScreen_KeyPress;
	s->MouseDown  = LScreen_MouseDown;
	s->MouseUp    = LScreen_MouseUp;
	s->MouseMove  = LScreen_MouseMove;
	s->MouseWheel = LScreen_MouseWheel;
	s->HoverWidget    = LScreen_HoverWidget;
	s->UnhoverWidget  = LScreen_UnhoverWidget;
	s->TextChanged    = LScreen_TextChanged;
	s->DrawBackground = LScreen_DrawBackground;

	/* reset all widgets mouse state */
	for (i = 0; i < s->numWidgets; i++) { 
		s->widgets[i]->hovered  = false;
		s->widgets[i]->selected = false;
	}

	s->onEnterWidget  = NULL;
	s->hoveredWidget  = NULL;
	s->selectedWidget = NULL;
}

static void SwitchToChooseMode(void* w, int idx) { ChooseModeScreen_SetActive(false); }
static void SwitchToColours(void* w, int idx) { ColoursScreen_SetActive(); }
static void SwitchToDirectConnect(void* w, int idx) { DirectConnectScreen_SetActive(); }
static void SwitchToMain(void* w, int idx) { MainScreen_SetActive(); }
static void SwitchToSettings(void* w, int idx) { SettingsScreen_SetActive(); }
static void SwitchToThemes(void* w, int idx)   { ThemesScreen_SetActive(); }
static void SwitchToUpdates(void* w, int idx)  { UpdatesScreen_SetActive(); }


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
	Launcher_UpdateLogoFont();
	MainScreen_SetActive();
}

static void UseModeEnhanced(void* w, int idx)   { ChooseMode_Click(false, false); }
static void UseModeClassicHax(void* w, int idx) { ChooseMode_Click(true,  true);  }
static void UseModeClassic(void* w, int idx)    { ChooseMode_Click(true,  false); }

static void ChooseModeScreen_Init(struct LScreen* s_) {
	struct ChooseModeScreen* s = (struct ChooseModeScreen*)s_;
	s->widgets     = chooseMode_widgets;
	s->numWidgets  = Array_Elems(chooseMode_widgets);

	LLine_Init(  &s->seps[0], 490);
	LLine_Init(  &s->seps[1], 490);

	LButton_Init(&s->btnEnhanced, 145, 35, "Enhanced");
	LLabel_Init( &s->lblEnhanced[0], "&eEnables custom blocks, changing env");
	LLabel_Init( &s->lblEnhanced[1], "&esettings, longer messages, and more");

	LButton_Init(&s->btnClassicHax, 145, 35, "Classic +hax");
	LLabel_Init( &s->lblClassicHax[0], "&eSame as Classic mode, except that");
	LLabel_Init( &s->lblClassicHax[1], "&ehacks (noclip/fly/speed) are enabled");

	LButton_Init(&s->btnClassic, 145, 35, "Classic");
	LLabel_Init( &s->lblClassic[0], "&eOnly uses blocks and features from");
	LLabel_Init( &s->lblClassic[1], "&ethe original minecraft classic");

	LLabel_Init( &s->lblHelp, "&eClick &fEnhanced &eif you're not sure which mode to choose.");
	LButton_Init(&s->btnBack, 80, 35, "Back");

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

static void ChooseModeScreen_Layout(struct LScreen* s_) {
	struct ChooseModeScreen* s = (struct ChooseModeScreen*)s_;
	LWidget_SetLocation(&s->seps[0],  ANCHOR_CENTRE, ANCHOR_CENTRE, -5,  -85);
	LWidget_SetLocation(&s->seps[1],  ANCHOR_CENTRE, ANCHOR_CENTRE, -5,  -15);

	LWidget_SetLocation(&s->btnEnhanced,    ANCHOR_CENTRE_MIN, ANCHOR_CENTRE, -250, -120);
	LWidget_SetLocation(&s->lblEnhanced[0], ANCHOR_CENTRE_MIN, ANCHOR_CENTRE,  -85, -120 - 12);
	LWidget_SetLocation(&s->lblEnhanced[1], ANCHOR_CENTRE_MIN, ANCHOR_CENTRE,  -85, -120 + 12);

	LWidget_SetLocation(&s->btnClassicHax,    ANCHOR_CENTRE_MIN, ANCHOR_CENTRE, -250, -50);
	LWidget_SetLocation(&s->lblClassicHax[0], ANCHOR_CENTRE_MIN, ANCHOR_CENTRE,  -85, -50 - 12);
	LWidget_SetLocation(&s->lblClassicHax[1], ANCHOR_CENTRE_MIN, ANCHOR_CENTRE,  -85, -50 + 12);

	LWidget_SetLocation(&s->btnClassic,    ANCHOR_CENTRE_MIN, ANCHOR_CENTRE, -250, 20);
	LWidget_SetLocation(&s->lblClassic[0], ANCHOR_CENTRE_MIN, ANCHOR_CENTRE,  -85, 20 - 12);
	LWidget_SetLocation(&s->lblClassic[1], ANCHOR_CENTRE_MIN, ANCHOR_CENTRE,  -85, 20 + 12);

	LWidget_SetLocation(&s->lblHelp, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 160);
	LWidget_SetLocation(&s->btnBack, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 170);
}

void ChooseModeScreen_SetActive(cc_bool firstTime) {
	struct ChooseModeScreen* s = &ChooseModeScreen;
	LScreen_Reset((struct LScreen*)s);
	s->Init      = ChooseModeScreen_Init;
	s->Show      = ChooseModeScreen_Show;
	s->Layout    = ChooseModeScreen_Layout;
	s->firstTime = firstTime;

	s->title_fore    = "&fChoose mode";
	s->title_back    = "&0Choose mode";
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

CC_NOINLINE static void ColoursScreen_Update(struct ColoursScreen* s, int i, BitmapCol color) {
	cc_string tmp; char tmpBuffer[3];

	String_InitArray(tmp, tmpBuffer);
	String_AppendInt(&tmp, BitmapCol_R(color));
	LInput_SetText(&s->iptColours[i + 0], &tmp);

	tmp.length = 0;
	String_AppendInt(&tmp, BitmapCol_G(color));
	LInput_SetText(&s->iptColours[i + 1], &tmp);

	tmp.length = 0;
	String_AppendInt(&tmp, BitmapCol_B(color));
	LInput_SetText(&s->iptColours[i + 2], &tmp);
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

	*color = BitmapCol_Make(r, g, b, 255);
	Launcher_SaveTheme();
	Launcher_Redraw();
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
	w->text.length = 0;
	String_AppendInt(&w->text, newVal);

	if (w->caretPos >= w->text.length) w->caretPos = -1;
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

static void ColoursScreen_ToggleBG(void* w, int idx) {
	struct LCheckbox* cb = (struct LCheckbox*)w;
	Launcher_Theme.ClassicBackground = !Launcher_Theme.ClassicBackground;
	cb->value = Launcher_Theme.ClassicBackground;

	Launcher_SaveTheme();
	Launcher_Redraw();
}

static void ColoursScreen_Init(struct LScreen* s_) {
	struct ColoursScreen* s = (struct ColoursScreen*)s_;
	int i;
	s->widgets    = colours_widgets;
	s->numWidgets = Array_Elems(colours_widgets);
	
	for (i = 0; i < 5 * 3; i++) {
		s->iptColours[i].type        = KEYBOARD_TYPE_INTEGER;
		s->iptColours[i].TextChanged = ColoursScreen_TextChanged;
		LInput_Init(&s->iptColours[i], 55, NULL);
	}

	LLabel_Init( &s->lblNames[0], "Background");
	LLabel_Init( &s->lblNames[1], "Button border");
	LLabel_Init( &s->lblNames[2], "Button highlight");
	LLabel_Init( &s->lblNames[3], "Button");
	LLabel_Init( &s->lblNames[4], "Active button");

	LLabel_Init( &s->lblRGB[0], "Red");
	LLabel_Init( &s->lblRGB[1], "Green");
	LLabel_Init( &s->lblRGB[2], "Blue");
	LButton_Init(&s->btnBack, 80, 35, "Back");

	LCheckbox_Init(&s->cbClassic, "Classic style");
	s->cbClassic.OnClick = ColoursScreen_ToggleBG;
	s->btnBack.OnClick   = SwitchToThemes;
}

static void ColoursScreen_Show(struct LScreen* s_) {
	struct ColoursScreen* s = (struct ColoursScreen*)s_;
	s->colourAcc       = 0;
	s->cbClassic.value = Launcher_Theme.ClassicBackground;
	ColoursScreen_UpdateAll(s);
}

static void ColoursScreen_Layout(struct LScreen* s_) {
	struct ColoursScreen* s = (struct ColoursScreen*)s_;
	int i, y;
	for (i = 0; i < 5; i++) {
		y = -100 + 40 * i;
		LWidget_SetLocation(&s->iptColours[i*3 + 0], ANCHOR_CENTRE, ANCHOR_CENTRE,  30, y);
		LWidget_SetLocation(&s->iptColours[i*3 + 1], ANCHOR_CENTRE, ANCHOR_CENTRE,  95, y);
		LWidget_SetLocation(&s->iptColours[i*3 + 2], ANCHOR_CENTRE, ANCHOR_CENTRE, 160, y);
	}

	LWidget_SetLocation(&s->lblNames[0], ANCHOR_CENTRE_MAX, ANCHOR_CENTRE, 10, -100);
	LWidget_SetLocation(&s->lblNames[1], ANCHOR_CENTRE_MAX, ANCHOR_CENTRE, 10,  -60);
	LWidget_SetLocation(&s->lblNames[2], ANCHOR_CENTRE_MAX, ANCHOR_CENTRE, 10,  -20);
	LWidget_SetLocation(&s->lblNames[3], ANCHOR_CENTRE_MAX, ANCHOR_CENTRE, 10,   20);
	LWidget_SetLocation(&s->lblNames[4], ANCHOR_CENTRE_MAX, ANCHOR_CENTRE, 10,   60);

	LWidget_SetLocation(&s->lblRGB[0], ANCHOR_CENTRE, ANCHOR_CENTRE,  30, -130);
	LWidget_SetLocation(&s->lblRGB[1], ANCHOR_CENTRE, ANCHOR_CENTRE,  95, -130);
	LWidget_SetLocation(&s->lblRGB[2], ANCHOR_CENTRE, ANCHOR_CENTRE, 160, -130);

	LWidget_SetLocation(&s->cbClassic,  ANCHOR_CENTRE, ANCHOR_CENTRE, -16, 130);
	LWidget_SetLocation(&s->btnBack,    ANCHOR_CENTRE, ANCHOR_CENTRE,   0, 170);
}

void ColoursScreen_SetActive(void) {
	struct ColoursScreen* s = &ColoursScreen;
	LScreen_Reset((struct LScreen*)s);
	s->Init       = ColoursScreen_Init;
	s->Show       = ColoursScreen_Show;
	s->Layout     = ColoursScreen_Layout;
	s->KeyDown    = ColoursScreen_KeyDown;
	s->MouseWheel = ColoursScreen_MouseWheel;

	s->title_fore = "&fCustom theme";
	s->title_back = "&0Custom theme";
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

CC_NOINLINE static void DirectConnectScreen_SetStatus(const char* text) {
	struct LLabel* w = &DirectConnectScreen.lblStatus;
	LLabel_SetConst(w, text);
	LWidget_Redraw(w);
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

static void DirectConnectScreen_StartClient(void* w, int idx) {
	static const cc_string defMppass = String_FromConst("(none)");
	static const cc_string defPort   = String_FromConst("25565");
	const cc_string* user   = &DirectConnectScreen.iptUsername.text;
	const cc_string* addr   = &DirectConnectScreen.iptAddress.text;
	const cc_string* mppass = &DirectConnectScreen.iptMppass.text;

	cc_string ip, port;
	cc_uint16 raw_port;

	int index = String_LastIndexOf(addr, ':');
	if (index == 0 || index == addr->length - 1) {
		DirectConnectScreen_SetStatus("&eInvalid address"); return;
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
		DirectConnectScreen_SetStatus("&eUsername required"); return;
	}
	if (!Socket_ValidAddress(&ip)) {
		DirectConnectScreen_SetStatus("&eInvalid ip"); return;
	}
	if (!Convert_ParseUInt16(&port, &raw_port)) {
		DirectConnectScreen_SetStatus("&eInvalid port"); return;
	}
	if (!mppass->length) mppass = &defMppass;

	Options_Set("launcher-dc-username", user);
	Options_Set("launcher-dc-ip",       &ip);
	Options_Set("launcher-dc-port",     &port);
	Options_SetSecure("launcher-dc-mppass", mppass);

	DirectConnectScreen_SetStatus("");
	Launcher_StartGame(user, mppass, &ip, &port, &String_Empty);
}

static void DirectConnectScreen_Init(struct LScreen* s_) {
	struct DirectConnectScreen* s = (struct DirectConnectScreen*)s_;
	s->widgets    = directConnect_widgets;
	s->numWidgets = Array_Elems(directConnect_widgets);

	LInput_Init(&s->iptUsername, 330, "Username..");
	LInput_Init(&s->iptAddress,  330, "IP address:Port number..");
	LInput_Init(&s->iptMppass,   330, "Mppass..");

	LButton_Init(&s->btnConnect, 110, 35, "Connect");
	LButton_Init(&s->btnBack,     80, 35, "Back");
	LLabel_Init( &s->lblStatus,  "");

	s->iptUsername.ClipboardFilter = DirectConnectScreen_UrlFilter;
	s->iptAddress.ClipboardFilter  = DirectConnectScreen_UrlFilter;
	s->iptMppass.ClipboardFilter   = DirectConnectScreen_UrlFilter;

	s->btnConnect.OnClick = DirectConnectScreen_StartClient;
	s->btnBack.OnClick    = SwitchToMain;
	/* Init input text from options */
	DirectConnectScreen_Load(s);
}

static void DirectConnectScreen_Layout(struct LScreen* s_) {
	struct DirectConnectScreen* s = (struct DirectConnectScreen*)s_;
	LWidget_SetLocation(&s->iptUsername, ANCHOR_CENTRE_MIN, ANCHOR_CENTRE, -165, -120);
	LWidget_SetLocation(&s->iptAddress,  ANCHOR_CENTRE_MIN, ANCHOR_CENTRE, -165,  -75);
	LWidget_SetLocation(&s->iptMppass,   ANCHOR_CENTRE_MIN, ANCHOR_CENTRE, -165,  -30);

	LWidget_SetLocation(&s->btnConnect, ANCHOR_CENTRE, ANCHOR_CENTRE, -110, 20);
	LWidget_SetLocation(&s->btnBack,    ANCHOR_CENTRE, ANCHOR_CENTRE,  125, 20);
	LWidget_SetLocation(&s->lblStatus,  ANCHOR_CENTRE, ANCHOR_CENTRE,    0, 70);
}

void DirectConnectScreen_SetActive(void) {
	struct DirectConnectScreen* s = &DirectConnectScreen;
	LScreen_Reset((struct LScreen*)s);
	s->Init          = DirectConnectScreen_Init;
	s->Layout        = DirectConnectScreen_Layout;

	s->title_fore    = "&fDirect connect";
	s->title_back    = "&0Direct connect";
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

static void MainScreen_DoLogin(void);
static void MFAScreen_SignIn(void* w, int idx) {
	MainScreen_SetActive();
	MainScreen_DoLogin();
}
static void MFAScreen_Cancel(void* w, int idx) {
	MFAScreen.iptCode.text.length = 0;
	MainScreen_SetActive();
}

static void MFAScreen_Init(struct LScreen* s_) {
	struct MFAScreen* s = (struct MFAScreen*)s_;
	s->widgets    = mfa_widgets;
	s->numWidgets = Array_Elems(mfa_widgets);
	
	LLabel_Init( &s->lblTitle,  "");
	LInput_Init( &s->iptCode,   280, "Login code..");
	LButton_Init(&s->btnSignIn, 100, 35, "Sign in");
	LButton_Init(&s->btnCancel, 100, 35, "Cancel");

	s->btnSignIn.OnClick = MFAScreen_SignIn;
	s->btnCancel.OnClick = MFAScreen_Cancel;
	s->iptCode.type      = KEYBOARD_TYPE_INTEGER;
}

static void MFAScreen_Show(struct LScreen* s_) {
	struct MFAScreen* s = (struct MFAScreen*)s_;
	LLabel_SetConst(&s->lblTitle, s->iptCode.text.length ?
		"&cWrong code entered  (Check emails)" :
		"&cLogin code required (Check emails)");
}

static void MFAScreen_Layout(struct LScreen* s_) {
	struct MFAScreen* s = (struct MFAScreen*)s_;
	LWidget_SetLocation(&s->lblTitle,  ANCHOR_CENTRE, ANCHOR_CENTRE,   0, -115);
	LWidget_SetLocation(&s->iptCode,   ANCHOR_CENTRE, ANCHOR_CENTRE,   0,  -75);
	LWidget_SetLocation(&s->btnSignIn, ANCHOR_CENTRE, ANCHOR_CENTRE, -90,  -25);
	LWidget_SetLocation(&s->btnCancel, ANCHOR_CENTRE, ANCHOR_CENTRE,  90,  -25);
}

void MFAScreen_SetActive(void) {
	struct MFAScreen* s = &MFAScreen;
	LScreen_Reset((struct LScreen*)s);
	s->Init   = MFAScreen_Init;
	s->Show   = MFAScreen_Show;
	s->Layout = MFAScreen_Layout;

	s->title_fore    = "&fEnter login code";
	s->title_back    = "&0Enter login code";
	s->onEnterWidget = (struct LWidget*)&s->btnSignIn;
	Launcher_SetScreen((struct LScreen*)s);
}


/*########################################################################################################################*
*----------------------------------------------------------MainScreen-----------------------------------------------------*
*#########################################################################################################################*/
static struct MainScreen {
	LScreen_Layout
	struct LButton btnLogin, btnResume, btnDirect, btnSPlayer, btnOptions, btnRegister;
	struct LInput iptUsername, iptPassword;
	struct LLabel lblStatus, lblUpdate;
	cc_bool signingIn;
} MainScreen;

static struct LWidget* main_widgets[] = {
	(struct LWidget*)&MainScreen.iptUsername, (struct LWidget*)&MainScreen.iptPassword,
	(struct LWidget*)&MainScreen.btnLogin,    (struct LWidget*)&MainScreen.btnResume,
	(struct LWidget*)&MainScreen.lblStatus,   (struct LWidget*)&MainScreen.btnDirect,
	(struct LWidget*)&MainScreen.btnSPlayer,  (struct LWidget*)&MainScreen.lblUpdate,
	(struct LWidget*)&MainScreen.btnRegister, (struct LWidget*)&MainScreen.btnOptions
};

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

CC_NOINLINE static void MainScreen_Error(struct LWebTask* task, const char* action) {
	cc_string str; char strBuffer[STRING_SIZE];
	struct MainScreen* s = &MainScreen;
	String_InitArray(str, strBuffer);

	LWebTask_DisplayError(task, action, &str);
	LLabel_SetText(&s->lblStatus, &str);
	LWidget_Redraw(&s->lblStatus);
	s->signingIn = false;
}

static void MainScreen_DoLogin(void) {
	struct MainScreen* s = &MainScreen;
	cc_string* user = &s->iptUsername.text;
	cc_string* pass = &s->iptPassword.text;

	if (!user->length) {
		LLabel_SetConst(&s->lblStatus, "&eUsername required");
		LWidget_Redraw(&s->lblStatus); return;
	}
	if (!pass->length) {
		LLabel_SetConst(&s->lblStatus, "&ePassword required");
		LWidget_Redraw(&s->lblStatus); return;
	}

	if (GetTokenTask.Base.working) return;
	Options_Set(LOPT_USERNAME, user);
	Options_SetSecure(LOPT_PASSWORD, pass);

	GetTokenTask_Run();
	LLabel_SetConst(&s->lblStatus, "&eSigning in..");
	LWidget_Redraw(&s->lblStatus);
	s->signingIn = true;
}
static void MainScreen_Login(void* w, int idx) { MainScreen_DoLogin(); }

static void MainScreen_Register(void* w, int idx) {
	static const cc_string regUrl = String_FromConst(REGISTERNEW_URL);
	cc_result res = Process_StartOpen(&regUrl);
	if (res) Logger_SimpleWarn(res, "opening register webpage in browser");
}

static void MainScreen_Resume(void* w, int idx) {
	struct ResumeInfo info;
	MainScreen_GetResume(&info, true);

	if (!info.valid) return;
	Launcher_StartGame(&info.user, &info.mppass, &info.ip, &info.port, &info.server);
}

static void MainScreen_Singleplayer(void* w, int idx) {
	static const cc_string defUser = String_FromConst("Singleplayer");
	const cc_string* user = &MainScreen.iptUsername.text;

	if (!user->length) user = &defUser;
	Launcher_StartGame(user, &String_Empty, &String_Empty, &String_Empty, &String_Empty);
}

static void MainScreen_Init(struct LScreen* s_) {
	cc_string user, pass; char passBuffer[STRING_SIZE];
	struct MainScreen* s = (struct MainScreen*)s_;
	s->widgets           = main_widgets;
	s->numWidgets        = Array_Elems(main_widgets);
	s->iptPassword.type  = KEYBOARD_TYPE_PASSWORD;

	LInput_Init( &s->iptUsername, 280, "Username..");
	LInput_Init( &s->iptPassword, 280, "Password..");
	LButton_Init(&s->btnLogin,    100, 35, "Sign in");
	LButton_Init(&s->btnResume,   100, 35, "Resume");

	LLabel_Init( &s->lblStatus,  "");
	LButton_Init(&s->btnDirect,  200, 35, "Direct connect");
	LButton_Init(&s->btnSPlayer, 200, 35, "Singleplayer");

	LLabel_Init( &s->lblUpdate,   "");
	LButton_Init(&s->btnRegister, 100, 35, "Register");
	LButton_Init(&s->btnOptions,  100, 35, "Options");
	
	s->btnLogin.OnClick    = MainScreen_Login;
	s->btnResume.OnClick   = MainScreen_Resume;
	s->btnDirect.OnClick   = SwitchToDirectConnect;
	s->btnSPlayer.OnClick  = MainScreen_Singleplayer;
	s->btnOptions.OnClick  = SwitchToSettings;
	s->btnRegister.OnClick = MainScreen_Register;

	/* need to set text here for right size */
	s->lblUpdate.font = &Launcher_HintFont;
	LLabel_SetConst(&s->lblUpdate, "&eChecking..");
	
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
	s->lblStatus.text.length = 0;
}

static void MainScreen_Layout(struct LScreen* s_) {
	struct MainScreen* s = (struct MainScreen*)s_;
	LWidget_SetLocation(&s->iptUsername, ANCHOR_CENTRE_MIN, ANCHOR_CENTRE, -140, -120);
	LWidget_SetLocation(&s->iptPassword, ANCHOR_CENTRE_MIN, ANCHOR_CENTRE, -140,  -75);

	LWidget_SetLocation(&s->btnLogin,  ANCHOR_CENTRE, ANCHOR_CENTRE, -90, -25);
	LWidget_SetLocation(&s->lblStatus, ANCHOR_CENTRE, ANCHOR_CENTRE,   0,  20);

	LWidget_SetLocation(&s->btnResume,  ANCHOR_CENTRE, ANCHOR_CENTRE, 90, -25);
	LWidget_SetLocation(&s->btnDirect,  ANCHOR_CENTRE, ANCHOR_CENTRE,  0,  60);
	LWidget_SetLocation(&s->btnSPlayer, ANCHOR_CENTRE, ANCHOR_CENTRE,  0, 110);

	LWidget_SetLocation(&s->lblUpdate,  ANCHOR_MAX, ANCHOR_MAX,  10,  45);
	LWidget_SetLocation(&s->btnOptions, ANCHOR_MAX, ANCHOR_MAX,   6,   6);
	LWidget_SetLocation(&s->btnRegister, ANCHOR_MIN, ANCHOR_MAX,  6,   6);
}

static void MainScreen_HoverWidget(struct LScreen* s_, struct LWidget* w) {
	cc_string str; char strBuffer[STRING_SIZE];
	struct MainScreen* s = (struct MainScreen*)s_;
	struct ResumeInfo info;
	if (s->signingIn || w != (struct LWidget*)&s->btnResume) return;

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
	LWidget_Redraw(&s->lblStatus);
}

static void MainScreen_UnhoverWidget(struct LScreen* s_, struct LWidget* w) {
	struct MainScreen* s = (struct MainScreen*)s_;
	if (s->signingIn || w != (struct LWidget*)&s->btnResume) return;

	LLabel_SetConst(&s->lblStatus, "");
	LWidget_Redraw(&s->lblStatus);
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
	LWebTask_Tick(&CheckUpdateTask.Base);
	if (!CheckUpdateTask.Base.completed) return;

	if (CheckUpdateTask.Base.success) {
		latest  = MainScreen_GetVersion(&CheckUpdateTask.latestRelease);
		current = MainScreen_GetVersion(&currentStr);
		LLabel_SetConst(&s->lblUpdate, latest > current ? "&aNew release" : "&eUp to date");
	} else {
		LLabel_SetConst(&s->lblUpdate, "&cCheck failed");
	}
	LWidget_Redraw(&s->lblUpdate);
}

static void MainScreen_LoginPhase2(struct MainScreen* s, const cc_string* user) {
	/* website returns case correct username */
	if (!String_Equals(&s->iptUsername.text, user)) {
		LInput_SetText(&s->iptUsername, user);
		LWidget_Redraw(&s->iptUsername);
	}
	String_Copy(&Launcher_Username, user);

	FetchServersTask_Run();
	LLabel_SetConst(&s->lblStatus, "&eRetrieving servers list..");
	LWidget_Redraw(&s->lblStatus);
}

static void MainScreen_TickGetToken(struct MainScreen* s) {
	if (!GetTokenTask.Base.working)   return;
	LWebTask_Tick(&GetTokenTask.Base);
	if (!GetTokenTask.Base.completed) return;

	if (!GetTokenTask.Base.success) {
		MainScreen_Error(&GetTokenTask.Base, "signing in");
		return;
	}

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
	LWebTask_Tick(&SignInTask.Base);
	if (!SignInTask.Base.completed) return;

	if (SignInTask.needMFA) { MFAScreen_SetActive(); return; }

	if (SignInTask.error) {
		LLabel_SetConst(&s->lblStatus, SignInTask.error);
		LWidget_Redraw(&s->lblStatus);
	} else if (SignInTask.Base.success) {
		MainScreen_LoginPhase2(s, &SignInTask.username);
	} else {
		MainScreen_Error(&SignInTask.Base, "signing in");
	}
}

static void MainScreen_TickFetchServers(struct MainScreen* s) {
	if (!FetchServersTask.Base.working)   return;
	LWebTask_Tick(&FetchServersTask.Base);
	if (!FetchServersTask.Base.completed) return;

	if (!FetchServersTask.Base.success) {
		MainScreen_Error(&FetchServersTask.Base, "retrieving servers list");
		return;
	}

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
	s->Init   = MainScreen_Init;
	s->Free   = MainScreen_Free;
	s->Tick   = MainScreen_Tick;
	s->Layout = MainScreen_Layout;

	s->title_fore    = "&fClassiCube";
	s->title_back    = "&0ClassiCube";
	s->HoverWidget   = MainScreen_HoverWidget;
	s->UnhoverWidget = MainScreen_UnhoverWidget;
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

static void CheckResourcesScreen_Yes(void*  w, int idx) { FetchResourcesScreen_SetActive(); }
static void CheckResourcesScreen_Next(void* w, int idx) {
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

	LLabel_Init( &s->lblLine1,  "Some required resources weren't found");
	LLabel_Init( &s->lblLine2,  "Okay to download?");
	LLabel_Init( &s->lblStatus, "");

	LButton_Init(&s->btnYes, 70, 35, "Yes");
	LButton_Init(&s->btnNo,  70, 35, "No");
	s->btnYes.OnClick = CheckResourcesScreen_Yes;
	s->btnNo.OnClick  = CheckResourcesScreen_Next;
}

static void CheckResourcesScreen_Show(struct LScreen* s_) {
	cc_string str; char buffer[STRING_SIZE];
	struct CheckResourcesScreen* s = (struct CheckResourcesScreen*)s_;
	float size = Resources_Size / 1024.0f;

	String_InitArray(str, buffer);
	String_Format1(&str, "&eDownload size: %f2 megabytes", &size);
	s->lblStatus.font = &Launcher_HintFont;
	LLabel_SetText(&s->lblStatus, &str);
}

static void CheckResourcesScreen_Layout(struct LScreen* s_) {
	struct CheckResourcesScreen* s = (struct CheckResourcesScreen*)s_;
	
	LWidget_SetLocation(&s->lblLine1,  ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -50);
	LWidget_SetLocation(&s->lblLine2,  ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -30);
	LWidget_SetLocation(&s->lblStatus, ANCHOR_CENTRE, ANCHOR_CENTRE, 0,  10);

	LWidget_SetLocation(&s->btnYes, ANCHOR_CENTRE, ANCHOR_CENTRE, -70, 45);
	LWidget_SetLocation(&s->btnNo,  ANCHOR_CENTRE, ANCHOR_CENTRE,  70, 45);
}

#define RESOURCES_BACK_COLOR BitmapCol_Make( 12,  12,  12, 255)
#define RESOURCES_FORE_COLOR BitmapCol_Make(120,  85, 151, 255)

static void CheckResourcesScreen_ResetArea(int x, int y, int width, int height) {
	Gradient_Noise(&Launcher_Framebuffer, RESOURCES_FORE_COLOR, 4, x, y, width, height);
	Launcher_MarkDirty(x, y, width, height);
}

static void CheckResourcesScreen_DrawBackground(struct LScreen* s, struct Bitmap* bmp) {
	int x, y, width, height;
	Drawer2D_Clear(bmp, RESOURCES_BACK_COLOR, 0, 0, bmp->width, bmp->height);
	width  = Display_ScaleX(380);
	height = Display_ScaleY(140);

	x = Gui_CalcPos(ANCHOR_CENTRE, 0, width,  bmp->width);
	y = Gui_CalcPos(ANCHOR_CENTRE, 0, height, bmp->height);
	Gradient_Noise(bmp, RESOURCES_FORE_COLOR, 4, x, y, width, height);
}

void CheckResourcesScreen_SetActive(void) {
	struct CheckResourcesScreen* s = &CheckResourcesScreen;
	LScreen_Reset((struct LScreen*)s);
	s->Init   = CheckResourcesScreen_Init;
	s->Show   = CheckResourcesScreen_Show;
	s->Layout = CheckResourcesScreen_Layout;
	s->DrawBackground = CheckResourcesScreen_DrawBackground;
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

static void FetchResourcesScreen_Init(struct LScreen* s_) {
	struct FetchResourcesScreen* s = (struct FetchResourcesScreen*)s_;
	s->widgets    = fetchResources_widgets;
	s->numWidgets = Array_Elems(fetchResources_widgets);

	LLabel_Init( &s->lblStatus,   "");
	LButton_Init(&s->btnCancel,   120, 35, "Cancel");
	LSlider_Init(&s->sdrProgress, 200, 12, BitmapCol_Make(0, 220, 0, 255));

	s->lblStatus.font    = &Launcher_HintFont;
	s->btnCancel.OnClick = CheckResourcesScreen_Next;
}
static void FetchResourcesScreen_Show(struct LScreen* s_) { Fetcher_Run(); }

static void FetchResourcesScreen_Layout(struct LScreen* s_) {
	struct FetchResourcesScreen* s = (struct FetchResourcesScreen*)s_;
	LWidget_SetLocation(&s->lblStatus,   ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -10);
	LWidget_SetLocation(&s->btnCancel,   ANCHOR_CENTRE, ANCHOR_CENTRE, 0,  45);
	LWidget_SetLocation(&s->sdrProgress, ANCHOR_CENTRE, ANCHOR_CENTRE, 0,  15);
}

static void FetchResourcesScreen_SetStatus(struct FetchResourcesScreen* s, const cc_string* str) {
	struct LLabel* w = &s->lblStatus;
	CheckResourcesScreen_ResetArea(w->last.X, w->last.Y,
									w->last.Width, w->last.Height);
	LLabel_SetText(w, str);
	LWidget_Draw(w);
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
	FetchResourcesScreen_SetStatus(s, &str);
}

static void FetchResourcesScreen_UpdateProgress(struct FetchResourcesScreen* s) {
	int reqID, progress;

	if (!Http_GetCurrent(&reqID, &progress)) return;
	FetchResourcesScreen_UpdateStatus(s, reqID);
	/* making request still, haven't started download yet */
	if (progress < 0 || progress > 100) return;

	if (progress == s->sdrProgress.value) return;
	s->sdrProgress.value = progress;
	LWidget_Draw(&s->sdrProgress);
}

static void FetchResourcesScreen_Error(struct FetchResourcesScreen* s) {
	cc_string str; char buffer[STRING_SIZE];
	String_InitArray(str, buffer);

	Launcher_DisplayHttpError(Fetcher_Result, Fetcher_StatusCode, "downloading resources", &str);
	FetchResourcesScreen_SetStatus(s, &str);
}

static void FetchResourcesScreen_Tick(struct LScreen* s_) {
	struct FetchResourcesScreen* s = (struct FetchResourcesScreen*)s_;
	if (!Fetcher_Working) return;

	FetchResourcesScreen_UpdateProgress(s);
	Fetcher_Update();

	if (!Fetcher_Completed) return;
	if (Fetcher_Failed) { FetchResourcesScreen_Error(s); return; }

	Launcher_TryLoadTexturePack();
	CheckResourcesScreen_Next(NULL, 0);
}

void FetchResourcesScreen_SetActive(void) {
	struct FetchResourcesScreen* s = &FetchResourcesScreen;
	LScreen_Reset((struct LScreen*)s);
	s->Init   = FetchResourcesScreen_Init;
	s->Show   = FetchResourcesScreen_Show;
	s->Tick   = FetchResourcesScreen_Tick;
	s->Layout = FetchResourcesScreen_Layout;
	s->DrawBackground = CheckResourcesScreen_DrawBackground;
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

static void ServersScreen_Connect(void* w, int idx) {
	struct LTable* table = &ServersScreen.table;
	cc_string* hash      = &ServersScreen.iptHash.text;

	if (!hash->length && table->rowsCount) { 
		hash = &LTable_Get(table->topRow)->hash; 
	}
	Launcher_ConnectToServer(hash);
}

static void ServersScreen_Refresh(void* w, int idx) {
	struct LButton* btn;
	if (FetchServersTask.Base.working) return;

	FetchServersTask_Run();
	btn = &ServersScreen.btnRefresh;
	LButton_SetConst(btn, "&eWorking..");
	LWidget_Redraw(btn);
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
	LWidget_Draw(&s->table);
}

static void ServersScreen_HashChanged(struct LInput* w) {
	struct ServersScreen* s = &ServersScreen;
	LTable_ShowSelected(&s->table);
	LWidget_Draw(&s->table);
}

static void ServersScreen_OnSelectedChanged(void) {
	struct ServersScreen* s = &ServersScreen;
	LWidget_Redraw(&s->iptHash);
	LWidget_Draw(&s->table);
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

	LInput_Init( &s->iptSearch, 370, "Search servers..");
	LInput_Init( &s->iptHash,   475, "classicube.net/server/play/...");

	LButton_Init(&s->btnBack,    110, 30, "Back");
	LButton_Init(&s->btnConnect, 130, 30, "Connect");
	LButton_Init(&s->btnRefresh, 110, 30, "Refresh");

	s->btnBack.OnClick    = SwitchToMain;
	s->btnConnect.OnClick = ServersScreen_Connect;
	s->btnRefresh.OnClick = ServersScreen_Refresh;

	s->iptSearch.TextChanged   = ServersScreen_SearchChanged;
	s->iptHash.TextChanged     = ServersScreen_HashChanged;
	s->iptHash.ClipboardFilter = ServersScreen_HashFilter;

	LTable_Init(&s->table,  &s->rowFont);
	s->table.filter       = &s->iptSearch.text;
	s->table.selectedHash = &s->iptHash.text;
	s->table.OnSelectedChanged = ServersScreen_OnSelectedChanged;
}

static void ServersScreen_Show(struct LScreen* s_) {
	struct ServersScreen* s = (struct ServersScreen*)s_;
	Drawer2D_MakeFont(&s->rowFont, 11, FONT_FLAGS_NONE);

	s->table.rowFont = &s->rowFont;
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
	LWebTask_Tick(&FetchFlagsTask.Base);
	/* TODO: Only redraw flags */
	if (count != FetchFlagsTask.count) LWidget_Draw(&s->table);

	if (!FetchServersTask.Base.working) return;
	LWebTask_Tick(&FetchServersTask.Base);
	if (!FetchServersTask.Base.completed) return;

	if (FetchServersTask.Base.success) {
		ServersScreen_ReloadServers(s);
		LWidget_Draw(&s->table);
	}

	LButton_SetConst(&s->btnRefresh, 
				FetchServersTask.Base.success ? "Refresh" : "&cFailed");
	LWidget_Redraw(&s->btnRefresh);
}

static void ServersScreen_Free(struct LScreen* s_) {
	struct ServersScreen* s = (struct ServersScreen*)s_;
	Font_Free(&s->rowFont);
}

static void ServersScreen_Layout(struct LScreen* s_) {
	struct ServersScreen* s = (struct ServersScreen*)s_;
	LWidget_SetLocation(&s->iptSearch, ANCHOR_MIN, ANCHOR_MIN, 10, 10);
	LWidget_SetLocation(&s->iptHash,   ANCHOR_MIN, ANCHOR_MAX, 10, 10);

	LWidget_SetLocation(&s->btnBack,    ANCHOR_MAX, ANCHOR_MIN,  10, 10);
	LWidget_SetLocation(&s->btnConnect, ANCHOR_MAX, ANCHOR_MAX,  10, 10);
	LWidget_SetLocation(&s->btnRefresh, ANCHOR_MAX, ANCHOR_MIN, 135, 10);
	
	LWidget_SetLocation(&s->table, ANCHOR_MIN, ANCHOR_MIN, 10, 50);
	s->table.width  = WindowInfo.Width  - s->table.x;
	s->table.height = WindowInfo.Height - s->table.y * 2;
	s->table.height = max(1, s->table.height);

	LTable_Reposition(&s->table);
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
	LScreen_MouseUp(s_, idx);
}

void ServersScreen_SetActive(void) {
	struct ServersScreen* s = &ServersScreen;
	LScreen_Reset((struct LScreen*)s);
	s->tableAcc = 0.0f;

	s->Init       = ServersScreen_Init;
	s->Show       = ServersScreen_Show;
	s->Tick       = ServersScreen_Tick;
	s->Free       = ServersScreen_Free;
	s->Layout     = ServersScreen_Layout;
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
	struct LButton btnUpdates, btnMode, btnColours, btnBack;
	struct LLabel  lblUpdates, lblMode, lblColours;
	struct LCheckbox cbExtra, cbEmpty;
	struct LLine sep;
} SettingsScreen;

static struct LWidget* settings_widgets[] = {
	(struct LWidget*)&SettingsScreen.sep,
	(struct LWidget*)&SettingsScreen.btnUpdates, (struct LWidget*)&SettingsScreen.lblUpdates,
	(struct LWidget*)&SettingsScreen.btnMode,    (struct LWidget*)&SettingsScreen.lblMode,
	(struct LWidget*)&SettingsScreen.btnColours, (struct LWidget*)&SettingsScreen.lblColours,
	(struct LWidget*)&SettingsScreen.cbExtra,    (struct LWidget*)&SettingsScreen.cbEmpty,
	(struct LWidget*)&SettingsScreen.btnBack
};
static struct LWidget* settings_classic[] = {
	(struct LWidget*)&SettingsScreen.sep,
	(struct LWidget*)&SettingsScreen.btnUpdates, (struct LWidget*)&SettingsScreen.lblUpdates,
	(struct LWidget*)&SettingsScreen.btnMode,    (struct LWidget*)&SettingsScreen.lblMode,
	(struct LWidget*)&SettingsScreen.cbExtra,    (struct LWidget*)&SettingsScreen.cbEmpty,
	(struct LWidget*)&SettingsScreen.btnBack
};

#if defined CC_BUILD_MOBILE
static void SettingsScreen_LockOrientation(void* w, int idx) {
	struct LCheckbox* cb = (struct LCheckbox*)w;
	cb->value = !cb->value;
	Options_SetBool(OPT_LANDSCAPE_MODE, cb->value);
	Window_LockLandscapeOrientation(cb->value);
	Launcher_Redraw();
}
#else
static void SettingsScreen_AutoClose(void* w, int idx) {
	struct LCheckbox* cb = (struct LCheckbox*)w;
	cb->value = !cb->value;
	Options_SetBool(LOPT_AUTO_CLOSE, cb->value);
	LWidget_Redraw(cb);
}
#endif
static void SettingsScreen_ShowEmpty(void* w, int idx) {
	struct LCheckbox* cb = (struct LCheckbox*)w;
	cb->value = !cb->value;
	Launcher_ShowEmptyServers = cb->value;

	Options_SetBool(LOPT_SHOW_EMPTY, cb->value);
	LWidget_Redraw(cb);
}

static void SettingsScreen_Init(struct LScreen* s_) {
	struct SettingsScreen* s = (struct SettingsScreen*)s_;
	LLine_Init(  &s->sep, 380);

	LButton_Init(&s->btnUpdates, 110, 35, "Updates");
	LLabel_Init( &s->lblUpdates, "&eGet the latest stuff");

	LButton_Init(&s->btnMode, 110, 35, "Mode");
	LLabel_Init( &s->lblMode, "&eChange the enabled features");

	LButton_Init(&s->btnColours, 110, 35, "Theme");
	LLabel_Init( &s->lblColours, "&eChange how the launcher looks");

#if defined CC_BUILD_MOBILE
	LCheckbox_Init(&s->cbExtra, "Force landscape");
	s->cbExtra.OnClick = SettingsScreen_LockOrientation;
#else
	LCheckbox_Init(&s->cbExtra, "Close this after game starts");
	s->cbExtra.OnClick = SettingsScreen_AutoClose;
#endif

	LCheckbox_Init(&s->cbEmpty, "Show empty servers in list");
	s->cbEmpty.OnClick = SettingsScreen_ShowEmpty;
	LButton_Init(  &s->btnBack, 80, 35, "Back");

	s->btnMode.OnClick    = SwitchToChooseMode;
	s->btnUpdates.OnClick = SwitchToUpdates;
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
	s->cbExtra.value = Options_GetBool(OPT_LANDSCAPE_MODE, false);
#else
	s->cbExtra.value = Options_GetBool(LOPT_AUTO_CLOSE, false);
#endif
	s->cbEmpty.value = Launcher_ShowEmptyServers;
}

static void SettingsScreen_Layout(struct LScreen* s_) {
	struct SettingsScreen* s = (struct SettingsScreen*)s_;
	LWidget_SetLocation(&s->btnUpdates, ANCHOR_CENTRE,     ANCHOR_CENTRE, -135, -120);
	LWidget_SetLocation(&s->lblUpdates, ANCHOR_CENTRE_MIN, ANCHOR_CENTRE,  -70, -120);

	LWidget_SetLocation(&s->btnMode, ANCHOR_CENTRE,     ANCHOR_CENTRE, -135, -70);
	LWidget_SetLocation(&s->lblMode, ANCHOR_CENTRE_MIN, ANCHOR_CENTRE,  -70, -70);

	LWidget_SetLocation(&s->btnColours, ANCHOR_CENTRE,     ANCHOR_CENTRE, -135, -20);
	LWidget_SetLocation(&s->lblColours, ANCHOR_CENTRE_MIN, ANCHOR_CENTRE,  -70, -20);

	LWidget_SetLocation(&s->sep,      ANCHOR_CENTRE,     ANCHOR_CENTRE,    0, 15);
	LWidget_SetLocation(&s->cbExtra,  ANCHOR_CENTRE_MIN, ANCHOR_CENTRE, -190, 44);
	LWidget_SetLocation(&s->cbEmpty,  ANCHOR_CENTRE_MIN, ANCHOR_CENTRE, -190, 84);

	LWidget_SetLocation(&s->btnBack,  ANCHOR_CENTRE, ANCHOR_CENTRE,       0, 170);
}

void SettingsScreen_SetActive(void) {
	struct SettingsScreen* s = &SettingsScreen;
	LScreen_Reset((struct LScreen*)s);
	s->Init   = SettingsScreen_Init;
	s->Show   = SettingsScreen_Show;
	s->Layout = SettingsScreen_Layout;

	s->title_fore = "&fOptions";
	s->title_back = "&0Options";
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

static void ThemesScreen_Set(const struct LauncherTheme* theme) {
	Launcher_Theme = *theme;
	Launcher_SaveTheme();
	Launcher_Redraw();
}

static void ThemesScreen_Modern(void* w, int idx) {
	ThemesScreen_Set(&Launcher_ModernTheme);
}
static void ThemesScreen_Classic(void* w, int idx) {
	ThemesScreen_Set(&Launcher_ClassicTheme);
}
static void ThemesScreen_Nordic(void* w, int idx) {
	ThemesScreen_Set(&Launcher_NordicTheme);
}

static void ThemesScreen_Init(struct LScreen* s_) {
	struct ThemesScreen* s = (struct ThemesScreen*)s_;
	s->widgets    = themes_widgets;
	s->numWidgets = Array_Elems(themes_widgets);

	LButton_Init(&s->btnModern,  200, 35, "Modern");
	LButton_Init(&s->btnClassic, 200, 35, "Classic");
	LButton_Init(&s->btnNordic,  200, 35, "Nordic");
	LButton_Init(&s->btnCustom,  200, 35, "Custom");
	LButton_Init(&s->btnBack,     80, 35, "Back");

	s->btnModern.OnClick  = ThemesScreen_Modern;
	s->btnClassic.OnClick = ThemesScreen_Classic;
	s->btnNordic.OnClick  = ThemesScreen_Nordic;
	s->btnCustom.OnClick  = SwitchToColours;
	s->btnBack.OnClick    = SwitchToSettings;
}

static void ThemesScreen_Layout(struct LScreen* s_) {
	struct ThemesScreen* s = (struct ThemesScreen*)s_;
	LWidget_SetLocation(&s->btnModern,  ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -120);
	LWidget_SetLocation(&s->btnClassic, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -70);
	LWidget_SetLocation(&s->btnNordic,  ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -20);
	LWidget_SetLocation(&s->btnCustom,  ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 120);
	LWidget_SetLocation(&s->btnBack,    ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 170);
}

void ThemesScreen_SetActive(void) {
	struct ThemesScreen* s = &ThemesScreen;
	LScreen_Reset((struct LScreen*)s);
	s->Init   = ThemesScreen_Init;
	s->Layout = ThemesScreen_Layout;

	s->title_fore = "&fSelect theme";
	s->title_back = "&0Select theme";
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
	LWidget_Redraw(lbl);
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
	LWidget_Redraw(&s->lblStatus);
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
	LWebTask_Tick(&CheckUpdateTask.Base);

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
	LWidget_Redraw(&s->lblStatus);
}

static void UpdatesScreen_FetchTick(struct UpdatesScreen* s) {
	cc_string str; char strBuffer[STRING_SIZE];
	if (!FetchUpdateTask.Base.working) return;

	LWebTask_Tick(&FetchUpdateTask.Base);
	UpdatesScreen_UpdateProgress(s, &FetchUpdateTask.Base);
	if (!FetchUpdateTask.Base.completed) return;

	if (!FetchUpdateTask.Base.success) {
		String_InitArray(str, strBuffer);
		LWebTask_DisplayError(&FetchUpdateTask.Base, "fetching update", &str);
		LLabel_SetText(&s->lblStatus, &str);
		LWidget_Redraw(&s->lblStatus);
	} else {
		/* FetchUpdateTask handles saving the updated file for us */
		Launcher_ShouldExit   = true;
		Launcher_ShouldUpdate = true;
	}
}

static void UpdatesScreen_Rel_0(void* w, int idx) { UpdatesScreen_Get(true,  0); }
static void UpdatesScreen_Rel_1(void* w, int idx) { UpdatesScreen_Get(true,  1); }
static void UpdatesScreen_Dev_0(void* w, int idx) { UpdatesScreen_Get(false, 0); }
static void UpdatesScreen_Dev_1(void* w, int idx) { UpdatesScreen_Get(false, 1); }

static void UpdatesScreen_Init(struct LScreen* s_) {
	struct UpdatesScreen* s = (struct UpdatesScreen*)s_;
	s->widgets    = updates_widgets;
	s->numWidgets = Array_Elems(updates_widgets);

	if (Updater_Info.numBuilds < 2) s->numWidgets -= 2;
	if (Updater_Info.numBuilds < 1) s->numWidgets -= 2;

	LLabel_Init(&s->lblYour, "Your build: (unknown)");
	LLine_Init( &s->seps[0],   320);
	LLine_Init( &s->seps[1],   320);

	LLabel_Init( &s->lblRel, "Latest release: Checking..");	
	LLabel_Init( &s->lblDev, "Latest dev build: Checking..");
	LLabel_Init( &s->lblStatus, "");
	LButton_Init(&s->btnBack, 80, 35, "Back");

	if (Updater_Info.numBuilds >= 1) {
		LButton_Init(&s->btnRel[0], 130, 35, Updater_Info.builds[0].name);
		LButton_Init(&s->btnDev[0], 130, 35, Updater_Info.builds[0].name);
	}
	if (Updater_Info.numBuilds >= 2) {
		LButton_Init(&s->btnRel[1], 130, 35, Updater_Info.builds[1].name);
		LButton_Init(&s->btnDev[1], 130, 35, Updater_Info.builds[1].name);
	}
	LLabel_Init(&s->lblInfo, Updater_Info.info);

	s->btnRel[0].OnClick = UpdatesScreen_Rel_0;
	s->btnRel[1].OnClick = UpdatesScreen_Rel_1;
	s->btnDev[0].OnClick = UpdatesScreen_Dev_0;
	s->btnDev[1].OnClick = UpdatesScreen_Dev_1;
	s->btnBack.OnClick   = SwitchToSettings;
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

static void UpdatesScreen_Layout(struct LScreen* s_) {
	struct UpdatesScreen* s = (struct UpdatesScreen*)s_;
	LWidget_SetLocation(&s->lblYour, ANCHOR_CENTRE, ANCHOR_CENTRE, -5, -120);
	LWidget_SetLocation(&s->seps[0], ANCHOR_CENTRE, ANCHOR_CENTRE,  0, -100);
	LWidget_SetLocation(&s->seps[1], ANCHOR_CENTRE, ANCHOR_CENTRE,  0,   -5);

	LWidget_SetLocation(&s->lblRel,    ANCHOR_CENTRE, ANCHOR_CENTRE, -20, -75);
	LWidget_SetLocation(&s->btnRel[0], ANCHOR_CENTRE, ANCHOR_CENTRE, -80, -40);

	if (Updater_Info.numBuilds >= 2) {
		LWidget_SetLocation(&s->btnRel[1], ANCHOR_CENTRE, ANCHOR_CENTRE, 80, -40);
	} else {
		LWidget_SetLocation(&s->btnRel[0], ANCHOR_CENTRE, ANCHOR_CENTRE,  0, -40);
	}

	LWidget_SetLocation(&s->lblDev,    ANCHOR_CENTRE, ANCHOR_CENTRE, -30, 20);
	LWidget_SetLocation(&s->btnDev[0], ANCHOR_CENTRE, ANCHOR_CENTRE, -80, 55);

	if (Updater_Info.numBuilds >= 2) {
		LWidget_SetLocation(&s->btnDev[1], ANCHOR_CENTRE, ANCHOR_CENTRE, 80, 55);
	} else {
		LWidget_SetLocation(&s->btnDev[0], ANCHOR_CENTRE, ANCHOR_CENTRE,  0, 55);
	}

	LWidget_SetLocation(&s->lblInfo,   ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 105);
	LWidget_SetLocation(&s->lblStatus, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 130);
	LWidget_SetLocation(&s->btnBack,   ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 170);
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
	s->Layout = UpdatesScreen_Layout;

	s->title_fore = "&fUpdate game";
	s->title_back = "&0Update game";
	Launcher_SetScreen((struct LScreen*)s);
}
#endif
