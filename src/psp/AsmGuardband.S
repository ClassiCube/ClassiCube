.set noreorder
#include "AsmCommon.i"

#define F_X $f12
#define F_Y $f13
#define F_Z $f14
#define F_R $f15

#define DST_L    S600
#define DST_R    S610
#define DST_B    S620
#define DST_T    S630
#define DST_F    S601
#define DST_LRBT R600

#define TMP_X    S700
#define TMP_Y    S710
#define TMP_Z    S720
#define TMP_W    S730
#define TMP_XYZW R700

#define TMP_R    S701

BEG_FUNC(Frustum_SphereSkipsClipping)
// FPU to CPU registers
	mfc1 $t0, F_X
	mfc1 $t1, F_Y
	mfc1 $t2, F_Z
	mfc1 $t3, F_R

// Init VFPU registers
	mtv $t0, TMP_X
	mtv $t1, TMP_Y
	mtv $t2, TMP_Z
	vone.s   TMP_W
	mtv $t3, TMP_R

// calculate distance to planes
	vdot.q DST_L, GBPLN_L, TMP_XYZW
	vdot.q DST_R, GBPLN_R, TMP_XYZW
	vdot.q DST_B, GBPLN_B, TMP_XYZW
	vdot.q DST_T, GBPLN_T, TMP_XYZW
	vdot.q DST_F, GBPLN_F, TMP_XYZW

// check if any L,R,B,T distance is < radius
	vcmp.q LT, DST_LRBT, R701[x,x,x,x]
	bvt FLG_ANY, not_fully_inside_all
	nop

// check if F plane distance is < radius
	vcmp.s LT, DST_F,    TMP_R
	bvt FLG_ANY, not_fully_inside_all
	nop

// fully inside L,R,T,B,F planes
	jr $ra
	li $v0, 1

not_fully_inside_all:
	jr $ra
	li $v0, 0

END_FUNC(Frustum_SphereSkipsClipping)

