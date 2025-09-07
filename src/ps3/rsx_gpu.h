/*########################################################################################################################*
*-----------------------------------------------------GPU commands--------------------------------------------------------*
*#########################################################################################################################*/
// NOTE: shared with nv2a (Xbox) GPU

// disables the default increment behaviour when writing multiple registers
// E.g. with RSX_3D_COMMAND(cmd, 4):
// - default: REG+0 = v1, REG+4 = v2, REG+8 = v3, REG+12= v4
// - disable: REG   = v1, REG   = v2, REG   = v3, REG   = v4
#define RSX_WRITE_SAME_REGISTER 0x40000000

#define RSX_COMMAND(subchan, cmd, num_params) (((num_params) << 18) | ((subchan) << 13) | (cmd))
#define RSX_3D_COMMAND(cmd, num_params) RSX_COMMAND(0, cmd, num_params)

#include <rsx/nv40.h>


/*########################################################################################################################*
*---------------------------------------------------GPU command buffer----------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE uint32_t* RSX_reserve(gcmContextData* ctx, int count) {
	if (ctx->current + count > ctx->end) {
		// TODO handle command buffer overflow properly
		Process_Abort("Command buffer overflow");
	}

	uint32_t* p = ctx->current;
	ctx->current += count;
	return p;
}

static CC_INLINE uint32_t* RSX_reserve_command(gcmContextData* context, uint32_t cmd, int num_params) {
	uint32_t* p = RSX_reserve(context, num_params + 1);
	*p++ = RSX_3D_COMMAND(cmd, num_params);
	return p;
}


/*########################################################################################################################*
*-----------------------------------------------------Raster control------------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE void RSX_set_scissor_rect(gcmContextData* ctx, int x, int y, int w, int h) {
	// NV40TCL_SCISSOR_HORIZ, then NV40TCL_SCISSOR_VERTI 
	uint32_t* p = RSX_reserve_command(ctx, NV40TCL_SCISSOR_HORIZ, 2);
	*p++ = x | (w << 16);
	*p++ = y | (h << 16);
}

static CC_INLINE void RSX_set_color_write_mask(gcmContextData* ctx, int r, int g, int b, int a) {
	uint32_t mask = 0;
	if (r) mask |= GCM_COLOR_MASK_R;
	if (g) mask |= GCM_COLOR_MASK_G;
	if (b) mask |= GCM_COLOR_MASK_B;
	if (a) mask |= GCM_COLOR_MASK_A;

	uint32_t* p = RSX_reserve_command(ctx, NV40TCL_COLOR_MASK, 1);
	*p++ = mask;
}
