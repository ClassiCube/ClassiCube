#ifndef CC_2DSTRUCTS_H
#define CC_2DSTRUCTS_H
#include "Typedefs.h"
/* Represents simple structures useful for 2D operations.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

/* Stores location and dimensions of a 2D rectangle. */
typedef struct Rectangle2D_ { Int32 X, Y, Width, Height; } Rectangle2D;
Rectangle2D Rectangle2D_Empty;

/* Stores a coordinate in 2D space.*/
typedef struct Point2D_ { Int32 X, Y; } Point2D;
Point2D Point2D_Empty;

/* Stores a point in 2D space. */
typedef struct Size2D_ { Int32 Width, Height; } Size2D;
Size2D Size2D_Empty;

Rectangle2D Rectangle2D_Make(Int32 x, Int32 y, Int32 width, Int32 height);
bool Rectangle2D_Contains(Rectangle2D a, Int32 x, Int32 y);
bool Rectangle2D_Equals(Rectangle2D a, Rectangle2D b);

Size2D Size2D_Make(Int32 width, Int32 height);
Point2D Point2D_Make(Int32 x, Int32 y);

typedef struct TextureRec_ { Real32 U1, V1, U2, V2; } TextureRec;
#endif