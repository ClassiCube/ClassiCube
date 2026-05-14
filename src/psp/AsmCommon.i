// ===============================
// FUNCTION MACROS
// ===============================
#define BEG_FUNC(name_) \
	.global name_; \
	.align 4; \
	name_:

#define END_FUNC(name_) \
	.size name_, . - name_; \
	.type name_, %function;


// ===============================
// REGISTER USAGE
// ===============================
// M0 - view matrix
// M1 - proj matrix
// M2 - MVP  matrix
// M3 - row1 = guardband scaling
// M4 - ?
// M5 - temp
// M6 - temp
// M7 - temp


// ===============================
// GLOBAL REGISTERS
// ===============================
#define MVP_CX  C200 // mvp.columns.X
#define MVP_CY  C210 // mvp.columns.Y
#define MVP_CZ  C220 // mvp.columns.Z
#define MVP_CW  C230 // mvp.columns.W

#define GBAND_X S300
#define GBAND_Y S301
#define GBINV_X S302
#define GBINV_Y S303


// ===============================
// VERTEX FIELD OFFSETS
// ===============================
#define VTEX_U  0
#define VTEX_V  4
#define VTEX_C  8
#define VTEX_X 12
#define VTEX_Y 16
#define VTEX_Z 20

#define VCOL_C  0
#define VCOL_X  4
#define VCOL_Y  8
#define VCOL_Z 12

#define VCLP_X  0
#define VCLP_Y  4
#define VCLP_Z  8
#define VCLP_W 12
#define VCLP_U 16
#define VCLP_V 20
#define VCLP_C 24
#define VCLP_F 28

#define VCLP_XYZW  0
#define VCLP_UVCF 16

// ===============================
// VERTEX STRUCT SIZES
// ===============================
#define S_VCOL 16 // coloured vertex
#define S_VTEX 24 // textured vertex
#define S_VCLP 32 // clipping vertex
#define S_VCLP_LOG2 5 // x << 5 = x * 32
