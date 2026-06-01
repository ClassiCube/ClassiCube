#include "../Funcs.h"

// https://github.com/pspdev/pspsdk/blob/c84807b8503fdf031d089cc0c6363d2d87715974/src/gu/doc/commands.txt
// Hacky way to access GU's command buffer 
extern struct GuDisplayListInternal
{
	unsigned int* start;
	unsigned int* current;
} *gu_list;
extern int ge_list_executed[2];

enum GE_COMMANDS {
	GE_NOP                      = 0x00,
	GE_SET_REL_VADDR            = 0x01,
	GE_SET_REL_IADDR            = 0x02,
	GE_DRAW_PRIMITIVES			= 0x04,
	GE_JUMP_TO_ADDR				= 0x08,
	GE_SET_ADDR_BASE            = 0x10,
	GE_SET_VERTEX_FORMAT        = 0x12,	

	GE_SET_DRAW_REGION_TL       = 0x15,
	GE_SET_DRAW_REGION_BR       = 0x16,		

	GE_SET_FACE_CULLING         = 0x1D,
	GE_SET_TEXTURING            = 0x1E,
	GE_SET_FOG_ACTIVE           = 0x1F,
	GE_SET_ALPHA_BLENDING       = 0x21,
	GE_SET_ALPHA_TESTING        = 0x22,
	GE_SET_DEPTH_TESTING        = 0x23,	

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

	GE_SET_TEX_OFFSET_X         = 0x4A,
	GE_SET_TEX_OFFSET_Y         = 0x4B,

	GE_SET_SCREEN_OFFSET_X      = 0x4C,
	GE_SET_SCREEN_OFFSET_Y      = 0x4D,

	GE_SET_CLUT_BUFFER_PTR_LO	= 0xB0,
	GE_SET_CLUT_BUFFER_PTR_HI	= 0xB1,

	GE_LOAD_CLUT_ENTRIES		= 0xC4,

	GE_SET_FOG_BIAS				= 0xCD,
	GE_SET_FOG_STEP				= 0xCE,
	GE_SET_FOG_COLOR            = 0xCF,
	
	GE_SET_CLEARING_STATE		= 0xD3,	
	GE_SET_SCISSOR_TL           = 0xD4,
	GE_SET_SCISSOR_BR           = 0xD5,
	GE_SET_Z_RANGE_MIN          = 0xD6,
	GE_SET_Z_RANGE_MAX          = 0xD7,

	GE_SET_DEPTH_MASK           = 0xE7,
	GE_SET_COLOR_MASK           = 0xE8,
	GE_SET_ALPHA_MASK           = 0xE9, 	
};

// Packs lower 24 bits of value into all 24 bits of command arg
#define GE_PACK_LO(value) (((cc_uintptr)(value)) & 0xffffff)
// Packs higher 8 bits of value into highest 8 bits of command arg
#define GE_PACK_HI(value) ((((cc_uintptr)(value)) >> 8) & 0xf0000)


// upper 8 bits are opcode, lower 24 bits are argument
#define GE_OPCODE_SHIFT 24
static CC_INLINE void GE_PushI(int cmd, int argument) {
	*(gu_list->current++) = (cmd << GE_OPCODE_SHIFT) | GE_PACK_LO(argument);
}

static CC_INLINE void GE_PushF(int cmd, float argument) {
	union IntAndFloat raw; raw.f = argument;
	*(gu_list->current++) = (cmd << GE_OPCODE_SHIFT) | (raw.u >> 8);
}

// Updates the stall address (end address that GE can process commands up to)
static CC_INLINE void GE_UpdateStallAddr(void) {
	sceGeListUpdateStallAddr(ge_list_executed[0], gu_list->current);			
}


/*########################################################################################################################*
*-----------------------------------------------------Matrix upload-------------------------------------------------------*
*#########################################################################################################################*/
// sets the world matrix
static void GE_upload_world_matrix(const float* matrix) {
	GE_PushI(GE_WORLDMATRIX_UPLOAD_INDEX, 0); // uploading all 3x4 entries

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 3; j++)
	{
		GE_PushF(GE_WORLDMATRIX_UPLOAD_DATA, matrix[(i * 4) + j]);
	}
}

// sets the view matrix
static void GE_upload_view_matrix(const float* matrix) {
	GE_PushI(GE_VIEW_MATRIX_UPLOAD_INDEX, 0); // uploading all 3x4 entries

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 3; j++)
	{
		GE_PushF(GE_VIEW_MATRIX_UPLOAD_DATA, matrix[(i * 4) + j]);
	}
}

// sets the projection matrix
static void GE_upload_proj_matrix(const float* matrix) {
	GE_PushI(GE_PROJ_MATRIX_UPLOAD_INDEX, 0); // uploading all 4x4 entries

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
	{
		GE_PushF(GE_PROJ_MATRIX_UPLOAD_DATA, matrix[(i * 4) + j]);
	}
}


/*########################################################################################################################*
*----------------------------------------------------------CLUT-----------------------------------------------------------*
*#########################################################################################################################*/
// sets CLUT buffer pointer that can be subsequently loaded from
static CC_INLINE void GE_set_clut_buffer(const void* ptr) {
	cc_uintptr addr = (cc_uintptr)ptr;
	// NOTE: addr must be 16 byte aligned

	GE_PushI(GE_SET_CLUT_BUFFER_PTR_LO, GE_PACK_LO(addr));
	GE_PushI(GE_SET_CLUT_BUFFER_PTR_HI, GE_PACK_HI(addr));
}

// loads CLUT/palette entries from CLUT buffer pointer
static CC_INLINE void GE_load_clut_entries(int num_entries) {
	// 8 entries per block
	GE_PushI(GE_LOAD_CLUT_ENTRIES, num_entries / 8);
}


/*########################################################################################################################*
*-------------------------------------------------------Texturing---------------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE void GE_set_texture_offset(float x, float y) {
	GE_PushF(GE_SET_TEX_OFFSET_X, x);
	GE_PushF(GE_SET_TEX_OFFSET_Y, y);
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

static CC_INLINE void GE_set_viewport_x(int origin, int scale) {
	GE_PushF(GE_SET_VIEWPORT_X_ORIGIN, origin);
	GE_PushF(GE_SET_VIEWPORT_X_SCALE,  scale);
}

static CC_INLINE void GE_set_viewport_y(int origin, int scale) {
	GE_PushF(GE_SET_VIEWPORT_Y_ORIGIN, origin);
	GE_PushF(GE_SET_VIEWPORT_Y_SCALE,  scale);
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
	cc_uintptr base = GE_PACK_HI(addr);
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

static CC_INLINE void GE_draw_array(int prim, int count) { 
	GE_PushI(GE_DRAW_PRIMITIVES, (prim << 16) | count);
	GE_UpdateStallAddr();
}

static CC_INLINE void GE_set_clearing_state(cc_bool clearing, int buffers) { 
	GE_PushI(GE_SET_CLEARING_STATE, (buffers << 8) | clearing);
}


/*########################################################################################################################*
*-----------------------------------------------------Vertex state--------------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE void GE_set_face_culling(int enabled) {
	GE_PushI(GE_SET_FACE_CULLING, enabled);
}


/*########################################################################################################################*
*-----------------------------------------------------Raster state--------------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE void GE_set_scissor_region(int x1, int y1, int x2, int y2) {
	int beg = (y1 << 10) | x1;
	int end = (y2 << 10) | x2;

	GE_PushI(GE_SET_SCISSOR_TL,     beg);
	GE_PushI(GE_SET_SCISSOR_BR,     end);
	GE_PushI(GE_SET_DRAW_REGION_TL,   0); // easier if it's just 0
	GE_PushI(GE_SET_DRAW_REGION_BR, end);
}

static CC_INLINE void GE_set_texturing(int enabled) {
	GE_PushI(GE_SET_TEXTURING, enabled);
}

static CC_INLINE void GE_set_depth_testing(int enabled) {
	GE_PushI(GE_SET_DEPTH_TESTING, enabled);
}

static CC_INLINE void GE_set_alpha_testing(int enabled) {
	GE_PushI(GE_SET_ALPHA_TESTING, enabled);
}

static CC_INLINE void GE_set_alpha_blending(int enabled) {
	GE_PushI(GE_SET_ALPHA_BLENDING, enabled);
}


static CC_INLINE void GE_set_fog_active(int enabled) {
	GE_PushI(GE_SET_FOG_ACTIVE, enabled);
}

static CC_INLINE void GE_set_fog_color(unsigned color) {
	GE_PushI(GE_SET_FOG_COLOR, color);
}

static CC_INLINE void GE_set_fog_range(float range) {
	float step = 1.0f / (range - 0.0f);

	GE_PushF(GE_SET_FOG_BIAS, range);
	GE_PushF(GE_SET_FOG_STEP, step);
}



/*########################################################################################################################*
*------------------------------------------------------Utilities----------------------------------------------------------*
*#########################################################################################################################*/
// Reserves space in the GE command list for temporary data
//   e.g. used to store clipping polygon vertices
// NOTE: size must be aligned to 4 bytes
static void* GE_ReserveListSpace(int size) {
	cc_uintptr addr = (cc_uintptr)gu_list->current;
	GE_set_base_addr(addr);

	// Reserve space for JUMP command and then data
	addr = (cc_uintptr)gu_list->current;
	addr += 4 + size;

	GE_PushI(GE_JUMP_TO_ADDR, GE_PACK_LO(addr));
	gu_list->current += size >> 2;

	GE_UpdateStallAddr();
	return (char*)gu_list->current - size;
}
