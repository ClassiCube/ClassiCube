#include "AsmCommon.i"

#define VTX r2 // PVR_CMD_VERTEX
#define EOS r3 // PVR_CMD_VERTEX_EOL
#define SRC r4 // src pointer ARG
#define DST r5 // dst pointer ARG
#define CNT r6 // quads count ARG
#define PFT r7 // prefetch address

#define F_U  fr1 // vertex.u
#define F_V  fr2 // vertex.v
#define F_C  fr3 // vertex.colour
#define F_X  fr4 // vertex.x
#define F_Y  fr5 // vertex.y
#define F_Z  fr6 // vertex.z
#define F_W  fr7 // vertex.w

#define XYZW fv4 // vertex.xyzw


// =========================================================
// ========================= TRANSFORM SETUP ===============
// =========================================================
.macro TransformSetup
    mov   SRC, PFT    // MT, pft = src
    add  #-32, DST    // EX, dst -= sizeof(VERTEX)
    mov #0xE0, VTX    // EX, VTX = 0x00 00 00 E0
    pref  @PFT        // LS, PREFETCH pft (first vertex)
    shll16 VTX        // EX, VTX = 0x00 E0 00 00
    shll8  VTX        // EX, VTX = 0xE0 00 00 00 (PVR_CMD_VERTEX)
    mov #0xF0, EOS    // EX, EOS = 0x00 00 00 F0
    shll16 EOS        // EX, EOS = 0x00 F0 00 00
    shll8  EOS        // EX, EOS = 0xF0 00 00 00 (PVR_CMD_VERTEX_EOL)
	add	#0x0F, EOS    // EX, EOS = 0xF0 00 00 0F (PVR_CMD_VERTEX_EOL | ALL_CLIPFLAGS)
.endm

.macro TransformEnd
    add #32, DST      // EX, DST += sizeof(VERTEX)
    rts               // CO, return after executing instruction in delay slot
    mov DST, r0       // MT, r0 = DST
.endm


// =========================================================
// ========================= VERTEX LOADING ================
// =========================================================
.macro LoadColouredVertex
// LOAD XYZ
    fmov.s @SRC+, F_X // LS, X = src->x
    fmov.s @SRC+, F_Y // LS, Y = src->y
    fmov.s @SRC+, F_Z // LS, Z = src->z
    fldi1 F_W         // LS, W = 1.0
// PREPARE NEXT VERTEX
    add    #16, PFT   // EX, pft += VERTEX_STRIDE
    pref   @PFT       // LS, PREFETCH pft (next vertex)
    add    #64, DST   // EX, dst += 2 * sizeof(VERTEX)
// TRANSFORM VERTEX
    ftrv xmtrx, XYZW  // FE, TRANSFORM(XYZW)
// LOAD ATTRIBUTES
    fmov.s @SRC+, F_C // LS, C = src->color
.endm

.macro LoadTexturedVertex
// LOAD XYZ
    fmov.s @SRC+, F_X // LS, X = src->x
    fmov.s @SRC+, F_Y // LS, Y = src->y
    fmov.s @SRC+, F_Z // LS, Z = src->z
    fldi1 F_W         // LS, W = 1.0
// PREPARE NEXT VERTEX
    add    #24, PFT   // EX, pft += VERTEX_STRIDE
    pref   @PFT       // LS, PREFETCH pft (next vertex)
    add    #64, DST   // EX, dst += 2 * sizeof(VERTEX)
// TRANSFORM VERTEX
    ftrv xmtrx, XYZW  // FE, TRANSFORM(XYZW)
// LOAD ATTRIBUTES
    fmov.s @SRC+, F_C // LS, C = src->color
    fmov.s @SRC+, F_U // LS, U = src->u
    fmov.s @SRC+, F_V // LS, V = src->v
.endm

// =========================================================
// ========================= VERTEX OUTPUT =================
// =========================================================
.macro ProcessVertex flg
   	add		#-4,  DST // EX, skip Z
    fmov.s  F_C,@-DST // LS, dst->c = C
    fmov.s  F_V,@-DST // LS, dst->v = V
    fmov.s  F_U,@-DST // LS, dst->u = U
    fmov.s  F_W,@-DST // LS, dst->w = W
    fmov.s  F_Y,@-DST // LS, dst->y = Y
    fmov.s  F_X,@-DST // LS, dst->x = X
    mov.l  \flg,@-DST // LS, dst->flags = PVR_CMD
.endm

// =========================================================
// ==================== TEXTURED VERTEX TRANSFORM ==========
// =========================================================
BEG_FUNC(_DrawTexturedQuads_Fast)
    TransformSetup

.T_TRANSFORM_QUAD:
    LoadTexturedVertex
    ProcessVertex VTX

    LoadTexturedVertex
    ProcessVertex VTX

    LoadTexturedVertex
    ProcessVertex VTX

    LoadTexturedVertex
    ProcessVertex EOS

    dt CNT               // count--; T = count == 0
    bf .T_TRANSFORM_QUAD // if !T then goto T_TRANSFORM_QUAD
    nop
    
    TransformEnd
END_FUNC(_DrawTexturedQuads_Fast)

// =========================================================
// ==================== COLOURED VERTEX TRANSFORM ==========
// =========================================================
BEG_FUNC(_DrawColouredQuads_Fast)
// Setup
    fldi0 F_U     // U = 0
    fldi0 F_V     // V = 0
    TransformSetup

.C_TRANSFORM_QUAD:
    LoadColouredVertex
    ProcessVertex VTX

    LoadColouredVertex
    ProcessVertex VTX

    LoadColouredVertex
    ProcessVertex VTX

    LoadColouredVertex
    ProcessVertex EOS

    dt CNT               // count--; T = count == 0
    bf .C_TRANSFORM_QUAD // if !T then goto TRANSFORM_QUAD
    nop
    
    TransformEnd
END_FUNC(_DrawColouredQuads_Fast)
