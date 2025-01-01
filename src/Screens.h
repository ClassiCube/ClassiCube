#ifndef CC_SCREENS_H
#define CC_SCREENS_H
#include "Core.h"
CC_BEGIN_HEADER

/* Contains all 2D non-menu screen implementations.
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct Screen;
struct InputDevice;

/* These always return false */
int Screen_FInput(void* s, int key, struct InputDevice* device);
int Screen_FKeyPress(void* s, char keyChar);
int Screen_FText(void* s, const cc_string* str);
int Screen_FMouseScroll(void* s, float delta);
int Screen_FPointer(void* s, int id, int x, int y);

/* These always return true */
int Screen_TInput(void* s, int key, struct InputDevice* device);
int Screen_TKeyPress(void* s, char keyChar);
int Screen_TText(void* s, const cc_string* str);
int Screen_TMouseScroll(void* s, float delta);
int Screen_TPointer(void* s, int id, int x, int y);

void Screen_NullFunc(void* screen);
void Screen_NullUpdate(void* screen, float delta);

void InventoryScreen_Show(void);
void HUDScreen_Show(void);
void LoadingScreen_Show(const cc_string* title, const cc_string* message);
void GeneratingScreen_Show(void);
void ChatScreen_Show(void);
void DisconnectScreen_Show(const cc_string* title, const cc_string* message);
#ifdef CC_BUILD_TOUCH
void TouchScreen_Refresh(void);
void TouchScreen_Show(void);
#endif

int HUDScreen_LayoutHotbar(void);
void TabListOverlay_Show(cc_bool staysOpen);

/* Opens chat input for the HUD with the given initial text. */
void ChatScreen_OpenInput(const cc_string* text);
/* Appends text to the chat input in the HUD. */
void ChatScreen_AppendInput(const cc_string* text);
/* Sets number of visible lines in the main chat widget. */
void ChatScreen_SetChatlines(int lines);

CC_END_HEADER
#endif
