/*########################################################################################################################*
*-----------------------------------------------------GPU commands--------------------------------------------------------*
*#########################################################################################################################*/
// NOTE: shared with nv2a (Xbox) GPU
#define RSX_MASK(mask, val) (((val) << (__builtin_ffs(mask)-1)) & (mask))

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
		// This flushes the command buffer to GPU via DMA
		// TODO: returns 0 if fails, but not checked for??
		rsxContextCallback(ctx, count);
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

#define RSX_append_single_command(context, cmd, value) \
	uint32_t* p = RSX_reserve_command(ctx, cmd, 1); \
	*p++ = value;


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

	RSX_append_single_command(ctx, NV40TCL_COLOR_MASK, mask);
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE void RSX_set_depth_write(gcmContextData* ctx, int enabled) {
	RSX_append_single_command(ctx, NV40TCL_DEPTH_WRITE_ENABLE, enabled);
}

static CC_INLINE void RSX_set_depth_test(gcmContextData* ctx, int enabled) {
	RSX_append_single_command(ctx, NV40TCL_DEPTH_TEST_ENABLE, enabled);
}

static CC_INLINE void RSX_set_depth_func(gcmContextData* ctx, int func) {
	RSX_append_single_command(ctx, NV40TCL_DEPTH_FUNC, func);
}


static CC_INLINE void RSX_set_alpha_test(gcmContextData* ctx, int enabled) {
	RSX_append_single_command(ctx, NV40TCL_ALPHA_TEST_ENABLE, enabled);
}

static CC_INLINE void RSX_set_alpha_test_func(gcmContextData* ctx, int func) {
	RSX_append_single_command(ctx, NV40TCL_ALPHA_TEST_FUNC, func);
}

static CC_INLINE void RSX_set_alpha_test_ref(gcmContextData* ctx, int ref) {
	RSX_append_single_command(ctx, NV40TCL_ALPHA_TEST_REF, ref);
}


static CC_INLINE void RSX_set_alpha_blend(gcmContextData* ctx, int enabled) {
	RSX_append_single_command(ctx, NV40TCL_BLEND_ENABLE, enabled);
}


static CC_INLINE void RSX_set_cull_face(gcmContextData* ctx, int enabled) {
	RSX_append_single_command(ctx, NV40TCL_CULL_FACE_ENABLE, enabled);
}


/*########################################################################################################################*
*-----------------------------------------------------Vertex attributes---------------------------------------------------*
*#########################################################################################################################*/
/*static CC_INLINE void RSX_set_vertex_attrib_format(gcmContextData* ctx, int attrib, int format, int size, int stride) {
	uint32_t mask =
		RSX_MASK(NV40TCL_VTXFMT_TYPE_MASK,   format) |
		RSX_MASK(NV40TCL_VTXFMT_SIZE_MASK,   size)   |
		RSX_MASK(NV40TCL_VTXFMT_STRIDE_MASK, stride);

	RSX_append_single_command(ctx, NV40TCL_VTXFMT(attrib), mask);
}

// NOTE: GCM_LOCATION_RSX implicitly assumed for location
static CC_INLINE void RSX_set_vertex_attrib_pointer(gcmContextData* ctx, int attrib, uint32_t addr) {
	RSX_append_single_command(ctx, NV40TCL_VTXBUF_ADDRESS(attrib), addr);
}*/


/*########################################################################################################################*
*------------------------------------------------------Buffer clearing----------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE void RSX_clear_buffers(gcmContextData* ctx, int color, int depth) {
	uint32_t targets = 0;
	if (color) targets |= (GCM_CLEAR_R | GCM_CLEAR_G | GCM_CLEAR_B | GCM_CLEAR_A);
	if (depth) targets |= (GCM_CLEAR_S | GCM_CLEAR_Z);

	RSX_append_single_command(ctx, NV40TCL_CLEAR_BUFFERS, targets);
}
