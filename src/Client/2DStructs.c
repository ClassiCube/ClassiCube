#include "2DStructs.h"

Rectangle2D Rectangle2D_Make(Int32 x, Int32 y, Int32 width, Int32 height) {
	Rectangle2D r;
	r.X = x; r.Y = y; r.Width = width; r.Height = height;
	return r;
}

bool Rectangle2D_Contains(Rectangle2D a, Int32 x, Int32 y) {
	return x >= a.X && y >= a.Y && x < (a.X + a.Width) && y < (a.Y + a.Height);
}