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
    uint32_t padding[6];
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


AV_FORCE_INLINE void* aligned_vector_at(const AlignedVector* vector, const uint32_t index) {
    assert(index < vector->size);
    return vector->data + (index * AV_ELEMENT_SIZE);
}

AV_FORCE_INLINE void* aligned_vector_reserve(AlignedVector* vector, uint32_t element_count) {
    uint32_t original_byte_size = (vector->size * AV_ELEMENT_SIZE);

    if(element_count < vector->capacity) {
        return vector->data + original_byte_size;
    }

    /* We overallocate so that we don't make small allocations during push backs */
    element_count = ROUND_TO_CHUNK_SIZE(element_count);

    uint32_t new_byte_size = (element_count * AV_ELEMENT_SIZE);
    uint8_t* original_data = vector->data;

    vector->data = (uint8_t*) memalign(0x20, new_byte_size);
    assert(vector->data);

    memcpy(vector->data, original_data, original_byte_size);
    free(original_data);

    vector->capacity = element_count;
    return vector->data + original_byte_size;
}

/* Resizes the array and returns a pointer to the first new element (if upsizing) or NULL (if downsizing) */
AV_FORCE_INLINE void* aligned_vector_resize(AlignedVector* vector, const uint32_t element_count) {
    void* ret = NULL;

    uint32_t previous_count = vector->size;
    if(vector->capacity <= element_count) {
        /* If we didn't have capacity, increase capacity (slow) */

        aligned_vector_reserve(vector, element_count);
        vector->size = element_count;

        ret = aligned_vector_at(vector, previous_count);

        av_assert(vector->size == element_count);
        av_assert(vector->size <= vector->capacity);
    } else if(previous_count < element_count) {
        /* So we grew, but had the capacity, just get a pointer to
         * where we were */
        vector->size = element_count;
        av_assert(vector->size < vector->capacity);
        ret = aligned_vector_at(vector, previous_count);
    } else if(vector->size != element_count) {
        vector->size = element_count;
        av_assert(vector->size < vector->capacity);
    }

    return ret;
}

AV_FORCE_INLINE void* aligned_vector_push_back(AlignedVector* vector, const void* objs, uint32_t count) {
    /* Resize enough room */
    assert(count);
#ifndef NDEBUG
    uint32_t initial_size = vector->size;
#endif

    uint8_t* dest = (uint8_t*) aligned_vector_resize(vector, vector->size + count);
    assert(dest);

    /* Copy the objects in */
    memcpy(dest, objs, count * AV_ELEMENT_SIZE);

    assert(vector->size == initial_size + count);
    return dest;
}


AV_FORCE_INLINE void* aligned_vector_extend(AlignedVector* vector, const uint32_t additional_count) {
    void* ret = aligned_vector_resize(vector, vector->size + additional_count);
    assert(ret);  // Should always return something
    return ret;
}

