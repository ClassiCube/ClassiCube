#ifndef CC_SCREENS_H
#define CC_SCREENS_H
#include "VertexStructs.h"
/* Contains all 2D non-menu screen implementations.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/
struct Screen;
struct Widget;

/* These always return false */
int Screen_FInput(void* s, int key);
int Screen_FKeyPress(void* s, char keyChar);
int Screen_FText(void* s, const String* str);
int Screen_FMouseScroll(void* s, float delta);
int Screen_FPointer(void* s, int id, int x, int y);

/* These always return true */
int Screen_TInput(void* s, int key);
int Screen_TKeyPress(void* s, char keyChar);
int Screen_TText(void* s, const String* str);
int Screen_TMouseScroll(void* s, float delta);
int Screen_TPointer(void* s, int id, int x, int y);

void Screen_NullFunc(void* screen);
void Screen_NullUpdate(void* screen, double delta);
int  Screen_InputDown(void* screen, int key);

/* Calls Elem_Render on each widget in the screen. */
void Screen_RenderWidgets(void* screen, double delta);
/* Calls Widget_Render2 on each widget in the screen. */
void Screen_Render2Widgets(void* screen, double delta);
/* Calls Widget_Layout on each widget in the screen. */
void Screen_Layout(void* screen);
/* Calls Widget_Free on each widget in the screen. */
/* Also deletes the screen's vb. */
void Screen_ContextLost(void* screen);
void Screen_CreateVb(void* screen);
void Screen_BuildMesh(void* screen);

void InventoryScreen_Show(void);
void HUDScreen_Show(void);
void LoadingScreen_Show(const String* title, const String* message);
void GeneratingScreen_Show(void);
void ChatScreen_Show(void);
void DisconnectScreen_Show(const String* title, const String* message);
void TablistScreen_Show(void);
#ifdef CC_BUILD_TOUCH
void TouchScreen_Show(void);
#endif

/* Raw pointer to loading screen. DO NOT USE THIS. Use LoadingScreen_MakeInstance() */
extern struct Screen* LoadingScreen_UNSAFE_RawPointer;
/* Opens chat input for the HUD with the given initial text. */
void ChatScreen_OpenInput(const String* text);
/* Appends text to the chat input in the HUD. */
void ChatScreen_AppendInput(const String* text);
/* Sets number of visible lines in the main chat widget. */
void ChatScreen_SetChatlines(int lines);
struct Widget* HUDScreen_GetHotbar(void);
#endif
