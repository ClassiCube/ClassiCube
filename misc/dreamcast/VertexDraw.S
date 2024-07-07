! r8  = STORE_QUEUE
! r9  = num vertices left
! r10 = PVR_CMD_VERTEX
! r11 = PVR_CMD_VERTEX_EOL
! r12 = ClipLine function
! r13 = cur vertex
! r14 = next vertex (prefetch)

!fr8  = VIEWPORT_HWIDTH
!fr9  = VIEWPORT_HHEIGHT
!fr10 = VIEWPORT_X_PLUS_HWIDTH
!fr11 = VIEWPORT_Y_PLUS_HHEIGHT

.align 4

! Pushes a vertex to the store queue
!   CLOBBERS: r3
!   INPUTS:   R (vertex), r8 (SQ global)
!   OUTPUTS:  r8 altered
.macro PushVertex R
	! memcpy(r8, \R, 32)
	mov.l   @(0,\R), r3
	mov.l   r3, @(0,r8)
	mov.l   @(4,\R), r3
	mov.l   r3, @(4,r8)
	mov.l   @(8,\R), r3
	mov.l   r3, @(8,r8)
	mov.l   @(12,\R),r3
	mov.l   r3,@(12,r8)
	mov.l   @(16,\R),r3
	mov.l   r3,@(16,r8)
	mov.l   @(20,\R),r3
	mov.l   r3,@(20,r8)
	mov.l   @(24,\R),r3
	mov.l   r3,@(24,r8)
	mov.l   @(28,\R),r3
	mov.l   r3,@(28,r8) 
	pref    @r8         ! LS, Trigger SQ
	add     #32,r8      ! EX, SQ += 32
.endm


! Pushes a vertex to the store queue
!   CLOBBERS: fr0, fr4, fr5
!   INPUTS:   R (vertex)
!   OUTPUTS:
! TODO optimise greatly  
.macro ViewportTransform R
! INVERSE W CALCULATION
    add #28, \R       ! EX, \R  = &vertex->w
    fmov.s  @\R,fr0   ! LS, fr0 = vertex->w
    fmul    fr0,fr0   ! FE, fr0 = fr0 * fr0
    add #-24, \R      ! EX, \R  = &vertex->x
    fsrra   fr0       ! FE, fr0 = 1 / sqrt(fr0) -> 1 / vertex->w

! TRANSFORM X
    fmov.s @\R,fr4    ! LS, fr4 = vertex->x
    fmov  fr10,fr5    ! LS, fr5 = VIEWPORT_X_PLUS_HWIDTH
    fmul  fr8,fr4     ! FE, fr4 = VIEWPORT_HWIDTH * vertex->x
    fmac  fr0,fr4,fr5 ! FE, fr5 = fr0 * fr4 + fr5 -- (X * F * hwidth) + x_plus_hwidth
    fmov.s fr5,@\R    ! LS, vertex->x = fr5
    add #4, \R        ! EX, \R  = &vertex->y

! TRANSFORM Y
    fmov.s @\R,fr4    ! LS, fr4 = vertex->y
    fmov  fr11,fr5    ! LS, fr5  = VIEWPORT_Y_PLUS_HHEIGHT
    fmul  fr9,fr4     ! FE, fr4  = VIEWPORT_HHEIGHT * vertex->y
    fmac  fr0,fr4,fr5 ! FE, fr5  = fr0 * fr4 + fr5 -- (Y * F * hheight) + y_plus_hheight
    fmov.s fr5,@\R    ! LS, vertex->y = fr5
    add #4, \R        ! EX, \R  = &vertex->z

! ASSIGN Z
    fmov.s fr0,@\R    ! LS, vertex->z = fr0
    add #-12, \R      ! EX, \R -= 12 (back to start of vertex)
.endm

! Transforms then pushes a vertex to the store queue
!   CLOBBERS: r3, fr0, fr4, fr5
!   INPUTS:   R (vertex), r8 (SQ global)
!   OUTPUTS:  r8 altered
.macro TransformVertex R
! INVERSE W CALCULATION
    add #28, \R       ! EX, \R  = &vertex->w
    fmov.s  @\R,fr0   ! LS, fr0 = vertex->w
    fmul    fr0,fr0   ! FE, fr0 = fr0 * fr0
    add #-24, \R      ! EX, \R  = &vertex->x
    fsrra   fr0       ! FE, fr0 = 1 / sqrt(fr0) -> 1 / vertex->w

! TRANSFORM X
    fmov.s @\R,fr4    ! LS, fr4 = vertex->x
    fmov  fr10,fr5    ! LS, fr5 = VIEWPORT_X_PLUS_HWIDTH
    fmul  fr8,fr4     ! FE, fr4 = VIEWPORT_HWIDTH * vertex->x
    fmac  fr0,fr4,fr5 ! FE, fr5 = fr0 * fr4 + fr5 -- (X * F * hwidth) + x_plus_hwidth
    fmov.s fr5,@\R    ! LS, vertex->x = fr5
    add #4, \R        ! EX, \R  = &vertex->y

! TRANSFORM Y
    fmov.s @\R,fr4    ! LS, fr4 = vertex->y
    fmov  fr11,fr5    ! LS, fr5  = VIEWPORT_Y_PLUS_HHEIGHT
    fmul  fr9,fr4     ! FE, fr4  = VIEWPORT_HHEIGHT * vertex->y
    fmac  fr0,fr4,fr5 ! FE, fr5  = fr0 * fr4 + fr5 -- (Y * F * hheight) + y_plus_hheight
    fmov.s fr5,@\R    ! LS, vertex->y = fr5
    add #4, \R        ! EX, \R  = &vertex->z

! ASSIGN Z
    fmov.s fr0,@\R    ! LS, vertex->z = fr0
    add #-12, \R      ! EX, \R -= 12 (back to start of vertex)	

! memcpy(r8, \R, 28)
	mov.l   @(0,\R), r3
	mov.l   r3, @(0,r8)
	mov.l   @(4,\R), r3
	mov.l   r3, @(4,r8)
	mov.l   @(8,\R), r3
	mov.l   r3, @(8,r8)
	mov.l   @(12,\R),r3
	mov.l   r3,@(12,r8)
	mov.l   @(16,\R),r3
	mov.l   r3,@(16,r8)
	mov.l   @(20,\R),r3
	mov.l   r3,@(20,r8)
	mov.l   @(24,\R),r3
	mov.l   r3,@(24,r8)
	pref    @r8         ! LS, Trigger SQ
	add     #32,r8      ! EX, SQ += 32
.endm


#define REG_CLIP1 r1
#define REG_CLIP2 r2

#define REG_V0 r4
#define REG_V1 r5
#define REG_V2 r6
#define REG_V3 r7

#define REG_CMD_VTX  r10
#define REG_CMD_EOL  r11
#define REG_CLIPFUNC r12

! r3 also matches out parameter for ClipLine
#define REG_TMP r3
#define TMP_SET_A \
	mov r15, REG_TMP

#define TMP_SET_B \
	mov r15, REG_TMP; add #32, REG_TMP


_Case_0_0_0_1:
	rts; nop
	!          v0
	!         / |
	!       /   |
	! .....A....B...
	!    /      |
	!  v3--v2---v1
	sts.l      pr,@-r15

	TMP_SET_A
	mov    REG_V3, REG_CLIP1
	mov    REG_V0, REG_CLIP2
	mov.l  REG_CMD_EOL, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TMP_SET_B
	mov    REG_V0, REG_CLIP1
	mov    REG_V1, REG_CLIP2
	mov.l  REG_CMD_VTX, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TransformVertex REG_V0
	TMP_SET_B
	TransformVertex REG_TMP
	TMP_SET_A
	TransformVertex REG_TMP

	rts
	lds.l   @r15+,pr

_Case_0_0_1_0:
	rts; nop
	!          v1
	!         / |
	!       /   |
	! ....A.....B...
	!    /      |
	!  v0--v3---v2
	sts.l      pr,@-r15

	TMP_SET_A
	mov    REG_V0, REG_CLIP1
	mov    REG_V1, REG_CLIP2
	mov.l  REG_CMD_VTX, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TMP_SET_B
	mov    REG_V0, REG_CLIP1
	mov    REG_V1, REG_CLIP2
	mov.l  REG_CMD_EOL, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TMP_SET_A
	TransformVertex REG_TMP
	TransformVertex REG_V1
	TMP_SET_B
	TransformVertex REG_TMP

	rts
	lds.l   @r15+,pr

_Case_0_1_0_0:
	rts; nop
 	!          v2
	!         / |
	!       /   |
	! ....A.....B...
	!    /      |
	!  v1--v0---v3
	sts.l      pr,@-r15

	TMP_SET_A
	mov    REG_V1, REG_CLIP1
	mov    REG_V2, REG_CLIP2
	mov.l  REG_CMD_VTX, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TMP_SET_B
	mov    REG_V2, REG_CLIP1
	mov    REG_V3, REG_CLIP2
	mov.l  REG_CMD_EOL, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TMP_SET_A
	TransformVertex REG_TMP
	TransformVertex REG_V2
	TMP_SET_B
	TransformVertex REG_TMP

	rts
	lds.l   @r15+,pr

_Case_1_0_0_0:
	rts; nop
	!          v3
	!         / |
	!       /   |
	! ....A.....B...
	!    /      |
	!  v2--v1---v0
	sts.l      pr,@-r15

	TMP_SET_A
	mov    REG_V2, REG_CLIP1
	mov    REG_V3, REG_CLIP2
	mov.l  REG_CMD_VTX, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TMP_SET_B
	mov    REG_V3, REG_CLIP1
	mov    REG_V0, REG_CLIP2
	mov.l  REG_CMD_VTX, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TMP_SET_B
	TransformVertex REG_TMP
	TMP_SET_A
	TransformVertex REG_TMP
	TransformVertex REG_V3

	rts
	lds.l   @r15+,pr


_Case_0_0_1_1:
	rts; nop
	!    v0-----------v1
	!      \           |
	!   ....B..........A...
	!         \        |
	!          v3-----v2
	sts.l      pr,@-r15

	TMP_SET_A
	mov    REG_V1, REG_CLIP1
	mov    REG_V2, REG_CLIP2
	mov.l  REG_CMD_VTX, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TMP_SET_B
	mov    REG_V3, REG_CLIP1
	mov    REG_V0, REG_CLIP2
	mov.l  REG_CMD_EOL, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TransformVertex REG_V1
	TMP_SET_A
	TransformVertex REG_TMP
	TransformVertex REG_V0
	TMP_SET_B
	TransformVertex REG_TMP

	rts
	lds.l   @r15+,pr

_Case_1_0_0_1:
	rts; nop
	!    v3-----------v0
	!      \           |
	!   ....B..........A...
	!         \        |
	!          v2-----v1
	sts.l      pr,@-r15

	TMP_SET_A
	mov    REG_V0, REG_CLIP1
	mov    REG_V1, REG_CLIP2
	mov.l  REG_CMD_VTX, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TMP_SET_B
	mov    REG_V2, REG_CLIP1
	mov    REG_V3, REG_CLIP2
	mov.l  REG_CMD_VTX, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TMP_SET_A
	TransformVertex REG_TMP
	TMP_SET_B
	TransformVertex REG_TMP
	TransformVertex REG_V0
	TransformVertex REG_V3

	rts
	lds.l   @r15+,pr

_Case_0_1_1_0:
	rts; nop
	!    v1-----------v2
	!      \           |
	!   ....B..........A...
	!         \        |
 	!          v0-----v3
	sts.l      pr,@-r15

	TMP_SET_A
	mov    REG_V2, REG_CLIP1
	mov    REG_V3, REG_CLIP2
	mov.l  REG_CMD_EOL, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TMP_SET_B
	mov    REG_V0, REG_CLIP1
	mov    REG_V1, REG_CLIP2
	mov.l  REG_CMD_VTX, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TransformVertex REG_V1
	TransformVertex REG_V2
	TMP_SET_B
	TransformVertex REG_TMP
	TMP_SET_A
	TransformVertex REG_TMP

	rts
	lds.l   @r15+,pr

_Case_1_1_0_0:
	rts; nop
	!    v2-----------v3
	!      \           |
	!   ....B..........A...
	!         \        |
	!          v1-----v0
	sts.l      pr,@-r15

	TMP_SET_A
	mov    REG_V3, REG_CLIP1
	mov    REG_V0, REG_CLIP2
	mov.l  REG_CMD_VTX, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TMP_SET_B
	mov    REG_V1, REG_CLIP1
	mov    REG_V2, REG_CLIP2
	mov.l  REG_CMD_VTX, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TMP_SET_B
	TransformVertex REG_TMP
	TransformVertex REG_V2
	TMP_SET_A
	TransformVertex REG_TMP
	TransformVertex REG_V3

	rts
	lds.l   @r15+,pr

_Case_0_1_1_1:
	rts; nop
	!        --v1--
	!    v0--      --v2
	!      \        |
	!   .....B.....A...
	!          \   |
	!            v3
	! v1,v2,v0  v2,v0,A  v0,A,B
	sts.l      pr,@-r15

	TMP_SET_A
	mov    REG_V2, REG_CLIP1
	mov    REG_V3, REG_CLIP2
	mov.l  REG_CMD_VTX, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TMP_SET_B
	mov    REG_V3, REG_CLIP1
	mov    REG_V0, REG_CLIP2
	mov.l  REG_CMD_EOL, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TransformVertex REG_V1
	TransformVertex REG_V2
	TransformVertex REG_V0
	TMP_SET_A
	TransformVertex REG_TMP
	TMP_SET_B
	TransformVertex REG_TMP

	rts
	lds.l   @r15+,pr

_Case_1_0_1_1:
	rts; nop
	!        --v0--
	!    v3--      --v1
	!      \        |
	!   .....B.....A...
	!          \   |
	!            v2
	! v0,v1,v3  v1,v3,A  v3,A,B
	sts.l      pr,@-r15

	TMP_SET_A
	mov    REG_V1, REG_CLIP1
	mov    REG_V2, REG_CLIP2
	mov.l  REG_CMD_VTX, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TMP_SET_B
	mov    REG_V2, REG_CLIP1
	mov    REG_V3, REG_CLIP2
	mov.l  REG_CMD_EOL, @REG_TMP
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_VTX, @REG_V3

	TransformVertex REG_V0
	TransformVertex REG_V1
	TransformVertex REG_V3
	TMP_SET_A
	TransformVertex REG_TMP
	TMP_SET_B
	TransformVertex REG_TMP

	rts
	lds.l   @r15+,pr

_Case_1_1_0_1:
	rts; nop
	!        --v3--
	!    v2--      --v0
	!      \        |
	!   .....B.....A...
	!          \   |
	!            v1
	! v3,v0,v2  v0,v2,A  v2,A,B
	sts.l      pr,@-r15

	TMP_SET_A
	mov    REG_V0, REG_CLIP1
	mov    REG_V1, REG_CLIP2
	mov.l  REG_CMD_VTX, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TMP_SET_B
	mov    REG_V1, REG_CLIP1
	mov    REG_V2, REG_CLIP2
	mov.l  REG_CMD_EOL, @REG_TMP
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_VTX, @REG_V3

	TransformVertex REG_V3
	TransformVertex REG_V0
	TransformVertex REG_V2
	TMP_SET_A
	TransformVertex REG_TMP
	TMP_SET_B
	TransformVertex REG_TMP

	rts
	lds.l   @r15+,pr

_Case_1_1_1_0:
	rts; nop
	!        --v2--
	!    v1--      --v3
	!      \        |
	!   .....B.....A...
	!          \   |
	!            v0
	! v2,v3,v1  v3,v1,A  v1,A,B
	sts.l      pr,@-r15

	TMP_SET_A
	mov    REG_V3, REG_CLIP1
	mov    REG_V0, REG_CLIP2
	mov.l  REG_CMD_VTX, @REG_TMP
	jsr @REG_CLIPFUNC
	nop

	TMP_SET_B
	mov    REG_V0, REG_CLIP1
	mov    REG_V1, REG_CLIP2
	mov.l  REG_CMD_EOL, @REG_TMP
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_VTX, @REG_V3

	TransformVertex REG_V2
	TransformVertex REG_V3
	TransformVertex REG_V1
	TMP_SET_A
	TransformVertex REG_TMP
	TMP_SET_B
	TransformVertex REG_TMP

	rts
	lds.l   @r15+,pr

_Case_1_1_1_1:
! Triangle strip: {1,2,0} {2,0,3}
	TransformVertex REG_V1
	TransformVertex REG_V2
	TransformVertex REG_V0
	TransformVertex REG_V3
	rts
	nop

.global _ProcessVertexList
.align 4
.type  _ProcessVertexList,%function
_ProcessVertexList:
! STORE CPU REGISTERS
	mov.l    r8,@-r15
	mov.l    r9,@-r15
	mov.l   r10,@-r15
	mov.l   r11,@-r15
	mov.l   r12,@-r15
	mov.l   r13,@-r15
	mov.l   r14,@-r15
	sts.l    pr,@-r15
! STORE FPU REGISTERS
	fmov.s  fr8,@-r15
	fmov.s  fr9,@-r15
	fmov.s fr10,@-r15
	fmov.s fr11,@-r15
! VIEWPORT SETUP
    mov.l  .VP_1,r0   ! LS,  r0  = &vp
    fmov.s	@r0+,fr8  ! LS, fr8  = vp.HWIDTH
    fmov.s	@r0+,fr9  ! LS, fr9  = vp.HHEIGHT
    fmov.s	@r0+,fr10 ! LS, fr10 = vp.X_PLUS_HWIDTH
    fmov.s	@r0+,fr11 ! LS, fr11 = vp.Y_PLUS_HHEIGHT
! REGISTER SETUP
	mov      r4,r14
	mov      r4,r13
	mov.l  .CLIPFUNC,r12
	mov.l  .PVR_EOL, r11
	mov.l  .PVR_VTX, r10
	mov      r5,r9
	mov      r6,r8	
	bra     SUBMIT_LOOP
	add    #-64,r15

! Handles a non-vertex command
DO_CMD:
	mov     r13,r4     ! r4 = CUR
	mov     r1,r0      ! r0 = MASK
	cmp/eq  #35,r0     ! T  = MASK == 0x23
	bt.s    9f         ! if (T) goto 9;
	nop
! PowerVR GPU command
	PushVertex REG_V0
	bra     NEXT_ITER
	nop
! Viewport update command
9:
	add      #4,r4
	mov.l   .VP_1,r2
	! Load VIEWPORT registers
	fmov.s  @r4+, fr8  ! VIEWPORT_HWIDTH  = src->x
	fmov.s  @r4+, fr9  ! VIEWPORT_HHEIGHT = src->y
	fmov.s  @r4+,fr10  ! VIEWPORT_X_PLUS_HWIDTH  = src->z
	add      #16,r2
	fmov.s  @r4+,fr11  ! VIEWPORT_Y_PLUS_HHEIGHT = src->u
	! And store to vp global
	fmov.s  fr11,@-r2
	fmov.s  fr10,@-r2
	fmov.s   fr9,@-r2
	bra     NEXT_ITER
	fmov.s   fr8,@-r2

SUBMIT_LOOP:
	mov.l   @r13,r0   ! FLAGS = CUR->flags
	add     #32,r14   ! NEXT += sizeof(Vertex)
	mov     r0,r2     ! TYPE = FLAGS
	and     r11,r2    ! TYPE = FLAGS & 0xF000000 (reuse PVR_CMD_VERTEX_EOL as type mask)
! Check for PVR_CMD_VERTEX
	cmp/eq  r10,r2    ! T = r2 == PVR_CMD_VERTEX
	bt.s    NEXT_ITER ! if (T) goto NEXT_ITER
	pref    @r14      ! prefetch(NEXT) -- always executed
! Check for non PVR_CMD_VERTEX_EOL
	cmp/eq  r11,r2    ! T = r2 == PVR_CMD_VERTEX_EOL
	bf.s    DO_CMD    ! if (!T) goto DO_CMD
! PVR_CMD_VERTEX_EOL case
	extu.b  r0,r1     ! EX, MASK = FLAGS & 0xFF (branch delay slot)
	mov.l   r11,@r13  ! LS, CUR->flags = PVR_CMD_VERTEX_EOL

! Prepare and then jump to quad drawing function, based on quad clipflags
	mova    .CASES,r0   ! LS, r0 = CASES
	mov     r13,r7      ! MT, r7 = v3
	shll2   r1          ! EX, MASK <<= 2
	mov     r13,r6      ! MT, r6 = v3
	mov.l   @(r0,r1),r2 ! LS, r1 = CASES[MASK]
	mov     r13,r5      ! MT, r5 = v3
	add     #-32,r6     ! EX, r6 = v3 - 1 (v2)
	mov     r13,r4      ! MT, r4 = v3
	add     #-64,r5     ! EX, r5 = v3 - 2 (v1)
	jsr     @r2         ! C0, jump CASES[MASK]
	add     #-96,r4     ! EX, r4 = v3 - 3 (v0) (branch delay slot)
NEXT_ITER:
	dt r9               ! NUM--; T = NUM == 0
	bf.s    SUBMIT_LOOP
	mov     r14,r13     ! CUR = NEXT 

	add      #64,r15
! VIEWPORT SAVE
	mov.l  .VP_1,r0
	add      #16,r0
	fmov.s  fr11,@-r0
	fmov.s  fr10,@-r0
 	fmov.s   fr9,@-r0
 	fmov.s   fr8,@-r0
! RESTORE FPU REGISTERS
	fmov.s  @r15+, fr8
	fmov.s  @r15+, fr9
	fmov.s  @r15+,fr10
	fmov.s  @r15+,fr11
! RESTORE CPU REGISTERS
	lds.l   @r15+,pr
	mov.l   @r15+,r14
	mov.l   @r15+,r13
	mov.l   @r15+,r12
	mov.l   @r15+,r11
	mov.l   @r15+,r10
	mov.l   @r15+,r9
	rts     
	mov.l   @r15+,r8

.align 4
.VP_1:
        .long   _vp
.PVR_VTX:
        .long   0xE0000000
.PVR_EOL:
        .long   0xF0000000
.CLIPFUNC:
        .long   _ClipLine

! CASES table holds the functions to transfer a quad,
!  based on the visibility clipflags of the 4 vertices
!  e.g. CASES[15] = V0_VIS | V1_VIS | V2_VIS | V3_VIS (all 4 visible)
.CASES:
	.long   _arch_exit ! Should never happen
	.long   _Case_0_0_0_1
	.long   _Case_0_0_1_0
	.long   _Case_0_0_1_1
	.long   _Case_0_1_0_0
	.long   _arch_exit ! V0_VIS | V2_VIS, Should never happen
	.long   _Case_0_1_1_0
	.long   _Case_0_1_1_1
	.long   _Case_1_0_0_0
	.long   _Case_1_0_0_1
	.long   _arch_exit ! V1_VIS | V3_VIS, Should never happen
	.long   _Case_1_0_1_1
	.long   _Case_1_1_0_0
	.long   _Case_1_1_0_1
	.long   _Case_1_1_1_0
	.long   _Case_1_1_1_1
