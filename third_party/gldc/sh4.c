#include <kos.h>
#include <dc/pvr.h>
#include "gldc.h"

// calculates 1/sqrt(x)
static GLDC_FORCE_INLINE float sh4_fsrra(float x) {
  asm volatile ("fsrra %[value]\n"
  : [value] "+f" (x) // outputs (r/w to FPU register)
  : // no inputs
  : // no clobbers
  );
  return x;
}

static GLDC_FORCE_INLINE void PushVertex(Vertex* v, volatile Vertex* dst) {
	float ww   = v->w * v->w;
	dst->flags = v->flags;
	float f	= sh4_fsrra(ww); // 1/sqrt(w^2) ~ 1/w
	// Convert to NDC (viewport already applied)
	float x	= v->x * f;
	float y	= v->y * f;

	dst->x	 = x;
	dst->y	 = y;
	dst->z	 = f;
	dst->u	 = v->u;
	dst->v	 = v->v;
	dst->bgra  = v->bgra;
	__asm__("pref @%0" : : "r"(dst));
}

static inline void PushCommand(Vertex* v, volatile Vertex* dst)  {
	uint32_t* s = (uint32_t*)v;
	volatile uint32_t* sq = (volatile uint32_t*)dst;

	sq[0] = *(s++);
	sq[1] = *(s++);
	sq[2] = *(s++);
	sq[3] = *(s++);
	sq[4] = *(s++);
	sq[5] = *(s++);
	sq[6] = *(s++);
	sq[7] = *(s++);
	__asm__("pref @%0" : : "r"(sq));
}

extern void ClipEdge(Vertex* const v1, Vertex* const v2, volatile Vertex* vout, char type);

#define V0_VIS (1 << 0)
#define V1_VIS (1 << 1)
#define V2_VIS (1 << 2)
#define V3_VIS (1 << 3)

#define TYPE_VTX 0xE0 // PVR vertex, data
#define TYPE_EOS 0xF0 // PVR vertex, end of strip

extern void ProcessVertexList(Vertex* v3, int n, void* sq_addr);

void SceneListSubmit(Vertex* v3, int n) {
	volatile Vertex* dst = (volatile Vertex*)MEM_AREA_SQ_BASE;

	for (int i = 0; i < n; i++, v3++) 
	{
		// Preload next vertex into memory
		__builtin_prefetch(v3 + 1);

		switch(v3->flags & 0xFF000000) {
		case PVR_CMD_VERTEX_EOL:
			break;
		case PVR_CMD_VERTEX:
			continue;
		default:
			PushCommand(v3, dst++);
			continue;
		};

		// Quads [0, 1, 2, 3] -> Triangles [{0, 1, 2}  {2, 3, 0}]
		Vertex* const v0 = v3 - 3;
		Vertex* const v1 = v3 - 2;
		Vertex* const v2 = v3 - 1;
		uint8_t mask = v3->flags & 0xFF;

		// Check if all vertices visible
		if (__builtin_expect(mask == (V0_VIS | V1_VIS | V2_VIS | V3_VIS), 1)) {
			// Triangle strip: {1,2,0} {2,0,3}
			PushVertex(v1, dst++);
			PushVertex(v2, dst++);
			PushVertex(v0, dst++);
			PushVertex(v3, dst++);
			continue;
		}


		// Only some vertices visible
		// https://casual-effects.com/research/McGuire2011Clipping/clip.glsl
		switch(mask) {
		case V0_VIS:
			//		  v0
			//		 / |
			//	   /   |
			// .....A....B...
			//	/	  |
			//  v3--v2---v1
			PushVertex(v0,   dst++);           // v0
			ClipEdge(v0, v1, dst++, TYPE_VTX); // B
			ClipEdge(v3, v0, dst++, TYPE_EOS); // A
			break;

		case V1_VIS:
			//		  v1
			//		 / |
			//	   /   |
			// ....A.....B...
			//	/	  |
			//  v0--v3---v2
			ClipEdge(v0, v1, dst++, TYPE_VTX); // A
			PushVertex(v1,   dst++);           // v1
			ClipEdge(v1, v2, dst++, TYPE_EOS); // B
			break;

		case V2_VIS:
			//		  v2
			//		 / |
			//	   /   |
			// ....A.....B...
			//	/	  |
			//  v1--v0---v3
			ClipEdge(v1, v2, dst++, TYPE_VTX); // A
			PushVertex(v2,   dst++);	       // v2
			ClipEdge(v2, v3, dst++, TYPE_EOS); // B
		break;

		case V3_VIS:
			//		  v3
			//		 / |
			//	   /   |
			// ....A.....B...
			//	/	  |
			//  v2--v1---v0
			ClipEdge(v3, v0, dst++, TYPE_VTX); // B
			ClipEdge(v2, v3, dst++, TYPE_EOS); // A
			PushVertex(v3,   dst++);           // v3
			break;

		case V0_VIS | V1_VIS:
			//	v0-----------v1
			//	  \		   |
			//   ....B..........A...
			//		 \		|
			//		  v3-----v2
			PushVertex(v1,   dst++);           // v1
			ClipEdge(v1, v2, dst++, TYPE_VTX); // A
			PushVertex(v0,   dst++);           // v0
			ClipEdge(v3, v0, dst++, TYPE_EOS); // B
			break;

		// case V0_VIS | V2_VIS: degenerate case that should never happen
		case V0_VIS | V3_VIS:
			//	v3-----------v0
			//	  \		   |
			//   ....B..........A...
			//		 \		|
			//		  v2-----v1
			ClipEdge(v0, v1, dst++, TYPE_VTX); // A
			ClipEdge(v2, v3, dst++, TYPE_VTX); // B
			PushVertex(v0,   dst++);           // v0
			PushVertex(v3,   dst++);           // v3
			break;

		case V1_VIS | V2_VIS:
			//	v1-----------v2
			//	  \		   |
			//   ....B..........A...
			//		 \		|
			//		  v0-----v3
			PushVertex(v1,   dst++);           // v1
			PushVertex(v2,   dst++);           // v2
			ClipEdge(v0, v1, dst++, TYPE_VTX); // B
			ClipEdge(v2, v3, dst++, TYPE_EOS); // A
			break;

		// case V1_VIS | V3_VIS: degenerate case that should never happen
		case V2_VIS | V3_VIS:
			//	v2-----------v3
			//	  \		   |
			//   ....B..........A...
			//		 \		|
			//		  v1-----v0
			ClipEdge(v1, v2, dst++, TYPE_VTX); // B
			PushVertex(v2,   dst++);           // v2
			ClipEdge(v3, v0, dst++, TYPE_VTX); // A
			PushVertex(v3,   dst++);           // v3
			break;

		case V0_VIS | V1_VIS | V2_VIS:
			//		--v1--
			//	v0--	  --v2
			//	  \		|
			//   .....B.....A...
			//		  \   |
			//			v3
			// v1,v2,v0  v2,v0,A  v0,A,B
			PushVertex(v1,   dst++);           // v1
			PushVertex(v2,   dst++);           // v2
			PushVertex(v0,   dst++);           // v0
			ClipEdge(v2, v3, dst++, TYPE_VTX); // A
			ClipEdge(v3, v0, dst++, TYPE_EOS); // B
			break;

		case V0_VIS | V1_VIS | V3_VIS:
			//		--v0--
			//	v3--	  --v1
			//	  \		|
			//   .....B.....A...
			//		  \   |
			//			v2
			// v0,v1,v3  v1,v3,A  v3,A,B
			v3->flags = PVR_CMD_VERTEX;
			PushVertex(v0,   dst++);           // v0
			PushVertex(v1,   dst++);           // v1
			PushVertex(v3,   dst++);           // v3
			ClipEdge(v1, v2, dst++, TYPE_VTX); // A
			ClipEdge(v2, v3, dst++, TYPE_EOS); // B
			break;

		case V0_VIS | V2_VIS | V3_VIS:
			//		--v3--
			//	v2--	  --v0
			//	  \		|
			//   .....B.....A...
			//		  \   |
			//			v1
			// v3,v0,v2  v0,v2,A  v2,A,B
			v3->flags = PVR_CMD_VERTEX;
			PushVertex(v3,   dst++);           // v3
			PushVertex(v0,   dst++);           // v0
			PushVertex(v2,   dst++);           // v2
			ClipEdge(v0, v1, dst++, TYPE_VTX); // A
			ClipEdge(v1, v2, dst++, TYPE_EOS); // B
			break;

		case V1_VIS | V2_VIS | V3_VIS:
			//		--v2--
			//	v1--	  --v3
			//	  \		|
			//   .....B.....A...
			//		  \   |
			//			v0
			// v2,v3,v1  v3,v1,A  v1,A,B
			v3->flags = PVR_CMD_VERTEX;
			PushVertex(v2,   dst++);           // v2
			PushVertex(v3,   dst++);           // v3
			PushVertex(v1,   dst++);           // v1
			ClipEdge(v3, v0, dst++, TYPE_VTX); // A
			ClipEdge(v0, v1, dst++, TYPE_EOS); // B
			break;
		}
	}
}
