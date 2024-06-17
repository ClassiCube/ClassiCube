#include "ViewportTransform.S"
.global _DrawColouredQuads
.align 4
.type  _DrawColouredQuads,%function

_DrawColouredQuads:
! Setup
    fldi0 fr1     ! U = 0
    fldi0 fr2     ! V = 0
    TransformSetup
    ViewportTransformSetup _VP_COL_HWIDTH

.TRANSFORM_QUAD:
    LoadColouredVertex
    ProcessVertex1

    LoadColouredVertex
    ProcessVertex2

    LoadColouredVertex
    ProcessVertex3

    LoadColouredVertex
    ProcessVertex4

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
    
    TransformEnd

.align 4

.global _VP_COL_HWIDTH
_VP_COL_HWIDTH:  .long 0

.global _VP_COL_HHEIGHT
_VP_COL_HHEIGHT: .long 0

.global _VP_COL_X_PLUS_HWIDTH
_VP_COL_X_PLUS_HWIDTH:  .long 0

.global _VP_COL_Y_PLUS_HHEIGHT
_VP_COL_Y_PLUS_HHEIGHT: .long 0
