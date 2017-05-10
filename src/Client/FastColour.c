#include "FastColour.h"

FastColour FastColour_Create4(UInt8 r, UInt8 g, UInt8 b, UInt8 a) {
	FastColour col;
	col.R = r; col.G = g; col.B = b; col.A = a;
	return col;
}

FastColour FastColour_Create3(UInt8 r, UInt8 g, UInt8 b) {
	FastColour col;
	col.R = r; col.G = g; col.B = b; col.A = 255;
	return col;
}

bool FastColour_Equals(FastColour a, FastColour b) {
	return a.Packed == b.Packed;
}

FastColour FastColour_Scale(FastColour value, Real32 t) {
	value.R = (UInt8)(value.R * t);
	value.G = (UInt8)(value.G * t);
	value.B = (UInt8)(value.B * t);
	return value;
}