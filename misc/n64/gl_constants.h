#ifndef __GL_CONSTANTS
#define __GL_CONSTANTS

#define VERTEX_CACHE_SIZE     4

#define MATRIX_SIZE           64

#define TEXTURE_BILINEAR_MASK       0x001
#define TEXTURE_INTERPOLATE_MASK    0x002
#define TEXTURE_MIPMAP_MASK         0x100

#define VTX_SHIFT 5
#define TEX_SHIFT 8

#define GUARD_BAND_FACTOR 2

#define ASSERT_INVALID_VTX_ID   0x2001

#define TEX_COORD_SHIFT             6
#define HALF_TEXEL                  0x0010

#define TEX_BILINEAR_SHIFT          13
#define TEX_BILINEAR_OFFSET_SHIFT   4

#define BILINEAR_TEX_OFFSET_SHIFT   9

//0-39 same as screenvtx
#define PRIM_VTX_TRCODE            40    // trivial-reject clipping flags (against -w/+w)
#define PRIM_VTX_SIZE              42


#endif
