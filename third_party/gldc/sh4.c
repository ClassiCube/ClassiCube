#include <kos.h>
#include <dc/pvr.h>
#include "gldc.h"

static GLDC_FORCE_INLINE void PushVertex(Vertex* v, volatile Vertex* dst) {
	asm volatile (
"fmov.d    @%0+, dr0\n" // LS, FX  = *src, src += 8
"fmov.d    @%0+, dr2\n" // LS, YW  = *src, src += 8
"add        #32, %1 \n" // EX, dst += 32
"fmul       fr3, fr3\n" // FE, W   = W * W 
"fmov.d    @%0+, dr4\n" // LS, UV  = *src, src += 8
"fmov.d    @%0+, dr6\n" // LS, C?  = *src, src += 8
"fsrra           fr3\n" // FE, W   = 1/sqrt(W*W) ~ 1/W
"fmov.d    dr6, @-%1\n" // LS, dst -= 8, *dst = C?
"fmov.d    dr4, @-%1\n" // LS, dst -= 8, *dst = UV
"fmul      fr3,  fr2\n" // FE, Y = W * Y
"add      #-32,  %0 \n" // EX, src -= 32
"fmov.d    dr2, @-%1\n" // LS, dst -= 8, *dst = YW
"fmul      fr3,  fr1\n" // FE, Y = X * X
"fmov.d    dr0, @-%1\n" // LS, dst -= 8, *dst = FX
"pref      @%1      \n" // LS, flush store queue
  :
  : "r" (v), "r" (dst)
  : "memory", "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7"
  );
}

static inline void PushCommand(Vertex* v, volatile Vertex* dst)  {
	asm volatile (
"add       #32, %1  \n" // EX, dst += 32
"fmov.d    @%0+, dr0\n" // LS, fr0_fr1 = *src, src += 8
"fmov.d    @%0+, dr2\n" // LS, fr2_fr3 = *src, src += 8
"fmov.d    @%0+, dr4\n" // LS, fr4_fr5 = *src, src += 8
"fmov.d    @%0+, dr6\n" // LS, fr6_fr7 = *src, src += 8
"fmov.d    dr6, @-%1\n" // LS, dst -= 8, *dst = fr6_fr7
"fmov.d    dr4, @-%1\n" // LS, dst -= 8, *dst = fr4_fr5
"fmov.d    dr2, @-%1\n" // LS, dst -= 8, *dst = fr2_fr3
"fmov.d    dr0, @-%1\n" // LS, dst -= 8, *dst = fr0_fr1
"pref      @%1      \n" // LS, flush store queue
"add       #-32, %0 \n" // EX, src -= 32
  :
  : "r" (v), "r" (dst)
  : "memory", "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7"
  );
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
	asm volatile ("fschg"); // swap to 64 bit loads/stores

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
	asm volatile ("fschg"); // swap back to 32 bit loads/stores
}
