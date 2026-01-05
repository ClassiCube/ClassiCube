#include "../Funcs.h"

// https://github.com/pspdev/pspsdk/blob/c84807b8503fdf031d089cc0c6363d2d87715974/src/gu/doc/commands.txt
// Hacky way to access GU's command buffer 
extern struct GuDisplayListInternal
{
	unsigned int* start;
	unsigned int* current;
} *gu_list;

enum GE_COMMANDS {
	GE_NOP                      = 0x00,
	GE_SET_REL_VADDR            = 0x01,
	GE_SET_REL_IADDR            = 0x02,
	GE_SET_ADDR_BASE            = 0x10,
	GE_SET_VERTEX_FORMAT        = 0x12,
	GE_WORLDMATRIX_UPLOAD_INDEX = 0x3A,
	GE_WORLDMATRIX_UPLOAD_DATA  = 0x3B,	
	GE_VIEW_MATRIX_UPLOAD_INDEX = 0x3C,
	GE_VIEW_MATRIX_UPLOAD_DATA  = 0x3D,	
	GE_PROJ_MATRIX_UPLOAD_INDEX = 0x3E,
	GE_PROJ_MATRIX_UPLOAD_DATA  = 0x3F,

	GE_SET_VIEWPORT_X_SCALE     = 0x42,
	GE_SET_VIEWPORT_Y_SCALE     = 0x43,
	GE_SET_VIEWPORT_Z_SCALE     = 0x44,
	GE_SET_VIEWPORT_X_ORIGIN    = 0x45,
	GE_SET_VIEWPORT_Y_ORIGIN    = 0x46,
	GE_SET_VIEWPORT_Z_ORIGIN    = 0x47,

	GE_SET_SCREEN_OFFSET_X      = 0x4C,
	GE_SET_SCREEN_OFFSET_Y      = 0x4D,

	GE_SET_Z_RANGE_MIN          = 0xD6,
	GE_SET_Z_RANGE_MAX          = 0xD7,

	GE_SET_DEPTH_MASK           = 0xE7,
	GE_SET_COLOR_MASK           = 0xE8,
	GE_SET_ALPHA_MASK           = 0xE9, 	
};


// upper 8 bits are opcode, lower 24 bits are argument
#define GE_OPCODE_SHIFT 24
static CC_INLINE void GE_PushI(int cmd, int argument) {
	*(gu_list->current++) = (cmd << GE_OPCODE_SHIFT) | (argument & 0xffffff);
}

static CC_INLINE void GE_PushF(int cmd, float argument) {
	union IntAndFloat raw; raw.f = argument;
	*(gu_list->current++) = (cmd << GE_OPCODE_SHIFT) | (raw.u >> 8);
}


/*########################################################################################################################*
*-----------------------------------------------------Matrix upload-------------------------------------------------------*
*#########################################################################################################################*/
static void GE_upload_world_matrix(const float* matrix) {
	GE_PushI(GE_WORLDMATRIX_UPLOAD_INDEX, 0); // uploading all 3x4 entries

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 3; j++)
	{
		GE_PushF(GE_WORLDMATRIX_UPLOAD_DATA, matrix[(i * 4) + j]);
	}
}

static void GE_upload_view_matrix(const float* matrix) {
	GE_PushI(GE_VIEW_MATRIX_UPLOAD_INDEX, 0); // uploading all 3x4 entries

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 3; j++)
	{
		GE_PushF(GE_VIEW_MATRIX_UPLOAD_DATA, matrix[(i * 4) + j]);
	}
}

static void GE_upload_proj_matrix(const float* matrix) {
	GE_PushI(GE_PROJ_MATRIX_UPLOAD_INDEX, 0); // uploading all 4x4 entries

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
	{
		GE_PushF(GE_PROJ_MATRIX_UPLOAD_DATA, matrix[(i * 4) + j]);
	}
}


/*########################################################################################################################*
*----------------------------------------------------Viewport/Offset------------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE void GE_set_depth_range(unsigned short minZ, unsigned short maxZ) {
    GE_PushI(GE_SET_Z_RANGE_MIN, minZ);
    GE_PushI(GE_SET_Z_RANGE_MAX, maxZ);
}

static CC_INLINE void GE_set_screen_offset(int x, int y) {
    GE_PushI(GE_SET_SCREEN_OFFSET_X, x << 4);
    GE_PushI(GE_SET_SCREEN_OFFSET_Y, y << 4);
}

static CC_INLINE void GE_set_viewport_xy(int x, int y, int w, int h) {
	GE_PushF(GE_SET_VIEWPORT_X_SCALE, w *  0.5f);
	GE_PushF(GE_SET_VIEWPORT_Y_SCALE, h * -0.5f);
	GE_PushF(GE_SET_VIEWPORT_X_ORIGIN, x);
	GE_PushF(GE_SET_VIEWPORT_Y_ORIGIN, y);
}

static CC_INLINE void GE_set_viewport_z(int n, int f) {
	int middle = (n + f) / 2;
	GE_PushF(GE_SET_VIEWPORT_Z_SCALE,  middle - n);
	GE_PushF(GE_SET_VIEWPORT_Z_ORIGIN, middle);
}


/*########################################################################################################################*
*----------------------------------------------------Vertex drawing-------------------------------------------------------*
*#########################################################################################################################*/
static int last_base = -1;

static CC_INLINE void GE_set_vertex_format(int format) {
    GE_PushI(GE_SET_VERTEX_FORMAT, format);
}

// Don't redundantly set base address (by avoiding 2 calls to this per draw call, reduces frame GE commands by ~25%)
static CC_INLINE void GE_set_base_addr(cc_uintptr addr) {
	cc_uintptr base = (addr >> 8) & 0xf0000;
	if (base == last_base) return;

	GE_PushI(GE_SET_ADDR_BASE, base);
	last_base = base;
}

static CC_INLINE void GE_set_vertices(const void* vertices) {
	cc_uintptr addr = (cc_uintptr)vertices;
	GE_set_base_addr(addr);
    GE_PushI(GE_SET_REL_VADDR, addr);
}

static CC_INLINE void GE_set_indices(const void* indices) {
	cc_uintptr addr = (cc_uintptr)indices;
	GE_set_base_addr(addr);
    GE_PushI(GE_SET_REL_IADDR, addr);
}
