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

bool Size2D_Equals(Size2D a, Size2D b) {
	return a.Width == b.Width && a.Height == b.Height;
}

Point2D Point2D_Make(Int32 x, Int32 y) {
	Point2D p; p.X = x; p.Y = y; return p;
}

bool Point2D_Equals(Point2D a, Point2D b) {
	return a.X == b.X && a.Y == b.Y;
}

TextureRec TextureRec_FromRegion(Real32 u, Real32 v, Real32 uWidth, Real32 vHeight) {
	TextureRec rec;
	rec.U1 = u; rec.V1 = v;
	rec.U2 = u + uWidth; rec.V2 = v + vHeight;
	return rec;
}

TextureRec TextureRec_FromPoints(Real32 u1, Real32 v1, Real32 u2, Real32 v2) {
	TextureRec rec;
	rec.U1 = u1; rec.V1 = v1;
	rec.U2 = u2; rec.V2 = v2;
	return rec;
}

void TextureRec_SwapU(TextureRec* rec) {
	Real32 u2 = rec->U2; rec->U2 = rec->U1; rec->U1 = u2;
}