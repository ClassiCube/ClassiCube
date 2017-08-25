#ifndef CS_2DSTRUCTS_H
#define CS_2DSTRUCTS_H
#include "Typedefs.h"
/* Represents simple structures useful for 2D operations.
   Copyright 22017 ClassicalSharp | Licensed under BSD-3
*/

/* Stores location and dimensions of a 2D rectangle. */
typedef struct Rectangle2D_ {
	/* Coordinates of top left corner of rectangle.*/
	Int32 X, Y;
	/* Size of rectangle. */
	Int32 Width, Height;
} Rectangle2D;
/* Empty 2D rectangle. */
Rectangle2D Rectangle2D_Empty;

/* Stores a coordinate in 2D space.*/
typedef struct Point2D_ {
	Int32 X, Y;
} Point2D;
/* Point at X = 0 and Y = 0 */
Point2D Point2D_Empty;

/* Stores a point in 2D space. */
typedef struct Size2D_ {
	Int32 Width, Height;
} Size2D;
/* Size with Width = 0 and Height = 0 */
Size2D Size2D_Empty;

/* Creates a new rectangle. */
Rectangle2D Rectangle2D_Make(Int32 x, Int32 y, Int32 width, Int32 height);

/* Returns whether the given rectangle contains the given point. */
bool Rectangle2D_Contains(Rectangle2D a, Int32 x, Int32 y);

/* Returns whether the two rectangles are equal. */
bool Rectangle2D_Equals(Rectangle2D a, Rectangle2D b);

/* Creates a new size. */
Size2D Size2D_Make(Int32 width, Int32 height);

/* Returns whether the two sizes are equal. */
bool Size2D_Equals(Size2D a, Size2D b);

/* Creates a new point. */
Point2D Point2D_Make(Int32 x, Int32 y);

/* Returns whether the two points are equal. */
bool Point2D_Equals(Point2D a, Point2D b);


/* Stores the four texture coordinates that describe a textured quad.  */
typedef struct TextureRec_ {
	Real32 U1, V1, U2, V2;
} TextureRec;


/* Constructs a texture rectangle from origin and size. */
TextureRec TextureRec_FromRegion(Real32 u, Real32 v, Real32 uWidth, Real32 vHeight);

/* Constructs a texture rectangle from four points. */
TextureRec TextureRec_FromPoints(Real32 u1, Real32 v1, Real32 u2, Real32 v2);

/* Swaps U1 and U2 of a texture rectangle. */
void TextureRec_SwapU(TextureRec* rec);
#endif