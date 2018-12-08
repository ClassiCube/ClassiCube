#include "LScreens.h"
#include "LWidgets.h"
#include "Launcher.h"
#include "Gui.h"
#include "Game.h"
#include "Drawer2D.h"

/*########################################################################################################################*
*---------------------------------------------------------Screen base-----------------------------------------------------*
*#########################################################################################################################*/
static void LScreen_NullFunc(struct LScreen* s) { }
CC_NOINLINE static int LScreen_IndexOf(struct LScreen* s, void* w) {
	int i;
	for (i = 0; i < s->NumWidgets; i++) {
		if (s->Widgets[i] == w) return i;
	}
	return -1;
}

CC_NOINLINE static struct LWidget* LScreen_WidgetAt(struct LScreen* s, int x, int y) {
	struct LWidget* w;
	int i = 0;

	for (i = 0; i < s->NumWidgets; i++) {
		w = s->Widgets[i];
		if (w->Hidden) continue;

		if (Gui_Contains(w->X, w->Y, w->Width, w->Height, x, y)) return w;
	}
	return NULL;
}

static void LScreen_DrawAll(struct LScreen* s) {
	struct LWidget* widget;
	int i;

	for (i = 0; i < s->NumWidgets; i++) {
		widget = s->Widgets[i];
		widget->VTABLE->Redraw(widget);
	}
}

CC_NOINLINE static void LScreen_HandleTab(struct LScreen* s) {
	struct LWidget* w;
	int dir   = Key_IsShiftPressed() ? -1 : 1;
	int index = 0, i, j;

	if (s->SelectedWidget) {
		index = LScreen_IndexOf(s, s->SelectedWidget) + dir;
	}

	for (j = 0; j < s->NumWidgets; j++) {
		i = (index + j * dir) % s->NumWidgets;
		if (i < 0) i += s->NumWidgets;

		w = s->Widgets[i];
		if (w->Hidden || !w->TabSelectable) continue;

		s->UnselectWidget(s, s->SelectedWidget);
		s->SelectWidget(s, w);
		return;
	}
}

static void LScreen_KeyDown(struct LScreen* s, Key key) {
	if (key == KEY_TAB) {
		LScreen_HandleTab(s);
	} else if (key == KEY_ENTER) {
		if (s->SelectedWidget && s->SelectedWidget->OnClick) {
			s->SelectedWidget->OnClick(s->SelectedWidget, Mouse_X, Mouse_Y);
		} else if (s->OnEnterWidget) {
			s->OnEnterWidget->OnClick(s->OnEnterWidget,   Mouse_X, Mouse_Y);
		}
	}
}

static void LScreen_MouseDown(struct LScreen* s, MouseButton btn) {
	struct LWidget* over = LScreen_WidgetAt(s, Mouse_X, Mouse_Y);
	struct LWidget* prev = s->SelectedWidget;

	if (over == prev) return;
	if (prev) s->UnselectWidget(s, prev);
	if (over) s->SelectWidget(s,   over);
}

static void LScreen_MouseUp(struct LScreen* s, MouseButton btn) {
	struct LWidget* over = LScreen_WidgetAt(s, Mouse_X, Mouse_Y);
	struct LWidget* prev = s->SelectedWidget;

	/* if user moves mouse away, it doesn't count */
	if (over != prev) {
		s->UnselectWidget(s, prev);
	} else if (over && over->OnClick) {
		over->OnClick(over, Mouse_X, Mouse_Y);
	}
}

static void LScreen_MouseMove(struct LScreen* s, int deltaX, int deltaY) {
	struct LWidget* over = LScreen_WidgetAt(s, Mouse_X, Mouse_Y);
	struct LWidget* prev = s->HoveredWidget;

	if (over == prev) return;
	if (prev) s->UnhoverWidget(s, prev);
	if (over) s->HoverWidget(s,   over);
}

static void LScreen_HoverWidget(struct LScreen* s, struct LWidget* w) {
	if (!w) return;
	w->Hovered       = true;
	s->HoveredWidget = w;
	if (w->VTABLE->OnHover) w->VTABLE->OnHover(w);
}

static void LScreen_UnhoverWidget(struct LScreen* s, struct LWidget* w) {
	if (!w) return;
	w->Hovered       = false;
	s->HoveredWidget = NULL;
	if (w->VTABLE->OnUnhover) w->VTABLE->OnUnhover(w);
}

static void LScreen_SelectWidget(struct LScreen* s, struct LWidget* w) {
	if (!w) return;
	w->Selected      = true;
	s->SelectedWidget = w;
	if (w->VTABLE->OnSelect) w->VTABLE->OnSelect(w);
}

static void LScreen_UnselectWidget(struct LScreen* s, struct LWidget* w) {
	if (!w) return;
	w->Selected       = false;
	s->SelectedWidget = NULL;
	if (w->VTABLE->OnUnselect) w->VTABLE->OnUnselect(w);
}

CC_NOINLINE static void LScreen_Reset(struct LScreen* s) {
	int i;

	s->Init = NULL; /* screens should always override this */
	s->Free = LScreen_NullFunc;
	s->DrawAll   = LScreen_DrawAll;
	s->Tick      = LScreen_NullFunc;
	s->OnDisplay = LScreen_NullFunc;
	s->KeyDown   = LScreen_KeyDown;
	s->MouseDown = LScreen_MouseDown;
	s->MouseUp   = LScreen_MouseUp;
	s->MouseMove = LScreen_MouseMove;
	s->HoverWidget    = LScreen_HoverWidget;
	s->UnhoverWidget  = LScreen_UnhoverWidget;
	s->SelectWidget   = LScreen_SelectWidget;
	s->UnselectWidget = LScreen_UnselectWidget;

	/* reset all widgets mouse state */
	for (i = 0; i < s->NumWidgets; i++) { 
		s->Widgets[i]->Hovered  = false;
		s->Widgets[i]->Selected = false;
	}

	s->OnEnterWidget  = NULL;
	s->HoveredWidget  = NULL;
	s->SelectedWidget = NULL;
}


/*########################################################################################################################*
*--------------------------------------------------------Widget helpers---------------------------------------------------*
*#########################################################################################################################*/
CC_NOINLINE static void LScreen_Button(struct LScreen* s, struct LButton* w, int width, int height, const char* text) {
	String str = String_FromReadonly(text);
	LButton_Init(w, width, height);
	LButton_SetText(w, &str, &Launcher_TitleFont);
	s->Widgets[s->NumWidgets++] = (struct LWidget*)w;
}

CC_NOINLINE static void LScreen_Label(struct LScreen* s, struct LLabel* w, const char* text) {
	String str = String_FromReadonly(text);
	LLabel_Init(w);
	LLabel_SetText(w, &str, &Launcher_TextFont);
	s->Widgets[s->NumWidgets++] = (struct LWidget*)w;
}

CC_NOINLINE static void LScreen_Input(struct LScreen* s, struct LInput* w, int width, bool password, const char* hintText) {
	LInput_Init(w, width, 30, hintText, &Launcher_HintFont);
	w->Password = password;
	s->Widgets[s->NumWidgets++] = (struct LWidget*)w;
}

CC_NOINLINE static void LScreen_Slider(struct LScreen* s, struct LSlider* w, int width, int height,
									  int initValue, int maxValue, BitmapCol progressCol) {
	LSlider_Init(w, width, height);
	w->Value = initValue; w->MaxValue = maxValue;
	w->ProgressCol = progressCol;
	s->Widgets[s->NumWidgets++] = (struct LWidget*)w;
}

static void SwitchToChooseMode(void* w, int x, int y) {
	Launcher_SetScreen(ChooseModeScreen_MakeInstance(false));
}
static void SwitchToColours(void* w, int x, int y) {
	Launcher_SetScreen(ColoursScreen_MakeInstance());
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
	struct LButton BtnEnhanced, BtnClassicHax, BtnClassic, BtnBack;
	struct LLabel  LblTitle, LblHelp, LblEnhanced[2], LblClassicHax[2], LblClassic[2];
	bool FirstTime;
	struct LWidget* _widgets[12];
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

static void ChooseModeScreen_InitWidgets(struct ChooseModeScreen* s) {
	static String titleText = String_FromConst("Choose game mode");
	struct LScreen* s_ = (struct LScreen*)s;
	s->Widgets = s->_widgets;
	LScreen_Label(s_,  &s->LblTitle, "");

	LScreen_Button(s_, &s->BtnEnhanced, 145, 35, "Enhanced");
	LScreen_Label(s_,  &s->LblEnhanced[0], "&eEnables custom blocks, changing env");
	LScreen_Label(s_,  &s->LblEnhanced[1], "&esettings, longer messages, and more");

	LScreen_Button(s_, &s->BtnClassicHax, 145, 35, "Classic +hax");
	LScreen_Label(s_,  &s->LblClassicHax[0], "&eSame as Classic mode, except that");
	LScreen_Label(s_,  &s->LblClassicHax[1], "&ehacks (noclip/fly/speed) are enabled");

	LScreen_Button(s_, &s->BtnClassic, 145, 35, "Classic");
	LScreen_Label(s_,  &s->LblClassic[0], "&eOnly uses blocks and features from");
	LScreen_Label(s_,  &s->LblClassic[1], "&ethe original minecraft classic");

	LScreen_Label(s_,  &s->LblHelp, "&eClick &fEnhanced &eif you'e not sure which mode to choose.");
	LScreen_Button(s_, &s->BtnBack, 80, 35, "Back");

	s->BtnEnhanced.OnClick   = UseModeEnhanced;
	s->BtnClassicHax.OnClick = UseModeClassicHax;
	s->BtnClassic.OnClick    = UseModeClassic;
	s->BtnBack.OnClick       = SwitchToSettings;

	LLabel_SetText(&s->LblTitle, &titleText, &Launcher_TitleFont);
}

static void ChooseModeScreen_Reposition(struct LScreen* s_) {
	struct ChooseModeScreen* s = (struct ChooseModeScreen*)s_;
	int middle = Game_Width / 2;
	LWidget_SetLocation(&s->LblTitle, ANCHOR_CENTRE, ANCHOR_CENTRE, 10, -135);

	LWidget_SetLocation(&s->BtnEnhanced,    ANCHOR_MIN, ANCHOR_CENTRE, middle - 250, -72);
	LWidget_SetLocation(&s->LblEnhanced[0], ANCHOR_MIN, ANCHOR_CENTRE, middle - 85,  -72 - 12);
	LWidget_SetLocation(&s->LblEnhanced[1], ANCHOR_MIN, ANCHOR_CENTRE, middle - 85,  -72 + 12);

	LWidget_SetLocation(&s->BtnClassicHax,    ANCHOR_MIN, ANCHOR_CENTRE, middle - 250, 0);
	LWidget_SetLocation(&s->LblClassicHax[0], ANCHOR_MIN, ANCHOR_CENTRE, middle - 85,  0 - 12);
	LWidget_SetLocation(&s->LblClassicHax[1], ANCHOR_MIN, ANCHOR_CENTRE, middle - 85,  0 + 12);

	LWidget_SetLocation(&s->BtnClassic,    ANCHOR_MIN, ANCHOR_CENTRE, middle - 250, 72);
	LWidget_SetLocation(&s->LblClassic[0], ANCHOR_MIN, ANCHOR_CENTRE, middle - 85,  72 - 12);
	LWidget_SetLocation(&s->LblClassic[1], ANCHOR_MIN, ANCHOR_CENTRE, middle - 85,  72 + 12);

	LWidget_SetLocation(&s->LblHelp, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 160);
	LWidget_SetLocation(&s->BtnBack, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 170);
}

static void ChooseModeScreen_Init(struct LScreen* s_) {
	struct ChooseModeScreen* s = (struct ChooseModeScreen*)s_;
	if (!s->NumWidgets) ChooseModeScreen_InitWidgets(s);

	s->LblHelp.Hidden = !s->FirstTime;
	s->BtnBack.Hidden =  s->FirstTime;
}

static void ChooseModeScreen_DrawAll(struct LScreen* s_) {
	int midX = Game_Width / 2, midY = Game_Height / 2;
	LScreen_DrawAll(s_);
	
	Drawer2D_Rect(&Launcher_Framebuffer, Launcher_ButtonBorderCol,
					midX - 250, midY - 35, 490, 1);
	Drawer2D_Rect(&Launcher_Framebuffer, Launcher_ButtonBorderCol, 
					midX - 250, midY + 35, 490, 1);
}

struct LScreen* ChooseModeScreen_MakeInstance(bool firstTime) {
	struct ChooseModeScreen* s = &ChooseModeScreen_Instance;
	LScreen_Reset((struct LScreen*)s);
	s->Init       = ChooseModeScreen_Init;
	s->Reposition = ChooseModeScreen_Reposition;
	s->DrawAll    = ChooseModeScreen_DrawAll;
	s->FirstTime  = firstTime;
	return (struct LScreen*)s;
}


/*########################################################################################################################*
*---------------------------------------------------------ColoursScreen---------------------------------------------------*
*#########################################################################################################################*/
/*struct ColoursScreen {
	LScreen_Layout
	struct LButton BtnDefault, BtnBack;
	struct LLabel LblNames[5], LblRGB[3];
	struct LInput IptColours[5 * 3];
	struct LWidget* _widgets[25];
};

static void MakeWidgets(struct ColoursScreen* s) {
	struct LScreen* s_ = (struct LScreen*)s;
	int i;
	for (i = 0; i < 5 * 3; i++) {
		LScreen_Input(s_, &s->IptColours[i], 55, false, NULL, )
	}
	MakeAllRGBTriplets(false);

	LScreen_Label(s_, &s->LblNames[0], "Background",
		ANCHOR_CENTRE, ANCHOR_CENTRE, -60, -100);
	LScreen_Label(s_, &s->LblNames[1], "Button border",
		ANCHOR_CENTRE, ANCHOR_CENTRE, -70, -60);
	LScreen_Label(s_, &s->LblNames[2], "Button highlight",
		ANCHOR_CENTRE, ANCHOR_CENTRE, -80, -20);
	LScreen_Label(s_, &s->LblNames[3], "Button",
		ANCHOR_CENTRE, ANCHOR_CENTRE, -40, 20);
	LScreen_Label(s_, &s->LblNames[4], "Active button",
		ANCHOR_CENTRE, ANCHOR_CENTRE, -70, 60);

	LScreen_Label(s_, &s->LblRGB[0], "Red",
		ANCHOR_CENTRE, ANCHOR_CENTRE, 30, -130);
	LScreen_Label(s_, &s->LblRGB[1], "Green",
		ANCHOR_CENTRE, ANCHOR_CENTRE, 95, -130);
	LScreen_Label(s_, &s->LblRGB[2], "Blue",
		ANCHOR_CENTRE, ANCHOR_CENTRE, 160, -130);

	LScreen_Button(s_, &s->BtnDefault, 160, 35, "Default colours",
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 120);
	LScreen_Button(s_, &s->BtnBack, 80, 35, "Back",
		ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 170);
}

void  MakeAllRGBTriplets(bool force) {
	widgetIndex = 0;
	MakeRGBTriplet(LauncherSkin.BackgroundCol, force, -100);
	MakeRGBTriplet(LauncherSkin.ButtonBorderCol, force, 60);
	MakeRGBTriplet(LauncherSkin.ButtonHighlightCol, force, -20);
	MakeRGBTriplet(LauncherSkin.ButtonForeCol, force, 20);
	MakeRGBTriplet(LauncherSkin.ButtonForeActiveCol, force, 60);
}

void MakeRGBTriplet(PackedCol defCol, bool force, int y) {
	MakeInput(GetCol(defCol.R, force), 55, false, 3, null)
		.SetLocation(ANCHOR_CENTRE, ANCHOR_CENTRE, 30, y);
	MakeInput(GetCol(defCol.G, force), 55, false, 3, null)
		.SetLocation(ANCHOR_CENTRE, ANCHOR_CENTRE, 95, y);
	MakeInput(GetCol(defCol.B, force), 55, false, 3, null)
		.SetLocation(ANCHOR_CENTRE, ANCHOR_CENTRE, 160, y);
}

string GetCol(byte col, bool force) {
	if (force) return col.ToString();
	Widget widget = widgets[widgetIndex];
	return widget == null ? col.ToString() : widget.Text;
}

void Init() {
	base.Init();
	view.Init();

	widgets[view.defIndex].OnClick = ResetColours;
	widgets[view.defIndex + 1].OnClick = SwitchToSettings;
	SetupInputHandlers();
	for (int i = 0; i < widgets.Length; i++) {
		InputWidget input = widgets[i] as InputWidget;
		if (input == null) continue;
		input.Chars.TextChanged = TextChanged;
	}
	Resize();
}

void ResetColours(int x, int y) {
	LauncherSkin.ResetToDefault();
	view.MakeAllRGBTriplets(true);
	game.RedrawBackground();
	Resize();
}

void Resize() {
	view.DrawAll();
	game.Dirty = true;
}

void Dispose() {
	view.Dispose();
	base.Dispose();
}


float colourAcc;
void MouseWheelChanged(float delta) {
	int steps = Utils.AccumulateWheelDelta(ref colourAcc, delta);
	AdjustSelectedColour(steps);
}

void KeyDown(Key key) {
	if (key == KEY_LEFT) {
		AdjustSelectedColour(-1);
	} else if (key == KEY_RIGHT) {
		AdjustSelectedColour(+1);
	} else if (key == KEY_UP) {
		AdjustSelectedColour(+10);
	} else if (key == KEY_DOWN) {
		AdjustSelectedColour(-10);
	} else {
		base.KeyDown(key);
	}
}

void AdjustSelectedColour(int delta) {
	if (curInput == null) return;
	int index = IndexOfWidget(curInput);
	if (index >= 15) return;

	byte col;
	if (!Byte.TryParse(curInput.Text, out col))  return;
	int newCol = col + delta;

	Utils.Clamp(ref newCol, 0, 255);
	curInput.Text = newCol.ToString();
	if (curInput.Chars.CaretPos >= curInput.Text.Length)
		curInput.Chars.CaretPos = -1;
	TextChanged(curInput);
}

void TextChanged(InputWidget widget) {
	bool changed = false;
	int index = IndexOfWidget(widget);
	if (index < 3)       changed |= Parse(0, ref LauncherSkin.BackgroundCol);
	else if (index < 6)  changed |= Parse(3, ref LauncherSkin.ButtonBorderCol);
	else if (index < 9)  changed |= Parse(6, ref LauncherSkin.ButtonHighlightCol);
	else if (index < 12) changed |= Parse(9, ref LauncherSkin.ButtonForeCol);
	else if (index < 15) changed |= Parse(12, ref LauncherSkin.ButtonForeActiveCol);

	if (!changed) return;
	game.RedrawBackground();
	Resize();
}

bool Parse(int index, ref PackedCol dst) {
	byte r, g, b;
	if (!Byte.TryParse(widgets[index + 0].Text, out r)
		|| !Byte.TryParse(widgets[index + 1].Text, out g)
		|| !Byte.TryParse(widgets[index + 2].Text, out b))
		return false;
	dst.R = r; dst.G = g; dst.B = b;
	return true;
}*/


/*########################################################################################################################*
*------------------------------------------------------DirectConnectScreen------------------------------------------------*
*#########################################################################################################################*/
static struct DirectConnectScreen {
	LScreen_Layout
	struct LButton BtnConnect, BtnBack;
	struct LInput IptUsername, IptAddress, IptMppass;
	struct LLabel LblStatus;
	struct LWidget* _widgets[6];
} DirectConnectScreen_Instance;

CC_NOINLINE static void DirectConnectScreen_SetStatus(const char* msg) {
	String str = String_FromReadonly(msg);
	struct LLabel* w = &DirectConnectScreen_Instance.LblStatus;
	Launcher_ResetArea(w->X, w->Y, w->Width, w->Height);

	LLabel_SetText(w, &str, &Launcher_TextFont);
	w->VTABLE->Redraw(w);
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
	Launcher_LoadSecureOpt("launcher-dc-mppass", &mppass, &user);
	String_InitArray(addr, addrBuffer);
	String_Format2(&addr, "%s:%s", &ip, &port);

	/* don't want just ':' for address */
	if (addr.length == 1) addr.length = 0;
	
	LInput_SetText(&s->IptUsername, &user,   &Launcher_TextFont);
	LInput_SetText(&s->IptAddress,  &addr,   &Launcher_TextFont);
	LInput_SetText(&s->IptMppass,   &mppass, &Launcher_TextFont);
}

static void DirectConnectScreen_Save(const String* user, const String* mppass, const String* ip, const String* port) {
	Options_Load();
	Options_Set("launcher-dc-username", user);
	Options_Set("launcher-dc-ip",       ip);
	Options_Set("launcher-dc-port",     port);
	Launcher_SaveSecureOpt("launcher-dc-mppass", mppass, user);
	Launcher_SaveOptions = true;
}

static void StartClient(void* w, int x, int y) {
	static String loopbackIp = String_FromConst("127.0.0.1");
	static String defMppass  = String_FromConst("(none)");
	String* user   = &DirectConnectScreen_Instance.IptUsername.Text;
	String* addr   = &DirectConnectScreen_Instance.IptAddress.Text;
	String* mppass = &DirectConnectScreen_Instance.IptMppass.Text;

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
	if (!Convert_TryParseUInt16(&port, &raw_port)) {
		DirectConnectScreen_SetStatus("&eInvalid port"); return;
	}
	if (!mppass->length) mppass = &defMppass;

	DirectConnectScreen_Save(user, mppass, &ip, &port);
	DirectConnectScreen_SetStatus("");
	Launcher_StartGame(user, mppass, &ip, &port, &String_Empty);
}

static void DirectConnectScreen_InitWidgets(struct DirectConnectScreen* s) {
	struct LScreen* s_ = (struct LScreen*)s;
	s->Widgets = s->_widgets;

	LScreen_Input(s_, &s->IptUsername, 330, false, "&gUsername..");
	LScreen_Input(s_, &s->IptAddress,  330, false, "&gIP address:Port number..");
	LScreen_Input(s_, &s->IptMppass,   330, false, "&gMppass..");

	LScreen_Button(s_, &s->BtnConnect, 110, 35, "Connect");
	LScreen_Button(s_, &s->BtnBack,     80, 35, "Back");
	LScreen_Label(s_,  &s->LblStatus, "");

	s->BtnConnect.OnClick = StartClient;
	s->BtnBack.OnClick    = SwitchToMain;

	/* Init input text from options */
	DirectConnectScreen_Load(s);
}

static void DirectConnectScreen_Reposition(struct LScreen* s_) {
	struct DirectConnectScreen* s = (struct DirectConnectScreen*)s_;
	LWidget_SetLocation(&s->IptUsername, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, -120);
	LWidget_SetLocation(&s->IptAddress,  ANCHOR_CENTRE, ANCHOR_CENTRE, 0,  -75);
	LWidget_SetLocation(&s->IptMppass,   ANCHOR_CENTRE, ANCHOR_CENTRE, 0,  -30);

	LWidget_SetLocation(&s->BtnConnect, ANCHOR_CENTRE, ANCHOR_CENTRE, -110, 20);
	LWidget_SetLocation(&s->BtnBack,    ANCHOR_CENTRE, ANCHOR_CENTRE,  125, 20);
	LWidget_SetLocation(&s->LblStatus,  ANCHOR_CENTRE, ANCHOR_CENTRE,    0, 70);
}

static void DirectConnectScreen_Init(struct LScreen* s_) {
	struct DirectConnectScreen* s = (struct DirectConnectScreen*)s_;
	if (!s->NumWidgets) DirectConnectScreen_InitWidgets(s);
}

struct LScreen* DirectConnectScreen_MakeInstance(void) {
	struct DirectConnectScreen* s = &DirectConnectScreen_Instance;
	LScreen_Reset((struct LScreen*)s);
	s->Init          = DirectConnectScreen_Init;
	s->Reposition    = DirectConnectScreen_Reposition;
	s->OnEnterWidget = (struct LWidget*)&s->BtnConnect;
	return (struct LScreen*)s;
}


/*########################################################################################################################*
*--------------------------------------------------------SettingsScreen---------------------------------------------------*
*#########################################################################################################################*/
static struct SettingsScreen {
	LScreen_Layout
	struct LButton BtnUpdates, BtnMode, BtnColours, BtnBack;
	struct LLabel  LblUpdates, LblMode, LblColours;
	struct LWidget* _widgets[7];
} SettingsScreen_Instance;

static void SettingsScreen_InitWidgets(struct SettingsScreen* s) {
	struct LScreen* s_ = (struct LScreen*)s;
	s->Widgets = s->_widgets;

	LScreen_Button(s_, &s->BtnUpdates, 110, 35, "Updates");
	LScreen_Label(s_,  &s->LblUpdates, "&eGet the latest stuff");

	LScreen_Button(s_, &s->BtnMode, 110, 35, "Mode");
	LScreen_Label(s_,  &s->LblMode, "&eChange the enabled features");

	LScreen_Button(s_, &s->BtnColours, 110, 35, "Colours");
	LScreen_Label(s_,  &s->LblColours, "&eChange how the launcher looks");

	LScreen_Button(s_, &s->BtnBack, 80, 35, "Back");

	s->BtnMode.OnClick    = SwitchToChooseMode;
	s->BtnUpdates.OnClick = SwitchToUpdates;
	s->BtnColours.OnClick = SwitchToColours;
	s->BtnBack.OnClick    = SwitchToMain;
}

static void SettingsScreen_Reposition(struct LScreen* s_) {
	struct SettingsScreen* s = (struct SettingsScreen*)s_;
	LWidget_SetLocation(&s->BtnUpdates, ANCHOR_CENTRE, ANCHOR_CENTRE, -135, -120);
	LWidget_SetLocation(&s->LblUpdates, ANCHOR_CENTRE, ANCHOR_CENTRE,   10, -120);

	LWidget_SetLocation(&s->BtnMode, ANCHOR_CENTRE, ANCHOR_CENTRE, -135, -70);
	LWidget_SetLocation(&s->LblMode, ANCHOR_CENTRE, ANCHOR_CENTRE,   55, -70);

	LWidget_SetLocation(&s->BtnColours, ANCHOR_CENTRE, ANCHOR_CENTRE, -135, -20);
	LWidget_SetLocation(&s->LblColours, ANCHOR_CENTRE, ANCHOR_CENTRE,   65, -20);

	LWidget_SetLocation(&s->BtnBack, ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 170);
}

static void SettingsScreen_Init(struct LScreen* s_) {
	struct SettingsScreen* s = (struct SettingsScreen*)s_;
	if (!s->NumWidgets) SettingsScreen_InitWidgets(s);

	s->BtnColours.Hidden = Launcher_ClassicBackground;
	s->LblColours.Hidden = Launcher_ClassicBackground;
}

struct LScreen* SettingsScreen_MakeInstance(void) {
	struct SettingsScreen* s = &SettingsScreen_Instance;
	LScreen_Reset((struct LScreen*)s);
	s->Init       = SettingsScreen_Init;
	s->Reposition = SettingsScreen_Reposition;
	return (struct LScreen*)s;
}
