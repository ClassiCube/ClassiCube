.set noreorder
#include "AsmCommon.i"

BEG_FUNC(TestVertex2)
	#define SRC $a0

	lv.s	S700, VTEX_X(SRC);  // M7[0,0] = src->x
	lv.s	S710, VTEX_Y(SRC);  // M7[0,1] = src->y
	lv.s	S720, VTEX_Z(SRC);  // M7[0,2] = src->z
	vone.s	S730				// M7[0,3] = 1.0

	vhtfm4.q R703, M200, R700;  // M7.R3 = M200 * M7.R0

	vcmp.q GT, R703[|x|,|y|,|z|,0], R703[|w|,|w|,|w|,0] // CC[0] = |x|>|w|...
	bvt 4, behind_a_clipping_plane // CC[4] = (|x|>|w|) | (|y|>|w|) | (|z|>|w|) | (0>0)
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

#define F_XP R300 // frustum plane x+ (left)
#define F_XN R301 // frustum plane x- (right)
#define F_YP R302 // frustum plane y+ (bottom)
#define F_YN R303 // frustum plane y- (top)
#define F_ZP R400 // frustum plane z+ (near)
#define F_ZN R401 // frustum plane z- (far)

#define M_CX C200 // mvp.columns.X
#define M_CY C210 // mvp.columns.Y
#define M_CZ C220 // mvp.columns.Z
#define M_CW C230 // mvp.columns.W

#define S_XP S700 // frustum plane x+ scale
#define S_XN S701 // frustum plane x- scale
#define S_YP S702 // frustum plane y+ scale
#define S_YN S703 // frustum plane y- scale
#define S_ZP S710 // frustum plane z+ scale
#define S_ZN S711 // frustum plane z- scale

BEG_FUNC(Frustum_CalcPlanes)
	#define DST $a0

	// Frustum planes calculation
	// Left:   a,b,c,d = mvp.row[1,2,3,4].w + mvp.row[1,2,3,4].x
	// Right:  a,b,c,d = mvp.row[1,2,3,4].w - mvp.row[1,2,3,4].x
	// Bottom: a,b,c,d = mvp.row[1,2,3,4].w + mvp.row[1,2,3,4].y
	// Top:    a,b,c,d = mvp.row[1,2,3,4].w - mvp.row[1,2,3,4].y
	// Near:   a,b,c,d = mvp.row[1,2,3,4].w + mvp.row[1,2,3,4].z
	// Far:    a,b,c,d = mvp.row[1,2,3,4].w - mvp.row[1,2,3,4].z

	vadd.q F_XP, M_CW, M_CX // mvp.cols.W + mvp.cols.X
	vsub.q F_XN, M_CW, M_CX // mvp.cols.W - mvp.cols.X
	vadd.q F_YP, M_CW, M_CY // mvp.cols.W + mvp.cols.Y
	vsub.q F_YN, M_CW, M_CY // mvp.cols.W - mvp.cols.Y
	vadd.q F_ZP, M_CW, M_CZ // mvp.cols.W + mvp.cols.Z
	vsub.q F_ZN, M_CW, M_CZ // mvp.cols.W - mvp.cols.Z

	// Normalise planes. For each plane:
	// 1) len = a*a+b*b+c*c
	// 2) scale = 1.0 / sqrt(len)
	// 3) a *= scale; b *= scale; c *= scale; d *= scale;
	vdot.t S_XP, F_XP, F_XP // len = a*a + b*b + c*c
	vdot.t S_XN, F_XN, F_XN // ..
	vdot.t S_YP, F_YP, F_YP // ..
	vdot.t S_YN, F_YN, F_YN // ..
	vdot.t S_ZP, F_ZP, F_ZP // ..
	vdot.t S_ZN, F_ZN, F_ZN // ..

	vrsq.s S_XP, S_XP       // scale = 1 / sqrt(len)
	vrsq.s S_XN, S_XN       // ..
	vrsq.s S_YP, S_YP       // ..
	vrsq.s S_YN, S_YN       // ..
	vrsq.s S_ZP, S_ZP       // ..
	vrsq.s S_ZN, S_ZN       // ..

	vscl.q F_XP, F_XP, S_XP // [a,b,c,d] *= scale
	vscl.q F_XN, F_XN, S_XN // ..
	vscl.q F_YP, F_YP, S_YP // ..
	vscl.q F_YN, F_YN, S_YN // ..
	vscl.q F_ZP, F_ZP, S_ZP // ..
	vscl.q F_ZN, F_ZN, S_ZN // ..

	// Store frustum planes back into memory
	sv.q F_XP,  0(DST)      // MEM_128[PLANE_0] = [a,b,c,d]
	sv.q F_XN, 16(DST)      // ..
	sv.q F_YP, 32(DST)      // ..
	sv.q F_YN, 48(DST)      // ..
	sv.q F_ZP, 64(DST)      // ..
	jr $ra
	sv.q F_ZN, 80(DST)      // ..

	#undef DST
END_FUNC(Frustum_CalcPlanes)
