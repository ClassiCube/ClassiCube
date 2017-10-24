#ifndef CC_SELECTIONBOX_H
#define CC_SELECTIONBOX_H
#include "Typedefs.h"
#include "PackedCol.h"
#include "Vectors.h"
#include "VertexStructs.h"
/* Describes a selection box, and contains methods related to the selection box.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Data for a selection box. */
typedef struct SelectionBox_ {
	/* ID of this selection box.*/
	UInt8 ID;
	/* Minimum corner of the box. */
	Vector3I Min;
	/* Maximum corner of the box. */
	Vector3I Max;
	/* Colour of this box. */
	PackedCol Col;
	/* Closest distance to player of any of the eight corners of the box. */
	Real32 MinDist;
	/* Furthest distance to player of any of the eight corners of the box. */
	Real32 MaxDist;
} SelectionBox;


/* Constructs a selection box. */
void SelectionBox_Make(SelectionBox* box, Vector3I* p1, Vector3I* p2, PackedCol col);
/* Constructs the vertices and line vertices for a selection box. */
void SelectionBox_Render(SelectionBox* box, VertexP3fC4b** vertices, VertexP3fC4b** lineVertices);

/* Draws a vertical quad. */
void SelectionBox_VerQuad(VertexP3fC4b** vertices, PackedCol col,
	Real32 x1, Real32 y1, Real32 z1, Real32 x2, Real32 y2, Real32 z2);
/* Draws a horizonal quad. */
void SelectionBox_HorQuad(VertexP3fC4b** vertices, PackedCol col,
	Real32 x1, Real32 z1, Real32 x2, Real32 z2, Real32 y);
/* Draws a line between two points. */
void SelectionBox_Line(VertexP3fC4b** vertices, PackedCol col,
	Real32 x1, Real32 y1, Real32 z1, Real32 x2, Real32 y2, Real32 z2);
#endif