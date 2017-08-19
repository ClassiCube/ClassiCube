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

/* Stores a point in 2D space. */
typedef struct Size2D_ {
	Int32 Width, Height;
} Size2D;


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
#endif