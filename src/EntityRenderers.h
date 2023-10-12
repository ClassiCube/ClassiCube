#ifndef CC_ENTITYRENDERERS_H
#define CC_ENTITYRENDERERS_H
#include "Core.h"
/* Renders supporting objects for entities (shadows)
   Copyright 2014-2023 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent EntityRenderers_Component;

/* Draws shadows under entities, depending on Entities.ShadowsMode */
void EntityShadows_Render(void);

#endif
