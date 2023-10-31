#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "gl_assert.h"
#include "types.h"

#define MEMSET(dst, v, size) memset((dst), (v), (size))

typedef enum GPUAlpha {
    GPU_ALPHA_DISABLE = 0,
    GPU_ALPHA_ENABLE = 1
} GPUAlpha;

typedef enum GPUTexture {
    GPU_TEXTURE_DISABLE = 0,
    GPU_TEXTURE_ENABLE = 1
} GPUTexture;

typedef enum GPUTextureAlpha {
    GPU_TXRALPHA_DISABLE = 1,
    GPU_TXRALPHA_ENABLE = 0
} GPUTextureAlpha;

typedef enum GPUList {
    GPU_LIST_OP_POLY       = 0,
    GPU_LIST_OP_MOD        = 1,
    GPU_LIST_TR_POLY       = 2,
    GPU_LIST_TR_MOD        = 3,
    GPU_LIST_PT_POLY       = 4
} GPUList;

typedef enum GPUDepthCompare {
    GPU_DEPTHCMP_NEVER      = 0,
    GPU_DEPTHCMP_LESS       = 1,
    GPU_DEPTHCMP_EQUAL      = 2,
    GPU_DEPTHCMP_LEQUAL     = 3,
    GPU_DEPTHCMP_GREATER    = 4,
    GPU_DEPTHCMP_NOTEQUAL   = 5,
    GPU_DEPTHCMP_GEQUAL     = 6,
    GPU_DEPTHCMP_ALWAYS     = 7
} GPUDepthCompare;

typedef enum GPUTextureFormat {
    GPU_TXRFMT_NONE,
    GPU_TXRFMT_VQ_DISABLE = (0 << 30),
    GPU_TXRFMT_VQ_ENABLE = (1 << 30),
    GPU_TXRFMT_ARGB1555 = (0 << 27),
    GPU_TXRFMT_RGB565 = (1 << 27),
    GPU_TXRFMT_ARGB4444 = (2 << 27),
    GPU_TXRFMT_YUV422 = (3 << 27),
    GPU_TXRFMT_BUMP = (4 << 27),
    GPU_TXRFMT_PAL4BPP = (5 << 27),
    GPU_TXRFMT_PAL8BPP = (6 << 27),
    GPU_TXRFMT_TWIDDLED = (0 << 26),
    GPU_TXRFMT_NONTWIDDLED = (1 << 26),
    GPU_TXRFMT_NOSTRIDE = (0 << 21),
    GPU_TXRFMT_STRIDE = (1 << 21)
} GPUTextureFormat;

typedef enum GPUCulling {
    GPU_CULLING_NONE = 0,
    GPU_CULLING_SMALL = 1,
    GPU_CULLING_CCW = 2,
    GPU_CULLING_CW = 3
} GPUCulling;

typedef enum GPUUVFlip {
    GPU_UVFLIP_NONE = 0,
    GPU_UVFLIP_V = 1,
    GPU_UVFLIP_U = 2,
    GPU_UVFLIP_UV = 3
} GPUUVFlip;

typedef enum GPUUVClamp {
    GPU_UVCLAMP_NONE = 0,
    GPU_UVCLAMP_V = 1,
    GPU_UVCLAMP_U = 2,
    GPU_UVCLAMP_UV = 3
} GPUUVClamp;

typedef enum GPUColorClamp {
    GPU_CLRCLAMP_DISABLE = 0,
    GPU_CLRCLAMP_ENABLE = 1
} GPUColorClamp;

typedef enum GPUFilter {
    GPU_FILTER_NEAREST = 0,
    GPU_FILTER_BILINEAR = 2,
    GPU_FILTER_TRILINEAR1 = 4,
    GPU_FILTER_TRILINEAR2 = 6
} GPUFilter;

typedef enum GPUDepthWrite {
    GPU_DEPTHWRITE_ENABLE = 0,
    GPU_DEPTHWRITE_DISABLE = 1
} GPUDepthWrite;

typedef enum GPUUserClip {
    GPU_USERCLIP_DISABLE = 0,
    GPU_USERCLIP_INSIDE = 2,
    GPU_USERCLIP_OUTSIDE = 3
} GPUUserClip;

typedef enum GPUColorFormat {
    GPU_CLRFMT_ARGBPACKED = 0,
    GPU_CLRFMT_4FLOATS = 1,
    GPU_CLRFMT_INTENSITY = 2,
    GPU_CLRFMT_INTENSITY_PREV = 3
} GPUColorFormat;

typedef enum GPUUVFormat {
    GPU_UVFMT_32BIT = 0,
    GPU_UVFMT_16BIT = 1
} GPUUVFormat;

typedef enum GPUFog {
    GPU_FOG_TABLE = 0,
    GPU_FOG_VERTEX = 1,
    GPU_FOG_DISABLE = 2,
    GPU_FOG_TABLE2 = 3
} GPUFog;

typedef enum GPUShade {
    GPU_SHADE_FLAT = 0,
    GPU_SHADE_GOURAUD = 1
} GPUShade;

typedef enum GPUTextureEnv {
    GPU_TXRENV_REPLACE = 0,
    GPU_TXRENV_MODULATE = 1,
    GPU_TXRENV_DECAL = 2,
    GPU_TXRENV_MODULATEALPHA = 3
} GPUTextureEnv;

/* Duplication of pvr_poly_cxt_t from KOS so that we can
 * compile on non-KOS platforms for testing */

typedef struct {
    GPUList     list_type;

    struct {
        int     alpha;
        int     shading;
        int     fog_type;
        int     culling;
        int     color_clamp;
        int     clip_mode;
        int     modifier_mode;
        int     specular;
        int     alpha2;
        int     fog_type2;
        int     color_clamp2;
    } gen;
    struct {
        int     src;
        int     dst;
        int     src_enable;
        int     dst_enable;
        int     src2;
        int     dst2;
        int     src_enable2;
        int     dst_enable2;
    } blend;
    struct {
        int     color;
        int     uv;
        int     modifier;
    } fmt;
    struct {
        int     comparison;
        int     write;
    } depth;
    struct {
        int     enable;
        int     filter;
        int     mipmap;
        int     mipmap_bias;
        int     uv_flip;
        int     uv_clamp;
        int     alpha;
        int     env;
        int     width;
        int     height;
        int     format;
        void*   base;
    } txr;
    struct {
        int     enable;
        int     filter;
        int     mipmap;
        int     mipmap_bias;
        int     uv_flip;
        int     uv_clamp;
        int     alpha;
        int     env;
        int     width;
        int     height;
        int     format;
        void*   base;
    } txr2;
} PolyContext;

typedef struct {
    uint32_t cmd;
    uint32_t mode1;
    uint32_t mode2;
    uint32_t mode3;
    uint32_t d1;
    uint32_t d2;
    uint32_t d3;
    uint32_t d4;
} PolyHeader;

enum GPUCommand {
    GPU_CMD_POLYHDR = 0x80840000,
    GPU_CMD_VERTEX = 0xe0000000,
    GPU_CMD_VERTEX_EOL = 0xf0000000,
    GPU_CMD_USERCLIP = 0x20000000,
    GPU_CMD_MODIFIER = 0x80000000,
    GPU_CMD_SPRITE = 0xA0000000
};

void SceneListSubmit(Vertex* v2, int n);

#define GPU_TA_CMD_TYPE_SHIFT       24
#define GPU_TA_CMD_TYPE_MASK        (7 << GPU_TA_CMD_TYPE_SHIFT)

#define GPU_TA_CMD_USERCLIP_SHIFT   16
#define GPU_TA_CMD_USERCLIP_MASK    (3 << GPU_TA_CMD_USERCLIP_SHIFT)

#define GPU_TA_CMD_CLRFMT_SHIFT     4
#define GPU_TA_CMD_CLRFMT_MASK      (7 << GPU_TA_CMD_CLRFMT_SHIFT)

#define GPU_TA_CMD_SPECULAR_SHIFT   2
#define GPU_TA_CMD_SPECULAR_MASK    (1 << GPU_TA_CMD_SPECULAR_SHIFT)

#define GPU_TA_CMD_SHADE_SHIFT      1
#define GPU_TA_CMD_SHADE_MASK       (1 << GPU_TA_CMD_SHADE_SHIFT)

#define GPU_TA_CMD_UVFMT_SHIFT      0
#define GPU_TA_CMD_UVFMT_MASK       (1 << GPU_TA_CMD_UVFMT_SHIFT)

#define GPU_TA_CMD_MODIFIER_SHIFT   7
#define GPU_TA_CMD_MODIFIER_MASK    (1 <<  GPU_TA_CMD_MODIFIER_SHIFT)

#define GPU_TA_CMD_MODIFIERMODE_SHIFT   6
#define GPU_TA_CMD_MODIFIERMODE_MASK    (1 <<  GPU_TA_CMD_MODIFIERMODE_SHIFT)

#define GPU_TA_PM1_DEPTHCMP_SHIFT   29
#define GPU_TA_PM1_DEPTHCMP_MASK    (7 << GPU_TA_PM1_DEPTHCMP_SHIFT)

#define GPU_TA_PM1_CULLING_SHIFT    27
#define GPU_TA_PM1_CULLING_MASK     (3 << GPU_TA_PM1_CULLING_SHIFT)

#define GPU_TA_PM1_DEPTHWRITE_SHIFT 26
#define GPU_TA_PM1_DEPTHWRITE_MASK  (1 << GPU_TA_PM1_DEPTHWRITE_SHIFT)

#define GPU_TA_PM1_TXRENABLE_SHIFT  25
#define GPU_TA_PM1_TXRENABLE_MASK   (1 << GPU_TA_PM1_TXRENABLE_SHIFT)

#define GPU_TA_PM1_MODIFIERINST_SHIFT   29
#define GPU_TA_PM1_MODIFIERINST_MASK    (3 <<  GPU_TA_PM1_MODIFIERINST_SHIFT)

#define GPU_TA_PM2_SRCBLEND_SHIFT   29
#define GPU_TA_PM2_SRCBLEND_MASK    (7 << GPU_TA_PM2_SRCBLEND_SHIFT)

#define GPU_TA_PM2_DSTBLEND_SHIFT   26
#define GPU_TA_PM2_DSTBLEND_MASK    (7 << GPU_TA_PM2_DSTBLEND_SHIFT)

#define GPU_TA_PM2_SRCENABLE_SHIFT  25
#define GPU_TA_PM2_SRCENABLE_MASK   (1 << GPU_TA_PM2_SRCENABLE_SHIFT)

#define GPU_TA_PM2_DSTENABLE_SHIFT  24
#define GPU_TA_PM2_DSTENABLE_MASK   (1 << GPU_TA_PM2_DSTENABLE_SHIFT)

#define GPU_TA_PM2_FOG_SHIFT        22
#define GPU_TA_PM2_FOG_MASK     (3 << GPU_TA_PM2_FOG_SHIFT)

#define GPU_TA_PM2_CLAMP_SHIFT      21
#define GPU_TA_PM2_CLAMP_MASK       (1 << GPU_TA_PM2_CLAMP_SHIFT)

#define GPU_TA_PM2_ALPHA_SHIFT      20
#define GPU_TA_PM2_ALPHA_MASK       (1 << GPU_TA_PM2_ALPHA_SHIFT)

#define GPU_TA_PM2_TXRALPHA_SHIFT   19
#define GPU_TA_PM2_TXRALPHA_MASK    (1 << GPU_TA_PM2_TXRALPHA_SHIFT)

#define GPU_TA_PM2_UVFLIP_SHIFT     17
#define GPU_TA_PM2_UVFLIP_MASK      (3 << GPU_TA_PM2_UVFLIP_SHIFT)

#define GPU_TA_PM2_UVCLAMP_SHIFT    15
#define GPU_TA_PM2_UVCLAMP_MASK     (3 << GPU_TA_PM2_UVCLAMP_SHIFT)

#define GPU_TA_PM2_FILTER_SHIFT     12
#define GPU_TA_PM2_FILTER_MASK      (7 << GPU_TA_PM2_FILTER_SHIFT)

#define GPU_TA_PM2_MIPBIAS_SHIFT    8
#define GPU_TA_PM2_MIPBIAS_MASK     (15 << GPU_TA_PM2_MIPBIAS_SHIFT)

#define GPU_TA_PM2_TXRENV_SHIFT     6
#define GPU_TA_PM2_TXRENV_MASK      (3 << GPU_TA_PM2_TXRENV_SHIFT)

#define GPU_TA_PM2_USIZE_SHIFT      3
#define GPU_TA_PM2_USIZE_MASK       (7 << GPU_TA_PM2_USIZE_SHIFT)

#define GPU_TA_PM2_VSIZE_SHIFT      0
#define GPU_TA_PM2_VSIZE_MASK       (7 << GPU_TA_PM2_VSIZE_SHIFT)

#define GPU_TA_PM3_MIPMAP_SHIFT     31
#define GPU_TA_PM3_MIPMAP_MASK      (1 << GPU_TA_PM3_MIPMAP_SHIFT)

#define GPU_TA_PM3_TXRFMT_SHIFT     0
#define GPU_TA_PM3_TXRFMT_MASK      0xffffffff

static inline int DimensionFlag(const int w) {
    switch(w) {
        case 16: return 1;
        case 32: return 2;
        case 64: return 3;
        case 128: return 4;
        case 256: return 5;
        case 512: return 6;
        case 1024: return 7;
        case 8:
        default:
            return 0;
    }
}

/* Compile a polygon context into a polygon header */
static inline void CompilePolyHeader(PolyHeader *dst, const PolyContext *src) {
    uint32_t  txr_base;

    /* Basically we just take each parameter, clip it, shift it
       into place, and OR it into the final result. */

    /* The base values for CMD */
    dst->cmd = GPU_CMD_POLYHDR;

    dst->cmd |= src->txr.enable << 3;

    /* Or in the list type, shading type, color and UV formats */
    dst->cmd |= (src->list_type << GPU_TA_CMD_TYPE_SHIFT) & GPU_TA_CMD_TYPE_MASK;
    dst->cmd |= (src->fmt.color << GPU_TA_CMD_CLRFMT_SHIFT) & GPU_TA_CMD_CLRFMT_MASK;
    dst->cmd |= (src->gen.shading << GPU_TA_CMD_SHADE_SHIFT) & GPU_TA_CMD_SHADE_MASK;
    dst->cmd |= (src->fmt.uv << GPU_TA_CMD_UVFMT_SHIFT) & GPU_TA_CMD_UVFMT_MASK;
    dst->cmd |= (src->gen.clip_mode << GPU_TA_CMD_USERCLIP_SHIFT) & GPU_TA_CMD_USERCLIP_MASK;
    dst->cmd |= (src->gen.specular << GPU_TA_CMD_SPECULAR_SHIFT) & GPU_TA_CMD_SPECULAR_MASK;

    /* Polygon mode 1 */
    dst->mode1  = (src->depth.comparison << GPU_TA_PM1_DEPTHCMP_SHIFT) & GPU_TA_PM1_DEPTHCMP_MASK;
    dst->mode1 |= (src->gen.culling << GPU_TA_PM1_CULLING_SHIFT) & GPU_TA_PM1_CULLING_MASK;
    dst->mode1 |= (src->depth.write << GPU_TA_PM1_DEPTHWRITE_SHIFT) & GPU_TA_PM1_DEPTHWRITE_MASK;
    dst->mode1 |= (src->txr.enable << GPU_TA_PM1_TXRENABLE_SHIFT) & GPU_TA_PM1_TXRENABLE_MASK;

    /* Polygon mode 2 */
    dst->mode2  = (src->blend.src << GPU_TA_PM2_SRCBLEND_SHIFT) & GPU_TA_PM2_SRCBLEND_MASK;
    dst->mode2 |= (src->blend.dst << GPU_TA_PM2_DSTBLEND_SHIFT) & GPU_TA_PM2_DSTBLEND_MASK;
    dst->mode2 |= (src->blend.src_enable << GPU_TA_PM2_SRCENABLE_SHIFT) & GPU_TA_PM2_SRCENABLE_MASK;
    dst->mode2 |= (src->blend.dst_enable << GPU_TA_PM2_DSTENABLE_SHIFT) & GPU_TA_PM2_DSTENABLE_MASK;
    dst->mode2 |= (src->gen.fog_type << GPU_TA_PM2_FOG_SHIFT) & GPU_TA_PM2_FOG_MASK;
    dst->mode2 |= (src->gen.color_clamp << GPU_TA_PM2_CLAMP_SHIFT) & GPU_TA_PM2_CLAMP_MASK;
    dst->mode2 |= (src->gen.alpha << GPU_TA_PM2_ALPHA_SHIFT) & GPU_TA_PM2_ALPHA_MASK;

    if(src->txr.enable == GPU_TEXTURE_DISABLE) {
        dst->mode3 = 0;
    }
    else {
        dst->mode2 |= (src->txr.alpha << GPU_TA_PM2_TXRALPHA_SHIFT) & GPU_TA_PM2_TXRALPHA_MASK;
        dst->mode2 |= (src->txr.uv_flip << GPU_TA_PM2_UVFLIP_SHIFT) & GPU_TA_PM2_UVFLIP_MASK;
        dst->mode2 |= (src->txr.uv_clamp << GPU_TA_PM2_UVCLAMP_SHIFT) & GPU_TA_PM2_UVCLAMP_MASK;
        dst->mode2 |= (src->txr.filter << GPU_TA_PM2_FILTER_SHIFT) & GPU_TA_PM2_FILTER_MASK;
        dst->mode2 |= (src->txr.mipmap_bias << GPU_TA_PM2_MIPBIAS_SHIFT) & GPU_TA_PM2_MIPBIAS_MASK;
        dst->mode2 |= (src->txr.env << GPU_TA_PM2_TXRENV_SHIFT) & GPU_TA_PM2_TXRENV_MASK;

        dst->mode2 |= (DimensionFlag(src->txr.width) << GPU_TA_PM2_USIZE_SHIFT) & GPU_TA_PM2_USIZE_MASK;
        dst->mode2 |= (DimensionFlag(src->txr.height) << GPU_TA_PM2_VSIZE_SHIFT) & GPU_TA_PM2_VSIZE_MASK;

        /* Polygon mode 3 */
        dst->mode3  = (src->txr.mipmap << GPU_TA_PM3_MIPMAP_SHIFT) & GPU_TA_PM3_MIPMAP_MASK;
        dst->mode3 |= (src->txr.format << GPU_TA_PM3_TXRFMT_SHIFT) & GPU_TA_PM3_TXRFMT_MASK;

        /* Convert the texture address */
        txr_base = (uint32_t) src->txr.base;
        txr_base = (txr_base & 0x00fffff8) >> 3;
        dst->mode3 |= txr_base;
    }

    dst->d1 = dst->d2 = 0xffffffff;
    dst->d3 = dst->d4 = 0xffffffff;
}

#include "sh4.h"