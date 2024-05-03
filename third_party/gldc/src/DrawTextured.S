!r0 = prefetch address
!r1 = GPU VERT command
!r2 = GPU EOS command
!r3 = colour
!r4 = src pointer ARG
!r5 = dst pointer ARG
!r6 = quads count ARG

!fr4  = x
!fr5  = y
!fr6  = z
!fr7  = w
!fr9  = ?
!fr10 = u
!fr11 = v

!fv4  = XYZW

.globl DrawTexturedQuads
.align 4
.type  DrawTexturedQuads,%function

DrawTexturedQuads:
.fnstart
! Setup
    mov.l CMD_TEX_VERT, r1 ! r1  = GPU VERT command
    mov.l CMD_TEX_EOS,  r2 ! r2  = GPU EOS  command
    mov r4,r0              ! r0  = src
    add #-32, r5           ! r5 -= sizeof(VERTEX)

.TRANSFORM_QUAD:
! TRANSFORM VERTEX 1
    add #24, r0     ! r0 += VERTEX_STRIDE
    add #64, r5     ! r5 += 2 * sizeof(VERTEX)
    pref @r0        ! PREFETCH r0 (next vertex)
    fmov @r4+, fr4  ! fr4 = src->x
    fmov @r4+, fr5  ! fr5 = src->y
    fmov @r4+, fr6  ! fr6 = src->z
    fldi1 fr7       ! fr7 = 1.0
    ftrv xmtrx, fv4 ! TRANSFORM(fr4..fr7)

    mov.l   @r4+,r3  ! r3 = src->color
    fmov    @r4+,fr10! fr10 = src->u
    fmov    @r4+,fr11! fr11 = src->v
    fmov.s  fr7,@-r5 ! dst->w = fr7
    mov.l    r3,@-r5 ! dst->bgra = r3
    fmov.s fr11,@-r5 ! dst->v = fr11
    fmov.s fr10,@-r5 ! dst->u = fr10
    fmov.s  fr6,@-r5 ! dst->z = fr6
    fmov.s  fr5,@-r5 ! dst->y = fr5
    fmov.s  fr4,@-r5 ! dst->x = fr4
    mov.l    r1,@-r5 ! dst->flags = CMD_VERT

! TRANSFORM VERTEX 2
    add #24, r0     ! r0 += VERTEX_STRIDE
    add #64, r5     ! r5 += 2 * sizeof(VERTEX)
    pref @r0        ! PREFETCH r0 (next vertex)
    fmov @r4+, fr4  ! fr4 = src->x
    fmov @r4+, fr5  ! fr5 = src->y
    fmov @r4+, fr6  ! fr6 = src->z
    fldi1 fr7       ! fr7 = 1.0
    ftrv xmtrx, fv4 ! TRANSFORM(fr4..fr7)

    mov.l   @r4+,r3  ! r3 = src->color
    fmov    @r4+,fr10! fr10 = src->u
    fmov    @r4+,fr11! fr11 = src->v
    fmov.s  fr7,@-r5 ! dst->w = fr7
    mov.l    r3,@-r5 ! dst->bgra = r3
    fmov.s fr11,@-r5 ! dst->v = fr11
    fmov.s fr10,@-r5 ! dst->u = fr10
    fmov.s  fr6,@-r5 ! dst->z = fr6
    fmov.s  fr5,@-r5 ! dst->y = fr5
    fmov.s  fr4,@-r5 ! dst->x = fr4
    mov.l    r1,@-r5 ! dst->flags = CMD_VERT

! TRANSFORM VERTEX 3
    add #24, r0     ! r0 += VERTEX_STRIDE
    add #64, r5     ! r5 += 2 * sizeof(VERTEX)
    pref @r0        ! PREFETCH r0 (next vertex)
    fmov @r4+, fr4  ! fr4 = src->x
    fmov @r4+, fr5  ! fr5 = src->y
    fmov @r4+, fr6  ! fr6 = src->z
    fldi1 fr7       ! fr7 = 1.0
    ftrv xmtrx, fv4 ! TRANSFORM(fr4..fr7)

    mov.l   @r4+,r3  ! r3 = src->color
    fmov    @r4+,fr10! fr10 = src->u
    fmov    @r4+,fr11! fr11 = src->v
    fmov.s  fr7,@-r5 ! dst->w = fr7
    mov.l    r3,@-r5 ! dst->bgra = r3
    fmov.s fr11,@-r5 ! dst->v = fr11
    fmov.s fr10,@-r5 ! dst->u = fr10
    fmov.s  fr6,@-r5 ! dst->z = fr6
    fmov.s  fr5,@-r5 ! dst->y = fr5
    fmov.s  fr4,@-r5 ! dst->x = fr4
    mov.l    r1,@-r5 ! dst->flags = CMD_VERT

! TRANSFORM VERTEX 4
    add #24, r0     ! r0 += VERTEX_STRIDE
    add #64, r5     ! r5 += 2 * sizeof(VERTEX)
    pref @r0        ! PREFETCH r0 (next vertex)
    fmov @r4+, fr4  ! fr4 = src->x
    fmov @r4+, fr5  ! fr5 = src->y
    fmov @r4+, fr6  ! fr6 = src->z
    fldi1 fr7       ! fr7 = 1.0
    ftrv xmtrx, fv4 ! TRANSFORM(fr4..fr7)

    mov.l   @r4+,r3  ! r3 = src->color
    fmov    @r4+,fr10! fr10 = src->u
    fmov    @r4+,fr11! fr11 = src->v
    fmov.s  fr7,@-r5 ! dst->w = fr7
    mov.l    r3,@-r5 ! dst->bgra = r3
    fmov.s fr11,@-r5 ! dst->v = fr11
    fmov.s fr10,@-r5 ! dst->u = fr10
    fmov.s  fr6,@-r5 ! dst->z = fr6
    fmov.s  fr5,@-r5 ! dst->y = fr5
    fmov.s  fr4,@-r5 ! dst->x = fr4
    mov.l    r2,@-r5 ! dst->flags = CMD_EOS

    dt r6 ! r6--; T = 1 if r6 == 0, else 0
    bf .TRANSFORM_QUAD ! if T == 0 then goto TRANSFORM_QUAD
    nop
    
    rts ! return
    nop
.fnend

.align 2
CMD_TEX_VERT: .long 0xe0000000
CMD_TEX_EOS:  .long 0xf0000000