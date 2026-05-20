.set noreorder
#include "AsmCommon.i"

// ===============================
// FUNCTIONS
// ===============================

// Loads view matrix into M0
BEG_FUNC(Clip_LoadView)
	#define SRC $a0
	lv.q	R000,  0(SRC)
	lv.q	R001, 16(SRC)
	lv.q	R002, 32(SRC)
	jr		$ra
	lv.q	R003, 48(SRC)

	#undef SRC
END_FUNC(Clip_LoadView)

// Loads projection matrix into M1
BEG_FUNC(Clip_LoadProj)
	#define SRC $a0
	lv.q	R100,  0(SRC)
	lv.q	R101, 16(SRC)
	lv.q	R102, 32(SRC)
	jr		$ra
	lv.q	R103, 48(SRC)

	#undef SRC
END_FUNC(Clip_LoadProj)

// Calculates MVP = VIEW * PROJ
BEG_FUNC(Clip_RecalcMVP)
	jr 		$ra
	vmmul.q M200, M000, M100 // M2 = M0 * M1
END_FUNC(Clip_RecalcMVP)

// Stores MVP matrix
BEG_FUNC(Clip_StoreMVP)
	#define DST $a0
	sv.q 	R200,  0(DST)
	sv.q 	R201, 16(DST)
	sv.q 	R202, 32(DST)
	jr 		$ra
	sv.q 	R203, 48(DST)

	#undef DST
END_FUNC(Clip_StoreMVP)


BEG_FUNC(Clip_SetGuardbandScale)
	#define SRC_X $a0
	#define SRC_Y $a1

	lv.s	GBAND_X, 0(SRC_X)
	lv.s	GBAND_Y, 0(SRC_Y)
	vrcp.s  GBINV_X, GBAND_X
	jr 		$ra
	vrcp.s  GBINV_Y, GBAND_Y

	#undef SRC_X
	#undef SRC_Y
END_FUNC(Clip_SetGuardbandScale)

BEG_FUNC(Clip_RescaleMVPtoGuardband)
	vscl.q MVP_CX, MVP_CX, GBAND_X
	jr 	   $ra
	vscl.q MVP_CY, MVP_CY, GBAND_Y
END_FUNC(Clip_RescaleMVPtoGuardband)


BEG_FUNC(Math_FastCos)
	#define ADDR   $a0
	lv.s	S700, 0(ADDR)
	vcos.s	S700, S700
	jr 	  	$ra
	sv.s	S700, 0(ADDR)
	#undef ADDR
END_FUNC(Math_FastCos)


BEG_FUNC(Matrix_Mul)
	#define DST   $a0
	#define SRC_L $a1
	#define SRC_R $a2

	lv.q	R500,  0(SRC_L)
	lv.q	R501, 16(SRC_L)
	lv.q	R502, 32(SRC_L)
	lv.q	R503, 48(SRC_L)

	lv.q	R600,  0(SRC_R)
	lv.q	R601, 16(SRC_R)
	lv.q	R602, 32(SRC_R)
	lv.q	R603, 48(SRC_R)

	vmmul.q M700, M500, M600 // M7 = M5 * M6

	sv.q 	R700,  0(DST)
	sv.q 	R701, 16(DST)
	sv.q 	R702, 32(DST)
	jr		$ra
	sv.q 	R703, 48(DST)

	#undef DST
	#undef SRC_L
	#undef SRC_R
END_FUNC(Matrix_Mul)

