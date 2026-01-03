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
// M3 - clip planes L,R,B,T
// M4 - clip planes N,F
// M5 - temp
// M6 - temp
// M7 - temp


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

#define POLY_X  0
#define POLY_Y  4
#define POLY_Z  8
#define POLY_W 12
#define POLY_U 16
#define POLY_V 20
#define POLY_C 24
#define POLY_F 28

#define POLY_XYZW  0
#define POLY_UVCF 16

// ===============================
// VERTEX STRUCT SIZES
// ===============================
#define S_VCOL 16
#define S_VTEX 24
#define S_POLY 32
#define S_POLY_LOG2 5 // x << 5 = x * 32
