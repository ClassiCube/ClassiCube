#ifndef PRIVATE_H
#define PRIVATE_H

#include <stdint.h>
#include "aligned_vector.h"

#define MAX_TEXTURE_COUNT 768

#define GLuint     unsigned int
#define GLenum     unsigned int
#define GLboolean  unsigned char


void glKosInit();
void glKosSwapBuffers();

typedef struct {
    /* Same 32 byte layout as pvr_vertex_t */
    uint32_t flags;
    float x, y, z;
    float u, v;
    uint8_t bgra[4];

    /* In the pvr_vertex_t structure, this next 4 bytes is oargb
     * but we're not using that for now, so having W here makes the code
     * simpler */
    float w;
} __attribute__ ((aligned (32))) Vertex;


#define GL_FORCE_INLINE static __attribute__((always_inline)) inline

typedef struct {
    uint32_t color; /* This is the PVR texture format */
    void     *data;
    uint16_t width;
    uint16_t height;
} TextureObject;


void _glInitTextures();
void* texmem_alloc(size_t size);
void  texmem_free(void* ptr);

GLuint _glFreeTextureMemory(void);
GLuint _glUsedTextureMemory(void);

extern TextureObject* TEXTURE_ACTIVE;
extern GLboolean TEXTURES_ENABLED;
extern TextureObject TEXTURE_LIST[MAX_TEXTURE_COUNT];

extern GLboolean DEPTH_TEST_ENABLED;
extern GLboolean DEPTH_MASK_ENABLED;

extern GLboolean CULLING_ENABLED;

extern GLboolean FOG_ENABLED;
extern GLboolean ALPHA_TEST_ENABLED;
extern GLboolean BLEND_ENABLED;

extern GLboolean SCISSOR_TEST_ENABLED;
extern GLenum SHADE_MODEL;
extern GLboolean AUTOSORT_ENABLED;


extern AlignedVector OP_LIST;
extern AlignedVector PT_LIST;
extern AlignedVector TR_LIST;

GL_FORCE_INLINE AlignedVector* _glActivePolyList() {
    if (BLEND_ENABLED)      return &TR_LIST;
    if (ALPHA_TEST_ENABLED) return &PT_LIST;

    return &OP_LIST;
}

extern GLboolean STATE_DIRTY;

void SceneListSubmit(Vertex* v2, int n, int type);

#endif // PRIVATE_H
