#ifndef CC_SCREENS_H
#define CC_SCREENS_H
#include "String.h"
/* Contains all 2D non-menu screen implementations.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct Screen;
struct Widget;
struct IGameComponent;

struct Screen* InventoryScreen_MakeInstance(void);
struct Screen* StatusScreen_MakeInstance(void);
void StatusScreen_MakeComponent(struct IGameComponent* comp);
struct Screen* LoadingScreen_MakeInstance(STRING_PURE String* title, STRING_PURE String* message);
struct Screen* GeneratingScreen_MakeInstance(void);
struct Screen* HUDScreen_MakeInstance(void);
void HUDScreen_MakeComponent(struct IGameComponent* comp);
struct Screen* DisconnectScreen_MakeInstance(STRING_PURE String* title, STRING_PURE String* message);

/* Raw pointer to inventory screen. DO NOT USE THIS. Use InventoryScreen_MakeInstance() */
extern struct Screen* InventoryScreen_UNSAFE_RawPointer;
/* Raw pointer to loading screen. DO NOT USE THIS. Use LoadingScreen_MakeInstance() */
extern struct Screen* LoadingScreen_UNSAFE_RawPointer;
void HUDScreen_OpenInput(struct Screen* hud, STRING_PURE String* text);
void HUDScreen_AppendInput(struct Screen* hud, STRING_PURE String* text);
struct Widget* HUDScreen_GetHotbar(struct Screen* hud);
#endif
