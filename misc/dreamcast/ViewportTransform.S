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
    add #28, r5       ! r5  = &vertex[0].w
    fmov.s  @r5+,fr0  ! fr0 = vertex->w
    fmul    fr0,fr0   ! fr0 = fr0 * fr0
    add #-32, r5      ! r5  -= sizeof(VERTEX)
    fsrra   fr0       ! fr0 = 1 / sqrt(fr0) -> 1 / vertex->w

! TRANSFORM X
    fmov.s @r5,fr4    ! fr4 = vertex->x
    fmov  fr10,fr5    ! fr5 = VIEWPORT_X_PLUS_HWIDTH
    fmul  fr8,fr4     ! fr4 = VIEWPORT_HWIDTH * vertex->x
    fmac  fr0,fr4,fr5 ! fr5 = fr0 * fr4 + fr5 -- (X * F * hwidth) + x_plus_hwidth
    fmov.s fr9,@r5    ! vertex->x = fr9
    add #4, r5        ! r5 += 4 (points to vertex->y)

! TRANSFORM Y
    fmov.s @r5,fr3    ! fr4 = vertex->y
    fmov  fr11,fr5    ! fr4  = VIEWPORT_Y_PLUS_HHEIGHT
    fmul  fr9,fr4     ! fr4  = VIEWPORT_HHEIGHT * vertex->y
    fmac  fr0,fr4,fr5 ! fr5  = fr0 * fr4 + fr5 -- (Y * F * hheight) + y_plus_hheight
    fmov.s fr9,@r5    ! vertex->y = fr9
    add #4, r5        ! r5 += 4 (points to vertex->z)

! ASSIGN Z
!   fmov.s @r5,fr0    ! vertex->z = fr0
!   add #24, r5       ! r5 += 24 (points to next vertex)
.endm


VP_ZSHIFT:
        .float 1.0001