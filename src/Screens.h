#ifndef CC_SCREENS_H
#define CC_SCREENS_H
#include "String.h"
/* Contains all 2D non-menu screen implementations.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/
struct Screen;
struct Widget;

struct Screen* InventoryScreen_MakeInstance(void);
struct Screen* StatusScreen_MakeInstance(void);
struct Screen* LoadingScreen_MakeInstance(const String* title, const String* message);
struct Screen* GeneratingScreen_MakeInstance(void);
struct Screen* HUDScreen_MakeInstance(void);
struct Screen* DisconnectScreen_MakeInstance(const String* title, const String* message);

/* Raw pointer to inventory screen. DO NOT USE THIS. Use InventoryScreen_MakeInstance() */
extern struct Screen* InventoryScreen_UNSAFE_RawPointer;
/* Raw pointer to loading screen. DO NOT USE THIS. Use LoadingScreen_MakeInstance() */
extern struct Screen* LoadingScreen_UNSAFE_RawPointer;
/* Opens chat input for the HUD with the given initial text. */
void HUDScreen_OpenInput(const String* text);
/* Appends text to the chat input in the HUD. */
void HUDScreen_AppendInput(const String* text);
struct Widget* HUDScreen_GetHotbar(void);
#endif
