#ifndef PRIVATE_H
#define PRIVATE_H
#include <stdint.h>

#define GLenum     unsigned int
#define GLboolean  unsigned char

typedef struct {
    /* Same 32 byte layout as pvr_vertex_t */
    uint32_t flags;
    float x, y, z;
    float u, v;
    uint32_t bgra;

    /* In the pvr_vertex_t structure, this next 4 bytes is oargb
     * but we're not using that for now, so having W here makes the code
     * simpler */
    float w;
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
