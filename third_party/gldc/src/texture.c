#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kos.h>
#include <dc/pvr.h>

#include "gldc.h"
#include "yalloc/yalloc.h"

#ifndef NDEBUG
/* We're debugging, use normal assert */
#include <assert.h>
#define gl_assert assert
#else
/* Release mode, use our custom assert */

#define gl_assert(x) \
    do {\
        if(!(x)) {\
            fprintf(stderr, "Assertion failed at %s:%d\n", __FILE__, __LINE__);\
            exit(1);\
        }\
    } while(0); \

#endif


/* We always leave this amount of vram unallocated to prevent
 * issues with the allocator */
#define PVR_MEM_BUFFER_SIZE (64 * 1024)

TextureObject* TEXTURE_ACTIVE = NULL;
static TextureObject TEXTURE_LIST[MAX_TEXTURE_COUNT];
static unsigned char TEXTURE_USED[MAX_TEXTURE_COUNT / 8];

static int texture_id_map_used(unsigned int id) {
    unsigned int i = id / 8;
    unsigned int j = id % 8;

    return TEXTURE_USED[i] & (unsigned char)(1 << j);
}

static void texture_id_map_reserve(unsigned int id) {
    unsigned int i = id / 8;
    unsigned int j = id % 8;
    TEXTURE_USED[i] |= (unsigned char)(1 << j);
}

static void texture_id_map_release(unsigned int id) {
    unsigned int i = id / 8;
    unsigned int j = id % 8;
    TEXTURE_USED[i] &= (unsigned char)~(1 << j);
}

unsigned int texture_id_map_alloc(void) {
    unsigned int id;
    
    // ID 0 is reserved for default texture
    for(id = 1; id < MAX_TEXTURE_COUNT; ++id) {
        if(!texture_id_map_used(id)) {
            texture_id_map_reserve(id);
            return id;
        }
    }
    return 0;
}


static void* YALLOC_BASE = NULL;
static size_t YALLOC_SIZE = 0;

static void* yalloc_alloc_and_defrag(size_t size) {
    void* ret = yalloc_alloc(YALLOC_BASE, size);

    if(!ret) {
        /* Tried to allocate, but out of room, let's try defragging
         * and repeating the alloc */
        fprintf(stderr, "Ran out of memory, defragmenting\n");
        glDefragmentTextureMemory_KOS();
        ret = yalloc_alloc(YALLOC_BASE, size);
    }

    gl_assert(ret && "Out of PVR memory!");

    return ret;
}

#define GL_KOS_INTERNAL_DEFAULT_MIPMAP_LOD_BIAS 4
static void _glInitializeTextureObject(TextureObject* txr, unsigned int id) {
    txr->index  = id;
    txr->width  = txr->height = 0;
    txr->mipmap = 0;
    txr->data   = NULL;
    txr->minFilter = GL_NEAREST;
    txr->magFilter = GL_NEAREST;
    txr->mipmap_bias = GL_KOS_INTERNAL_DEFAULT_MIPMAP_LOD_BIAS;
}

void _glInitTextures() {
    memset(TEXTURE_USED, 0, sizeof(TEXTURE_USED));

    // Initialize zero as an actual texture object though because apparently it is!
    TextureObject* default_tex = &TEXTURE_LIST[0];
    _glInitializeTextureObject(default_tex, 0);
    texture_id_map_reserve(0);
    TEXTURE_ACTIVE = default_tex;

    size_t vram_free = pvr_mem_available();
    YALLOC_SIZE = vram_free - PVR_MEM_BUFFER_SIZE; /* Take all but 64kb VRAM */
    YALLOC_BASE = pvr_mem_malloc(YALLOC_SIZE);

#ifdef __DREAMCAST__
    /* Ensure memory is aligned */
    gl_assert((uintptr_t) YALLOC_BASE % 32 == 0);
#endif

    yalloc_init(YALLOC_BASE, YALLOC_SIZE);
}

GLuint gldcGenTexture(void) {
    GLuint id = texture_id_map_alloc();
    gl_assert(id);  // Generated IDs must never be zero
    
    TextureObject* txr = &TEXTURE_LIST[id];
    _glInitializeTextureObject(txr, id);

    gl_assert(txr->index == id);
    
    return id;
}

void gldcDeleteTexture(GLuint id) {
    if(id == 0) return;
    /* Zero is the "default texture" and we never allow deletion of it */

    if(texture_id_map_used(id)) {
    	TextureObject* txr = &TEXTURE_LIST[id];
        gl_assert(txr->index == id);

        if(txr == TEXTURE_ACTIVE) {
            // Reset to the default texture
            TEXTURE_ACTIVE = &TEXTURE_LIST[0];
        }

        if(txr->data) {
            yalloc_free(YALLOC_BASE, txr->data);
            txr->data = NULL;
        }

        texture_id_map_release(id);
    }
}

void gldcBindTexture(GLuint id) {
    gl_assert(texture_id_map_used(id));
    TextureObject* txr = &TEXTURE_LIST[id];

    TEXTURE_ACTIVE = txr;
    gl_assert(TEXTURE_ACTIVE->index == id);
}

int gldcAllocTexture(int w, int h, int format) {
    TextureObject* active = TEXTURE_ACTIVE;

    if (active->data) {
        /* pre-existing texture - check if changed */
        if (active->width != w || active->height != h) {
            /* changed - free old texture memory */
            yalloc_free(YALLOC_BASE, active->data);
            active->data = NULL;
            active->mipmap = 0;
        }
    }

    /* All colour formats are represented as shorts internally. */
    GLuint bytes   = w * h * 2;
    active->width  = w;
    active->height = h;
    active->color  = format;

    if(!active->data) {
        /* need texture memory */
        active->data = yalloc_alloc_and_defrag(bytes);
    }
    if (!active->data) return GL_OUT_OF_MEMORY;

    /* Mark level 0 as set in the mipmap bitmask */
    active->mipmap |= (1 << 0);
    return 0;
}

void gldcGetTexture(void** data, int* width, int* height) {
    TextureObject* active = TEXTURE_ACTIVE;
    *data   = active->data;
    *width  = active->width;
    *height = active->height;
}

GLuint _glMaxTextureMemory() {
    return YALLOC_SIZE;
}

GLuint _glFreeTextureMemory() {
    return yalloc_count_free(YALLOC_BASE);
}

GLuint _glUsedTextureMemory() {
    return YALLOC_SIZE - _glFreeTextureMemory();
}

GLuint _glFreeContiguousTextureMemory() {
    return yalloc_count_continuous(YALLOC_BASE);
}

void glDefragmentTextureMemory_KOS(void) {
    yalloc_defrag_start(YALLOC_BASE);

    GLuint id;

    /* Replace all texture pointers */
    for(id = 0; id < MAX_TEXTURE_COUNT; id++){
        if(texture_id_map_used(id)){
            TextureObject* txr = &TEXTURE_LIST[id];
            gl_assert(txr->index == id);
            txr->data = yalloc_defrag_address(YALLOC_BASE, txr->data);
        }
    }

    yalloc_defrag_commit(YALLOC_BASE);
}
