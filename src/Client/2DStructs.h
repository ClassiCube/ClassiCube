#ifndef CS_2DSTRUCTS_H
#define CS_2DSTRUCTS_H
#include "Typedefs.h"
/* Represents simple structures useful for 2D operations.
   Copyright 22017 ClassicalSharp | Licensed under BSD-3
*/

/* Stores location and dimensions of a 2D rectangle. */
typedef struct Rectangle_ {
	/* Coordinates of top left corner of rectangle.*/
	Int32 X, Y;
	/* Size of rectangle. */
	Int32 Width, Height;
} Rectangle;


/* Creates a new rectangle. */
Rectangle Rectangle_Make(Int32 x, Int32 y, Int32 width, Int32 height);

/* Returns whether the given rectangle contains the given point. */
bool Rectangle_Contains(Rectangle a, Int32 x, Int32 y);
#endif