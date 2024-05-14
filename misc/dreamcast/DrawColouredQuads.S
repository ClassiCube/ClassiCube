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
!fr9  = u (0.0)
!fr10 = v (0.0)
!fr11 = saved fr0 (for viewport transform)

!fv4  = XYZW

#include "ViewportTransform.S"
.global _DrawColouredQuads
.align 4
.type  _DrawColouredQuads,%function

_DrawColouredQuads:
! Setup
    fldi0 fr9     ! U = 0
    fldi0 fr10    ! V = 0
    mov r4,r3     ! r3  = src
    add #-32, r5  ! r5 -= sizeof(VERTEX)
    fmov fr0,fr11 ! fr11 = saved fr0
    nop           ! loop align

.TRANSFORM_QUAD:
    mov.l CMD_COL_VERT, r1 ! r1  = GPU VERT command

! TRANSFORM VERTEX 1
    add #16, r3     ! r3 += VERTEX_STRIDE
    add #64, r5     ! r5 += 2 * sizeof(VERTEX)
    pref @r3        ! PREFETCH r3 (next vertex)
    fmov @r4+, fr4  ! fr4 = src->x
    fmov @r4+, fr5  ! fr5 = src->y
    fmov @r4+, fr6  ! fr6 = src->z
    fldi1 fr7       ! fr7 = 1.0
    ftrv xmtrx, fv4 ! TRANSFORM(fr4..fr7)

    fmov   @r4+,fr8 ! C = src->color
    ProcessVertex1

! TRANSFORM VERTEX 2
    add #16, r3     ! r3 += VERTEX_STRIDE
    add #64, r5     ! r5 += 2 * sizeof(VERTEX)
    pref @r3        ! PREFETCH r3 (next vertex)
    fmov @r4+, fr4  ! fr4 = src->x
    fmov @r4+, fr5  ! fr5 = src->y
    fmov @r4+, fr6  ! fr6 = src->z
    fldi1 fr7       ! fr7 = 1.0
    ftrv xmtrx, fv4 ! TRANSFORM(fr4..fr7)

    fmov   @r4+,fr8 ! C = src->color
    ProcessVertex2

! TRANSFORM VERTEX 3
    add #16, r3     ! r3 += VERTEX_STRIDE
    add #64, r5     ! r5 += 2 * sizeof(VERTEX)
    pref @r3        ! PREFETCH r3 (next vertex)
    fmov @r4+, fr4  ! fr4 = src->x
    fmov @r4+, fr5  ! fr5 = src->y
    fmov @r4+, fr6  ! fr6 = src->z
    fldi1 fr7       ! fr7 = 1.0
    ftrv xmtrx, fv4 ! TRANSFORM(fr4..fr7)

    fmov   @r4+,fr8 ! C = src->color
    ProcessVertex3

! TRANSFORM VERTEX 4
    add #16, r3     ! r3 += VERTEX_STRIDE
    add #64, r5     ! r5 += 2 * sizeof(VERTEX)
    pref @r3        ! PREFETCH r3 (next vertex)
    fmov @r4+, fr4  ! fr4 = src->x
    fmov @r4+, fr5  ! fr5 = src->y
    fmov @r4+, fr6  ! fr6 = src->z
    fldi1 fr7       ! fr7 = 1.0
    ftrv xmtrx, fv4 ! TRANSFORM(fr4..fr7)

    fmov    @r4+,fr8 ! C = src->color
    ProcessVertex4 CMD_COL_EOS

! CLIPFLAGS TESTING
    cmp/eq   #0,r0 ! T = r0 == 0 (all points invisible)
    bt/s    .NO_POINTS_VISIBLE  ! if T goto NO_POINTS_VISIBLE
    nop
    bra .SOME_POINTS_VISIBLE
    nop

.NO_POINTS_VISIBLE:
    bra .LOOP_END ! jump to loop end after executing instruction in delay slot
    add #-128, r5 ! r5 -= 4 * sizeof(VERTEX), move back to 1 vertex before start of quad

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
CMD_COL_VERT: .long 0xe0000000
CMD_COL_EOS:  .long 0xf0000000

.global _VP_COL_HWIDTH
.type   _VP_COL_HWIDTH,%object
.size   _VP_COL_HWIDTH,4
_VP_COL_HWIDTH:  .long 0

.global _VP_COL_HHEIGHT
.type   _VP_COL_HHEIGHT,%object
.size   _VP_COL_HHEIGHT,4
_VP_COL_HHEIGHT: .long 0

.global _VP_COL_X_PLUS_HWIDTH
.type   _VP_COL_X_PLUS_HWIDTH,%object
.size   _VP_COL_X_PLUS_HWIDTH,4
_VP_COL_X_PLUS_HWIDTH:  .long 0

.global _VP_COL_Y_PLUS_HHEIGHT
.type   _VP_COL_Y_PLUS_HHEIGHT,%object
.size   _VP_COL_Y_PLUS_HHEIGHT,4
_VP_COL_Y_PLUS_HHEIGHT: .long 0