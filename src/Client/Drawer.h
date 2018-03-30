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
PackedCol Drawer_TintColour;
/* Minimum base block bounding box corner. (For texture UV) */
Vector3 Drawer_MinBB;
/* Maximum base block bounding box corner. (For texture UV) */
Vector3 Drawer_MaxBB;
/* Coordinate of minimum block bounding box corner in the world. */
Real32 Drawer_X1, Drawer_Y1, Drawer_Z1;
/* Coordinate of maximum block bounding box corner in the world. */
Real32 Drawer_X2, Drawer_Y2, Drawer_Z2;

/* Draws the left face of the given cuboid region. */
void Drawer_XMin(Int32 count, PackedCol col, TextureLoc texLoc, VertexP3fT2fC4b** vertices);
/* Draws the right face of the given cuboid region. */
void Drawer_XMax(Int32 count, PackedCol col, TextureLoc texLoc, VertexP3fT2fC4b** vertices);
/* Draws the front face of the given cuboid region. */
void Drawer_ZMin(Int32 count, PackedCol col, TextureLoc texLoc, VertexP3fT2fC4b** vertices);
/* Draws the back face of the given cuboid region. */
void Drawer_ZMax(Int32 count, PackedCol col, TextureLoc texLoc, VertexP3fT2fC4b** vertices);
/* Draws the bottom face of the given cuboid region. */
void Drawer_YMin(Int32 count, PackedCol col, TextureLoc texLoc, VertexP3fT2fC4b** vertices);
/* Draws the top face of the given cuboid region. */
void Drawer_YMax(Int32 count, PackedCol col, TextureLoc texLoc, VertexP3fT2fC4b** vertices);
#endif