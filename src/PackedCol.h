#ifndef CC_PACKEDCOL_H
#define CC_PACKEDCOL_H
#include "Core.h"
/* Manipulates a packed 32 bit RGBA colour, in a format suitable for the native 3D graphics API.
   Copyright 2014-2023 ClassiCube | Licensed under BSD-3
*/

typedef cc_uint32 PackedCol;
#if defined CC_BUILD_D3D9 || defined CC_BUILD_XBOX || defined CC_BUILD_DREAMCAST
	#define PACKEDCOL_B_SHIFT  0
	#define PACKEDCOL_G_SHIFT  8
	#define PACKEDCOL_R_SHIFT 16
	#define PACKEDCOL_A_SHIFT 24
#elif defined CC_BIG_ENDIAN
	#define PACKEDCOL_R_SHIFT 24
	#define PACKEDCOL_G_SHIFT 16
	#define PACKEDCOL_B_SHIFT  8
	#define PACKEDCOL_A_SHIFT  0
#else
	#define PACKEDCOL_R_SHIFT  0
	#define PACKEDCOL_G_SHIFT  8
	#define PACKEDCOL_B_SHIFT 16
	#define PACKEDCOL_A_SHIFT 24
#endif

#define PACKEDCOL_R_MASK (0xFFU << PACKEDCOL_R_SHIFT)
#define PACKEDCOL_G_MASK (0xFFU << PACKEDCOL_G_SHIFT)
#define PACKEDCOL_B_MASK (0xFFU << PACKEDCOL_B_SHIFT)
#define PACKEDCOL_A_MASK (0xFFU << PACKEDCOL_A_SHIFT)

#define PackedCol_R(col) ((cc_uint8)(col >> PACKEDCOL_R_SHIFT))
#define PackedCol_G(col) ((cc_uint8)(col >> PACKEDCOL_G_SHIFT))
#define PackedCol_B(col) ((cc_uint8)(col >> PACKEDCOL_B_SHIFT))
#define PackedCol_A(col) ((cc_uint8)(col >> PACKEDCOL_A_SHIFT))

#define PackedCol_R_Bits(col) ((cc_uint8)(col) << PACKEDCOL_R_SHIFT)
#define PackedCol_G_Bits(col) ((cc_uint8)(col) << PACKEDCOL_G_SHIFT)
#define PackedCol_B_Bits(col) ((cc_uint8)(col) << PACKEDCOL_B_SHIFT)
#define PackedCol_A_Bits(col) ((cc_uint8)(col) << PACKEDCOL_A_SHIFT)

#define PackedCol_Make(r, g, b, a) (PackedCol_R_Bits(r) | PackedCol_G_Bits(g) | PackedCol_B_Bits(b) | PackedCol_A_Bits(a))
#define PACKEDCOL_WHITE PackedCol_Make(255, 255, 255, 255)
#define PACKEDCOL_RGB_MASK (PACKEDCOL_R_MASK | PACKEDCOL_G_MASK | PACKEDCOL_B_MASK)

/* Scales RGB components of the given colour. */
CC_API PackedCol PackedCol_Scale(PackedCol value, float t);
/* Linearly interpolates RGB components of the two given colours. */
CC_API PackedCol PackedCol_Lerp(PackedCol a, PackedCol b, float t);
/* Multiplies RGB components of the two given colours. */
CC_API PackedCol PackedCol_Tint(PackedCol a, PackedCol b);

CC_NOINLINE int PackedCol_DeHex(char hex);
CC_NOINLINE cc_bool PackedCol_Unhex(const char* src, int* dst, int count);
CC_NOINLINE void PackedCol_ToHex(cc_string* str, PackedCol value);
CC_NOINLINE cc_bool PackedCol_TryParseHex(const cc_string* str, cc_uint8* rgb);

#define PACKEDCOL_SHADE_X 0.6f
#define PACKEDCOL_SHADE_Z 0.8f
#define PACKEDCOL_SHADE_YMIN 0.5f
/* Retrieves shaded colours for ambient block face lighting */
void PackedCol_GetShaded(PackedCol normal, PackedCol* xSide, PackedCol* zSide, PackedCol* yMin);
#endif
