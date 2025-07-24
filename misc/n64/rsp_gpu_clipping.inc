#define CLIPPING_PLANE_COUNT  6
#define CLIPPING_CACHE_SIZE   10
#define CLIPPING_PLANE_SIZE   8

    .section .data.gl_clipping

    .align 4
CLIP_PLANES:
    .half 1, 0, 0, GUARD_BAND_FACTOR
    .half 0, 1, 0, GUARD_BAND_FACTOR
    .half 0, 0, 1, 1
    .half 1, 0, 0, -GUARD_BAND_FACTOR
    .half 0, 1, 0, -GUARD_BAND_FACTOR
    .half 0, 0, 1, -1

    .align 4
CACHE_OFFSETS: .half 2,4,6,8, 10,12,14,16, 18,20

    .section .bss.gl_clipping

CLIP_CACHE: .dcb.b     SCREEN_VTX_SIZE * CLIPPING_CACHE_SIZE
CLIP_CACHE_END:

CLIP_LISTS:
    CLIP_LIST0: .dcb.w  CLIPPING_CACHE_SIZE
    CLIP_LIST1: .dcb.w  CLIPPING_CACHE_SIZE


    .section .text.gl_clipping

    ################################################################
    # GL_ClipTriangle
    #   Clip a triangle against the view-frustum by using the Sutherland-Hodgman algorithm
    #   https://en.wikipedia.org/wiki/Sutherland%E2%80%93Hodgman_algorithm
    # Args:
    #   a1-a3,a0 = Vertices
    #   t5       = OR'd clip flags of the triangle's vertices
    # Returns:
    #   s1    = Pointer to list of output vertices
    #   s2    = Pointer to end of list
    ################################################################
    .func GL_ClipTriangle
GL_ClipTriangle:
    #define out_count       v1
    #define clip_flags      t5
    #define plane_flag      t6
    #define in_count        t7
    #define in_end          t8
    #define in_list         v0
    #define out_list        s1
    #define plane           s2
    #define intersection    s3
    #define cur_ptr         a0
    #define prev_ptr        a1
    #define cur_vtx         a2
    #define prev_vtx        a3
    #define p0              k0
    #define p1              k1
    #define vtx1            a1
    #define vtx2            a2
    #define vtx3            a3
    #define vtx4            a0

    #define vplane          $v01
    #define vint_f          $v02
    #define vint_i          $v03
    #define vdot_i          $v04
    #define vdot_f          $v05
    #define vdiff_i         $v06
    #define vdiff_f         $v07
    #define va_i            $v08
    #define va_f            $v09
    #define vpos_i          $v10
    #define vpos_f          $v11
    #define vattr0          $v12
    #define vattr1          $v13
    #define voff0           $v14
    #define voff1           $v15
    #define vcache0         $v16
    #define vcache1         $v17
	// v18,v19 - reserved for viewport
	// v20,v21 - reserved for vguard
    #define vguard_f        $v27
    #define vguard_i        $v28
    #define v__             $v29

    move ra2, ra

    # Init in_list as empty
    li in_list, %lo(CLIP_LIST0)
    move in_count, zero

    # Put four original vertices in the out_list
    # (So after the initial swap they will be in the in_list)
    li out_list, %lo(CLIP_LIST1)
    sh vtx1, 0(out_list)
    sh vtx2, 2(out_list)
    sh vtx3, 4(out_list)
    sh vtx4, 6(out_list)
    li out_count, 4*2

#undef vtx1
#undef vtx2
#undef vtx3
#undef vtx4

    li plane, %lo(CLIP_PLANES)
    li plane_flag, 1

    # Load cache offsets    
    li t0, %lo(CACHE_OFFSETS)
    vxor voff1, voff1
    lqv voff0,  0,t0
    ldv voff1, 16,t0

    # Temporarily use the RDP staging area as a map of which cache slots are used
    # Init to zero
    li t0, %lo(RDPQ_CMD_STAGING)
    sqv vzero,  0,t0
    sqv vzero, 16,t0

    # Iterate over the 6 clipping planes
gl_clip_plane_loop:
    and t0, clip_flags, plane_flag
    beqz t0, gl_clip_plane_loop_end
    move t1, in_list

    # Swap in and out lists

    # If the out list is empty from the last iteration, 
    # the triangle has no visible points and we are done
    beqz out_count, gl_clip_return
    move in_list, out_list
    move out_list, t1
    move in_count, out_count
    move out_count, zero

    # Iterate over the egdes of the polygon in the input list
    # The current edge is between cur_vtx and prev_vtx
    move cur_ptr, in_list
    add in_end, in_list, in_count
    # Init the "previous" vertex to the last in the list for the wrap-around
    addi prev_ptr, in_end, -2

gl_clip_edge_loop:
    #define cur_flag  t3
    #define prev_flag t4

    # Check which side of the plane the two vertices are on
    lhu cur_vtx,   0(cur_ptr)
    lhu prev_vtx,  0(prev_ptr)
    lbu cur_flag,  SCREEN_VTX_CLIP_CODE(cur_vtx)
    lbu prev_flag, SCREEN_VTX_CLIP_CODE(prev_vtx)
    and cur_flag,  plane_flag
    and prev_flag, plane_flag

    # If they are on opposite sides, there is an intersection
    xor t0, cur_flag, prev_flag
    beqz t0, gl_clip_no_intersection
    move p0, cur_vtx

    # Swap the two points if necessary to make intersection calculation consistent
    # This will make sure p0 is always inside and p1 is always outside
    bnez prev_flag, gl_clip_no_swap
    move p1, prev_vtx
    xor p0, p0, p1
    xor p1, p0, p1
    xor p0, p0, p1

    #undef prev_flag

gl_clip_no_swap:
    # Calculate intersection of the line segment and the plane

    li t0, %lo(RDPQ_CMD_STAGING)
    lqv vcache0,    0,t0
    lqv vcache1,   16,t0

    # Repeat plane coefficients twice
    ldv vplane.e0,  0,plane
    ldv vplane.e4,  0,plane

    # vpos: x0  y0  z0  w0  x1  y1  z1  w1
    ldv vpos_i.e0,  SCREEN_VTX_CS_POSi,p0
    ldv vpos_f.e0,  SCREEN_VTX_CS_POSf,p0
    ldv vpos_i.e4,  SCREEN_VTX_CS_POSi,p1
    ldv vpos_f.e4,  SCREEN_VTX_CS_POSf,p1

    # vint: x1  y1  z1  w1
    ldv vint_i.e0,  SCREEN_VTX_CS_POSi,p1
    ldv vint_f.e0,  SCREEN_VTX_CS_POSf,p1

    # vattr0: r0  g0  b0  a0  s0  t0
    luv vattr0.e0,  SCREEN_VTX_RGBA   ,p0
    llv vattr0.e4,  SCREEN_VTX_S_T    ,p0

    # vattr1: r1  g1  b1  a1  s1  t1
    luv vattr1.e0,  SCREEN_VTX_RGBA   ,p1
    llv vattr1.e4,  SCREEN_VTX_S_T    ,p1

    # Find first free slot in clip cache

    # Add the values from the "used slots map" to the cache offsets
    # After this, each lane will contain the offset of its corresponding cache slot,
    # but only if the slot is not used. If it is used, it will contain some large value.
    vaddc vcache0, voff0
    vaddc vcache1, voff1

    # Look for the smallest value, which will end up in vcache.e0
    # Because used slots are marked as large values, they will never be found.
    vlt vcache0, vcache0.q1
    vlt vcache0, vcache0.h2
    vlt vcache0, vcache0.e4
    vlt vcache0, vcache1.e0
    vlt vcache0, vcache1.e1

    mfc2 t0, vcache0.e0

    # Mark slot as used by storing some large value (careful of overflows!)
    li t1, 0xFF
    sh t1, %lo(RDPQ_CMD_STAGING)-2(t0)

    # t0 is the index multiplied by 2
    # intersection = t0 * 20 = t0 * 16 + t0 * 4
    sll intersection, t0, 4
    sll t1, t0, 2
    add intersection, t1

    # CAUTION: intersection might point to the same address as either p0 or p1,
    # because one of them is the previous point, which could have been marked unused
    # in the previous iteration. As long as we don't access p0 or p1 after writing to
    # intersection, this is fine.
    addi intersection, %lo(CLIP_CACHE) - SCREEN_VTX_SIZE

    # Store the cache offset in unused memory (used later when finding the cache slot to mark as unused)
    sb t0, SCREEN_VTX_PADDING(intersection)

    # Compute dot products of both positions with the clip plane
    # vdot.e0: d0 = dot(p0, plane)
    # vdot.e4: d1 = dot(p1, plane)
    vmudn vdot_f, vpos_f, vplane
    vmadh vdot_i, vpos_i, vplane
    vaddc vdot_f, vdot_f.q1
    vadd  vdot_i, vdot_i.q1
    vaddc vdot_f, vdot_f.h2
    vadd  vdot_i, vdot_i.h2

    # d0 - d1
    vsubc vdiff_f, vdot_f, vdot_f.e4
    vsub  vdiff_i, vdot_i, vdot_i.e4

    # 1 / (d0 - d1)
    vrcph v__.e0,  vdiff_i.e0
    vrcpl va_f.e0, vdiff_f.e0
    vrcph va_i.e0, vzero.e0

    # a = d0 / (d0 - d1)
    vmudl v__,  va_f, vdot_f.e0
    vmadm v__,  va_i, vdot_f.e0
    vmadn va_f, va_f, vdot_i.e0

    # Prepare 0x7FFF in va_i.e0
    vsubc va_i, vshift8, K1

    # a = min(a, 1)
    vge  v__,  va_f, vzero
    vmrg va_f, va_f, va_i.e0

    # Account for right shift introduced by vrcp
    vmudn va_f, va_f, K2

    # p1 - p0
    vsubc vint_f, vpos_f
    vsub  vint_i, vpos_i
    # attr1 - attr0
    vsubc vattr1, vattr0

    # Result of linear interpolation:
    # p0 + a * (p1 - p0)
    vmudl v__,    vint_f, va_f.e0
    vmadm v__,    vint_i, va_f.e0
    vmadn vint_f, vpos_f, K1
    vmadh vint_i, vpos_i, K1

    # a * (attr1 - attr0)
    vmudm vattr1, vattr1, va_f.e0

    # attr0 + a * (attr1 - attr0)
    vaddc vattr0, vattr1

    # Store results
    sdv vint_i.e0,  SCREEN_VTX_CS_POSi,intersection
    sdv vint_f.e0,  SCREEN_VTX_CS_POSf,intersection
    suv vattr0.e0,  SCREEN_VTX_RGBA   ,intersection
    slv vattr0.e4,  SCREEN_VTX_S_T    ,intersection

	# Update clip flags
	vmudn vguard_f, vint_f, vguardscale // vint_f is vcspos_f
    vmadh vguard_i, vint_i, vguardscale // vint_i is vcspos_i
    
    vch v__, vguard_i, vguard_i.e3 // w
    vcl v__, vguard_f, vguard_f.e3 // w

    cfc2 t0, COP2_CTRL_VCC
    compressClipCodes
    sb t2, SCREEN_VTX_CLIP_CODE(intersection)

    # Add intersection to the output list
    add t0, out_list, out_count
    sh intersection, 0(t0)
    addi out_count, 2

gl_clip_no_intersection:
    # If cur_vtx is inside, add it to the output list
    bnez cur_flag, gl_clip_no_current
    add t0, out_list, out_count
    sh cur_vtx, 0(t0)
    b gl_clip_edge_loop_end
    addi out_count, 2

    #undef cur_flag

gl_clip_no_current:
    # Check if the vertex is stored in the clip cache
    lbu t0, SCREEN_VTX_PADDING(cur_vtx)
    beqz t0, gl_clip_edge_loop_end
    # Reset the padding field to zero, so the screen space values won't be recalculated below
    sb zero, SCREEN_VTX_PADDING(cur_vtx)
    # If so, mark it as unused
    sh zero, %lo(RDPQ_CMD_STAGING)-2(t0)
    
gl_clip_edge_loop_end:
    # Advance to the next edge
    addi cur_ptr, 2
    blt cur_ptr, in_end, gl_clip_edge_loop
    addi prev_ptr, cur_ptr, -2

gl_clip_plane_loop_end:
    # Advance to the next clipping plane
    sll plane_flag, 1
    blt plane_flag, (1<<CLIPPING_PLANE_COUNT), gl_clip_plane_loop
    addi plane, CLIPPING_PLANE_SIZE

gl_clip_return:
    # Done!
    jr ra2
    add s2, out_list, out_count

    #undef clip_flags
    #undef plane_flag
    #undef in_count
    #undef out_count
    #undef in_end
    #undef intersection
    #undef in_list
    #undef out_list
    #undef plane
    #undef cur_ptr
    #undef prev_ptr
    #undef cur_vtx
    #undef prev_vtx
    #undef p0
    #undef p1
    #undef vplane
    #undef vpos_i
    #undef vpos_f
    #undef vdot_i
    #undef vdot_f
    #undef vdiff_i
    #undef vdiff_f
    #undef va_f
    #undef vint_i
    #undef vint_f
    #undef vattr0
    #undef vattr1
    #undef v__
    #undef vguard_i
    #undef vguard_f

    .endfunc
