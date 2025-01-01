#ifndef CC_HELDBLOCKRENDERER_H
#define CC_HELDBLOCKRENDERER_H
#include "Core.h"
CC_BEGIN_HEADER

/* 
Renders the held block/arm at bottom right of game
Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent HeldBlockRenderer_Component;
/* Whether held block/arm should be shown at all. */
extern cc_bool HeldBlockRenderer_Show;

void HeldBlockRenderer_ClickAnim(cc_bool digging);
void HeldBlockRenderer_Render(float delta);

CC_END_HEADER
#endif
