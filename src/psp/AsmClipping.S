.set noreorder
#include "AsmCommon.i"

#define D_POS R500
#define D_OTH R501

#define C_POS R600
#define C_OTH R601
#define P_POS R602
#define P_OTH R603

#define PLANE R700
#define ZERO_ S701
#define C_DST S711
#define P_DST S721
#define D_DST S731
#define T_S   S702

.macro ComputeIntersection
	vsub.s	D_DST, C_DST, P_DST    	// delta = cur_dist - prev_dist
	vdiv.s	T_S[0:1], C_DST, D_DST 	// t     = clamp(cur_dist / delta, 0.0, 1.0)
	vmov.q  D_OTH, C_OTH			// delta.uvcf = cur.uvcf

	vsub.q	D_POS, P_POS, C_POS		// delta.xyzw = prev.xyzw - cur.xyzw
	vsub.p	D_OTH, P_OTH, C_OTH		// delta.uv   = prev.uv   - cur.uv
	vscl.q	D_POS, D_POS, T_S		// delta.xyzw *= t
	vscl.p	D_OTH, D_OTH, T_S		// delta.uv   *= t
	vadd.q	D_POS, D_POS, C_POS		// delta.xyzw += cur.xyzw
	vadd.p	D_OTH, D_OTH, C_OTH		// delta.uv   += cur.uv
.endm

#define BUF_SRC $a0
#define BUF_DST $a1
#define PLN_CNT $a2
#define PLN_PTR $a3
#define CNT_SRC $t4
#define CNT_DST $t5

#define CUR $t0 // current vertex
#define PRV $t1 // previous vertex
#define END $t2
#define DST $t3
#define RET $v0
#define TMP $v0

BEG_FUNC(Clip_PolyToPlanes)
	vzero.s ZERO_
	li CNT_SRC, 4 // start with 4 quads

clip_next_plane:
	// swap src and dst buffers
	move	TMP,	 BUF_SRC
	move	BUF_SRC, BUF_DST
	move	BUF_DST, TMP

	// prev = (cur - 1) % in_count
	// so for first iteration, prev is 'src_count - 1'
	// calculate prv = (1 << log2(sizeof(ClipVertex))) - sizeof(ClipVertex) + BUF_SRC
	//				 = BUF_SRC +  (count - 1) * sizeof(ClipVertex))
	sll		PRV, CNT_SRC, S_VCLP_LOG2
	addiu	PRV, 	 PRV, -S_VCLP
	addu	PRV, 	 PRV, BUF_SRC

	move 	CUR, BUF_SRC
	move	END, PRV
	move	DST, BUF_DST
	lv.q	PLANE, 0(PLN_PTR)


clip_next_line:
	lv.q	C_POS, VCLP_XYZW(CUR)
	lv.q	C_OTH, VCLP_UVCF(CUR)// TODO delay loaded ?
	lv.q	P_POS, VCLP_XYZW(PRV)
	lv.q	P_OTH, VCLP_UVCF(PRV)

	vdot.q	C_DST, PLANE, C_POS	// C_DST = plane * CUR (distance from current vertex to plane)
	vdot.q	P_DST, PLANE, P_POS	// P_DST = plane * PRV (distance from previous vertex to plane)

	vcmp.s	LE, C_DST, ZERO_	// CC = dist(current, plane) <= 0.0 (is current behind plane)
	bvt 0, cur_behind		    // if (CC) goto cur_behind
	nop

	vcmp.s	LE, P_DST, ZERO_	// CC = dist(previous, plane) <= 0.0 (is previous behind plane)
	bvt 0, cur_front_prev_behind// if (CC) goto cur_front_prev_behind
	nop

cur_front_prev_front:
	sv.q	C_POS, VCLP_XYZW(DST)
	sv.q	C_OTH, VCLP_UVCF(DST)
	j		line_finished
	addiu	DST, DST, S_VCLP 		// dst += sizeof(PolyVertex)

cur_front_prev_behind:
	ComputeIntersection
	sv.q	D_POS, VCLP_XYZW(DST)
	sv.q	D_OTH, VCLP_UVCF(DST)
	addiu	DST, DST, S_VCLP // dst += sizeof(ClipVertex)

	sv.q	C_POS, VCLP_XYZW(DST)
	sv.q	C_OTH, VCLP_UVCF(DST)
	j		line_finished
	addiu	DST, DST, S_VCLP // dst += sizeof(ClipVertex)

cur_behind:
	vcmp.s	LE, P_DST, ZERO_		// CC = dist(previous, plane) <= 0.0 (is previous behind plane)
	bvt 0, cur_behind_prev_behind // if (CC) goto cur_behind_prev_behind
	nop

cur_behind_prev_front:
	ComputeIntersection
	sv.q	D_POS, VCLP_XYZW(DST)
	sv.q	D_OTH, VCLP_UVCF(DST)
	j		line_finished
	addiu	DST, DST, S_VCLP // dst += sizeof(ClipVertex)

cur_behind_prev_behind:
	// No vertices are output if both points are behind the plane

line_finished:
	move  	PRV, CUR				 // PRV = CUR
	bne   	CUR, END, clip_next_line // if (CUR != END) goto clip_next_line
	addiu	CUR, CUR, S_VCLP		 // CUR += sizeof(PolyVertex) (delay slot)


plane_finished:
	// if (dst == buf_dst) goto no_vertices_left;
	beq		DST, BUF_DST, no_vertices_left
	addiu	PLN_CNT, -1				// delay slot

	// dst_count = (int)(dst - buf) >> log2(sizeof(PolyVertex))
	subu	CNT_DST,     DST, BUF_DST
	srl		CNT_DST, CNT_DST, S_VCLP_LOG2
	// set src to dst count for next iteration
	move	CNT_SRC, CNT_DST

	bne   	PLN_CNT, $0, clip_next_plane
	addiu	PLN_PTR, 16 			// delay slot


no_more_planes:
	jr		$ra
	// lower 4 bits are count, upper bits are clip buffer
	or		RET, BUF_DST, CNT_DST // delay slot
		
no_vertices_left:
	jr		$ra
	move 	RET, $0

END_FUNC(Clip_PolyToPlanes)

