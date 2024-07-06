! r8  = STORE_QUEUE
! r9  = num vertices left
! r10 = PVR_CMD_VERTEX
! r11 = PVR_CMD_VERTEX_EOL
! r12 = 0xFF000000
! r13 = cur vertex
! r14 = next vertex (prefetch)

#define REG_V0 r4
#define REG_V1 r5
#define REG_V2 r6
#define REG_V3 r7

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


_Case_1_1_1_1:
! Triangle strip: {1,2,0} {2,0,3}
	!_glPerspectiveDivideVertex(v1);
	PushVertex REG_V1
	
	!_glPerspectiveDivideVertex(v2);
	PushVertex REG_V2
	
	!_glPerspectiveDivideVertex(v0);
	PushVertex REG_V0
	
	!_glPerspectiveDivideVertex(v3);
	PushVertex REG_V3
	rts
	nop


_ProcessVertexList:
! STORE REGISTERS
	mov.l   r8,@-r15
	mov.l   r9,@-r15
	mov.l   r10,@-r15
	mov.l   r11,@-r15
	mov.l   r12,@-r15
	mov.l   r13,@-r15
	mov.l   r14,@-r15
	sts.l   pr,@-r15
	mov     r4,r14
	mov     r4,r13
	mov.l   .L33,r12
	mov.l   .L35,r11
	mov.l   .L34,r10
	mov     r5,r9
	bra     SUBMIT_LOOP
	nop

DO_CMD:
	mov.l   .L37,r2
	jsr     @r2
	nop
	bra     NEXT_ITER
	nop

SUBMIT_LOOP:
	mov.l   @r13,r0   ! FLAGS = CUR->flags
	add     #32,r14   ! NEXT += sizeof(Vertex)
	mov     r1,r2     ! TYPE = FLAGS
	and     r12,r2    ! TYPE = FLAGS & 0xFF000000
! Check for PVR_CMD_VERTEX
	cmp/eq  r10,r2    ! T = r2 == PVR_CMD_VERTEX
	bt.s    NEXT_ITER ! if (T) goto NEXT_ITER
	pref    @r14      ! prefetch(NEXT) -- always executed
! Check for non PVR_CMD_VERTEX_EOL
	cmp/eq  r11,r2    ! T = r2 == PVR_CMD_VERTEX_EOL
	bf.s    DO_CMD    ! if (!T) goto DO_CMD
! PVR_CMD_VERTEX_EOL case
	extu.b  r1,r1     ! EX, MASK = FLAGS & 0xFF (branch delay slot)
	mov.l   r12,@r13  ! LS, CUR->flags = TYPE

! Prepare and then jump to quad drawing function, based on quad clipflags
	mova    CASES,r0    ! LS, r0 = CASES
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

! RESTORE REGISTERS
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
.L33:
        .long   -16777216
.L34:
        .long   -536870912
.L35:
        .long   -268435456
.L37:
        .long   _HandleCommand

! CASES table holds the functions to transfer a quad,
!  based on the visibility clipflags of the 4 vertices
!  e.g. CASES[15] = V0_VIS | V1_VIS | V2_VIS | V3_VIS (all 4 visible)
CASES:
	.long   0 ! Should never happen
	.long   _Case_0_0_0_1
	.long   _Case_0_0_1_0
	.long   _Case_0_0_1_1
	.long   _Case_0_1_0_0
	.long   _Case_0_1_0_1
	.long   _Case_0_1_1_0
	.long   _Case_0_1_1_1
	.long   _Case_1_0_0_0
	.long   _Case_1_0_0_1
	.long   _Case_1_0_1_0
	.long   _Case_1_0_1_1
	.long   _Case_1_1_0_0
	.long   _Case_1_1_0_1
	.long   _Case_1_1_1_0
	.long   _Case_1_1_1_1
