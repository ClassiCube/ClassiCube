#include <rsp_queue.inc>
#include <rdpq_macros.h>

#define GUARD_BAND_FACTOR 2

// 1 << VTX_SHIFT, keep in sync with gpu.c
#define ONE_W K32

#define xxxxXXXX h0
#define yyyyYYYY h1
#define zzzzZZZZ h2
#define wwwwWWWW h3

#define XYZ_CLIP_FLAGS 0x707 // Isolate -X/Y/Z and +X/Y/Z clipping flags


#define SCREEN_VTX_CS_POSi          0     // X, Y, Z, W (all 32-bit)
#define SCREEN_VTX_CS_POSf          8     // X, Y, Z, W (all 32-bit)
#define SCREEN_VTX_X               16
#define SCREEN_VTX_Y               18
#define SCREEN_VTX_Z               20
#define SCREEN_VTX_CLIP_CODE       22
#define SCREEN_VTX_PADDING         23
#define SCREEN_VTX_RGBA            24
#define SCREEN_VTX_S_T             28     // 28 S, 30 T
#define SCREEN_VTX_W               32     // FIXME: this is duplicated in CS_POS
#define SCREEN_VTX_INVW            36     // 32-bit
#define SCREEN_VTX_SIZE            40

#define V0_OFFSET 0 * SCREEN_VTX_SIZE
#define V1_OFFSET 1 * SCREEN_VTX_SIZE
#define V2_OFFSET 2 * SCREEN_VTX_SIZE
#define V3_OFFSET 3 * SCREEN_VTX_SIZE

.data

    RSPQ_BeginOverlayHeader
        RSPQ_DefineCommand GPUCmd_SetByte,       8   # 0x0
        RSPQ_DefineCommand GPUCmd_SetShort,      8   # 0x1
        RSPQ_DefineCommand GPUCmd_SetWord,       8   # 0x2
        RSPQ_DefineCommand GPUCmd_SetLong,       12  # 0x3

        RSPQ_DefineCommand GPUCmd_DrawQuad,      68  # 0x4
        RSPQ_DefineCommand GPUCmd_MatrixLoad,    68  # 0x5

        RSPQ_DefineCommand GPUCmd_PushRDP,       12  # 0x6
    RSPQ_EndOverlayHeader

    .align 4
BANNER0: .ascii " RSP OpenGL T&L "
BANNER1: .ascii "Rasky & Snacchus"

    RSPQ_BeginSavedState

    GPU_MATRIX_MVP:         .ds.b   128
GL_STATE:
    # This is the GL state that is updated by CPU via GPUCmd_Set commands
    GL_VIEWPORT_SCALE:      .half   0,0,0,0
    GL_VIEWPORT_OFFSET:     .half   0,0,0,0
    GL_STATE_TEX_SIZE:      .half   0,0
    GL_STATE_TEX_OFFSET:    .half   0,0
    GL_TRI_CMD:             .half   0
    GL_TRI_CULL:            .half   0

    RSPQ_EndSavedState

    .align 4
CLIP_CODE_FACTORS:      .half 1, 1, GUARD_BAND_FACTOR, GUARD_BAND_FACTOR
DRAW_TRI_RA:            .word 0

.bss
    .align 3

VERTEX_CACHE:   .dcb.b      SCREEN_VTX_SIZE * 4

.text

    .func GPUCmd_SetByte
GPUCmd_SetByte:
    jr ra
    sb a1, %lo(GL_STATE)(a0)
    .endfunc

    .func GPUCmd_SetShort
GPUCmd_SetShort:
    jr ra
    sh a1, %lo(GL_STATE)(a0)
    .endfunc

    .func GPUCmd_SetWord
GPUCmd_SetWord:
    jr ra
    sw a1, %lo(GL_STATE) + 0(a0)
    .endfunc

    .func GPUCmd_SetLong
GPUCmd_SetLong:
    sw a2, %lo(GL_STATE) + 4(a0)
    jr ra
    sw a1, %lo(GL_STATE) + 0(a0)
    .endfunc


    .func GPUCmd_PushRDP
GPUCmd_PushRDP:
	# RDP command is expected in a0 and a1
    move a0, a1
	move a1, a2

    jal_and_j RDPQ_Write8, RDPQ_Finalize
    .endfunc


    .func GPUCmd_MatrixLoad
GPUCmd_MatrixLoad:
    #define src         s6
    #define dst         s7

    #define vmat0_i  $v02
    #define vmat1_i  $v03
    #define vmat2_i  $v04
    #define vmat3_i  $v05
    #define vmat0_f  $v06
    #define vmat1_f  $v07
    #define vmat2_f  $v08
    #define vmat3_f  $v09

    addi src, rspq_dmem_buf_ptr, %lo(RSPQ_DMEM_BUFFER) - 64
    addi dst, zero, %lo(GPU_MATRIX_MVP)

    // Load the matrix from command parameters
    ldv vmat0_i, 0x00,src
    ldv vmat1_i, 0x08,src
    ldv vmat2_i, 0x10,src
    ldv vmat3_i, 0x18,src
    ldv vmat0_f, 0x20,src
    ldv vmat1_f, 0x28,src
    ldv vmat2_f, 0x30,src
    ldv vmat3_f, 0x38,src

	// Store the matrices, with each row stored twice
	// This is used by T&L to transform two vertices at once 
    sdv vmat0_i, 0x00,dst
    sdv vmat0_i, 0x08,dst
    sdv vmat1_i, 0x10,dst
    sdv vmat1_i, 0x18,dst
    sdv vmat2_i, 0x20,dst
    sdv vmat2_i, 0x28,dst
    sdv vmat3_i, 0x30,dst
    sdv vmat3_i, 0x38,dst
    sdv vmat0_f, 0x40,dst
    sdv vmat0_f, 0x48,dst
    sdv vmat1_f, 0x50,dst
    sdv vmat1_f, 0x58,dst
    sdv vmat2_f, 0x60,dst
    sdv vmat2_f, 0x68,dst
    sdv vmat3_f, 0x70,dst
    jr ra
    sdv vmat3_f, 0x78,dst

#undef src
#undef dst
    .endfunc

    ################################################################
    # GL_CalcScreenSpace
    #
    # Args:
    #   s3   = Destination vertex address
    #   $v02 = Clip space position (fractional part)
    #   $v03 = Clip space position (integer part)
    #
    ################################################################
    .func GL_CalcScreenSpace
GL_CalcScreenSpace:
    #define dst          s3
    #define vcspos_f     $v02
    #define vcspos_i     $v03
    #define vinvw_f      $v23
    #define vinvw_i      $v24
    #define vviewscale   $v25
    #define vviewoff     $v26
    #define vscreenpos_i $v27
    #define vscreenpos_f $v28
    #define v___         $v29
    #define w            e3

    # Calculate 32-bit inverse W
    # TODO: NR?
    vrcph vinvw_i.w, vcspos_i.w
    vrcpl vinvw_f.w, vcspos_f.w
    vrcph vinvw_i.w, vzero.e0

    # Calculate screenspace coords
    li t0, %lo(GL_VIEWPORT_SCALE)
    ldv vviewscale, 0,t0
    ldv vviewoff,   8,t0

    vmudl v___,         vcspos_f, vinvw_f.w
    vmadm v___,         vcspos_i, vinvw_f.w
    vmadn vscreenpos_f, vcspos_f, vinvw_i.w
    vmadh vscreenpos_i, vcspos_i, vinvw_i.w

    vmudn vscreenpos_f, vscreenpos_f, vviewscale
    vmadh vscreenpos_i, vscreenpos_i, vviewscale
    vadd vscreenpos_i, vviewoff

    sdv vscreenpos_i, SCREEN_VTX_X     ,dst
    ssv vcspos_i.w,   SCREEN_VTX_W+0   ,dst 
    ssv vcspos_f.w,   SCREEN_VTX_W+2   ,dst 
    ssv vinvw_i.w,    SCREEN_VTX_INVW+0,dst
    ssv vinvw_f.w,    SCREEN_VTX_INVW+2,dst
    jr ra
    sb zero,          SCREEN_VTX_PADDING(dst)

    #undef dst
    #undef vcspos_f
    #undef vcspos_i
    #undef vinvw_f
    #undef vinvw_i
    #undef vviewscale
    #undef vviewoff
    #undef vscreenpos_i
    #undef vscreenpos_f
    #undef v___
    #undef w

    .endfunc

    ################################################################
    # GL_CalcClipCodes
    #
    # Args:
    #   s3   = Destination vertex address
    #   $v02 = Clip space position (fractional part)
    #   $v03 = Clip space position (integer part)
    #
    ################################################################
    .func GL_CalcClipCodes
GL_CalcClipCodes:
    #define dst          s3
    #define vcspos_f     $v02
    #define vcspos_i     $v03
    #define vguard_f     $v27
    #define vguard_i     $v28
    #define v___         $v29
    #define w            e3

    li t0, %lo(CLIP_CODE_FACTORS)
    ldv vguard_i,  0,t0

    vmudn vguard_f, vcspos_f, vguard_i
    vmadh vguard_i, vcspos_i, vguard_i
    
    vch v___, vguard_i, vguard_i.w
    vcl v___, vguard_f, vguard_f.w

    cfc2 t0, COP2_CTRL_VCC
    andi t2, t0, 0x707 // Isolate X/Y/Z clipping flags
    srl  t1, t2, 5     // Shift hi flags to be aligned next to lo flags
    andi t2, t2, 0x7   // Isolate lo clip flags
    or   t2, t1        // Merge clip flags (compressed to 6 bits)
    jr ra
    sb t2,  SCREEN_VTX_CLIP_CODE(dst)

    #undef dst
    #undef vcspos_i
    #undef vcspos_f
    #undef vguard_i
    #undef vguard_f
    #undef v___
    #undef w

    .endfunc

    ################################################################
    # GL_TnL
    #
    # Args:
    #   s3 = address of the vertex in DMEM (usually within VERTEX_CACHE)
    #
    ################################################################
    .func GL_TnL
GL_TnL:
    #define vtx          s3

    #define v___         $v01
    #define vcspos_f     $v02
    #define vcspos_i     $v03
    #define vtexsize     $v06
    #define vtexoffset   $v07
    #define vst          $v08
    #define vst_i        $v28
    #define vst_f        $v29
    move ra2, ra

    llv vst, SCREEN_VTX_S_T, vtx  # S + T

    li t0, %lo(GL_STATE_TEX_SIZE)
    llv vtexsize,   0,t0
    llv vtexoffset, 4,t0

    # Scale texcoord by texsize and subtract offset (to correct for bilinear sampling if active)
    #vmudn v___,  vst, vtexsize
    # vmadh vst, vtexoffset, K1

    #vmudn v___,  vst, vtexsize
    #vmudl vst,   vst, vtexsize

	vmudn vst_f, vst,   vtexsize # ACC  = vst * vtexsize, VST_F = ACC & 0xFFFF
    #####vmadn vst_f,   vtexoffset, K1
	vmadh vst_i, vzero, vzero    # ACC += zero * zero,    VST_I = ACC >> 16

	// Shift texture coords right 5 bits
	vmudm v___,  vst_i, K2048    # ACC  = (vst_i << 11)
	vmadl vst,   vst_f, K2048    # ACC += (vst_f << 11) >> 16, VST = ACC & 0xFFFF

    #undef vst_i
    #undef vst_f

    slv vst,   SCREEN_VTX_S_T, vtx

    ldv vcspos_f, SCREEN_VTX_CS_POSf,vtx
    jal GL_CalcScreenSpace
    ldv vcspos_i, SCREEN_VTX_CS_POSi,vtx

    j GL_CalcClipCodes
    move ra, ra2

    #undef vcspos_f
    #undef vcspos_i
    #undef vtexsize
    #undef vtexoffset

    #undef vtx

    #undef v___
    #undef vrgba
    #undef vst
    #undef s

    .endfunc


    .align 3
    .func GPUCmd_DrawQuad
GPUCmd_DrawQuad:
    #define vtx         a0
    #define mtx_ptr     s0
    #define src_ptr     s4

    #define v___        $v01

    #define vmtx0_i     $v16       //  m00 m01 m02 m03
    #define vmtx0_f     $v17
    #define vmtx1_i     $v18       //  m10 m11 m12 m13
    #define vmtx1_f     $v19
    #define vmtx2_i     $v20       //  m20 m21 m22 m23
    #define vmtx2_f     $v21
    #define vmtx3_i     $v22       //  m30 m31 m32 m03
    #define vmtx3_f     $v23

    #define vpos        $v24
    #define vcol        $v25
    #define vtex        $v26
    #define vcspos_i    $v28
    #define vcspos_f    $v29

	#define tmp         t0
	#define v0_cflags   t1
	#define v1_cflags   t2
	#define v2_cflags   t3
	#define v3_cflags   t4

    addi src_ptr, rspq_dmem_buf_ptr, %lo(RSPQ_DMEM_BUFFER) - 64
    li vtx, %lo(VERTEX_CACHE)

    li mtx_ptr, %lo(GPU_MATRIX_MVP)
    lqv vmtx0_i, 0x00,mtx_ptr // [m00.I m01.I m02.I m03.I  m00.I m01.I m02.I m03.I]
    lqv vmtx1_i, 0x10,mtx_ptr // etc
    lqv vmtx2_i, 0x20,mtx_ptr
    lqv vmtx3_i, 0x30,mtx_ptr
    lqv vmtx0_f, 0x40,mtx_ptr
    lqv vmtx1_f, 0x50,mtx_ptr
    lqv vmtx2_f, 0x60,mtx_ptr
    lqv vmtx3_f, 0x70,mtx_ptr

// ########################
// Vertex 0 and 1 transform
// ########################
	ldv vpos.e0,  0, src_ptr // Load v0 X, Y, Z
	ldv vpos.e4, 16, src_ptr // Load v1 X, Y, Z

	// matrix multiply
    vmudn v___,      vmtx0_f, vpos.xxxxXXXX
    vmadh v___,      vmtx0_i, vpos.xxxxXXXX
    vmadn v___,      vmtx1_f, vpos.yyyyYYYY
    vmadh v___,      vmtx1_i, vpos.yyyyYYYY
    vmadn v___,      vmtx2_f, vpos.zzzzZZZZ
    vmadh v___,      vmtx2_i, vpos.zzzzZZZZ
    vmadn v___,      vmtx3_f, ONE_W
    vmadh vcspos_i,  vmtx3_i, ONE_W
    vmadn vcspos_f,  vzero,   vzero

	llv vcol.e0,  8, src_ptr // Load v0 RGBA
	llv vtex.e0, 12, src_ptr // Load v0 U, V
	llv vcol.e2, 24, src_ptr // Load v1 RGBA
	llv vtex.e2, 28, src_ptr // Load v1 U, V

    # 32-bit right shift by 5, to keep the clip space coordinates unscaled
    vmudm vcspos_i, vcspos_i, K2048
    vmadl vcspos_f, vcspos_f, K2048

    slv vcol.e0, SCREEN_VTX_RGBA + V0_OFFSET, vtx
    slv vtex.e0, SCREEN_VTX_S_T  + V0_OFFSET, vtx
    slv vcol.e2, SCREEN_VTX_RGBA + V1_OFFSET, vtx
    slv vtex.e2, SCREEN_VTX_S_T  + V1_OFFSET, vtx

    # Calculate and store clipping flags against CS.W.
    # These will be used for trivial rejections.
    vch v___, vcspos_i, vcspos_i.wwwwWWWW
    vcl v___, vcspos_f, vcspos_f.wwwwWWWW

    cfc2 tmp, COP2_CTRL_VCC
    sdv vcspos_i.e0, SCREEN_VTX_CS_POSi + V0_OFFSET, vtx
    sdv vcspos_f.e0, SCREEN_VTX_CS_POSf + V0_OFFSET, vtx
    sdv vcspos_i.e4, SCREEN_VTX_CS_POSi + V1_OFFSET, vtx
    sdv vcspos_f.e4, SCREEN_VTX_CS_POSf + V1_OFFSET, vtx

// ########################
// Vertex 2 and 3 transform
// ########################
	ldv vpos.e0, 32, src_ptr // Load v2 X, Y, Z
	ldv vpos.e4, 48, src_ptr // Load v3 X, Y, Z

    andi v0_cflags, tmp, XYZ_CLIP_FLAGS
	srl  tmp, tmp, 4
    andi v1_cflags, tmp, XYZ_CLIP_FLAGS

	# matrix multiply
    vmudn v___,      vmtx0_f, vpos.xxxxXXXX
    vmadh v___,      vmtx0_i, vpos.xxxxXXXX
    vmadn v___,      vmtx1_f, vpos.yyyyYYYY
    vmadh v___,      vmtx1_i, vpos.yyyyYYYY
    vmadn v___,      vmtx2_f, vpos.zzzzZZZZ
    vmadh v___,      vmtx2_i, vpos.zzzzZZZZ
    vmadn v___,      vmtx3_f, ONE_W
    vmadh vcspos_i,  vmtx3_i, ONE_W
    vmadn vcspos_f,  vzero,   vzero

	llv vcol.e4, 40, src_ptr # Load v2 RGBA
	llv vtex.e4, 44, src_ptr # Load v2 U, V
	llv vcol.e6, 56, src_ptr # Load v3 RGBA
	llv vtex.e6, 60, src_ptr # Load v3 U, V

    # 32-bit right shift by 5, to keep the clip space coordinates unscaled
    vmudm vcspos_i, vcspos_i, K2048
    vmadl vcspos_f, vcspos_f, K2048

    slv vcol.e4, SCREEN_VTX_RGBA + V2_OFFSET, vtx
    slv vtex.e4, SCREEN_VTX_S_T  + V2_OFFSET, vtx
    slv vcol.e6, SCREEN_VTX_RGBA + V3_OFFSET, vtx
    slv vtex.e6, SCREEN_VTX_S_T  + V3_OFFSET, vtx

    # Calculate and store clipping flags against CS.W.
    # These will be used for trivial rejections.
    vch v___, vcspos_i, vcspos_i.wwwwWWWW
    vcl v___, vcspos_f, vcspos_f.wwwwWWWW

    cfc2 tmp, COP2_CTRL_VCC
    sdv vcspos_i.e0, SCREEN_VTX_CS_POSi + V2_OFFSET, vtx
    sdv vcspos_f.e0, SCREEN_VTX_CS_POSf + V2_OFFSET, vtx
    sdv vcspos_i.e4, SCREEN_VTX_CS_POSi + V3_OFFSET, vtx
    sdv vcspos_f.e4, SCREEN_VTX_CS_POSf + V3_OFFSET, vtx

    andi v2_cflags, tmp, XYZ_CLIP_FLAGS
	srl  tmp, tmp, 4
    andi v3_cflags, tmp, XYZ_CLIP_FLAGS
    #undef src_ptr
    #undef vtx
    #undef v___     

    #undef vmtx0_i   
    #undef vmtx0_f  
    #undef vmtx1_i   
    #undef vmtx1_f  
    #undef vmtx2_i   
    #undef vmtx2_f  
    #undef vmtx3_i   
    #undef vmtx3_f  

    #undef vpos      
    #undef vcspos_i  
    #undef vcspos_f  

    #define vtx1 a1
    #define vtx2 a2
    #define vtx3 a3
	#define vtx4 a0

// ########################
// Trivial rejection check
// ########################
	// If for any plane, all 4 vertices are outside the plane,
	//  then the quad is out of the viewport and can be trivially rejected
    and tmp, v0_cflags, v1_cflags
    and tmp, v2_cflags
    and tmp, v3_cflags
    bnez tmp, JrRa
    nop

// ########################
// Perform rest of T&L
// ########################
    jal GL_TnL
    li s3, %lo(VERTEX_CACHE) + V0_OFFSET
    jal GL_TnL
    li s3, %lo(VERTEX_CACHE) + V1_OFFSET
    jal GL_TnL
    li s3, %lo(VERTEX_CACHE) + V2_OFFSET
    jal GL_TnL
    li s3, %lo(VERTEX_CACHE) + V3_OFFSET

// ########################
// Guardband check
// ########################
	// Check if all vertices fit within guardband
    lbu v0_cflags, (%lo(VERTEX_CACHE) + V0_OFFSET + SCREEN_VTX_CLIP_CODE)(zero)
    lbu v1_cflags, (%lo(VERTEX_CACHE) + V1_OFFSET + SCREEN_VTX_CLIP_CODE)(zero)
    lbu v2_cflags, (%lo(VERTEX_CACHE) + V2_OFFSET + SCREEN_VTX_CLIP_CODE)(zero)
    lbu v3_cflags, (%lo(VERTEX_CACHE) + V3_OFFSET + SCREEN_VTX_CLIP_CODE)(zero)

    or tmp, v0_cflags, v1_cflags
    or tmp, v2_cflags
    or tmp, v3_cflags

    beqz tmp, DrawQuadForRDP
	nop

	#undef tmp
	#undef v0_cflags
	#undef v1_cflags
	#undef v2_cflags
	#undef v3_cflags

// ########################
// Clipped triangle path
// ########################
	// If not, go with slow clipping path
	
	# now do the actual drawing
	li a1, %lo(VERTEX_CACHE) + V0_OFFSET
	li a2, %lo(VERTEX_CACHE) + V1_OFFSET
	jal DrawClippedTriangle
	li a3, %lo(VERTEX_CACHE) + V2_OFFSET

	li a1, %lo(VERTEX_CACHE) + V0_OFFSET
	li a2, %lo(VERTEX_CACHE) + V2_OFFSET
	jal DrawClippedTriangle
	li a3, %lo(VERTEX_CACHE) + V3_OFFSET

    j RSPQ_Loop
    nop
.endfunc

    ################################################################
    # DrawClippedTriangle - Breaks a triangle into one or more clipped tris
    ################################################################
.func DrawClippedTriangle
DrawClippedTriangle:
    sw ra, %lo(DRAW_TRI_RA) // TODO find a register for this

    lbu t0, SCREEN_VTX_CLIP_CODE(vtx1)
    lbu t1, SCREEN_VTX_CLIP_CODE(vtx2)
    lbu t2, SCREEN_VTX_CLIP_CODE(vtx3)
    or t5, t0, t1
    or t5, t2

    move s1, zero
    jal GL_ClipTriangle
    move s2, zero

    beqz v1, gl_draw_triangle_end
    addi s2, -6
    lhu s5, 0(s1)
gl_draw_clipped_triangles_loop:
    move vtx1, s5
    lhu vtx2, 2(s1)
    lhu vtx3, 4(s1)

gl_draw_single_triangle:
    addi vtx1, SCREEN_VTX_X
    addi vtx2, SCREEN_VTX_X
    addi vtx3, SCREEN_VTX_X
    
    lhu a0, %lo(GL_TRI_CMD)
    lh  v0, %lo(GL_TRI_CULL)
    jal RDPQ_Triangle
    li s3, %lo(RDPQ_CMD_STAGING)

    jal RDPQ_Send
    li s4, %lo(RDPQ_CMD_STAGING)

    blt s1, s2, gl_draw_clipped_triangles_loop
    addi s1, 2

gl_draw_triangle_end:
	lw ra, %lo(DRAW_TRI_RA)
    jr ra
    nop
.endfunc

    ################################################################
    # DrawQuadForRDP - Draws two triangles for a quad
    ################################################################
.func DrawQuadForRDP
DrawQuadForRDP:
    lhu a0, %lo(GL_TRI_CMD)
    lh  v0, %lo(GL_TRI_CULL)
	li  a1, %lo(VERTEX_CACHE) + V0_OFFSET + SCREEN_VTX_X
	li  a2, %lo(VERTEX_CACHE) + V1_OFFSET + SCREEN_VTX_X
	li  a3, %lo(VERTEX_CACHE) + V2_OFFSET + SCREEN_VTX_X
    jal RDPQ_Triangle
    li  s3, %lo(RDPQ_CMD_STAGING)

    jal RDPQ_Send
    li s4, %lo(RDPQ_CMD_STAGING)

    lhu a0, %lo(GL_TRI_CMD)
    lh  v0, %lo(GL_TRI_CULL)
	li  a1, %lo(VERTEX_CACHE) + V2_OFFSET + SCREEN_VTX_X
	li  a2, %lo(VERTEX_CACHE) + V3_OFFSET + SCREEN_VTX_X
	li  a3, %lo(VERTEX_CACHE) + V0_OFFSET + SCREEN_VTX_X
    jal RDPQ_Triangle
    li  s3, %lo(RDPQ_CMD_STAGING)

    jal RDPQ_Send
    li s4, %lo(RDPQ_CMD_STAGING)

	j RSPQ_Loop
	nop
.endfunc

    #undef vtx1
    #undef vtx2
    #undef vtx3

#include "rsp_gpu_clipping.inc"
#include <rsp_rdpq.inc>
