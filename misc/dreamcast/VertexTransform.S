! =========================================================
! ======================== PROCESSOR INFO =================
! =========================================================
! The SH4 can dual issue (i.e. parallel execution) two instructions
! as long as the groups of the two instructions are different:
! * LS - most ALU and FPU register load/stores
! * EX - most ALU arithmetic instructions
! * MT - TST, CMP, NOP, MOV Rm,Rn
! * FE - most FPU arithmetic instructions
! * CO - other instructions (NOTE: Cannot be exeucted in parallel)

! Thee following general aspects of instructions are important to note per the SH4 manual:
! * Issue rate: Interval between the issue of an instruction and that of the next instruction
! * Latency: Interval between the issue of an instruction and the generation of its result (completion)
! * Latency is also the interval between the execution of two instructions with an interdependent relationship.
!   (although different cases may either increase or decrease Latency)


! =========================================================
! ======================== REGISTER USAGES ================
! =========================================================
! SH4 C ABI:
! -  R0  to  R3 are return values (can be overwritten)
! -  R4  to  R7 are input arguments (can be overwritten)
! -  R8  to R13 are non-volatile (must be restored at end)
! - R14  is the frame pointer (must be restored at end)
! - R15  is the stack pointer (must be restored at end)
! - FR0  to FR3 are return values (can be overwritten)
! - FR4  to FR11 are input arguments (can be overwritten)
! - FR12 to FR13 are non-volatile (must be restored at end)

!r0 = clip flags
!r1 = GPU command
!r2 = temp
!r3 = prefetch address
!r4 = src pointer ARG
!r5 = dst pointer ARG
!r6 = quads count ARG
!r7 = ?
!r10  = PVR_CMD_VERTEX
!r11  = PVR_CMD_VERTEX_EOL

!fr0  = temp
!fr1  = u
!fr2  = v
!fr3  = c
!fr4  = x
!fr5  = y
!fr6  = z
!fr7  = w
!fr8  = VIEWPORT_HWIDTH
!fr9  = VIEWPORT_HHEIGHT
!fr10 = VIEWPORT_X_PLUS_HWIDTH
!fr11 = VIEWPORT_Y_PLUS_HHEIGHT

!fv4  = XYZW


! =========================================================
! ========================= TRANSFORM SETUP ===============
! =========================================================
.macro TransformSetup
    mov r4,r3        ! MT, r3  = src
    mov.l r10, @-r15 ! LS, push(r10)
    add #-32, r5     ! EX, r5 -= sizeof(VERTEX)
    mov.l r11, @-r15 ! LS, push(r11)
    mov #0xE0, r10   ! EX, r10 = 0x00 00 00 E0
    pref @r3         ! LS, PREFETCH r3 (first vertex)
    shll16 r10       ! EX, r10 = 0x00 E0 00 00
    shll8  r10       ! EX, r10 = 0xE0 00 00 00 (PVR_CMD_VERTEX)
    mov #0xF0, r11   ! EX, r11 = 0x00 00 00 F0
    shll16 r11       ! EX, r11 = 0x00 F0 00 00
    shll8  r11       ! EX, r11 = 0xF0 00 00 00 (PVR_CMD_VERTEX_EOL)
    nop              ! MT, align to even boundary
.endm

.macro TransformEnd
    mov.l @r15+, r11 ! LS, pop(r11)
    mov.l @r15+, r10 ! LS, pop(r10)

    add #32, r5      ! EX, r5 += sizeof(VERTEX)
    rts              ! CO, return after executing instruction in delay slot
    mov r5,r0        ! MT, r0 = r5
.endm


! =========================================================
! ========================= VERTEX LOADING ================
! =========================================================
.macro LoadColouredVertex
! LOAD XYZ
    fmov @r4+, fr4   ! LS, X = src->x
    fmov @r4+, fr5   ! LS, Y = src->y
    fmov @r4+, fr6   ! LS, Z = src->z
    fldi1 fr7        ! LS, W = 1.0
! PREPARE NEXT VERTEX
    add #16, r3      ! EX, r3 += VERTEX_STRIDE
    pref @r3         ! LS, PREFETCH r3 (next vertex)
    add #64, r5      ! EX, r5 += 2 * sizeof(VERTEX)
! TRANSFORM VERTEX
    ftrv xmtrx, fv4  ! FE, TRANSFORM(XYZW)
! LOAD ATTRIBUTES
    fmov   @r4+,fr3  ! LS, C = src->color
.endm

.macro LoadTexturedVertex
! LOAD XYZ
    fmov @r4+, fr4   ! LS, X = src->x
    fmov @r4+, fr5   ! LS, Y = src->y
    fmov @r4+, fr6   ! LS, Z = src->z
    fldi1 fr7        ! LS, W = 1.0
! PREPARE NEXT VERTEX
    add #24, r3      ! EX, r3 += VERTEX_STRIDE
    pref @r3         ! LS, PREFETCH r3 (next vertex)
    add #64, r5      ! EX, r5 += 2 * sizeof(VERTEX)
! TRANSFORM VERTEX
    ftrv xmtrx, fv4  ! FE, TRANSFORM(XYZW)
! LOAD ATTRIBUTES
    fmov    @r4+,fr3 ! LS, C = src->color
    fmov    @r4+,fr1 ! LS, U = src->u
    fmov    @r4+,fr2 ! LS, V = src->v
.endm

! =========================================================
! ========================= VERTEX OUTPUT =================
! =========================================================
! To take advantage of SH4 dual instruction processing, 
!  clipflag calculation and vertex output are interleaved
.macro ProcessVertex1
    fmov.s  fr7,@-r5 ! LS, dst->w = W
    fmov.s  fr3,@-r5 ! LS, dst->c = C
    fldi0   fr0      ! LS, fr0 = 0.0
    fmov.s  fr2,@-r5 ! LS, dst->v = V
    fcmp/gt fr0,fr6  ! FE, T = Z > 0
    fmov.s  fr1,@-r5 ! LS, dst->u = U
    movt    r0       ! EX, CLIPFLAGS = T
    fmov.s  fr6,@-r5 ! LS, dst->z = Z
    fmov.s  fr5,@-r5 ! LS, dst->y = Y
    fmov.s  fr4,@-r5 ! LS, dst->x = X
    mov.l   r10,@-r5 ! LS, dst->flags = PVR_CMD_VERTEX
.endm

.macro ProcessVertex2
    fmov.s  fr7,@-r5 ! LS, dst->w = W
    fmov.s  fr3,@-r5 ! LS, dst->c = C
    fldi0   fr0      ! LS, fr0 = 0.0
    fmov.s  fr2,@-r5 ! LS, dst->v = V
    fcmp/gt fr0,fr6  ! FE, T = Z > 0
    fmov.s  fr1,@-r5 ! LS, dst->u = U
    movt    r2       ! EX, tmp = T
    fmov.s  fr6,@-r5 ! LS, dst->z = Z
    add     r2,r2    ! EX, tmp = tmp + tmp
    fmov.s  fr5,@-r5 ! LS, dst->y = Y
    or      r2,r0    ! EX, CLIPFLAGS |= tmp (T << 1)
    fmov.s  fr4,@-r5 ! LS, dst->x = X
    mov.l   r10,@-r5 ! LS, dst->flags = PVR_CMD_VERTEX
.endm

.macro ProcessVertex3
    fmov.s  fr7,@-r5 ! LS, dst->w = W
    fmov.s  fr3,@-r5 ! LS, dst->c = C
    fldi0   fr0      ! LS, fr0 = 0.0
    fmov.s  fr2,@-r5 ! LS, dst->v = V
    fcmp/gt fr0,fr6  ! FE, T = Z > 0
    fmov.s  fr1,@-r5 ! LS, dst->u = U
    movt    r2       ! EX, tmp = T
    fmov.s  fr6,@-r5 ! LS, dst->z = Z
    fmov.s  fr5,@-r5 ! LS, dst->y = Y
    shll2   r2       ! EX, tmp = tmp << 2
    fmov.s  fr4,@-r5 ! LS, dst->x = X
    or      r2,r0    ! EX, CLIPFLAGS |= tmp (T << 2)
    mov.l   r10,@-r5 ! LS, dst->flags = PVR_CMD_VERTEX
.endm

.macro ProcessVertex4
    fmov.s  fr7,@-r5 ! LS, dst->w = W
    or      r11,r0   ! EX, CLIPFLAGS |= PVR_CMD_VERTEX_EOL
    fmov.s  fr3,@-r5 ! LS, dst->c = C
    fldi0   fr0      ! LS, fr0 = 0.0
    fmov.s  fr2,@-r5 ! LS, dst->v = V
    fcmp/gt fr0,fr6  ! FE, T = Z > 0
    fmov.s  fr1,@-r5 ! LS, dst->u = U
    movt    r2       ! EX, tmp = T
    fmov.s  fr6,@-r5 ! LS, dst->z = Z
    shll2   r2       ! EX, tmp = tmp << 2
    fmov.s  fr5,@-r5 ! LS, dst->y = Y
    add     r2,r2    ! EX, tmp = (tmp << 2) + (tmp << 2) (T << 3)
    fmov.s  fr4,@-r5 ! LS, dst->x = X
    or      r2,r0    ! EX, CLIPFLAGS |= tmp (T << 3)
    mov.l   r0,@-r5  ! LS, dst->flags = PVR_CMD_VERTEX_EOL | CLIPFLAGS
.endm


! =========================================================
! ====================== VIEWPORT TRANSFORM ===============
! =========================================================
!r2 = return addr
!r0 = temp
!r5 = dst pointer

!fr0  = temp
!fr4  = temp
!fr5  = temp
!fr5  = temp
!fr8  = VIEWPORT_HWIDTH
!fr9  = VIEWPORT_HHEIGHT
!fr10 = VIEWPORT_X_PLUS_HWIDTH
!fr11 = VIEWPORT_Y_PLUS_HHEIGHT

.macro ViewportTransformSetup vp_addr
    mov.l \vp_addr, r0 ! EX,  r0  = &vp
    fmov.s	@r0+,fr8   ! LS, fr8  = vp->HWIDTH
    fmov.s	@r0+,fr9   ! LS, fr9  = vp->HHEIGHT
    fmov.s	@r0+,fr10  ! LS, fr10 = vp->X_PLUS_HWIDTH
    fmov.s	@r0+,fr11  ! LS, fr11 = vp->Y_PLUS_HHEIGHT
    nop                ! MT (align to even instructions boundary)
.endm

.macro ViewportTransformVertex
! INVERSE W CALCULATION
    add #28, r5       ! EX, r5  = &vertex->w
    fmov.s  @r5,fr0   ! LS, fr0 = vertex->w
    fmul    fr0,fr0   ! FE, fr0 = fr0 * fr0
    add #-24, r5      ! EX, r5  = &vertex->x
    fsrra   fr0       ! FE, fr0 = 1 / sqrt(fr0) -> 1 / vertex->w

! TRANSFORM X
    fmov.s @r5,fr4    ! LS, fr4 = vertex->x
    fmov  fr10,fr5    ! LS, fr5 = VIEWPORT_X_PLUS_HWIDTH
    fmul  fr8,fr4     ! FE, fr4 = VIEWPORT_HWIDTH * vertex->x
    fmac  fr0,fr4,fr5 ! FE, fr5 = fr0 * fr4 + fr5 -- (X * F * hwidth) + x_plus_hwidth
    fmov.s fr5,@r5    ! LS, vertex->x = fr5
    add #4, r5        ! EX, r5  = &vertex->y

! TRANSFORM Y
    fmov.s @r5,fr4    ! LS, fr4 = vertex->y
    fmov  fr11,fr5    ! LS, fr5  = VIEWPORT_Y_PLUS_HHEIGHT
    fmul  fr9,fr4     ! FE, fr4  = VIEWPORT_HHEIGHT * vertex->y
    fmac  fr0,fr4,fr5 ! FE, fr5  = fr0 * fr4 + fr5 -- (Y * F * hheight) + y_plus_hheight
    fmov.s fr5,@r5    ! LS, vertex->y = fr5
    add #4, r5        ! EX, r5  = &vertex->z

! ASSIGN Z
    fmov.s fr0,@r5    ! LS, vertex->z = fr0
    add #20, r5       ! EX, r5 += 20 (points to start of next vertex)
.endm
