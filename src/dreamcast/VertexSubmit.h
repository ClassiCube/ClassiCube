typedef struct {
    // Same layout as a PVR vertex
    uint32_t flags;
    float x, y, w;
    uint32_t u, v; // really floats, but stored as uint for better load/store codegen
    uint32_t bgra;
    float z; // actually oargb, but repurposed since unused
} __attribute__ ((aligned (32))) Vertex;

void CC_NOINLINE SubmitCommands(Vertex* v3, int n);

#define PushVertex(src, dst) __asm__ volatile ( \
"fmov.d    @%1+, dr0 ! LS, FX  = *src, src += 8\n"    \
"fmov.d    @%1+, dr2 ! LS, YW  = *src, src += 8\n"    \
"add        #64, %0  ! EX, dst += 64\n"               \
"fmul       fr3, fr3 ! FE, W   = W * W\n"             \
"fmov.d    @%1+, dr4 ! LS, UV  = *src, src += 8\n"    \
"fmov.d    @%1+, dr6 ! LS, C?  = *src, src += 8\n"    \
"fsrra           fr3 ! FE, W   = 1/sqrt(W*W) ~ 1/W\n" \
"fmov.d    dr6, @-%0 ! LS, dst -= 8, *dst = C?\n"     \
"fmov.d    dr4, @-%0 ! LS, dst -= 8, *dst = UV\n"     \
"fmul      fr3,  fr2 ! FE, Y = W * Y\n"               \
"add      #-32,  %1  ! EX, src -= 32\n"               \
"fmov.d    dr2, @-%0 ! LS, dst -= 8, *dst = YW\n"     \
"fmul      fr3,  fr1 ! FE, Y = X * X\n"               \
"fmov.d    dr0, @-%0 ! LS, dst -= 8, *dst = FX\n"     \
"pref      @%0       ! LS, flush store queue\n"       \
  : "+r" (dst) \
  : "r"  (src) \
  : "memory", "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7" \
  );

#define PushCommand(src, dst) __asm__ volatile ( \
"fmov.d    @%1+, dr0 ! LS, D01  = *src, src += 8\n" \
"fmov.d    @%1+, dr2 ! LS, D23  = *src, src += 8\n" \
"fmov.d    @%1+, dr4 ! LS, D45  = *src, src += 8\n" \
"fmov.d    @%1+, dr6 ! LS, D67  = *src, src += 8\n" \
"add        #64, %0  ! EX, dst += 64\n"             \
"fmov.d    dr6, @-%0 ! LS, dst -= 8, *dst = D67\n"  \
"fmov.d    dr4, @-%0 ! LS, dst -= 8, *dst = D45\n"  \
"fmov.d    dr2, @-%0 ! LS, dst -= 8, *dst = D23\n"  \
"fmov.d    dr0, @-%0 ! LS, dst -= 8, *dst = D01\n"  \
"pref      @%0       ! LS, flush store queue\n"     \
"add      #-32,  %1  ! EX, src -= 32\n"             \
  : "+r" (dst) \
  : "r"  (src) \
  : "memory", "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7" \
  );

extern void ClipEdge(Vertex* const v1, Vertex* const v2, volatile Vertex* vout, char type);

#define V0_VIS (1 << 0)
#define V1_VIS (1 << 1)
#define V2_VIS (1 << 2)
#define V3_VIS (1 << 3)

#define TYPE_VTX 0xE0 // PVR vertex, data
#define TYPE_EOS 0xF0 // PVR vertex, end of strip

extern void ProcessVertexList(Vertex* v3, int n, void* sq_addr);

void SubmitCommands(Vertex* v3, int n) {
	volatile Vertex* dst = (volatile Vertex*)MEM_AREA_SQ_BASE;
    Vertex* vEnd = v3 + n;
	asm volatile ("fschg"); // swap to 64 bit loads/stores

	for (; v3 < vEnd; v3++) 
	{
		// Preload next vertex into memory
		__builtin_prefetch(v3 + 1);

		switch(v3->flags & 0xFF000000) {
		case PVR_CMD_VERTEX_EOL:
			break;
		case PVR_CMD_VERTEX:
			continue;
		default:
			PushCommand(v3, dst);
			continue;
		};

		// Quads [0, 1, 2, 3] -> Triangles [{0, 1, 2}  {2, 3, 0}]
		Vertex* v0 = v3 - 3;
		Vertex* v1 = v3 - 2;
		Vertex* v2 = v3 - 1;
		uint8_t mask = v3->flags & 0xFF;

		// Check if all vertices visible
		if (__builtin_expect(mask == (V0_VIS | V1_VIS | V2_VIS | V3_VIS), 1)) {
			// Triangle strip: {1,2,0} {2,0,3}
			PushVertex(v1, dst);
			PushVertex(v2, dst);
			PushVertex(v0, dst);
			PushVertex(v3, dst);
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
			PushVertex(v0,   dst);             // v0
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
			PushVertex(v1,   dst);             // v1
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
			PushVertex(v2,   dst);	           // v2
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
			ClipEdge(v2, v3, dst++, TYPE_VTX); // A
			PushVertex(v3,   dst);             // v3
			break;

		case V0_VIS | V1_VIS:
			//	v0-----------v1
			//	  \		   |
			//   ....B..........A...
			//		 \		|
			//		  v3-----v2
			PushVertex(v1,   dst);             // v1
			ClipEdge(v1, v2, dst++, TYPE_VTX); // A
			PushVertex(v0,   dst);             // v0
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
			PushVertex(v0,   dst);             // v0
			PushVertex(v3,   dst);             // v3
			break;

		case V1_VIS | V2_VIS:
			//	v1-----------v2
			//	  \		   |
			//   ....B..........A...
			//		 \		|
			//		  v0-----v3
			PushVertex(v1,   dst);             // v1
			PushVertex(v2,   dst);             // v2
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
			PushVertex(v2,   dst);             // v2
			ClipEdge(v3, v0, dst++, TYPE_VTX); // A
			PushVertex(v3,   dst);             // v3
			break;

		case V0_VIS | V1_VIS | V2_VIS:
			//		--v1--
			//	v0--	  --v2
			//	  \		|
			//   .....B.....A...
			//		  \   |
			//			v3
			// v1,v2,v0  v2,v0,A  v0,A,B
			PushVertex(v1,   dst);             // v1
			PushVertex(v2,   dst);             // v2
			PushVertex(v0,   dst);             // v0
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
			PushVertex(v0,   dst);             // v0
			PushVertex(v1,   dst);             // v1
			PushVertex(v3,   dst);             // v3
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
			PushVertex(v3,   dst);             // v3
			PushVertex(v0,   dst);             // v0
			PushVertex(v2,   dst);             // v2
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
			PushVertex(v2,   dst);             // v2
			PushVertex(v3,   dst);             // v3
			PushVertex(v1,   dst);             // v1
			ClipEdge(v3, v0, dst++, TYPE_VTX); // A
			ClipEdge(v0, v1, dst++, TYPE_EOS); // B
			break;
		}
	}
	asm volatile ("fschg"); // swap back to 32 bit loads/stores
}
