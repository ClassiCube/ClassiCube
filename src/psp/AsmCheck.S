.set noreorder
#include "AsmCommon.i"

#define V1_X S600
#define V1_Y S610
#define V1_Z S620
#define V1_W S630
#define V1   R600
#define F1   R700

#define V2_X S601
#define V2_Y S611
#define V2_Z S621
#define V2_W S631
#define V2   R601
#define F2   R701

#define V3_X S602
#define V3_Y S612
#define V3_Z S622
#define V3_W S632
#define V3   R602
#define F3   R702

#define V4_X S603
#define V4_Y S613
#define V4_Z S623
#define V4_W S633
#define V4   R603
#define F4   R703

#define ADJ_MVP M200 // NOTE: MVP adjusted by guardband scale


// Tests left, right, top, bottom, far planes
//   Does NOT test near plane, as PSP GPU natively clips that
.macro TestInsidePlanes row
	vcmp.q GT, \row[|x|,|y|,z,0], \row[|w|,|w|,w,0] // CC[0] = |x|>|w|...
	bvt 4, behind_a_clipping_plane // CC[4] = (|x|>|w|) | (|y|>|w|) | (z>w) | (0>0)
	nop
.endm

BEG_FUNC(QuadNeedsClipping)
	#define SRC    $a0
	#define STRIDE $a1

	// Load vertex 1
	lv.s	V1_X, 0(SRC);
	lv.s	V1_Y, 4(SRC);
	lv.s	V1_Z, 8(SRC);
	vone.s	V1_W
	addu    SRC, SRC, STRIDE // src++

	// Transform vertex 1
	vhtfm4.q F1, ADJ_MVP, V1

	// Load vertex 2
	lv.s	V2_X, 0(SRC);
	lv.s	V2_Y, 4(SRC);
	lv.s	V2_Z, 8(SRC);
	vone.s	V2_W
	addu    SRC, SRC, STRIDE // src++

	// Check vertex 1 inside guardband region
	TestInsidePlanes F1

	// Transform vertex 2
	vhtfm4.q F2, ADJ_MVP, V2

	// Load vertex 3
	lv.s	V3_X, 0(SRC);
	lv.s	V3_Y, 4(SRC);
	lv.s	V3_Z, 8(SRC);
	vone.s	V3_W
	addu    SRC, SRC, STRIDE // src++

	// Check vertex 2 inside guardband region
	TestInsidePlanes F2

	// Transform vertex 3
	vhtfm4.q F3, ADJ_MVP, V3

	// Load vertex 4
	lv.s	V4_X, 0(SRC);
	lv.s	V4_Y, 4(SRC);
	lv.s	V4_Z, 8(SRC);
	vone.s	V4_W
	//addu    SRC, SRC, STRIDE // src++

	// Check vertex 3 inside guardband region
	TestInsidePlanes F3

	// Transform vertex 4
	vhtfm4.q F4, ADJ_MVP, V4

	// Check vertex 4 inside guardband region
	TestInsidePlanes F4

	jr	$ra
	li $v0, 0

behind_a_clipping_plane:
	jr	$ra
	li $v0, 1

	#undef SRC
	#undef STRIDE
END_FUNC(QuadNeedsClipping)

