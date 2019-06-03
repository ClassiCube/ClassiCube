#ifndef CC_PICKEDPOSRENDERER_H
#define CC_PICKEDPOSRENDERER_H
#include "Core.h"
/* Renders an outline around the block the player is looking at.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/
struct PickedPos;
struct IGameComponent;
extern struct IGameComponent PickedPosRenderer_Component;

void PickedPosRenderer_Render(double delta);
void PickedPosRenderer_Update(struct PickedPos* selected);
#endif
