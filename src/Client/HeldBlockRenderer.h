#ifndef CC_HELDBLOCKRENDERER_H
#define CC_HELDBLOCKRENDERER_H
#include "Typedefs.h"
/* Implements rendering of held block/arm at bottom right of game.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct IGameComponent;

void HeldBlockRenderer_ClickAnim(bool digging);
void HeldBlockRenderer_Render(Real64 delta);
void HeldBlockRenderer_MakeComponent(struct IGameComponent* comp);
#endif
