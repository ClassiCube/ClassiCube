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

GL_FORCE_INLINE void _glPerspectiveDivideVertex(Vertex* vertex, const float h) {
    TRACE();

    const float f = _glFastInvert(vertex->w);

    /* Convert to NDC and apply viewport */
    vertex->xyz[0] = (vertex->xyz[0] * f * 320) + 320;
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
static volatile uint32_t *PVR_LMMODE1 = (uint32_t*) 0xA05F6888;
static volatile uint32_t *QACR = (uint32_t*) 0xFF000038;

void SceneListSubmit(Vertex* v2, int n) {
    TRACE();

    /* You need at least a header, and 3 vertices to render anything */
    if(n < 4) {
        return;
    }

    const float h = vid_mode->height;

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
    uint8_t counter = 0;

    sq = SQ_BASE_ADDRESS;

    for(int i = 0; i < n; ++i, ++v2) {
        PREFETCH(v2 + 1);
        switch(v2->flags) {
        case GPU_CMD_VERTEX_EOL:
            if(counter < 2) {
                continue;
            }
            counter = 0;
            break;
        case GPU_CMD_VERTEX:
            ++counter;
            if(counter < 3) {
                continue;
            }
            break;
        default:
            _glPushHeaderOrVertex(v2);
            counter = 0;
            continue;
        };

        Vertex* const v0 = v2 - 2;
        Vertex* const v1 = v2 - 1;

        visible_mask = (
            (v0->xyz[2] > -v0->w) << 0 |
            (v1->xyz[2] > -v1->w) << 1 |
            (v2->xyz[2] > -v2->w) << 2 |
            (counter == 0) << 3
        );

        switch(visible_mask) {
        case 15: /* All visible, but final vertex in strip */
        {
            _glPerspectiveDivideVertex(v0, h);
            _glPushHeaderOrVertex(v0);

            _glPerspectiveDivideVertex(v1, h);
            _glPushHeaderOrVertex(v1);

            _glPerspectiveDivideVertex(v2, h);
            _glPushHeaderOrVertex(v2);
        }
        break;
        case 7:
            /* All visible, push the first vertex and move on */
            _glPerspectiveDivideVertex(v0, h);
            _glPushHeaderOrVertex(v0);
            break;
        case 9:
            /* First vertex was visible, last in strip */
            {
                Vertex __attribute__((aligned(32))) scratch[2];
                Vertex* a = &scratch[0];
                Vertex* b = &scratch[1];

                _glClipEdge(v0, v1, a);
                a->flags = GPU_CMD_VERTEX;

                _glClipEdge(v2, v0, b);
                b->flags = GPU_CMD_VERTEX_EOL;

                _glPerspectiveDivideVertex(v0, h);
                _glPushHeaderOrVertex(v0);

                _glPerspectiveDivideVertex(a, h);
                _glPushHeaderOrVertex(a);

                _glPerspectiveDivideVertex(b, h);
                _glPushHeaderOrVertex(b);
            }
            break;
        case 1:
            /* First vertex was visible, but not last in strip */
            {
                Vertex __attribute__((aligned(32))) scratch[2];
                Vertex* a = &scratch[0];
                Vertex* b = &scratch[1];

                _glClipEdge(v0, v1, a);
                a->flags = GPU_CMD_VERTEX;

                _glClipEdge(v2, v0, b);
                b->flags = GPU_CMD_VERTEX;

                _glPerspectiveDivideVertex(v0, h);
                _glPushHeaderOrVertex(v0);

                _glPerspectiveDivideVertex(a, h);
                _glPushHeaderOrVertex(a);

                _glPerspectiveDivideVertex(b, h);
                _glPushHeaderOrVertex(b);
                _glPushHeaderOrVertex(b);
            }
            break;
        case 10:
        case 2:
            /* Second vertex was visible. In self case we need to create a triangle and produce
                two new vertices: 1-2, and 2-3. */
            {
                Vertex __attribute__((aligned(32))) scratch[3];
                Vertex* a = &scratch[0];
                Vertex* b = &scratch[1];
                Vertex* c = &scratch[2];

                memcpy_vertex(c, v1);

                _glClipEdge(v0, v1, a);
                a->flags = GPU_CMD_VERTEX;

                _glClipEdge(v1, v2, b);
                b->flags = v2->flags;

                _glPerspectiveDivideVertex(a, h);
                _glPushHeaderOrVertex(a);

                _glPerspectiveDivideVertex(c, h);
                _glPushHeaderOrVertex(c);

                _glPerspectiveDivideVertex(b, h);
                _glPushHeaderOrVertex(b);
            }
            break;
        case 11:
        case 3:  /* First and second vertex were visible */
        {
            Vertex __attribute__((aligned(32))) scratch[3];
            Vertex* a = &scratch[0];
            Vertex* b = &scratch[1];
            Vertex* c = &scratch[2];

            memcpy_vertex(c, v1);

            _glClipEdge(v2, v0, b);
            b->flags = GPU_CMD_VERTEX;

            _glPerspectiveDivideVertex(v0, h);
            _glPushHeaderOrVertex(v0);

            _glClipEdge(v1, v2, a);
            a->flags = v2->flags;

            _glPerspectiveDivideVertex(c, h);
            _glPushHeaderOrVertex(c);

            _glPerspectiveDivideVertex(b, h);
            _glPushHeaderOrVertex(b);

            _glPerspectiveDivideVertex(a, h);
            _glPushHeaderOrVertex(c);
            _glPushHeaderOrVertex(a);
        }
        break;
        case 12:
        case 4:
            /* Third vertex was visible. */
            {
                Vertex __attribute__((aligned(32))) scratch[3];
                Vertex* a = &scratch[0];
                Vertex* b = &scratch[1];
                Vertex* c = &scratch[2];

                memcpy_vertex(c, v2);

                _glClipEdge(v2, v0, a);
                a->flags = GPU_CMD_VERTEX;

                _glClipEdge(v1, v2, b);
                b->flags = GPU_CMD_VERTEX;

                _glPerspectiveDivideVertex(a, h);
                _glPushHeaderOrVertex(a);

                if(counter % 2 == 1) {
                    _glPushHeaderOrVertex(a);
                }

                _glPerspectiveDivideVertex(b, h);
                _glPushHeaderOrVertex(b);

                _glPerspectiveDivideVertex(c, h);
                _glPushHeaderOrVertex(c);
            }
            break;
        case 13:
        {
            Vertex __attribute__((aligned(32))) scratch[3];
            Vertex* a = &scratch[0];
            Vertex* b = &scratch[1];
            Vertex* c = &scratch[2];

            memcpy_vertex(c, v2);
            c->flags = GPU_CMD_VERTEX;

            _glClipEdge(v0, v1, a);
            a->flags = GPU_CMD_VERTEX;

            _glClipEdge(v1, v2, b);
            b->flags = GPU_CMD_VERTEX;

            _glPerspectiveDivideVertex(v0, h);
            _glPushHeaderOrVertex(v0);

            _glPerspectiveDivideVertex(a, h);
            _glPushHeaderOrVertex(a);

            _glPerspectiveDivideVertex(c, h);
            _glPushHeaderOrVertex(c);
            _glPerspectiveDivideVertex(b, h);
            _glPushHeaderOrVertex(b);

            c->flags = GPU_CMD_VERTEX_EOL;
            _glPushHeaderOrVertex(c);
        }
        break;
        case 5:  /* First and third vertex were visible */
        {
            Vertex __attribute__((aligned(32))) scratch[3];
            Vertex* a = &scratch[0];
            Vertex* b = &scratch[1];
            Vertex* c = &scratch[2];

            memcpy_vertex(c, v2);
            c->flags = GPU_CMD_VERTEX;

            _glClipEdge(v0, v1, a);
            a->flags = GPU_CMD_VERTEX;

            _glClipEdge(v1, v2, b);
            b->flags = GPU_CMD_VERTEX;

            _glPerspectiveDivideVertex(v0, h);
            _glPushHeaderOrVertex(v0);

            _glPerspectiveDivideVertex(a, h);
            _glPushHeaderOrVertex(a);

            _glPerspectiveDivideVertex(c, h);
            _glPushHeaderOrVertex(c);
            _glPerspectiveDivideVertex(b, h);
            _glPushHeaderOrVertex(b);
            _glPushHeaderOrVertex(c);
        }
        break;
        case 14:
        case 6:  /* Second and third vertex were visible */
        {
            Vertex __attribute__((aligned(32))) scratch[4];
            Vertex* a = &scratch[0];
            Vertex* b = &scratch[1];
            Vertex* c = &scratch[2];
            Vertex* d = &scratch[3];

            memcpy_vertex(c, v1);
            memcpy_vertex(d, v2);

            _glClipEdge(v0, v1, a);
            a->flags = GPU_CMD_VERTEX;

            _glClipEdge(v2, v0, b);
            b->flags = GPU_CMD_VERTEX;

            _glPerspectiveDivideVertex(a, h);
            _glPushHeaderOrVertex(a);

            _glPerspectiveDivideVertex(c, h);
            _glPushHeaderOrVertex(c);

            _glPerspectiveDivideVertex(b, h);
            _glPushHeaderOrVertex(b);
            _glPushHeaderOrVertex(c);

            _glPerspectiveDivideVertex(d, h);
            _glPushHeaderOrVertex(d);
        }
        break;
        case 8:
        default:
            break;
        }
    }

    _glFlushBuffer();
}