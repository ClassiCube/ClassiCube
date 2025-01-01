#ifndef CC_DRAWER_H
#define CC_DRAWER_H
#include "PackedCol.h"
#include "Vectors.h"
CC_BEGIN_HEADER

/* 
Draws the vertices for a cuboid region
Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct VertexTextured;

CC_VAR extern struct _DrawerData {
	/* Whether a colour tinting effect should be applied to all faces. */
	cc_bool Tinted;
	/* The colour to multiply colour of faces by (tinting effect). */
	PackedCol TintCol;
	/* Minimum corner of base block bounding box. (For texture UV) */
	Vec3 MinBB;
	/* Maximum corner of base block bounding box. (For texture UV) */
	Vec3 MaxBB;
	/* Coordinate of minimum block bounding box corner in the world. */
	float X1, Y1, Z1;
	/* Coordinate of maximum block bounding box corner in the world. */
	float X2, Y2, Z2;
} Drawer;

/* Draws minimum X face of the cuboid. (i.e. at X1) */
CC_API void Drawer_XMin(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices);
/* Draws maximum X face of the cuboid. (i.e. at X2) */
CC_API void Drawer_XMax(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices);
/* Draws minimum Z face of the cuboid. (i.e. at Z1) */
CC_API void Drawer_ZMin(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices);
/* Draws maximum Z face of the cuboid. (i.e. at Z2) */
CC_API void Drawer_ZMax(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices);
/* Draws minimum Y face of the cuboid. (i.e. at Y1) */
CC_API void Drawer_YMin(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices);
/* Draws maximum Y face of the cuboid. (i.e. at Y2) */
CC_API void Drawer_YMax(int count, PackedCol col, TextureLoc texLoc, struct VertexTextured** vertices);

CC_END_HEADER
#endif
