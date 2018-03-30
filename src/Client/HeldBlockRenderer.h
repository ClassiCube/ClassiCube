#ifndef CC_HELDBLOCKRENDERER_H
#define CC_HELDBLOCKRENDERER_H
#include "GameStructs.h"
/* Implements rendering of held block/arm at bottom right of game.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

void HeldBlockRenderer_ClickAnim(bool digging);
void HeldBlockRenderer_Render(Real64 delta);
IGameComponent HeldBlockRenderer_MakeComponent(void);
#endif