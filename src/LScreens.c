#include "LScreens.h"
#include "LWidgets.h"
#include "Launcher.h"
#include "Gui.h"

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
*--------------------------------------------------------SettingsScreen---------------------------------------------------*
*#########################################################################################################################*/
/*static struct SettingsScreen {
	LScreen_Layout
	struct LWidget* Widgets[7];
	struct LButton BtnUpdates, BtnMode, BtnColours, BtnBack;
	struct LLabel  LblUpdates, LblMode, LblColours;
} SettingsScreen_Instance;

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
*/