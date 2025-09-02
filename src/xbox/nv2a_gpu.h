// disables the default increment behaviour when writing multiple registers
// E.g. with pb_push4(p, REG, v1, v2, v3, v4):
// - default: REG+0 = v1, REG+4 = v2, REG+8 = v3, REG+12= v4
// - disable: REG   = v1, REG   = v2, REG   = v3, REG   = v4
#define NV2A_WRITE_SAME_REGISTER 0x40000000

static CC_INLINE uint32_t* NV2A_reset_control0(uint32_t* p) {
	// resets "z perspective" flag
	return pb_push1(p, NV097_SET_CONTROL0, 0);
}


/*########################################################################################################################*
*-----------------------------------------------------Raster control------------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE uint32_t* NV2A_set_clip_rect(uint32_t* p, int x, int y, int w, int h) {
	// NV097_SET_SURFACE_CLIP_HORIZONTAL, then NV097_SET_SURFACE_CLIP_VERTICAL 
	return pb_push2(p, NV097_SET_SURFACE_CLIP_HORIZONTAL, x | (w << 16), y | (h << 16));
}

static CC_INLINE uint32_t* NV2A_set_color_write_mask(uint32_t* p, int r, int g, int b, int a) {
	unsigned mask = 0;
	if (r) mask |= NV097_SET_COLOR_MASK_RED_WRITE_ENABLE;
	if (g) mask |= NV097_SET_COLOR_MASK_GREEN_WRITE_ENABLE;
	if (b) mask |= NV097_SET_COLOR_MASK_BLUE_WRITE_ENABLE;
	if (a) mask |= NV097_SET_COLOR_MASK_ALPHA_WRITE_ENABLE;
	
	return pb_push1(p, NV097_SET_COLOR_MASK, mask);
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE uint32_t* NV2A_set_fog_colour(uint32_t* p, int R, int G, int B, int A) {
	return pb_push1(p, NV097_SET_FOG_COLOR, 
					MASK(NV097_SET_FOG_COLOR_RED,   R) |
					MASK(NV097_SET_FOG_COLOR_GREEN, G) |
					MASK(NV097_SET_FOG_COLOR_BLUE,  B) |
					MASK(NV097_SET_FOG_COLOR_ALPHA, A));
}


/*########################################################################################################################*
*--------------------------------------------------Vertex shader constants------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE uint32_t* NV2A_set_constant_upload_offset(uint32_t* p, int offset) {
	// set shader constants cursor to: C0 + offset
	return pb_push1(p, NV097_SET_TRANSFORM_CONSTANT_LOAD, 96 + offset);
}

static CC_INLINE uint32_t* NV2A_upload_constants(uint32_t* p, void* src, int num_dwords) {
	pb_push(p++, NV097_SET_TRANSFORM_CONSTANT, num_dwords);
	Mem_Copy(p, src, num_dwords * 4); p += num_dwords;
	return p;
}


/*########################################################################################################################*
*---------------------------------------------------Vertex shader programs-------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE uint32_t* NV2A_set_program_upload_offset(uint32_t* p, int offset) {
	return pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, offset);
}

static CC_INLINE uint32_t* NV2A_upload_program(uint32_t* p, uint32_t* program, int size) {
	// Copy program instructions (16 bytes each)
	for (int i = 0; i < size / 16; i++, program += 4) 
	{
		pb_push(p++, NV097_SET_TRANSFORM_PROGRAM, 4);
		Mem_Copy(p, program, 16); p += 4;
	}
	return p;
}

static CC_INLINE uint32_t* NV2A_set_program_run_offset(uint32_t* p, int offset) {
	return pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, offset);
}


/*########################################################################################################################*
*-----------------------------------------------------Vertex attributes---------------------------------------------------*
*#########################################################################################################################*/
// https://xboxdevwiki.net/NV2A/Vertex_Shader#Input_registers
// 16 input vertex attribute registers
#define NV2A_MAX_INPUT_ATTRIBS 16

static uint32_t* NV2A_reset_all_vertex_attribs(uint32_t* p) {
	pb_push(p++, NV097_SET_VERTEX_DATA_ARRAY_FORMAT, NV2A_MAX_INPUT_ATTRIBS);

	for (int i = 0; i < NV2A_MAX_INPUT_ATTRIBS; i++) 
	{
		*(p++) = NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F;
	}
	return p;
}

static uint32_t* NV2A_set_vertex_attrib_format(uint32_t* p, int index, int format, int size, int stride) {
	return pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_FORMAT + index * 4,
						MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE, format) |
						MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_SIZE, size)   |
						MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_STRIDE, stride));
}


/*########################################################################################################################*
*------------------------------------------------------Buffer clearing----------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE uint32_t* NV2A_set_clear_rect(uint32_t* p, int x, int y, int w, int h) {
	// Sets NV097_SET_CLEAR_RECT_HORIZONTAL then NV097_SET_CLEAR_RECT_VERTICAL
	return pb_push2(p, NV097_SET_CLEAR_RECT_HORIZONTAL,
					((x + w - 1) << 16) | x,
					((y + h - 1) << 16) | y);			
}

static CC_INLINE uint32_t* NV2A_set_clear_colour(uint32_t* p, uint32_t colour) {
	// Sets NV097_SET_ZSTENCIL_CLEAR_VALUE then NV097_SET_COLOR_CLEAR_VALUE
	return pb_push2(p, NV097_SET_ZSTENCIL_CLEAR_VALUE,
					0xFFFFFF00, // (depth << 8) | stencil
					colour);			
}

static CC_INLINE uint32_t* NV2A_start_clear(uint32_t* p, int color, int depth) {
    uint32_t mask = 0;
	if (color) mask |= NV097_CLEAR_SURFACE_COLOR;
	if (depth) mask |= NV097_CLEAR_SURFACE_Z;
	if (depth) mask |= NV097_CLEAR_SURFACE_STENCIL;

	return pb_push1(p, NV097_CLEAR_SURFACE, mask);
}

