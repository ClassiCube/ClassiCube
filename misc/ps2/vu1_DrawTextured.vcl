// === GLOBAL DATA ===
// === DRAW DATA ===
// ADDR_VERTS_COUNT              0
// === GLOBAL REGISTERS ===
// === LOCAL REGISTERS ===
.syntax new
.init_vf // no registers available for dynamic allocation
.init_vi_all // all registers available for dynamic allocation
.vu
.name Reload_MVP
--enter
--endenter
 lq vf01, 0(vi00)
 lq vf02, 1(vi00)
 lq vf03, 2(vi00)
 lq vf04, 3(vi00)
--exit
--endexit
.name Reload_Viewport
--enter
--endenter
 lq vf05, 5(vi00)
 lq vf06, 6(vi00)
--exit
--endexit
.name Reload_Guardband
--enter
--endenter
 lq vf07, 4(vi00)
--exit
--endexit
.name DrawTexturedQuad
--enter
 in_vf mvp_r1(vf01)
 in_vf mvp_r2(vf02)
 in_vf mvp_r3(vf03)
 in_vf mvp_r4(vf04)
 in_vf gb_scale(vf07)
--endenter
 xtop iBase
 ilw.x numQuads, 0(iBase) // ADDR_VERTS_COUNT
 iaddiu srcAddr, iBase, 1 // ADDR_SRC_OFFSET
 iaddiu dstAddr, iBase, 100 // ADDR_DST_OFFSET
 iaddiu kickAddr, iBase, 100 // ADDR_DST_OFFSET
 // Copy the GIF packet
 lqi vf18, (srcAddr++)
 sqi vf18, (dstAddr++)
TRANSFORM_LOOP:
 // Transform first vertex
 lqi vf31, (srcAddr++)
 mula acc, mvp_r4, vf00[w] // ACC  = mvp.row4
 madda acc, mvp_r1, vf31[x] // ACC  = ACC + mvp.row1 * src.x
 madda acc, mvp_r2, vf31[y] // ACC  = ACC + mvp.row2 * src.y
 madd vf30, mvp_r3, vf31[z] // dst  = ACC + mvp.row3 * src.z
 div q, vf00[w], vf30[w]// Q    = 1 / dst.w
 mul vf18, vf30, gb_scale // TMP  = TRANSFORMED(dst) * CLIP_PLANES_ADJUST
 clipw.xyz vf18, vf18 // CLIP_FLAGS.append(CLIP(TMP.xyz, TMP.w))
 lqi vf29, (srcAddr++)
 lqi vf28, (srcAddr++)
 // Perspective divide vertex 1
 mul.xyz vf30, vf30, q
 mul.xyz vf29, vf29, q
 ftoi0 vf30, vf30
 // Transform second vertex
 lqi vf31, (srcAddr++)
 mula acc, mvp_r4, vf00[w] // ACC  = mvp.row4
 madda acc, mvp_r1, vf31[x] // ACC  = ACC + mvp.row1 * src.x
 madda acc, mvp_r2, vf31[y] // ACC  = ACC + mvp.row2 * src.y
 madd vf27, mvp_r3, vf31[z] // dst  = ACC + mvp.row3 * src.z
 div q, vf00[w], vf27[w]// Q    = 1 / dst.w
 mul vf18, vf27, gb_scale // TMP  = TRANSFORMED(dst) * CLIP_PLANES_ADJUST
 clipw.xyz vf18, vf18 // CLIP_FLAGS.append(CLIP(TMP.xyz, TMP.w))
 lqi vf26, (srcAddr++)
 lqi vf25, (srcAddr++)
 // Perspective divide vertex 2
 mul.xyz vf27, vf27, q
 mul.xyz vf26, vf26, q
 ftoi0 vf27, vf27
 // Transform third vertex
 lqi vf31, (srcAddr++)
 mula acc, mvp_r4, vf00[w] // ACC  = mvp.row4
 madda acc, mvp_r1, vf31[x] // ACC  = ACC + mvp.row1 * src.x
 madda acc, mvp_r2, vf31[y] // ACC  = ACC + mvp.row2 * src.y
 madd vf24, mvp_r3, vf31[z] // dst  = ACC + mvp.row3 * src.z
 div q, vf00[w], vf24[w]// Q    = 1 / dst.w
 mul vf18, vf24, gb_scale // TMP  = TRANSFORMED(dst) * CLIP_PLANES_ADJUST
 clipw.xyz vf18, vf18 // CLIP_FLAGS.append(CLIP(TMP.xyz, TMP.w))
 lqi vf23, (srcAddr++)
 lqi vf22, (srcAddr++)
 // Perspective divide vertex 3
 mul.xyz vf24, vf24, q
 mul.xyz vf23, vf23, q
 ftoi0 vf24, vf24
 // Transform fourth vertex
 lqi vf31, (srcAddr++)
 mula acc, mvp_r4, vf00[w] // ACC  = mvp.row4
 madda acc, mvp_r1, vf31[x] // ACC  = ACC + mvp.row1 * src.x
 madda acc, mvp_r2, vf31[y] // ACC  = ACC + mvp.row2 * src.y
 madd vf21, mvp_r3, vf31[z] // dst  = ACC + mvp.row3 * src.z
 div q, vf00[w], vf21[w]// Q    = 1 / dst.w
 mul vf18, vf21, gb_scale // TMP  = TRANSFORMED(dst) * CLIP_PLANES_ADJUST
 clipw.xyz vf18, vf18 // CLIP_FLAGS.append(CLIP(TMP.xyz, TMP.w))
 lqi vf20, (srcAddr++)
 lqi vf19, (srcAddr++)
 // Perspective divide vertex 4
 mul.xyz vf21, vf21, q
 mul.xyz vf20, vf20, q
 ftoi0 vf21, vf21
 // Calculate clip flags
 fcand VI01, 0xFFFFFF
 iaddiu iADC, VI01, 0x7FFF
 // Write V0, V1, V2, V2, V3, V0
 sqi vf30, (dstAddr++)
 sqi vf29, (dstAddr++)
 sqi vf28, (dstAddr++)
 isw.w iADC, -3(dstAddr)
 sqi vf27, (dstAddr++)
 sqi vf26, (dstAddr++)
 sqi vf25, (dstAddr++)
 isw.w iADC, -3(dstAddr)
 sqi vf24, (dstAddr++)
 sqi vf23, (dstAddr++)
 sqi vf22, (dstAddr++)
 isw.w iADC, -3(dstAddr)
 sqi vf24, (dstAddr++)
 sqi vf23, (dstAddr++)
 sqi vf22, (dstAddr++)
 isw.w iADC, -3(dstAddr)
 sqi vf21, (dstAddr++)
 sqi vf20, (dstAddr++)
 sqi vf19, (dstAddr++)
 isw.w iADC, -3(dstAddr)
 sqi vf30, (dstAddr++)
 sqi vf29, (dstAddr++)
 sqi vf28, (dstAddr++)
 isw.w iADC, -3(dstAddr)
 // Next loop
 iaddi numQuads, numQuads, -1
 ibne numQuads, vi00, TRANSFORM_LOOP
 // Finished
 xgkick kickAddr
--exit
--endexit
