// https://patents.google.com/patent/US6618048
// https://patents.google.com/patent/US6639595B1 - general
// https://patents.google.com/patent/US6700586 - Transform Functions (XF) unit mostly
// https://patents.google.com/patent/US7002591 - general
// https://patents.google.com/patent/US6697074 - Command Processor (CP) unit + opcodes
// https://patents.google.com/patent/US7995069B2 - Blitter Processor (BP) unit
// https://patents.google.com/patent/US20070165043A1 - FIFO unit
// https://patents.google.com/patent/US7034828B1 - some texture functions
// https://patents.google.com/patent/US6664962B1 - some texture functions
// yet another gamecube documentation
// dolphin headers

typedef struct { float m[4][4]; } GX_matrix_4x4;
typedef struct { float m[3][4]; } GX_matrix_3x4;
typedef struct { float m[2][4]; } GX_matrix_2x4;

#define ENTRIES_2x4  8
#define ENTRIES_3x4 12
#define ENTRIES_4x4 16


/*########################################################################################################################*
*----------------------------------------------------FIFO functionality---------------------------------------------------*
*#########################################################################################################################*/
#define FIFO_PUSH_I32(value) wgPipe->I32 = value;
#define FIFO_PUSH_F32(value) wgPipe->F32 = value;
#define FIFO_PUSH_U32(value) wgPipe->U32 = value;

#define FIFO_PUSH_U8(value) wgPipe->U8 = value;

#define FIFO_PUSH_F32x2(x, y)       FIFO_PUSH_F32(x); FIFO_PUSH_F32(y);
#define FIFO_PUSH_F32x3(x, y, z)    FIFO_PUSH_F32(x); FIFO_PUSH_F32(y); FIFO_PUSH_F32(z);
#define FIFO_PUSH_F32x4(x, y, z, w) FIFO_PUSH_F32(x); FIFO_PUSH_F32(y); FIFO_PUSH_F32(z); FIFO_PUSH_F32(w);

#define FIFO_PUSH_MTX_2x4(mtx) \
	FIFO_PUSH_F32x4(mtx->m[0][0], mtx->m[0][1], mtx->m[0][2], mtx->m[0][3]); \
	FIFO_PUSH_F32x4(mtx->m[1][0], mtx->m[1][1], mtx->m[1][2], mtx->m[1][3]); \

#define FIFO_PUSH_MTX_3x4(mtx) \
	FIFO_PUSH_F32x4(mtx->m[0][0], mtx->m[0][1], mtx->m[0][2], mtx->m[0][3]); \
	FIFO_PUSH_F32x4(mtx->m[1][0], mtx->m[1][1], mtx->m[1][2], mtx->m[1][3]); \
	FIFO_PUSH_F32x4(mtx->m[2][0], mtx->m[2][1], mtx->m[2][2], mtx->m[2][3]); \


/*########################################################################################################################*
*------------------------------------------------Command processor commands-----------------------------------------------*
*#########################################################################################################################*/
enum {
	// No operation
	CP_CMD_NOP           = 0x00,
	// Loads Command Processor register
	CP_CMD_LOAD_CP_REG   = 0x08,
	// Loads Transform Unit register(s)
	CP_CMD_LOAD_XF_REGS  = 0x10,
	// Runs all commands in a display list
	CP_CMD_CALL_DISPLIST = 0x40,
	// Loads Blitting Processor register
	CP_CMD_LOAD_BP_REG   = 0x61,
	// Primitive command types
	CP_CMD_PRIM_BASE     = 0x80,
	// TODO missing indexed + stat commands
};

#define GX_NOP() \
	FIFO_PUSH_U8(CP_CMD_NOP);
	
#define GX_LOAD_XF_REGISTERS(start_reg, num_regs) \
	FIFO_PUSH_U8(CP_CMD_LOAD_XF_REGS); \
	FIFO_PUSH_U32( ((((num_regs) - 1) & 0xFFFF) << 16) | (start_reg) ); \


/*########################################################################################################################*
*-----------------------------------------------Transform Function registers----------------------------------------------*
*#########################################################################################################################*/
enum {
	// https://www.gc-forever.com/yagcd/chap5.html#sec5.11.3
	// 256 floats -> 64 rows of 4xFloats -> ~21 entries of 3x4Matrices
	XF_REG_BASE_MATRICES_BEG = 0x0000,
	XF_REG_BASE_MATRICES_END = 0x0100,
	#define XF_REG_BASE_MATRIX(idx) XF_REG_BASE_MATRICES_BEG + ((idx) * ENTRIES_3x4)
};

/*########################################################################################################################*
*-----------------------------------------------Transform Function commands-----------------------------------------------*
*#########################################################################################################################*/
typedef enum {
	XF_BASE_MATRIX00, XF_BASE_MATRIX01, XF_BASE_MATRIX02,
	XF_BASE_MATRIX03, XF_BASE_MATRIX04, XF_BASE_MATRIX05,
	XF_BASE_MATRIX06, XF_BASE_MATRIX07, XF_BASE_MATRIX08,
	XF_BASE_MATRIX09, XF_BASE_MATRIX10, XF_BASE_MATRIX11,
	XF_BASE_MATRIX12, XF_BASE_MATRIX13, XF_BASE_MATRIX14,
	XF_BASE_MATRIX15, XF_BASE_MATRIX16, XF_BASE_MATRIX17,
	XF_BASE_MATRIX18, XF_BASE_MATRIX19, XF_BASE_MATRIX20,
	
	// Default mapping, allocate first 10 matrices for positions
	// (that way can use the same index as a corresponding normal matrix) 
	XF_POS_MATRIX0 = XF_BASE_MATRIX00, 
	XF_POS_MATRIX1 = XF_BASE_MATRIX01,
	XF_POS_MATRIX2 = XF_BASE_MATRIX02,
	XF_POS_MATRIX3 = XF_BASE_MATRIX03,
	XF_POS_MATRIX4 = XF_BASE_MATRIX04,
	XF_POS_MATRIX5 = XF_BASE_MATRIX05,
	XF_POS_MATRIX6 = XF_BASE_MATRIX06,
	XF_POS_MATRIX7 = XF_BASE_MATRIX07,
	XF_POS_MATRIX8 = XF_BASE_MATRIX08,
	XF_POS_MATRIX9 = XF_BASE_MATRIX09,

	// Default mapping, allocate rest of matrices for textures
	// (avoids overlapping with position matrices)
	XF_TEX_MATRIX0 = XF_BASE_MATRIX10, 
	XF_TEX_MATRIX1 = XF_BASE_MATRIX11,
	XF_TEX_MATRIX2 = XF_BASE_MATRIX12,
	XF_TEX_MATRIX3 = XF_BASE_MATRIX13,
	XF_TEX_MATRIX4 = XF_BASE_MATRIX14,
	XF_TEX_MATRIX5 = XF_BASE_MATRIX15,
	XF_TEX_MATRIX6 = XF_BASE_MATRIX16,
	XF_TEX_MATRIX7 = XF_BASE_MATRIX17,
	XF_TEX_MATRIX8 = XF_BASE_MATRIX18,
	XF_TEX_MATRIX9 = XF_BASE_MATRIX19,
} XF_BASE_MATRIX_INDEX;

static CC_INLINE void XF_SetMatrix_3x4(const GX_matrix_3x4* matrix, XF_BASE_MATRIX_INDEX index)
{
	GX_LOAD_XF_REGISTERS(XF_REG_BASE_MATRIX(index), ENTRIES_3x4);
	FIFO_PUSH_MTX_3x4(matrix);
}

static CC_INLINE void XF_SetMatrix_2x4(const GX_matrix_2x4* matrix, XF_BASE_MATRIX_INDEX index)
{
	GX_LOAD_XF_REGISTERS(XF_REG_BASE_MATRIX(index), ENTRIES_2x4);
	FIFO_PUSH_MTX_2x4(matrix);
}
