!r2 = return addr
!r0 = temp
!r5 = dst pointer

!fr4  = VIEWPORT_HWIDTH
!fr5  = VIEWPORT_HHEIGHT
!fr6  = VIEWPORT_X_PLUS_HWIDTH
!fr7  = VIEWPORT_Y_PLUS_HHEIGHT
!fr8  = ???
!fr9  = temp
!fr10 = temp
!fr11 = saved fr0

.macro ViewportTransformSetup viewport_addr
    mova \viewport_addr, r0
    fmov.s	@r0+,fr4 ! fr4 = VIEWPORT_HWIDTH
    fmov.s	@r0+,fr5 ! fr5 = VIEWPORT_HHEIGHT
    fmov.s	@r0+,fr6 ! fr6 = VIEWPORT_X_PLUS_HWIDTH
    fmov.s	@r0+,fr7 ! fr7 = VIEWPORT_Y_PLUS_HHEIGHT
.endm

.macro ViewportTransformVertex
! INVERSE W CALCULATION
    add #28, r5       ! r5  = &vertex[0].w
    fmov.s  @r5+,fr0  ! fr0 = vertex->w
    fmul    fr0,fr0   ! fr0 = fr0 * fr0
    add #-32, r5      ! r5  -= sizeof(VERTEX)
    fsrra   fr0       ! fr0 = 1 / sqrt(fr0) -> 1 / vertex->w

! TRANSFORM X
    fmov.s @r5,fr10   ! fr10 = vertex->x
    fmov   fr6,fr9    ! fr9  = VIEWPORT_X_PLUS_HWIDTH
    fmul fr4,fr10     ! fr10 = VIEWPORT_HWIDTH * vertex->x
    fmac fr0,fr10,fr9 ! fr9  = fr0 * fr10 + fr9 -- (X * F * hwidth) + x_plus_hwidth
    fmov.s fr9,@r5    ! vertex->x = fr9
    add #4, r5        ! r5 += 4 (points to vertex->y)

! TRANSFORM Y
    fmov.s @r5,fr10   ! fr10 = vertex->y
    fmov   fr7,fr9    ! fr9  = VIEWPORT_Y_PLUS_HHEIGHT
    fmul fr5,fr10     ! fr10 = VIEWPORT_HHEIGHT * vertex->y
    fmac fr0,fr10,fr9 ! fr9  = fr0 * fr10 + fr9 -- (Y * F * hheight) + y_plus_hheight
    fmov.s fr9,@r5    ! vertex->y = fr9
    add #4, r5        ! r5 += 4 (points to vertex->z)

! ASSIGN Z
    fmov.s @r5,fr0    ! vertex->z = fr0
    add #24, r5       ! r5 += 24 (points to next vertex)
.endm