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

void aligned_vector_shrink_to_fit(AlignedVector* vector) {
    AlignedVectorHeader* const hdr = &vector->hdr;
    if(hdr->size == 0) {
        uint32_t element_size = hdr->element_size;
        free(vector->data);

        /* Reallocate the header */
        vector->data = NULL;
        hdr->size = hdr->capacity = 0;
        hdr->element_size = element_size;
    } else {
        uint32_t new_byte_size = (hdr->size * hdr->element_size);
        uint8_t* original_data = vector->data;
        vector->data = (unsigned char*) memalign(0x20, new_byte_size);

        if(original_data) {
            FASTCPY(vector->data, original_data, new_byte_size);
            free(original_data);
        }
        hdr->capacity = hdr->size;
    }
}

void aligned_vector_cleanup(AlignedVector* vector) {
    aligned_vector_clear(vector);
    aligned_vector_shrink_to_fit(vector);
}
