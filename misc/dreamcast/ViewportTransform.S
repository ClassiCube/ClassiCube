! =========================================================
! ========================= VERTEX LOADING ================
! =========================================================
.macro LoadColouredVertex
! PREPARE NEXT VERTEX
    add #16, r3     ! r3 += VERTEX_STRIDE
    pref @r3        ! PREFETCH r3 (next vertex)
    add #64, r5     ! r5 += 2 * sizeof(VERTEX)
! LOAD XYZ
    fmov @r4+, fr4  ! X = src->x
    fmov @r4+, fr5  ! Y = src->y
    fmov @r4+, fr6  ! Z = src->z
    fldi1 fr7       ! W = 1.0
! TRANSFORM VERTEX
    ftrv xmtrx, fv4 ! TRANSFORM(XYZW)
! LOAD ATTRIBUTES
    fmov   @r4+,fr3 ! C = src->color
.endm

.macro LoadTexturedVertex
! PREPARE NEXT VERTEX
    add #24, r3     ! r3 += VERTEX_STRIDE
    pref @r3        ! PREFETCH r3 (next vertex)
    add #64, r5     ! r5 += 2 * sizeof(VERTEX)
! LOAD XYZ
    fmov @r4+, fr4  ! X = src->x
    fmov @r4+, fr5  ! Y = src->y
    fmov @r4+, fr6  ! Z = src->z
    fldi1 fr7       ! W = 1.0
! TRANSFORM VERTEX
    ftrv xmtrx, fv4 ! TRANSFORM(XYZW)
! LOAD ATTRIBUTES
    fmov    @r4+,fr3 ! C = src->color
    fmov    @r4+,fr1 ! U = src->u
    fmov    @r4+,fr2 ! V = src->v
.endm

! =========================================================
! ========================= VERTEX OUTPUT =================
! =========================================================
! To take advantage of SH4 dual instruction processing, interleave
!  the clipflag calculation and vertex output instructions
.macro ProcessVertex1
    fmov.s  fr7,@-r5 ! dst->w = W
    fmov.s  fr3,@-r5 ! dst->c = C
    fneg    fr7      ! W = -W
    fmov.s  fr2,@-r5 ! dst->v = V
    fcmp/gt fr7,fr6  ! T = Z > W (i.e. Z > -W)
    fmov.s  fr1,@-r5 ! dst->u = U
    movt    r0       ! CLIPFLAGS = T
    fmov.s  fr6,@-r5 ! dst->z = Z
    fmov.s  fr5,@-r5 ! dst->y = Y
    fmov.s  fr4,@-r5 ! dst->x = X
    mov.l   r1,@-r5  ! dst->flags = CMD_VERT
.endm

.macro ProcessVertex2
    fmov.s  fr7,@-r5 ! dst->w = W
    fmov.s  fr3,@-r5 ! dst->c = C
    fneg    fr7      ! W = -W
    fmov.s  fr2,@-r5 ! dst->v = V
    fcmp/gt fr7,fr6  ! T = Z > W (i.e. Z > -W)
    fmov.s  fr1,@-r5 ! dst->u = U
    movt    r2       ! tmp = T
    fmov.s  fr6,@-r5 ! dst->z = Z
    add     r2,r2    ! tmp = tmp + tmp
    fmov.s  fr5,@-r5 ! dst->y = Y
    or      r2,r0    ! CLIPFLAGS |= tmp (T << 1)
    fmov.s  fr4,@-r5 ! dst->x = X
    mov.l   r1,@-r5  ! dst->flags = CMD_VERT
.endm

.macro ProcessVertex3
    fmov.s  fr7,@-r5 ! dst->w = W
    fmov.s  fr3,@-r5 ! dst->c = C
    fneg    fr7      ! W = -W
    fmov.s  fr2,@-r5 ! dst->v = V
    fcmp/gt fr7,fr6  ! T = Z > W (i.e. Z > -W)
    fmov.s  fr1,@-r5 ! dst->u = U
    movt    r2       ! tmp = T
    fmov.s  fr6,@-r5 ! dst->z = Z
    fmov.s  fr5,@-r5 ! dst->y = Y
    shll2   r2       ! tmp = tmp << 2
    fmov.s  fr4,@-r5 ! dst->x = X
    or      r2,r0    ! CLIPFLAGS |= tmp (T << 2)
    mov.l   r1,@-r5  ! dst->flags = CMD_VERT
.endm

.macro ProcessVertex4 eos_addr
    fmov.s  fr7,@-r5 ! dst->w = W
    fmov.s  fr3,@-r5 ! dst->c = C
    fneg    fr7      ! W = -W
    fmov.s  fr2,@-r5 ! dst->v = V
    fcmp/gt fr7,fr6  ! T = Z > W (i.e. Z > -W)
    fmov.s  fr1,@-r5 ! dst->u = U
    movt    r2       ! tmp = T
    fmov.s  fr6,@-r5 ! dst->z = Z
    shll2   r2       ! tmp = tmp << 2
    fmov.s  fr5,@-r5 ! dst->y = Y
    add     r2,r2    ! tmp = (tmp << 2) + (tmp << 2)
    fmov.s  fr4,@-r5 ! dst->x = X
    mov.l \eos_addr, r1 ! r1  = GPU EOS command
    or      r2,r0    ! CLIPFLAGS |= tmp (T << 3)
    or      r0,r1    ! r1 |= CLIPFLAGS
    mov.l   r1,@-r5  ! dst->flags = GPU EOS | CLIPFLAGS
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

.macro ViewportTransformSetup viewport_addr
    mova \viewport_addr, r0
    fmov.s	@r0+,fr8  ! fr8  = VIEWPORT_HWIDTH
    fmov.s	@r0+,fr9  ! fr9  = VIEWPORT_HHEIGHT
    fmov.s	@r0+,fr10 ! fr10 = VIEWPORT_X_PLUS_HWIDTH
    fmov.s	@r0+,fr11 ! fr11 = VIEWPORT_Y_PLUS_HHEIGHT
    nop               ! align to even instructions
.endm

.macro ViewportTransformVertex
! INVERSE W CALCULATION
    add #28, r5       ! r5  = &vertex->w
    fmov.s  @r5,fr0   ! fr0 = vertex->w
    fmul    fr0,fr0   ! fr0 = fr0 * fr0
    add #-24, r5      ! r5  = &vertex->x
    fsrra   fr0       ! fr0 = 1 / sqrt(fr0) -> 1 / vertex->w

! TRANSFORM X
    fmov.s @r5,fr4    ! fr4 = vertex->x
    fmov  fr10,fr5    ! fr5 = VIEWPORT_X_PLUS_HWIDTH
    fmul  fr8,fr4     ! fr4 = VIEWPORT_HWIDTH * vertex->x
    fmac  fr0,fr4,fr5 ! fr5 = fr0 * fr4 + fr5 -- (X * F * hwidth) + x_plus_hwidth
    fmov.s fr5,@r5    ! vertex->x = fr5
    add #4, r5        ! r5  = &vertex->y

! TRANSFORM Y
    fmov.s @r5,fr4    ! fr4 = vertex->y
    fmov  fr11,fr5    ! fr5  = VIEWPORT_Y_PLUS_HHEIGHT
    fmul  fr9,fr4     ! fr4  = VIEWPORT_HHEIGHT * vertex->y
    fmac  fr0,fr4,fr5 ! fr5  = fr0 * fr4 + fr5 -- (Y * F * hheight) + y_plus_hheight
    fmov.s fr5,@r5    ! vertex->y = fr5
    add #4, r5        ! r5  = &vertex->z

! ASSIGN Z
    fmov.s fr0,@r5    ! vertex->z = fr0
    add #20, r5       ! r5 += 20 (points to start of next vertex)
.endm