# REGISTER USAGE
# vf0 = hardware coded to (0,0,0,1)
# vf1 = mvp.row1
# vf2 = mvp.row2
# vf3 = mvp.row3
# vf4 = mvp.row4
# vf5 = clipping scale adjustments to match guardbands
# vf6 = viewport origin
# vf7 = viewport scale
# NOTE: vclipw.xyz takes 4 cycles to produce result, which must be accounted for

.align 4

.global LoadMvpMatrix
.type   LoadMvpMatrix,%function
.global LoadClipScaleFactors
.type   LoadClipScaleFactors,%function
.global LoadViewportOrigin
.type   LoadViewportOrigin,%function
.global LoadViewportScale
.type   LoadViewportScale,%function

.global TransformTexturedQuad
.type   TransformTexturedQuad,%function
.global TransformColouredQuad
.type   TransformColouredQuad,%function
.global ViewportTransform
.type   ViewportTransform,%function

# Loads matrix into VU0 registers
#	$a0 = addresss of mvp
LoadMvpMatrix:
	lqc2 	$vf1, 0x00($a0) # vf1 = mvp.row1
	lqc2 	$vf2, 0x10($a0) # vf2 = mvp.row2
	lqc2 	$vf3, 0x20($a0) # vf3 = mvp.row3
	lqc2 	$vf4, 0x30($a0) # vf4 = mvp.row4
	jr		$ra
	nop


# Loads clipping scaling factors into VU0 registers
#	$a0 = addresss of factors
LoadClipScaleFactors:
	lqc2 	$vf5, 0x00($a0) # vf5 = factors
	jr		$ra
	nop


# Loads viewport origin into VU0 registers
#	$a0 = addresss of origin
LoadViewportOrigin:
	lqc2 	$vf6, 0x00($a0) # vf6 = origin
	jr		$ra
	nop


# Loads viewport scale into VU0 registers
#	$a0 = addresss of scale
LoadViewportScale:
	lqc2 	$vf7, 0x00($a0) # vf7 = scale
	jr		$ra
	nop


.macro TransformVertex1
	# LOAD VERTEX 1
	lqc2	$vf10, 0x00($a2)   # IN = tmp
	# TRANSFORM VERTEX 1
	vmulaw	$ACC,  $vf4,  $vf0 # ACC[xyzw] = mvp.row3[xyzw] * 1.0f; (vf0.w is 1)
	vmaddax	$ACC,  $vf1, $vf10 # ACC[xyzw] = ACC[xyzw] + mvp.row0[xyzw] * IN.x
	vmadday	$ACC,  $vf2, $vf10 # ACC[xyzw] = ACC[xyzw] + mvp.row1[xyzw] * IN.y
	vmaddz	$vf11, $vf3, $vf10 # OUT[xyzw] = ACC[xyzw] + mvp.row2[xyzw] * IN.z
	sqc2	$vf11, 0x00($a1)   # dst[0] = TRANSFORMED(V0)
	vmul	$vf10, $vf11, $vf5 # TMP = TRANSFORMED(V0) * CLIP_PLANES_ADJUST
	# BEGIN CLIP FLAGS CALCULATION VERTEX 1
	vclipw.xyz $vf10, $vf10    # CLIP_FLAGS.append(CLIP(TMP.xyz, TMP.w))
.endm

.macro TransformVertex2
	# LOAD VERTEX 2
	lqc2	$vf12, 0x00($a2)   # IN = tmp
	# TRANSFORM VERTEX 2
	vmulaw	$ACC,  $vf4,  $vf0 # ACC[xyzw] = mvp.row3[xyzw] * 1.0f; (vf0.w is 1)
	vmaddax	$ACC,  $vf1, $vf12 # ACC[xyzw] = ACC[xyzw] + mvp.row0[xyzw] * IN.x
	vmadday	$ACC,  $vf2, $vf12 # ACC[xyzw] = ACC[xyzw] + mvp.row1[xyzw] * IN.y
	vmaddz	$vf13, $vf3, $vf12 # OUT[xyzw] = ACC[xyzw] + mvp.row2[xyzw] * IN.z
	sqc2	$vf13, 0x10($a1)   # dst[1] = TRANSFORMED(V1)
	vmul	$vf12, $vf13, $vf5 # TMP = TRANSFORMED(V1) * CLIP_PLANES_ADJUST
	# STORE CLIP FLAGS VERTEX 1 RESULT
	cfc2	$t0, $18	  # t0 = VP0_REGS[CLIP_FLAGS]
	sw		$t0,0x00($a3) # clip_flags[0] = t0
	# BEGIN CLIP FLAGS CALCULATION VERTEX 2
	vclipw.xyz $vf12, $vf12    # CLIP_FLAGS.append(CLIP(TMP.xyz, TMP.w))
.endm

.macro TransformVertex3
	# LOAD VERTEX 3
	lqc2	$vf14, 0x00($a2)   # IN = tmp
	# TRANSFORM VERTEX 3
	vmulaw	$ACC,  $vf4,  $vf0 # ACC[xyzw] = mvp.row3[xyzw] * 1.0f; (vf0.w is 1)
	vmaddax	$ACC,  $vf1, $vf14 # ACC[xyzw] = ACC[xyzw] + mvp.row0[xyzw] * IN.x
	vmadday	$ACC,  $vf2, $vf14 # ACC[xyzw] = ACC[xyzw] + mvp.row1[xyzw] * IN.y
	vmaddz	$vf15, $vf3, $vf14 # OUT[xyzw] = ACC[xyzw] + mvp.row2[xyzw] * IN.z
	sqc2	$vf15, 0x20($a1)   # dst[2] = TRANSFORMED(V2)
	vmul	$vf14, $vf15, $vf5 # TMP = TRANSFORMED(V2) * CLIP_PLANES_ADJUST
	# STORE CLIP FLAGS VERTEX 2 RESULT
	cfc2	$t0, $18	  # t0 = VP0_REGS[CLIP_FLAGS]
	sw		$t0,0x04($a3) # clip_flags[1] = t0
	# BEGIN CLIP FLAGS CALCULATION VERTEX 3
	vclipw.xyz $vf14, $vf14    # CLIP_FLAGS.append(CLIP(TMP.xyz, TMP.w))
.endm

.macro TransformVertex4
	# LOAD VERTEX 4
	lqc2	$vf16, 0x00($a2)   # IN = tmp
	# TRANSFORM VERTEX 4
	vmulaw	$ACC,  $vf4,  $vf0 # ACC[xyzw] = mvp.row3[xyzw] * 1.0f; (vf0.w is 1)
	vmaddax	$ACC,  $vf1, $vf16 # ACC[xyzw] = ACC[xyzw] + mvp.row0[xyzw] * IN.x
	vmadday	$ACC,  $vf2, $vf16 # ACC[xyzw] = ACC[xyzw] + mvp.row1[xyzw] * IN.y
	vmaddz	$vf17, $vf3, $vf16 # OUT[xyzw] = ACC[xyzw] + mvp.row2[xyzw] * IN.z
	vmul	$vf16, $vf17, $vf5 # TMP = TRANSFORMED(V3) * CLIP_PLANES_ADJUST
	# STORE CLIP FLAGS VERTEX 3 RESULT
	cfc2	$t0, $18	  # t0 = VP0_REGS[CLIP_FLAGS]
	sw		$t0,0x08($a3) # clip_flags[2] = t0
	# BEGIN CLIP FLAGS CALCULATION VERTEX 4
	vclipw.xyz $vf16, $vf16    # CLIP_FLAGS.append(CLIP(TMP.xyz, TMP.w))
.endm

.macro TransformFinish
	# Vertex output
	# dst[0] = V0 (done by TransformVertex1)
	# dst[1] = V1 (done by TransformVertex2)
	# dst[2] = V2 (done by TransformVertex3)
	# dst[3] = V2
	# dst[4] = V3
	# dst[5] = V0
	sqc2	$vf15, 0x30($a1)   # dst[3] = TRANSFORMED(V2)
	sqc2	$vf17, 0x40($a1)   # dst[4] = TRANSFORMED(V3)
	sqc2	$vf11, 0x50($a1)   # dst[5] = TRANSFORMED(V0)
	vnop					   # adjust for delay

	# STORE CLIP FLAGS 4 RESULT
	cfc2	$t0, $18	  	   # t0 = VP0_REGS[CLIP_FLAGS]
	sw		$t0,0x0C($a3)      # clip_flags[3] = t0
.endm


# Transforms 4 vertices with size of 24 bytes
#	$a0 = addresss of src  vertices
#	$a1 = addresss of dst  vertices
#   $a2 = address of  tmp  vertex
#   $a3 = address of clip flags
TransformTexturedQuad:
	# LOAD 1.0 into W
	lw		$t0,ONE_VALUE # t0 = 1.0f
	sw		$t0,0x0C($a2) # tmp.w = f5

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
	jr		$ra
	nop

# Transforms 4 vertices with size of 16 bytes
#	$a0 = addresss of src  vertices
#	$a1 = addresss of dst  vertices
#   $a2 = address of  tmp  vertex
#   $a3 = address of clip flags
TransformColouredQuad:
	# LOAD 1.0 into W
	lw		$t0,ONE_VALUE # t0 = 1.0f
	sw		$t0,0x0C($a2) # tmp.w = f5

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
	jr		$ra
	nop

.global ONE_VALUE
ONE_VALUE:  .float 1.0


#	$a0 = addresss of src
#	$a1 = addresss of dst
ViewportTransform:
	lqc2	$vf16, 0x00($a0)    # IN = src
	vmulw	$vf17, $vf16, $vf16 # TMP = IN[xyzw] * IN.w (inverse W)
	vmul	$vf18, $vf17, $vf7  # TMP = TMP * viewport_scale
	vadd	$vf19, $vf18, $vf6  # TMP = TMP + viewport_origin
	vftoi0  $vf19, $vf19	    # TMP = int(TMP)
	sqc2	$vf19, 0x00($a1)    # dst = TMP
	jr		$ra
	nop
