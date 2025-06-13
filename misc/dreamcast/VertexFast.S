#define FLG  r0   // clip flags
#define TMP  r1   // temp
#define V1_U r2   // vertex 1.u
#define V1_V r3   // vertex 1.v
#define SRC  r4   // src pointer ARG
#define CNT  r5   // quads count ARG
#define V1_C r6   // vertex 1.colour
#define V2_U r7   // vertex 2.u
#define V2_V r8   // vertex 2.u
#define V2_C r9   // vertex 2.colour
#define V3_U r10  // vertex 3.u
#define V3_V r11  // vertex 3.v
#define V3_C r12  // vertex 3.colour
#define V4_U r13  // vertex 4.u
#define V4_V r14  // vertex 4.v
#define V4_C r15  // vertex 4.colour

#define VTX macl  // PVR_CMD_VERTEX
#define EOS mach  // PVR_CMD_VERTEX_EOL

#define F1_X fr0  // vertex 1.x
#define F1_Y fr1  // vertex 1.y
#define F1_Z fr2  // vertex 1.z
#define F1_W fr3  // vertex 1.w
#define F2_X fr4  // vertex 2.x
#define F2_Y fr5  // vertex 2.y
#define F2_Z fr6  // vertex 2.z
#define F2_W fr7  // vertex 2.w
#define F3_X fr8  // vertex 3.x
#define F3_Y fr9  // vertex 3.y
#define F3_Z fr10 // vertex 3.z
#define F3_W fr11 // vertex 3.w
#define F4_X fr12 // vertex 4.x
#define F4_Y fr13 // vertex 4.y
#define F4_Z fr14 // vertex 4.z
#define F4_W fr15 // vertex 4.w

#define F1_XYZW fv0
#define F2_XYZW fv4
#define F3_XYZW fv8
#define F4_XYZW fv12


.global _SubmitQuads
.align 4

_SubmitQuads:
! Setup
    mov #0xE0, r0     ! EX, tmp = 0x00 00 00 E0
    pref  @SRC        ! LS, prefetch 0..31 (all 24 bytes v1, first 8 bytes v2)
    shll16 r0         ! EX, tmp = 0x00 E0 00 00
    shll8  r0         ! EX, tmp = 0xE0 00 00 00 (PVR_CMD_VERTEX)
	lds	r0,VTX		  ! CO, VTX = tmp
	
    mov #0xF0, r1     ! EX, tmp = 0x00 00 00 F0
    shll16 r1         ! EX, tmp = 0x00 F0 00 00
    shll8  r1         ! EX, tmp = 0xF0 00 00 00 (PVR_CMD_VERTEX_EOL)
	lds	r1,EOS		  ! CO, EOS = tmp

! Save non-volatile registers
    fmov.s fr15,@-r15	! LS
    fmov.s fr14,@-r15	! LS
    fmov.s fr13,@-r15	! LS
    fmov.s fr12,@-r15	! LS
	mov.l    r8,@-r15	! LS
	mov.l    r9,@-r15	! LS
	mov.l   r10,@-r15	! LS
	mov.l   r11,@-r15	! LS
	mov.l   r12,@-r15	! LS
	mov.l   r13,@-r15	! LS
	mov.l   r14,@-r15	! LS
!	sts.l    pr,@-r15	! LS
! Save stack pointer elsewhere
	ldc		r15,DBR		! CO

.T_TRANSFORM_QUAD:

! LOAD VERTEX 1 XYZW
    fmov @SRC+, F1_X    ! LS, v1.X = src->x
    fmov @SRC+, F1_Y    ! LS, v1.Y = src->y
    fmov @SRC+, F1_Z    ! LS, v1.Z = src->z
    fldi1 F1_W          ! LS, v1.W = 1.0
! MISC OPERATIONS
    add    #20, SRC     ! EX, src += 20 (offset now 32)
    pref   @SRC         ! LS, prefetch 32..63 (last 16 bytes of v2, first 16 bytes v3)
! TRANSFORM VERTEX 1
    ftrv xmtrx, F1_XYZW ! FE, TRANSFORM(v1.XYZW)
! LOAD VERTEX 1 ATTRIBUTES + MISC OPERATIONS
	add   #-20, SRC		! EX, src -= 20 (offset now 12)
    mov.l @SRC+, V1_C   ! LS, v1.C = src->color
    fldi0 F4_W          ! LS, v4.W = 0.0
    mov.l @SRC+, V1_U   ! LS, v1.U = src->u
    mov.l @SRC+, V1_V   ! LS, v1.V = src->v
! PROCESS VERTEX 1
    fcmp/gt F4_W, F1_Z  ! FE, T = v1.Z > 0
	fmul    F1_W, F1_W  ! FE, v1.W = v1.W * v1.W
    movt    FLG         ! EX, CLIPFLAGS = T (00 00 00 V1)


! LOAD VERTEX 2 XYZW
    fmov @SRC+, F2_X    ! LS, v2.X = src->x
    fmov @SRC+, F2_Y    ! LS, v2.Y = src->y
    fmov @SRC+, F2_Z    ! LS, v2.Z = src->z
    fldi1 F2_W          ! LS, v2.W = 1.0
! MISC OPERATIONS
    add    #28, SRC     ! EX, src += 28 (offset now 64)
    pref   @SRC         ! LS, prefetch 64..96 (last 8 bytes of v3, first 24 bytes v4)
! TRANSFORM VERTEX 2
    ftrv xmtrx, F2_XYZW ! FE, TRANSFORM(v2.XYZW)
! LOAD VERTEX 2 ATTRIBUTES + MISC OPERATIONS
	add   #-28, SRC		! EX, src -= 28 (offset now 36)
    mov.l @SRC+, V2_C   ! LS, v2.C = src->color
    shll    FLG         ! EX, CLIPFLAGS <<= 1  (00 00 V1 00)
    mov.l @SRC+, V2_U   ! LS, v2.U = src->u
    mov.l @SRC+, V2_V   ! LS, v2.V = src->v
! PROCESS VERTEX 2    
	fsrra  F1_W 		! FE, v1.W = 1/sqrt(v1.W*v1.W) ~ 1/v1.W
    fcmp/gt F4_W,F2_Z   ! FE, T = v2.Z > 0
	fmul    F2_W, F2_W  ! FE, v2.W = v2.W * v2.W
    movt    TMP         ! EX, tmp = T


! LOAD VERTEX 3 XYZW
    fmov @SRC+, F3_X    ! LS, v3.X = src->x
    fmov @SRC+, F3_Y    ! LS, v3.Y = src->y
    fmov @SRC+, F3_Z    ! LS, v3.Z = src->z
    fldi1 F3_W          ! LS, v3.W = 1.0
! MISC OPERATIONS
    flds F1_Z, fpul     ! LS, ftmp = v1.Z
! TRANSFORM VERTEX 3
    ftrv xmtrx, F3_XYZW ! FE, TRANSFORM(v3.XYZW)
! LOAD VERTEX 3 ATTRIBUTES + MISC OPERATIONS
    or      TMP,FLG     ! EX, CLIPFLAGS |= tmp (00 00 V1 V2)
    mov.l @SRC+, V3_C   ! LS, v3.C = src->color
    shll    FLG         ! EX, CLIPFLAGS <<= 1  (00 V1 V2 00)
    mov.l @SRC+, V3_U   ! LS, v3.U = src->u
    mov.l @SRC+, V3_V   ! LS, v3.V = src->v
! PROCESS VERTEX 3
	fsrra  F2_W 		! FE, v2.W = 1/sqrt(v2.W*v2.W) ~ 1/v2.W
    fcmp/gt F4_W,F3_Z   ! FE, T = v3.Z > 0
	fmul    F3_W, F3_W  ! FE, v3.W = v3.W * v3.W
    movt    TMP         ! EX, tmp = T


! LOAD VERTEX 4 XYZW
    fmov @SRC+, F4_X    ! LS, v4.X = src->x
    fmov @SRC+, F4_Y    ! LS, v4.Y = src->y
    fmov @SRC+, F4_Z    ! LS, v4.Z = src->z
    fldi1 F4_W          ! LS, v4.W = 1.0
! MISC OPERATIONS
    add    #12, SRC     ! EX, src += 12 (offset now 96)
    pref  @SRC          ! LS, prefetch next 0..31 (all 24 bytes v1, first 8 bytes v2)
! TRANSFORM VERTEX 4
    ftrv xmtrx, F4_XYZW ! FE, TRANSFORM(v4.XYZW)
! LOAD VERTEX 4 ATTRIBUTES + MISC OPERATIONS
    add   #-12, SRC     ! EX, src -= 12 (offset now 84)
    fldi0 F1_Z          ! LS, v1.Z = 0.0
    or      TMP,FLG     ! EX, CLIPFLAGS |= tmp (00 V1 V2 V3)
    mov.l @SRC+, V4_C   ! LS, v4.C = src->color
    shll    FLG         ! EX, CLIPFLAGS <<= 1  (V1 V2 V3 00)
    mov.l @SRC+, V4_U   ! LS, v4.U = src->u
    mov.l @SRC+, V4_V   ! LS, v4.V = src->v
! PROCESS VERTEX 4
	fsrra  F3_W 		! FE, v3.W = 1/sqrt(v3.W*v3.W) ~ 1/v3.W
    fcmp/gt F1_Z, F4_Z  ! FE, T = v4.Z > 0
	fmul    F4_W, F4_W  ! FE, v4.W = v4.W * v4.W
    movt    TMP         ! EX, tmp = T
    or      TMP,FLG     ! EX, CLIPFLAGS |= tmp (V1 V2 V3 V4)


! CLIPFLAGS TESTING
    cmp/eq  #15,FLG      ! T = CLIPFLAGS == 15 (all points visible)
    bf/s    .T_LOOP_END  ! if !T goto LOOP_END
    nop


! VERTEX SUBMISSION (Triangle strip: {2,3,1} {3,1,4})
	sts	VTX,TMP			 ! CO, TMP = E0000000 (store queue address)
	fsrra  F4_W 		 ! FE, v4.W = 1/sqrt(v4.W*v4.W) ~ 1/v4.W
! SUBMIT VERTEX 2
	add       #28, TMP   ! EX, dst += 28
	fmul     F2_W, F2_Y  ! FE, v2.Y = v2.W * v2.Y
	mov.l    V2_C, @-TMP ! LS, dst -= 4, *dst = v2.C
	fmul     F2_W, F2_X  ! FE, v2.X = v2.W * v2.X
	mov.l    V2_V, @-TMP ! LS, dst -= 4, *dst = v2.V
	mov.l    V2_U, @-TMP ! LS, dst -= 4, *dst = v2.U
	fmov.s   F2_W, @-TMP ! LS, dst -= 4, *dst = v2.W
	fmov.s   F2_Y, @-TMP ! LS, dst -= 4, *dst = v2.Y
	fmov.s   F2_X, @-TMP ! LS, dst -= 4, *dst = v2.X
	sts.l	 VTX,  @-TMP ! CO, dst -= 4, *dst = VTX
	mov      TMP, FLG    ! MT, dst2 = dst1
	add      #60, FLG    ! EX, dst2 += 60
	pref      @TMP       ! LS, flush store queue
! SUBMIT VERTEX 3
	fmul     F3_W, F3_Y  ! FE, v3.Y = v3.W * v3.Y
	mov.l    V3_C, @-FLG ! LS, dst -= 4, *dst = v3.C
	fmul     F3_W, F3_X  ! FE, v3.X = v3.W * v3.X
	mov.l    V3_V, @-FLG ! LS, dst -= 4, *dst = v3.V
	mov.l    V3_U, @-FLG ! LS, dst -= 4, *dst = v3.U
	fmov.s   F3_W, @-FLG ! LS, dst -= 4, *dst = v3.W
	fmov.s   F3_Y, @-FLG ! LS, dst -= 4, *dst = v3.Y
	fmov.s   F3_X, @-FLG ! LS, dst -= 4, *dst = v3.X
	sts.l	 VTX,  @-FLG ! CO, dst -= 4, *dst = VTX
	mov      FLG, TMP    ! dst3 = dst2
	add      #60, TMP    ! dst3 += 60
	pref      @FLG       ! LS, flush store queue
! SUBMIT VERTEX 1
	fmul     F1_W, F1_Y  ! FE, v1.Y = v1.W * v1.Y
	mov.l    V1_C, @-TMP ! LS, dst -= 4, *dst = v1.C
	fmul     F1_W, F1_X  ! FE, v1.X = v1.W * v1.X
	mov.l    V1_V, @-TMP ! LS, dst -= 4, *dst = v1.V
	mov.l    V1_U, @-TMP ! LS, dst -= 4, *dst = v1.U
	fmov.s   F1_W, @-TMP ! LS, dst -= 4, *dst = v1.W
	fmov.s   F1_Y, @-TMP ! LS, dst -= 4, *dst = v1.Y
	fmov.s   F1_X, @-TMP ! LS, dst -= 4, *dst = v1.X
	sts.l	 VTX,  @-TMP ! CO, dst -= 4, *dst = VTX
	mov      TMP, FLG    ! dst4 = dst3
	add      #60, FLG    ! dst4 += 60
	pref      @TMP       ! LS, flush store queue
! SUBMIT VERTEX 4
	fmul     F4_W, F4_Y  ! FE, v4.Y = v4.W * v4.Y
	mov.l    V4_C, @-FLG ! LS, dst -= 4, *dst = v4.C
	fmul     F4_W, F4_X  ! FE, v4.X = v4.W * v4.X
	mov.l    V4_V, @-FLG ! LS, dst -= 4, *dst = v4.V
	mov.l    V4_U, @-FLG ! LS, dst -= 4, *dst = v4.U
	fmov.s   F4_W, @-FLG ! LS, dst -= 4, *dst = v4.W
	fmov.s   F4_Y, @-FLG ! LS, dst -= 4, *dst = v4.Y
	fmov.s   F4_X, @-FLG ! LS, dst -= 4, *dst = v4.X
	sts.l	 EOS,  @-FLG ! CO, dst -= 4, *dst = EOS
	pref      @FLG       ! LS, flush store queue


.T_LOOP_END:
    dt CNT               ! count--; T = count == 0
    bf .T_TRANSFORM_QUAD ! if !T then goto T_TRANSFORM_QUAD
    nop

! Restore stack pointer
	stc		DBR,r15		! CO
! RESTORE CPU REGISTERS
!	lds.l   @r15+,pr	! CO
	mov.l   @r15+,r14	! LS
	mov.l   @r15+,r13	! LS
	mov.l   @r15+,r12	! LS
	mov.l   @r15+,r11	! LS
	mov.l   @r15+,r10	! LS
	mov.l   @r15+,r9 	! LS
	mov.l   @r15+,r8	! LS
    fmov.s  @r15+,fr12	! LS
    fmov.s  @r15+,fr13	! LS
    fmov.s  @r15+,fr14	! LS
	rts					! CO, return after executing instruction in delay slot
    fmov.s  @r15+,fr15	! LS

.size _SubmitQuads, .-_SubmitQuads
.type _SubmitQuads, %function

