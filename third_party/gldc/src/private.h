#ifndef PRIVATE_H
#define PRIVATE_H

#include <stdint.h>
#include <stdio.h>

#include "gl_assert.h"
#include "platform.h"
#include "types.h"

#include "../include/gldc.h"

#include "aligned_vector.h"
#include "named_array.h"

/* This figure is derived from the needs of Quake 1 */
#define MAX_TEXTURE_COUNT 1088


extern void* memcpy4 (void *dest, const void *src, size_t count);

#define GL_NO_INSTRUMENT inline __attribute__((no_instrument_function))
#define GL_INLINE_DEBUG GL_NO_INSTRUMENT __attribute__((always_inline))
#define GL_FORCE_INLINE static GL_INLINE_DEBUG
#define _GL_UNUSED(x) (void)(x)

#define TRACE_ENABLED 0
#define TRACE() if(TRACE_ENABLED) {fprintf(stderr, "%s\n", __func__);} (void) 0

typedef struct {
    unsigned int flags;      /* Constant PVR_CMD_USERCLIP */
    unsigned int d1, d2, d3; /* Ignored for this type */
    unsigned int sx,         /* Start x */
             sy,         /* Start y */
             ex,         /* End x */
             ey;         /* End y */
} PVRTileClipCommand; /* Tile Clip command for the pvr */

typedef struct {
    unsigned int list_type;
    AlignedVector vector;
} PolyList;

typedef struct {
    GLint x;
    GLint y;
    GLint width;
    GLint height;

    float x_plus_hwidth;
    float y_plus_hheight;
    float hwidth;  /* width * 0.5f */
    float hheight; /* height * 0.5f */
} Viewport;

extern Viewport VIEWPORT;

typedef struct {
    //0
    GLuint   index;
    GLuint   color; /* This is the PVR texture format */
    //8
    GLenum minFilter;
    GLenum magFilter;
    //16
    GLvoid *data;
    //20
    GLushort width;
    GLushort height;
    //24
    GLushort  mipmap;  /* Bitmask of supplied mipmap levels */
	// 26
	GLushort _pad0;
    // 28
    GLubyte mipmap_bias;
    GLubyte  env;
    GLubyte _pad1;
    GLubyte _pad2;
    //32
    GLubyte padding[32];  // Pad to 64-bytes
} __attribute__((aligned(32))) TextureObject;


GL_FORCE_INLINE void memcpy_vertex(Vertex *dest, const Vertex *src) {
#ifdef __DREAMCAST__
    _Complex float double_scratch;

    asm volatile (
        "fschg\n\t"
        "clrs\n\t"
        ".align 2\n\t"
        "fmov.d @%[in]+, %[scratch]\n\t"
        "fmov.d %[scratch], @%[out]\n\t"
        "fmov.d @%[in]+, %[scratch]\n\t"
        "add #8, %[out]\n\t"
        "fmov.d %[scratch], @%[out]\n\t"
        "fmov.d @%[in]+, %[scratch]\n\t"
        "add #8, %[out]\n\t"
        "fmov.d %[scratch], @%[out]\n\t"
        "fmov.d @%[in], %[scratch]\n\t"
        "add #8, %[out]\n\t"
        "fmov.d %[scratch], @%[out]\n\t"
        "fschg\n"
        : [in] "+&r" ((uint32_t) src), [scratch] "=&d" (double_scratch), [out] "+&r" ((uint32_t) dest)
        :
        : "t", "memory" // clobbers
    );
#else
    *dest = *src;
#endif
}

#define swapVertex(a, b)   \
do {                 \
    Vertex __attribute__((aligned(32))) c;   \
    memcpy_vertex(&c, a); \
    memcpy_vertex(a, b); \
    memcpy_vertex(b, &c); \
} while(0)

/* Generating PVR vertices from the user-submitted data gets complicated, particularly
 * when a realloc could invalidate pointers. This structure holds all the information
 * we need on the target vertex array to allow passing around to the various stages (e.g. generate/clip etc.)
 */
typedef struct __attribute__((aligned(32))) {
    PolyList* output;
    uint32_t header_offset; // The offset of the header in the output list
    uint32_t start_offset; // The offset into the output list
    uint32_t count; // The number of vertices in this output
} SubmissionTarget;

Vertex* _glSubmissionTargetStart(SubmissionTarget* target);
Vertex* _glSubmissionTargetEnd(SubmissionTarget* target);

typedef enum {
    CLIP_RESULT_ALL_IN_FRONT,
    CLIP_RESULT_ALL_BEHIND,
    CLIP_RESULT_ALL_ON_PLANE,
    CLIP_RESULT_FRONT_TO_BACK,
    CLIP_RESULT_BACK_TO_FRONT
} ClipResult;


struct SubmissionTarget;

void _glInitAttributePointers();
void _glInitContext();
void _glInitMatrices();
void _glInitSubmissionTarget();

GLubyte _glInitTextures();

void _glUpdatePVRTextureContext(PolyContext* context, GLshort textureUnit);


extern TextureObject* TEXTURE_ACTIVE;
extern GLboolean TEXTURES_ENABLED;

extern GLenum DEPTH_FUNC;
extern GLboolean DEPTH_TEST_ENABLED;
extern GLboolean DEPTH_MASK_ENABLED;

extern GLboolean CULLING_ENABLED;

extern GLboolean FOG_ENABLED;
extern GLboolean ALPHA_TEST_ENABLED;

extern GLboolean SCISSOR_TEST_ENABLED;
extern GLenum SHADE_MODEL;

extern GLboolean BLEND_ENABLED;
extern GLenum BLEND_SRC_FACTOR;
extern GLenum BLEND_DST_FACTOR;


extern PolyList OP_LIST;
extern PolyList PT_LIST;
extern PolyList TR_LIST;

GL_FORCE_INLINE PolyList* _glActivePolyList() {
    if(BLEND_ENABLED) {
        return &TR_LIST;
    } else if(ALPHA_TEST_ENABLED) {
        return &PT_LIST;
    } else {
        return &OP_LIST;
    }
}

extern GLenum LAST_ERROR;
extern char ERROR_FUNCTION[64];

GL_FORCE_INLINE const char* _glErrorEnumAsString(GLenum error) {
    switch(error) {
        case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
        case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
        default:
            return "GL_UNKNOWN_ERROR";
    }
}

GL_FORCE_INLINE void _glKosThrowError(GLenum error, const char *function) {
    if(LAST_ERROR == GL_NO_ERROR) {
        LAST_ERROR = error;
        sprintf(ERROR_FUNCTION, "%s\n", function);
        fprintf(stderr, "GL ERROR: %s when calling %s\n", _glErrorEnumAsString(LAST_ERROR), ERROR_FUNCTION);
    }
}

GL_FORCE_INLINE void _glKosResetError() {
    LAST_ERROR = GL_NO_ERROR;
    sprintf(ERROR_FUNCTION, "\n");
}

unsigned char _glIsClippingEnabled();
void _glEnableClipping(unsigned char v);

GLuint _glFreeTextureMemory();
GLuint _glUsedTextureMemory();
GLuint _glFreeContiguousTextureMemory();

void _glApplyScissor(bool force);

extern GLboolean ZNEAR_CLIPPING_ENABLED;
extern GLboolean STATE_DIRTY;


/* This is from KOS pvr_buffers.c */
#define PVR_MIN_Z 0.0001f

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define CLAMP( X, _MIN, _MAX )  ( (X)<(_MIN) ? (_MIN) : ((X)>(_MAX) ? (_MAX) : (X)) )

#endif // PRIVATE_H
