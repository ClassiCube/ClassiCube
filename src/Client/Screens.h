#ifndef CC_SCREENS_H
#define CC_SCREENS_H
#include "Gui.h"

/* Contains all 2D non-menu screen implementations.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

Screen* InventoryScreen_MakeInstance(void);
Screen* StatusScreen_MakeInstance(void);
IGameComponent StatusScreen_MakeComponent(void);
Screen* LoadingScreen_MakeInstance(STRING_PURE String* title, STRING_PURE String* message);
Screen* GeneratingScreen_MakeInstance(void);
Screen* HUDScreen_MakeInstance(void);
IGameComponent HUDScreen_MakeComponent(void);
void HUDScreen_OpenInput(Screen* hud, STRING_PURE String* text);
void HUDScreen_AppendInput(Screen* hud, STRING_PURE String* text);
Screen* DisconnectScreen_MakeInstance(STRING_PURE String* title, STRING_PURE String* message);

Screen* OptionsGroupScreen_MakeInstance(void);
Screen* PauseScreen_MakeInstance(void);

/* Raw pointer to inventory screen. DO NOT USE THIS. Use InventoryScreen_MakeInstance() */
extern Screen* InventoryScreen_UNSAFE_RawPointer;
#endif