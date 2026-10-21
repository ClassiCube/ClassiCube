.set noreorder
#include "AsmCommon.i"

#define TMP_XYZW R700
#define TMP_X    S700
#define TMP_Y    S710
#define TMP_Z    S720
#define TMP_W    S730

#define TMP_UVCF R701
#define TMP_U    S701
#define TMP_V    S711
#define TMP_C    S721

#define SRC $a0
#define DST $a1
#define END $t0

// Converts 4 'VertexTextured' vertices into 4 'ClipVertex'
BEG_FUNC(ConvertTexturedToClipQuad)
	vone.s		TMP_W
	addiu		END, DST, 3*S_VCLP

tex_next_iter:
	lv.s		TMP_U, VTEX_U(SRC)
	lv.s		TMP_V, VTEX_V(SRC)
	lv.s		TMP_C, VTEX_C(SRC)

	lv.s		TMP_X, VTEX_X(SRC)
	lv.s		TMP_Y, VTEX_Y(SRC)
	lv.s		TMP_Z, VTEX_Z(SRC)

	addiu		SRC, SRC, S_VTEX

	sv.q		TMP_XYZW, VCLP_XYZW(DST)
	sv.q		TMP_UVCF, VCLP_UVCF(DST)
	bne			DST, END, tex_next_iter
	addiu		DST, DST, S_VCLP; // delay slot

	jr	$ra
	nop
END_FUNC(ConvertTexturedToClipQuad)

// Converts 4 'VertexColoured' vertices into 4 'ClipVertex'
BEG_FUNC(ConvertColouredToClipQuad)
	vzero.q		TMP_UVCF
	vone.s		TMP_W
	addiu		END, DST, 3*S_VCLP

col_next_iter:
	lv.s		TMP_C, VCOL_C(SRC)

	lv.s		TMP_X, VCOL_X(SRC)
	lv.s		TMP_Y, VCOL_Y(SRC)
	lv.s		TMP_Z, VCOL_Z(SRC)

	addiu		SRC, SRC, S_VCOL;

	sv.q		TMP_XYZW, VCLP_XYZW(DST)
	sv.q		TMP_UVCF, VCLP_UVCF(DST)
	bne			DST, END, col_next_iter
	addiu		DST, DST, S_VCLP

	jr	$ra
	nop
END_FUNC(ConvertColouredToClipQuad)
