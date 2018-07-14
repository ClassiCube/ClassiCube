#ifndef CC_PICKEDPOSRENDERER_H
#define CC_PICKEDPOSRENDERER_H
#include "GameStructs.h"
/* Renders an outline around the block the player is looking at.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
typedef struct PickedPos_ PickedPos;

IGameComponent PickedPosRenderer_MakeComponent(void);
void PickedPosRenderer_Render(Real64 delta);
void PickedPosRenderer_Update(PickedPos* selected);
#endif
