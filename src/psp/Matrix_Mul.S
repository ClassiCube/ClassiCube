.set noreorder
#include "AsmCommon.i"

#define LFT_R1  R500
#define LFT_R2  R501
#define LFT_R3  R502
#define LFT_R4  R503
#define LFT_MTX M500

#define RHT_R1  R600
#define RHT_R2  R601
#define RHT_R3  R602
#define RHT_R4  R603
#define RHT_MTX M600

#define RES_R1  R700
#define RES_R2  R701
#define RES_R3  R702
#define RES_R4  R703
#define RES_MTX M700

// Multiplies two matrices together, then writes the output
//	$a0 = addresss of destination matrix
//	$a1 = addresss of source left matrix
//	$a2 = addresss of source right matrix
BEG_FUNC(Matrix_Mul)
	#define DST   $a0
	#define SRC_L $a1
	#define SRC_R $a2

	lv.q	LFT_R1,  0(SRC_L)
	lv.q	LFT_R2, 16(SRC_L)
	lv.q	LFT_R3, 32(SRC_L)
	lv.q	LFT_R4, 48(SRC_L)

	lv.q	RHT_R1,  0(SRC_R)
	lv.q	RHT_R2, 16(SRC_R)
	lv.q	RHT_R3, 32(SRC_R)
	lv.q	RHT_R4, 48(SRC_R)

	vmmul.q RES_MTX, LFT_MTX, RHT_MTX

	sv.q 	RES_R1,  0(DST)
	sv.q 	RES_R2, 16(DST)
	sv.q 	RES_R3, 32(DST)
	jr		$ra
	sv.q 	RES_R4, 48(DST)

	#undef DST
	#undef SRC_L
	#undef SRC_R
END_FUNC(Matrix_Mul)

