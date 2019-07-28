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
		if (w->Hidden) continue;

		if (Gui_Contains(w->X, w->Y, w->Width, w->Height, x, y)) return w;
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

CC_NOINLINE static void LScreen_SelectWidget(struct LScreen* s, struct LWidget* w, bool was) {
	if (!w) return;
	w->Selected       = true;
	s->selectedWidget = w;
	if (w->VTABLE->OnSelect) w->VTABLE->OnSelect(w, was);
}

CC_NOINLINE static void LScreen_UnselectWidget(struct LScreen* s, struct LWidget* w) {
	if (!w) return;
	w->Selected       = false;
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
		if (w->Hidden || !w->TabSelectable) continue;

		LScreen_UnselectWidget(s, s->selectedWidget);
		LScreen_SelectWidget(s, w, false);
		return;
	}
}

static void LScreen_KeyDown(struct LScreen* s, Key key, bool was) {
	if (key == KEY_TAB) {
		LScreen_HandleTab(s);
	} else if (key == KEY_ENTER) {
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

static void LScreen_MouseDown(struct LScreen* s, MouseButton btn) {
	struct LWidget* over = LScreen_WidgetAt(s, Mouse_X, Mouse_Y);
	struct LWidget* prev = s->selectedWidget;

	if (prev && over != prev) LScreen_UnselectWidget(s, prev);
	if (over) LScreen_SelectWidget(s, over, over == prev);
}

static void LScreen_MouseUp(struct LScreen* s, MouseButton btn) {
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
	bool overSame = prev == over;

	if (prev && !overSame) {
		prev->Hovered    = false;
		s->hoveredWidget = NULL;
		s->UnhoverWidget(s, prev);

		if (prev->VTABLE->MouseLeft) prev->VTABLE->MouseLeft(prev);
	}

	if (over) {
		over->Hovered    = true;
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
	s->OnDisplay  = LScreen_NullFunc;
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
		s->widgets[i]->Hovered  = false;
		s->widgets[i]->Selected = false;
	}

	s->onEnterWidget  = NULL;
	s->hoveredWidget  = NULL;
	s->selectedWidget = NULL;
}


/*########################################################################################################################*
*--------------------------------------------------------Widget helpers---------------------------------------------------*
*#########################################################################################################################*/
CC_NOINLINE static void LScreen_Button(struct LScreen* s, struct LButton* w, int width, int height, const char* text) {
	String str = String_FromReadonly(text);
	LButton_Init(w, &Launcher_TitleFont, width, height);
	LButton_SetText(w, &str);
	s->widgets[s->numWidgets++] = (struct LWidget*)w;
}

CC_NOINLINE static void LScreen_Label(struct LScreen* s, struct LLabel* w, const char* text) {
	String str = String_FromReadonly(text);
	LLabel_Init(w, &Launcher_TextFont);
	LLabel_SetText(w, &str);
	s->widgets[s->numWidgets++] = (struct LWidget*)w;
}

CC_NOINLINE static void LScreen_Input(struct LScreen* s, struct LInput* w, int width, bool password, const char* hintText) {
	LInput_Init(w, &Launcher_TextFont, width, 30, 
				hintText, &Launcher_HintFont);
	w->Password = password;
	s->widgets[s->numWidgets++] = (struct LWidget*)w;
}

CC_NOINLINE static void LScreen_Box(struct LScreen* s, struct LBox* w, int width, int height) {
	LBox_Init(w, width, height);
	s->widgets[s->numWidgets++] = (struct LWidget*)w;
}

CC_NOINLINE static void LScreen_Slider(struct LScreen* s, struct LSlider* w, int width, int height,
									  int initValue, int maxValue, BitmapCol progressCol) {
	LSlider_Init(w, width, height);
	w->Value = initValue; w->MaxValue = maxValue;
	w->ProgressCol = progressCol;
	s->widgets[s->numWidgets++] = (struct LWidget*)w;
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
	struct LBox seps[2];
	struct LButton btnEnhanced, btnClassicHax, btnClassic, btnBack;
	struct LLabel  lblTitle, lblHelp, lblEnhanced[2], lblClassicHax[2], lblClassic[2];
	bool firstTime;
	struct LWidget* _widgets[14];
} ChooseModeScreen_Instance;

CC_NOINLINE static void ChooseMode_Click(bool classic, bool classicHacks) {
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
	static const String titleText = String_FromConst("Choose game mode");
	struct ChooseModeScreen* s = (struct ChooseModeScreen*)s_;	

	s->lblHelp.Hidden = !s->firstTime;
	s->btnBack.Hidden = s->firstTime;
	s->seps[0].Col    = Launcher_ButtonBorderCol;
	s->seps[1].Col    = Launcher_ButtonBorderCol;

	if (s->numWidgets) return;
	s->widgets = s->_widgets;
	LScreen_Label(s_, &s->lblTitle, "");
	LScreen_Box(s_,   &s->seps[0], 490, 2);
	LScreen_Box(s_,   &s->seps[1], 490, 2);

	LScreen_Button(s_, &s->btnEnhanced, 145, 35, "Enhanced");
	LScreen_Label(s_,  &s->lblEnhanced[0], "&eEnables custom blocks, changing env");
	LScreen_Label(s_,  &s->lblEnhanced[1], "&esettings, longer messages, and more");

	LScreen_Button(s_, &s->btnClassicHax, 145, 35, "Classic +hax");
	LScreen_Label(s_,  &s->lblClassicHax[0], "&eSame as Classic mode, except that");
	LScreen_Label(s_,  &s->lblClassicHax[1], "&ehacks (noclip/fly/speed) are enabled");

	LScreen_Button(s_, &s->btnClassic, 145, 35, "Classic");
	LScreen_Label(s_,  &s->lblClassic[0], "&eOnly uses blocks and features from");
	LScreen_Label(s_,  &s->lblClassic[1], "&ethe original minecraft classic");

	LScreen_Label(s_,  &s->lblHelp, "&eClick &fEnhanced &eif you're not sure which mode to choose.");
	LScreen_Button(s_, &s->btnBack, 80, 35, "Back");

	s->btnEnhanced.OnClick   = UseModeEnhanced;
	s->btnClassicHax.OnClick = UseModeClassicHax;
	s->btnClassic.OnClick    = UseModeClassic;
	s->btnBack.OnClick       = SwitchToSettings;

	s->lblTitle.Font = Launcher_TitleFont;
	LLabel_SetText(&s->lblTitle, &titleText);
}

static void ChooseModeScreen_Reposition(struct LScreen* s_) {
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

struct LScreen* ChooseModeScreen_MakeInstance(bool firstTime) {
	struct ChooseModeScreen* s = &ChooseModeScreen_Instance;
	LScreen_Reset((struct LScreen*)s);
	s->Init       = ChooseModeScreen_Init;
	s->Reposition = ChooseModeScreen_Reposition;
	s->firstTime  = firstTime;
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
	String_AppendInt(&tmp, col.R);
	LInput_SetText(&s->iptColours[i + 0], &tmp);

	tmp.length = 0;
	String_AppendInt(&tmp, col.G);
	LInput_SetText(&s->iptColours[i + 1], &tmp);

	tmp.length = 0;
	String_AppendInt(&tmp, col.B);
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
	uint8_t r, g, b;

	if (index < 3)       col = &Launcher_BackgroundCol;
	else if (index < 6)  col = &Launcher_ButtonBorderCol;
	else if (index < 9)  col = &Launcher_ButtonHighlightCol;
	else if (index < 12) col = &Launcher_ButtonForeCol;
	else                 col = &Launcher_ButtonForeActiveCol;

	/* if index of G input, changes to index of R input */
	index = (index / 3) * 3;
	if (!Convert_ParseUInt8(&s->iptColours[index + 0].Text, &r)) return;
	if (!Convert_ParseUInt8(&s->iptColours[index + 1].Text, &g)) return;
	if (!Convert_ParseUInt8(&s->iptColours[index + 2].Text, &b)) return;

	col->R = r; col->G = g; col->B = b;
	Launcher_SaveSkin();
	Launcher_Redraw();
}

static void ColoursScreen_AdjustSelected(struct LScreen* s, int delta) {
	struct LInput* w;
	int index, newCol;
	uint8_t col;

	if (!s->selectedWidget) return;
	index = LScreen_IndexOf(s, s->selectedWidget);
	if (index >= 15) return;

	w = (struct LInput*)s->selectedWidget;
	if (!Convert_ParseUInt8(&w->Text, &col)) return;
	newCol = col + delta;

	Math_Clamp(newCol, 0, 255);
	w->Text.length = 0;
	String_AppendInt(&w->Text, newCol);

	if (w->CaretPos >= w->Text.length) w->CaretPos = -1;
	ColoursScreen_TextChanged(w);
}

static void ColoursScreen_MouseWheel(struct LScreen* s_, float delta) {
	struct ColoursScreen* s = (struct ColoursScreen*)s_;
	int steps = Utils_AccumulateWheelDelta(&s->colourAcc, delta);
	ColoursScreen_AdjustSelected(s_, steps);
}

static void ColoursScreen_KeyDown(struct LScreen* s, Key key, bool was) {
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

static void ColoursScreen_Init(struct LScreen* s_) {
	struct ColoursScreen* s = (struct ColoursScreen*)s_;
	int i;

	s->colourAcc = 0;
	if (s->numWidgets) return;
	s->widgets = s->_widgets;
	
	for (i = 0; i < 5 * 3; i++) {
		LScreen_Input(s_, &s->iptColours[i], 55, false, NULL);
		s->iptColours[i].TextChanged = ColoursScreen_TextChanged;
	}

	LScreen_Label(s_, &s->lblNames[0], "Background");
	LScreen_Label(s_, &s->lblNames[1], "Button border");
	LScreen_Label(s_, &s->lblNames[2], "Button highlight");
	LScreen_Label(s_, &s->lblNames[3], "Button");
	LScreen_Label(s_, &s->lblNames[4], "Active button");

	LScreen_Label(s_, &s->lblRGB[0], "Red");
	LScreen_Label(s_, &s->lblRGB[1], "Green");
	LScreen_Label(s_, &s->lblRGB[2], "Blue");

	LScreen_Button(s_, &s->btnDefault, 160, 35, "Default colours");
	LScreen_Button(s_, &s->btnBack,    80,  35, "Back");

	s->btnDefault.OnClick = ColoursScreen_ResetAll;
	s->btnBack.OnClick    = SwitchToSettings;
	ColoursScreen_UpdateAll(s);
}

static void ColoursScreen_Reposition(struct LScreen* s_) {
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
	s->Reposition = ColoursScreen_Reposition;
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

CC_NOINLINE static void DirectConnectScreen_SetStatus(const char* msg) {
	String str = String_FromReadonly(msg);
	struct LLabel* w = &DirectConnectScreen_Instance.lblStatus;

	LLabel_SetText(w, &str);
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
	const String* user   = &DirectConnectScreen_Instance.iptUsername.Text;
	const String* addr   = &DirectConnectScreen_Instance.iptAddress.Text;
	const String* mppass = &DirectConnectScreen_Instance.iptMppass.Text;

	String ip, port;
	uint8_t  raw_ip[4];
	uint16_t raw_port;

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

	LScreen_Input(s_, &s->iptUsername, 330, false, "&gUsername..");
	LScreen_Input(s_, &s->iptAddress,  330, false, "&gIP address:Port number..");
	LScreen_Input(s_, &s->iptMppass,   330, false, "&gMppass..");

	LScreen_Button(s_, &s->btnConnect, 110, 35, "Connect");
	LScreen_Button(s_, &s->btnBack,     80, 35, "Back");
	LScreen_Label(s_,  &s->lblStatus, "");

	s->btnConnect.OnClick = DirectConnectScreen_StartClient;
	s->btnBack.OnClick    = SwitchToMain;
	/* Init input text from options */
	DirectConnectScreen_Load(s);
}

static void DirectConnectScreen_Reposition(struct LScreen* s_) {
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
	s->Reposition    = DirectConnectScreen_Reposition;
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
	bool signingIn;
} MainScreen_Instance;

struct ResumeInfo {
	String user, ip, port, server, mppass;
	char _userBuffer[STRING_SIZE], _serverBuffer[STRING_SIZE];
	char _ipBuffer[16], _portBuffer[16], _mppassBuffer[STRING_SIZE];
	bool valid;
};

CC_NOINLINE static void MainScreen_GetResume(struct ResumeInfo* info, bool full) {
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
	static const String needUser = String_FromConst("&eUsername required");
	static const String needPass = String_FromConst("&ePassword required");
	static const String signIn   = String_FromConst("&e&eSigning in..");

	struct MainScreen* s = &MainScreen_Instance;
	String* user = &s->iptUsername.Text;
	String* pass = &s->iptPassword.Text;

	if (!user->length) {
		LLabel_SetText(&s->lblStatus, &needUser);
		LWidget_Redraw(&s->lblStatus); return;
	}
	if (!pass->length) {
		LLabel_SetText(&s->lblStatus, &needPass);
		LWidget_Redraw(&s->lblStatus); return;
	}

	if (GetTokenTask.Base.Working) return;
	Options_Set("launcher-cc-username", user);
	Options_SetSecure("launcher-cc-password", pass, user);

	GetTokenTask_Run();
	LLabel_SetText(&s->lblStatus, &signIn);
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
	const String* user = &MainScreen_Instance.iptUsername.Text;

	if (!user->length) user = &defUser;
	Launcher_StartGame(user, &String_Empty, &String_Empty, &String_Empty, &String_Empty);
}

static void MainScreen_Init(struct LScreen* s_) {
	String user, pass; char passBuffer[STRING_SIZE];
	static const String update = String_FromConst("&eChecking..");
	struct MainScreen* s = (struct MainScreen*)s_;

	/* status should reset after user has gone to another menu */
	s->lblStatus.Text.length = 0;
	if (s->numWidgets) return;
	s->widgets = s->_widgets;

	LScreen_Input(s_, &s->iptUsername, 280, false, "&gUsername..");
	LScreen_Input(s_, &s->iptPassword, 280, true,  "&gPassword..");

	LScreen_Button(s_, &s->btnLogin, 100, 35, "Sign in");
	LScreen_Label(s_,  &s->lblStatus, "");

	LScreen_Button(s_, &s->btnResume,  100, 35, "Resume");
	LScreen_Button(s_, &s->btnDirect,  200, 35, "Direct connect");
	LScreen_Button(s_, &s->btnSPlayer, 200, 35, "Singleplayer");

	LScreen_Label(s_,  &s->lblUpdate, "");
	LScreen_Button(s_, &s->btnOptions,  100, 35, "Options");
	LScreen_Button(s_, &s->btnRegister, 100, 35, "Register");
	
	s->btnLogin.OnClick    = MainScreen_Login;
	s->btnResume.OnClick   = MainScreen_Resume;
	s->btnDirect.OnClick   = SwitchToDirectConnect;
	s->btnSPlayer.OnClick  = MainScreen_Singleplayer;
	s->btnOptions.OnClick  = SwitchToSettings;
	s->btnRegister.OnClick = MainScreen_Register;

	/* need to set text here for right size */
	s->lblUpdate.Font = Launcher_HintFont;
	LLabel_SetText(&s->lblUpdate, &update);
	
	String_InitArray(pass, passBuffer);
	Options_UNSAFE_Get("launcher-cc-username", &user);
	Options_GetSecure("launcher-cc-password", &pass, &user);

	LInput_SetText(&s->iptUsername, &user);
	LInput_SetText(&s->iptPassword, &pass);
}

static void MainScreen_Reposition(struct LScreen* s_) {
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

	if (info.server.length && String_Equals(&info.user,	&s->iptUsername.Text)) {
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

	LLabel_SetText(&s->lblStatus, &String_Empty);
	LWidget_Redraw(&s->lblStatus);
}

CC_NOINLINE static uint32_t MainScreen_GetVersion(const String* version) {
	uint8_t raw[4] = { 0, 0, 0, 0 };
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
	static const String needUpdate = String_FromConst("&aNew release");
	static const String upToDate   = String_FromConst("&eUp to date");
	static const String failed     = String_FromConst("&cCheck failed");
	static const String currentStr = String_FromConst(GAME_APP_VER);
	uint32_t latest, current;

	if (!CheckUpdateTask.Base.Working)   return;
	LWebTask_Tick(&CheckUpdateTask.Base);
	if (!CheckUpdateTask.Base.Completed) return;

	if (CheckUpdateTask.Base.Success) {
		latest  = MainScreen_GetVersion(&CheckUpdateTask.LatestRelease);
		current = MainScreen_GetVersion(&currentStr);
		LLabel_SetText(&s->lblUpdate, latest > current ? &needUpdate : &upToDate);
	} else {
		LLabel_SetText(&s->lblUpdate, &failed);
	}
	LWidget_Redraw(&s->lblUpdate);
}

static void MainScreen_TickGetToken(struct MainScreen* s) {
	if (!GetTokenTask.Base.Working)   return;
	LWebTask_Tick(&GetTokenTask.Base);
	if (!GetTokenTask.Base.Completed) return;

	if (GetTokenTask.Base.Success) {
		SignInTask_Run(&s->iptUsername.Text, &s->iptPassword.Text);
	} else {
		MainScreen_Error(&GetTokenTask.Base, "signing in");
	}
}

static void MainScreen_TickSignIn(struct MainScreen* s) {
	static const String fetchMsg = String_FromConst("&eRetrieving servers list..");
	if (!SignInTask.Base.Working)   return;
	LWebTask_Tick(&SignInTask.Base);
	if (!SignInTask.Base.Completed) return;

	if (SignInTask.Error.length) {
		LLabel_SetText(&s->lblStatus, &SignInTask.Error);
		LWidget_Redraw(&s->lblStatus);
	} else if (SignInTask.Base.Success) {
		/* website returns case correct username */
		if (!String_Equals(&s->iptUsername.Text, &SignInTask.Username)) {
			LInput_SetText(&s->iptUsername, &SignInTask.Username);
			LWidget_Redraw(&s->iptUsername);
		}

		FetchServersTask_Run();
		LLabel_SetText(&s->lblStatus, &fetchMsg);
		LWidget_Redraw(&s->lblStatus);
	} else {
		MainScreen_Error(&SignInTask.Base, "signing in");
	}
}

static void MainScreen_TickFetchServers(struct MainScreen* s) {
	if (!FetchServersTask.Base.Working)   return;
	LWebTask_Tick(&FetchServersTask.Base);
	if (!FetchServersTask.Base.Completed) return;

	if (FetchServersTask.Base.Success) {
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
	s->Init       = MainScreen_Init;
	s->Tick       = MainScreen_Tick;
	s->Reposition = MainScreen_Reposition;
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

	s->btnYes.Hidden   = true;
	s->btnNo.Hidden    = true;
	s->lblLine1.Hidden = true;
	s->lblLine2.Hidden = true;
	s->btnCancel.Hidden   = false;
	s->sdrProgress.Hidden = false;
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
	BitmapCol progressCol = BITMAPCOL_CONST(0, 220, 0, 255);
	struct ResourcesScreen* s = (struct ResourcesScreen*)s_;
	float size;

	s->statusYOffset = 10;
	if (s->numWidgets) return;
	s->widgets = s->_widgets;

	LScreen_Label(s_, &s->lblLine1, "Some required resources weren't found");
	LScreen_Label(s_, &s->lblLine2, "Okay to download?");
	LScreen_Label(s_, &s->lblStatus, "");
	
	LScreen_Button(s_, &s->btnYes, 70, 35, "Yes");
	LScreen_Button(s_, &s->btnNo,  70, 35, "No");

	LScreen_Button(s_, &s->btnCancel, 120, 35, "Cancel");
	LScreen_Slider(s_, &s->sdrProgress, 200, 12, 0, 100, progressCol);

	s->btnCancel.Hidden   = true;
	s->sdrProgress.Hidden = true;

	/* TODO: Size 13 italic font?? does it matter?? */
	String_InitArray(str, buffer);
	size = Resources_Size / 1024.0f;

	s->lblStatus.Font = Launcher_HintFont;
	String_Format1(&str, "&eDownload size: %f2 megabytes", &size);
	LLabel_SetText(&s->lblStatus, &str);

	s->btnYes.OnClick    = ResourcesScreen_Download;
	s->btnNo.OnClick     = ResourcesScreen_Next;
	s->btnCancel.OnClick = ResourcesScreen_Next;
}

static void ResourcesScreen_Reposition(struct LScreen* s_) {
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
	BitmapCol boxCol = BITMAPCOL_CONST(120, 85, 151, 255);
	Gradient_Noise(&Launcher_Framebuffer, boxCol, 4, x, y, width, height);
	Launcher_MarkDirty(x, y, width, height);
}

#define RESOURCES_XSIZE 190
#define RESOURCES_YSIZE 70
static void ResourcesScreen_Draw(struct LScreen* s) {
	BitmapCol backCol = BITMAPCOL_CONST( 12, 12,  12, 255);

	Drawer2D_Clear(&Launcher_Framebuffer, backCol, 
					0, 0, Window_Width, Window_Height);
	ResourcesScreen_ResetArea(
		Window_Width / 2 - RESOURCES_XSIZE, Window_Height / 2 - RESOURCES_YSIZE,
		RESOURCES_XSIZE * 2,                RESOURCES_YSIZE * 2);
	LScreen_Draw(s);
}

static void ResourcesScreen_SetStatus(const String* str) {
	struct LLabel* w = &ResourcesScreen_Instance.lblStatus;
	ResourcesScreen_ResetArea(w->Last.X, w->Last.Y, 
							  w->Last.Width, w->Last.Height);
	LLabel_SetText(w, str);

	w->YOffset = -10;
	ResourcesScreen_Instance.statusYOffset = w->YOffset;	
	LWidget_CalcPosition(w);
	LWidget_Draw(w);
}

static void ResourcesScreen_UpdateStatus(struct HttpRequest* req) {
	String str; char strBuffer[STRING_SIZE];
	String id;
	struct LLabel* w = &ResourcesScreen_Instance.lblStatus;
	int count;

	id = String_FromRawArray(req->ID);
	String_InitArray(str, strBuffer);
	count = Fetcher_Downloaded + 1;
	String_Format3(&str, "&eFetching %s.. (%i/%i)", &id, &count, &Resources_Count);

	if (String_Equals(&str, &w->Text)) return;
	ResourcesScreen_SetStatus(&str);
}

static void ResourcesScreen_UpdateProgress(struct ResourcesScreen* s) {
	struct HttpRequest req;
	int progress;

	if (!Http_GetCurrent(&req, &progress)) return;
	ResourcesScreen_UpdateStatus(&req);
	/* making request still, haven't started download yet */
	if (progress < 0 || progress > 100) return;

	if (progress == s->sdrProgress.Value) return;
	s->sdrProgress.Value = progress;
	s->sdrProgress.Hidden = false;
	s->sdrProgress.VTABLE->Draw(&s->sdrProgress);
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
	s->Init       = ResourcesScreen_Init;
	s->Draw       = ResourcesScreen_Draw;
	s->Tick       = ResourcesScreen_Tick;
	s->Reposition = ResourcesScreen_Reposition;
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
	FontDesc rowFont;
	float tableAcc;
} ServersScreen_Instance;

static void ServersScreen_Connect(void* w, int x, int y) {
	struct LTable* table = &ServersScreen_Instance.table;
	String* hash         = &ServersScreen_Instance.iptHash.Text;

	if (!hash->length && table->RowsCount) { 
		hash = &LTable_Get(table->TopRow)->hash; 
	}
	Launcher_ConnectToServer(hash);
}

static void ServersScreen_Refresh(void* w, int x, int y) {
	static const String working = String_FromConst("&eWorking..");
	struct LButton* btn;
	if (FetchServersTask.Base.Working) return;

	FetchServersTask_Run();
	btn = &ServersScreen_Instance.btnRefresh;
	LButton_SetText(btn, &working);
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

	for (i = 0; i < FetchServersTask.NumServers; i++) {
		FetchFlagsTask_Add(&FetchServersTask.Servers[i]);
	}
}

static void ServersScreen_InitWidgets(struct LScreen* s_) {
	struct ServersScreen* s = (struct ServersScreen*)s_;
	s->widgets = s->_widgets;

	LScreen_Input(s_, &s->iptSearch, 370, false, "&gSearch servers..");
	LScreen_Input(s_, &s->iptHash,   475, false, "&gclassicube.net/server/play/...");

	LScreen_Button(s_, &s->btnBack,    110, 30, "Back");
	LScreen_Button(s_, &s->btnConnect, 130, 30, "Connect");
	LScreen_Button(s_, &s->btnRefresh, 110, 30, "Refresh");

	s->btnBack.OnClick    = SwitchToMain;
	s->btnConnect.OnClick = ServersScreen_Connect;
	s->btnRefresh.OnClick = ServersScreen_Refresh;

	s->iptSearch.TextChanged   = ServersScreen_SearchChanged;
	s->iptHash.TextChanged     = ServersScreen_HashChanged;
	s->iptHash.ClipboardFilter = ServersScreen_HashFilter;

	LTable_Init(&s->table, &Launcher_TextFont, &s->rowFont);
	s->table.Filter       = &s->iptSearch.Text;
	s->table.SelectedHash = &s->iptHash.Text;
	s->table.OnSelectedChanged = ServersScreen_OnSelectedChanged;

	s->widgets[s->numWidgets++] = (struct LWidget*)&s->table;
}

static void ServersScreen_Init(struct LScreen* s_) {
	struct ServersScreen* s = (struct ServersScreen*)s_;
	Drawer2D_MakeFont(&s->rowFont, 11, FONT_STYLE_NORMAL);

	if (!s->numWidgets) ServersScreen_InitWidgets(s_);
	s->table.RowFont = s->rowFont;
	/* also resets hash and filter */
	LTable_Reset(&s->table);

	ServersScreen_ReloadServers(s);
	LScreen_SelectWidget(s_, (struct LWidget*)&s->iptSearch, false);
}

static void ServersScreen_Tick(struct LScreen* s_) {
	static const String refresh = String_FromConst("Refresh");
	static const String failed  = String_FromConst("&cFailed");
	struct ServersScreen* s = (struct ServersScreen*)s_;
	int count;
	LScreen_Tick(s_);

	count = FetchFlagsTask.Count;
	LWebTask_Tick(&FetchFlagsTask.Base);
	/* TODO: Only redraw flags */
	if (count != FetchFlagsTask.Count) LWidget_Draw(&s->table);

	if (!FetchServersTask.Base.Working) return;
	LWebTask_Tick(&FetchServersTask.Base);
	if (!FetchServersTask.Base.Completed) return;

	if (FetchServersTask.Base.Success) {
		ServersScreen_ReloadServers(s);
		LWidget_Draw(&s->table);
	}

	LButton_SetText(&s->btnRefresh, 
				FetchServersTask.Base.Success ? &refresh : &failed);
	LWidget_Redraw(&s->btnRefresh);
}

static void ServersScreen_Free(struct LScreen* s_) {
	struct ServersScreen* s = (struct ServersScreen*)s_;
	Font_Free(&s->rowFont);
}

static void ServersScreen_Reposition(struct LScreen* s_) {
	struct ServersScreen* s = (struct ServersScreen*)s_;
	LWidget_SetLocation(&s->iptSearch, ANCHOR_MIN, ANCHOR_MIN, 10, 10);
	LWidget_SetLocation(&s->iptHash,   ANCHOR_MIN, ANCHOR_MAX, 10, 10);

	LWidget_SetLocation(&s->btnBack,    ANCHOR_MAX, ANCHOR_MIN,  10, 10);
	LWidget_SetLocation(&s->btnConnect, ANCHOR_MAX, ANCHOR_MAX,  10, 10);
	LWidget_SetLocation(&s->btnRefresh, ANCHOR_MAX, ANCHOR_MIN, 135, 10);
	
	LWidget_SetLocation(&s->table, ANCHOR_MIN, ANCHOR_MIN, 10, 50);
	s->table.Width  = Window_Width  - s->table.X;
	s->table.Height = Window_Height - s->table.Y * 2;
	s->table.Height = max(1, s->table.Height);

	LTable_Reposition(&s->table);
}

static void ServersScreen_MouseWheel(struct LScreen* s_, float delta) {
	struct ServersScreen* s = (struct ServersScreen*)s_;
	s->table.VTABLE->MouseWheel(&s->table, delta);
}

static void ServersScreen_KeyDown(struct LScreen* s_, Key key, bool was) {
	struct ServersScreen* s = (struct ServersScreen*)s_;
	if (!LTable_HandlesKey(key)) {
		LScreen_KeyDown(s_, key, was);
	} else {
		s->table.VTABLE->KeyDown(&s->table, key, was);
	}
}

static void ServersScreen_MouseUp(struct LScreen* s_, MouseButton btn) {
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
	s->Reposition = ServersScreen_Reposition;
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

	s->btnColours.Hidden = Launcher_ClassicBackground;
	s->lblColours.Hidden = Launcher_ClassicBackground;

	if (s->numWidgets) return;
	s->widgets = s->_widgets;

	LScreen_Button(s_, &s->btnUpdates, 110, 35, "Updates");
	LScreen_Label(s_,  &s->lblUpdates, "&eGet the latest stuff");

	LScreen_Button(s_, &s->btnMode, 110, 35, "Mode");
	LScreen_Label(s_,  &s->lblMode, "&eChange the enabled features");

	LScreen_Button(s_, &s->btnColours, 110, 35, "Colours");
	LScreen_Label(s_,  &s->lblColours, "&eChange how the launcher looks");

	LScreen_Button(s_, &s->btnBack, 80, 35, "Back");

	s->btnMode.OnClick    = SwitchToChooseMode;
	s->btnUpdates.OnClick = SwitchToUpdates;
	s->btnColours.OnClick = SwitchToColours;
	s->btnBack.OnClick    = SwitchToMain;
}

static void SettingsScreen_Reposition(struct LScreen* s_) {
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
	s->Init       = SettingsScreen_Init;
	s->Reposition = SettingsScreen_Reposition;
	return (struct LScreen*)s;
}


/*########################################################################################################################*
*--------------------------------------------------------UpdatesScreen----------------------------------------------------*
*#########################################################################################################################*/
static struct UpdatesScreen {
	LScreen_Layout
	struct LBox seps[2];
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

static void UpdatesScreen_Format(struct LLabel* lbl, const char* prefix, TimeMS time) {
	String str; char buffer[STRING_SIZE];
	TimeMS now;
	int delta;

	String_InitArray(str, buffer);
	String_AppendConst(&str, prefix);

	if (!time) {
		String_AppendConst(&str, "&cCheck failed");
	} else {
		now   = DateTime_CurrentUTC_MS();
		/* must divide as uint64_t, int delta overflows after 26 days */
		delta = (int)((now - time) / 1000);

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
	UpdatesScreen_Format(&s->lblRel, "Latest release: ",   CheckUpdateTask.RelTimestamp);
	UpdatesScreen_Format(&s->lblDev, "Latest dev build: ", CheckUpdateTask.DevTimestamp);
}

static void UpdatesScreen_Get(bool release, bool d3d9) {
	String str; char strBuffer[STRING_SIZE];
	struct UpdatesScreen* s = &UpdatesScreen_Instance;

	TimeMS time = release ? CheckUpdateTask.RelTimestamp : CheckUpdateTask.DevTimestamp;
	if (!time || FetchUpdateTask.Base.Working) return;
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
	if (!CheckUpdateTask.Base.Working) return;
	LWebTask_Tick(&CheckUpdateTask.Base);
	if (!CheckUpdateTask.Base.Completed) return;
	UpdatesScreen_FormatBoth(s);
}

static void UpdatesScreen_UpdateProgress(struct UpdatesScreen* s, struct LWebTask* task) {
	String str; char strBuffer[STRING_SIZE];
	String identifier;
	struct HttpRequest item;
	int progress;
	if (!Http_GetCurrent(&item, &progress)) return;

	identifier = String_FromRawArray(item.ID);
	if (!String_Equals(&identifier, &task->Identifier)) return;
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
	if (!FetchUpdateTask.Base.Working) return;

	LWebTask_Tick(&FetchUpdateTask.Base);
	UpdatesScreen_UpdateProgress(s, &FetchUpdateTask.Base);
	if (!FetchUpdateTask.Base.Completed) return;

	if (!FetchUpdateTask.Base.Success) {
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

static void UpdatesScreen_Init(struct LScreen* s_) {
	String path; char pathBuffer[FILENAME_SIZE];
	struct UpdatesScreen* s = (struct UpdatesScreen*)s_;
	TimeMS buildTime;
	ReturnCode res;

	s->seps[0].Col = Launcher_ButtonBorderCol;
	s->seps[1].Col = Launcher_ButtonBorderCol;
	if (s->numWidgets) { CheckUpdateTask_Run(); return; }

	s->widgets = s->_widgets;
	LScreen_Label(s_,  &s->lblYour, "Your build: (unknown)");
	LScreen_Box(s_,    &s->seps[0],   320, 2);
	LScreen_Box(s_,    &s->seps[1],   320, 2);

	LScreen_Label(s_,  &s->lblRel, "Latest release: Checking..");
	LScreen_Button(s_, &s->btnRel[0], 130, 35, "Direct3D 9");
	LScreen_Button(s_, &s->btnRel[1], 130, 35, "OpenGL");

	LScreen_Label(s_,  &s->lblDev, "Latest dev build: Checking..");
	LScreen_Button(s_, &s->btnDev[0], 130, 35, "Direct3D 9");
	LScreen_Button(s_, &s->btnDev[1], 130, 35, "OpenGL");

	LScreen_Label(s_,  &s->lblInfo, "&eDirect3D 9 is recommended for Windows");
	LScreen_Label(s_,  &s->lblStatus, "");
	LScreen_Button(s_, &s->btnBack, 80, 35, "Back");

	s->btnRel[0].OnClick = UpdatesScreen_RelD3D9;
	s->btnRel[1].OnClick = UpdatesScreen_RelOpenGL;
	s->btnDev[0].OnClick = UpdatesScreen_DevD3D9;
	s->btnDev[1].OnClick = UpdatesScreen_DevOpenGL;
	s->btnBack.OnClick   = SwitchToSettings;

	/* Initially fill out with update check result from main menu */
	if (CheckUpdateTask.Base.Completed && CheckUpdateTask.Base.Success) {
		UpdatesScreen_FormatBoth(s);
	}
	CheckUpdateTask_Run();

	String_InitArray(path, pathBuffer);
	res = Process_GetExePath(&path);
	if (res) { Logger_Warn(res, "getting .exe path"); return; }
	
	res = File_GetModifiedTime(&path, &buildTime);
	if (res) { Logger_Warn(res, "getting build time"); return; }

	UpdatesScreen_Format(&s->lblYour, "Your build: ", buildTime);
}

static void UpdatesScreen_Reposition(struct LScreen* s_) {
	struct UpdatesScreen* s = (struct UpdatesScreen*)s_;
	LWidget_SetLocation(&s->lblYour, ANCHOR_CENTRE, ANCHOR_CENTRE, -5, -120);
	LWidget_SetLocation(&s->seps[0], ANCHOR_CENTRE, ANCHOR_CENTRE,  0, -100);
	LWidget_SetLocation(&s->seps[1], ANCHOR_CENTRE, ANCHOR_CENTRE,  0,   -5);

	LWidget_SetLocation(&s->lblRel,    ANCHOR_CENTRE, ANCHOR_CENTRE, -20, -75);
	LWidget_SetLocation(&s->btnRel[0], ANCHOR_CENTRE, ANCHOR_CENTRE, -80, -40);
	LWidget_SetLocation(&s->btnRel[1], ANCHOR_CENTRE, ANCHOR_CENTRE,  80, -40);

	LWidget_SetLocation(&s->lblDev,    ANCHOR_CENTRE, ANCHOR_CENTRE, -30, 20);
	LWidget_SetLocation(&s->btnDev[0], ANCHOR_CENTRE, ANCHOR_CENTRE, -80, 55);
	LWidget_SetLocation(&s->btnDev[1], ANCHOR_CENTRE, ANCHOR_CENTRE,  80, 55);

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

	FetchUpdateTask.Base.Working = false;
	s->lblStatus.Text.length     = 0;
}

struct LScreen* UpdatesScreen_MakeInstance(void) {
	struct UpdatesScreen* s = &UpdatesScreen_Instance;
	LScreen_Reset((struct LScreen*)s);
	s->Init       = UpdatesScreen_Init;
	s->Tick       = UpdatesScreen_Tick;
	s->Free       = UpdatesScreen_Free;
	s->Reposition = UpdatesScreen_Reposition;
	return (struct LScreen*)s;
}
#endif
