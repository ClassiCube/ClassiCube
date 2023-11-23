#include "platform.h"
#include "sh4.h"

#define CLIP_DEBUG 0

#define PVR_VERTEX_BUF_SIZE 2560 * 256

#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

#define SQ_BASE_ADDRESS (void*) 0xe0000000

void InitGPU(_Bool autosort, _Bool fsaa) {
    pvr_init_params_t params = {
        /* Enable opaque and translucent polygons with size 32 and 32 */
        {PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_32},
        PVR_VERTEX_BUF_SIZE, /* Vertex buffer size */
        0, /* No DMA */
        fsaa, /* No FSAA */
        (autosort) ? 0 : 1 /* Disable translucent auto-sorting to match traditional GL */
    };

    pvr_init(&params);

    /* If we're PAL and we're NOT VGA, then use 50hz by default. This is the safest
    thing to do. If someone wants to force 60hz then they can call vid_set_mode later and hopefully
    that'll work... */

    int cable = vid_check_cable();
    int region = flashrom_get_region();

    if(region == FLASHROM_REGION_EUROPE && cable != CT_VGA) {
        printf("PAL region without VGA - enabling 50hz");
        vid_set_mode(DM_640x480_PAL_IL, PM_RGB565);
    }
}

GL_FORCE_INLINE float _glFastInvert(float x) {
    return MATH_fsrra(x * x);
}

GL_FORCE_INLINE void _glPerspectiveDivideVertex(Vertex* vertex) {
    TRACE();

    const float f = _glFastInvert(vertex->w);

    /* Convert to NDC and apply viewport */
    vertex->xyz[0] = (vertex->xyz[0] * f *  320) + 320;
    vertex->xyz[1] = (vertex->xyz[1] * f * -240) + 240;

    /* Orthographic projections need to use invZ otherwise we lose
    the depth information. As w == 1, and clip-space range is -w to +w
    we add 1.0 to the Z to bring it into range. We add a little extra to
    avoid a divide by zero.
    */
    if(vertex->w == 1.0f) {
        vertex->xyz[2] = _glFastInvert(1.0001f + vertex->xyz[2]);
    } else {
        vertex->xyz[2] = f;
    }
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

static inline void _glClipEdge(const Vertex* const v1, const Vertex* const v2, Vertex* vout) {
    const static float o = 0.003921569f;  // 1 / 255
    const float d0 = v1->w + v1->xyz[2];
    const float d1 = v2->w + v2->xyz[2];
    const float t = (fabs(d0) * MATH_fsrra((d1 - d0) * (d1 - d0))) + 0.000001f;
    const float invt = 1.0f - t;

    vout->xyz[0] = invt * v1->xyz[0] + t * v2->xyz[0];
    vout->xyz[1] = invt * v1->xyz[1] + t * v2->xyz[1];
    vout->xyz[2] = invt * v1->xyz[2] + t * v2->xyz[2];

    vout->uv[0] = invt * v1->uv[0] + t * v2->uv[0];
    vout->uv[1] = invt * v1->uv[1] + t * v2->uv[1];

    vout->w = invt * v1->w + t * v2->w;

    const float m = 255 * t;
    const float n = 255 - m;

    vout->bgra[0] = (v1->bgra[0] * n + v2->bgra[0] * m) * o;
    vout->bgra[1] = (v1->bgra[1] * n + v2->bgra[1] * m) * o;
    vout->bgra[2] = (v1->bgra[2] * n + v2->bgra[2] * m) * o;
    vout->bgra[3] = (v1->bgra[3] * n + v2->bgra[3] * m) * o;
}

#define SPAN_SORT_CFG 0x005F8030
static volatile uint32_t* PVR_LMMODE0 = (uint32_t*) 0xA05F6884;
static volatile uint32_t* PVR_LMMODE1 = (uint32_t*) 0xA05F6888;
static volatile uint32_t* QACR = (uint32_t*) 0xFF000038;

#define V0_VIS (1 << 0)
#define V1_VIS (1 << 1)
#define V2_VIS (1 << 2)
#define V3_VIS (1 << 3)

static void SubmitTriangle(Vertex* v0, Vertex* v1, Vertex* v2, uint8_t visible_mask) {
    Vertex __attribute__((aligned(32))) scratch[4];

    switch(visible_mask) {
    case V0_VIS | V1_VIS | V2_VIS: // All vertices visible
    {
        _glPerspectiveDivideVertex(v0);
        _glPushHeaderOrVertex(v0);

        _glPerspectiveDivideVertex(v1);
        _glPushHeaderOrVertex(v1);

        _glPerspectiveDivideVertex(v2);
        _glPushHeaderOrVertex(v2);
    }
    break;
    case V0_VIS: // First vertex was visible
    {
        Vertex* a = &scratch[0];
        Vertex* b = &scratch[1];

        _glClipEdge(v0, v1, a);
        a->flags = GPU_CMD_VERTEX;

        _glClipEdge(v2, v0, b);
        b->flags = GPU_CMD_VERTEX_EOL;

        _glPerspectiveDivideVertex(v0);
        _glPushHeaderOrVertex(v0);

        _glPerspectiveDivideVertex(a);
        _glPushHeaderOrVertex(a);

        _glPerspectiveDivideVertex(b);
        _glPushHeaderOrVertex(b);
    }
    break;
    case V1_VIS: // Second vertex was visible
    {
        /* Second vertex was visible. In self case we need to create a triangle and produce
            two new vertices: 1-2, and 2-3. */
        Vertex* a = &scratch[0];
        Vertex* b = &scratch[1];
        Vertex* c = &scratch[2];

        memcpy_vertex(c, v1);

        _glClipEdge(v0, v1, a);
        a->flags = GPU_CMD_VERTEX;

        _glClipEdge(v1, v2, b);
        b->flags = GPU_CMD_VERTEX_EOL;

        _glPerspectiveDivideVertex(a);
        _glPushHeaderOrVertex(a);

        _glPerspectiveDivideVertex(c);
        _glPushHeaderOrVertex(c);

        _glPerspectiveDivideVertex(b);
        _glPushHeaderOrVertex(b);
    }
    break;
    case V0_VIS | V1_VIS: // First and second vertex were visible
    {
        Vertex* a = &scratch[0];
        Vertex* b = &scratch[1];
        Vertex* c = &scratch[2];

        memcpy_vertex(c, v1);

        _glClipEdge(v2, v0, b);
        b->flags = GPU_CMD_VERTEX;

        _glPerspectiveDivideVertex(v0);
        _glPushHeaderOrVertex(v0);

        _glClipEdge(v1, v2, a);
        a->flags = GPU_CMD_VERTEX_EOL;

        _glPerspectiveDivideVertex(c);
        _glPushHeaderOrVertex(c);

        _glPerspectiveDivideVertex(b);
        _glPushHeaderOrVertex(b);

        _glPerspectiveDivideVertex(a);
        _glPushHeaderOrVertex(c);
        _glPushHeaderOrVertex(a);
    }
    break;
    case V2_VIS: // Third vertex was visible
    {
        Vertex* a = &scratch[0];
        Vertex* b = &scratch[1];
        Vertex* c = &scratch[2];

        memcpy_vertex(c, v2);

        _glClipEdge(v2, v0, a);
        a->flags = GPU_CMD_VERTEX;

        _glClipEdge(v1, v2, b);
        b->flags = GPU_CMD_VERTEX;

        _glPerspectiveDivideVertex(a);
        _glPushHeaderOrVertex(a);

        _glPerspectiveDivideVertex(b);
        _glPushHeaderOrVertex(b);

        _glPerspectiveDivideVertex(c);
        _glPushHeaderOrVertex(c);
    }
    break;
    case V0_VIS | V2_VIS: // First and third vertex were visible
    {
        Vertex* a = &scratch[0];
        Vertex* b = &scratch[1];
        Vertex* c = &scratch[2];

        memcpy_vertex(c, v2);
        c->flags = GPU_CMD_VERTEX;

        _glClipEdge(v0, v1, a);
        a->flags = GPU_CMD_VERTEX;

        _glClipEdge(v1, v2, b);
        b->flags = GPU_CMD_VERTEX;

        _glPerspectiveDivideVertex(v0);
        _glPushHeaderOrVertex(v0);

        _glPerspectiveDivideVertex(a);
        _glPushHeaderOrVertex(a);

        _glPerspectiveDivideVertex(c);
        _glPushHeaderOrVertex(c);
        _glPerspectiveDivideVertex(b);
        _glPushHeaderOrVertex(b);

        c->flags = GPU_CMD_VERTEX_EOL;
        _glPushHeaderOrVertex(c);
    }
    break;
    case V1_VIS | V2_VIS: // Second and third vertex were visible
    {
        Vertex* a = &scratch[0];
        Vertex* b = &scratch[1];
        Vertex* c = &scratch[2];

        memcpy_vertex(c, v1);

        _glClipEdge(v0, v1, a);
        a->flags = GPU_CMD_VERTEX;

        _glClipEdge(v2, v0, b);
        b->flags = GPU_CMD_VERTEX;

        _glPerspectiveDivideVertex(a);
        _glPushHeaderOrVertex(a);

        _glPerspectiveDivideVertex(c);
        _glPushHeaderOrVertex(c);

        _glPerspectiveDivideVertex(b);
        _glPushHeaderOrVertex(b);
        _glPushHeaderOrVertex(c);

        _glPerspectiveDivideVertex(v2);
        _glPushHeaderOrVertex(v2);
    }
    break;
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
        switch(v3->flags) {
        case GPU_CMD_VERTEX_EOL:
            break;
        case GPU_CMD_VERTEX:
            continue;
        default:
            _glPushHeaderOrVertex(v3);
            continue;
        };

	// Quads [0, 1, 2, 3] -> Triangles [{0, 1, 2}  {2, 3, 0}]
        Vertex* const v0 = v3 - 3;
        Vertex* const v1 = v3 - 2;
        Vertex* const v2 = v3 - 1;

        visible_mask = (
            (v0->xyz[2] > -v0->w) << 0 |
            (v1->xyz[2] > -v1->w) << 1 |
            (v2->xyz[2] > -v2->w) << 2 | 
            (v3->xyz[2] > -v3->w) << 3
        );
        
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
            _glPerspectiveDivideVertex(v0);
            _glPushHeaderOrVertex(v0);

            _glPerspectiveDivideVertex(v1);
            _glPushHeaderOrVertex(v1);

            v2->flags = GPU_CMD_VERTEX_EOL;
            _glPerspectiveDivideVertex(v2);
            _glPushHeaderOrVertex(v2);
            
            
            v2->flags = GPU_CMD_VERTEX;
            _glPushHeaderOrVertex(v2);

            v3->flags = GPU_CMD_VERTEX;
            _glPerspectiveDivideVertex(v3);
            _glPushHeaderOrVertex(v3);
            
            v0->flags = GPU_CMD_VERTEX_EOL;
            _glPushHeaderOrVertex(v0);
        }
        break;
        
        case 0: // No vertices visible
        break;
        
        default: // Some vertices visible
        {
            // vertices are modified in SubmitTriangle, so need to copy them
            Vertex __attribute__((aligned(32))) scratch[4];
            Vertex* a0 = &scratch[0];
            Vertex* a2 = &scratch[1];
            memcpy_vertex(a0, v0);
            memcpy_vertex(a2, v2);
            
            visible_mask &= (V0_VIS | V1_VIS | V2_VIS);       
            v2->flags = GPU_CMD_VERTEX_EOL;
            SubmitTriangle(v0, v1, v2, visible_mask);
            
            visible_mask = (
                (a2->xyz[2] > -a2->w) << 0 |
                (v3->xyz[2] > -v3->w) << 1 |
                (a0->xyz[2] > -a0->w) << 2
            );         
            v3->flags = GPU_CMD_VERTEX;
            a0->flags = GPU_CMD_VERTEX_EOL;
            SubmitTriangle(a2, v3, a0, visible_mask);
        }
        break;
        }
    }

    _glFlushBuffer();
}