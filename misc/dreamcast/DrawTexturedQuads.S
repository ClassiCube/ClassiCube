!r0 = clip flags
!r1 = GPU command
!r2 = temp
!r3 = prefetch address
!r4 = src pointer ARG
!r5 = dst pointer ARG
!r6 = quads count ARG

!fr4  = x
!fr5  = y
!fr6  = z
!fr7  = w
!fr8  = c
!fr9  = u
!fr10 = v
!fr11 = saved fr0 (for viewport transform)

!fv4  = XYZW

#include "ViewportTransform.S"
.global _DrawTexturedQuads
.align 4
.type  _DrawTexturedQuads,%function

_DrawTexturedQuads:
! Setup
    mov r4,r3     ! r3  = src
    add #-32, r5  ! r5 -= sizeof(VERTEX)
    fmov fr0,fr11 ! fr11 = saved fr0
    nop           ! loop align

.TRANSFORM_QUAD:
    mov.l CMD_TEX_VERT, r1 ! r1  = GPU VERT command

! TRANSFORM VERTEX 1
    add #24, r3     ! r3 += VERTEX_STRIDE
    add #64, r5     ! r5 += 2 * sizeof(VERTEX)
    pref @r3        ! PREFETCH r3 (next vertex)
    fmov @r4+, fr4  ! X = src->x
    fmov @r4+, fr5  ! Y = src->y
    fmov @r4+, fr6  ! Z = src->z
    fldi1 fr7       ! W = 1.0
    ftrv xmtrx, fv4 ! TRANSFORM(XYZW)
! OUTPUT + CLIPFLAGS VERTEX 1
    fmov    @r4+,fr8 ! C = src->color
    fmov    @r4+,fr9 ! U = src->u
    fmov    @r4+,fr10! V = src->v
    ProcessVertex1

! TRANSFORM VERTEX 2
    add #24, r3     ! r3 += VERTEX_STRIDE
    add #64, r5     ! r5 += 2 * sizeof(VERTEX)
    pref @r3        ! PREFETCH r3 (next vertex)
    fmov @r4+, fr4  ! X = src->x
    fmov @r4+, fr5  ! Y = src->y
    fmov @r4+, fr6  ! Z = src->z
    fldi1 fr7       ! W = 1.0
    ftrv xmtrx, fv4 ! TRANSFORM(XYZW)
! OUTPUT + CLIPFLAGS VERTEX 2
    fmov    @r4+,fr8 ! C = src->color
    fmov    @r4+,fr9 ! U = src->u
    fmov    @r4+,fr10! V = src->v
    ProcessVertex2

! TRANSFORM VERTEX 3
    add #24, r3     ! r3 += VERTEX_STRIDE
    add #64, r5     ! r5 += 2 * sizeof(VERTEX)
    pref @r3        ! PREFETCH r3 (next vertex)
    fmov @r4+, fr4  ! X = src->x
    fmov @r4+, fr5  ! Y = src->y
    fmov @r4+, fr6  ! Z = src->z
    fldi1 fr7       ! W = 1.0
    ftrv xmtrx, fv4 ! TRANSFORM(XYZW)
! OUTPUT + CLIPFLAGS VERTEX 3
    fmov    @r4+,fr8 ! C = src->color
    fmov    @r4+,fr9 ! U = src->u
    fmov    @r4+,fr10! V = src->v
    ProcessVertex3

! TRANSFORM VERTEX 4
    add #24, r3     ! r3 += VERTEX_STRIDE
    add #64, r5     ! r5 += 2 * sizeof(VERTEX)
    pref @r3        ! PREFETCH r3 (next vertex)
    fmov @r4+, fr4  ! X = src->x
    fmov @r4+, fr5  ! Y = src->y
    fmov @r4+, fr6  ! Z = src->z
    fldi1 fr7       ! W = 1.0
    ftrv xmtrx, fv4 ! TRANSFORM(XYZW)
! OUTPUT + CLIPFLAGS VERTEX 4
    fmov    @r4+,fr8 ! C = src->color
    fmov    @r4+,fr9 ! U = src->u
    fmov    @r4+,fr10! V = src->v
    ProcessVertex4 CMD_TEX_EOS

! CLIPFLAGS TESTING
    cmp/eq  #0,r0  ! T = r0 == 0 (all points invisible)
    bt/s    .NO_POINTS_VISIBLE  ! if T goto NO_POINTS_VISIBLE
    nop
    bra .SOME_POINTS_VISIBLE
    nop

.NO_POINTS_VISIBLE:
    bra .LOOP_END ! jump to loop end after executing instruction in delay slot
    add #-128, r5 ! r5 -= 4 * sizeof(VERTEX), move back to prior quad, so that this invisible quad gets overwritten in next iteration

.SOME_POINTS_VISIBLE:

.LOOP_END:
    dt r6 ! r6--; T = r6 == 0
    bf .TRANSFORM_QUAD ! if !T then goto TRANSFORM_QUAD
    nop
    
    add #32, r5     ! r5 += sizeof(VERTEX)
    mov r5,r0       ! r0 = r5
    rts             ! return after executing instruction in delay slot
    fmov fr11,fr0   ! fr0 = original fr0

.align 2
CMD_TEX_VERT: .long 0xe0000000
CMD_TEX_EOS:  .long 0xf0000000

.global _VP_TEX_HWIDTH
.type   _VP_TEX_HWIDTH,%object
.size   _VP_TEX_HWIDTH,4
_VP_TEX_HWIDTH:  .long 0

.global _VP_TEX_HHEIGHT
.type   _VP_TEX_HHEIGHT,%object
.size   _VP_TEX_HHEIGHT,4
_VP_TEX_HHEIGHT: .long 0

.global _VP_TEX_X_PLUS_HWIDTH
.type   _VP_TEX_X_PLUS_HWIDTH,%object
.size   _VP_TEX_X_PLUS_HWIDTH,4
_VP_TEX_X_PLUS_HWIDTH:  .long 0

.global _VP_TEX_Y_PLUS_HHEIGHT
.type   _VP_TEX_Y_PLUS_HHEIGHT,%object
.size   _VP_TEX_Y_PLUS_HHEIGHT,4
_VP_TEX_Y_PLUS_HHEIGHT: .long 0