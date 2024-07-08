#pragma once

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <malloc.h>

#define AV_FORCE_INLINE static __attribute__((always_inline)) inline
#define AV_ELEMENT_SIZE 32

typedef struct {
    uint32_t size;
    uint32_t capacity;
    uint32_t list_type;
    uint32_t padding[5];
    uint8_t* data;
} __attribute__((aligned(32))) AlignedVector;

#define ALIGNED_VECTOR_CHUNK_SIZE 256u

#define av_assert(x) \
    do {\
        if(!(x)) {\
            fprintf(stderr, "Assertion failed at %s:%d\n", __FILE__, __LINE__);\
            exit(1);\
        }\
    } while(0); \

#define ROUND_TO_CHUNK_SIZE(v) \
    ((((v) + ALIGNED_VECTOR_CHUNK_SIZE - 1) / ALIGNED_VECTOR_CHUNK_SIZE) * ALIGNED_VECTOR_CHUNK_SIZE)

AV_FORCE_INLINE void* aligned_vector_reserve(AlignedVector* vector, uint32_t element_count) {
    uint32_t original_byte_size = (vector->size * AV_ELEMENT_SIZE);

    if(element_count < vector->capacity) {
        return vector->data + original_byte_size;
    }

    /* We overallocate so that we don't make small allocations during push backs */
    element_count = ROUND_TO_CHUNK_SIZE(element_count);

    uint32_t new_byte_size = (element_count * AV_ELEMENT_SIZE);
    uint8_t* original_data = vector->data;

    uint8_t* data = (uint8_t*) memalign(0x20, new_byte_size);
    if (!data) return NULL;

    memcpy(data, original_data, original_byte_size);
    free(original_data);

	vector->data     = data;
    vector->capacity = element_count;
    return vector->data + original_byte_size;
}

AV_FORCE_INLINE void* aligned_vector_push_back(AlignedVector* vector, const void* objs, uint32_t count) {
    uint8_t* dest = (uint8_t*) aligned_vector_reserve(vector, vector->size + count);
    assert(dest);

    /* Copy the objects in */
    memcpy(dest, objs, count * AV_ELEMENT_SIZE);

    vector->size += count;
    return dest;
}

