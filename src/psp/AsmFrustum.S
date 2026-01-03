.set noreorder
#include "AsmCommon.i"

// TODO use macros
#define F_XP R300 // frustum x+ (left)
#define FR_XN R301 // frustum x- (right)

// TODO use MVP instead?
BEG_FUNC(TestVertex)
	#define SRC $a0
	#define PLN $a1
	#define CUR $t0
	#define END $t1

	lv.s	S700, VTEX_X(SRC);  // M7[0,0] = src->x
	lv.s	S710, VTEX_Y(SRC);  // M7[0,1] = src->y
	lv.s	S720, VTEX_Z(SRC);  // M7[0,2] = src->z
	vone.s	S730				// M7[0,3] = 1.0
	vzero.s	S701				// M7[1,0] = 0.0

	li CUR, 0
	li END, 6

plane_next_iter:
	lv.q	R703, 0(PLN)		// PLN = MEM_128[plane]
	vdot.q 	S711, R703, R700	// M7[1,1] = dot

	vcmp.s LT, S711, S701
	bvt 0, behind_a_plane
	nop

	addiu	CUR, CUR, 1;	    // iter++
	bne		CUR, END, plane_next_iter // if (dst != end) goto plane_next_iter;
	addiu	PLN, PLN, 16		// plane++

	jr	$ra
	li $v0, 1

behind_a_plane:
	jr	$ra
	li $v0, 0

	#undef SRC
	#undef PLN
	#undef CUR
	#undef END
END_FUNC(TestVertex)



// TODO use MVP instead?
BEG_FUNC(TestVertex2)
	#define SRC $a0

	lv.s	S700, VTEX_X(SRC);  // M7[0,0] = src->x
	lv.s	S710, VTEX_Y(SRC);  // M7[0,1] = src->y
	lv.s	S720, VTEX_Z(SRC);  // M7[0,2] = src->z
	vone.s	S730				// M7[0,3] = 1.0

	vhtfm4.q R703, M200, R700;  // M7.R3 = M200 * M7.R0

	vcmp.q GT, R703[|x|,|y|,|z|,0], R703[|w|,|w|,|w|,0]
	bvt 4, behind_a_clipping_plane
	nop

	sv.q R703, 0($a1) // TODO temp
	jr	$ra
	li $v0, 1

behind_a_clipping_plane:
	sv.q R703, 0($a1) // TODO temp
	jr	$ra
	li $v0, 0

	#undef SRC
END_FUNC(TestVertex2)

BEG_FUNC(CalcFrustumPlanes)
	#define DST $a0

	// Frustum planes calculation
	// Left:   a,b,c,d = mvp.row[1,2,3,4].w + mvp.row[1,2,3,4].x
	// Right:  a,b,c,d = mvp.row[1,2,3,4].w - mvp.row[1,2,3,4].x
	// Bottom: a,b,c,d = mvp.row[1,2,3,4].w + mvp.row[1,2,3,4].y
	// Top:    a,b,c,d = mvp.row[1,2,3,4].w - mvp.row[1,2,3,4].y
	// Near:   a,b,c,d = mvp.row[1,2,3,4].w + mvp.row[1,2,3,4].z
	// Far:    a,b,c,d = mvp.row[1,2,3,4].w - mvp.row[1,2,3,4].z

	vadd.q R300, C230, C200 // mvp.cols.W + mvp.cols.X
	vsub.q R301, C230, C200 // mvp.cols.W - mvp.cols.X
	vadd.q R302, C230, C210 // mvp.cols.W + mvp.cols.Y
	vsub.q R303, C230, C210 // mvp.cols.W - mvp.cols.Y
	vadd.q R400, C230, C220 // mvp.cols.W + mvp.cols.Z
	vsub.q R401, C230, C220 // mvp.cols.W - mvp.cols.Z

	// Normalise planes. For each plane:
	// 1) len = a*a+b*b+c*c
	// 2) scale = 1.0 / sqrt(len)
	// 3) a *= scale; b *= scale; c *= scale; d *= scale;
	vdot.t S700, R300, R300 // len = a*a + b*b + c*c
	vdot.t S701, R301, R301 // ..
	vdot.t S702, R302, R302 // ..
	vdot.t S703, R303, R303 // ..
	vdot.t S710, R400, R400 // ..
	vdot.t S711, R401, R401 // ..

	vrsq.s S700, S700       // scale = 1 / sqrt(len)
	vrsq.s S701, S701       // ..
	vrsq.s S702, S702       // ..
	vrsq.s S703, S703       // ..
	vrsq.s S710, S710       // ..
	vrsq.s S711, S711       // ..

	vscl.q R300, R300, S700 // [a,b,c,d] *= scale
	vscl.q R301, R301, S701 // ..
	vscl.q R302, R302, S702 // ..
	vscl.q R303, R303, S703 // ..
	vscl.q R400, R400, S710 // ..
	vscl.q R401, R401, S711 // ..

	// Store frustum planes back into memory
	sv.q R300,  0(DST)      // MEM_128[PLANE_0] = [a,b,c,d]
	sv.q R301, 16(DST)      // ..
	sv.q R302, 32(DST)      // ..
	sv.q R303, 48(DST)      // ..
	sv.q R400, 64(DST)      // ..
	jr $ra
	sv.q R401, 80(DST)      // ..

	#undef DST
END_FUNC(CalcFrustumPlanes)
