.set noreorder
#include "AsmCommon.i"

#define LFT_R1  $vf20
#define LFT_R2  $vf21
#define LFT_R3  $vf22
#define LFT_R4  $vf23

#define RHT_R1  $vf24
#define RHT_R2  $vf25
#define RHT_R3  $vf26
#define RHT_R4  $vf27

#define RES_R1  $vf28
#define RES_R2  $vf29
#define RES_R3  $vf30
#define RES_R4  $vf31

// Multiplies two matrices together, then writes the output
//	$a0 = addresss of destination matrix
//	$a1 = addresss of source left matrix
//	$a2 = addresss of source right matrix
BEG_FUNC(Matrix_Mul)
	#define DST   $a0
	#define SRC_L $a1
	#define SRC_R $a2

	lqc2	LFT_R1,  0(SRC_L)
	lqc2	LFT_R2, 16(SRC_L)
	lqc2	LFT_R3, 32(SRC_L)
	lqc2	LFT_R4, 48(SRC_L)

	lqc2	RHT_R1,  0(SRC_R)
	lqc2	RHT_R2, 16(SRC_R)
	lqc2	RHT_R3, 32(SRC_R)
	lqc2	RHT_R4, 48(SRC_R)

	// Calculate row 1
	vmulax		$ACC,  	RHT_R1, LFT_R1 // ACC = R.r1 * L.r1.x
	vmadday		$ACC,  	RHT_R2, LFT_R1 // ACC = R.r2 * L.r1.y + ACC
	vmaddaz		$ACC,  	RHT_R3, LFT_R1 // ACC = R.r3 * L.r1.z + ACC
	vmaddw		RES_R1, RHT_R4, LFT_R1 // RES = R.r4 * L.r1.w + ACC

	// Calculate row 2
	vmulax		$ACC,  	RHT_R1, LFT_R2
	vmadday		$ACC,  	RHT_R2, LFT_R2
	vmaddaz		$ACC,  	RHT_R3, LFT_R2
	vmaddw		RES_R2, RHT_R4, LFT_R2

	// Calculate row 3
	vmulax		$ACC,  	RHT_R1, LFT_R3
	vmadday		$ACC,  	RHT_R2, LFT_R3
	vmaddaz		$ACC,  	RHT_R3, LFT_R3
	vmaddw		RES_R3, RHT_R4, LFT_R3

	// Calculate row 4
	vmulax		$ACC,  	RHT_R1, LFT_R4
	vmadday		$ACC,  	RHT_R2, LFT_R4
	vmaddaz		$ACC,  	RHT_R3, LFT_R4
	vmaddw		RES_R4, RHT_R4, LFT_R4

	sqc2 	RES_R1,  0(DST)
	sqc2	RES_R2, 16(DST)
	sqc2 	RES_R3, 32(DST)
	jr		$ra
	sqc2 	RES_R4, 48(DST)

	#undef DST
	#undef SRC_L
	#undef SRC_R
END_FUNC(Matrix_Mul)

