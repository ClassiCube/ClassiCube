#ifndef CC_PACKEDCOL_H
#define CC_PACKEDCOL_H
#include "String.h"
/* Manipulates an ARGB colour, in a format suitable for the native 3D graphics API.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Represents an ARGB colour, suitable for native graphics API colours. */
 typedef CC_ALIGN_HINT(4) struct PackedCol_ {
#ifdef CC_BUILD_D3D9
	uint8_t B, G, R, A;
#else
	uint8_t R, G, B, A;
#endif
} PackedCol;

/* Represents an ARGB colour, suitable for native graphics API colours. */
/* Unioned with Packed member for efficient equality comparison */
typedef union PackedColUnion_ { PackedCol C; uint32_t Raw; } PackedColUnion;

#ifdef CC_BUILD_D3D9
#define PACKEDCOL_CONST(r, g, b, a) { b, g, r, a }
#else
#define PACKEDCOL_CONST(r, g, b, a) { r, g, b, a }
#endif
#define PACKEDCOL_WHITE PACKEDCOL_CONST(255, 255, 255, 255)

PackedCol PackedCol_Create4(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
PackedCol PackedCol_Create3(uint8_t r, uint8_t g, uint8_t b);
bool PackedCol_Equals(PackedCol a, PackedCol b);

#define PackedCol_ARGB(r, g, b, a) (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | ((uint32_t)(b)) | ((uint32_t)(a) << 24))
PackedCol PackedCol_Scale(PackedCol value, float t);
PackedCol PackedCol_Lerp(PackedCol a, PackedCol b, float t);
CC_NOINLINE bool PackedCol_Unhex(char hex, int* value);
CC_NOINLINE void PackedCol_ToHex(String* str, PackedCol value);
CC_NOINLINE bool PackedCol_TryParseHex(const String* str, PackedCol* value);

#define PACKEDCOL_SHADE_X 0.6f
#define PACKEDCOL_SHADE_Z 0.8f
#define PACKEDCOL_SHADE_YMIN 0.5f
/* Retrieves shaded colours for ambient block face lighting */
void PackedCol_GetShaded(PackedCol normal, PackedCol* xSide, PackedCol* zSide, PackedCol* yMin);
#endif
