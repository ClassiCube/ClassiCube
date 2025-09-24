/*########################################################################################################################*
*-----------------------------------------------------GPU commands--------------------------------------------------------*
*#########################################################################################################################*/
// NOTE: shared with RSX (PS3) GPU
#define NV2A_MASK(mask, val) (((val) << (__builtin_ffs(mask)-1)) & (mask))

// disables the default increment behaviour when writing multiple registers
// E.g. with NV2A_3D_COMMAND(cmd, 4):
// - default: REG+0 = v1, REG+4 = v2, REG+8 = v3, REG+12= v4
// - disable: REG   = v1, REG   = v2, REG   = v3, REG   = v4
#define NV2A_WRITE_SAME_REGISTER 0x40000000

#define NV2A_COMMAND(subchan, cmd, num_params) (((num_params) << 18) | ((subchan) << 13) | (cmd))
#define NV2A_3D_COMMAND(cmd, num_params) NV2A_COMMAND(SUBCH_3D, cmd, num_params)

#define _NV_ALPHAKILL_EN (1 << 2)

// Enables perspective correct texture interpolation
#define _NV_CONTROL0_TEX_PERSPECTIVE_ENABLE (1 << 20)


/*########################################################################################################################*
*-----------------------------------------------------GPU pushbuffer------------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE uint32_t* NV2A_push1(uint32_t* p, int cmd, uint32_t value) {
	*p++ = NV2A_3D_COMMAND(cmd, 1);
	*p++ = value;
	return p;
}

static CC_INLINE uint32_t* NV2A_push2(uint32_t* p, int cmd, uint32_t val1, uint32_t val2) {
	*p++ = NV2A_3D_COMMAND(cmd, 2);
	*p++ = val1;
	*p++ = val2;
	return p;
}


/*########################################################################################################################*
*-----------------------------------------------------Misc commands-------------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE uint32_t* NV2A_reset_control0(uint32_t* p) {
	// resets "z perspective" flag, set "texture perspective flag"
	return NV2A_push1(p, NV097_SET_CONTROL0, _NV_CONTROL0_TEX_PERSPECTIVE_ENABLE);
}


/*########################################################################################################################*
*-----------------------------------------------------Raster control------------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE uint32_t* NV2A_set_clip_rect(uint32_t* p, int x, int y, int w, int h) {
	// NV097_SET_SURFACE_CLIP_HORIZONTAL, then NV097_SET_SURFACE_CLIP_VERTICAL 
	return NV2A_push2(p, NV097_SET_SURFACE_CLIP_HORIZONTAL,
					x | (w << 16), y | (h << 16));
}

static CC_INLINE uint32_t* NV2A_set_color_write_mask(uint32_t* p, int r, int g, int b, int a) {
	unsigned mask = 0;
	if (r) mask |= NV097_SET_COLOR_MASK_RED_WRITE_ENABLE;
	if (g) mask |= NV097_SET_COLOR_MASK_GREEN_WRITE_ENABLE;
	if (b) mask |= NV097_SET_COLOR_MASK_BLUE_WRITE_ENABLE;
	if (a) mask |= NV097_SET_COLOR_MASK_ALPHA_WRITE_ENABLE;
	
	return NV2A_push1(p, NV097_SET_COLOR_MASK, mask);
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE uint32_t* NV2A_set_fog_colour(uint32_t* p, int R, int G, int B, int A) {
	uint32_t mask = 
		NV2A_MASK(NV097_SET_FOG_COLOR_RED,   R) |
		NV2A_MASK(NV097_SET_FOG_COLOR_GREEN, G) |
		NV2A_MASK(NV097_SET_FOG_COLOR_BLUE,  B) |
		NV2A_MASK(NV097_SET_FOG_COLOR_ALPHA, A);

	return NV2A_push1(p, NV097_SET_FOG_COLOR, mask);
}


static CC_INLINE uint32_t* NV2A_set_depth_write(uint32_t* p, int enabled) {
	return NV2A_push1(p, NV097_SET_DEPTH_MASK, enabled);
}

static CC_INLINE uint32_t* NV2A_set_depth_test(uint32_t* p, int enabled) {
	return NV2A_push1(p, NV097_SET_DEPTH_TEST_ENABLE, enabled);
}

static CC_INLINE uint32_t* NV2A_set_depth_func(uint32_t* p, int func) {
	return NV2A_push1(p, NV097_SET_DEPTH_FUNC, func);
}


static CC_INLINE uint32_t* NV2A_set_alpha_test(uint32_t* p, int enabled) {
	return NV2A_push1(p, NV097_SET_ALPHA_TEST_ENABLE, enabled);
}

static CC_INLINE uint32_t* NV2A_set_alpha_test_func(uint32_t* p, int func) {
	return NV2A_push1(p, NV097_SET_ALPHA_FUNC, func);
}

static CC_INLINE uint32_t* NV2A_set_alpha_test_ref(uint32_t* p, int ref) {
	return NV2A_push1(p, NV097_SET_ALPHA_REF, ref);
}


static CC_INLINE uint32_t* NV2A_set_alpha_blend(uint32_t* p, int enabled) {
	return NV2A_push1(p, NV097_SET_BLEND_ENABLE, enabled);
}

static CC_INLINE uint32_t* NV2A_set_alpha_blend_src(uint32_t* p, int factor) {
	return NV2A_push1(p, NV097_SET_BLEND_FUNC_SFACTOR, factor);
}

static CC_INLINE uint32_t* NV2A_set_alpha_blend_dst(uint32_t* p, int factor) {
	return NV2A_push1(p, NV097_SET_BLEND_FUNC_DFACTOR, factor);
}

static CC_INLINE uint32_t* NV2A_set_alpha_blend_eq(uint32_t* p, int equation) {
	return NV2A_push1(p, NV097_SET_BLEND_EQUATION, equation);
}


static CC_INLINE uint32_t* NV2A_set_cull_face(uint32_t* p, int enabled) {
	return NV2A_push1(p, NV097_SET_CULL_FACE_ENABLE, enabled);
}

static CC_INLINE uint32_t* NV2A_set_cull_face_mode(uint32_t* p, int mode) {
	return NV2A_push1(p, NV097_SET_CULL_FACE, mode);
}


/*########################################################################################################################*
*-----------------------------------------------------Primitive drawing---------------------------------------------------*
*#########################################################################################################################*/
// NV097_DRAW_ARRAYS_COUNT is an 8 bit mask, so each draw call count must be <= 256
#define DA_BATCH_SIZE 256

static void NV2A_DrawArrays(int mode, unsigned start, unsigned count) {
	uint32_t *p = pb_begin();
	p = NV2A_push1(p, NV097_SET_BEGIN_END, mode);

	// Ceiling division by DA_BATCH_SIZE
	unsigned num_batches = (count + DA_BATCH_SIZE - 1) / DA_BATCH_SIZE;
	*p++ = NV2A_3D_COMMAND(NV2A_WRITE_SAME_REGISTER | NV097_DRAW_ARRAYS, num_batches);

	while (count > 0)
	{
		int batch_count = min(count, DA_BATCH_SIZE);
	
		*p++ = NV2A_MASK(NV097_DRAW_ARRAYS_COUNT, batch_count-1) | 
			   NV2A_MASK(NV097_DRAW_ARRAYS_START_INDEX, start);
		
		start += batch_count;
		count -= batch_count;
	}

	p = NV2A_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
	pb_end(p);
}


/*########################################################################################################################*
*--------------------------------------------------Vertex shader constants------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE uint32_t* NV2A_set_constant_upload_offset(uint32_t* p, int offset) {
	// set shader constants cursor to: C0 + offset
	return NV2A_push1(p, NV097_SET_TRANSFORM_CONSTANT_LOAD, 96 + offset);
}

static CC_INLINE uint32_t* NV2A_upload_constants(uint32_t* p, void* src, int num_dwords) {
	*p++ = NV2A_3D_COMMAND(NV097_SET_TRANSFORM_CONSTANT, num_dwords);
	Mem_Copy(p, src, num_dwords * 4); p += num_dwords;
	return p;
}


/*########################################################################################################################*
*---------------------------------------------------Vertex shader programs-------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE uint32_t* NV2A_set_program_upload_offset(uint32_t* p, int offset) {
	return NV2A_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, offset);
}

static CC_INLINE uint32_t* NV2A_upload_program(uint32_t* p, uint32_t* program, int size) {
	// Copy program instructions (16 bytes each)
	for (int i = 0; i < size / 16; i++, program += 4) 
	{
		*p++ = NV2A_3D_COMMAND(NV097_SET_TRANSFORM_PROGRAM, 4);
		Mem_Copy(p, program, 16); p += 4;
	}
	return p;
}

static CC_INLINE uint32_t* NV2A_set_program_run_offset(uint32_t* p, int offset) {
	return NV2A_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, offset);
}

static CC_INLINE uint32_t* NV2A_set_execution_mode_shaders(uint32_t* p) {
	p = NV2A_push1(p, NV097_SET_TRANSFORM_EXECUTION_MODE,
					NV2A_MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE,       NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_PROGRAM) |
					NV2A_MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));

	p = NV2A_push1(p, NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, 0);
	return p;
}


/*########################################################################################################################*
*-----------------------------------------------------Vertex attributes---------------------------------------------------*
*#########################################################################################################################*/
// https://xboxdevwiki.net/NV2A/Vertex_Shader#Input_registers
// 16 input vertex attribute registers
#define NV2A_MAX_INPUT_ATTRIBS 16

static uint32_t* NV2A_reset_all_vertex_attribs(uint32_t* p) {
	*p++ = NV2A_3D_COMMAND(NV097_SET_VERTEX_DATA_ARRAY_FORMAT, NV2A_MAX_INPUT_ATTRIBS);

	for (int i = 0; i < NV2A_MAX_INPUT_ATTRIBS; i++) 
	{
		*p++ = NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F;
	}
	return p;
}

static uint32_t* NV2A_set_vertex_attrib_format(uint32_t* p, int attrib, int format, int size, int stride) {
	uint32_t mask =
		NV2A_MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE, format) |
		NV2A_MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_SIZE, size)   |
		NV2A_MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_STRIDE, stride);

	return NV2A_push1(p, NV097_SET_VERTEX_DATA_ARRAY_FORMAT + attrib * 4, mask);
}

static uint32_t* NV2A_set_vertex_attrib_pointer(uint32_t* p, int attrib, cc_uint8* data) {
	uint32_t offset = (uint32_t)data & 0x03ffffff;
	return NV2A_push1(p, NV097_SET_VERTEX_DATA_ARRAY_OFFSET + attrib * 4, offset);
}


/*########################################################################################################################*
*------------------------------------------------------Buffer clearing----------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE uint32_t* NV2A_set_clear_rect(uint32_t* p, int x, int y, int w, int h) {
	// Sets NV097_SET_CLEAR_RECT_HORIZONTAL then NV097_SET_CLEAR_RECT_VERTICAL
	return NV2A_push2(p, NV097_SET_CLEAR_RECT_HORIZONTAL,
					((x + w - 1) << 16) | x,
					((y + h - 1) << 16) | y);		
}

static CC_INLINE uint32_t* NV2A_set_clear_colour(uint32_t* p, uint32_t colour) {
	// Sets NV097_SET_ZSTENCIL_CLEAR_VALUE then NV097_SET_COLOR_CLEAR_VALUE
	return NV2A_push2(p, NV097_SET_ZSTENCIL_CLEAR_VALUE,
					0xFFFFFF00, // (depth << 8) | stencil,
					colour);
}

static CC_INLINE uint32_t* NV2A_clear_buffers(uint32_t* p, int color, int depth) {
    uint32_t mask = 0;
	if (color) mask |= NV097_CLEAR_SURFACE_COLOR;
	if (depth) mask |= NV097_CLEAR_SURFACE_Z;
	if (depth) mask |= NV097_CLEAR_SURFACE_STENCIL;

	return NV2A_push1(p, NV097_CLEAR_SURFACE, mask);
}


/*########################################################################################################################*
*--------------------------------------------------------Texturing--------------------------------------------------------*
*#########################################################################################################################*/
// NOTE: API is hardcoded for one texture only, even though hardware supports multiple textures

static CC_INLINE uint32_t* NV2A_set_texture0_control0(uint32_t* p, int texkill) {
	return NV2A_push1(p, NV097_SET_TEXTURE_CONTROL0, 
					NV097_SET_TEXTURE_CONTROL0_ENABLE |
					(texkill ? _NV_ALPHAKILL_EN : 0) |
					NV2A_MASK(NV097_SET_TEXTURE_CONTROL0_MIN_LOD_CLAMP, 0) |
					NV2A_MASK(NV097_SET_TEXTURE_CONTROL0_MAX_LOD_CLAMP, 1));
}

static uint32_t* NV2A_set_texture0_pointer(uint32_t* p, void* pixels) {
	uint32_t offset = (uint32_t)pixels & 0x03ffffff;
	return NV2A_push1(p, NV097_SET_TEXTURE_OFFSET, offset);
}

static uint32_t* NV2A_set_texture0_format(uint32_t* p, unsigned log_u, unsigned log_v) {
	return NV2A_push1(p, NV097_SET_TEXTURE_FORMAT,
					NV2A_MASK(NV097_SET_TEXTURE_FORMAT_CONTEXT_DMA,    2) |
					NV2A_MASK(NV097_SET_TEXTURE_FORMAT_BORDER_SOURCE,  NV097_SET_TEXTURE_FORMAT_BORDER_SOURCE_COLOR) |
					NV2A_MASK(NV097_SET_TEXTURE_FORMAT_COLOR,          NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8) |
					NV2A_MASK(NV097_SET_TEXTURE_FORMAT_DIMENSIONALITY, 2)  | // textures have U and V
					NV2A_MASK(NV097_SET_TEXTURE_FORMAT_MIPMAP_LEVELS,  1)  |
					NV2A_MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_U, log_u) |
					NV2A_MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_V, log_v) |
					NV2A_MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_P,    0)); // log2(1) slice = 0
}

static uint32_t* NV2A_set_texture0_wrapmode(uint32_t* p) {
	return NV2A_push1(p, NV097_SET_TEXTURE_ADDRESS, 
					0x00010101); // modes (0x0W0V0U wrapping: 1=wrap 2=mirror 3=clamp 4=border 5=clamp to edge)
}

static uint32_t* NV2A_set_texture0_filter(uint32_t* p) {
	return NV2A_push1(p, NV097_SET_TEXTURE_FILTER,
					0x2000 |
					NV2A_MASK(NV097_SET_TEXTURE_FILTER_MIN, 1) |
					NV2A_MASK(NV097_SET_TEXTURE_FILTER_MAG, 1)); // 1 = nearest filter
}

static uint32_t* NV2A_set_texture0_matrix(uint32_t* p, int enabled) {
	return NV2A_push1(p, NV097_SET_TEXTURE_MATRIX_ENABLE, enabled);
}

