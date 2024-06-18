#pragma once

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <malloc.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
#define AV_FORCE_INLINE static inline
#else
#define AV_NO_INSTRUMENT inline __attribute__((no_instrument_function))
#define AV_INLINE_DEBUG AV_NO_INSTRUMENT __attribute__((always_inline))
#define AV_FORCE_INLINE static AV_INLINE_DEBUG
#endif


#ifdef __DREAMCAST__
#include <kos/string.h>

AV_FORCE_INLINE void *AV_MEMCPY4(void *dest, const void *src, size_t len)
{
  if(!len)
  {
    return dest;
  }

  const uint8_t *s = (uint8_t *)src;
  uint8_t *d = (uint8_t *)dest;

  uint32_t diff = (uint32_t)d - (uint32_t)(s + 1); // extra offset because input gets incremented before output is calculated
  // Underflow would be like adding a negative offset

  // Can use 'd' as a scratch reg now
  asm volatile (
    "clrs\n" // Align for parallelism (CO) - SH4a use "stc SR, Rn" instead with a dummy Rn
  ".align 2\n"
  "0:\n\t"
    "dt %[size]\n\t" // (--len) ? 0 -> T : 1 -> T (EX 1)
    "mov.b @%[in]+, %[scratch]\n\t" // scratch = *(s++) (LS 1/2)
    "bf.s 0b\n\t" // while(s != nexts) aka while(!T) (BR 1/2)
    " mov.b %[scratch], @(%[offset], %[in])\n" // *(datatype_of_s*) ((char*)s + diff) = scratch, where src + diff = dest (LS 1)
    : [in] "+&r" ((uint32_t)s), [scratch] "=&r" ((uint32_t)d), [size] "+&r" (len) // outputs
    : [offset] "z" (diff) // inputs
    : "t", "memory" // clobbers
  );

  return dest;
}

#else
#define AV_MEMCPY4 memcpy
#endif
#define AV_ELEMENT_SIZE 32

typedef struct {
    uint32_t size;
    uint32_t capacity;
    uint32_t padding[6];
    uint8_t* data;
} __attribute__((aligned(32))) AlignedVector;

#define ALIGNED_VECTOR_CHUNK_SIZE 256u


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

    AV_MEMCPY4(vector->data, original_data, original_byte_size);
    free(original_data);

    vector->capacity = element_count;
    return vector->data + original_byte_size;
}

#define av_assert(x) \
    do {\
        if(!(x)) {\
            fprintf(stderr, "Assertion failed at %s:%d\n", __FILE__, __LINE__);\
            exit(1);\
        }\
    } while(0); \

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
    AV_MEMCPY4(dest, objs, count * AV_ELEMENT_SIZE);

    assert(vector->size == initial_size + count);
    return dest;
}


AV_FORCE_INLINE void* aligned_vector_extend(AlignedVector* vector, const uint32_t additional_count) {
    void* ret = aligned_vector_resize(vector, vector->size + additional_count);
    assert(ret);  // Should always return something
    return ret;
}

AV_FORCE_INLINE void aligned_vector_clear(AlignedVector* vector){
    vector->size = 0;
}

AV_FORCE_INLINE void aligned_vector_init(AlignedVector* vector) {
    /* Now initialize the header*/
    vector->size = 0;
    vector->capacity = 0;
    vector->data = NULL;
}


#ifdef __cplusplus
}
#endif
