#include <stdint.h>
#include "private.h"
#include "platform.h"

static const GLubyte* VERTEX_PTR;

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

extern void DrawColouredQuads(Vertex* dst, const GLubyte* src, const GLuint numQuads);
extern void DrawTexturedQuads(Vertex* dst, const GLubyte* src, const GLuint numQuads);

void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    TRACE();
    if (!count) return;
    Vertex* start = submitVertices(count);

    if (TEXTURES_ENABLED) {
        DrawTexturedQuads(start, VERTEX_PTR + (first * 24), count >> 2);
    } else {
        DrawColouredQuads(start, VERTEX_PTR + (first * 16), count >> 2);
    }
}

void APIENTRY gldcVertexPointer(GLsizei stride, const GLvoid * pointer) {
    VERTEX_PTR = pointer;
}