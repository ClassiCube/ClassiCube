#ifndef FASTCOLOUR_H
#define FASTCOLOUR_H
#include "Typedefs.h"
/* Manipulates an ARGB colour, in a format suitable for the native graphics api.
Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

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
FastColour FastColour_Create4(UInt8 r, UInt8 g, UInt8 b, UInt8 a);

/* Constructs a new ARGB colour. */
FastColour FastColour_Create3(UInt8 r, UInt8 g, UInt8 b);

/* Multiplies the RGB components by t, where t is in [0, 1] */
FastColour FastColour_Scale(FastColour value, Real32 t);

// TODO: actual constant values? may need to rethink FastColour
#define FastColour_White FastColour_Create3(255, 255, 255)
#endif