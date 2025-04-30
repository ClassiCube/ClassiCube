#ifndef __GL_CONSTANTS
#define __GL_CONSTANTS

#define VERTEX_CACHE_SIZE     16

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

#define TRICMD_ATTR_MASK        0x300

#define PRIM_VTX_CS_POSi           0     // X, Y, Z, W (all 32-bit)
#define PRIM_VTX_CS_POSf           8     // X, Y, Z, W (all 32-bit)
#define PRIM_VTX_X                 16    // Object space position (16-bit)
#define PRIM_VTX_Y                 18    // Object space position (16-bit)
#define PRIM_VTX_Z                 20    // Object space position (16-bit)
#define PRIM_VTX_W                 22    // Object space position (16-bit)
#define PRIM_VTX_R                 24
#define PRIM_VTX_G                 26
#define PRIM_VTX_B                 28
#define PRIM_VTX_A                 30
#define PRIM_VTX_TEX_S             32
#define PRIM_VTX_TEX_T             34
#define PRIM_VTX_TEX_R             36
#define PRIM_VTX_TEX_Q             38
#define PRIM_VTX_TRCODE            40    // trivial-reject clipping flags (against -w/+w)
#define PRIM_VTX_SIZE              42


#endif
