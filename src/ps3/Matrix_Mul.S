#include "AsmCommon.i"

// See https://refspecs.linuxfoundation.org/ELF/ppc64/PPC-elf64abi-1.9.html#PARAM-PASS
// v0-v1     Volatile scratch registers
// v2-v13    Volatile vector parameters registers
// v14-v19   Volatile scratch registers

#define LFT_R1   0
#define LFT_R2   1
#define LFT_R3   2
#define LFT_R4   3

#define RHT_R1   4
#define RHT_R2   5
#define RHT_R3   6
#define RHT_R4   7

#define RES_R1   8
#define RES_R2   9
#define RES_R3  10
#define RES_R4  11

#define TMP_X	12
#define TMP_Y   13
#define TMP_Z   14
#define TMP_W   15

#define V_ZERO  16


#define OFF_00  0
#define OFF_16  7
#define OFF_32  8
#define OFF_48	9


#define SELECT_X 0
#define SELECT_Y 1
#define SELECT_Z 2
#define SELECT_W 3


// Multiplies two matrices together, then writes the output
//	$r3 = addresss of destination matrix
//	$r4 = addresss of source left matrix
//	$r5 = addresss of source right matrix
BEG_FUNC(Matrix_Mul)
	#define DST 3
	#define LFT 4
	#define RHT 5

	// NOTE: load/store instructions compute address as follows:
	// - if rA is r0, then address = rB
	// - otherwise, address = rA + rB
	li		OFF_16, 16
	vspltisw V_ZERO, 0
	li		OFF_32, 32
	li		OFF_48, 48

	// Load left matrix
	lvx		LFT_R1, OFF_00, LFT
	lvx		LFT_R2, OFF_16, LFT
	lvx		LFT_R3, OFF_32, LFT
	lvx		LFT_R4, OFF_48, LFT

	// Load right matrix
	lvx		RHT_R1, OFF_00, RHT
	lvx		RHT_R2, OFF_16, RHT
	lvx		RHT_R3, OFF_32, RHT
	lvx		RHT_R4, OFF_48, RHT

	// NOTE: arithmetic/permute operations interleaved for potentially better pipelining	

	// Calculate row 1
	vspltw	TMP_X, LFT_R1, SELECT_X
	vspltw	TMP_Y, LFT_R1, SELECT_Y
	vmaddfp RES_R1, TMP_X, RHT_R1, V_ZERO
	vspltw	TMP_Z, LFT_R1, SELECT_Z
	vmaddfp RES_R1, TMP_Y, RHT_R2, RES_R1
	vspltw	TMP_W, LFT_R1, SELECT_W
	vmaddfp RES_R1, TMP_Z, RHT_R3, RES_R1
	vmaddfp RES_R1, TMP_W, RHT_R4, RES_R1

	// Calculate row 2
	vspltw	TMP_X, LFT_R2, SELECT_X
	vspltw	TMP_Y, LFT_R2, SELECT_Y
	vmaddfp RES_R2, TMP_X, RHT_R1, V_ZERO
	vspltw	TMP_Z, LFT_R2, SELECT_Z
	vmaddfp RES_R2, TMP_Y, RHT_R2, RES_R2
	vspltw	TMP_W, LFT_R2, SELECT_W
	vmaddfp RES_R2, TMP_Z, RHT_R3, RES_R2
	vmaddfp RES_R2, TMP_W, RHT_R4, RES_R2

	// Store row 1
	stvx	RES_R1, OFF_00, DST

	// Calculate row 3
	vspltw	TMP_X, LFT_R3, SELECT_X
	vspltw	TMP_Y, LFT_R3, SELECT_Y
	vmaddfp RES_R3, TMP_X, RHT_R1, V_ZERO
	vspltw	TMP_Z, LFT_R3, SELECT_Z
	vmaddfp RES_R3, TMP_Y, RHT_R2, RES_R3
	vspltw	TMP_W, LFT_R3, SELECT_W
	vmaddfp RES_R3, TMP_Z, RHT_R3, RES_R3
	vmaddfp RES_R3, TMP_W, RHT_R4, RES_R3

	// Store row 2
	stvx	RES_R2, OFF_16, DST

	// Calculate row 4
	vspltw	TMP_X, LFT_R4, SELECT_X
	vspltw	TMP_Y, LFT_R4, SELECT_Y
	vmaddfp RES_R4, TMP_X, RHT_R1, V_ZERO
	vspltw	TMP_Z, LFT_R4, SELECT_Z
	vmaddfp RES_R4, TMP_Y, RHT_R2, RES_R4
	vspltw	TMP_W, LFT_R4, SELECT_W
	vmaddfp RES_R4, TMP_Z, RHT_R3, RES_R4
	vmaddfp RES_R4, TMP_W, RHT_R4, RES_R4

	// Store rows 3 and 4
	stvx	RES_R3, OFF_32, DST
	stvx	RES_R4, OFF_48, DST
	blr

END_FUNC(Matrix_Mul)

