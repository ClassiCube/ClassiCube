#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kos.h>
#include <dc/pvr.h>
#include "gldc.h"
#include "yalloc/yalloc.h"

/* We always leave this amount of vram unallocated to prevent
 * issues with the allocator */
#define PVR_MEM_BUFFER_SIZE (64 * 1024)

TextureObject* TEXTURE_ACTIVE;
TextureObject  TEXTURE_LIST[MAX_TEXTURE_COUNT];
static void* YALLOC_BASE;
static size_t YALLOC_SIZE;

static void glDefragmentTextureMemory_KOS(void) {
    yalloc_defrag_start(YALLOC_BASE);

    /* Replace all texture pointers */
    for(int i = 0; i < MAX_TEXTURE_COUNT; i++)
    {
        TextureObject* txr = &TEXTURE_LIST[i];
        if (!txr->data) continue;

        txr->data = yalloc_defrag_address(YALLOC_BASE, txr->data);
    }

    yalloc_defrag_commit(YALLOC_BASE);
}

void texmem_init() {
    size_t vram_free = pvr_mem_available();
    YALLOC_SIZE = vram_free - PVR_MEM_BUFFER_SIZE; /* Take all but 64kb VRAM */
    YALLOC_BASE = pvr_mem_malloc(YALLOC_SIZE);

    yalloc_init(YALLOC_BASE, YALLOC_SIZE);
}

void* texmem_alloc(size_t size) {
    void* ret = yalloc_alloc(YALLOC_BASE, size);
    if(ret) return ret;

    /* Tried to allocate, but out of room, let's try defragging
     * and repeating the alloc */
    fprintf(stderr, "Ran out of memory, defragmenting\n");
    glDefragmentTextureMemory_KOS();
    ret = yalloc_alloc(YALLOC_BASE, size);
    return ret;
}

void texmem_free(void* ptr) {
    yalloc_free(YALLOC_BASE, ptr);
}

GLuint _glFreeTextureMemory() {
    return yalloc_count_free(YALLOC_BASE);
}

GLuint _glUsedTextureMemory() {
    return YALLOC_SIZE - _glFreeTextureMemory();
}
