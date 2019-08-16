#ifndef CC_SCREENS_H
#define CC_SCREENS_H
#include "String.h"
/* Contains all 2D non-menu screen implementations.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/
struct Screen;
struct Widget;

void InventoryScreen_Show(void);
void StatusScreen_Show(void);
void LoadingScreen_Show(const String* title, const String* message);
void GeneratingScreen_Show(void);
void HUDScreen_Show(void);
void DisconnectScreen_Show(const String* title, const String* message);

/* Raw pointer to loading screen. DO NOT USE THIS. Use LoadingScreen_MakeInstance() */
extern struct Screen* LoadingScreen_UNSAFE_RawPointer;
/* Opens chat input for the HUD with the given initial text. */
void HUDScreen_OpenInput(const String* text);
/* Appends text to the chat input in the HUD. */
void HUDScreen_AppendInput(const String* text);
struct Widget* HUDScreen_GetHotbar(void);
#endif
