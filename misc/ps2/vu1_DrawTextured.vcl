// === GLOBAL DATA ===
// === DRAW DATA ===
// ADDR_VERTS_COUNT              0
// === GLOBAL REGISTERS ===
// === LOCAL REGISTERS ===
.syntax new
.init_vf_all
.init_vi_all
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
--endenter
 xtop iBase
 ilw.x numQuads, 0(iBase) // ADDR_VERTS_COUNT
 iaddiu srcAddr, iBase, 1 // ADDR_SRC_OFFSET
 iaddiu dstAddr, iBase, 100 // ADDR_DST_OFFSET
 iaddiu kickAddr, iBase, 100 // ADDR_DST_OFFSET
 // Copy the GIF packet
 lqi gif_tag, (srcAddr++)
 sqi gif_tag, (dstAddr++)
TRANSFORM_LOOP:
 // Transform first vertex
 lqi vf31, (srcAddr++)
 mula acc, vf04, vf00[w] // ACC  = mvp.row4
 madda acc, vf01, vf31[x] // ACC  = ACC + mvp.row1 * src.x
 madda acc, vf02, vf31[y] // ACC  = ACC + mvp.row2 * src.y
 madd V0, vf03, vf31[z] // dst  = ACC + mvp.row3 * src.z
 div q, vf00[w], V0[w] // Q    = 1 / dst.w
 mul tmp, V0, vf07 // TMP  = TRANSFORMED(dst) * CLIP_PLANES_ADJUST
 clipw.xyz tmp, tmp // CLIP_FLAGS.append(CLIP(TMP.xyz, TMP.w))
 lqi uv0, (srcAddr++)
 lqi col0, (srcAddr++)
 // Perspective divide vertex 1
 mul.xyz V0, V0, q
 mul.xyz uv0, uv0, q
 ftoi0 V0, V0
 // Transform second vertex
 lqi vf31, (srcAddr++)
 mula acc, vf04, vf00[w] // ACC  = mvp.row4
 madda acc, vf01, vf31[x] // ACC  = ACC + mvp.row1 * src.x
 madda acc, vf02, vf31[y] // ACC  = ACC + mvp.row2 * src.y
 madd V0, vf03, vf31[z] // dst  = ACC + mvp.row3 * src.z
 div q, vf00[w], V1[w] // Q    = 1 / dst.w
 mul tmp, V1, vf07 // TMP  = TRANSFORMED(dst) * CLIP_PLANES_ADJUST
 clipw.xyz tmp, tmp // CLIP_FLAGS.append(CLIP(TMP.xyz, TMP.w))
 lqi uv1, (srcAddr++)
 lqi col1, (srcAddr++)
 // Perspective divide vertex 2
 addq.z uv1, VF00, q
 mul.xyz V1, V1, q
 mul.xy uv1, uv1, q
 ftoi0 V1, V1
 // Transform third vertex
 lqi vf31, (srcAddr++)
 mula acc, vf04, vf00[w] // ACC  = mvp.row4
 madda acc, vf01, vf31[x] // ACC  = ACC + mvp.row1 * src.x
 madda acc, vf02, vf31[y] // ACC  = ACC + mvp.row2 * src.y
 madd V0, vf03, vf31[z] // dst  = ACC + mvp.row3 * src.z
 div q, vf00[w], V2[w] // Q    = 1 / dst.w
 mul tmp, V2, vf07 // TMP  = TRANSFORMED(dst) * CLIP_PLANES_ADJUST
 clipw.xyz tmp, tmp // CLIP_FLAGS.append(CLIP(TMP.xyz, TMP.w))
 lqi uv2, (srcAddr++)
 lqi col2, (srcAddr++)
 // Perspective divide vertex 3
 mul.xyz V2, V2, q
 mul.xyz uv2, uv2, q
 ftoi0 V2, V2
 // Transform fourth vertex
 lqi vf31, (srcAddr++)
 mula acc, vf04, vf00[w] // ACC  = mvp.row4
 madda acc, vf01, vf31[x] // ACC  = ACC + mvp.row1 * src.x
 madda acc, vf02, vf31[y] // ACC  = ACC + mvp.row2 * src.y
 madd V0, vf03, vf31[z] // dst  = ACC + mvp.row3 * src.z
 div q, vf00[w], V3[w] // Q    = 1 / dst.w
 mul tmp, V3, vf07 // TMP  = TRANSFORMED(dst) * CLIP_PLANES_ADJUST
 clipw.xyz tmp, tmp // CLIP_FLAGS.append(CLIP(TMP.xyz, TMP.w))
 lqi uv3, (srcAddr++)
 lqi col3, (srcAddr++)
 // Perspective divide vertex 4
 mul.xyz V3, V3, q // dst /= dst.w
 mul.xy uv3, uv3, q // uv  /= dst.w
 madd V3, mvp[2], xyz3[z] // dst = ACC + mvp.row1 * src.z
 ftoi0 V3, V3
 // Calculate clip flags
 fcand VI01, 0xFFFFFF
 iaddiu iADC, VI01, 0x7FFF
 // Write V0, V1, V2, V2, V3, V0
 sqi V0, (dstAddr++)
 sqi uv0, (dstAddr++)
 sqi col0,(dstAddr++)
 isw.w iADC, -3(dstAddr)
 sqi V1, (dstAddr++)
 sqi uv1, (dstAddr++)
 sqi col1,(dstAddr++)
 isw.w iADC, -3(dstAddr)
 sqi V2, (dstAddr++)
 sqi uv2, (dstAddr++)
 sqi col2,(dstAddr++)
 isw.w iADC, -3(dstAddr)
 sqi V2, (dstAddr++)
 sqi uv2, (dstAddr++)
 sqi col2,(dstAddr++)
 isw.w iADC, -3(dstAddr)
 sqi V3, (dstAddr++)
 sqi uv3, (dstAddr++)
 sqi col3,(dstAddr++)
 isw.w iADC, -3(dstAddr)
 sqi V0, (dstAddr++)
 sqi uv0, (dstAddr++)
 sqi col0,(dstAddr++)
 isw.w iADC, -3(dstAddr)
 // Next loop
 iaddi numQuads, numQuads, -1
 ibne numQuads, vi00, TRANSFORM_LOOP
 // Finished
 xgkick kickAddr
--exit
--endexit
