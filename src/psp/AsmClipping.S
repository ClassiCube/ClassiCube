.set noreorder
#include "AsmCommon.i"

#define SRC $a0
#define DST $a1
#define CNT $a2
#define PLN $a3
#define CUR $t0 // current vertex
#define PRV $t1 // previous vertex
#define END $t2

#define D_POS R500
#define D_OTH R501

#define C_POS R600
#define C_OTH R601
#define P_POS R602
#define P_OTH R603

#define PLN_Q R700
#define ZRO_S S701
#define C_DST S711
#define P_DST S721
#define D_DST S731
#define T_S   S702

.macro ComputeIntersection
	vsub.s	D_DST, C_DST, P_DST    	// delta = cur_dist - prev_dist
	vdiv.s	T_S[0:1], C_DST, D_DST 	// t     = clamp(cur_dist / delta, 0.0, 1.0)
	vmov.q  D_OTH, C_OTH			// delta.uvcf = cur.uvcf

	vsub.q	D_POS, C_POS, P_POS		// delta.xyzw = cur.xyzw - prev.xyzw
	vsub.p	D_OTH, C_OTH, P_OTH		// delta.uv   = cur.uv   - prev.uv
	vscl.q	D_POS, D_POS, T_S		// delta.xyzw *= t
	vscl.p	D_OTH, D_OTH, T_S		// delta.uv   *= t
	vadd.q	D_POS, D_POS, C_POS		// delta.xyzw += cur.xyzw
	vadd.p	D_OTH, D_OTH, C_OTH		// delta.uv   += cur.uv
.endm


BEG_FUNC(Clip_PolyToPlane)
// Initialise current and previous vertices
	move 	CUR, SRC			// CUR = src
	// prev = (cur - 1) % in_count
	// so for first iteration, prev is in_count - 1
	sll		PRV, CNT,S_POLY_LOG2// PRV = count * sizeof(POLY_VERTEX)
	addiu	PRV, PRV, -S_POLY 	// PRV -= sizeof(POLY_VERTEX)
	add		PRV, PRV, SRC		// PRV += src
	move	END, PRV			// END = PRV (src + (count-1) * sizeof(POLY_VERTEX))

	lv.q	PLN_Q, 0(PLN)		// PLANE = MEM_128[PLN]
	vzero.s ZRO_S				// ZRO_S = 0.0

clip_next_line:
	lv.q	C_POS, POLY_XYZW(CUR)
	lv.q	C_OTH, POLY_UVCF(CUR)// TODO delay loaded ?
	lv.q	P_POS, POLY_XYZW(PRV)
	lv.q	P_OTH, POLY_UVCF(PRV)

	vdot.q	C_DST, PLN_Q, C_POS	// C_DST = plane * CUR (distance from current vertex to plane)
	vdot.q	P_DST, PLN_Q, P_POS	// P_DST = plane * PRV (distance from previous vertex to plane)

	vcmp.s	LE, C_DST, ZRO_S	// CC = dist(current, plane) <= 0.0 (is current behind plane)
	bvt 0, cur_behind		    // if (CC) goto cur_behind
	nop

	vcmp.s	LE, P_DST, ZRO_S	// CC = dist(previous, plane) <= 0.0 (is previous behind plane)
	bvt 0, cur_front_prev_behind// if (CC) goto cur_front_prev_behind
	nop


cur_front_prev_front:
	sv.q	C_POS, POLY_XYZW(DST)
	sv.q	C_OTH, POLY_UVCF(DST)
	j		line_finished
	addiu	DST, DST, S_POLY 		// dst += sizeof(PolyVertex)

cur_front_prev_behind:
	ComputeIntersection
	sv.q	D_POS, POLY_XYZW(DST)
	sv.q	D_OTH, POLY_UVCF(DST)
	addiu	DST, DST, S_POLY // dst += sizeof(PolyVertex)

	sv.q	C_POS, POLY_XYZW(DST)
	sv.q	C_OTH, POLY_UVCF(DST)
	j		line_finished
	addiu	DST, DST, S_POLY // dst += sizeof(PolyVertex)


cur_behind:
	vcmp.s	LE, P_DST, ZRO_S	// CC = dist(previous, plane) <= 0.0 (is previous behind plane)
	bvt 0, cur_behind_prev_behind // if (CC) goto cur_behind_prev_behind
	nop

cur_behind_prev_front:
	ComputeIntersection
	sv.q	D_POS, POLY_XYZW(DST)
	sv.q	D_OTH, POLY_UVCF(DST)
	j		line_finished
	addiu	DST, DST, S_POLY // dst += sizeof(PolyVertex)

cur_behind_prev_behind:
	// No vertices are output if both points are behind the plane


line_finished:
	move  PRV, CUR					// PRV = CUR
	bne   CUR, END, clip_next_line	// if (CUR != END) goto clip_next_line
	addiu CUR, CUR, S_POLY			// CUR += sizeof(PolyVertex) (delay slot)
	// TODO
	
	jr $ra
	move $v0, DST // V0 = DST (delay slot)

