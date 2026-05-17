.set noreorder
#include "AsmCommon.i"

#define FSM_XP R600 // frustum plane x+ (left)
#define FSM_XN R601 // frustum plane x- (right)
#define FSM_YP R602 // frustum plane y+ (bottom)
#define FSM_YN R603 // frustum plane y- (top)
#define FSM_ZP R700 // frustum plane z+ (near)
#define FSM_ZN R701 // frustum plane z- (far)

#define SCL_XP S702 // frustum plane x+ scale
#define SCL_XN S712 // frustum plane x- scale
#define SCL_YP S722 // frustum plane y+ scale
#define SCL_YN S732 // frustum plane y- scale
#define SCL_ZP S703 // frustum plane z+ scale
#define SCL_ZN S713 // frustum plane z- scale

BEG_FUNC(Frustum_CalcAllPlanes)
	#define DST $a0

	// Frustum planes calculation
	// Left:   a,b,c,d = mvp.row[1,2,3,4].w + mvp.row[1,2,3,4].x
	// Right:  a,b,c,d = mvp.row[1,2,3,4].w - mvp.row[1,2,3,4].x
	// Bottom: a,b,c,d = mvp.row[1,2,3,4].w + mvp.row[1,2,3,4].y
	// Top:    a,b,c,d = mvp.row[1,2,3,4].w - mvp.row[1,2,3,4].y
	// Near:   a,b,c,d = mvp.row[1,2,3,4].w + mvp.row[1,2,3,4].z
	// Far:    a,b,c,d = mvp.row[1,2,3,4].w - mvp.row[1,2,3,4].z

	vadd.q FSM_XP, MVP_CW, MVP_CX // mvp.cols.W + mvp.cols.X
	vsub.q FSM_XN, MVP_CW, MVP_CX // mvp.cols.W - mvp.cols.X
	vadd.q FSM_YP, MVP_CW, MVP_CY // mvp.cols.W + mvp.cols.Y
	vsub.q FSM_YN, MVP_CW, MVP_CY // mvp.cols.W - mvp.cols.Y
	vadd.q FSM_ZP, MVP_CW, MVP_CZ // mvp.cols.W + mvp.cols.Z
	vsub.q FSM_ZN, MVP_CW, MVP_CZ // mvp.cols.W - mvp.cols.Z

	// Normalise planes. For each plane:
	// 1) len = a*a+b*b+c*c
	// 2) scale = 1.0 / sqrt(len)
	// 3) a *= scale; b *= scale; c *= scale; d *= scale;

	// len = a*a + b*b + c*c
	vdot.t SCL_XP, FSM_XP, FSM_XP 
	vdot.t SCL_XN, FSM_XN, FSM_XN
	vdot.t SCL_YP, FSM_YP, FSM_YP
	vdot.t SCL_YN, FSM_YN, FSM_YN
	vdot.t SCL_ZP, FSM_ZP, FSM_ZP
	vdot.t SCL_ZN, FSM_ZN, FSM_ZN

	// scale = 1 / sqrt(len)
	vrsq.s SCL_XP, SCL_XP
	vrsq.s SCL_XN, SCL_XN
	vrsq.s SCL_YP, SCL_YP
	vrsq.s SCL_YN, SCL_YN
	vrsq.s SCL_ZP, SCL_ZP
	vrsq.s SCL_ZN, SCL_ZN

	// [a,b,c,d] *= scale
	vscl.q FSM_XP, FSM_XP, SCL_XP 
	vscl.q FSM_XN, FSM_XN, SCL_XN
	vscl.q FSM_YP, FSM_YP, SCL_YP
	vscl.q FSM_YN, FSM_YN, SCL_YN
	vscl.q FSM_ZP, FSM_ZP, SCL_ZP
	vscl.q FSM_ZN, FSM_ZN, SCL_ZN

	// Store frustum planes back into memory
	// MEM_128[PLANE] = [a,b,c,d]
	sv.q FSM_XP,  0(DST)
	sv.q FSM_XN, 16(DST)
	sv.q FSM_YP, 32(DST)
	sv.q FSM_YN, 48(DST)
	sv.q FSM_ZP, 64(DST)
	jr $ra
	sv.q FSM_ZN, 80(DST)

	#undef DST
END_FUNC(Frustum_CalcAllPlanes)

