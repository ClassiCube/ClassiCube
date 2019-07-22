#ifndef CC_PACKEDCOL_H
#define CC_PACKEDCOL_H
#include "String.h"
/* Manipulates an ARGB colour, in a format suitable for the native 3D graphics API.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/

/* Represents an ARGB colour, suitable for native graphics API colours. */
typedef union PackedCol_ {
#ifdef CC_BUILD_D3D9
	 struct { uint8_t B, G, R, A; };
#else
	 struct { uint8_t R, G, B, A; };
#endif
	 uint32_t _raw;
} PackedCol;

#ifdef CC_BUILD_D3D9
#define PACKEDCOL_CONST(r, g, b, a) { b, g, r, a }
#else
#define PACKEDCOL_CONST(r, g, b, a) { r, g, b, a }
#endif
#define PACKEDCOL_WHITE PACKEDCOL_CONST(255, 255, 255, 255)
#define PackedCol_ARGB(r, g, b, a) (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | ((uint32_t)(b)) | ((uint32_t)(a) << 24))

/* Whether components of two colours are all equal. */
#define PackedCol_Equals(a,b) ((a)._raw == (b)._raw)
/* Scales RGB components of the given colour. */
CC_API PackedCol PackedCol_Scale(PackedCol value, float t);
/* Linearly interpolates RGB components of the two given colours. */
CC_API PackedCol PackedCol_Lerp(PackedCol a, PackedCol b, float t);

CC_NOINLINE bool PackedCol_Unhex(const char* src, int* dst, int count);
CC_NOINLINE void PackedCol_ToHex(String* str, PackedCol value);
CC_NOINLINE bool PackedCol_TryParseHex(const String* str, PackedCol* value);

#define PACKEDCOL_SHADE_X 0.6f
#define PACKEDCOL_SHADE_Z 0.8f
#define PACKEDCOL_SHADE_YMIN 0.5f
/* Retrieves shaded colours for ambient block face lighting */
void PackedCol_GetShaded(PackedCol normal, PackedCol* xSide, PackedCol* zSide, PackedCol* yMin);
#endif
