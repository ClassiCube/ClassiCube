#include <math.h>
#include <kos.h>
#include <dc/pvr.h>
#include "gldc.h"
#include "sh4_math.h"

#define CLIP_DEBUG 0

#define SQ_BASE_ADDRESS (void*) 0xe0000000
#define PREFETCH(addr) __builtin_prefetch((addr))

GL_FORCE_INLINE float _glFastInvert(float x) {
    return MATH_fsrra(x * x);
}

GL_FORCE_INLINE void _glPerspectiveDivideVertex(Vertex* vertex) {
    const float f = _glFastInvert(vertex->w);

    /* Convert to NDC and apply viewport */
    vertex->x = (vertex->x * f * VIEWPORT.hwidth)  + VIEWPORT.x_plus_hwidth;
    vertex->y = (vertex->y * f * VIEWPORT.hheight) + VIEWPORT.y_plus_hheight;
    vertex->z = f;
}


volatile uint32_t *sq = SQ_BASE_ADDRESS;

static inline void _glFlushBuffer() {
    TRACE();

    /* Wait for both store queues to complete */
    sq = (uint32_t*) 0xe0000000;
    sq[0] = sq[8] = 0;
}

static inline void _glPushHeaderOrVertex(Vertex* v)  {
    TRACE();

    uint32_t* s = (uint32_t*) v;
    sq[0] = *(s++);
    sq[1] = *(s++);
    sq[2] = *(s++);
    sq[3] = *(s++);
    sq[4] = *(s++);
    sq[5] = *(s++);
    sq[6] = *(s++);
    sq[7] = *(s++);
    __asm__("pref @%0" : : "r"(sq));
    sq += 8;
}

static void ClipEdge(const Vertex* const v1, const Vertex* const v2, Vertex* vout) {
    float d0 = v1->z;
    float d1 = v2->z;
    float t = (fabsf(d0) * MATH_fsrra((d1 - d0) * (d1 - d0))) + 0.000001f;
    float invt = 1.0f - t;

    vout->x = invt * v1->x + t * v2->x;
    vout->y = invt * v1->y + t * v2->y;
    vout->z = 0.0f; // clipped against near plane, where Z = 0 anyways

    vout->u = invt * v1->u + t * v2->u;
    vout->v = invt * v1->v + t * v2->v;
    vout->w = invt * v1->w + t * v2->w;

    vout->bgra[0] = invt * v1->bgra[0] + t * v2->bgra[0];
    vout->bgra[1] = invt * v1->bgra[1] + t * v2->bgra[1];
    vout->bgra[2] = invt * v1->bgra[2] + t * v2->bgra[2];
    vout->bgra[3] = invt * v1->bgra[3] + t * v2->bgra[3];
}

#define SPAN_SORT_CFG 0x005F8030
static volatile uint32_t* PVR_LMMODE0 = (uint32_t*) 0xA05F6884;
static volatile uint32_t* PVR_LMMODE1 = (uint32_t*) 0xA05F6888;
static volatile uint32_t* QACR = (uint32_t*) 0xFF000038;

#define V0_VIS (1 << 0)
#define V1_VIS (1 << 1)
#define V2_VIS (1 << 2)
#define V3_VIS (1 << 3)


// https://casual-effects.com/research/McGuire2011Clipping/clip.glsl
static void SubmitClipped(Vertex* v0, Vertex* v1, Vertex* v2, Vertex* v3, uint8_t visible_mask) {
    Vertex __attribute__((aligned(32))) scratch[4];
    Vertex* a = &scratch[0];
    Vertex* b = &scratch[1];

    switch(visible_mask) {
    case V0_VIS:
    {
        //          v0
        //         / |
        //       /   |
        // .....A....B...
        //    /      |
        //  v3--v2---v1
        ClipEdge(v3, v0, a);
        a->flags = PVR_CMD_VERTEX_EOL;
        ClipEdge(v0, v1, b);
        b->flags = PVR_CMD_VERTEX;

        _glPerspectiveDivideVertex(v0);
        _glPushHeaderOrVertex(v0);

        _glPerspectiveDivideVertex(b);
        _glPushHeaderOrVertex(b);

        _glPerspectiveDivideVertex(a);
        _glPushHeaderOrVertex(a);
    }
    break;
    case V1_VIS:
    {
        //          v1
        //         / |
        //       /   |
        // ....A.....B...
        //    /      |
        //  v0--v3---v2
        ClipEdge(v0, v1, a);
        a->flags = PVR_CMD_VERTEX;
        ClipEdge(v1, v2, b);
        b->flags = PVR_CMD_VERTEX_EOL;

        _glPerspectiveDivideVertex(a);
        _glPushHeaderOrVertex(a);

        _glPerspectiveDivideVertex(v1);
        _glPushHeaderOrVertex(v1);

        _glPerspectiveDivideVertex(b);
        _glPushHeaderOrVertex(b);
    } break;
    case V2_VIS:
    {
        //          v2
        //         / |
        //       /   |
        // ....A.....B...
        //    /      |
        //  v1--v0---v3

        ClipEdge(v1, v2, a);
        a->flags = PVR_CMD_VERTEX;
        ClipEdge(v2, v3, b);
        b->flags = PVR_CMD_VERTEX_EOL;

        _glPerspectiveDivideVertex(a);
        _glPushHeaderOrVertex(a);

        _glPerspectiveDivideVertex(v2);
        _glPushHeaderOrVertex(v2);

        _glPerspectiveDivideVertex(b);
        _glPushHeaderOrVertex(b);
    } break;
    case V3_VIS:
    {
        //          v3
        //         / |
        //       /   |
        // ....A.....B...
        //    /      |
        //  v2--v1---v0
        ClipEdge(v2, v3, a);
        a->flags = PVR_CMD_VERTEX;
        ClipEdge(v3, v0, b);
        b->flags = PVR_CMD_VERTEX;

        _glPerspectiveDivideVertex(b);
        _glPushHeaderOrVertex(b);

        _glPerspectiveDivideVertex(a);
        _glPushHeaderOrVertex(a);

        _glPerspectiveDivideVertex(v3);
        _glPushHeaderOrVertex(v3);
    }
    break;
    case V0_VIS | V1_VIS:
    {
        //    v0-----------v1
        //      \           |
        //   ....B..........A...
        //         \        |
        //          v3-----v2
        ClipEdge(v1, v2, a);
        a->flags = PVR_CMD_VERTEX;
        ClipEdge(v3, v0, b);
        b->flags = PVR_CMD_VERTEX_EOL;

        _glPerspectiveDivideVertex(v1);
        _glPushHeaderOrVertex(v1);

        _glPerspectiveDivideVertex(a);
        _glPushHeaderOrVertex(a);

        _glPerspectiveDivideVertex(v0);
        _glPushHeaderOrVertex(v0);

        _glPerspectiveDivideVertex(b);
        _glPushHeaderOrVertex(b);
    } break;
    // case V0_VIS | V2_VIS: degenerate case that should never happen
    case V0_VIS | V3_VIS:
    {
        //    v3-----------v0
        //      \           |
        //   ....B..........A...
        //         \        |
        //          v2-----v1
        ClipEdge(v0, v1, a);
        a->flags = PVR_CMD_VERTEX;
        ClipEdge(v2, v3, b);
        b->flags = PVR_CMD_VERTEX;

        _glPerspectiveDivideVertex(a);
        _glPushHeaderOrVertex(a);

        _glPerspectiveDivideVertex(b);
        _glPushHeaderOrVertex(b);

        _glPerspectiveDivideVertex(v0);
        _glPushHeaderOrVertex(v0);

        _glPerspectiveDivideVertex(v3);
        _glPushHeaderOrVertex(v3);
    } break;
    case V1_VIS | V2_VIS:
    {
        //    v1-----------v2
        //      \           |
        //   ....B..........A...
        //         \        |
        //          v0-----v3
        ClipEdge(v2, v3, a);
        a->flags = PVR_CMD_VERTEX_EOL;
        ClipEdge(v0, v1, b);
        b->flags = PVR_CMD_VERTEX;

        _glPerspectiveDivideVertex(v1);
        _glPushHeaderOrVertex(v1);

        _glPerspectiveDivideVertex(v2);
        _glPushHeaderOrVertex(v2);

        _glPerspectiveDivideVertex(b);
        _glPushHeaderOrVertex(b);

        _glPerspectiveDivideVertex(a);
        _glPushHeaderOrVertex(a);
    } break;
    // case V1_VIS | V3_VIS: degenerate case that should never happen
    case V2_VIS | V3_VIS:
    {
        //    v2-----------v3
        //      \           |
        //   ....B..........A...
        //         \        |
        //          v1-----v0
        ClipEdge(v3, v0, a);
        a->flags = PVR_CMD_VERTEX;
        ClipEdge(v1, v2, b);
        b->flags = PVR_CMD_VERTEX;

        _glPerspectiveDivideVertex(b);
        _glPushHeaderOrVertex(b);

        _glPerspectiveDivideVertex(v2);
        _glPushHeaderOrVertex(v2);

        _glPerspectiveDivideVertex(a);
        _glPushHeaderOrVertex(a);

        _glPerspectiveDivideVertex(v3);
        _glPushHeaderOrVertex(v3);
    } break;
    case V0_VIS | V1_VIS | V2_VIS:
    {
        //        --v1--
        //    v0--      --v2
        //      \        |
        //   .....B.....A...
        //          \   |
        //            v3
        // v1,v2,v0  v2,v0,A  v0,A,B
        ClipEdge(v2, v3, a);
        a->flags = PVR_CMD_VERTEX;
        ClipEdge(v3, v0, b);
        b->flags = PVR_CMD_VERTEX_EOL;

        _glPerspectiveDivideVertex(v1);
        _glPushHeaderOrVertex(v1);

        _glPerspectiveDivideVertex(v2);
        _glPushHeaderOrVertex(v2);

        _glPerspectiveDivideVertex(v0);
        _glPushHeaderOrVertex(v0);

        _glPerspectiveDivideVertex(a);
        _glPushHeaderOrVertex(a);

        _glPerspectiveDivideVertex(b);
        _glPushHeaderOrVertex(b);
    } break;
    case V0_VIS | V1_VIS | V3_VIS:
    {
        //        --v0--
        //    v3--      --v1
        //      \        |
        //   .....B.....A...
        //          \   |
        //            v2
        // v0,v1,v3  v1,v3,A  v3,A,B
        ClipEdge(v1, v2, a);
        a->flags  = PVR_CMD_VERTEX;
        ClipEdge(v2, v3, b);
        b->flags  = PVR_CMD_VERTEX_EOL;
        v3->flags = PVR_CMD_VERTEX;

        _glPerspectiveDivideVertex(v0);
        _glPushHeaderOrVertex(v0);

        _glPerspectiveDivideVertex(v1);
        _glPushHeaderOrVertex(v1);

        _glPerspectiveDivideVertex(v3);
        _glPushHeaderOrVertex(v3);

        _glPerspectiveDivideVertex(a);
        _glPushHeaderOrVertex(a);

        _glPerspectiveDivideVertex(b);
        _glPushHeaderOrVertex(b);
    } break;
    case V0_VIS | V2_VIS | V3_VIS:
    {
        //        --v3--
        //    v2--      --v0
        //      \        |
        //   .....B.....A...
        //          \   |
        //            v1
        // v3,v0,v2  v0,v2,A  v2,A,B
        ClipEdge(v0, v1, a);
        a->flags  = PVR_CMD_VERTEX;
        ClipEdge(v1, v2, b);
        b->flags  = PVR_CMD_VERTEX_EOL;
        v3->flags = PVR_CMD_VERTEX;

        _glPerspectiveDivideVertex(v3);
        _glPushHeaderOrVertex(v3);

        _glPerspectiveDivideVertex(v0);
        _glPushHeaderOrVertex(v0);

        _glPerspectiveDivideVertex(v2);
        _glPushHeaderOrVertex(v2);

        _glPerspectiveDivideVertex(a);
        _glPushHeaderOrVertex(a);

        _glPerspectiveDivideVertex(b);
        _glPushHeaderOrVertex(b);
    } break;
    case V1_VIS | V2_VIS | V3_VIS:
    {
        //        --v2--
        //    v1--      --v3
        //      \        |
        //   .....B.....A...
        //          \   |
        //            v0
        // v2,v3,v1  v3,v1,A  v1,A,B
        ClipEdge(v3, v0, a);
        a->flags  = PVR_CMD_VERTEX;
        ClipEdge(v0, v1, b);
        b->flags  = PVR_CMD_VERTEX_EOL;
        v3->flags = PVR_CMD_VERTEX;

        _glPerspectiveDivideVertex(v2);
        _glPushHeaderOrVertex(v2);

        _glPerspectiveDivideVertex(v3);
        _glPushHeaderOrVertex(v3);

        _glPerspectiveDivideVertex(v1);
        _glPushHeaderOrVertex(v1);

        _glPerspectiveDivideVertex(a);
        _glPushHeaderOrVertex(a);

        _glPerspectiveDivideVertex(b);
        _glPushHeaderOrVertex(b);
    } break;
    }
}

void SceneListSubmit(Vertex* v3, int n) {
    TRACE();
    /* You need at least a header, and 3 vertices to render anything */
    if(n < 4) return;

    PVR_SET(SPAN_SORT_CFG, 0x0);

    //Set PVR DMA registers
    *PVR_LMMODE0 = 0;
    *PVR_LMMODE1 = 0;

    //Set QACR registers
	QACR[1] = QACR[0] = 0x11;

#if CLIP_DEBUG
    Vertex* vertex = (Vertex*) src;
    for(int i = 0; i < n; ++i) {
        fprintf(stderr, "{%f, %f, %f, %f}, // %x (%x)\n", vertex[i].xyz[0], vertex[i].xyz[1], vertex[i].xyz[2], vertex[i].w, vertex[i].flags, &vertex[i]);
    }

    fprintf(stderr, "----\n");
#endif
    uint8_t visible_mask = 0;

    sq = SQ_BASE_ADDRESS;

    for(int i = 0; i < n; ++i, ++v3) {
        PREFETCH(v3 + 1);
        switch(v3->flags & 0xFF000000) {
        case PVR_CMD_VERTEX_EOL:
            break;
        case PVR_CMD_VERTEX:
            continue;
        default:
            _glPushHeaderOrVertex(v3);
            continue;
        };

    // Quads [0, 1, 2, 3] -> Triangles [{0, 1, 2}  {2, 3, 0}]
        Vertex* const v0 = v3 - 3;
        Vertex* const v1 = v3 - 2;
        Vertex* const v2 = v3 - 1;

        visible_mask = v3->flags & 0xFF;
        v3->flags &= ~0xFF;
        
        // Stats gathering found that when testing a 64x64x64 sized world, at most
        //   ~400-500 triangles needed clipping
        //   ~13% of the triangles in a frame needed clipping (percentage increased when less triangles overall)
        // Based on this, the decision was made to optimise for rendering quads there 
        //  were either entirely visible or entirely culled, at the expensive at making
        //  partially visible quads a bit slower due to needing to be split into two triangles first
        // Performance measuring indicated that overall FPS improved from this change
        //  to switching to try to process 1 quad instead of 2 triangles though

        switch(visible_mask) {
        case V0_VIS | V1_VIS | V2_VIS | V3_VIS: // All vertices visible
        {
            // Triangle strip: {1,2,0} {2,0,3}
            _glPerspectiveDivideVertex(v1);
            _glPushHeaderOrVertex(v1);

            _glPerspectiveDivideVertex(v2);
            _glPushHeaderOrVertex(v2);

            _glPerspectiveDivideVertex(v0);
            _glPushHeaderOrVertex(v0);

            _glPerspectiveDivideVertex(v3);
            _glPushHeaderOrVertex(v3);
        }
        break;
        
        default: // Some vertices visible
            SubmitClipped(v0, v1, v2, v3, visible_mask);
            break;
        }
    }

    _glFlushBuffer();
}
