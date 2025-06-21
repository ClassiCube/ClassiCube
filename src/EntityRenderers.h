#ifndef CC_ENTITYRENDERERS_H
#define CC_ENTITYRENDERERS_H
#include "Core.h"
CC_BEGIN_HEADER

/* Renders supporting objects for entities (shadows and names)
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent EntityRenderers_Component;
struct Entity;

/* Draws shadows under entities, depending on Entities.ShadowsMode */
void EntityShadows_Render(void);

/* Deletes the texture containing the entity's nametag */
void EntityNames_Delete(struct Entity* e);
/* Renders the name tags of entities, depending on Entities.NamesMode */
void EntityNames_Render(void);
/* Renders hovered entity name tags (these appears through blocks) */
void EntityNames_RenderHovered(void);

CC_END_HEADER
#endif
