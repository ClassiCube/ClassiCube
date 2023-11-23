#include "private.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"

#include "yalloc/yalloc.h"

/* We always leave this amount of vram unallocated to prevent
 * issues with the allocator */
#define PVR_MEM_BUFFER_SIZE (64 * 1024)

TextureObject* TEXTURE_ACTIVE = NULL;
static NamedArray TEXTURE_OBJECTS;

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
    txr->index = id;
    txr->width = txr->height = 0;
    txr->mipmap = 0;
    txr->env = GPU_TXRENV_MODULATEALPHA;
    txr->data = NULL;
    txr->minFilter = GL_NEAREST;
    txr->magFilter = GL_NEAREST;
    txr->mipmap_bias = GL_KOS_INTERNAL_DEFAULT_MIPMAP_LOD_BIAS;
}

GLubyte _glInitTextures() {
    named_array_init(&TEXTURE_OBJECTS, sizeof(TextureObject), MAX_TEXTURE_COUNT);

    // Reserve zero so that it is never given to anyone as an ID!
    named_array_reserve(&TEXTURE_OBJECTS, 0);

    // Initialize zero as an actual texture object though because apparently it is!
    TextureObject* default_tex = (TextureObject*) named_array_get(&TEXTURE_OBJECTS, 0);
    _glInitializeTextureObject(default_tex, 0);
    TEXTURE_ACTIVE = default_tex;

    size_t vram_free = pvr_mem_available();
    YALLOC_SIZE = vram_free - PVR_MEM_BUFFER_SIZE; /* Take all but 64kb VRAM */
    YALLOC_BASE = GPUMemoryAlloc(YALLOC_SIZE);

#ifdef __DREAMCAST__
    /* Ensure memory is aligned */
    gl_assert((uintptr_t) YALLOC_BASE % 32 == 0);
#endif

    yalloc_init(YALLOC_BASE, YALLOC_SIZE);

    gl_assert(TEXTURE_OBJECTS.element_size > 0);
    return 1;
}

GLuint APIENTRY gldcGenTexture(void) {
    TRACE();

    gl_assert(TEXTURE_OBJECTS.element_size > 0);

    GLuint id = 0;
    TextureObject* txr = (TextureObject*) named_array_alloc(&TEXTURE_OBJECTS, &id);
    gl_assert(txr);
    gl_assert(id);  // Generated IDs must never be zero

    _glInitializeTextureObject(txr, id);

    gl_assert(txr->index == id);
    gl_assert(TEXTURE_OBJECTS.element_size > 0);
    
    return id;
}

void APIENTRY gldcDeleteTexture(GLuint id) {
    TRACE();

    gl_assert(TEXTURE_OBJECTS.element_size > 0);

    if(id == 0) {
        /* Zero is the "default texture" and we never allow deletion of it */
        return;
    }

    TextureObject* txr = (TextureObject*) named_array_get(&TEXTURE_OBJECTS, id);

    if(txr) {
        gl_assert(txr->index == id);

        if(txr == TEXTURE_ACTIVE) {
            // Reset to the default texture
            TEXTURE_ACTIVE = (TextureObject*) named_array_get(&TEXTURE_OBJECTS, 0);
        }

        if(txr->data) {
            yalloc_free(YALLOC_BASE, txr->data);
            txr->data = NULL;
        }

        named_array_release(&TEXTURE_OBJECTS, id);
    }

    gl_assert(TEXTURE_OBJECTS.element_size > 0);
}

void APIENTRY gldcBindTexture(GLuint id) {
    TRACE();

    TextureObject* txr = (TextureObject*) named_array_get(&TEXTURE_OBJECTS, id);

    TEXTURE_ACTIVE = txr;
    gl_assert(TEXTURE_ACTIVE->index == id);

    gl_assert(TEXTURE_OBJECTS.element_size > 0);

    STATE_DIRTY = GL_TRUE;
}

static GLuint _determinePVRFormat(GLint internalFormat, GLenum type) {
    /* Given a cleaned internalFormat, return the Dreamcast format
     * that can hold it
     */
    switch(internalFormat) {
        case GL_RGBA:
        /* OK so if we have something that requires alpha, we return 4444 unless
         * the type was already 1555 (1-bit alpha) in which case we return that
         */
            if(type == GL_UNSIGNED_SHORT_1_5_5_5_REV) {
                return GPU_TXRFMT_ARGB1555 | GPU_TXRFMT_NONTWIDDLED;
            } else if(type == GL_UNSIGNED_SHORT_1_5_5_5_REV_TWID_KOS) {
                return GPU_TXRFMT_ARGB1555 | GPU_TXRFMT_TWIDDLED;
            } else if(type == GL_UNSIGNED_SHORT_4_4_4_4_REV_TWID_KOS) {
                return GPU_TXRFMT_ARGB4444 | GPU_TXRFMT_TWIDDLED;
            } else {
                return GPU_TXRFMT_ARGB4444 | GPU_TXRFMT_NONTWIDDLED;
            }
        /* Compressed and twiddled versions */
        case GL_UNSIGNED_SHORT_5_6_5_TWID_KOS:
            return GPU_TXRFMT_RGB565 | GPU_TXRFMT_TWIDDLED;
        case GL_UNSIGNED_SHORT_4_4_4_4_REV_TWID_KOS:
            return GPU_TXRFMT_ARGB4444 | GPU_TXRFMT_TWIDDLED;
        case GL_UNSIGNED_SHORT_1_5_5_5_REV_TWID_KOS:
            return GPU_TXRFMT_ARGB1555 | GPU_TXRFMT_TWIDDLED;
        default:
            return 0;
    }
}


#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define INFO_MSG(x) fprintf(stderr, "%s:%s > %s\n", __FILE__, TOSTRING(__LINE__), x)

int APIENTRY gldcAllocTexture(GLsizei w, GLsizei h, GLenum format, GLenum type) {
    TRACE();

    if (w > 1024 || h > 1024){
        INFO_MSG("Invalid texture size");
        return GL_INVALID_VALUE;
    }

    if((w < 8 || (w & -w) != w)) {
        /* Width is not a power of two. Must be!*/
        INFO_MSG("Invalid texture width");
        return GL_INVALID_VALUE;
    }

    if((h < 8 || (h & -h) != h)) {
        /* height is not a power of two. Must be!*/
        INFO_MSG("Invalid texture height");
        return GL_INVALID_VALUE;
    }

    TextureObject* active = TEXTURE_ACTIVE;
    /* Calculate the format that we need to convert the data to */
    GLuint pvr_format = _determinePVRFormat(format, type);

    if(active->data) {
        /* pre-existing texture - check if changed */
        if(active->width != w || active->height != h) {
            /* changed - free old texture memory */
            yalloc_free(YALLOC_BASE, active->data);
            active->data = NULL;
            active->mipmap = 0;
        }
    }

    /* All colour formats are represented as shorts internally. */
    GLuint bytes = w * h * 2;

    if(!active->data) {
        /* need texture memory */
        active->width  = w;
        active->height = h;
        active->color  = pvr_format;

        active->data   = yalloc_alloc_and_defrag(bytes);
    }

    gl_assert(active->data);

    /* If we run out of PVR memory just return */
    if(!active->data) {
        INFO_MSG("Out of texture memory");
        return GL_OUT_OF_MEMORY;
    }

    /* Mark level 0 as set in the mipmap bitmask */
    active->mipmap |= (1 << 0);

    /* We make sure we remove nontwiddled and add twiddled. We could always
     * make it twiddled when determining the format but I worry that would make the
     * code less flexible to change in the future */
    active->color &= ~(1 << 26);

    STATE_DIRTY = GL_TRUE;
    return 0;
}

GLAPI void APIENTRY gldcGetTexture(GLvoid** data, GLsizei* width, GLsizei* height) {
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

GLAPI GLvoid APIENTRY glDefragmentTextureMemory_KOS(void) {
    yalloc_defrag_start(YALLOC_BASE);

    GLuint id;

    /* Replace all texture pointers */
    for(id = 0; id < MAX_TEXTURE_COUNT; id++){
        TextureObject* txr = (TextureObject*) named_array_get(&TEXTURE_OBJECTS, id);
        if(txr){
            gl_assert(txr->index == id);
            txr->data = yalloc_defrag_address(YALLOC_BASE, txr->data);
        }
    }

    yalloc_defrag_commit(YALLOC_BASE);
}
