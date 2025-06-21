#include <dc/pvr.h>
#include "gldc.h"

static TextureObject* TEXTURE_ACTIVE;

static uint8_t  DEPTH_TEST_ENABLED;
static uint8_t  DEPTH_MASK_ENABLED;

static uint8_t  CULLING_ENABLED;

static uint8_t  FOG_ENABLED;
static uint8_t  ALPHA_TEST_ENABLED;

static uint8_t  SCISSOR_TEST_ENABLED;
static uint32_t SHADE_MODEL = PVR_SHADE_GOURAUD;

static uint8_t  BLEND_ENABLED;

static uint8_t  TEXTURES_ENABLED;
static uint8_t  AUTOSORT_ENABLED;

static inline int DimensionFlag(int w) {
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

static GLDC_NO_INLINE void apply_poly_header(pvr_poly_hdr_t* dst, int list_type) {
    TextureObject* tx1 = TEXTURE_ACTIVE;

    int gen_culling = CULLING_ENABLED    ? PVR_CULLING_CW : PVR_CULLING_SMALL;
    int depth_comp  = DEPTH_TEST_ENABLED ? PVR_DEPTHCMP_GEQUAL : PVR_DEPTHCMP_ALWAYS;
    int depth_write = DEPTH_MASK_ENABLED ? PVR_DEPTHWRITE_ENABLE : PVR_DEPTHWRITE_DISABLE;

    int clip_mode = SCISSOR_TEST_ENABLED ? PVR_USERCLIP_INSIDE : PVR_USERCLIP_DISABLE;
    int fog_type  = FOG_ENABLED          ? PVR_FOG_TABLE : PVR_FOG_DISABLE;

    int gen_alpha = (BLEND_ENABLED || ALPHA_TEST_ENABLED) ? PVR_ALPHA_ENABLE : PVR_ALPHA_DISABLE;
    int blend_src = PVR_BLEND_SRCALPHA;
    int blend_dst = PVR_BLEND_INVSRCALPHA;

    if (list_type == PVR_LIST_OP_POLY) {
        /* Opaque polys are always one/zero */
        blend_src  = PVR_BLEND_ONE;
        blend_dst  = PVR_BLEND_ZERO;
    } else if (list_type == PVR_LIST_PT_POLY) {
        /* Punch-through polys require fixed blending and depth modes */
        depth_comp = PVR_DEPTHCMP_LEQUAL;
    } else if (list_type == PVR_LIST_TR_POLY && AUTOSORT_ENABLED) {
        /* Autosort mode requires this mode for transparent polys */
        depth_comp = PVR_DEPTHCMP_GEQUAL;
    }

    int txr_enable, txr_alpha;
    if (!TEXTURES_ENABLED || !tx1) {
        /* Disable all texturing to start with */
        txr_enable = PVR_TEXTURE_DISABLE;
    } else {
        txr_alpha  = (BLEND_ENABLED || ALPHA_TEST_ENABLED) ? PVR_TXRALPHA_ENABLE : PVR_TXRALPHA_DISABLE;
        txr_enable = PVR_TEXTURE_ENABLE;
    }

    /* The base values for CMD */
    dst->cmd = PVR_CMD_POLYHDR;
    dst->cmd |= txr_enable << 3;
    /* Force bits 18 and 19 on to switch to 6 triangle strips */
    dst->cmd |= 0xC0000;

    /* Or in the list type, shading type, color and UV formats */
    dst->cmd |= (list_type             << PVR_TA_CMD_TYPE_SHIFT)     & PVR_TA_CMD_TYPE_MASK;
    dst->cmd |= (PVR_CLRFMT_ARGBPACKED << PVR_TA_CMD_CLRFMT_SHIFT)   & PVR_TA_CMD_CLRFMT_MASK;
    dst->cmd |= (SHADE_MODEL           << PVR_TA_CMD_SHADE_SHIFT)    & PVR_TA_CMD_SHADE_MASK;
    dst->cmd |= (PVR_UVFMT_32BIT       << PVR_TA_CMD_UVFMT_SHIFT)    & PVR_TA_CMD_UVFMT_MASK;
    dst->cmd |= (clip_mode             << PVR_TA_CMD_USERCLIP_SHIFT) & PVR_TA_CMD_USERCLIP_MASK;

    dst->mode1  = (depth_comp  << PVR_TA_PM1_DEPTHCMP_SHIFT)   & PVR_TA_PM1_DEPTHCMP_MASK;
    dst->mode1 |= (gen_culling << PVR_TA_PM1_CULLING_SHIFT)    & PVR_TA_PM1_CULLING_MASK;
    dst->mode1 |= (depth_write << PVR_TA_PM1_DEPTHWRITE_SHIFT) & PVR_TA_PM1_DEPTHWRITE_MASK;
    dst->mode1 |= (txr_enable  << PVR_TA_PM1_TXRENABLE_SHIFT)  & PVR_TA_PM1_TXRENABLE_MASK;

    dst->mode2  = (blend_src << PVR_TA_PM2_SRCBLEND_SHIFT) & PVR_TA_PM2_SRCBLEND_MASK;
    dst->mode2 |= (blend_dst << PVR_TA_PM2_DSTBLEND_SHIFT) & PVR_TA_PM2_DSTBLEND_MASK;
    dst->mode2 |= (fog_type  << PVR_TA_PM2_FOG_SHIFT)      & PVR_TA_PM2_FOG_MASK;
    dst->mode2 |= (gen_alpha << PVR_TA_PM2_ALPHA_SHIFT)    & PVR_TA_PM2_ALPHA_MASK;

    if (txr_enable == PVR_TEXTURE_DISABLE) {
        dst->mode3 = 0;
    } else {
        dst->mode2 |= (txr_alpha                << PVR_TA_PM2_TXRALPHA_SHIFT) & PVR_TA_PM2_TXRALPHA_MASK;
        dst->mode2 |= (PVR_FILTER_NEAREST       << PVR_TA_PM2_FILTER_SHIFT)   & PVR_TA_PM2_FILTER_MASK;
        dst->mode2 |= (PVR_MIPBIAS_NORMAL       << PVR_TA_PM2_MIPBIAS_SHIFT)  & PVR_TA_PM2_MIPBIAS_MASK;
        dst->mode2 |= (PVR_TXRENV_MODULATEALPHA << PVR_TA_PM2_TXRENV_SHIFT)   & PVR_TA_PM2_TXRENV_MASK;

        dst->mode2 |= (DimensionFlag(tx1->width)  << PVR_TA_PM2_USIZE_SHIFT) & PVR_TA_PM2_USIZE_MASK;
        dst->mode2 |= (DimensionFlag(tx1->height) << PVR_TA_PM2_VSIZE_SHIFT) & PVR_TA_PM2_VSIZE_MASK;

        dst->mode3  = (0           << PVR_TA_PM3_MIPMAP_SHIFT) & PVR_TA_PM3_MIPMAP_MASK;
        dst->mode3 |= (tx1->format << PVR_TA_PM3_TXRFMT_SHIFT) & PVR_TA_PM3_TXRFMT_MASK;
        dst->mode3 |= ((uint32_t)tx1->data & 0x00fffff8) >> 3;
    }
}
