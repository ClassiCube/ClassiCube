#include <stdint.h>
#include "private.h"
#include "platform.h"

static const void* VERTEX_PTR;

#define ITERATE(count) \
    GLuint i = count; \
    while(i--)

static void generateColouredQuads(Vertex* dst, const GLsizei first, const GLuint count) {
    /* Read from the client buffers and generate an array of ClipVertices */
    GLuint numQuads = count / 4;
    /* Copy the pos, uv and color directly in one go */
    const GLubyte* src = VERTEX_PTR + (first * 16);

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

static void generateTexturedQuads(Vertex* dst, const GLsizei first, const GLuint count) {
    /* Read from the client buffers and generate an array of ClipVertices */
    GLuint numQuads = count / 4;
    /* Copy the pos, uv and color directly in one go */
    const GLubyte* src = VERTEX_PTR + (first * 24);

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

extern void apply_poly_header(PolyHeader* header, PolyList* activePolyList);

GL_FORCE_INLINE Vertex* submitVertices(GLuint vertexCount) {
    TRACE();
    PolyList* output = _glActivePolyList();
    uint32_t header_offset;
    uint32_t start_offset;
    
    uint32_t vector_size      = aligned_vector_size(&output->vector);
    GLboolean header_required = (vector_size == 0) || STATE_DIRTY;

    header_offset = vector_size;
    start_offset  = header_offset + (header_required ? 1 : 0);

    /* Make room for the vertices and header */
    aligned_vector_extend(&output->vector, (header_required) + vertexCount);
    gl_assert(header_offset < aligned_vector_size(&output->vector));

    if (header_required) {
        apply_poly_header(aligned_vector_at(&output->vector, header_offset), output);
        STATE_DIRTY = GL_FALSE;
    }
    return aligned_vector_at(&output->vector, start_offset);
}

void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    TRACE();
    if (!count) return;
    Vertex* start = submitVertices(count);

    if (TEXTURES_ENABLED) {
        generateTexturedQuads(start, first, count);
    } else {
        generateColouredQuads(start, first, count);
    }
}

void APIENTRY gldcVertexPointer(GLsizei stride, const GLvoid * pointer) {
    VERTEX_PTR = pointer;
}