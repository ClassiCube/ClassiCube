#ifndef CC_SELOUTLINERENDERER_H
#define CC_SELOUTLINERENDERER_H
#include "Core.h"
CC_BEGIN_HEADER

/* Renders an outline around the block the player is looking at.
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct RayTracer;
struct IGameComponent;
extern struct IGameComponent SelOutlineRenderer_Component;

void SelOutlineRenderer_Render(struct RayTracer* selected, cc_bool dirty);

CC_END_HEADER
#endif
