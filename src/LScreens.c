#include "LScreens.h"
#include "LWidgets.h"
#include "Launcher.h"
#include "Gui.h"
#include "Game.h"

static void LScreen_NullFunc(struct LScreen* s) { }
static void LScreen_DrawAll(struct LScreen* s) {
	struct LWidget* widget;
	int i;

	for (i = 0; i < s->NumWidgets; i++) {
		widget = s->Widgets[i];
		widget->VTABLE->Redraw(widget);
	}
}

static void LScreen_HoverWidget(struct LScreen* s, struct LWidget* w) {
	if (!w) return;
	w->Hovered = true;
	if (w->VTABLE->OnHover) w->VTABLE->OnHover(w);
}

static void LScreen_UnhoverWidget(struct LScreen* s, struct LWidget* w) {
	if (!w) return;
	w->Hovered = false;
	if (w->VTABLE->OnUnhover) w->VTABLE->OnUnhover(w);
}

CC_NOINLINE static void LScreen_Reset(struct LScreen* s) {
	int i;

	s->Init = NULL; /* screens should always override this */
	s->Free = LScreen_NullFunc;
	s->DrawAll   = LScreen_DrawAll;
	s->Tick      = LScreen_NullFunc;
	s->OnDisplay = LScreen_NullFunc;
	s->HoverWidget   = LScreen_HoverWidget;
	s->UnhoverWidget = LScreen_UnhoverWidget;

	/* reset all widgets to unselected */
	for (i = 0; i < s->NumWidgets; i++) { 
		s->Widgets[i]->Hovered = false;
	}
}


/*########################################################################################################################*
*---------------------------------------------------------Widget init-----------------------------------------------------*
*#########################################################################################################################*/
CC_NOINLINE static void LScreen_Button(struct LScreen* s, struct LButton* w, int width, int height, const char* text,
									   uint8_t horAnchor, uint8_t verAnchor, int xOffset, int yOffset) {
	String str = String_FromReadonly(text);
	LButton_Init(w, width, height);
	LButton_SetText(w, &str, &Launcher_TitleFont);

	w->Hovered = false;
	s->Widgets[s->NumWidgets++] = (struct LWidget*)w;
	LWidget_SetLocation(w, horAnchor, verAnchor, xOffset, yOffset);
}

CC_NOINLINE static void LScreen_Label(struct LScreen* s, struct LLabel* w, const char* text,
									  uint8_t horAnchor, uint8_t verAnchor, int xOffset, int yOffset) {
	String str = String_FromReadonly(text);
	LLabel_Init(w);
	LLabel_SetText(w, &str, &Launcher_TextFont);

	w->Hovered = false;
	s->Widgets[s->NumWidgets++] = (struct LWidget*)w;
	LWidget_SetLocation(w, horAnchor, verAnchor, xOffset, yOffset);
}

CC_NOINLINE static void LScreen_Slider(struct LScreen* s, struct LSlider* w, int width, int height,
									  int initValue, int maxValue, BitmapCol progressCol,
									  uint8_t horAnchor, uint8_t verAnchor, int xOffset, int yOffset) {
	LSlider_Init(w, width, height);
	w->Value = initValue; w->MaxValue = maxValue;
	w->ProgressCol = progressCol;

	w->Hovered = false;
	s->Widgets[s->NumWidgets++] = (struct LWidget*)w;
	LWidget_SetLocation(w, horAnchor, verAnchor, xOffset, yOffset);
}

/*########################################################################################################################*
*-------------------------------------------------------ChooseModeScreen--------------------------------------------------*
*#########################################################################################################################*/
static struct ChooseModeScreen {
	LScreen_Layout
	struct LWidget* Widgets[12];
	struct LButton BtnEnhanced, BtnClassicHax, BtnClassic, BtnBack;
	struct LLabel  LblTitle, LblHelp, LblEnhanced[2], LblClassicHax[2], LblClassic[2];
	bool FirstTime;
} ChooseModeScreen_Instance;

static void ChooseMode_Click(bool classic, bool classicHacks) {
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

static void UseModeEnhanced(void* w, int x, int y) {
	Launcher_SetScreen(ChooseModeScreen_MakeInstance(false));
}
static void UseModeClassicHax(void* w, int x, int y) {
	Launcher_SetScreen(UpdatesScreen_MakeInstance());
}
static void UseModeClassic(void* w, int x, int y) {
	Launcher_SetScreen(ColoursScreen_MakeInstance());
}
static void SwitchToSettings(void* w, int x, int y) {
	Launcher_SetScreen(SettingsScreen_MakeInstance());
}

static void ChooseModeScreenScreen_InitWidgets(struct ChooseModeScreen* s) {
	struct LScreen* s_ = (struct LScreen*)s;
	int middle = Game_Width / 2;

	LScreen_Label(s_,  &s->LblTitle, "&eGet the latest stuff",
					ANCHOR_CENTRE, ANCHOR_CENTRE, 10, -135);

	LScreen_Button(s_, &s->BtnEnhanced, 145, 35, "Enhanced",
					ANCHOR_MIN, ANCHOR_CENTRE, middle - 250, -72);
	LScreen_Label(s_,  &s->LblEnhanced[0], "&eEnables custom blocks, changing env",
					ANCHOR_MIN, ANCHOR_CENTRE, middle - 85,  -72 - 12);
	LScreen_Label(s_,  &s->LblEnhanced[1], "&esettings, longer messages, and more",
					ANCHOR_MIN, ANCHOR_CENTRE, middle - 85,  -72 + 12);

	LScreen_Button(s_, &s->BtnClassicHax, 145, 35, "Classic +hax",
					ANCHOR_MIN, ANCHOR_CENTRE, middle - 250, 0);
	LScreen_Label(s_,  &s->LblClassicHax[0], "&eSame as Classic mode, except that",
					ANCHOR_MIN, ANCHOR_CENTRE, middle - 85,  0 - 12);
	LScreen_Label(s_,  &s->LblClassicHax[1], "&ehacks (noclip/fly/speed) are enabled",
					ANCHOR_MIN, ANCHOR_CENTRE, middle - 85,  0 + 12);

	LScreen_Button(s_, &s->BtnClassic, 145, 35, "Classic",
					ANCHOR_MIN, ANCHOR_CENTRE, middle - 250, 72);
	LScreen_Label(s_,  &s->LblClassic[0], "&eOnly uses blocks and features from",
					ANCHOR_MIN, ANCHOR_CENTRE, middle - 85,  72 - 12);
	LScreen_Label(s_,  &s->LblClassic[1], "&ethe original minecraft classic",
					ANCHOR_MIN, ANCHOR_CENTRE, middle - 85,   72 + 12);

	LScreen_Label(s_,  &s->LblHelp, "&eClick &fEnhanced &eif you'e not sure which mode to choose.",
					ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 160);
	LScreen_Button(s_, &s->BtnBack, 80, 35, "Back",
					ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 170);

	s->BtnEnhanced.OnClick   = UseModeEnhanced;
	s->BtnClassicHax.OnClick = UseModeClassicHax;
	s->BtnClassic.OnClick    = UseModeClassic;
	s->BtnBack.OnClick       = SwitchToSettings;
}

static void ChooseModeScreen_Init(struct LScreen* s_) {
	struct ChooseModeScreen* s = (struct ChooseModeScreen*)s_;
	if (!s->NumWidgets) ChooseModeScreen_InitWidgets(s);

	s->LblHelp.Hidden = !s->FirstTime;
	s->BtnBack.Hidden =  s->FirstTime;
	s->DrawAll(s);
}

struct LScreen* ChooseModeScreen_MakeInstance(bool firstTime) {
	struct ChooseModeScreen* s = &ChooseModeScreen_Instance;
	LScreen_Reset((struct LScreen*)s);
	s->Init = ChooseModeScreen_Init;
	s->FirstTime = firstTime;
	return (struct LScreen*)s;
}


/*########################################################################################################################*
*--------------------------------------------------------SettingsScreen---------------------------------------------------*
*#########################################################################################################################*/
static struct SettingsScreen {
	LScreen_Layout
	struct LWidget* Widgets[7];
	struct LButton BtnUpdates, BtnMode, BtnColours, BtnBack;
	struct LLabel  LblUpdates, LblMode, LblColours;
} SettingsScreen_Instance;

static void SwitchToChooseMode(void* w, int x, int y) { 
	Launcher_SetScreen(ChooseModeScreen_MakeInstance(false)); 
}
static void SwitchToUpdates(void* w, int x, int y) { 
	Launcher_SetScreen(UpdatesScreen_MakeInstance()); 
}
static void SwitchToColours(void* w, int x, int y) {
	Launcher_SetScreen(ColoursScreen_MakeInstance()); 
}
static void SwitchToMain(void* w, int x, int y) { 
	Launcher_SetScreen(MainScreen_MakeInstance()); 
}

static void SettingsScreen_InitWidgets(struct SettingsScreen* s) {
	struct LScreen* s_ = (struct LScreen*)s;

	LScreen_Button(s_, &s->BtnUpdates, 110, 35, "Updates",
					ANCHOR_CENTRE, ANCHOR_CENTRE, -135, -120);
	LScreen_Label(s_,  &s->LblUpdates, "&eGet the latest stuff",
					ANCHOR_CENTRE, ANCHOR_CENTRE, 10, -120);

	LScreen_Button(s_, &s->BtnMode, 110, 35, "Mode",
					ANCHOR_CENTRE, ANCHOR_CENTRE, -135, -70);
	LScreen_Label(s_,  &s->LblMode, "&eChange the enabled features",
					ANCHOR_CENTRE, ANCHOR_CENTRE, 55, -70);

	LScreen_Button(s_, &s->BtnColours, 110, 35, "Colours",
					ANCHOR_CENTRE, ANCHOR_CENTRE, -135, -20);
	LScreen_Label(s_,  &s->LblColours, "&eChange how the launcher looks",
					ANCHOR_CENTRE, ANCHOR_CENTRE, 65, -20);

	LScreen_Button(s_, &s->BtnBack, 80, 35, "Back",
					ANCHOR_CENTRE, ANCHOR_CENTRE, 0, 170);

	s->BtnMode.OnClick    = SwitchToChooseMode;
	s->BtnUpdates.OnClick = SwitchToUpdates;
	s->BtnColours.OnClick = SwitchToColours;
	s->BtnBack.OnClick    = SwitchToMain;
}

static void SettingsScreen_Init(struct LScreen* s_) {
	struct SettingsScreen* s = (struct SettingsScreen*)s_;
	if (!s->NumWidgets) SettingsScreen_InitWidgets(s);

	s->BtnColours.Hidden = Launcher_ClassicBackground;
	s->LblColours.Hidden = Launcher_ClassicBackground;
	s->DrawAll(s);
}

struct LScreen* SettingsScreen_MakeInstance(void) {
	struct SettingsScreen* s = &SettingsScreen_Instance;
	LScreen_Reset((struct LScreen*)s);
	s->Init = SettingsScreen_Init;
	return (struct LScreen*)s;
}
