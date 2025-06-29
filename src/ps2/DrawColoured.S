# NOTE: vclipw.xyz takes 4 cycles to produce result, which must be accounted for

.macro FUNC name
	.global \name
	.type   \name,%function
	\name:
.endm

# mips ISA has explicit delay slots
# (i.e. instruction after branches/jumps are always unconditionally executed)
.set noreorder

# Note that registers are numbered for N32 ABI, but when disassembling
#   in ghidra or PCSX2, they are disassembled as O32 ABI ?
# https://github.com/ps2dev/binutils-gdb/blob/e9cf3691bfa140469d52815a2307b00eecf7917c/gas/config/tc-mips.c#L2786

# global registers
	#define V0001 $vf0 // hardware coded to (0,0,0,1)
	#define MVP1  $vf1 // mvp.row1
	#define MVP2  $vf2 // mvp.row2
	#define MVP3  $vf3 // mvp.row3
	#define MVP4  $vf4 // mvp.row4
	#define CL_F  $vf5 // clipping scale adjustments to match guardbands
	#define VP_O  $vf6 // viewport origin
	#define VP_S  $vf7 // viewport scale

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

	#define COL1 $f12
	#define COL2 $f13
	#define COL3 $f14
	#define COL4 $f15

	#define Z_1 $f0
	#define Z_2 $f1
	#define Z_3 $f2
	#define Z_4 $f3

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

# Fully transforms 4 vertices with size of 16 bytes
#	$a0 = addresss of src  vertices
#	$a1 = addresss of dst  vertices
#   $a2 = address of  tmp  vertex
#	$v0 = address of final vertices (return value)
FUNC DrawColouredQuad

### VERTEX 1 ###
	lqc2		POS_1, 0x00(SRC)

	TransformVertex POS_1
	lwc1		COL1, 0x0C(SRC)
	vdiv	 	$Q,   _one, POS1w
	BeginClip POS_1

### VERTEX 2 ###
	lqc2		POS_2, 0x10(SRC)

	TransformVertex POS_2
	lwc1		COL2,  0x1C(SRC)
	vmulq.w 	POS_1, V0001, $Q
	vdiv	 	$Q,    _one, POS2w
	BeginClip POS_2

### VERTEX 3 ###
	lqc2		POS_3, 0x20(SRC)

	TransformVertex POS_3
	lwc1		COL3,  0x2C(SRC)
	vmulq.w 	POS_2, V0001, $Q
	vdiv 		$Q,    _one, POS3w
	BeginClip POS_3

### VERTEX 4 ###
	lqc2		POS_4, 0x30(SRC)

	TransformVertex POS_4
	lwc1		COL4,  0x3C(SRC)
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
	lwc1		 Z_2, 0x18(TMP)
	lwc1		 W_2, 0x1C(TMP)

	lhu			XY_3, 0x20(TMP)
	lhu			 Y_3, 0x24(TMP)
	lwc1		 Z_3, 0x28(TMP)
	lwc1		 W_3, 0x2C(TMP)

	lhu			XY_4, 0x30(TMP)
	lhu			 Y_4, 0x34(TMP)
	lwc1		 Z_4, 0x38(TMP)
	lwc1		 W_4, 0x3C(TMP)

	sll 		Y_1, Y_1, 16
	sll			Y_2, Y_2, 16
	sll 		Y_3, Y_3, 16
	sll 		Y_4, Y_4, 16

	or			XY_1, XY_1, Y_1
	or			XY_2, XY_2, Y_2
	or			XY_3, XY_3, Y_3
	or			XY_4, XY_4, Y_4

	# write 1,2,3 3,4,1
	swc1 COL1, 0x00(DST)
	swc1  W_1, 0x04(DST)
	sw   XY_1, 0x08(DST)
	swc1  Z_1, 0x0C(DST)

	swc1 COL2, 0x10(DST)
	swc1  W_2, 0x14(DST)
	sw   XY_2, 0x18(DST)
	swc1  Z_2, 0x1C(DST)

	swc1 COL3, 0x20(DST)
	swc1  W_3, 0x24(DST)
	sw   XY_3, 0x28(DST)
	swc1  Z_3, 0x2C(DST)

	swc1 COL3, 0x30(DST)
	swc1  W_3, 0x34(DST)
	sw   XY_3, 0x38(DST)
	swc1  Z_3, 0x3C(DST)

	swc1 COL4, 0x40(DST)
	swc1  W_4, 0x44(DST)
	sw   XY_4, 0x48(DST)
	swc1  Z_4, 0x4C(DST)

	swc1 COL1, 0x50(DST)
	swc1  W_1, 0x54(DST)
	sw   XY_1, 0x58(DST)
	swc1  Z_1, 0x5C(DST)

	addi DST, 16*6

	# TODO clipping
any_clipped_vertices:
	jr $ra
	move $v0, DST

