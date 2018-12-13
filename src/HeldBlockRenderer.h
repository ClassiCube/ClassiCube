#ifndef CC_HELDBLOCKRENDERER_H
#define CC_HELDBLOCKRENDERER_H
#include "Core.h"
/* Implements rendering of held block/arm at bottom right of game.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent HeldBlockRenderer_Component;
/* Whether held block/arm should be shown at all. */
extern bool HeldBlockRenderer_Show;

void HeldBlockRenderer_ClickAnim(bool digging);
void HeldBlockRenderer_Render(double delta);
#endif
