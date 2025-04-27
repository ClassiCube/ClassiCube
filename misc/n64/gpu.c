#include "GL/gl.h"
#include "rspq.h"
#include "rdpq.h"
#include "rdpq_rect.h"
#include "rdpq_mode.h"
#include "rdpq_debug.h"
#include "display.h"
#include "rdp.h"
#include <string.h>
#include <math.h>
#include <malloc.h>
#include "gl_constants.h"

// This is a severely cutdown version of libdragon's OpenGL implementation

static uint32_t glp_id;
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
    GPU_CMD_SET_FLAG         = 0x0,
    GPU_CMD_SET_BYTE         = 0x1,
    GPU_CMD_SET_SHORT        = 0x2,
    GPU_CMD_SET_WORD         = 0x3,
    GPU_CMD_SET_LONG         = 0x4,

    GPU_CMD_DRAW_TRI         = 0x5,
    GPU_CMD_UPLOAD_VTX       = 0x6,

    GPU_CMD_MATRIX_LOAD      = 0x7,
    GPU_CMD_PRE_INIT_PIPE    = 0x8,
};

enum {
    ATTRIB_VERTEX,
    ATTRIB_COLOR,
    ATTRIB_TEXCOORD,
    ATTRIB_COUNT
};

typedef struct {
    GLfloat scale[3];
    GLfloat offset[3];
} gl_viewport_t;

typedef struct {
    int16_t  i[4][4];
    uint16_t f[4][4];
} gl_matrix_srv_t;
_Static_assert(sizeof(gl_matrix_srv_t) == MATRIX_SIZE, "Matrix size does not match");

typedef struct {
    rspq_write_t w;
    union {
        uint8_t  bytes[4];
        uint32_t word;
    };
    uint32_t buffer_head;
} gl_cmd_stream_t;

typedef struct {
    GLsizei stride;
    const GLvoid *pointer;
    bool enabled;
} gl_array_t;

typedef struct {
    gl_matrix_srv_t mvp_matrix;
    int16_t viewport_scale[4];
    int16_t viewport_offset[4];
    uint32_t flags;
    uint16_t tex_size[2];
    uint16_t tex_offset[2];
    uint16_t tri_cmd;
    uint16_t tri_cull;
} __attribute__((aligned(8), packed)) gl_server_state_t;

static inline const void *gl_get_attrib_element(const gl_array_t *src, uint32_t index)
{
    return src->pointer + index * src->stride;
}

static inline gl_cmd_stream_t gl_cmd_stream_begin(uint32_t ovl_id, uint32_t cmd_id, int size)
{
    return (gl_cmd_stream_t) {
        .w = rspq_write_begin(ovl_id, cmd_id, size),
        .buffer_head = 2,
    };
}

static inline void gl_cmd_stream_commit(gl_cmd_stream_t *s)
{
    rspq_write_arg(&s->w, s->word);
    s->buffer_head = 0;
    s->word = 0;
}

static inline void gl_cmd_stream_put_half(gl_cmd_stream_t *s, uint16_t v)
{
    s->bytes[s->buffer_head++] = v >> 8;
    s->bytes[s->buffer_head++] = v & 0xFF;
    
    if (s->buffer_head == sizeof(uint32_t)) {
        gl_cmd_stream_commit(s);
    }
}

static inline void gl_cmd_stream_end(gl_cmd_stream_t *s)
{
    if (s->buffer_head > 0) {
        gl_cmd_stream_commit(s);
    }

    rspq_write_end(&s->w);
}

__attribute__((always_inline))
static inline void gl_set_flag_raw(uint32_t offset, uint32_t flag, bool value)
{
    rspq_write(glp_id, GPU_CMD_SET_FLAG, offset | value, value ? flag : ~flag);
}

__attribute__((always_inline))
static inline void gl_set_flag(uint32_t flag, bool value)
{
    gl_set_flag_raw(offsetof(gl_server_state_t, flags), flag, value);
}

__attribute__((always_inline))
static inline void gl_set_byte(uint32_t offset, uint8_t value)
{
    rspq_write(glp_id, GPU_CMD_SET_BYTE, offset, value);
}

__attribute__((always_inline))
static inline void gl_set_short(uint32_t offset, uint16_t value)
{
    rspq_write(glp_id, GPU_CMD_SET_SHORT, offset, value);
}

__attribute__((always_inline))
static inline void gl_set_word(uint32_t offset, uint32_t value)
{
    rspq_write(glp_id, GPU_CMD_SET_WORD, offset, value);
}

__attribute__((always_inline))
static inline void gl_set_long(uint32_t offset, uint64_t value)
{
    rspq_write(glp_id, GPU_CMD_SET_LONG, offset, value >> 32, value & 0xFFFFFFFF);
}

static inline void glpipe_draw_triangle(int i0, int i1, int i2)
{
    // We pass -1 because the triangle can be clipped and split into multiple
    // triangles.
    rdpq_write(-1, glp_id, GPU_CMD_DRAW_TRI,
        (i0*PRIM_VTX_SIZE),
        ((i1*PRIM_VTX_SIZE)<<16) | (i2*PRIM_VTX_SIZE)
    );
}


static gl_viewport_t state_viewport;
static gl_array_t    state_arrays[ATTRIB_COUNT];

void gl_init()
{
    glp_id = rspq_overlay_register(&rsp_gpu);
    glDepthRange(0, 1);
}

void gl_close()
{
    rspq_wait();
    rspq_overlay_unregister(glp_id);
}

void gl_set_flag2(GLenum target, bool value)
{
    switch (target) {
    case GL_DEPTH_TEST:
        gl_set_flag(FLAG_DEPTH_TEST, value);
        break;
    case GL_TEXTURE_2D:
        gl_set_flag(FLAG_TEXTURE_ACTIVE, value);
        break;
    }
}

void glEnable(GLenum target)
{
    gl_set_flag2(target, true);
}

void glDisable(GLenum target)
{
    gl_set_flag2(target, false);
}

void glTexSizeN64(uint16_t width, uint16_t height)
{
    gl_set_word(offsetof(gl_server_state_t, tex_size[0]), (width << 16) | height);
}


static inline void write_shorts(rspq_write_t *w, const uint16_t *s, uint32_t count)
{
    for (uint32_t i = 0; i < count; i += 2)
    {
        uint32_t packed = ((uint32_t)s[i] << 16) | (uint32_t)s[i+1];
        rspq_write_arg(w, packed);
    }
}

static inline void gl_matrix_write(rspq_write_t *w, const GLfloat *m)
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

void glLoadMatrixf(const GLfloat *m)
{
    rspq_write_t w = rspq_write_begin(glp_id, GPU_CMD_MATRIX_LOAD, 17);
    rspq_write_arg(&w, false); // no multiply
    gl_matrix_write(&w, m);
    rspq_write_end(&w);
}

static void upload_vertex(const gl_array_t *arrays, uint32_t index, uint8_t cache_index)
{
    gl_cmd_stream_t s = gl_cmd_stream_begin(glp_id, GPU_CMD_UPLOAD_VTX, 6);
    gl_cmd_stream_put_half(&s, cache_index * PRIM_VTX_SIZE);

	const float* vtx = gl_get_attrib_element(&arrays[ATTRIB_VERTEX], index);
	gl_cmd_stream_put_half(&s, vtx[0] * (1<<VTX_SHIFT));
	gl_cmd_stream_put_half(&s, vtx[1] * (1<<VTX_SHIFT));
	gl_cmd_stream_put_half(&s, vtx[2] * (1<<VTX_SHIFT));
	gl_cmd_stream_put_half(&s, 1.0f   * (1<<VTX_SHIFT));

	const uint8_t* col = gl_get_attrib_element(&arrays[ATTRIB_COLOR], index);
	gl_cmd_stream_put_half(&s, col[0] << 7); // TODO put_byte ?
	gl_cmd_stream_put_half(&s, col[1] << 7); // TODO put_byte ?
	gl_cmd_stream_put_half(&s, col[2] << 7); // TODO put_byte ?
	gl_cmd_stream_put_half(&s, col[3] << 7); // TODO put_byte ?

	if (arrays[ATTRIB_TEXCOORD].enabled) {
		const float* tex = gl_get_attrib_element(&arrays[ATTRIB_TEXCOORD], index);
		gl_cmd_stream_put_half(&s, tex[0] * (1<<TEX_SHIFT));
		gl_cmd_stream_put_half(&s, tex[1] * (1<<TEX_SHIFT));
	} else {
		gl_cmd_stream_put_half(&s, 0);
		gl_cmd_stream_put_half(&s, 0);
    }
    gl_cmd_stream_end(&s);
}

static void gl_rsp_draw_arrays(uint32_t first, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        uint8_t cache_index = i % VERTEX_CACHE_SIZE;
        upload_vertex(state_arrays, first + i, cache_index);

		// Last vertex of quad?
		if ((i & 3) != 3) continue;

		// Add two triangles
		uint8_t idx = cache_index - 3;
		glpipe_draw_triangle(idx + 0, idx + 1, idx + 2);
		glpipe_draw_triangle(idx + 0, idx + 2, idx + 3);
    }
}

int gl_array_type_from_enum(GLenum array)
{
    switch (array) {
    case GL_VERTEX_ARRAY:        return ATTRIB_VERTEX;
    case GL_TEXTURE_COORD_ARRAY: return ATTRIB_TEXCOORD;
    case GL_COLOR_ARRAY:         return ATTRIB_COLOR;
    default:                     return -1;
    }
}

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    gl_array_t *array = &state_arrays[ATTRIB_VERTEX];
    array->stride = stride;
    array->pointer = pointer;
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    gl_array_t *array = &state_arrays[ATTRIB_TEXCOORD];
    array->stride = stride;
    array->pointer = pointer;
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    gl_array_t *array = &state_arrays[ATTRIB_COLOR];
    array->stride = stride;
    array->pointer = pointer;
}

void gl_set_array_enabled(int array_type, bool enabled)
{
    state_arrays[array_type].enabled = enabled;
}

void glEnableClientState(GLenum array)
{
    gl_set_array_enabled(gl_array_type_from_enum(array), true);
}

void glDisableClientState(GLenum array)
{
    gl_set_array_enabled(gl_array_type_from_enum(array), false);
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    rspq_write(glp_id, GPU_CMD_PRE_INIT_PIPE);
    gl_rsp_draw_arrays(first, count);
}

void glDepthRange(GLclampd n, GLclampd f)
{
    state_viewport.scale[2]  = (f - n) * 0.5f;
    state_viewport.offset[2] = n + (f - n) * 0.5f;

    gl_set_short( 
        offsetof(gl_server_state_t, viewport_scale) + sizeof(int16_t) * 2, 
        state_viewport.scale[2] * 4);
    gl_set_short( 
        offsetof(gl_server_state_t, viewport_offset) + sizeof(int16_t) * 2, 
        state_viewport.offset[2] * 4);
}

void glViewport(GLint x, GLint y, GLsizei w, GLsizei h)
{
    state_viewport.scale[0]  = w * 0.5f;
    state_viewport.scale[1]  = h * -0.5f;
    state_viewport.offset[0] = x + w * 0.5f;
    state_viewport.offset[1] = y + h * 0.5f;

    // Screen coordinates are s13.2
    #define SCREEN_XY_SCALE   4.0f
    #define SCREEN_Z_SCALE    32767.0f

    // * 2.0f to compensate for RSP reciprocal missing 1 bit
    uint16_t scale_x  = state_viewport.scale[0] * SCREEN_XY_SCALE * 2.0f;
    uint16_t scale_y  = state_viewport.scale[1] * SCREEN_XY_SCALE * 2.0f;
    uint16_t scale_z  = state_viewport.scale[2] * SCREEN_Z_SCALE  * 2.0f;

    uint16_t offset_x = state_viewport.offset[0] * SCREEN_XY_SCALE;
    uint16_t offset_y = state_viewport.offset[1] * SCREEN_XY_SCALE;
    uint16_t offset_z = state_viewport.offset[2] * SCREEN_Z_SCALE;

    gl_set_long( 
        offsetof(gl_server_state_t, viewport_scale), 
        ((uint64_t)scale_x << 48) | ((uint64_t)scale_y << 32) | ((uint64_t)scale_z << 16));
    gl_set_long( 
        offsetof(gl_server_state_t, viewport_offset), 
        ((uint64_t)offset_x << 48) | ((uint64_t)offset_y << 32) | ((uint64_t)offset_z << 16));
}

void glCullFace(GLenum mode)
{
	// 1 = cull backfaces
	// 2 = don't cull
    gl_set_short(offsetof(gl_server_state_t, tri_cull), mode ? 1 : 2);
}
