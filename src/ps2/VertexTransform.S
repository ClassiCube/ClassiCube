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


.align 4
# Loads matrix into VU0 registers
#	$a0 = addresss of mvp
FUNC LoadMvpMatrix
	lqc2 	MVP1, 0x00($a0) # vf1 = mvp.row1
	lqc2 	MVP2, 0x10($a0) # vf2 = mvp.row2
	lqc2 	MVP3, 0x20($a0) # vf3 = mvp.row3
	jr		$ra
	lqc2 	MVP4, 0x30($a0) # vf4 = mvp.row4

# Loads clipping scaling factors into VU0 registers
#	$a0 = addresss of factors
FUNC LoadClipScaleFactors
	jr		$ra
	lqc2 	CL_F, 0x00($a0)

# Loads viewport origin into VU0 registers
#	$a0 = addresss of origin
FUNC LoadViewportOrigin
	jr		$ra
	lqc2 	VP_O, 0x00($a0)

# Loads viewport scale into VU0 registers
#	$a0 = addresss of scale
FUNC LoadViewportScale
	jr		$ra
	lqc2 	VP_S, 0x00($a0)


.macro TransformVertex1
	# LOAD VERTEX 1
	lqc2	POS_1, 0x00($a2)   # IN = tmp
	# TRANSFORM VERTEX 1
	vmulaw	$ACC,  MVP4, V0001 # ACC[xyzw] = mvp.row3[xyzw] * 1.0f; (vf0.w is 1)
	vmaddax	$ACC,  MVP1, POS_1 # ACC[xyzw] = ACC[xyzw] + mvp.row0[xyzw] * IN.x
	vmadday	$ACC,  MVP2, POS_1 # ACC[xyzw] = ACC[xyzw] + mvp.row1[xyzw] * IN.y
	vmaddz	POS_1, MVP3, POS_1 # OUT[xyzw] = ACC[xyzw] + mvp.row2[xyzw] * IN.z
	sqc2	POS_1, 0x00($a1)   # dst[0] = TRANSFORMED(V0)
	vmul	POSCL, POS_1, CL_F # TMP = TRANSFORMED(V0) * CLIP_PLANES_ADJUST
	# BEGIN CLIP FLAGS CALCULATION VERTEX 1
	vclipw.xyz POSCL, POSCL    # CLIP_FLAGS.append(CLIP(TMP.xyz, TMP.w))
.endm

.macro TransformVertex2
	# LOAD VERTEX 2
	lqc2	POS_2, 0x00($a2)   # IN = tmp
	# TRANSFORM VERTEX 2
	vmulaw	$ACC,  MVP4, V0001 # ACC[xyzw] = mvp.row3[xyzw] * 1.0f; (vf0.w is 1)
	vmaddax	$ACC,  MVP1, POS_2 # ACC[xyzw] = ACC[xyzw] + mvp.row0[xyzw] * IN.x
	vmadday	$ACC,  MVP2, POS_2 # ACC[xyzw] = ACC[xyzw] + mvp.row1[xyzw] * IN.y
	vmaddz	POS_2, MVP3, POS_2 # OUT[xyzw] = ACC[xyzw] + mvp.row2[xyzw] * IN.z
	sqc2	POS_2, 0x10($a1)   # dst[1] = TRANSFORMED(V1)
	vmul	POSCL, POS_2, CL_F # TMP = TRANSFORMED(V1) * CLIP_PLANES_ADJUST
	# STORE CLIP FLAGS VERTEX 1 RESULT
	cfc2	$t0, $18	       # t0 = VP0_REGS[CLIP_FLAGS]
	sw		$t0,0x00($a3)      # clip_flags[0] = t0
	# BEGIN CLIP FLAGS CALCULATION VERTEX 2
	vclipw.xyz POSCL, POSCL    # CLIP_FLAGS.append(CLIP(TMP.xyz, TMP.w))
.endm

.macro TransformVertex3
	# LOAD VERTEX 3
	lqc2	POS_3, 0x00($a2)   # IN = tmp
	# TRANSFORM VERTEX 3
	vmulaw	$ACC,  MVP4, V0001 # ACC[xyzw] = mvp.row3[xyzw] * 1.0f; (vf0.w is 1)
	vmaddax	$ACC,  MVP1, POS_3 # ACC[xyzw] = ACC[xyzw] + mvp.row0[xyzw] * IN.x
	vmadday	$ACC,  MVP2, POS_3 # ACC[xyzw] = ACC[xyzw] + mvp.row1[xyzw] * IN.y
	vmaddz	POS_3, MVP3, POS_3 # OUT[xyzw] = ACC[xyzw] + mvp.row2[xyzw] * IN.z
	sqc2	POS_3, 0x20($a1)   # dst[2] = TRANSFORMED(V2)
	vmul	POSCL, POS_3, CL_F # TMP = TRANSFORMED(V2) * CLIP_PLANES_ADJUST
	# STORE CLIP FLAGS VERTEX 2 RESULT
	cfc2	$t0, $18	       # t0 = VP0_REGS[CLIP_FLAGS]
	sw		$t0,0x04($a3)      # clip_flags[1] = t0
	# BEGIN CLIP FLAGS CALCULATION VERTEX 3
	vclipw.xyz POSCL, POSCL    # CLIP_FLAGS.append(CLIP(TMP.xyz, TMP.w))
.endm

.macro TransformVertex4
	# LOAD VERTEX 4
	lqc2	POS_4, 0x00($a2)   # IN = tmp
	# TRANSFORM VERTEX 4
	vmulaw	$ACC,  MVP4, V0001 # ACC[xyzw] = mvp.row3[xyzw] * 1.0f; (vf0.w is 1)
	vmaddax	$ACC,  MVP1, POS_4 # ACC[xyzw] = ACC[xyzw] + mvp.row0[xyzw] * IN.x
	vmadday	$ACC,  MVP2, POS_4 # ACC[xyzw] = ACC[xyzw] + mvp.row1[xyzw] * IN.y
	vmaddz	POS_4, MVP3, POS_4 # OUT[xyzw] = ACC[xyzw] + mvp.row2[xyzw] * IN.z
	vmul	POSCL, POS_4, CL_F # TMP = TRANSFORMED(V3) * CLIP_PLANES_ADJUST
	# STORE CLIP FLAGS VERTEX 3 RESULT
	cfc2	$t0, $18	       # t0 = VP0_REGS[CLIP_FLAGS]
	sw		$t0,0x08($a3)      # clip_flags[2] = t0
	# BEGIN CLIP FLAGS CALCULATION VERTEX 4
	vclipw.xyz POSCL, POSCL    # CLIP_FLAGS.append(CLIP(TMP.xyz, TMP.w))
.endm

.macro TransformFinish
	# Vertex output
	# dst[0] = V0 (done by TransformVertex1)
	# dst[1] = V1 (done by TransformVertex2)
	# dst[2] = V2 (done by TransformVertex3)
	# dst[3] = V2
	# dst[4] = V3
	# dst[5] = V0
	sqc2	POS_3, 0x30($a1)   # dst[3] = TRANSFORMED(V2)
	sqc2	POS_4, 0x40($a1)   # dst[4] = TRANSFORMED(V3)
	sqc2	POS_1, 0x50($a1)   # dst[5] = TRANSFORMED(V0)
	vnop					   # adjust for delay

	# STORE CLIP FLAGS 4 RESULT
	cfc2	$t0, $18	  	   # t0 = VP0_REGS[CLIP_FLAGS]
	jr		$ra
	sw		$t0,0x0C($a3)      # clip_flags[3] = t0
.endm


# Transforms 4 vertices with size of 24 bytes
#	$a0 = addresss of src  vertices
#	$a1 = addresss of dst  vertices
#   $a2 = address of  tmp  vertex
#   $a3 = address of clip flags
FUNC TransformTexturedQuad
	# LOAD VERTEX 1
	ld		$t0,0x00($a0) # t0 = src[0].x,y
	sd		$t0,0x00($a2) # tmp.x,y = t0
	lw		$t0,0x08($a0) # t0 = src[0].z
	sw		$t0,0x08($a2) # tmp.z = t0
	TransformVertex1

	# LOAD VERTEX 2
	ld		$t0,0x18($a0) # t0 = src[1].x,y
	sd		$t0,0x00($a2) # tmp.x,y = t0
	lw		$t0,0x20($a0) # t0 = src[1].z
	sw		$t0,0x08($a2) # tmp.z = t0
	TransformVertex2

	# LOAD VERTEX 3
	ld		$t0,0x30($a0) # t0 = src[2].x,y
	sd		$t0,0x00($a2) # tmp.x,y = t0
	lw		$t0,0x38($a0) # t0 = src[2].z
	sw		$t0,0x08($a2) # tmp.z = t0
	TransformVertex3

	# LOAD VERTEX 4
	ld		$t0,0x48($a0) # t0 = src[3].x,y
	sd		$t0,0x00($a2) # tmp.x,y = t0
	lw		$t0,0x50($a0) # t0 = src[3].z
	sw		$t0,0x08($a2) # tmp.z = t0
	TransformVertex4

	TransformFinish

# Transforms 4 vertices with size of 16 bytes
#	$a0 = addresss of src  vertices
#	$a1 = addresss of dst  vertices
#   $a2 = address of  tmp  vertex
#   $a3 = address of clip flags
FUNC TransformColouredQuad
	# LOAD VERTEX 1
	ld		$t0,0x00($a0) # t0 = src[0].x,y
	sd		$t0,0x00($a2) # tmp.x,y = t0
	lw		$t0,0x08($a0) # t0 = src[0].z
	sw		$t0,0x08($a2) # tmp.z = t0
	TransformVertex1

	# LOAD VERTEX 2
	ld		$t0,0x10($a0) # t0 = src[1].x,y
	sd		$t0,0x00($a2) # tmp.x,y = t0
	lw		$t0,0x18($a0) # t0 = src[1].z
	sw		$t0,0x08($a2) # tmp.z = t0
	TransformVertex2

	# LOAD VERTEX 3
	ld		$t0,0x20($a0) # t0 = src[2].x,y
	sd		$t0,0x00($a2) # tmp.x,y = t0
	lw		$t0,0x28($a0) # t0 = src[2].z
	sw		$t0,0x08($a2) # tmp.z = t0
	TransformVertex3

	# LOAD VERTEX 4
	ld		$t0,0x30($a0) # t0 = src[3].x,y
	sd		$t0,0x00($a2) # tmp.x,y = t0
	lw		$t0,0x38($a0) # t0 = src[3].z
	sw		$t0,0x08($a2) # tmp.z = t0
	TransformVertex4

	TransformFinish


#	$a0 = addresss of src
#	$a1 = addresss of dst
FUNC ViewportTransform
	lqc2	$vf16, 0x00($a0)    # IN = src
	vmulw	$vf17, $vf16, $vf16 # TMP = IN[xyzw] * IN.w (inverse W)
	vmul	$vf18, $vf17, VP_S  # TMP = TMP * viewport_scale
	vadd	$vf19, $vf18, VP_O  # TMP = TMP + viewport_origin
	vftoi0  $vf19, $vf19	    # TMP = int(TMP)
	jr		$ra
	sqc2	$vf19, 0x00($a1)    # dst = TMP
