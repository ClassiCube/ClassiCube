#include "2DStructs.h"

Rectangle2D Rectangle2D_Make(Int32 x, Int32 y, Int32 width, Int32 height) {
	Rectangle2D r;
	r.X = x; r.Y = y; r.Width = width; r.Height = height;
	return r;
}

bool Rectangle2D_Contains(Rectangle2D a, Int32 x, Int32 y) {
	return x >= a.X && y >= a.Y && x < (a.X + a.Width) && y < (a.Y + a.Height);
}

bool Rectangle2D_Equals(Rectangle2D a, Rectangle2D b) {
	return a.X == b.X && a.Y == b.Y && a.Width == b.Width && a.Height == b.Height;
}

Size2D Size2D_Make(Int32 width, Int32 height) {
	Size2D s; s.Width = width; s.Height = height; return s;
}

Point2D Point2D_Make(Int32 x, Int32 y) {
	Point2D p; p.X = x; p.Y = y; return p;
}