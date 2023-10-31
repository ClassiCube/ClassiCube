#ifndef CC_SELECTIONBOX_H
#define CC_SELECTIONBOX_H
#include "Vectors.h"
#include "PackedCol.h"
/* Describes a selection box, and contains methods related to the selection box.
   Copyright 2014-2023 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent Selections_Component;

void Selections_Render(void);
/* Adds or replaces the selection box with the given ID */
CC_API void Selections_Add(cc_uint8 id, const IVec3* p1, const IVec3* p2, PackedCol color);
/* Removes the selection box with the givne ID */
CC_API void Selections_Remove(cc_uint8 id);
#endif
