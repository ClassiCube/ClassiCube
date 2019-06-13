#ifndef CC_SELECTIONBOX_H
#define CC_SELECTIONBOX_H
#include "VertexStructs.h"
#include "Vectors.h"
/* Describes a selection box, and contains methods related to the selection box.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent Selections_Component;

void Selections_Render(double delta);
void Selections_Add(uint8_t id, IVec3 p1, IVec3 p2, PackedCol col);
void Selections_Remove(uint8_t id);
#endif
