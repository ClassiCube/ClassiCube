#ifndef FASTCOLOUR_H
#define FASTCOLOUR_H
#include "Typedefs.h"

/* Represents an ARGB colour, in a format suitable for the native graphics api. */
typedef struct FastColour {
	union {
#if USE_DX
		struct { UInt8 B; UInt8 G; UInt8 R; UInt8 A; };
#else
		struct { UInt8 R; UInt8 G; UInt8 B; UInt8 A; };
#endif
		UInt32 Packed;
	};
} FastColour;

/* Constructs a new ARGB colour. */
FastColour FastColour_Create4(UInt8 r, UInt8 g, UInt8 b, UInt8 a) {
	FastColour col;
	col.R = r; col.G = g; col.B = b; col.A = a;
	return col;
}

/* Constructs a new ARGB colour. */
FastColour FastColour_Create3(UInt8 r, UInt8 g, UInt8 b) {
	FastColour col;
	col.R = r; col.G = g; col.B = b; col.A = 255;
	return col;
}

/* Multiplies the RGB components by t, where t is in [0, 1] */
FastColour FastColour_Scale(FastColour value, Real32 t) {
	value.R = (UInt8)(value.R * t);
	value.G = (UInt8)(value.G * t);
	value.B = (UInt8)(value.B * t);
	return value;
}
#endif