#include "PackedCol.h"
#include "ExtMath.h"

PackedCol PackedCol_Create4(UInt8 r, UInt8 g, UInt8 b, UInt8 a) {
	PackedCol col;
	col.R = r; col.G = g; col.B = b; col.A = a;
	return col;
}

PackedCol PackedCol_Create3(UInt8 r, UInt8 g, UInt8 b) {
	PackedCol col;
	col.R = r; col.G = g; col.B = b; col.A = 255;
	return col;
}

UInt32 PackedCol_ToARGB(PackedCol col) {
	return PackedCol_ARGB(col.R, col.G, col.B, col.A);
}

PackedCol PackedCol_Scale(PackedCol value, Real32 t) {
	value.R = (UInt8)(value.R * t);
	value.G = (UInt8)(value.G * t);
	value.B = (UInt8)(value.B * t);
	return value;
}

PackedCol PackedCol_Lerp(PackedCol a, PackedCol b, Real32 t) {
	a.R = (UInt8)Math_Lerp(a.R, b.R, t);
	a.G = (UInt8)Math_Lerp(a.G, b.G, t);
	a.B = (UInt8)Math_Lerp(a.B, b.B, t);
	return a;
}

void PackedCol_GetShaded(PackedCol normal, PackedCol* xSide, PackedCol* zSide, PackedCol* yBottom) {
	*xSide = PackedCol_Scale(normal, PackedCol_ShadeX);
	*zSide = PackedCol_Scale(normal, PackedCol_ShadeZ);
	*yBottom = PackedCol_Scale(normal, PackedCol_ShadeYBottom);
}