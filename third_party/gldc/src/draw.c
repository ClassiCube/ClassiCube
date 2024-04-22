#include <stdint.h>
#include "private.h"
#include "platform.h"

static const void* VERTEX_PTR;

#define ITERATE(count) \
    GLuint i = count; \
    while(i--)

void _glInitAttributePointers() {
    VERTEX_PTR    = NULL;
    VERTEX_STRIDE = 0;
}


/* Generating PVR vertices from the user-submitted data gets complicated, particularly
 * when a realloc could invalidate pointers. This structure holds all the information
 * we need on the target vertex array to allow passing around to the various stages (e.g. generate/clip etc.)
 */
typedef struct __attribute__((aligned(32))) {
    PolyList* output;
    uint32_t header_offset; // The offset of the header in the output list
    uint32_t start_offset; // The offset into the output list
} SubmissionTarget;
static SubmissionTarget SUBMISSION_TARGET;

GL_FORCE_INLINE PolyHeader* _glSubmissionTargetHeader(SubmissionTarget* target) {
    return aligned_vector_at(&target->output->vector, target->header_offset);
}

GL_FORCE_INLINE Vertex* _glSubmissionTargetStart(SubmissionTarget* target) {
    return aligned_vector_at(&target->output->vector, target->start_offset);
}

static void generateColouredQuads(SubmissionTarget* target, const GLsizei first, const GLuint count) {
    /* Read from the client buffers and generate an array of ClipVertices */
    GLuint numQuads = count / 4;
    Vertex* start   = _glSubmissionTargetStart(target);
    /* Copy the pos, uv and color directly in one go */
    const GLubyte* src = VERTEX_PTR + (first * 16);

    Vertex* dst = start;
    const float w = 1.0f;
    PREFETCH(src);

    // TODO: optimise
    ITERATE(numQuads) {
        // 4 vertices per quad
        Vertex* it = dst;
        
        for(GLuint j = 0; j < 4; ++j) {
            PREFETCH(src + 16);
            TransformVertex((const float*)src, &w, it->xyz, &it->w);
            
            *((uint32_t*)&it->bgra) = *((uint32_t*)(src + 12));
            it->uv[0] = 0.0f; 
            it->uv[1] = 0.0f;

            src += 16;
            it->flags = GPU_CMD_VERTEX;
            it++;
        }

        dst[3].flags = GPU_CMD_VERTEX_EOL;
        dst += 4;
    }
}

static void generateTexturedQuads(SubmissionTarget* target, const GLsizei first, const GLuint count) {
    /* Read from the client buffers and generate an array of ClipVertices */
    GLuint numQuads = count / 4;
    Vertex* start   = _glSubmissionTargetStart(target);
    /* Copy the pos, uv and color directly in one go */
    const GLubyte* src = VERTEX_PTR + (first * 24);

    Vertex* dst = start;
    const float w = 1.0f;
    PREFETCH(src);

    // TODO: optimise
    ITERATE(numQuads) {
        // 4 vertices per quad
        Vertex* it = dst;
        
        for(GLuint j = 0; j < 4; ++j) {
            PREFETCH(src + 24);
            TransformVertex((const float*)src, &w, it->xyz, &it->w);
            
            *((uint32_t*)&it->bgra)  = *((uint32_t*)(src + 12));
            *((uint32_t*)&it->uv[0]) = *((uint32_t*)(src + 16));
            *((uint32_t*)&it->uv[1]) = *((uint32_t*)(src + 20));

            src += 24;
            it->flags = GPU_CMD_VERTEX;
            it++;
        }

        dst[3].flags = GPU_CMD_VERTEX_EOL;
        dst += 4;
    }
}

void _glInitSubmissionTarget() {
    SubmissionTarget* target = &SUBMISSION_TARGET;

    target->output = NULL;
    target->header_offset = target->start_offset = 0;
}

extern void apply_poly_header(PolyHeader* header, PolyList* activePolyList);

GL_FORCE_INLINE void submitVertices(GLuint vertexCount) {
    SubmissionTarget* const target = &SUBMISSION_TARGET;
    TRACE();
    target->output = _glActivePolyList();
    
    uint32_t vector_size      = aligned_vector_size(&target->output->vector);
    GLboolean header_required = (vector_size == 0) || STATE_DIRTY;

    target->header_offset = vector_size;
    target->start_offset  = target->header_offset + (header_required ? 1 : 0);
    gl_assert(target->header_offset >= 0);

    /* Make room for the vertices and header */
    aligned_vector_extend(&target->output->vector, (header_required) + vertexCount);    
    gl_assert(target->header_offset < aligned_vector_size(&target->output->vector));

    if (header_required) {
        apply_poly_header(_glSubmissionTargetHeader(target), target->output);
        STATE_DIRTY = GL_FALSE;
    }
}

void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    TRACE();
    if (!count) return;
    
    submitVertices(count);

    if (TEXTURES_ENABLED) {
        generateTexturedQuads(&SUBMISSION_TARGET, first, count);
    } else {
        generateColouredQuads(&SUBMISSION_TARGET, first, count);
    }
}

void APIENTRY gldcVertexPointer(GLsizei stride, const GLvoid * pointer) {
    VERTEX_PTR    = pointer;
}