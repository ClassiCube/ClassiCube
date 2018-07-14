#ifndef CC_2DSTRUCTS_H
#define CC_2DSTRUCTS_H
#include "Typedefs.h"
/* Represents simple structures useful for 2D operations.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

struct Rectangle2D { Int32 X, Y, Width, Height; };
struct Rectangle2D Rectangle2D_Empty;

struct Point2D { Int32 X, Y; };
struct Point2D Point2D_Empty;

struct Size2D { Int32 Width, Height; };
struct Size2D Size2D_Empty;

struct Rectangle2D Rectangle2D_Make(Int32 x, Int32 y, Int32 width, Int32 height);
bool Rectangle2D_Contains(struct Rectangle2D a, Int32 x, Int32 y);
bool Rectangle2D_Equals(struct Rectangle2D a, struct Rectangle2D b);

struct Size2D Size2D_Make(Int32 width, Int32 height);
struct Point2D Point2D_Make(Int32 x, Int32 y);

struct TextureRec { Real32 U1, V1, U2, V2; };
#endif
