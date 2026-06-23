#include "AsmCommon.i"

// https://github.com/ARM-software/abi-aa/blob/main/aapcs32/aapcs32.rst#6121vfp-register-usage-conventions
// Registers s16-s31 (d8-d15, q4-q7) must be preserved across subroutine calls; 
// Registers  s0-s15 (d0-d7,  q0-q3) do not need to be preserved
// Registers d16-d31 (q8-q15), if present, do not need to be preserved.
// https://developer.arm.com/community/arm-community-blogs/b/architectures-and-processors-blog/posts/coding-for-neon---part-1-load-and-stores

#define LFT_R1XY d0
#define LFT_R1ZW d1
#define LFT_R2XY d2
#define LFT_R2ZW d3
#define LFT_R3XY d4
#define LFT_R3ZW d5
#define LFT_R4XY d6
#define LFT_R4ZW d7

#define RHT_R1XY d16
#define RHT_R1ZW d17
#define RHT_R2XY d18
#define RHT_R2ZW d19
#define RHT_R3XY d20
#define RHT_R3ZW d21
#define RHT_R4XY d22
#define RHT_R4ZW d23

#define LFT_R1   q0
#define LFT_R2   q1
#define LFT_R3   q2
#define LFT_R4   q3

#define RHT_R1   q8
#define RHT_R2   q9
#define RHT_R3   q10
#define RHT_R4   q11

#define RES_R1   q12
#define RES_R2   q13
#define RES_R3   q14
#define RES_R4   q15

// Multiplies two matrices together, then writes the output
//	$r0 = addresss of destination matrix
//	$r1 = addresss of source left matrix
//	$r2 = addresss of source right matrix
BEG_FUNC(Matrix_Mul)
#define DST r0
#define LFT r1
#define RHT r2

	vldm LFT!, {LFT_R1,LFT_R2,LFT_R3,LFT_R4}
//vldm RHT!, {RHT_R1,RHT_R2,RHT_R3,RHT_R4} try to make it easier to parallelise with other instructions by splitting up loads
	vldm RHT!, {RHT_R1,RHT_R2}

	vmul.f32	RES_R1,	RHT_R1,	LFT_R1XY[0]
	vmul.f32	RES_R2,	RHT_R1,	LFT_R2XY[0]
	vmul.f32	RES_R3,	RHT_R1,	LFT_R3XY[0]
	vmul.f32	RES_R4,	RHT_R1,	LFT_R4XY[0]
	vldm RHT!, {RHT_R3}

	vmla.f32	RES_R1,	RHT_R2,	LFT_R1XY[1]
	vmla.f32	RES_R2,	RHT_R2,	LFT_R2XY[1]
	vmla.f32	RES_R3,	RHT_R2,	LFT_R3XY[1]
	vmla.f32	RES_R4,	RHT_R2,	LFT_R4XY[1]
	vldm RHT!, {RHT_R4}

	vmla.f32	RES_R1,	RHT_R3,	LFT_R1ZW[0]
	vmla.f32	RES_R2,	RHT_R3,	LFT_R2ZW[0]
	vmla.f32	RES_R3,	RHT_R3,	LFT_R3ZW[0]
	vmla.f32	RES_R4,	RHT_R3,	LFT_R4ZW[0]

	vmla.f32	RES_R1,	RHT_R4,	LFT_R1ZW[1]
	vmla.f32	RES_R2,	RHT_R4,	LFT_R2ZW[1]
	vmla.f32	RES_R3,	RHT_R4,	LFT_R3ZW[1]
	vmla.f32	RES_R4,	RHT_R4,	LFT_R4ZW[1]

	vstm DST, {RES_R1,RES_R2,RES_R3,RES_R4}
	bx		lr
END_FUNC(Matrix_Mul)

