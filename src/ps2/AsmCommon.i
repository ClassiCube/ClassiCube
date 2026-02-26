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
// GLOBAL REGISTERS
// ===============================
#define V0001 $vf0 // hardware coded to (0,0,0,1)
#define MVP1  $vf1 // mvp.row1
#define MVP2  $vf2 // mvp.row2
#define MVP3  $vf3 // mvp.row3
#define MVP4  $vf4 // mvp.row4
#define CL_F  $vf5 // clipping scale adjustments to match guardbands
#define VP_O  $vf6 // viewport origin
#define VP_S  $vf7 // viewport scale


// ===============================
// POLY FIELD OFFSETS
// ===============================
#define PTEX__C  0
#define PTEX__W  4
#define PTEX__U  8
#define PTEX__V 12
#define PTEX_XY 16
#define PTEX__Z 20

#define PCOL__C  0
#define PCOL__W  4
#define PCOL_XY  8
#define PCOL__Z 12


// ===============================
// POLY STRUCT SIZES
// ===============================
#define S_PCOL 16
#define S_PTEX 24
