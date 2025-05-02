#include <rsp_queue.inc>
#include <rdpq_macros.h>
#define MATRIX_SIZE           64
#define GUARD_BAND_FACTOR 2

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

GL_STATE:
    # This is the GL state that is also used by the pipeline.
    GL_MATRIX_MVP:          .ds.b   MATRIX_SIZE
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

	.bss
    .align 3
#define VERTEX_CACHE_SIZE     4
//0-39 same as screenvtx
#define PRIM_VTX_TRCODE            40    // trivial-reject clipping flags (against -w/+w)
#define PRIM_VTX_SIZE              42

VERTEX_CACHE:   .dcb.b      PRIM_VTX_SIZE * VERTEX_CACHE_SIZE

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

    #define vrhs01_i     $v02
    #define vrhs01_f     $v03
    #define vrhs23_i     $v04
    #define vrhs23_f     $v05

    addi src, rspq_dmem_buf_ptr, %lo(RSPQ_DMEM_BUFFER) - 64
    addi dst, zero, %lo(GL_MATRIX_MVP)

    # Load the matrix from command parameters (misaligned)
    lqv vrhs01_i, 0x00,src
    lrv vrhs01_i, 0x10,src
    lqv vrhs23_i, 0x10,src
    lrv vrhs23_i, 0x20,src
    lqv vrhs01_f, 0x20,src
    lrv vrhs01_f, 0x30,src
    lqv vrhs23_f, 0x30,src
    lrv vrhs23_f, 0x40,src

    sqv vrhs01_i, 0x00,dst
    sqv vrhs23_i, 0x10,dst
    sqv vrhs01_f, 0x20,dst
    jr ra
    sqv vrhs23_f, 0x30,dst

#undef src
#undef dst
    .endfunc

    .align 3
    .func GPUCmd_DrawQuad
GPUCmd_DrawQuad:
    #define vtx         a0
    #define mtx_ptr     s0
    #define src_ptr     s4
	#define vcount      s3

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

    #define x  e0
    #define y  e1
    #define z  e2
    #define w  e3

    addi src_ptr, rspq_dmem_buf_ptr, %lo(RSPQ_DMEM_BUFFER) - 64
    li vtx, %lo(VERTEX_CACHE)
	li vcount, 4

    li mtx_ptr, %lo(GL_MATRIX_MVP)
    ldv vmtx0_i.e0,  0x00,mtx_ptr
    ldv vmtx1_i.e0,  0x08,mtx_ptr
    ldv vmtx2_i.e0,  0x10,mtx_ptr
    ldv vmtx3_i.e0,  0x18,mtx_ptr
    ldv vmtx0_f.e0,  0x20,mtx_ptr
    ldv vmtx1_f.e0,  0x28,mtx_ptr
    ldv vmtx2_f.e0,  0x30,mtx_ptr
    ldv vmtx3_f.e0,  0x38,mtx_ptr

upload_vertex:
	ldv vpos,  0, src_ptr # Load X, Y, Z, W
	llv vcol,  8, src_ptr # Load RGBA
	llv vtex, 12, src_ptr # Load U, V

	# matrix multiply
    vmudn v___,      vmtx0_f, vpos.h0
    vmadh v___,      vmtx0_i, vpos.h0
    vmadn v___,      vmtx1_f, vpos.h1
    vmadh v___,      vmtx1_i, vpos.h1
    vmadn v___,      vmtx2_f, vpos.h2
    vmadh v___,      vmtx2_i, vpos.h2
    vmadn v___,      vmtx3_f, vpos.h3
    vmadh vcspos_i,  vmtx3_i, vpos.h3
    vmadn vcspos_f,  vzero,   vzero

    slv vcol, SCREEN_VTX_RGBA, vtx
    slv vtex, SCREEN_VTX_S_T,  vtx

    # 32-bit right shift by 5, to keep the clip space coordinates unscaled
    vmudm vcspos_i, vcspos_i, vshift8.e4
    vmadl vcspos_f, vcspos_f, vshift8.e4

	addi vcount,  -1
	addi src_ptr, 16

    sdv vcspos_i, SCREEN_VTX_CS_POSi,vtx
    sdv vcspos_f, SCREEN_VTX_CS_POSf,vtx

    # Calculate and store clipping flags against CS.W.
    # These will be used for trivial rejections.
    vch v___, vcspos_i, vcspos_i.w
    vcl v___, vcspos_f, vcspos_f.w
    cfc2 t0, COP2_CTRL_VCC
    andi t0, 0x707   # Isolate X/Y/Z flags

    # Compress flags to 8 bit
    srl t1, t0, 5
    andi t0, 0x7
    or t0, t1
    sb t0, PRIM_VTX_TRCODE(vtx)

	bnez vcount, upload_vertex
	addi vtx, PRIM_VTX_SIZE

	
	# now do the actual drawing
	li a1, %lo(VERTEX_CACHE) + 0*PRIM_VTX_SIZE
	li a2, %lo(VERTEX_CACHE) + 1*PRIM_VTX_SIZE
	jal GPUCmd_DrawTriangle
	li a3, %lo(VERTEX_CACHE) + 2*PRIM_VTX_SIZE

	li a1, %lo(VERTEX_CACHE) + 0*PRIM_VTX_SIZE
	li a2, %lo(VERTEX_CACHE) + 2*PRIM_VTX_SIZE
	jal GPUCmd_DrawTriangle
	li a3, %lo(VERTEX_CACHE) + 3*PRIM_VTX_SIZE

    j RSPQ_Loop
    nop
    #undef src_ptr
    #undef vtx

    #undef x
    #undef y
    #undef z
    #undef w

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
    andi t0, 0x707
    srl t1, t0, 5
    andi t0, 0x7
    or t0, t1
    jr ra
    sb t0,  SCREEN_VTX_CLIP_CODE(dst)

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
    #vmadh vst,   vtexoffset, K1
    #vmudl vst,   vst, vtexsize

	vmudh v___,  vst, vtexsize
	vsar  vst_i, COP2_ACC_HI
	vsar  vst_f, COP2_ACC_MD

	vmudl vst_f, vst_f, K8192
	vmadm vst_i, vst_i, K8192
	vmadn vst,   vzero, vzero

    #undef vst_i
    #undef vst_f

    lbu t0,    PRIM_VTX_TRCODE(vtx)
    slv vst,   SCREEN_VTX_S_T, vtx

    ldv vcspos_f, SCREEN_VTX_CS_POSf,vtx
    ldv vcspos_i, SCREEN_VTX_CS_POSi,vtx

    # Mark this vertex as having T&L applied
    ori t0, 0x80

    jal GL_CalcScreenSpace
    sb t0, PRIM_VTX_TRCODE(vtx)

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


    .func GPUCmd_DrawTriangle
GPUCmd_DrawTriangle:
    #define vtx1        a1
    #define vtx2        a2
    #define vtx3        a3
    #define trcode1     t6
    #define trcode2     t7
    #define trcode3     t8
    sw ra, %lo(DRAW_TRI_RA) # TODO find a register for this

    # Trivial reject: if all the vertices are out of the same plane (at least one),
    # the triangle is out of the viewport.
    # NOTE: This deliberately uses lb instead of lbu so the sign bit is extended.
    #       The MSB of each TR-code is a bit flag that is set if the vertex has already
    #       had T&L applied once.
    lb trcode1, PRIM_VTX_TRCODE(vtx1)
    lb trcode2, PRIM_VTX_TRCODE(vtx2)
    lb trcode3, PRIM_VTX_TRCODE(vtx3)
    and t0, trcode1, trcode2
    and t0, trcode3
    andi t0, 0x3F
    bnez t0, JrRa
    nop

    # Perform T&L for each vertex if we haven't already
    bgezal trcode1, GL_TnL
    move s3, vtx1

    bgezal trcode2, GL_TnL
    move s3, vtx2

    bgezal trcode3, GL_TnL
    move s3, vtx3

    lbu t0, SCREEN_VTX_CLIP_CODE(vtx1)
    lbu t1, SCREEN_VTX_CLIP_CODE(vtx2)
    lbu t2, SCREEN_VTX_CLIP_CODE(vtx3)
    or t5, t0, t1
    or t5, t2

    move s1, zero
    beqz t5, gl_draw_single_triangle
    move s2, zero

    jal GL_ClipTriangle
    nop

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

    #undef vtx1
    #undef vtx2
    #undef vtx3
    .endfunc

#include "rsp_gpu_clipping.inc"
#include <rsp_rdpq.inc>
