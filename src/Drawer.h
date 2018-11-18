#ifndef CC_DRAWER_H
#define CC_DRAWER_H
#include "VertexStructs.h"
#include "Vectors.h"
/* Draws the vertices for a cuboid region.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Whether a colour tinting effect should be applied to all faces. */
bool Drawer_Tinted;
/* The colour to multiply colour of faces by (tinting effect). */
PackedCol Drawer_TintCol;
/* Minimum corner of base block bounding box. (For texture UV) */
Vector3 Drawer_MinBB;
/* Maximum corner of base block bounding box. (For texture UV) */
Vector3 Drawer_MaxBB;
/* Coordinate of minimum block bounding box corner in the world. */
float Drawer_X1, Drawer_Y1, Drawer_Z1;
/* Coordinate of maximum block bounding box corner in the world. */
float Drawer_X2, Drawer_Y2, Drawer_Z2;

void Drawer_XMin(int count, PackedCol col, TextureLoc texLoc, VertexP3fT2fC4b** vertices);
void Drawer_XMax(int count, PackedCol col, TextureLoc texLoc, VertexP3fT2fC4b** vertices);
void Drawer_ZMin(int count, PackedCol col, TextureLoc texLoc, VertexP3fT2fC4b** vertices);
void Drawer_ZMax(int count, PackedCol col, TextureLoc texLoc, VertexP3fT2fC4b** vertices);
void Drawer_YMin(int count, PackedCol col, TextureLoc texLoc, VertexP3fT2fC4b** vertices);
void Drawer_YMax(int count, PackedCol col, TextureLoc texLoc, VertexP3fT2fC4b** vertices);
#endif
