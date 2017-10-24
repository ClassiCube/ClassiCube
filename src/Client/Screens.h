#ifndef CC_SCREEN_H
#define CC_SCREEN_H
#include "Typedefs.h"
#include "Gui.h"

/* Contains all 2D screen implementations.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

typedef struct ClickableScreen_ {
	Screen Base;
	Widget** Widgets;
	UInt32 WidgetsCount;
	Int32 LastX, LastY;
	void (*OnWidgetSelected)(GuiElement* elem, Widget* widget);
} ClickableScreen;
void ClickableScreen_Create(ClickableScreen* screen);

/* Raw pointer to inventory screen. DO NOT USE THIS. Use InventoryScreen_GetInstance() */
extern Screen* InventoryScreen_Unsafe_RawPointer;
#endif