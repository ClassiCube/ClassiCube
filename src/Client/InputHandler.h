#ifndef CC_INPUTHANDLER_H
#define CC_INPUTHANDLER_H
#include "Typedefs.h"
/* Implements base handlers for mouse and keyboard input.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct Screen;

bool InputHandler_SetFOV(Int32 fov, bool setZoom);
void InputHandler_PickBlocks(bool cooldown, bool left, bool middle, bool right);
void InputHandler_Init(void);
void InputHandler_ScreenChanged(struct Screen* oldScreen, struct Screen* newScreen);
#endif
