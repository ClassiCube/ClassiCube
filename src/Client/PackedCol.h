#ifndef CS_PACKEDCOL_H
#define CS_PACKEDCOL_H
#include "Typedefs.h"
/* Manipulates an ARGB colour, in a format suitable for the native 3d graphics api.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Represents an ARGB colour, in a format suitable for the native graphics api. */
typedef struct PackedCol_ {
	union {
#if USE_DX
		struct { UInt8 B; UInt8 G; UInt8 R; UInt8 A; };
#else
		struct { UInt8 R; UInt8 G; UInt8 B; UInt8 A; };
#endif
		UInt32 Packed;
	};
} PackedCol;


/* Constructs a new ARGB colour. */
PackedCol PackedCol_Create4(UInt8 r, UInt8 g, UInt8 b, UInt8 a);

/* Constructs a new ARGB colour. */
PackedCol PackedCol_Create3(UInt8 r, UInt8 g, UInt8 b);

/* Returns whether two packed colours are equal. */
#define PackedCol_Equals(a, b) (a.Packed == b.Packed)

/* Multiplies the RGB components by t, where t is in [0, 1] */
PackedCol PackedCol_Scale(PackedCol value, Real32 t);

/* Linearly interpolates the RGB components of both colours by t, where t is in [0, 1] */
PackedCol PackedCol_Lerp(PackedCol a, PackedCol b, Real32 t);


#define PackedCol_ShadeX 0.6f
#define PackedCol_ShadeZ 0.8f
#define PackedCol_ShadeYBottom 0.5f

/* Retrieves shaded colours for ambient block face lighting. */
void PackedCol_GetShaded(PackedCol normal, PackedCol* xSide, PackedCol* zSide, PackedCol* yBottom);


/* TODO: actual constant values? may need to rethink PackedCol */
#define PackedCol_White PackedCol_Create3(255, 255, 255)
#define PackedCol_Black PackedCol_Create3(  0,   0,   0)
#define PackedCol_Red   PackedCol_Create3(255,   0,   0)
#define PackedCol_Green PackedCol_Create3(  0, 255,   0)
#define PackedCol_Blue  PackedCol_Create3(  0,   0, 255)
#endif