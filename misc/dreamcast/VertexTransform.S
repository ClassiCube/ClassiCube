#define FLG r0 // clip flags
#define TMP r1 // temp
#define VTX r2 // PVR_CMD_VERTEX
#define EOS r3 // PVR_CMD_VERTEX_EOL
#define SRC r4 // src pointer ARG
#define DST r5 // dst pointer ARG
#define CNT r6 // quads count ARG
#define PFT r7 // prefetch address

#define ZERO fr0 // 0.0
#define F_U  fr1 // vertex.u
#define F_V  fr2 // vertex.v
#define F_C  fr3 // vertex.colour
#define F_X  fr4 // vertex.x
#define F_Y  fr5 // vertex.y
#define F_Z  fr6 // vertex.z
#define F_W  fr7 // vertex.w

#define XYZW fv4 // vertex.xyzw


! =========================================================
! ========================= TRANSFORM SETUP ===============
! =========================================================
.macro TransformSetup
    mov   SRC, PFT    ! MT, pft = src
    add  #-32, DST    ! EX, dst -= sizeof(VERTEX)
    mov #0xE0, VTX    ! EX, VTX = 0x00 00 00 E0
    pref  @PFT        ! LS, PREFETCH pft (first vertex)
    shll16 VTX        ! EX, VTX = 0x00 E0 00 00
    shll8  VTX        ! EX, VTX = 0xE0 00 00 00 (PVR_CMD_VERTEX)
    mov #0xF0, EOS    ! EX, EOS = 0x00 00 00 F0
    shll16 EOS        ! EX, EOS = 0x00 F0 00 00
    shll8  EOS        ! EX, EOS = 0xF0 00 00 00 (PVR_CMD_VERTEX_EOL)
    fldi0  ZERO       ! LS, fr0 = 0.0
.endm

.macro TransformEnd
    add #32, DST      ! EX, DST += sizeof(VERTEX)
    rts               ! CO, return after executing instruction in delay slot
    mov DST, r0       ! MT, r0 = DST
.endm


! =========================================================
! ========================= VERTEX LOADING ================
! =========================================================
.macro LoadColouredVertex
! LOAD XYZ
    fmov @SRC+, F_X   ! LS, X = src->x
    fmov @SRC+, F_Y   ! LS, Y = src->y
    fmov @SRC+, F_Z   ! LS, Z = src->z
    fldi1 F_W         ! LS, W = 1.0
! PREPARE NEXT VERTEX
    add    #16, PFT   ! EX, pft += VERTEX_STRIDE
    pref   @PFT       ! LS, PREFETCH pft (next vertex)
    add    #64, DST   ! EX, dst += 2 * sizeof(VERTEX)
! TRANSFORM VERTEX
    ftrv xmtrx, XYZW  ! FE, TRANSFORM(XYZW)
! LOAD ATTRIBUTES
    fmov @SRC+, F_C   ! LS, C = src->color
.endm

.macro LoadTexturedVertex
! LOAD XYZ
    fmov @SRC+, F_X   ! LS, X = src->x
    fmov @SRC+, F_Y   ! LS, Y = src->y
    fmov @SRC+, F_Z   ! LS, Z = src->z
    fldi1 F_W         ! LS, W = 1.0
! PREPARE NEXT VERTEX
    add    #24, PFT   ! EX, pft += VERTEX_STRIDE
    pref   @PFT       ! LS, PREFETCH pft (next vertex)
    add    #64, DST   ! EX, dst += 2 * sizeof(VERTEX)
! TRANSFORM VERTEX
    ftrv xmtrx, XYZW  ! FE, TRANSFORM(XYZW)
! LOAD ATTRIBUTES
    fmov @SRC+, F_C   ! LS, C = src->color
    fmov @SRC+, F_U   ! LS, U = src->u
    fmov @SRC+, F_V   ! LS, V = src->v
.endm

! =========================================================
! ========================= VERTEX OUTPUT =================
! =========================================================
! To take advantage of SH4 dual instruction processing, 
!  clipflag calculation and vertex output are interleaved
.macro ProcessVertex1
    fmov.s  F_Z,@-DST ! LS, dst->z = Z
    fmov.s  F_C,@-DST ! LS, dst->c = C
    fmov.s  F_V,@-DST ! LS, dst->v = V
    fcmp/gt ZERO, F_Z ! FE, T = Z > 0
    fmov.s  F_U,@-DST ! LS, dst->u = U
    movt    FLG       ! EX, CLIPFLAGS = T
    fmov.s  F_W,@-DST ! LS, dst->w = W
    fmov.s  F_Y,@-DST ! LS, dst->y = Y
    fmov.s  F_X,@-DST ! LS, dst->x = X
    mov.l   VTX,@-DST ! LS, dst->flags = PVR_CMD_VERTEX
.endm

.macro ProcessVertex2
    fmov.s  F_Z,@-DST ! LS, dst->z = Z
    fmov.s  F_C,@-DST ! LS, dst->c = C
    fmov.s  F_V,@-DST ! LS, dst->v = V
    fcmp/gt ZERO,F_Z  ! FE, T = Z > 0
    fmov.s  F_U,@-DST ! LS, dst->u = U
    movt    TMP       ! EX, tmp = T
    fmov.s  F_W,@-DST ! LS, dst->w = W
    add     TMP,TMP   ! EX, tmp = tmp + tmp
    fmov.s  F_Y,@-DST ! LS, dst->y = Y
    or      TMP,FLG   ! EX, CLIPFLAGS |= tmp (T << 1)
    fmov.s  F_X,@-DST ! LS, dst->x = X
    mov.l   VTX,@-DST ! LS, dst->flags = PVR_CMD_VERTEX
.endm

.macro ProcessVertex3
    fmov.s  F_Z,@-DST ! LS, dst->z = Z
    fmov.s  F_C,@-DST ! LS, dst->c = C
    fmov.s  F_V,@-DST ! LS, dst->v = V
    fcmp/gt ZERO, F_Z ! FE, T = Z > 0
    fmov.s  F_U,@-DST ! LS, dst->u = U
    movt    TMP       ! EX, tmp = T
    fmov.s  F_W,@-DST ! LS, dst->w = W
    fmov.s  F_Y,@-DST ! LS, dst->y = Y
    shll2   TMP       ! EX, tmp = tmp << 2
    fmov.s  F_X,@-DST ! LS, dst->x = X
    or      TMP,FLG   ! EX, CLIPFLAGS |= tmp (T << 2)
    mov.l   VTX,@-DST ! LS, dst->flags = PVR_CMD_VERTEX
.endm

.macro ProcessVertex4
    fmov.s  F_Z,@-DST ! LS, dst->z = Z
    or      EOS,FLG   ! EX, CLIPFLAGS |= PVR_CMD_VERTEX_EOL
    fmov.s  F_C,@-DST ! LS, dst->c = C
    fmov.s  F_V,@-DST ! LS, dst->v = V
    fcmp/gt ZERO, F_Z ! FE, T = Z > 0
    fmov.s  F_U,@-DST ! LS, dst->u = U
    movt    TMP       ! EX, tmp = T
    fmov.s  F_W,@-DST ! LS, dst->w = W
    shll2   TMP       ! EX, tmp = tmp << 2
    fmov.s  F_Y,@-DST ! LS, dst->y = Y
    add     TMP,TMP   ! EX, tmp = (tmp << 2) + (tmp << 2) (T << 3)
    fmov.s  F_X,@-DST ! LS, dst->x = X
    or      TMP,FLG   ! EX, CLIPFLAGS |= tmp (T << 3)
    mov.l   FLG,@-DST ! LS, dst->flags = PVR_CMD_VERTEX_EOL | CLIPFLAGS
.endm


! =========================================================
! ==================== TEXTURED VERTEX TRANSFORM ==========
! =========================================================
.global _DrawTexturedQuads
.align 4

_DrawTexturedQuads:
! Setup
    TransformSetup

.T_TRANSFORM_QUAD:
    LoadTexturedVertex
    ProcessVertex1

    LoadTexturedVertex
    ProcessVertex2

    LoadTexturedVertex
    ProcessVertex3

    LoadTexturedVertex
    ProcessVertex4

! CLIPFLAGS TESTING
    and     #15,FLG
    cmp/eq   #0,FLG      ! T = CLIPFLAGS == 0 (all points invisible)
    bf/s    .T_LOOP_END  ! if !T goto LOOP_END
    nop

! No points visible case
    add #-128, DST       ! DST -= 4 * sizeof(VERTEX), move back to prior quad, so that this invisible quad gets overwritten in next iteration

.T_LOOP_END:
    dt CNT               ! count--; T = count == 0
    bf .T_TRANSFORM_QUAD ! if !T then goto T_TRANSFORM_QUAD
    nop
    
    TransformEnd
.size _DrawTexturedQuads, .-_DrawTexturedQuads
.type _DrawTexturedQuads, %function

! =========================================================
! ==================== COLOURED VERTEX TRANSFORM ==========
! =========================================================
.global _DrawColouredQuads
.align 4

_DrawColouredQuads:
! Setup
    fldi0 F_U     ! U = 0
    fldi0 F_V     ! V = 0
    TransformSetup

.C_TRANSFORM_QUAD:
    LoadColouredVertex
    ProcessVertex1

    LoadColouredVertex
    ProcessVertex2

    LoadColouredVertex
    ProcessVertex3

    LoadColouredVertex
    ProcessVertex4

! CLIPFLAGS TESTING
    and     #15,FLG
    cmp/eq   #0,FLG      ! T = CLIPFLAGS == 0 (all points invisible)
    bf/s    .C_LOOP_END  ! if !T goto LOOP_END
    nop

! No points visible case
    add #-128, DST       ! dst -= 4 * sizeof(VERTEX), move back to 1 vertex before start of quad

.C_LOOP_END:
    dt CNT               ! count--; T = count == 0
    bf .C_TRANSFORM_QUAD ! if !T then goto TRANSFORM_QUAD
    nop
    
    TransformEnd
.size _DrawColouredQuads, .-_DrawColouredQuads
.type _DrawColouredQuads, %function
