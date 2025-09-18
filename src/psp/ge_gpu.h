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
	GE_SET_VB_ADDR              = 0x01,
	GE_SET_IB_ADDR              = 0x02,
	GE_SET_BASE_ADDR            = 0x10,
	GE_SET_VERTEX_FORMAT        = 0x12,
	GE_WORLDMATRIX_UPLOAD_INDEX = 0x3A,
	GE_WORLDMATRIX_UPLOAD_DATA  = 0x3B,	
	GE_VIEW_MATRIX_UPLOAD_INDEX = 0x3C,
	GE_VIEW_MATRIX_UPLOAD_DATA  = 0x3D,	
	GE_PROJ_MATRIX_UPLOAD_INDEX = 0x3E,
	GE_PROJ_MATRIX_UPLOAD_DATA  = 0x3F,

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
*----------------------------------------------------Vertex drawing-------------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE void GE_set_vertex_format(int format) {
    GE_PushI(GE_SET_VERTEX_FORMAT, format);
}
