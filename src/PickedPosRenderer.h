#ifndef CC_PICKEDPOSRENDERER_H
#define CC_PICKEDPOSRENDERER_H
#include "Core.h"
/* Renders an outline around the block the player is looking at.
   Copyright 2014-2023 ClassiCube | Licensed under BSD-3
*/
struct RayTracer;
struct IGameComponent;
extern struct IGameComponent PickedPosRenderer_Component;

void PickedPosRenderer_Render(struct RayTracer* selected, cc_bool dirty);
#endif
