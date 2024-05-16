#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "gl_assert.h"
#include "types.h"

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

#include "sh4.h"