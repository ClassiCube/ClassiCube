.set noreorder
#include "AsmCommon.i"

// ===============================
// UTILITIES
// ===============================
#define JUMP_RA jr $ra;
#define NO_JUMP ;

// NOTE: Not using lvl.q because it is broken on some PSP CPUs
//   https://pspdev.github.io/vfpu-docs/#ulvq-lvlq-lvrq-register-corruption
// NOTE: MAYBE_JUMP_RA may or may not be 'jr $ra' (so that delay slot can be filled in some cases)

// S500 = MEM_32[$a0+0]
//  ...       ....
#define LoadMatrixUnaligned(mtx, SRC, MAYBE_JUMP_RA) \
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
	MAYBE_JUMP_RA;			 	 \
	lv.s	S##mtx##33, 60(SRC);

// Write/Store matrix
// See https://stackoverflow.com/questions/57522055/what-do-the-mips-load-word-left-lwl-and-load-word-right-lwr-instructions-do
#define StoreMatrixUnaligned(mtx, DST, MAYBE_JUMP_RA) \
	svl.q 	R##mtx##00, 12(DST); \
	svr.q 	R##mtx##00,  0(DST); \
\
	svl.q 	R##mtx##01, 28(DST); \
	svr.q 	R##mtx##01, 16(DST); \
\
	svl.q 	R##mtx##02, 44(DST); \
	svr.q 	R##mtx##02, 32(DST); \
\
	svl.q 	R##mtx##03, 60(DST); \
	MAYBE_JUMP_RA;				 \
	svr.q 	R##mtx##03, 48(DST);


// ===============================
// FUNCTIONS
// ===============================

// Loads view matrix into M0
BEG_FUNC(Clip_LoadView)
	#define SRC $a0
	LoadMatrixUnaligned(0, SRC, JUMP_RA)

	#undef SRC
END_FUNC(Clip_LoadView)

// Loads projection matrix into M1
BEG_FUNC(Clip_LoadProj)
	#define SRC $a0
	LoadMatrixUnaligned(1, SRC, JUMP_RA)

	#undef SRC
END_FUNC(Clip_LoadView)

// Calculates MVP = VIEW * PROJ
BEG_FUNC(Clip_RecalcMVP)
	jr 		$ra
	vmmul.q M200, M000, M100 // M2 = M0 * M1
END_FUNC(Clip_RecalcMVP)

// Stores MVP matrix
BEG_FUNC(Clip_StoreMVP)
	#define DST $a0
	StoreMatrixUnaligned(2, DST, JUMP_RA)

	#undef DST
END_FUNC(Clip_StoreMVP)



BEG_FUNC(Clip_SetGuardbandScale)
	#define SRC_X $a0
	#define SRC_Y $a1

	lv.s	S700, 0(SRC_X)
	lv.s	S701, 0(SRC_Y)
	vscl.q C200, C200, S700
	jr 		$ra
	vscl.q C210, C210, S701

	#undef SRC_X
	#undef SRC_Y
END_FUNC(Clip_SetGuardbandScale)


BEG_FUNC(Matrix_Mul2)
	#define DST   $a0
	#define SRC_L $a1
	#define SRC_R $a2

	LoadMatrixUnaligned(5, SRC_L, NO_JUMP)
	LoadMatrixUnaligned(6, SRC_R, NO_JUMP)

	vmmul.q M700, M500, M600 // M7 = M5 * M6

	StoreMatrixUnaligned(7, DST, JUMP_RA);

	#undef DST
	#undef SRC_L
	#undef SRC_R
END_FUNC(Matrix_Mul2)


BEG_FUNC(TransformTexturedQuad)
	#define SRC $a0
	#define DST $a1
	#define END $t0

	vone.s		S730;               // M7[0,3] = 1.0
	addiu		END, DST, 3*S_POLY; // TMP = DST + 3 * sizeof(PolyVertex)

tex_next_iter:
	lv.s		S701, VTEX_U(SRC);  // M7[1,0] = src->u
	lv.s		S711, VTEX_V(SRC);  // M7[1,1] = src->v
	lv.s		S721, VTEX_C(SRC);  // M7[1,2] = src->color

	lv.s		S700, VTEX_X(SRC);  // M7[0,0] = src->x
	lv.s		S710, VTEX_Y(SRC);  // M7[0,1] = src->y
	lv.s		S720, VTEX_Z(SRC);  // M7[0,2] = src->z

	vhtfm4.q 	R703, M200, R700;   // M7.R3 = M200 * M7.R0
	// TODO TP 4, LT 10 cycles
	addiu		SRC, SRC, S_VTEX;   // src += sizeof(VertexTextured)

	sv.q		R703,  0(DST) 		// MEM_128[DST+ 0] = M7.R3
	sv.q		R701, 16(DST) 		// MEM_128[DST+16] = M7.R1
	bne			DST, END, tex_next_iter // if (dst != end) goto tex_next_iter;
	addiu		DST, DST, S_POLY;	// dst += sizeof(PolyVertex)

	jr	$ra
	nop

	#undef SRC
	#undef DST
	#udnef END
END_FUNC(TransformTexturedQuad)
