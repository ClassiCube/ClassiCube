#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>

#include "private.h"
#include "aligned_vector.h"

extern inline void* aligned_vector_resize(AlignedVector* vector, const uint32_t element_count);
extern inline void* aligned_vector_extend(AlignedVector* vector, const uint32_t additional_count);
extern inline void* aligned_vector_reserve(AlignedVector* vector, uint32_t element_count);
extern inline void* aligned_vector_push_back(AlignedVector* vector, const void* objs, uint32_t count);

void aligned_vector_init(AlignedVector* vector, uint32_t element_size) {
    /* Now initialize the header*/
    AlignedVectorHeader* const hdr = &vector->hdr;
    hdr->size = 0;
    hdr->capacity = ALIGNED_VECTOR_CHUNK_SIZE;
    hdr->element_size = element_size;
    vector->data = NULL;

    /* Reserve some initial capacity. This will do the allocation but not set up the header */
    void* ptr = aligned_vector_reserve(vector, ALIGNED_VECTOR_CHUNK_SIZE);
    assert(ptr);
    (void) ptr;
}
