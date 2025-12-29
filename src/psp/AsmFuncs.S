.set noreorder

#define BEG_FUNC(name_) \
	.global name_; \
	.align 4; \
	name_:

#define END_FUNC(name_) \
	.size name_, . - name_; \
	.type name_, %function;


// ===============================
// UTILITIES
// ===============================

// NOTE: Not using lvl.q because it is broken on some PSP CPUs
//   https://pspdev.github.io/vfpu-docs/#ulvq-lvlq-lvrq-register-corruption
// S500 = MEM_32[$a0+0]
//  ...       ....
#define LoadMatrixUnaligned(mtx, SRC) \
	lv.s	S##mtx##00,  0(SRC); \
	lv.s	S##mtx##10,  4(SRC); \
	lv.s	S##mtx##20,  8(SRC); \
	lv.s	S##mtx##30, 12(SRC); \
\
	lv.s	S##mtx##01, 16(SRC); \
	lv.s	S##mtx##11, 20(SRC); \
	lv.s	S##mtx##21, 24(SRC); \
	lv.s	S##mtx##31, 28(SRC); \
\
	lv.s	S##mtx##02, 32(SRC); \
	lv.s	S##mtx##12, 36(SRC); \
	lv.s	S##mtx##22, 40(SRC); \
	lv.s	S##mtx##32, 44(SRC); \
\
	lv.s	S##mtx##03, 48(SRC); \
	lv.s	S##mtx##13, 52(SRC); \
	lv.s	S##mtx##23, 56(SRC); \
	lv.s	S##mtx##33, 60(SRC);


// ===============================
// FUNCTIONS
// ===============================
BEG_FUNC(Clip_SetMatrices)
	#define VIEW $a0
	#define PROJ $a1

	lv.q	R000,  0(VIEW) // M0.row1 = MEM_128[VIEW+ 0]
	lv.q	R001, 16(VIEW) // M0.row2 = MEM_128[VIEW+16]
	lv.q	R002, 32(VIEW) // M0.row3 = MEM_128[VIEW+32]
	lv.q	R003, 48(VIEW) // M0.row4 = MEM_128[VIEW+48]

	lv.q	R100,  0(PROJ) // M1.row1 = MEM_128[PROJ+ 0]
	lv.q	R101, 16(PROJ) // M1.row2 = MEM_128[PROJ+16]
	lv.q	R102, 32(PROJ) // M1.row3 = MEM_128[PROJ+32]
	lv.q	R103, 48(PROJ) // M1.row4 = MEM_128[PROJ+48]

	jr 		$ra
	vmmul.q M200, M000, M100 // M2 = M0 * M1

	#undef VIEW
	#undef PROJ
END_FUNC(Clip_SetMatrices)

BEG_FUNC(Matrix_Mul2)
	#define DST   $a0
	#define SRC_L $a1
	#define SRC_R $a2

	LoadMatrixUnaligned(5, SRC_L)
	LoadMatrixUnaligned(6, SRC_R)

	vmmul.q M700, M500, M600 // M7 = M5 * M6

// Write/Store multiplied matrix
// See https://stackoverflow.com/questions/57522055/what-do-the-mips-load-word-left-lwl-and-load-word-right-lwr-instructions-do
	svl.q 	R700, 12(DST) // MEM_128[DST+0] = ROW1
	svr.q 	R700,  0(DST)

	svl.q 	R701, 28(DST)
	svr.q 	R701, 16(DST)

	svl.q 	R702, 44(DST)
	svr.q 	R702, 32(DST)

	svl.q 	R703, 60(DST)
	jr 		$ra
	svr.q 	R703, 48(DST)

	#undef DST
	#undef SRC_L
	#undef SRC_R
END_FUNC(Matrix_Mul2)
