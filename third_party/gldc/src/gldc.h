#ifndef PRIVATE_H
#define PRIVATE_H
#include <stdint.h>

#define GLenum     unsigned int
#define GLboolean  unsigned char

typedef struct {
    /* Same 32 byte layout as pvr_vertex_t */
    uint32_t flags;
    float x, y, z;
    uint32_t u, v; // really floats, but stored as uint for better load/store codegen
    uint32_t bgra;
    float w; // actually oargb, but repurposed since unused
} __attribute__ ((aligned (32))) Vertex;

typedef struct {
    uint32_t color; /* This is the PVR texture format */
    void     *data;
    uint16_t width;
    uint16_t height;
} TextureObject;

extern TextureObject* TEXTURE_ACTIVE;
extern GLboolean TEXTURES_ENABLED;

extern GLboolean DEPTH_TEST_ENABLED;
extern GLboolean DEPTH_MASK_ENABLED;

extern GLboolean CULLING_ENABLED;

extern GLboolean FOG_ENABLED;
extern GLboolean ALPHA_TEST_ENABLED;
extern GLboolean BLEND_ENABLED;

extern GLboolean SCISSOR_TEST_ENABLED;
extern GLenum SHADE_MODEL;
extern GLboolean AUTOSORT_ENABLED;

void SceneListSubmit(Vertex* v2, int n);

#endif // PRIVATE_H
