#ifndef CC_PICKEDPOSRENDERER_H
#define CC_PICKEDPOSRENDERER_H
#include "GameStructs.h"
#include "Picking.h"
#include "Vectors.h"
/* Renders an outline around the block the player is looking at.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Creates game component implementation. */
IGameComponent PickedPosRenderer_MakeGameComponent(void);
/* Renders outline around block player is looking at, if they are looking at a block. */
void PickedPosRenderer_Render(Real64 delta);
/* Updates state for the outlined block. */
void PickedPosRenderer_UpdateState(PickedPos* selected);
#endif