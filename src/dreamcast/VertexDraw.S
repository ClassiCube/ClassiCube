! r8  = STORE_QUEUE
! r9  = num vertices left
! r10 = PVR_CMD_VERTEX
! r11 = PVR_CMD_VERTEX_EOL
! r12 = ClipLine function
! r13 = cur vertex
! r14 = next vertex (prefetch)

#define R_VTX        r10
#define R_EOL        r11
#define REG_CMD_VTX  r10
#define REG_CMD_EOL  r11
#define REG_CLIPFUNC r12


.align 4

! Pushes a vertex to the store queue
!   CLOBBERS: r2
!   INPUTS:   R (vertex), r8 (SQ global)
!   OUTPUTS:  r8 altered
.macro PushVertex R
	! memcpy(r8, \R, 32)
	mov.l   @(0,\R), r2
	mov.l   r2, @(0,r8)
	mov.l   @(4,\R), r2
	mov.l   r2, @(4,r8)
	mov.l   @(8,\R), r2
	mov.l   r2, @(8,r8)
	mov.l   @(12,\R),r2
	mov.l   r2,@(12,r8)
	mov.l   @(16,\R),r2
	mov.l   r2,@(16,r8)
	mov.l   @(20,\R),r2
	mov.l   r2,@(20,r8)
	mov.l   @(24,\R),r2
	mov.l   r2,@(24,r8)
	mov.l   @(28,\R),r2
	mov.l   r2,@(28,r8) 
	pref    @r8         ! LS, Trigger SQ
	add     #32,r8      ! EX, SQ += 32
.endm

! Transforms then pushes a vertex to the store queue
! note: Vertices are assumed as pre viewport transformed already
!   CLOBBERS: r2, fr0, fr4, fr5
!   INPUTS:   R (vertex), r8 (SQ global)
!   OUTPUTS:  R, r8 altered
.macro TransformVertex R
! INVERSE W CALCULATION
    add #28, \R       ! EX, SRC += 28
    fmov.s  @\R,fr0   ! LS, fr0 = v->w
    fmul    fr0,fr0   ! FE, fr0 = fr0 * fr0
    add #-28, \R      ! EX, SRC -= 28
	mov.l   @\R+, r2  ! LS, tmp = SRC->flags, SRC += 4
	mov.l   r2,@r8    ! LS, DST->flags = tmp
    fsrra   fr0       ! FE, invW = 1 / sqrt(SRC->W * SRC->W)
    add    #4, r8     ! EX, DST += 4

! COPY U,V
	mov.l @(12,\R),r2 ! LS, tmp = SRC->u
	mov.l r2,@(12,r8) ! LS, DST->u = tmp
	mov.l @(16,\R),r2 ! LS, tmp = SRC->v
	mov.l r2,@(16,r8) ! LS, DST->v = tmp

! TRANSFORM X
    fmov.s @\R,fr4    ! LS, fr4 = SRC->x
    fmul   fr0,fr4    ! FE, fr4 = invW * SRC->x
	mov.l @(20,\R),r2 ! LS, tmp = SRC->bgra
	mov.l r2,@(20,r8) ! LS, SRC->bgra = tmp
    add    #4, \R     ! EX, SRC += 4
    fmov.s fr4,@r8    ! LS, DST->x = fr4

! TRANSFORM Y
    fmov.s @\R,fr4    ! LS, fr4  = SRC->y
    add    #8, r8     ! EX, DST += 8
    fmul   fr0,fr4    ! FE, fr4 = invW * SRC->y
    fmov.s fr0,@r8    ! LS, DST->z = invW
    add   #-4, r8     ! EX, DST -= 4
    add   #-8, \R     ! EX, src -= 8 (back to start of vertex)
    fmov.s fr4,@r8    ! LS, DST->y = fr4

    add   #-8,r8      ! EX, DST -= 8 (back to start of vertex)	
	pref    @r8       ! LS, Trigger SQ
	add     #32,r8    ! EX, SQ += 32
.endm


#define REG_CLIP1 r1
#define REG_CLIP2 r2

#define REG_V0 r4
#define REG_V1 r5
#define REG_V2 r6
#define REG_V3 r7

! r3 also matches out parameter for ClipLine
#define REG_TMP r3
#define TMP_SET_A \
	mov r15, REG_TMP

#define TMP_SET_B \
	mov r15, REG_TMP; add #32, REG_TMP


_Case_0_0_0_1:
	!          v0
	!         / |
	!       /   |
	! .....A....B...
	!    /      |
	!  v3--v2---v1
	sts    pr,r13

	TMP_SET_A
	mov    REG_V3, REG_CLIP1
	mov    REG_V0, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_EOL, @REG_TMP

	TMP_SET_B
	mov    REG_V0, REG_CLIP1
	mov    REG_V1, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_VTX, @REG_TMP

	TransformVertex REG_V0
	TMP_SET_B
	TransformVertex REG_TMP
	TMP_SET_A
	TransformVertex REG_TMP

	lds   r13,pr
	rts
	nop

_Case_0_0_1_0:
	!          v1
	!         / |
	!       /   |
	! ....A.....B...
	!    /      |
	!  v0--v3---v2
	sts    pr,r13

	TMP_SET_A
	mov    REG_V0, REG_CLIP1
	mov    REG_V1, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_VTX, @REG_TMP

	TMP_SET_B
	mov    REG_V1, REG_CLIP1
	mov    REG_V2, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_EOL, @REG_TMP

	TMP_SET_A
	TransformVertex REG_TMP
	TransformVertex REG_V1
	TMP_SET_B
	TransformVertex REG_TMP

	lds   r13,pr
	rts
	nop

_Case_0_1_0_0:
 	!          v2
	!         / |
	!       /   |
	! ....A.....B...
	!    /      |
	!  v1--v0---v3
	sts    pr,r13

	TMP_SET_A
	mov    REG_V1, REG_CLIP1
	mov    REG_V2, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_VTX, @REG_TMP

	TMP_SET_B
	mov    REG_V2, REG_CLIP1
	mov    REG_V3, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_EOL, @REG_TMP

	TMP_SET_A
	TransformVertex REG_TMP
	TransformVertex REG_V2
	TMP_SET_B
	TransformVertex REG_TMP

	lds   r13,pr
	rts
	nop

_Case_1_0_0_0:
	!          v3
	!         / |
	!       /   |
	! ....A.....B...
	!    /      |
	!  v2--v1---v0
	sts    pr,r13

	TMP_SET_A
	mov    REG_V2, REG_CLIP1
	mov    REG_V3, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_VTX, @REG_TMP

	TMP_SET_B
	mov    REG_V3, REG_CLIP1
	mov    REG_V0, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_VTX, @REG_TMP

	TMP_SET_B
	TransformVertex REG_TMP
	TMP_SET_A
	TransformVertex REG_TMP
	TransformVertex REG_V3

	lds   r13,pr
	rts
	nop


_Case_0_0_1_1:
	!    v0-----------v1
	!      \           |
	!   ....B..........A...
	!         \        |
	!          v3-----v2
	sts    pr,r13

	TMP_SET_A
	mov    REG_V1, REG_CLIP1
	mov    REG_V2, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_VTX, @REG_TMP

	TMP_SET_B
	mov    REG_V3, REG_CLIP1
	mov    REG_V0, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_EOL, @REG_TMP

	TransformVertex REG_V1
	TMP_SET_A
	TransformVertex REG_TMP
	TransformVertex REG_V0
	TMP_SET_B
	TransformVertex REG_TMP

	lds   r13,pr
	rts
	nop

_Case_1_0_0_1:
	!    v3-----------v0
	!      \           |
	!   ....B..........A...
	!         \        |
	!          v2-----v1
	sts    pr,r13

	TMP_SET_A
	mov    REG_V0, REG_CLIP1
	mov    REG_V1, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_VTX, @REG_TMP

	TMP_SET_B
	mov    REG_V2, REG_CLIP1
	mov    REG_V3, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_VTX, @REG_TMP

	TMP_SET_A
	TransformVertex REG_TMP
	TMP_SET_B
	TransformVertex REG_TMP
	TransformVertex REG_V0
	TransformVertex REG_V3

	lds   r13,pr
	rts
	nop

_Case_0_1_1_0:
	!    v1-----------v2
	!      \           |
	!   ....B..........A...
	!         \        |
 	!          v0-----v3
	sts    pr,r13

	TMP_SET_A
	mov    REG_V2, REG_CLIP1
	mov    REG_V3, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_EOL, @REG_TMP

	TMP_SET_B
	mov    REG_V0, REG_CLIP1
	mov    REG_V1, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_VTX, @REG_TMP

	TransformVertex REG_V1
	TransformVertex REG_V2
	TMP_SET_B
	TransformVertex REG_TMP
	TMP_SET_A
	TransformVertex REG_TMP

	lds   r13,pr
	rts
	nop

_Case_1_1_0_0:
	!    v2-----------v3
	!      \           |
	!   ....B..........A...
	!         \        |
	!          v1-----v0
	sts    pr,r13

	TMP_SET_A
	mov    REG_V3, REG_CLIP1
	mov    REG_V0, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_VTX, @REG_TMP

	TMP_SET_B
	mov    REG_V1, REG_CLIP1
	mov    REG_V2, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_VTX, @REG_TMP

	TMP_SET_B
	TransformVertex REG_TMP
	TransformVertex REG_V2
	TMP_SET_A
	TransformVertex REG_TMP
	TransformVertex REG_V3

	lds   r13,pr
	rts
	nop

_Case_0_1_1_1:
	!        --v1--
	!    v0--      --v2
	!      \        |
	!   .....B.....A...
	!          \   |
	!            v3
	! v1,v2,v0  v2,v0,A  v0,A,B
	sts    pr,r13

	TMP_SET_A
	mov    REG_V2, REG_CLIP1
	mov    REG_V3, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_VTX, @REG_TMP

	TMP_SET_B
	mov    REG_V3, REG_CLIP1
	mov    REG_V0, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_EOL, @REG_TMP

	TransformVertex REG_V1
	TransformVertex REG_V2
	TransformVertex REG_V0
	TMP_SET_A
	TransformVertex REG_TMP
	TMP_SET_B
	TransformVertex REG_TMP

	lds   r13,pr
	rts
	nop

_Case_1_0_1_1:
	!        --v0--
	!    v3--      --v1
	!      \        |
	!   .....B.....A...
	!          \   |
	!            v2
	! v0,v1,v3  v1,v3,A  v3,A,B
	sts    pr,r13

	TMP_SET_A
	mov    REG_V1, REG_CLIP1
	mov    REG_V2, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_VTX, @REG_TMP

	TMP_SET_B
	mov    REG_V2, REG_CLIP1
	mov    REG_V3, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_EOL, @REG_TMP
	mov.l  REG_CMD_VTX, @REG_V3

	TransformVertex REG_V0
	TransformVertex REG_V1
	TransformVertex REG_V3
	TMP_SET_A
	TransformVertex REG_TMP
	TMP_SET_B
	TransformVertex REG_TMP

	lds   r13,pr
	rts
	nop

_Case_1_1_0_1:
	!        --v3--
	!    v2--      --v0
	!      \        |
	!   .....B.....A...
	!          \   |
	!            v1
	! v3,v0,v2  v0,v2,A  v2,A,B
	sts    pr,r13

	TMP_SET_A
	mov    REG_V0, REG_CLIP1
	mov    REG_V1, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_VTX, @REG_TMP

	TMP_SET_B
	mov    REG_V1, REG_CLIP1
	mov    REG_V2, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_EOL, @REG_TMP
	mov.l  REG_CMD_VTX, @REG_V3

	TransformVertex REG_V3
	TransformVertex REG_V0
	TransformVertex REG_V2
	TMP_SET_A
	TransformVertex REG_TMP
	TMP_SET_B
	TransformVertex REG_TMP

	lds   r13,pr
	rts
	nop

_Case_1_1_1_0:
	!        --v2--
	!    v1--      --v3
	!      \        |
	!   .....B.....A...
	!          \   |
	!            v0
	! v2,v3,v1  v3,v1,A  v1,A,B
	sts    pr,r13

	TMP_SET_A
	mov    REG_V3, REG_CLIP1
	mov    REG_V0, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_VTX, @REG_TMP

	TMP_SET_B
	mov    REG_V0, REG_CLIP1
	mov    REG_V1, REG_CLIP2
	jsr @REG_CLIPFUNC
	mov.l  REG_CMD_EOL, @REG_TMP
	mov.l  REG_CMD_VTX, @REG_V3

	TransformVertex REG_V2
	TransformVertex REG_V3
	TransformVertex REG_V1
	TMP_SET_A
	TransformVertex REG_TMP
	TMP_SET_B
	TransformVertex REG_TMP

	lds   r13,pr
	rts
	nop

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

! Submits a PVR2 TA GPU command
DO_CMD:
	PushVertex REG_V0
	bra     NEXT_ITER
	nop

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
.size _ProcessVertexList, .-_ProcessVertexList
.type _ProcessVertexList, %function

.align 4
.VP_1:
        .long   _vp
.PVR_VTX:
        .long   0xE0000000
.PVR_EOL:
        .long   0xF0000000
.CLIPFUNC:
        .long   _ClipLine

BUGGY_CASE:
rts
nop

! CASES table holds the functions to transfer a quad,
!  based on the visibility clipflags of the 4 vertices
!  e.g. CASES[15] = V0_VIS | V1_VIS | V2_VIS | V3_VIS (all 4 visible)
.CASES:
	.long   BUGGY_CASE ! Should never happen
	.long   _Case_0_0_0_1
	.long   _Case_0_0_1_0
	.long   _Case_0_0_1_1
	.long   _Case_0_1_0_0
	.long   BUGGY_CASE ! V0_VIS | V2_VIS, Should never happen
	.long   _Case_0_1_1_0
	.long   _Case_0_1_1_1
	.long   _Case_1_0_0_0
	.long   _Case_1_0_0_1
	.long   BUGGY_CASE ! V1_VIS | V3_VIS, Should never happen
	.long   _Case_1_0_1_1
	.long   _Case_1_1_0_0
	.long   _Case_1_1_0_1
	.long   _Case_1_1_1_0
	.long   _Case_1_1_1_1
