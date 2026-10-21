#include "AsmCommon.i"

// See https://refspecs.linuxfoundation.org/ELF/ppc64/PPC-elf64abi-1.9.html#PARAM-PASS
// f0        Volatile scratch register
// f1-f4     Volatile floating point parameter and return value registers
// f5-f13    Volatile floating point parameter registers
// f14-f31   Nonvolatile registers

#define RHT_R1XY	0
#define RHT_R1ZW	1
#define RHT_R2XY	2
#define RHT_R2ZW	3
#define RHT_R3XY	4
#define RHT_R3ZW	5
#define RHT_R4XY	6
#define RHT_R4ZW	7

#define LFT_XY		8
#define LFT_ZW		9
#define DST_XY		10
#define DST_ZW		11

// TODO fr12/fr13 are still free. could they be utilised somehow?

// Offsets into matrix struct
#define XY1  0
#define ZW1  8
#define XY2 16
#define ZW2 24
#define XY3 32
#define ZW3 40
#define XY4 48
#define ZW4 56

// https://wiibrew.org/wiki/Paired_single
#define LD_BOTH 0 // loads both lo and hi in packed singles
#define ST_BOTH 0 // stores both lo and hi from packed singles

// Multiplies two matrices together, then writes the output
//	$r3 = addresss of destination matrix
//	$r4 = addresss of source left matrix
//	$r5 = addresss of source right matrix
BEG_FUNC(Matrix_Mul)
#define DST 3
#define LFT 4
#define RHT 5

	psq_l		RHT_R1XY,	XY1(RHT),	LD_BOTH, 0
	psq_l		RHT_R1ZW,	ZW1(RHT),	LD_BOTH, 0
	psq_l		LFT_XY,		XY1(LFT),	LD_BOTH, 0
	psq_l		LFT_ZW,		ZW1(LFT),	LD_BOTH, 0
	// NOTE: load operations interleaved for potentially better pipelining	

	// Row 1
	ps_muls0	DST_XY,		RHT_R1XY,	LFT_XY
	psq_l		RHT_R2XY,	XY2(RHT),	LD_BOTH, 0
	ps_muls0	DST_ZW,		RHT_R1ZW,	LFT_XY
	psq_l		RHT_R2ZW,	ZW2(RHT),	LD_BOTH, 0
	ps_madds1	DST_XY,		RHT_R2XY,	LFT_XY, DST_XY
	psq_l		RHT_R3XY,	XY3(RHT),	LD_BOTH, 0
	ps_madds1	DST_ZW,		RHT_R2ZW,	LFT_XY, DST_ZW
	psq_l		RHT_R3ZW,	ZW3(RHT),	LD_BOTH, 0
	ps_madds0	DST_XY,		RHT_R3XY,	LFT_ZW, DST_XY
	psq_l		RHT_R4XY,	XY4(RHT),	LD_BOTH, 0
	ps_madds0	DST_ZW,		RHT_R3ZW,	LFT_ZW, DST_ZW
	psq_l		RHT_R4ZW,	ZW4(RHT),	LD_BOTH, 0
	ps_madds1	DST_XY,		RHT_R4XY,	LFT_ZW, DST_XY
	psq_l		LFT_XY,		XY2(LFT),	LD_BOTH, 0
	ps_madds1	DST_ZW,		RHT_R4ZW,	LFT_ZW, DST_ZW
	psq_l		LFT_ZW,		ZW2(LFT),	LD_BOTH, 0
	psq_st		DST_XY,		XY1(DST),	ST_BOTH, 0
	psq_st		DST_ZW,		ZW1(DST),	ST_BOTH, 0

	// Row 2
	ps_muls0	DST_XY,		RHT_R1XY,	LFT_XY
	ps_muls0	DST_ZW,		RHT_R1ZW,	LFT_XY
	ps_madds1	DST_XY,		RHT_R2XY,	LFT_XY, DST_XY
	ps_madds1	DST_ZW,		RHT_R2ZW,	LFT_XY, DST_ZW
	ps_madds0	DST_XY,		RHT_R3XY,	LFT_ZW, DST_XY
	ps_madds0	DST_ZW,		RHT_R3ZW,	LFT_ZW, DST_ZW
	psq_l		LFT_XY,		XY3(LFT),	LD_BOTH, 0
	ps_madds1	DST_XY,		RHT_R4XY,	LFT_ZW, DST_XY
	ps_madds1	DST_ZW,		RHT_R4ZW,	LFT_ZW, DST_ZW
	psq_l		LFT_ZW,		ZW3(LFT),	LD_BOTH, 0
	psq_st		DST_XY,		XY2(DST),	ST_BOTH, 0
	psq_st		DST_ZW,		ZW2(DST),	ST_BOTH, 0

    // Row 3
	ps_muls0	DST_XY,		RHT_R1XY,	LFT_XY
	ps_muls0	DST_ZW,		RHT_R1ZW,	LFT_XY
	ps_madds1	DST_XY,		RHT_R2XY,	LFT_XY, DST_XY
	ps_madds1	DST_ZW,		RHT_R2ZW,	LFT_XY, DST_ZW
	ps_madds0	DST_XY,		RHT_R3XY,	LFT_ZW, DST_XY
	ps_madds0	DST_ZW,		RHT_R3ZW,	LFT_ZW, DST_ZW
	psq_l		LFT_XY,		XY4(LFT), 	LD_BOTH, 0
	ps_madds1	DST_XY,		RHT_R4XY,	LFT_ZW, DST_XY
	ps_madds1	DST_ZW,		RHT_R4ZW,	LFT_ZW, DST_ZW
	psq_l		LFT_ZW,		ZW4(LFT), 	LD_BOTH, 0
	psq_st		DST_XY,		XY3(DST), 	ST_BOTH, 0
	psq_st		DST_ZW,		ZW3(DST), 	ST_BOTH, 0

	// Row 4
	ps_muls0	DST_XY,		RHT_R1XY,	LFT_XY
	ps_muls0	DST_ZW,		RHT_R1ZW,	LFT_XY
	ps_madds1	DST_XY,		RHT_R2XY,	LFT_XY, DST_XY
	ps_madds1	DST_ZW,		RHT_R2ZW,	LFT_XY, DST_ZW
	ps_madds0	DST_XY,		RHT_R3XY,	LFT_ZW, DST_XY
	ps_madds0	DST_ZW,		RHT_R3ZW,	LFT_ZW, DST_ZW
	ps_madds1	DST_XY,		RHT_R4XY,	LFT_ZW, DST_XY
	ps_madds1	DST_ZW,		RHT_R4ZW,	LFT_ZW, DST_ZW
	psq_st		DST_XY,		XY4(DST), 	ST_BOTH, 0
	psq_st		DST_ZW,		ZW4(DST), 	ST_BOTH, 0

	blr

END_FUNC(Matrix_Mul)

