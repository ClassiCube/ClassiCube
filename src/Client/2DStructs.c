#include "2DStructs.h"

Rectangle Rectangle_Make(Int32 x, Int32 y, Int32 width, Int32 height) {
	Rectangle r;
	r.X = x; r.Y = y; r.Width = width; r.Height = height;
	return r;
}

bool Rectangle_Contains(Rectangle a, Int32 x, Int32 y) {
	return x >= a.X && y >= a.Y && x < (a.X + a.Width) && y < (a.Y + a.Height);
}