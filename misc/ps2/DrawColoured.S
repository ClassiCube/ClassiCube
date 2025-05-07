# NOTE: vclipw.xyz takes 4 cycles to produce result, which must be accounted for

.macro FUNC name
	.global \name
	.type   \name,%function
	\name:
.endm

# mips ISA has explicit delay slots
# (i.e. instruction after branches/jumps are always unconditionally executed)
.set noreorder

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

	#define CL1 $t1 // clip flags for vertex 1
	#define CL2 $t2 // clip flags for vertex 2
	#define CL3 $t3 // clip flags for vertex 3
	#define CL4 $t4 // clip flags for vertex 4

	#define COL0 $f12
	#define COL1 $f13
	#define COL2 $f14
	#define COL3 $f15

	#define XY_0 $t1
	#define XY_1 $t2
	#define XY_2 $t3
	#define XY_3 $t4


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
	vmulw.xyz	\vpos, \vpos, POS_1 # TMP.xyz = IN.xyz * IN.w (inverse W)
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
	# LOAD VERTEX 1
	ld			$t0,0x00(SRC) # t0 = src[0].x,y
	sd			$t0,0x00(TMP) # tmp.x,y = t0
	lw			$t0,0x08(SRC) # t0 = src[0].z
	sw			$t0,0x08(TMP) # tmp.z = t0
	lqc2		POS_1, 0x00(TMP) # V1 = tmp

	TransformVertex POS_1
	vdiv	 	$Q, _one, POS1w
	BeginClip POS_1

### VERTEX 2 ###
	# LOAD VERTEX 2
	ld			$t0,0x10(SRC) # t0 = src[1].x,y
	sd			$t0,0x00(TMP) # tmp.x,y = t0
	lw			$t0,0x18(SRC) # t0 = src[1].z
	sw			$t0,0x08(TMP) # tmp.z = t0
	lqc2		POS_2, 0x00(TMP) # V2 = tmp

	TransformVertex POS_2
	vmulq.w 	POS_1, V0001, $Q
	vdiv	 	$Q, _one, POS2w
	BeginClip POS_2

### VERTEX 3 ###
	# LOAD VERTEX 3
	ld			$t0,0x20(SRC) # t0 = src[2].x,y
	sd			$t0,0x00(TMP) # tmp.x,y = t0
	lw			$t0,0x28(SRC) # t0 = src[2].z
	sw			$t0,0x08(TMP) # tmp.z = t0
	lqc2		POS_3, 0x00(TMP) # V3 = tmp

	TransformVertex POS_3
	vmulq.w 	POS_2, V0001, $Q
	vdiv 		$Q, _one, POS3w
	BeginClip POS_3

### VERTEX 4 ###
	# LOAD VERTEX 4
	ld			$t0,0x30(SRC) # t0 = src[3].x,y
	sd			$t0,0x00(TMP) # tmp.x,y = t0
	lw			$t0,0x38(SRC) # t0 = src[3].z
	sw			$t0,0x08(TMP) # tmp.z = t0
	lqc2		POS_4, 0x00(TMP) # V4 = tmp

	TransformVertex POS_4
	vmulq.w 	POS_3, V0001, $Q
	vdiv 		$Q, _one, POS4w
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

	# TODO clipping
any_clipped_vertices:
	jr $ra
	move $v0, DST

