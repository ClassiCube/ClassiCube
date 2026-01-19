#include "AsmCommon.i"
// NOTE: vclipw.xyz takes 4 cycles to produce result, which must be accounted for

// mips ISA has explicit delay slots
// (i.e. instruction after branches/jumps are always unconditionally executed)
.set noreorder

// Note that registers are numbered for N32 ABI, but when disassembling
//   in ghidra or PCSX2, they are disassembled as O32 ABI ?
// https://github.com/ps2dev/binutils-gdb/blob/e9cf3691bfa140469d52815a2307b00eecf7917c/gas/config/tc-mips.c#L2786

# transform temp registers
	#define POSCL $vf10 // TRANSFORMED(POS_[1234]) * CLIP_PLANES_ADJUST
	#define POS_1 $vf11 // vertex 1 position
	#define POS_2 $vf12 // vertex 2 position
	#define POS_3 $vf13 // vertex 3 position
	#define POS_4 $vf14 // vertex 4 position

	#define _one   $vf0w
	#define POS1w $vf11w
	#define POS2w $vf12w
	#define POS3w $vf13w
	#define POS4w $vf14w

	#define SRC $a0
	#define DST $a1
	#define TMP $a2

	#define Z_1 $f0
	#define Z_2 $f1
	#define Z_3 $f2
	#define Z_4 $f3

	#define U_1 $f4
	#define U_2 $f5
	#define U_3 $f6
	#define U_4 $f7

	#define V_1 $f8
	#define V_2 $f9
	#define V_3 $f10
	#define V_4 $f11

	#define COL1 $f12
	#define COL2 $f13
	#define COL3 $f14
	#define COL4 $f15

	#define W_1 $f16
	#define W_2 $f17
	#define W_3 $f18
	#define W_4 $f19

	#define XY_1 $t1
	#define XY_2 $t2
	#define XY_3 $t3
	#define XY_4 $a3

	#define Y_1 $a4
	#define Y_2 $a5
	#define Y_3 $a6
	#define Y_4 $a7


.macro TransformVertex vpos
	vmulaw		$ACC,  MVP4, V0001 # ACC[xyzw] = mvp.row3[xyzw] * 1.0f; (vf0.w is 1)
	vmaddax		$ACC,  MVP1, \vpos # ACC[xyzw] = ACC[xyzw] + mvp.row0[xyzw] * VEC.x
	vmadday		$ACC,  MVP2, \vpos # ACC[xyzw] = ACC[xyzw] + mvp.row1[xyzw] * VEC.y
	vmaddz		\vpos, MVP3, \vpos # VEC[xyzw] = ACC[xyzw] + mvp.row2[xyzw] * VEC.z
.endm

.macro BeginClip vpos
	vmul		POSCL, \vpos, CL_F  # TMP = TRANSFORMED(VEC) * CLIP_PLANES_ADJUST
	# begin clip flags calculation
	vclipw.xyz	POSCL, POSCL    	# CLIP_FLAGS.append(CLIP(TMP.xyz, TMP.w))
.endm

.macro VPTransform vpos
	vmulw.xyz	\vpos, \vpos, \vpos # TMP.xyz = IN.xyz * IN.w (inverse W)
	vmul.xyz	\vpos, \vpos, VP_S  # TMP.xyz = TMP * viewport_scale
	vadd.xyz	\vpos, \vpos, VP_O  # TMP.xyz = TMP + viewport_origin
	vftoi0.xyz	\vpos, \vpos	    # TMP.xyz = int(TMP)
.endm

// Fully transforms 4 vertices with size of 16 bytes
//	$a0 = addresss of src  vertices
//	$a1 = addresss of dst  vertices
//  $a2 = address of  tmp  vertex
//	$v0 = address of final vertices (return value)
BEG_FUNC(DrawTexturedQuad)

### VERTEX 1 ###
	lqc2		POS_1, 0x00(SRC)

	TransformVertex POS_1
	lwc1		COL1, 0x0C(SRC)
	lwc1		U_1,  0x10(SRC)
	lwc1		V_1,  0x14(SRC)
	vdiv	 	$Q,   _one, POS1w
	BeginClip POS_1

### VERTEX 2 ###
	# LOAD VERTEX 2 (todo rearrange vertices for efficient loading)
	ld			$t0,0x18(SRC) # t0 = src[1].x,y
	sd			$t0,0x00(TMP) # tmp.x,y = t0
	lw			$t0,0x20(SRC) # t0 = src[1].z
	sw			$t0,0x08(TMP) # tmp.z = t0
	lqc2		POS_2, 0x00(TMP) # V2 = tmp

	TransformVertex POS_2
	lwc1		COL2,  0x24(SRC)
	lwc1		U_2,   0x28(SRC)
	lwc1		V_2,   0x2C(SRC)

	vmulq.w 	POS_1, V0001, $Q
	vdiv	 	$Q,    _one, POS2w
	BeginClip POS_2

### VERTEX 3 ###
	lqc2		POS_3, 0x30(SRC)

	TransformVertex POS_3
	lwc1		COL3,  0x3C(SRC)
	lwc1		U_3,   0x40(SRC)
	lwc1		V_3,   0x44(SRC)

	vmulq.w 	POS_2, V0001, $Q
	vdiv 		$Q,    _one, POS3w
	BeginClip POS_3

### VERTEX 4 ###
	# LOAD VERTEX 4
	ld			$t0,0x48(SRC) # t0 = src[3].x,y
	sd			$t0,0x00(TMP) # tmp.x,y = t0
	lw			$t0,0x50(SRC) # t0 = src[3].z
	sw			$t0,0x08(TMP) # tmp.z = t0
	lqc2		POS_4, 0x00(TMP) # V4 = tmp

	TransformVertex POS_4
	lwc1		COL4,  0x54(SRC)
	lwc1		U_4,   0x58(SRC)
	lwc1		V_4,   0x5C(SRC)

	vmulq.w 	POS_3, V0001, $Q
	vdiv 		$Q,    _one, POS4w
	BeginClip POS_4

	vnop # adjust for delay
	vnop # adjust for delay
	vnop # adjust for delay

	# STORE CLIP FLAGS 4 RESULT
	vwaitq
	vmulq.w 	POS_4, V0001, $Q

	# check if any vertices would need clipping
	cfc2 		$t0, $18 # t0 = VP0_REGS[CLIP_FLAGS]
	bnez 		$t0, any_clipped_vertices
	nop

	# output vertices
	VPTransform POS_1
	VPTransform POS_2
	VPTransform POS_3
	VPTransform POS_4

	# Convert to register format
	sqc2		POS_1, 0x00(TMP)
	sqc2		POS_2, 0x10(TMP)
	sqc2		POS_3, 0x20(TMP)
	sqc2		POS_4, 0x30(TMP)

	lhu			XY_1, 0x00(TMP)
	lhu			 Y_1, 0x04(TMP)
	lwc1		 Z_1, 0x08(TMP)
	lwc1		 W_1, 0x0C(TMP)

	lhu			XY_2, 0x10(TMP)
	lhu			 Y_2, 0x14(TMP)
	mul.s		 U_1, U_1, W_1
	lwc1		 Z_2, 0x18(TMP)
	lwc1		 W_2, 0x1C(TMP)
	mul.s		 V_1, V_1, W_1

	lhu			XY_3, 0x20(TMP)
	lhu			 Y_3, 0x24(TMP)
	mul.s		 U_2, U_2, W_2
	lwc1		 Z_3, 0x28(TMP)
	lwc1		 W_3, 0x2C(TMP)
	mul.s		 V_2, V_2, W_2

	lhu			XY_4, 0x30(TMP)
	lhu			 Y_4, 0x34(TMP)
	mul.s		 U_3, U_3, W_3
	lwc1		 Z_4, 0x38(TMP)
	lwc1		 W_4, 0x3C(TMP)
	mul.s		 V_3, V_3, W_3

	sll 		Y_1, Y_1, 16
	sll			Y_2, Y_2, 16
	sll 		Y_3, Y_3, 16
	sll 		Y_4, Y_4, 16
	mul.s		U_4, U_4, W_4

	or			XY_1, XY_1, Y_1
	or			XY_2, XY_2, Y_2
	or			XY_3, XY_3, Y_3
	or			XY_4, XY_4, Y_4
	mul.s		 V_4,  V_4, W_4

	# write 1,2,3 3,4,1
	swc1 COL1, (0*S_PTEX + PTEX__C)(DST)
	swc1  W_1, (0*S_PTEX + PTEX__W)(DST)
	swc1  U_1, (0*S_PTEX + PTEX__U)(DST)
	swc1  V_1, (0*S_PTEX + PTEX__V)(DST)
	sw   XY_1, (0*S_PTEX + PTEX_XY)(DST)
	swc1  Z_1, (0*S_PTEX + PTEX__Z)(DST)

	swc1 COL2, (1*S_PTEX + PTEX__C)(DST)
	swc1  W_2, (1*S_PTEX + PTEX__W)(DST)
	swc1  U_2, (1*S_PTEX + PTEX__U)(DST)
	swc1  V_2, (1*S_PTEX + PTEX__V)(DST)
	sw   XY_2, (1*S_PTEX + PTEX_XY)(DST)
	swc1  Z_2, (1*S_PTEX + PTEX__Z)(DST)

	swc1 COL3, (2*S_PTEX + PTEX__C)(DST)
	swc1  W_3, (2*S_PTEX + PTEX__W)(DST)
	swc1  U_3, (2*S_PTEX + PTEX__U)(DST)
	swc1  V_3, (2*S_PTEX + PTEX__V)(DST)
	sw   XY_3, (2*S_PTEX + PTEX_XY)(DST)
	swc1  Z_3, (2*S_PTEX + PTEX__Z)(DST)

	swc1 COL3, (3*S_PTEX + PTEX__C)(DST)
	swc1  W_3, (3*S_PTEX + PTEX__W)(DST)
	swc1  U_3, (3*S_PTEX + PTEX__U)(DST)
	swc1  V_3, (3*S_PTEX + PTEX__V)(DST)
	sw   XY_3, (3*S_PTEX + PTEX_XY)(DST)
	swc1  Z_3, (3*S_PTEX + PTEX__Z)(DST)

	swc1 COL4, (4*S_PTEX + PTEX__C)(DST)
	swc1  W_4, (4*S_PTEX + PTEX__W)(DST)
	swc1  U_4, (4*S_PTEX + PTEX__U)(DST)
	swc1  V_4, (4*S_PTEX + PTEX__V)(DST)
	sw   XY_4, (4*S_PTEX + PTEX_XY)(DST)
	swc1  Z_4, (4*S_PTEX + PTEX__Z)(DST)

	swc1 COL1, (5*S_PTEX + PTEX__C)(DST)
	swc1  W_1, (5*S_PTEX + PTEX__W)(DST)
	swc1  U_1, (5*S_PTEX + PTEX__U)(DST)
	swc1  V_1, (5*S_PTEX + PTEX__V)(DST)
	sw   XY_1, (5*S_PTEX + PTEX_XY)(DST)
	swc1  Z_1, (5*S_PTEX + PTEX__Z)(DST)

	addi DST, 6 * S_PTEX

	# TODO clipping
any_clipped_vertices:
	jr $ra
	move $v0, DST
END_FUNC(DrawTexturedQuad)
