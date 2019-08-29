#ifndef CC_INPUTHANDLER_H
#define CC_INPUTHANDLER_H
#include "Input.h"
/* Implements base handlers for mouse and keyboard input.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/
struct Screen;

bool InputHandler_SetFOV(int fov);
void InputHandler_PickBlocks(bool cooldown, bool left, bool middle, bool right);
void InputHandler_Init(void);
void InputHandler_OnScreensChanged(void);
#endif
