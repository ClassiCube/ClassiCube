#ifndef PRIVATE_H
#define PRIVATE_H

#include <stdint.h>
#include "aligned_vector.h"

#define MAX_TEXTURE_COUNT 768

#define GL_NEAREST          0x2600
#define GL_LINEAR           0x2601
#define GL_OUT_OF_MEMORY    0x0505

#define GLushort   unsigned short
#define GLuint     unsigned int
#define GLenum     unsigned int
#define GLubyte    unsigned char
#define GLboolean  unsigned char


GLuint gldcGenTexture(void);
void   gldcDeleteTexture(GLuint texture);
void   gldcBindTexture(GLuint texture);

/* Loads texture from SH4 RAM into PVR VRAM */
int  gldcAllocTexture(int w, int h, int format);
void gldcGetTexture(void** data, int* width, int* height);

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
    //0
    GLuint   index;
    GLuint   color; /* This is the PVR texture format */
    //8
    GLenum minFilter;
    GLenum magFilter;
    //16
    void *data;
    //20
    GLushort width;
    GLushort height;
    // 24
    GLushort  mipmap;  /* Bitmask of supplied mipmap levels */
    // 26
    GLubyte mipmap_bias;
    GLubyte _pad3;
    // 28
    GLushort _pad0;
    // 30
    GLubyte _pad1;
    GLubyte _pad2;
} __attribute__((aligned(32))) TextureObject;


void _glInitTextures();

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


extern AlignedVector OP_LIST;
extern AlignedVector PT_LIST;
extern AlignedVector TR_LIST;

GL_FORCE_INLINE AlignedVector* _glActivePolyList() {
    if (BLEND_ENABLED)      return &TR_LIST;
    if (ALPHA_TEST_ENABLED) return &PT_LIST;

    return &OP_LIST;
}

/* Memory allocation extension (GL_KOS_texture_memory_management) */
void glDefragmentTextureMemory_KOS(void);

GLuint _glFreeTextureMemory(void);
GLuint _glUsedTextureMemory(void);
GLuint _glFreeContiguousTextureMemory(void);

extern GLboolean STATE_DIRTY;

void SceneListSubmit(Vertex* v2, int n, int type);

#endif // PRIVATE_H
