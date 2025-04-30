#include "rspq.h"
#include "rdpq.h"
#include "rdpq_rect.h"
#include "rdpq_mode.h"
#include "rdpq_debug.h"
#include "display.h"
#include "gl_constants.h"

// This is a severely cutdown version of libdragon's OpenGL implementation

static uint32_t gpup_id;
//DEFINE_RSP_UCODE(rsp_gpu);
extern uint8_t _binary_build_n64_rsp_gpu_text_bin_start[];
extern uint8_t _binary_build_n64_rsp_gpu_data_bin_start[];
extern uint8_t _binary_build_n64_rsp_gpu_meta_bin_start[];
extern uint8_t _binary_build_n64_rsp_gpu_text_bin_end[0];
extern uint8_t _binary_build_n64_rsp_gpu_data_bin_end[0];
extern uint8_t _binary_build_n64_rsp_gpu_meta_bin_end[0];

static rsp_ucode_t rsp_gpu = (rsp_ucode_t){
	.code     = _binary_build_n64_rsp_gpu_text_bin_start,
	.code_end = _binary_build_n64_rsp_gpu_text_bin_end,
	.data     = _binary_build_n64_rsp_gpu_data_bin_start,
	.data_end = _binary_build_n64_rsp_gpu_data_bin_end,
	.meta     = _binary_build_n64_rsp_gpu_meta_bin_start,
	.meta_end = _binary_build_n64_rsp_gpu_meta_bin_end,
	.name     = "rsp_gpu"
};

enum {
    GPU_CMD_SET_BYTE         = 0x0,
    GPU_CMD_SET_SHORT        = 0x1,
    GPU_CMD_SET_WORD         = 0x2,
    GPU_CMD_SET_LONG         = 0x3,

    GPU_CMD_DRAW_TRI         = 0x4,
    GPU_CMD_UPLOAD_VTX       = 0x5,
    GPU_CMD_MATRIX_LOAD      = 0x6,
};

typedef struct {
    int16_t  i[4][4];
    uint16_t f[4][4];
} gpu_matrix_srv_t;
_Static_assert(sizeof(gpu_matrix_srv_t) == MATRIX_SIZE, "Matrix size does not match");

typedef struct {
    gpu_matrix_srv_t mvp_matrix;
    int16_t vp_scale[4];
    int16_t vp_offset[4];
    uint16_t tex_size[2];
    uint16_t tex_offset[2];
    uint16_t tri_cmd;
    uint16_t tri_cull;
} __attribute__((aligned(8), packed)) gpu_state;

__attribute__((always_inline))
static inline void gpu_set_byte(uint32_t offset, uint8_t value)
{
    rspq_write(gpup_id, GPU_CMD_SET_BYTE, offset, value);
}

__attribute__((always_inline))
static inline void gpu_set_short(uint32_t offset, uint16_t value)
{
    rspq_write(gpup_id, GPU_CMD_SET_SHORT, offset, value);
}

__attribute__((always_inline))
static inline void gpu_set_word(uint32_t offset, uint32_t value)
{
    rspq_write(gpup_id, GPU_CMD_SET_WORD, offset, value);
}

__attribute__((always_inline))
static inline void gpu_set_long(uint32_t offset, uint64_t value)
{
    rspq_write(gpup_id, GPU_CMD_SET_LONG, offset, value >> 32, value & 0xFFFFFFFF);
}

static inline void gpu_draw_triangle(int i0, int i1, int i2)
{
    // We pass -1 because the triangle can be clipped and split into multiple
    // triangles.
    rdpq_write(-1, gpup_id, GPU_CMD_DRAW_TRI,
        (i0*PRIM_VTX_SIZE),
        ((i1*PRIM_VTX_SIZE)<<16) | (i2*PRIM_VTX_SIZE)
    );
}


static float gpu_vp_scale[3];
static float gpu_vp_offset[3];
static bool  gpu_texturing;
static void* gpu_pointer;
static int   gpu_stride;

#define GPU_ATTR_Z     (1 <<  8)
#define GPU_ATTR_TEX   (1 <<  9)
#define GPU_ATTR_SHADE (1 << 10)
#define GPU_ATTR_EDGE  (1 << 11)
static bool gpu_attr_z, gpu_attr_tex;

static void gpuUpdateFormat(void)
{
	uint16_t cmd = 0xC000 | GPU_ATTR_SHADE | GPU_ATTR_EDGE;

	if (gpu_attr_z)   cmd |= GPU_ATTR_Z;
	if (gpu_attr_tex) cmd |= GPU_ATTR_TEX;

	gpu_set_short(offsetof(gpu_state, tri_cmd), cmd);
}

static void gpuSetTexSize(uint16_t width, uint16_t height)
{
    gpu_set_word(offsetof(gpu_state, tex_size[0]), (width << 16) | height);
}


static inline void write_shorts(rspq_write_t *w, const uint16_t *s, uint32_t count)
{
    for (uint32_t i = 0; i < count; i += 2)
    {
        uint32_t packed = ((uint32_t)s[i] << 16) | (uint32_t)s[i+1];
        rspq_write_arg(w, packed);
    }
}

static inline void gpu_matrix_write(rspq_write_t* w, const float* m)
{
    uint16_t integer[16];
    uint16_t fraction[16];

    for (uint32_t i = 0; i < 16; i++)
    {
        int32_t fixed = m[i] * (1<<16);
        integer[i] = (uint16_t)((fixed & 0xFFFF0000) >> 16);
        fraction[i] = (uint16_t)(fixed & 0x0000FFFF);
    }

    write_shorts(w, integer, 16);
    write_shorts(w, fraction, 16);
}

static void gpuLoadMatrix(const float* m)
{
    rspq_write_t w = rspq_write_begin(gpup_id, GPU_CMD_MATRIX_LOAD, 17);
    rspq_write_arg(&w, false); // no multiply
    gpu_matrix_write(&w, m);
    rspq_write_end(&w);
}

static inline void put_word(rspq_write_t* s, uint16_t v1, uint16_t v2)
{
	rspq_write_arg(s, v2 | (v1 << 16));
}

static void upload_vertex(uint32_t index, uint8_t cache_index)
{
    rspq_write_t s = rspq_write_begin(gpup_id, GPU_CMD_UPLOAD_VTX, 6);
    rspq_write_arg(&s, cache_index * PRIM_VTX_SIZE);
	char* ptr = gpu_pointer + index * gpu_stride;

	float* vtx = (float*)(ptr + 0);
	put_word(&s, vtx[0] * (1<<VTX_SHIFT),
				 vtx[1] * (1<<VTX_SHIFT));
	put_word(&s, vtx[2] * (1<<VTX_SHIFT),
				 1.0f   * (1<<VTX_SHIFT));

	uint8_t* col = (uint8_t*)(ptr + 12); // TODO put_byte ?
	put_word(&s, col[0] << 7,
				 col[1] << 7);
	put_word(&s, col[2] << 7,
				 col[3] << 7);

	if (gpu_texturing) {
		float* tex = (float*)(ptr + 16);
		put_word(&s, tex[0] * (1<<TEX_SHIFT),
					 tex[1] * (1<<TEX_SHIFT));
	} else {
		put_word(&s, 0,
					 0);
    }
    rspq_write_end(&s);
}

static void gpuDrawArrays(uint32_t first, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        uint8_t cache_index = i % VERTEX_CACHE_SIZE;
        upload_vertex(first + i, cache_index);

		// Last vertex of quad?
		if ((i & 3) != 3) continue;

		// Add two triangles
		uint8_t idx = cache_index - 3;
		gpu_draw_triangle(idx + 0, idx + 1, idx + 2);
		gpu_draw_triangle(idx + 0, idx + 2, idx + 3);
    }
}

static void gpuDepthRange(float n, float f)
{
    gpu_vp_scale[2]  = (f - n) * 0.5f;
    gpu_vp_offset[2] = n + (f - n) * 0.5f;

    gpu_set_short(offsetof(gpu_state, vp_scale[2]),  gpu_vp_scale[2]  * 4);
    gpu_set_short(offsetof(gpu_state, vp_offset[2]), gpu_vp_offset[2] * 4);
}

static void gpuViewport(int x, int y, int w, int h)
{
    gpu_vp_scale[0]  = w * 0.5f;
    gpu_vp_scale[1]  = h * -0.5f;
    gpu_vp_offset[0] = x + w * 0.5f;
    gpu_vp_offset[1] = y + h * 0.5f;

    // Screen coordinates are s13.2
    #define SCREEN_XY_SCALE   4.0f
    #define SCREEN_Z_SCALE    32767.0f

    // * 2.0f to compensate for RSP reciprocal missing 1 bit
    uint16_t scale_x  = gpu_vp_scale[0] * SCREEN_XY_SCALE * 2.0f;
    uint16_t scale_y  = gpu_vp_scale[1] * SCREEN_XY_SCALE * 2.0f;
    uint16_t scale_z  = gpu_vp_scale[2] * SCREEN_Z_SCALE  * 2.0f;

    uint16_t offset_x = gpu_vp_offset[0] * SCREEN_XY_SCALE;
    uint16_t offset_y = gpu_vp_offset[1] * SCREEN_XY_SCALE;
    uint16_t offset_z = gpu_vp_offset[2] * SCREEN_Z_SCALE;

    gpu_set_long( 
        offsetof(gpu_state, vp_scale), 
        ((uint64_t)scale_x << 48) | ((uint64_t)scale_y << 32) | ((uint64_t)scale_z << 16));
    gpu_set_long( 
        offsetof(gpu_state, vp_offset), 
        ((uint64_t)offset_x << 48) | ((uint64_t)offset_y << 32) | ((uint64_t)offset_z << 16));
}

static void gpuSetCullFace(bool enabled) {
	// 1 = cull backfaces
	// 2 = don't cull
    gpu_set_short(offsetof(gpu_state, tri_cull), enabled ? 1 : 2);
}

static void gpu_init() {
    gpup_id = rspq_overlay_register(&rsp_gpu);
    gpuDepthRange(0, 1);
}

static void gpu_close() {
    rspq_wait();
    rspq_overlay_unregister(gpup_id);
}
