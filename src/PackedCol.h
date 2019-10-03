#ifndef CC_PACKEDCOL_H
#define CC_PACKEDCOL_H
#include "String.h"
/* Manipulates an ARGB colour, in a format suitable for the native 3D graphics API.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/

/* Represents an ARGB colour, suitable for native graphics API colours. */
typedef union PackedCol_ {
#ifdef CC_BUILD_D3D9
	 struct { cc_uint8 B, G, R, A; };
#else
	 struct { cc_uint8 R, G, B, A; };
#endif
	 cc_uint32 _raw;
} PackedCol;

#ifdef CC_BUILD_D3D9
#define PACKEDCOL_CONST(r, g, b, a) { b, g, r, a }
#else
#define PACKEDCOL_CONST(r, g, b, a) { r, g, b, a }
#endif
#define PACKEDCOL_WHITE PACKEDCOL_CONST(255, 255, 255, 255)
#define PackedCol_ARGB(r, g, b, a) (((cc_uint32)(r) << 16) | ((cc_uint32)(g) << 8) | ((cc_uint32)(b)) | ((cc_uint32)(a) << 24))

/* Whether components of two colours are all equal. */
#define PackedCol_Equals(a,b) ((a)._raw == (b)._raw)
/* Scales RGB components of the given colour. */
CC_API PackedCol PackedCol_Scale(PackedCol value, float t);
/* Linearly interpolates RGB components of the two given colours. */
CC_API PackedCol PackedCol_Lerp(PackedCol a, PackedCol b, float t);
/* Multiplies RGB components of the two given colours. */
CC_API PackedCol PackedCol_Tint(PackedCol a, PackedCol b);

CC_NOINLINE bool PackedCol_Unhex(const char* src, int* dst, int count);
CC_NOINLINE void PackedCol_ToHex(String* str, PackedCol value);
CC_NOINLINE bool PackedCol_TryParseHex(const String* str, PackedCol* value);

#define PACKEDCOL_SHADE_X 0.6f
#define PACKEDCOL_SHADE_Z 0.8f
#define PACKEDCOL_SHADE_YMIN 0.5f
/* Retrieves shaded colours for ambient block face lighting */
void PackedCol_GetShaded(PackedCol normal, PackedCol* xSide, PackedCol* zSide, PackedCol* yMin);
#endif
