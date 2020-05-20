#include "LScreens.h"
#include "LWidgets.h"
#include "LWeb.h"
#include "Launcher.h"
#include "Gui.h"
#include "Game.h"
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

#ifndef CC_BUILD_WEB
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

CC_NOINLINE static struct LWidget* LScreen_WidgetAt(struct LScreen* s, int x, int y) {
	struct LWidget* w;
	int i = 0;

	for (i = 0; i < s->numWidgets; i++) {
		w = s->widgets[i];
		if (w->hidden) continue;

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

CC_NOINLINE static void LScreen_SelectWidget(struct LScreen* s, struct LWidget* w, cc_bool was) {
	if (!w) return;
	w->selected       = true;
	s->selectedWidget = w;
	if (w->VTABLE->OnSelect) w->VTABLE->OnSelect(w, was);
}

CC_NOINLINE static void LScreen_UnselectWidget(struct LScreen* s, struct LWidget* w) {
	if (!w) return;
	w->selected       = false;
	s->selectedWidget = NULL;
	if (w->VTABLE->OnUnselect) w->VTABLE->OnUnselect(w);
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
		if (w->hidden || !w->tabSelectable) continue;

		LScreen_UnselectWidget(s, s->selectedWidget);
		LScreen_SelectWidget(s, w, false);
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
			s->selectedWidget->OnClick(s->selectedWidget, Mouse_X, Mouse_Y);
		} else if (s->hoveredWidget && s->hoveredWidget->OnClick) {
			s->hoveredWidget->OnClick(s->hoveredWidget,   Mouse_X, Mouse_Y);
		} else if (s->onEnterWidget) {
			s->onEnterWidget->OnClick(s->onEnterWidget,   Mouse_X, Mouse_Y);
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

static void LScreen_MouseDown(struct LScreen* s, int btn) {
	struct LWidget* over = LScreen_WidgetAt(s, Mouse_X, Mouse_Y);
	struct LWidget* prev = s->selectedWidget;

	if (prev && over != prev) LScreen_UnselectWidget(s, prev);
	if (over) LScreen_SelectWidget(s, over, over == prev);
}

static void LScreen_MouseUp(struct LScreen* s, int btn) {
	struct LWidget* over = LScreen_WidgetAt(s, Mouse_X, Mouse_Y);
	struct LWidget* prev = s->selectedWidget;

	/* if user moves mouse away, it doesn't count */
	if (over != prev) {
		LScreen_UnselectWidget(s, prev);
	} else if (over && over->OnClick) {
		over->OnClick(over, Mouse_X, Mouse_Y);
	}
}

static void LScreen_MouseMove(struct LScreen* s, int deltaX, int deltaY) {
	struct LWidget* over = LScreen_WidgetAt(s, Mouse_X, Mouse_Y);
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
		over->VTABLE->MouseMove(over, deltaX, deltaY, overSame);
	}
}
static void LScreen_MouseWheel(struct LScreen* s, float delta) { }

CC_NOINLINE static void LScreen_Reset(struct LScreen* s) {
	int i;

	s->Init = NULL; /* screens should always override this */
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

	/* reset all widgets mouse state */
	for (i = 0; i < s->numWidgets; i++) { 
		s->widgets[i]->hovered  = false;
		s->widgets[i]->selected = false;
	}

	s->onEnterWidget  = NULL;
	s->hoveredWidget  = NULL;
	s->selectedWidget = NULL;
}


static void SwitchToChooseMode(void* w, int x, int y) {
	Launcher_SetScreen(ChooseModeScreen_MakeInstance(false));
}
static void SwitchToColours(void* w, int x, int y) {
	Launcher_SetScreen(ColoursScreen_MakeInstance());
}
static void SwitchToDirectConnect(void* w, int x, int y) {
	Launcher_SetScreen(DirectConnectScreen_MakeInstance());
}
static void SwitchToMain(void* w, int x, int y) {
	Launcher_SetScreen(MainScreen_MakeInstance());
}
static void SwitchToSettings(void* w, int x, int y) {
	Launcher_SetScreen(SettingsScreen_MakeInstance());
}
static void SwitchToUpdates(void* w, int x, int y) {
	Launcher_SetScreen(UpdatesScreen_MakeInstance());
}


/*########################################################################################################################*
*-------------------------------------------------------ChooseModeScreen--------------------------------------------------*
*#########################################################################################################################*/
static struct ChooseModeScreen {
	LScreen_Layout
	struct LLine seps[2];
	struct LButton btnEnhanced, btnClassicHax, btnClassic, btnBack;
	struct LLabel  lblTitle, lblHelp, lblEnhanced[2], lblClassicHax[2], lblClassic[2];
	cc_bool firstTime;
	struct LWidget* _widgets[14];
} ChooseModeScreen_Instance;

CC_NOINLINE static void ChooseMode_Click(cc_bool classic, cc_bool classicHacks) {
	Launcher_ClassicBackground = classic;
	Options_Load();
	Options_SetBool(OPT_CLASSIC_MODE, classic);
	if (classic) Options_SetBool(OPT_CLASSIC_HACKS, classicHacks);

	Options_SetBool("nostalgia-classicbg", classic);
	Options_SetBool(OPT_CUSTOM_BLOCKS,   !classic);
	Options_SetBool(OPT_CPE,             !classic);
	Options_SetBool(OPT_SERVER_TEXTURES, !classic);
	Options_SetBool(OPT_CLASSIC_TABLIST, classic);
	Options_SetBool(OPT_CLASSIC_OPTIONS, classic);
	Options_Save();

	Launcher_SetScreen(MainScreen_MakeInstance());
}

static void UseModeEnhanced(void* w, int x, int y)   { ChooseMode_Click(false, false); }
static void UseModeClassicHax(void* w, int x, int y) { ChooseMode_Click(true,  true);  }
static void UseModeClassic(void* w, int x, int y)    { ChooseMode_Click(true,  false); }

static void ChooseModeScreen_Init(struct LScreen* s_) {
	struct ChooseModeScreen* s = (struct ChooseModeScreen*)s_;	

	s->lblHelp.hidden = !s->firstTime;
	s->btnBack.hidden = s->firstTime;
	s->seps[0].col    = Launcher_ButtonBorderCol;
	s->seps[1].col    = Launcher_ButtonBorderCol;

	if (s->numWidgets) return;
	s->widgets = s->_widgets;
	LLabel_Init(s_,  &s->lblTitle, "");
	LLine_Init(s_,   &s->seps[0], 490);
	LLine_Init(s_,   &s->seps[1], 490);

	LButton_Init(s_, &s->btnEnhanced, 145, 35, "Enhanced");
	LLabel_Init(s_,  &s->lblEnhanced[0], "&eEnables custom blocks, changing env");
	LLabel_Init(s_,  &s->lblEnhanced[1], "&esettings, longer messages, and more");

	LButton_Init(s_, &s->btnClassicHax, 145, 35, "Classic +hax");
	LLabel_Init(s_,  &s->lblClassicHax[0], "&eSame as Classic mode, except that");
	LLabel_Init(s_,  &s->lblClassicHax[1], "&ehacks (noclip/fly/speed) are enabled");

	LButton_Init(s_, &s->btnClassic, 145, 35, "Classic");
	LLabel_Init(s_,  &s->lblClassic[0], "&eOnly uses blocks and features from");
	LLabel_Init(s_,  &s->lblClassic[1], "&ethe original minecraft classic");

	LLabel_Init(s_,  &s->lblHelp, "&eClick &fEnhanced &eif you're not sure which mode to choose.");
	LButton_Init(s_, &s->btnBack, 80, 35, "Back");

	s->btnEnhanced.OnClick   = UseModeEnhanced;
	s->btnClassicHax.OnClick = UseModeClassicHax;
	s->btnClassic.OnClick    = UseModeClassic;
	s->btnBack.OnClick       = SwitchToSettings;

	s->lblTitle.font = &Launcher_TitleFont;
	LLabel_SetConst(&s->lblTitle, "Choose game mode");
}

static void ChooseModeScreen_Layout(struct LScreen* s_) {
	struct ChooseModeScreen* s = (struct ChooseModeScreen*)s_;
	LWidget_SetLocation(&s->lblTitle, ANCHOR_CENTRE, ANCHOR_CENTRE, 10, -135);
	LWidget_SetLocation(&s->seps[0],  ANCHOR_CENTRE, ANCHOR_CENTRE, -5,  -35);
	LWidget_SetLocation(&s->seps[1],  ANCHOR_CENTRE, ANCHOR_CENTRE, -5,   35);

	LWidget_SetLocation(&s->btnEnhanced,    ANCHOR_CENTRE_MIN, ANCHOR_CENTRE, -250, -72);
	LWidget_SetLocation(&s->lblEnhanced[0], ANCHOR_CENTRE_MIN, ANCHOR_CENTRE,  -85, -72 - 12);
	LWidget_SetLocation(&s->lblEnhanced[1], ANCHOR_CENTRE_MIN, ANCHOR_CENTRE,  -85, -72 + 12);

	LWidget_SetLocation(&s->btnClassicHax,    ANCHOR_CENTRE_MIN, ANCHOR_CENTRE, -250, 0);
	LWidget_SetLocation(&s->lblClassicHax[0], ANCHOR_CENTRE_MIN, ANCHOR_CENTRE,  -85, 0 - 12);
	LWidget_SetLocation(&s->lblClassicHax[1], ANCHOR_CENTRE_MIN, ANCHOR_CENTRE,  -85, 0 + 12);

	LWidget_SetLocation(&s->btnClassic,    ANCHOR_CENTRE_MIN, ANCHOR_CENTRE, -250, 72);
	LWidget_SetLocation(&s->lblClassic[0], ANCHOR_CENTRE_MIN, ANCHOR_CENTRE,  -85, 72 - 12);
	LWidget_SetLocation(&s->lblClassic[1], ANCHOR_CENTRE_MIN, ANCHOR_CENTRE,  -85, 72 + 12);

	LWidget_SetLocation(&s->lblHelp, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 160);
	LWidget_SetLocation(&s->btnBack, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 170);
}

struct LScreen* ChooseModeScreen_MakeInstance(cc_bool firstTime) {
	struct ChooseModeScreen* s = &ChooseModeScreen_Instance;
	LScreen_Reset((struct LScreen*)s);
	s->Init      = ChooseModeScreen_Init;
	s->Layout    = ChooseModeScreen_Layout;
	s->firstTime = firstTime;
	s->onEnterWidget = (struct LWidget*)&s->btnEnhanced;
	return (struct LScreen*)s;
}


/*########################################################################################################################*
*---------------------------------------------------------ColoursScreen---------------------------------------------------*
*#########################################################################################################################*/
static struct ColoursScreen {
	LScreen_Layout
	struct LButton btnDefault, btnBack;
	struct LLabel lblNames[5], lblRGB[3];
	struct LInput iptColours[5 * 3];
	struct LWidget* _widgets[25];
	float colourAcc;
} ColoursScreen_Instance;

CC_NOINLINE static void ColoursScreen_Update(struct ColoursScreen* s, int i, BitmapCol col) {
	String tmp; char tmpBuffer[3];

	String_InitArray(tmp, tmpBuffer);
	String_AppendInt(&tmp, BitmapCol_R(col));
	LInput_SetText(&s->iptColours[i + 0], &tmp);

	tmp.length = 0;
	String_AppendInt(&tmp, BitmapCol_G(col));
	LInput_SetText(&s->iptColours[i + 1], &tmp);

	tmp.length = 0;
	String_AppendInt(&tmp, BitmapCol_B(col));
	LInput_SetText(&s->iptColours[i + 2], &tmp);
}

CC_NOINLINE static void ColoursScreen_UpdateAll(struct ColoursScreen* s) {
	ColoursScreen_Update(s,  0, Launcher_BackgroundCol);
	ColoursScreen_Update(s,  3, Launcher_ButtonBorderCol);
	ColoursScreen_Update(s,  6, Launcher_ButtonHighlightCol);
	ColoursScreen_Update(s,  9, Launcher_ButtonForeCol);
	ColoursScreen_Update(s, 12, Launcher_ButtonForeActiveCol);
}

static void ColoursScreen_TextChanged(struct LInput* w) {
	struct ColoursScreen* s = &ColoursScreen_Instance;
	int index = LScreen_IndexOf((struct LScreen*)s, w);
	BitmapCol* col;
	cc_uint8 r, g, b;

	if (index < 3)       col = &Launcher_BackgroundCol;
	else if (index < 6)  col = &Launcher_ButtonBorderCol;
	else if (index < 9)  col = &Launcher_ButtonHighlightCol;
	else if (index < 12) col = &Launcher_ButtonForeCol;
	else                 col = &Launcher_ButtonForeActiveCol;

	/* if index of G input, changes to index of R input */
	index = (index / 3) * 3;
	if (!Convert_ParseUInt8(&s->iptColours[index + 0].text, &r)) return;
	if (!Convert_ParseUInt8(&s->iptColours[index + 1].text, &g)) return;
	if (!Convert_ParseUInt8(&s->iptColours[index + 2].text, &b)) return;

	*col = BitmapCol_Make(r, g, b, 255);
	Launcher_SaveSkin();
	Launcher_Redraw();
}

static void ColoursScreen_AdjustSelected(struct LScreen* s, int delta) {
	struct LInput* w;
	int index, newCol;
	cc_uint8 col;

	if (!s->selectedWidget) return;
	index = LScreen_IndexOf(s, s->selectedWidget);
	if (index >= 15) return;

	w = (struct LInput*)s->selectedWidget;
	if (!Convert_ParseUInt8(&w->text, &col)) return;
	newCol = col + delta;

	Math_Clamp(newCol, 0, 255);
	w->text.length = 0;
	String_AppendInt(&w->text, newCol);

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

static void ColoursScreen_ResetAll(void* w, int x, int y) {
	Launcher_ResetSkin();
	Launcher_SaveSkin();
	ColoursScreen_UpdateAll(&ColoursScreen_Instance);
	Launcher_Redraw();
}

static cc_bool ColoursScreen_InputFilter(char c) {
	return c >= '0' && c <= '9';
}

static void ColoursScreen_Init(struct LScreen* s_) {
	struct ColoursScreen* s = (struct ColoursScreen*)s_;
	int i;

	s->colourAcc = 0;
	if (s->numWidgets) return;
	s->widgets = s->_widgets;
	
	for (i = 0; i < 5 * 3; i++) {
		LInput_Init(s_, &s->iptColours[i], 55, NULL);
		s->iptColours[i].TextFilter  = ColoursScreen_InputFilter;
		s->iptColours[i].TextChanged = ColoursScreen_TextChanged;
	}

	LLabel_Init(s_, &s->lblNames[0], "Background");
	LLabel_Init(s_, &s->lblNames[1], "Button border");
	LLabel_Init(s_, &s->lblNames[2], "Button highlight");
	LLabel_Init(s_, &s->lblNames[3], "Button");
	LLabel_Init(s_, &s->lblNames[4], "Active button");

	LLabel_Init(s_, &s->lblRGB[0], "Red");
	LLabel_Init(s_, &s->lblRGB[1], "Green");
	LLabel_Init(s_, &s->lblRGB[2], "Blue");

	LButton_Init(s_, &s->btnDefault, 160, 35, "Default colours");
	LButton_Init(s_, &s->btnBack,    80,  35, "Back");

	s->btnDefault.OnClick = ColoursScreen_ResetAll;
	s->btnBack.OnClick    = SwitchToSettings;
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

	LWidget_SetLocation(&s->btnDefault, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 120);
	LWidget_SetLocation(&s->btnBack,    ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 170);
}

struct LScreen* ColoursScreen_MakeInstance(void) {
	struct ColoursScreen* s = &ColoursScreen_Instance;
	LScreen_Reset((struct LScreen*)s);
	s->Init       = ColoursScreen_Init;
	s->Layout     = ColoursScreen_Layout;
	s->KeyDown    = ColoursScreen_KeyDown;
	s->MouseWheel = ColoursScreen_MouseWheel;
	return (struct LScreen*)s;
}


/*########################################################################################################################*
*------------------------------------------------------DirectConnectScreen------------------------------------------------*
*#########################################################################################################################*/
static struct DirectConnectScreen {
	LScreen_Layout
	struct LButton btnConnect, btnBack;
	struct LInput iptUsername, iptAddress, iptMppass;
	struct LLabel lblStatus;
	struct LWidget* _widgets[6];
} DirectConnectScreen_Instance;

CC_NOINLINE static void DirectConnectScreen_SetStatus(const char* text) {
	struct LLabel* w = &DirectConnectScreen_Instance.lblStatus;
	LLabel_SetConst(w, text);
	LWidget_Redraw(w);
}

static void DirectConnectScreen_Load(struct DirectConnectScreen* s) {
	String addr; char addrBuffer[STRING_SIZE];
	String mppass; char mppassBuffer[STRING_SIZE];
	String user, ip, port;
	Options_Load();

	Options_UNSAFE_Get("launcher-dc-username", &user);
	Options_UNSAFE_Get("launcher-dc-ip",       &ip);
	Options_UNSAFE_Get("launcher-dc-port",     &port);
	
	String_InitArray(mppass, mppassBuffer);
	Options_GetSecure("launcher-dc-mppass", &mppass, &user);
	String_InitArray(addr, addrBuffer);
	String_Format2(&addr, "%s:%s", &ip, &port);

	/* don't want just ':' for address */
	if (addr.length == 1) addr.length = 0;
	
	LInput_SetText(&s->iptUsername, &user);
	LInput_SetText(&s->iptAddress,  &addr);
	LInput_SetText(&s->iptMppass,   &mppass);
}

static void DirectConnectScreen_Save(const String* user, const String* mppass, const String* ip, const String* port) {
	Options_Load();
	Options_Set("launcher-dc-username", user);
	Options_Set("launcher-dc-ip",       ip);
	Options_Set("launcher-dc-port",     port);
	Options_SetSecure("launcher-dc-mppass", mppass, user);
}

static void DirectConnectScreen_StartClient(void* w, int x, int y) {
	static const String loopbackIp = String_FromConst("127.0.0.1");
	static const String defMppass  = String_FromConst("(none)");
	const String* user   = &DirectConnectScreen_Instance.iptUsername.text;
	const String* addr   = &DirectConnectScreen_Instance.iptAddress.text;
	const String* mppass = &DirectConnectScreen_Instance.iptMppass.text;

	String ip, port;
	cc_uint8  raw_ip[4];
	cc_uint16 raw_port;

	int index = String_LastIndexOf(addr, ':');
	if (index <= 0 || index == addr->length - 1) {
		DirectConnectScreen_SetStatus("&eInvalid address"); return;
	}

	ip   = String_UNSAFE_Substring(addr, 0, index);
	port = String_UNSAFE_SubstringAt(addr, index + 1);
	if (String_CaselessEqualsConst(&ip, "localhost")) ip = loopbackIp;

	if (!user->length) {
		DirectConnectScreen_SetStatus("&eUsername required"); return;
	}
	if (!Utils_ParseIP(&ip, raw_ip)) {
		DirectConnectScreen_SetStatus("&eInvalid ip"); return;
	}
	if (!Convert_ParseUInt16(&port, &raw_port)) {
		DirectConnectScreen_SetStatus("&eInvalid port"); return;
	}
	if (!mppass->length) mppass = &defMppass;

	DirectConnectScreen_Save(user, mppass, &ip, &port);
	DirectConnectScreen_SetStatus("");
	Launcher_StartGame(user, mppass, &ip, &port, &String_Empty);
}

static void DirectConnectScreen_Init(struct LScreen* s_) {
	struct DirectConnectScreen* s = (struct DirectConnectScreen*)s_;
	if (s->numWidgets) return;
	s->widgets = s->_widgets;

	LInput_Init(s_, &s->iptUsername, 330, "&gUsername..");
	LInput_Init(s_, &s->iptAddress,  330, "&gIP address:Port number..");
	LInput_Init(s_, &s->iptMppass,   330, "&gMppass..");

	LButton_Init(s_, &s->btnConnect, 110, 35, "Connect");
	LButton_Init(s_, &s->btnBack,     80, 35, "Back");
	LLabel_Init(s_,  &s->lblStatus, "");

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

struct LScreen* DirectConnectScreen_MakeInstance(void) {
	struct DirectConnectScreen* s = &DirectConnectScreen_Instance;
	LScreen_Reset((struct LScreen*)s);
	s->Init          = DirectConnectScreen_Init;
	s->Layout        = DirectConnectScreen_Layout;
	s->onEnterWidget = (struct LWidget*)&s->btnConnect;
	return (struct LScreen*)s;
}


/*########################################################################################################################*
*----------------------------------------------------------MainScreen-----------------------------------------------------*
*#########################################################################################################################*/
static struct MainScreen {
	LScreen_Layout
	struct LButton btnLogin, btnResume, btnDirect, btnSPlayer, btnOptions, btnRegister;
	struct LInput iptUsername, iptPassword;
	struct LLabel lblStatus, lblUpdate;
	struct LWidget* _widgets[10];
	cc_bool signingIn;
} MainScreen_Instance;

struct ResumeInfo {
	String user, ip, port, server, mppass;
	char _userBuffer[STRING_SIZE], _serverBuffer[STRING_SIZE];
	char _ipBuffer[16], _portBuffer[16], _mppassBuffer[STRING_SIZE];
	cc_bool valid;
};

CC_NOINLINE static void MainScreen_GetResume(struct ResumeInfo* info, cc_bool full) {
	String_InitArray(info->server,   info->_serverBuffer);
	Options_Get("launcher-server",   &info->server, "");
	String_InitArray(info->user,     info->_userBuffer);
	Options_Get("launcher-username", &info->user, "");

	String_InitArray(info->ip,   info->_ipBuffer);
	Options_Get("launcher-ip",   &info->ip, "");
	String_InitArray(info->port, info->_portBuffer);
	Options_Get("launcher-port", &info->port, "");

	if (!full) return;
	String_InitArray(info->mppass, info->_mppassBuffer);
	Options_GetSecure("launcher-mppass", &info->mppass, &info->user);

	info->valid = 
		info->user.length && info->mppass.length &&
		info->ip.length   && info->port.length;
}

CC_NOINLINE static void MainScreen_Error(struct LWebTask* task, const char* action) {
	String str; char strBuffer[STRING_SIZE];
	struct MainScreen* s = &MainScreen_Instance;
	String_InitArray(str, strBuffer);

	LWebTask_DisplayError(task, action, &str);
	LLabel_SetText(&s->lblStatus, &str);
	LWidget_Redraw(&s->lblStatus);
	s->signingIn = false;
}

static void MainScreen_Login(void* w, int x, int y) {
	struct MainScreen* s = &MainScreen_Instance;
	String* user = &s->iptUsername.text;
	String* pass = &s->iptPassword.text;

	if (!user->length) {
		LLabel_SetConst(&s->lblStatus, "&eUsername required");
		LWidget_Redraw(&s->lblStatus); return;
	}
	if (!pass->length) {
		LLabel_SetConst(&s->lblStatus, "&ePassword required");
		LWidget_Redraw(&s->lblStatus); return;
	}

	if (GetTokenTask.Base.working) return;
	Options_Set("launcher-cc-username", user);
	Options_SetSecure("launcher-cc-password", pass, user);

	GetTokenTask_Run();
	LLabel_SetConst(&s->lblStatus, "&eSigning in..");
	LWidget_Redraw(&s->lblStatus);
	s->signingIn = true;
}

static void MainScreen_Register(void* w, int x, int y) {
	static const String ccUrl = String_FromConst("https://www.classicube.net/acc/register/");
	Process_StartOpen(&ccUrl);
}

static void MainScreen_Resume(void* w, int x, int y) {
	struct ResumeInfo info;
	MainScreen_GetResume(&info, true);

	if (!info.valid) return;
	Launcher_StartGame(&info.user, &info.mppass, &info.ip, &info.port, &info.server);
}

static void MainScreen_Singleplayer(void* w, int x, int y) {
	static const String defUser = String_FromConst("Singleplayer");
	const String* user = &MainScreen_Instance.iptUsername.text;

	if (!user->length) user = &defUser;
	Launcher_StartGame(user, &String_Empty, &String_Empty, &String_Empty, &String_Empty);
}

static cc_bool MainScreen_PasswordFilter(char c) { return true; }

static void MainScreen_Init(struct LScreen* s_) {
	String user, pass; char passBuffer[STRING_SIZE];
	struct MainScreen* s = (struct MainScreen*)s_;

	/* status should reset after user has gone to another menu */
	s->lblStatus.text.length = 0;
	if (s->numWidgets) return;
	s->widgets = s->_widgets;

	LInput_Init(s_, &s->iptUsername, 280, "&gUsername..");
	LInput_Init(s_, &s->iptPassword, 280, "&gPassword..");

	LButton_Init(s_, &s->btnLogin, 100, 35, "Sign in");
	LLabel_Init(s_,  &s->lblStatus, "");

	LButton_Init(s_, &s->btnResume,  100, 35, "Resume");
	LButton_Init(s_, &s->btnDirect,  200, 35, "Direct connect");
	LButton_Init(s_, &s->btnSPlayer, 200, 35, "Singleplayer");

	LLabel_Init(s_,  &s->lblUpdate, "");
	LButton_Init(s_, &s->btnOptions,  100, 35, "Options");
	LButton_Init(s_, &s->btnRegister, 100, 35, "Register");
	
	s->btnLogin.OnClick    = MainScreen_Login;
	s->btnResume.OnClick   = MainScreen_Resume;
	s->btnDirect.OnClick   = SwitchToDirectConnect;
	s->btnSPlayer.OnClick  = MainScreen_Singleplayer;
	s->btnOptions.OnClick  = SwitchToSettings;
	s->btnRegister.OnClick = MainScreen_Register;

	/* need to set text here for right size */
	s->lblUpdate.font = &Launcher_HintFont;
	LLabel_SetConst(&s->lblUpdate, "&eChecking..");
	s->iptPassword.password   = true;
	s->iptPassword.TextFilter = MainScreen_PasswordFilter;
	
	String_InitArray(pass, passBuffer);
	Options_UNSAFE_Get("launcher-cc-username", &user);
	Options_GetSecure("launcher-cc-password", &pass, &user);

	LInput_SetText(&s->iptUsername, &user);
	LInput_SetText(&s->iptPassword, &pass);
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
	String str; char strBuffer[STRING_SIZE];
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

CC_NOINLINE static cc_uint32 MainScreen_GetVersion(const String* version) {
	cc_uint8 raw[4] = { 0, 0, 0, 0 };
	String parts[4];
	int i, count;
	
	/* 1.0.1 -> { 1, 0, 1, 0 } */
	count = String_UNSAFE_Split(version, '.', parts, 4);
	for (i = 0; i < count; i++) {
		Convert_ParseUInt8(&parts[i], &raw[i]);
	}
	return Stream_GetU32_BE(raw);
}

static void MainScreen_TickCheckUpdates(struct MainScreen* s) {
	static const String currentStr = String_FromConst(GAME_APP_VER);
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

static void MainScreen_TickGetToken(struct MainScreen* s) {
	if (!GetTokenTask.Base.working)   return;
	LWebTask_Tick(&GetTokenTask.Base);
	if (!GetTokenTask.Base.completed) return;

	if (GetTokenTask.Base.success) {
		SignInTask_Run(&s->iptUsername.text, &s->iptPassword.text);
	} else {
		MainScreen_Error(&GetTokenTask.Base, "signing in");
	}
}

static void MainScreen_TickSignIn(struct MainScreen* s) {
	if (!SignInTask.Base.working)   return;
	LWebTask_Tick(&SignInTask.Base);
	if (!SignInTask.Base.completed) return;

	if (SignInTask.error) {
		LLabel_SetConst(&s->lblStatus, SignInTask.error);
		LWidget_Redraw(&s->lblStatus);
	} else if (SignInTask.Base.success) {
		/* website returns case correct username */
		if (!String_Equals(&s->iptUsername.text, &SignInTask.username)) {
			LInput_SetText(&s->iptUsername, &SignInTask.username);
			LWidget_Redraw(&s->iptUsername);
		}

		FetchServersTask_Run();
		LLabel_SetConst(&s->lblStatus, "&eRetrieving servers list..");
		LWidget_Redraw(&s->lblStatus);
	} else {
		MainScreen_Error(&SignInTask.Base, "signing in");
	}
}

static void MainScreen_TickFetchServers(struct MainScreen* s) {
	if (!FetchServersTask.Base.working)   return;
	LWebTask_Tick(&FetchServersTask.Base);
	if (!FetchServersTask.Base.completed) return;

	if (FetchServersTask.Base.success) {
		s->signingIn = false;
		if (Game_Hash.length) {
			Launcher_ConnectToServer(&Game_Hash);
			Game_Hash.length = 0;
		} else {
			Launcher_SetScreen(ServersScreen_MakeInstance());
		}
	} else {
		MainScreen_Error(&FetchServersTask.Base, "retrieving servers list");
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

struct LScreen* MainScreen_MakeInstance(void) {
	struct MainScreen* s = &MainScreen_Instance;
	LScreen_Reset((struct LScreen*)s);
	s->Init   = MainScreen_Init;
	s->Tick   = MainScreen_Tick;
	s->Layout = MainScreen_Layout;
	s->HoverWidget   = MainScreen_HoverWidget;
	s->UnhoverWidget = MainScreen_UnhoverWidget;
	s->onEnterWidget = (struct LWidget*)&s->btnLogin;
	return (struct LScreen*)s;
}


/*########################################################################################################################*
*-------------------------------------------------------ResourcesScreen---------------------------------------------------*
*#########################################################################################################################*/
static struct ResourcesScreen {
	LScreen_Layout
	struct LLabel  lblLine1, lblLine2, lblStatus;
	struct LButton btnYes, btnNo, btnCancel;
	struct LSlider sdrProgress;
	int statusYOffset; /* gets changed when downloading resources */
	struct LWidget* _widgets[7];
} ResourcesScreen_Instance;

static void ResourcesScreen_Download(void* w, int x, int y) {
	struct ResourcesScreen* s = &ResourcesScreen_Instance;
	if (Fetcher_Working) return;

	Fetcher_Run();
	s->selectedWidget = NULL;

	s->btnYes.hidden   = true;
	s->btnNo.hidden    = true;
	s->lblLine1.hidden = true;
	s->lblLine2.hidden = true;
	s->btnCancel.hidden   = false;
	s->sdrProgress.hidden = false;
	s->Draw((struct LScreen*)s);
}

static void ResourcesScreen_Next(void* w, int x, int y) {
	static const String optionsTxt = String_FromConst("options.txt");
	Http_ClearPending();

	if (File_Exists(&optionsTxt)) {
		Launcher_SetScreen(MainScreen_MakeInstance());
	} else {
		Launcher_SetScreen(ChooseModeScreen_MakeInstance(true));
	}
}

static void ResourcesScreen_Init(struct LScreen* s_) {
	String str; char buffer[STRING_SIZE];
	BitmapCol progressCol = BitmapCol_Make(0, 220, 0, 255);
	struct ResourcesScreen* s = (struct ResourcesScreen*)s_;
	float size;

	s->statusYOffset = 10;
	if (s->numWidgets) return;
	s->widgets = s->_widgets;

	LLabel_Init(s_, &s->lblLine1, "Some required resources weren't found");
	LLabel_Init(s_, &s->lblLine2, "Okay to download?");
	LLabel_Init(s_, &s->lblStatus, "");
	
	LButton_Init(s_, &s->btnYes, 70, 35, "Yes");
	LButton_Init(s_, &s->btnNo,  70, 35, "No");

	LButton_Init(s_, &s->btnCancel, 120, 35, "Cancel");
	LSlider_Init(s_, &s->sdrProgress, 200, 12, progressCol);

	s->btnCancel.hidden   = true;
	s->sdrProgress.hidden = true;

	/* TODO: Size 13 italic font?? does it matter?? */
	String_InitArray(str, buffer);
	size = Resources_Size / 1024.0f;

	s->lblStatus.font = &Launcher_HintFont;
	String_Format1(&str, "&eDownload size: %f2 megabytes", &size);
	LLabel_SetText(&s->lblStatus, &str);

	s->btnYes.OnClick    = ResourcesScreen_Download;
	s->btnNo.OnClick     = ResourcesScreen_Next;
	s->btnCancel.OnClick = ResourcesScreen_Next;
}

static void ResourcesScreen_Layout(struct LScreen* s_) {
	struct ResourcesScreen* s = (struct ResourcesScreen*)s_;
	
	LWidget_SetLocation(&s->lblLine1,  ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -50);
	LWidget_SetLocation(&s->lblLine2,  ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -30);
	LWidget_SetLocation(&s->lblStatus, ANCHOR_CENTRE, ANCHOR_CENTRE, 0,  s->statusYOffset);

	LWidget_SetLocation(&s->btnYes, ANCHOR_CENTRE, ANCHOR_CENTRE, -70, 45);
	LWidget_SetLocation(&s->btnNo,  ANCHOR_CENTRE, ANCHOR_CENTRE,  70, 45);

	LWidget_SetLocation(&s->btnCancel,   ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 45);
	LWidget_SetLocation(&s->sdrProgress, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 15);
}

CC_NOINLINE static void ResourcesScreen_ResetArea(int x, int y, int width, int height) {
	BitmapCol boxCol = BitmapCol_Make(120, 85, 151, 255);
	Gradient_Noise(&Launcher_Framebuffer, boxCol, 4, x, y, width, height);
	Launcher_MarkDirty(x, y, width, height);
}

static void ResourcesScreen_Draw(struct LScreen* s) {
	BitmapCol backCol = BitmapCol_Make(12, 12, 12, 255);
	int x, y, width, height;

	Drawer2D_Clear(&Launcher_Framebuffer, backCol, 
					0, 0, WindowInfo.Width, WindowInfo.Height);
	width  = Display_ScaleX(380);
	height = Display_ScaleY(140);

	x = Gui_CalcPos(ANCHOR_CENTRE, 0, width,  WindowInfo.Width);
	y = Gui_CalcPos(ANCHOR_CENTRE, 0, height, WindowInfo.Height);

	ResourcesScreen_ResetArea(x, y, width, height);
	LScreen_Draw(s);
}

static void ResourcesScreen_SetStatus(const String* str) {
	struct LLabel* w = &ResourcesScreen_Instance.lblStatus;
	ResourcesScreen_ResetArea(w->last.X, w->last.Y, 
							  w->last.Width, w->last.Height);
	LLabel_SetText(w, str);

	w->yOffset = -10;
	ResourcesScreen_Instance.statusYOffset = w->yOffset;	
	LWidget_CalcPosition(w);
	LWidget_Draw(w);
}

static void ResourcesScreen_UpdateStatus(struct HttpRequest* req) {
	String str; char strBuffer[STRING_SIZE];
	String id;
	struct LLabel* w = &ResourcesScreen_Instance.lblStatus;
	int count;

	id = String_FromRawArray(req->id);
	String_InitArray(str, strBuffer);
	count = Fetcher_Downloaded + 1;
	String_Format3(&str, "&eFetching %s.. (%i/%i)", &id, &count, &Resources_Count);

	if (String_Equals(&str, &w->text)) return;
	ResourcesScreen_SetStatus(&str);
}

static void ResourcesScreen_UpdateProgress(struct ResourcesScreen* s) {
	struct HttpRequest req;
	int progress;

	if (!Http_GetCurrent(&req, &progress)) return;
	ResourcesScreen_UpdateStatus(&req);
	/* making request still, haven't started download yet */
	if (progress < 0 || progress > 100) return;

	if (progress == s->sdrProgress.value) return;
	s->sdrProgress.value = progress;
	LWidget_Draw(&s->sdrProgress);
}

static void ResourcesScreen_Error(struct ResourcesScreen* s) {
	String str; char buffer[STRING_SIZE];
	String_InitArray(str, buffer);

	Launcher_DisplayHttpError(Fetcher_Result, Fetcher_StatusCode, "downloading resources", &str);
	ResourcesScreen_SetStatus(&str);
}

static void ResourcesScreen_Tick(struct LScreen* s_) {
	struct ResourcesScreen* s = (struct ResourcesScreen*)s_;
	if (!Fetcher_Working) return;

	ResourcesScreen_UpdateProgress(s);
	Fetcher_Update();

	if (!Fetcher_Completed) return;
	if (Fetcher_Failed) { ResourcesScreen_Error(s); return; }

	Launcher_TryLoadTexturePack();
	ResourcesScreen_Next(NULL, 0, 0);
}

struct LScreen* ResourcesScreen_MakeInstance(void) {
	struct ResourcesScreen* s = &ResourcesScreen_Instance;
	LScreen_Reset((struct LScreen*)s);
	s->Init   = ResourcesScreen_Init;
	s->Draw   = ResourcesScreen_Draw;
	s->Tick   = ResourcesScreen_Tick;
	s->Layout = ResourcesScreen_Layout;
	s->onEnterWidget = (struct LWidget*)&s->btnYes;
	return (struct LScreen*)s;
}


/*########################################################################################################################*
*--------------------------------------------------------ServersScreen----------------------------------------------------*
*#########################################################################################################################*/
static struct ServersScreen {
	LScreen_Layout
	struct LInput iptSearch, iptHash;
	struct LButton btnBack, btnConnect, btnRefresh;
	struct LTable table;
	struct LWidget* _widgets[6];
	struct FontDesc rowFont;
	float tableAcc;
} ServersScreen_Instance;

static void ServersScreen_Connect(void* w, int x, int y) {
	struct LTable* table = &ServersScreen_Instance.table;
	String* hash         = &ServersScreen_Instance.iptHash.text;

	if (!hash->length && table->rowsCount) { 
		hash = &LTable_Get(table->topRow)->hash; 
	}
	Launcher_ConnectToServer(hash);
}

static void ServersScreen_Refresh(void* w, int x, int y) {
	struct LButton* btn;
	if (FetchServersTask.Base.working) return;

	FetchServersTask_Run();
	btn = &ServersScreen_Instance.btnRefresh;
	LButton_SetConst(btn, "&eWorking..");
	LWidget_Redraw(btn);
}

static void ServersScreen_HashFilter(String* str) {
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
	struct ServersScreen* s = &ServersScreen_Instance;
	LTable_ApplyFilter(&s->table);
	LWidget_Draw(&s->table);
}

static void ServersScreen_HashChanged(struct LInput* w) {
	struct ServersScreen* s = &ServersScreen_Instance;
	LTable_ShowSelected(&s->table);
	LWidget_Draw(&s->table);
}

static void ServersScreen_OnSelectedChanged(void) {
	struct ServersScreen* s = &ServersScreen_Instance;
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

static void ServersScreen_InitWidgets(struct LScreen* s_) {
	struct ServersScreen* s = (struct ServersScreen*)s_;
	s->widgets = s->_widgets;

	LInput_Init(s_, &s->iptSearch, 370, "&gSearch servers..");
	LInput_Init(s_, &s->iptHash,   475, "&gclassicube.net/server/play/...");

	LButton_Init(s_, &s->btnBack,    110, 30, "Back");
	LButton_Init(s_, &s->btnConnect, 130, 30, "Connect");
	LButton_Init(s_, &s->btnRefresh, 110, 30, "Refresh");

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

	s->widgets[s->numWidgets++] = (struct LWidget*)&s->table;
}

static void ServersScreen_Init(struct LScreen* s_) {
	struct ServersScreen* s = (struct ServersScreen*)s_;
	Drawer2D_MakeFont(&s->rowFont, 11, FONT_STYLE_NORMAL);

	if (!s->numWidgets) ServersScreen_InitWidgets(s_);
	s->table.rowFont = &s->rowFont;
	/* also resets hash and filter */
	LTable_Reset(&s->table);

	ServersScreen_ReloadServers(s);
	/* This is so typing on keyboard by default searchs server list */
	/* But don't do that when it would cause on-screen keyboard to show */
	if (WindowInfo.SoftKeyboard) return;
	LScreen_SelectWidget(s_, (struct LWidget*)&s->iptSearch, false);
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

static void ServersScreen_MouseUp(struct LScreen* s_, int btn) {
	struct ServersScreen* s = (struct ServersScreen*)s_;
	s->table.VTABLE->OnUnselect(&s->table);
	LScreen_MouseUp(s_, btn);
}

struct LScreen* ServersScreen_MakeInstance(void) {
	struct ServersScreen* s = &ServersScreen_Instance;
	LScreen_Reset((struct LScreen*)s);
	s->hidesTitlebar = true;
	s->tableAcc = 0.0f;

	s->Init       = ServersScreen_Init;
	s->Tick       = ServersScreen_Tick;
	s->Free       = ServersScreen_Free;
	s->Layout     = ServersScreen_Layout;
	s->MouseWheel = ServersScreen_MouseWheel;
	s->KeyDown    = ServersScreen_KeyDown;
	s->MouseUp    = ServersScreen_MouseUp;
	s->onEnterWidget = (struct LWidget*)&s->btnConnect;
	return (struct LScreen*)s;
}


/*########################################################################################################################*
*--------------------------------------------------------SettingsScreen---------------------------------------------------*
*#########################################################################################################################*/
static struct SettingsScreen {
	LScreen_Layout
	struct LButton btnUpdates, btnMode, btnColours, btnBack;
	struct LLabel  lblUpdates, lblMode, lblColours;
	struct LWidget* _widgets[7];
} SettingsScreen_Instance;

static void SettingsScreen_Init(struct LScreen* s_) {
	struct SettingsScreen* s = (struct SettingsScreen*)s_;

	s->btnColours.hidden = Launcher_ClassicBackground;
	s->lblColours.hidden = Launcher_ClassicBackground;

	if (s->numWidgets) return;
	s->widgets = s->_widgets;

	LButton_Init(s_, &s->btnUpdates, 110, 35, "Updates");
	LLabel_Init(s_,  &s->lblUpdates, "&eGet the latest stuff");

	LButton_Init(s_, &s->btnMode, 110, 35, "Mode");
	LLabel_Init(s_,  &s->lblMode, "&eChange the enabled features");

	LButton_Init(s_, &s->btnColours, 110, 35, "Colours");
	LLabel_Init(s_,  &s->lblColours, "&eChange how the launcher looks");

	LButton_Init(s_, &s->btnBack, 80, 35, "Back");

	s->btnMode.OnClick    = SwitchToChooseMode;
	s->btnUpdates.OnClick = SwitchToUpdates;
	s->btnColours.OnClick = SwitchToColours;
	s->btnBack.OnClick    = SwitchToMain;
}

static void SettingsScreen_Layout(struct LScreen* s_) {
	struct SettingsScreen* s = (struct SettingsScreen*)s_;
	LWidget_SetLocation(&s->btnUpdates, ANCHOR_CENTRE,     ANCHOR_CENTRE, -135, -120);
	LWidget_SetLocation(&s->lblUpdates, ANCHOR_CENTRE_MIN, ANCHOR_CENTRE,  -70, -120);

	LWidget_SetLocation(&s->btnMode, ANCHOR_CENTRE,     ANCHOR_CENTRE, -135, -70);
	LWidget_SetLocation(&s->lblMode, ANCHOR_CENTRE_MIN, ANCHOR_CENTRE,  -70, -70);

	LWidget_SetLocation(&s->btnColours, ANCHOR_CENTRE,     ANCHOR_CENTRE, -135, -20);
	LWidget_SetLocation(&s->lblColours, ANCHOR_CENTRE_MIN, ANCHOR_CENTRE,  -70, -20);

	LWidget_SetLocation(&s->btnBack, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 170);
}

struct LScreen* SettingsScreen_MakeInstance(void) {
	struct SettingsScreen* s = &SettingsScreen_Instance;
	LScreen_Reset((struct LScreen*)s);
	s->Init   = SettingsScreen_Init;
	s->Layout = SettingsScreen_Layout;
	return (struct LScreen*)s;
}


/*########################################################################################################################*
*--------------------------------------------------------UpdatesScreen----------------------------------------------------*
*#########################################################################################################################*/
static struct UpdatesScreen {
	LScreen_Layout
	struct LLine seps[2];
	struct LButton btnRel[2], btnDev[2], btnBack;
	struct LLabel  lblYour, lblRel, lblDev, lblInfo, lblStatus;
	struct LWidget* _widgets[12];
	const char* buildName;
	int buildProgress;
} UpdatesScreen_Instance;

CC_NOINLINE static void UpdatesScreen_FormatTime(String* str, char* type, int delta, int unit) {
	delta /= unit;
	String_AppendInt(str, delta);
	String_Append(str, ' ');
	String_AppendConst(str, type);

	if (delta > 1) String_Append(str, 's');
	String_AppendConst(str, " ago");
}

static void UpdatesScreen_Format(struct LLabel* lbl, const char* prefix, cc_uint64 timestamp) {
	String str; char buffer[STRING_SIZE];
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

		if (delta < SECS_PER_MIN) {
			UpdatesScreen_FormatTime(&str, "second", delta, 1);
		} else if (delta < SECS_PER_HOUR) {
			UpdatesScreen_FormatTime(&str, "minute", delta, SECS_PER_MIN);
		} else if (delta < SECS_PER_DAY) {
			UpdatesScreen_FormatTime(&str, "hour",   delta, SECS_PER_HOUR);
		} else {
			UpdatesScreen_FormatTime(&str, "day",    delta, SECS_PER_DAY);
		}
	}
	LLabel_SetText(lbl, &str);
	LWidget_Redraw(lbl);
}

static void UpdatesScreen_FormatBoth(struct UpdatesScreen* s) {
	UpdatesScreen_Format(&s->lblRel, "Latest release: ",   CheckUpdateTask.relTimestamp);
	UpdatesScreen_Format(&s->lblDev, "Latest dev build: ", CheckUpdateTask.devTimestamp);
}

static void UpdatesScreen_Get(cc_bool release, cc_bool d3d9) {
	String str; char strBuffer[STRING_SIZE];
	struct UpdatesScreen* s = &UpdatesScreen_Instance;

	TimeMS time = release ? CheckUpdateTask.relTimestamp : CheckUpdateTask.devTimestamp;
	if (!time || FetchUpdateTask.Base.working) return;
	FetchUpdateTask_Run(release, d3d9);

	if (release && d3d9)   s->buildName = "&eFetching latest release (Direct3D9)";
	if (release && !d3d9)  s->buildName = "&eFetching latest release (OpenGL)";
	if (!release && d3d9)  s->buildName = "&eFetching latest dev build (Direct3D9)";
	if (!release && !d3d9) s->buildName = "&eFetching latest dev build (OpenGL)";

	s->buildProgress = -1;
	String_InitArray(str, strBuffer);

	String_Format1(&str, "%c..", s->buildName);
	LLabel_SetText(&s->lblStatus, &str);
	LWidget_Redraw(&s->lblStatus);
}

static void UpdatesScreen_CheckTick(struct UpdatesScreen* s) {
	if (!CheckUpdateTask.Base.working) return;
	LWebTask_Tick(&CheckUpdateTask.Base);
	if (!CheckUpdateTask.Base.completed) return;
	UpdatesScreen_FormatBoth(s);
}

static void UpdatesScreen_UpdateProgress(struct UpdatesScreen* s, struct LWebTask* task) {
	String str; char strBuffer[STRING_SIZE];
	String identifier;
	struct HttpRequest item;
	int progress;
	if (!Http_GetCurrent(&item, &progress)) return;

	identifier = String_FromRawArray(item.id);
	if (!String_Equals(&identifier, &task->identifier)) return;
	if (progress == s->buildProgress) return;

	s->buildProgress = progress;
	if (progress < 0 || progress > 100) return;
	String_InitArray(str, strBuffer);

	String_Format2(&str, "%c &a%i%%", s->buildName, &s->buildProgress);
	LLabel_SetText(&s->lblStatus, &str);
	LWidget_Redraw(&s->lblStatus);
}

static void UpdatesScreen_FetchTick(struct UpdatesScreen* s) {
	String str; char strBuffer[STRING_SIZE];
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
		/* WebTask handles saving of ClassiCube.update for us */
		Launcher_ShouldExit   = true;
		Launcher_ShouldUpdate = true;
	}
}

static void UpdatesScreen_RelD3D9(void* w, int x, int y)   { UpdatesScreen_Get(true,  true);  }
static void UpdatesScreen_RelOpenGL(void* w, int x, int y) { UpdatesScreen_Get(true,  false); }
static void UpdatesScreen_DevD3D9(void* w, int x, int y)   { UpdatesScreen_Get(false, true);  }
static void UpdatesScreen_DevOpenGL(void* w, int x, int y) { UpdatesScreen_Get(false, false); }

static void UpdatesScreen_Update(struct UpdatesScreen* s) {
	TimeMS buildTime;
	cc_result res;

	/* Initially fill out with update check result from main menu */
	if (CheckUpdateTask.Base.completed && CheckUpdateTask.Base.success) {
		UpdatesScreen_FormatBoth(s);
	}
	CheckUpdateTask_Run();

	res = Updater_GetBuildTime(&buildTime);
	if (res) { Logger_Warn(res, "getting build time"); return; }
	UpdatesScreen_Format(&s->lblYour, "Your build: ", buildTime);
}

static void UpdatesScreen_Init(struct LScreen* s_) {
	struct UpdatesScreen* s = (struct UpdatesScreen*)s_;
	s->seps[0].col = Launcher_ButtonBorderCol;
	s->seps[1].col = Launcher_ButtonBorderCol;
	if (s->numWidgets) { UpdatesScreen_Update(s); return; }

	s->widgets = s->_widgets;
	LLabel_Init(s_,  &s->lblYour, "Your build: (unknown)");
	LLine_Init(s_,   &s->seps[0],   320);
	LLine_Init(s_,   &s->seps[1],   320);

	LLabel_Init(s_,  &s->lblRel, "Latest release: Checking..");	
	LLabel_Init(s_,  &s->lblDev, "Latest dev build: Checking..");
	LLabel_Init(s_,  &s->lblStatus, "");
	LButton_Init(s_, &s->btnBack, 80, 35, "Back");

	if (Updater_D3D9) {
		LButton_Init(s_, &s->btnRel[0], 130, 35, "Direct3D 9");
		LButton_Init(s_, &s->btnDev[0], 130, 35, "Direct3D 9");
		LLabel_Init(s_,  &s->lblInfo, "&eDirect3D 9 is recommended");
	}

	if (Updater_OGL) {
		LButton_Init(s_, &s->btnRel[1], 130, 35, "OpenGL");
		LButton_Init(s_, &s->btnDev[1], 130, 35, "OpenGL");
	}

	s->btnRel[0].OnClick = UpdatesScreen_RelD3D9;
	s->btnRel[1].OnClick = UpdatesScreen_RelOpenGL;
	s->btnDev[0].OnClick = UpdatesScreen_DevD3D9;
	s->btnDev[1].OnClick = UpdatesScreen_DevOpenGL;
	s->btnBack.OnClick   = SwitchToSettings;
	UpdatesScreen_Update(s);
}

static void UpdatesScreen_Layout(struct LScreen* s_) {
	struct UpdatesScreen* s = (struct UpdatesScreen*)s_;
	LWidget_SetLocation(&s->lblYour, ANCHOR_CENTRE, ANCHOR_CENTRE, -5, -120);
	LWidget_SetLocation(&s->seps[0], ANCHOR_CENTRE, ANCHOR_CENTRE,  0, -100);
	LWidget_SetLocation(&s->seps[1], ANCHOR_CENTRE, ANCHOR_CENTRE,  0,   -5);

	LWidget_SetLocation(&s->lblRel,    ANCHOR_CENTRE, ANCHOR_CENTRE, -20, -75);
	LWidget_SetLocation(&s->btnRel[0], ANCHOR_CENTRE, ANCHOR_CENTRE, -80, -40);

	if (Updater_D3D9) {
		LWidget_SetLocation(&s->btnRel[1], ANCHOR_CENTRE, ANCHOR_CENTRE, 80, -40);
	} else {
		LWidget_SetLocation(&s->btnRel[1], ANCHOR_CENTRE, ANCHOR_CENTRE,  0, -40);
	}

	LWidget_SetLocation(&s->lblDev,    ANCHOR_CENTRE, ANCHOR_CENTRE, -30, 20);
	LWidget_SetLocation(&s->btnDev[0], ANCHOR_CENTRE, ANCHOR_CENTRE, -80, 55);

	if (Updater_D3D9) {
		LWidget_SetLocation(&s->btnDev[1], ANCHOR_CENTRE, ANCHOR_CENTRE, 80, 55);
	} else {
		LWidget_SetLocation(&s->btnDev[1], ANCHOR_CENTRE, ANCHOR_CENTRE,  0, 55);
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
	s->buildName     = NULL;
	s->buildProgress = -1;

	FetchUpdateTask.Base.working = false;
	s->lblStatus.text.length     = 0;
}

struct LScreen* UpdatesScreen_MakeInstance(void) {
	struct UpdatesScreen* s = &UpdatesScreen_Instance;
	LScreen_Reset((struct LScreen*)s);
	s->Init   = UpdatesScreen_Init;
	s->Tick   = UpdatesScreen_Tick;
	s->Free   = UpdatesScreen_Free;
	s->Layout = UpdatesScreen_Layout;
	return (struct LScreen*)s;
}
#endif
