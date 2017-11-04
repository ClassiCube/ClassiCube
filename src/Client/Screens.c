#include "Screens.h"
#include "Widgets.h"
#include "Game.h"
#include "Events.h"
#include "GraphicsCommon.h"

void Screen_FreeWidgets(Widget** widgets, UInt32 widgetsCount) {
	if (widgets == NULL) return;
	UInt32 i;
	for (i = 0; i < widgetsCount; i++) {
		if (widgets[i] == NULL) continue;
		widgets[i]->Base.Free(&widgets[i]->Base);
	}
}

void Screen_RepositionWidgets(Widget** widgets, UInt32 widgetsCount) {
	if (widgets == NULL) return;
	UInt32 i;
	for (i = 0; i < widgetsCount; i++) {
		if (widgets[i] == NULL) continue;
		widgets[i]->Reposition(widgets[i]);
	}
}

void Screen_RenderWidgets(Widget** widgets, UInt32 widgetsCount, Real64 delta) {
	if (widgets == NULL) return;

	UInt32 i;
	for (i = 0; i < widgetsCount; i++) {
		if (widgets[i] == NULL) continue;
		widgets[i]->Base.Render(&widgets[i]->Base, delta);
	}
}



/* These were sourced by taking a screenshot of vanilla
   Then using paint to extract the colour components
   Then using wolfram alpha to solve the glblendfunc equation */
PackedCol Menu_TopBackCol    = PACKEDCOL_CONST(24, 24, 24, 105);
PackedCol Menu_BottomBackCol = PACKEDCOL_CONST(51, 51, 98, 162);

void ClickableScreen_RenderMenuBounds(void) {
	GfxCommon_Draw2DGradient(0, 0, Game_Width, Game_Height, Menu_TopBackCol, Menu_BottomBackCol);
}

void ClickableScreen_DefaultWidgetSelected(GuiElement* elem, Widget* widget) { }

bool ClickableScreen_HandlesMouseDown(GuiElement* elem, Int32 x, Int32 y, MouseButton btn) {
	ClickableScreen* screen = (ClickableScreen*)elem;
	UInt32 i;
	/* iterate backwards (because last elements rendered are shown over others) */
	for (i = screen->WidgetsCount; i > 0; i--) {
		Widget* widget = screen->Widgets[i - 1];
		if (widget != NULL && Gui_Contains(widget->X, widget->Y, widget->Width, widget->Height, x, y)) {
			if (!widget->Disabled) {
				elem = &widget->Base;
				elem->HandlesMouseDown(elem, x, y, btn);
			}
			return true;
		}
	}
	return false;
}

bool ClickableScreens_HandleMouseMove(GuiElement* elem, Int32 x, Int32 y) {
	ClickableScreen* screen = (ClickableScreen*)elem;
	if (screen->LastX == x && screen->LastY == y) return true;
	UInt32 i;
	for (i = 0; i < screen->WidgetsCount; i++) {
		Widget* widget = screen->Widgets[i];
		if (widget != NULL) widget->Active = false;
	}

	for (i = screen->WidgetsCount; i > 0; i--) {
		Widget* widget = screen->Widgets[i];
		if (widget != NULL && Gui_Contains(widget->X, widget->Y, widget->Width, widget->Height, x, y)) {
			widget->Active = true;
			screen->LastX = x; screen->LastY = y;
			screen->OnWidgetSelected(elem, widget);
			return true;
		}
	}

	screen->LastX = x; screen->LastY = y;
	screen->OnWidgetSelected(elem, NULL);
	return false;
}

void ClickableScreen_Create(ClickableScreen* screen) {
	Screen_Init(&screen->Base);
	screen->Base.Base.HandlesMouseDown = ClickableScreen_HandlesMouseDown;
	screen->Base.Base.HandlesMouseMove = ClickableScreens_HandleMouseMove;

	screen->LastX = -1; screen->LastY = -1;
	screen->OnWidgetSelected = ClickableScreen_DefaultWidgetSelected;
}

Screen AA;
extern Screen* InventoryScreen_Unsafe_RawPointer = &AA;
