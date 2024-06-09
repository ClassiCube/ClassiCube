#include "ViewportTransform.S"
.global _DrawTexturedQuads
.align 4
.type  _DrawTexturedQuads,%function

_DrawTexturedQuads:
! Setup
    mov r4,r3     ! r3  = src
    add #-32, r5  ! r5 -= sizeof(VERTEX)
    ViewportTransformSetup _VP_TEX_HWIDTH

.TRANSFORM_QUAD:
    mov.l CMD_TEX_VERT, r1 ! r1  = GPU VERT command

    LoadTexturedVertex
    ProcessVertex1

    LoadTexturedVertex
    ProcessVertex2

    LoadTexturedVertex
    ProcessVertex3

    LoadTexturedVertex
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
    rts             ! return after executing instruction in delay slot
    mov r5,r0       ! r0 = r5

.align 2
CMD_TEX_VERT: .long 0xe0000000
CMD_TEX_EOS:  .long 0xf0000000

.global _VP_TEX_HWIDTH
.type   _VP_TEX_HWIDTH,%object
_VP_TEX_HWIDTH:  .long 0

.global _VP_TEX_HHEIGHT
.type   _VP_TEX_HHEIGHT,%object
_VP_TEX_HHEIGHT: .long 0

.global _VP_TEX_X_PLUS_HWIDTH
.type   _VP_TEX_X_PLUS_HWIDTH,%object
_VP_TEX_X_PLUS_HWIDTH:  .long 0

.global _VP_TEX_Y_PLUS_HHEIGHT
.type   _VP_TEX_Y_PLUS_HHEIGHT,%object
_VP_TEX_Y_PLUS_HHEIGHT: .long 0
