#ifndef PRIVATE_H
#define PRIVATE_H
#include <stdint.h>

#define GLDC_FORCE_INLINE __attribute__((always_inline)) inline
#define GLDC_NO_INLINE    __attribute__((noinline))

typedef struct {
    /* Same 32 byte layout as pvr_vertex_t */
    uint32_t flags;
    float x, y, w;
    uint32_t u, v; // really floats, but stored as uint for better load/store codegen
    uint32_t bgra;
    float z; // actually oargb, but repurposed since unused
} __attribute__ ((aligned (32))) Vertex;

typedef struct {
    uint32_t format; /* This is the PVR texture format */
    void     *data;
    uint16_t width;
    uint16_t height;
} TextureObject;

void GLDC_NO_INLINE SubmitCommands(Vertex* v3, int n);

#endif // PRIVATE_H
