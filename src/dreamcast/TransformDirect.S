#include "AsmCommon.i"

#define VTX r2 // PVR_CMD_VERTEX
#define EOS r3 // PVR_CMD_VERTEX_EOL
#define SRC r4 // src pointer ARG
#define DST r5 // dst pointer ARG
#define CNT r6 // quads count ARG
#define NXT r7 // next vertex address

#define F_U  fr1 // vertex.u
#define F_V  fr2 // vertex.v
#define F_C  fr3 // vertex.colour
#define F_X  fr4 // vertex.x
#define F_Y  fr5 // vertex.y
#define F_Z  fr6 // vertex.z
#define F_W  fr7 // vertex.w

#define XYZW fv4 // vertex.xyzw


// =========================================================
// ========================= VERTEX LOADING ================
// =========================================================
.macro LoadTexturedVertex off
// LOAD XYZ
	mov		NXT,  SRC // MT, SRC = NXT
    fmov.s @SRC+, F_X // LS, X = src->x
    fmov.s @SRC+, F_Y // LS, Y = src->y
    fmov.s @SRC+, F_Z // LS, Z = src->z
    fldi1 F_W         // LS, W = 1.0
// PREPARE NEXT VERTEX
    add   #\off,  NXT // EX, nxt += offset to next vertex
    pref   @NXT       // LS, PREFETCH next vertex
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
   	add		#60,  DST // EX, skip Z
    fmov.s  F_C,@-DST // LS, dst->c = C
	fmul    F_W, F_W  // FE, W   = W * W
    fmov.s  F_V,@-DST // LS, dst->v = V
    fmov.s  F_U,@-DST // LS, dst->u = U
	fsrra   	 F_W  // FE, W   = 1/sqrt(W*W) ~ 1/W
	fmul    F_W, F_Y  // FE, Y   = Y * W
	fmul    F_W, F_X  // FE, X   = X * W
    fmov.s  F_W,@-DST // LS, dst->w = W
    fmov.s  F_Y,@-DST // LS, dst->y = Y
    fmov.s  F_X,@-DST // LS, dst->x = X
    mov.l  \flg,@-DST // LS, dst->flags = PVR_CMD
	pref      @DST    // LS, flush store queue
.endm

// =========================================================
// ==================== TEXTURED VERTEX TRANSFORM ==========
// =========================================================
BEG_FUNC(_DrawTexturedQuads_Direct)
    mov   SRC, NXT    // MT, nxt = src
	add   #24, NXT    // EX, nxt = first v1
    mov #0xE0, VTX    // EX, VTX = 0x00 00 00 E0
    pref  @NXT        // LS, PREFETCH pft (first vertex)
    shll16 VTX        // EX, VTX = 0x00 E0 00 00
    shll8  VTX        // EX, VTX = 0xE0 00 00 00 (PVR_CMD_VERTEX)
    mov #0xF0, EOS    // EX, EOS = 0x00 00 00 F0
    shll16 EOS        // EX, EOS = 0x00 F0 00 00
    shll8  EOS        // EX, EOS = 0xF0 00 00 00 (PVR_CMD_VERTEX_EOL)

.T_TRANSFORM_QUAD:
	// Triangle strip: {1,2,0} {2,0,3}
    LoadTexturedVertex 24 // v1 -> v2
    ProcessVertex VTX

    LoadTexturedVertex -48 // v2 -> v0
    ProcessVertex VTX

    LoadTexturedVertex  72 // v0 -> v3
    ProcessVertex VTX

    LoadTexturedVertex  48 // v3 -> next v1
    ProcessVertex EOS

    dt CNT               // count--; T = count == 0
    bf .T_TRANSFORM_QUAD // if !T then goto T_TRANSFORM_QUAD
    nop
    
	rts
	nop
END_FUNC(_DrawTexturedQuads_Direct)

