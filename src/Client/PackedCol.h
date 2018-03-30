#ifndef CC_PACKEDCOL_H
#define CC_PACKEDCOL_H
#include "String.h"
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

#if USE_DX
#define PACKEDCOL_CONST(r, g, b, a) { b, g, r, a };
#else
#define PACKEDCOL_CONST(r, g, b, a) { r, g, b, a };
#endif

PackedCol PackedCol_Create4(UInt8 r, UInt8 g, UInt8 b, UInt8 a);
PackedCol PackedCol_Create3(UInt8 r, UInt8 g, UInt8 b);
#define PackedCol_Equals(a, b) ((a).Packed == (b).Packed)
#define PackedCol_ARGB(r, g, b, a) (((UInt32)(r) << 16) | ((UInt32)(g) << 8) | ((UInt32)(b)) | ((UInt32)(a) << 24))
UInt32 PackedCol_ToARGB(PackedCol col);
PackedCol PackedCol_Scale(PackedCol value, Real32 t);
PackedCol PackedCol_Lerp(PackedCol a, PackedCol b, Real32 t);
bool PackedCol_TryParseHex(STRING_PURE String* str, PackedCol* value);

#define PACKEDCOL_SHADE_X 0.6f
#define PACKEDCOL_SHADE_Z 0.8f
#define PACKEDCOL_SHADE_YMIN 0.5f
/* Retrieves shaded colours for ambient block face lighting. */
void PackedCol_GetShaded(PackedCol normal, PackedCol* xSide, PackedCol* zSide, PackedCol* yMin);

#define PACKEDCOL_WHITE   PACKEDCOL_CONST(255, 255, 255, 255)
#define PACKEDCOL_BLACK   PACKEDCOL_CONST(  0,   0,   0, 255)
#define PACKEDCOL_RED     PACKEDCOL_CONST(255,   0,   0, 255)
#define PACKEDCOL_GREEN   PACKEDCOL_CONST(  0, 255,   0, 255)
#define PACKEDCOL_BLUE    PACKEDCOL_CONST(  0,   0, 255, 255)
#define PACKEDCOL_YELLOW  PACKEDCOL_CONST(255, 255,   0, 255)
#define PACKEDCOL_MAGENTA PACKEDCOL_CONST(255,   0, 255, 255)
#define PACKEDCOL_CYAN    PACKEDCOL_CONST(  0, 255, 255, 255)
#endif