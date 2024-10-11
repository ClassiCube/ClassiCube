#ifndef PRIVATE_H
#define PRIVATE_H

#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <malloc.h>

#define AV_FORCE_INLINE static __attribute__((always_inline)) inline
#define VERTEX_SIZE 32

typedef struct {
    uint32_t size;
    uint32_t capacity;
    uint32_t list_type;
    uint8_t* data;
} __attribute__((aligned(32))) AlignedVector;

#define ALIGNED_VECTOR_CHUNK_SIZE 256u

#define ROUND_TO_CHUNK_SIZE(v) \
    ((((v) + ALIGNED_VECTOR_CHUNK_SIZE - 1) / ALIGNED_VECTOR_CHUNK_SIZE) * ALIGNED_VECTOR_CHUNK_SIZE)

AV_FORCE_INLINE void* aligned_vector_reserve(AlignedVector* vector, uint32_t element_count) {
    uint32_t original_byte_size = (vector->size * VERTEX_SIZE);

    if(element_count < vector->capacity) {
        return vector->data + original_byte_size;
    }

    /* We overallocate so that we don't make small allocations during push backs */
    element_count = ROUND_TO_CHUNK_SIZE(element_count);

    uint32_t new_byte_size = (element_count * VERTEX_SIZE);
    uint8_t* original_data = vector->data;

    uint8_t* data = (uint8_t*) memalign(0x20, new_byte_size);
    if (!data) return NULL;

    memcpy(data, original_data, original_byte_size);
    free(original_data);

	vector->data     = data;
    vector->capacity = element_count;
    return data + original_byte_size;
}

#define GLenum     unsigned int
#define GLboolean  unsigned char

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
