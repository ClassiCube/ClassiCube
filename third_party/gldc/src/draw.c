#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "private.h"
#include "platform.h"


static void* VERTEX_PTR;
static GLsizei VERTEX_STRIDE;

extern GLboolean AUTOSORT_ENABLED;

#define ITERATE(count) \
    GLuint i = count; \
    while(i--)


void _glInitAttributePointers() {
    TRACE();
    VERTEX_PTR    = NULL;
    VERTEX_STRIDE = 0;
}


GL_FORCE_INLINE PolyHeader *_glSubmissionTargetHeader(SubmissionTarget* target) {
    gl_assert(target->header_offset < aligned_vector_size(&target->output->vector));
    return aligned_vector_at(&target->output->vector, target->header_offset);
}

GL_INLINE_DEBUG Vertex* _glSubmissionTargetStart(SubmissionTarget* target) {
    gl_assert(target->start_offset < aligned_vector_size(&target->output->vector));
    return aligned_vector_at(&target->output->vector, target->start_offset);
}

Vertex* _glSubmissionTargetEnd(SubmissionTarget* target) {
    return _glSubmissionTargetStart(target) + target->count;
}

typedef struct {
    float u, v;
} Float2;

static const Float2 F2ZERO = {0.0f, 0.0f};

static void generateQuads(SubmissionTarget* target, const GLsizei first, const GLuint count) {
    /* Read from the client buffers and generate an array of ClipVertices */
    TRACE();
    
    GLuint numQuads = count / 4;
    GLuint idx      = first;
    
    Vertex* start = _glSubmissionTargetStart(target);

    const GLuint stride = VERTEX_STRIDE;

    /* Copy the pos, uv and color directly in one go */
    const GLubyte* pos = VERTEX_PTR;
    const GLubyte* uv  = TEXTURES_ENABLED ? VERTEX_PTR : NULL;
    const GLubyte* col = VERTEX_PTR;

    Vertex* dst = start;
    const float w = 1.0f;

    // TODO: optimise
    for (GLuint i = 0; i < numQuads; ++i) {
        // 4 vertices per quad
        Vertex* it = dst;
        PREFETCH(it); // TODO: more prefetching?
        
        for(GLuint j = 0; j < 4; ++j) {
            pos = (GLubyte*) VERTEX_PTR + (idx * stride);
            PREFETCH(pos);
            TransformVertex((const float*) pos, &w, it->xyz, &it->w);
            
            col = pos + 12;
            *((uint32_t*) it->bgra) = *((uint32_t*) col);

            if(uv) {
                uv = pos + 16;
                MEMCPY4(it->uv, uv, sizeof(float) * 2);
            } else {
                *((Float2*) it->uv) = F2ZERO;
            }

            it++;
            idx++;
        }
        
        // Quads [0, 1, 2, 3] -> Triangles [{0, 1, 2}  {2, 3, 0}]
        PREFETCH(dst); // TODO: more prefetching?   
        memcpy_vertex(dst + 5, dst + 0); dst[5].flags = GPU_CMD_VERTEX_EOL;
        memcpy_vertex(dst + 4, dst + 3); dst[4].flags = GPU_CMD_VERTEX;
        memcpy_vertex(dst + 3, dst + 2); dst[3].flags = GPU_CMD_VERTEX;
        
        dst[2].flags = GPU_CMD_VERTEX_EOL;
        dst[1].flags = GPU_CMD_VERTEX;
        dst[0].flags = GPU_CMD_VERTEX;
        // TODO copy straight to dst?? 
        
        
        dst += 6;
    }
}

GL_FORCE_INLINE void divide(SubmissionTarget* target) {
    TRACE();

    /* Perform perspective divide on each vertex */
    Vertex* vertex = _glSubmissionTargetStart(target);

    const float h = vid_mode->height;

    ITERATE(target->count) {
        const float f = MATH_Fast_Invert(vertex->w);

        /* Convert to NDC and apply viewport */
        vertex->xyz[0] = MATH_fmac(
            VIEWPORT.hwidth, vertex->xyz[0] * f, VIEWPORT.x_plus_hwidth
        );
        vertex->xyz[1] = h - MATH_fmac(
            VIEWPORT.hheight, vertex->xyz[1] * f, VIEWPORT.y_plus_hheight
        );

        /* Apply depth range */
        vertex->xyz[2] = MAX(
            1.0f - MATH_fmac(vertex->xyz[2] * f, 0.5f, 0.5f),
            PVR_MIN_Z
        );

        ++vertex;
    }
}

GL_FORCE_INLINE int _calc_pvr_face_culling() {
    if(!CULLING_ENABLED) {
        return GPU_CULLING_SMALL;
    } else {
        return GPU_CULLING_CW;
    }
}

GL_FORCE_INLINE int _calc_pvr_depth_test() {
    if(!DEPTH_TEST_ENABLED) {
        return GPU_DEPTHCMP_ALWAYS;
    }

    switch(DEPTH_FUNC) {
        case GL_NEVER:
            return GPU_DEPTHCMP_NEVER;
        case GL_LESS:
            return GPU_DEPTHCMP_GREATER;
        case GL_EQUAL:
            return GPU_DEPTHCMP_EQUAL;
        case GL_LEQUAL:
            return GPU_DEPTHCMP_GEQUAL;
        case GL_GREATER:
            return GPU_DEPTHCMP_LESS;
        case GL_NOTEQUAL:
            return GPU_DEPTHCMP_NOTEQUAL;
        case GL_GEQUAL:
            return GPU_DEPTHCMP_LEQUAL;
        break;
        case GL_ALWAYS:
        default:
            return GPU_DEPTHCMP_ALWAYS;
    }
}

GL_FORCE_INLINE void _updatePVRBlend(PolyContext* context) {
    if(BLEND_ENABLED || ALPHA_TEST_ENABLED) {
        context->gen.alpha = GPU_ALPHA_ENABLE;
    } else {
        context->gen.alpha = GPU_ALPHA_DISABLE;
    }

    context->blend.src = BLEND_SRC_FACTOR;
    context->blend.dst = BLEND_DST_FACTOR;
}

GL_FORCE_INLINE void apply_poly_header(PolyHeader* header, PolyList* activePolyList) {
    TRACE();

    // Compile the header
    PolyContext ctx;
    memset(&ctx, 0, sizeof(PolyContext));

    ctx.list_type = activePolyList->list_type;
    ctx.fmt.color = GPU_CLRFMT_ARGBPACKED;
    ctx.fmt.uv = GPU_UVFMT_32BIT;
    ctx.gen.color_clamp = GPU_CLRCLAMP_DISABLE;

    ctx.gen.culling = _calc_pvr_face_culling();
    ctx.depth.comparison = _calc_pvr_depth_test();
    ctx.depth.write = DEPTH_MASK_ENABLED ? GPU_DEPTHWRITE_ENABLE : GPU_DEPTHWRITE_DISABLE;

    ctx.gen.shading = (SHADE_MODEL == GL_SMOOTH) ? GPU_SHADE_GOURAUD : GPU_SHADE_FLAT;

    if(SCISSOR_TEST_ENABLED) {
        ctx.gen.clip_mode = GPU_USERCLIP_INSIDE;
    } else {
        ctx.gen.clip_mode = GPU_USERCLIP_DISABLE;
    }

    if(FOG_ENABLED) {
        ctx.gen.fog_type = GPU_FOG_TABLE;
    } else {
        ctx.gen.fog_type = GPU_FOG_DISABLE;
    }

    _updatePVRBlend(&ctx);

    if(ctx.list_type == GPU_LIST_OP_POLY) {
        /* Opaque polys are always one/zero */
        ctx.blend.src = PVR_BLEND_ONE;
        ctx.blend.dst = PVR_BLEND_ZERO;
    } else if(ctx.list_type == GPU_LIST_PT_POLY) {
        /* Punch-through polys require fixed blending and depth modes */
        ctx.blend.src = PVR_BLEND_SRCALPHA;
        ctx.blend.dst = PVR_BLEND_INVSRCALPHA;
        ctx.depth.comparison = GPU_DEPTHCMP_LEQUAL;
    } else if(ctx.list_type == GPU_LIST_TR_POLY && AUTOSORT_ENABLED) {
        /* Autosort mode requires this mode for transparent polys */
        ctx.depth.comparison = GPU_DEPTHCMP_GEQUAL;
    }

    _glUpdatePVRTextureContext(&ctx, 0);

    CompilePolyHeader(header, &ctx);

    /* Force bits 18 and 19 on to switch to 6 triangle strips */
    header->cmd |= 0xC0000;

    /* Post-process the vertex list */
    /*
     * This is currently unnecessary. aligned_vector memsets the allocated objects
     * to zero, and we don't touch oargb, also, we don't *enable* oargb yet in the
     * pvr header so it should be ignored anyway. If this ever becomes a problem,
     * uncomment this.
    ClipVertex* vout = output;
    const ClipVertex* end = output + count;
    while(vout < end) {
        vout->oargb = 0;
    }
    */
}

#define DEBUG_CLIPPING 0


static SubmissionTarget SUBMISSION_TARGET;


void _glInitSubmissionTarget() {
    SubmissionTarget* target = &SUBMISSION_TARGET;

    target->count = 0;
    target->output = NULL;
    target->header_offset = target->start_offset = 0;
}


GL_FORCE_INLINE void submitVertices(GLenum mode, GLsizei first, GLuint count) {

    SubmissionTarget* const target = &SUBMISSION_TARGET;

    TRACE();

    /* No vertices? Do nothing */
    if(!count) return;

    target->output = _glActivePolyList();
    gl_assert(target->output);

    uint32_t vector_size = aligned_vector_size(&target->output->vector);

    GLboolean header_required = (vector_size == 0) || STATE_DIRTY;

    target->count = count * 6 / 4; // quads -> triangles
    target->header_offset = vector_size;
    target->start_offset = target->header_offset + (header_required ? 1 : 0);

    gl_assert(target->header_offset >= 0);
    gl_assert(target->start_offset >= target->header_offset);
    gl_assert(target->count);

    /* Make room for the vertices and header */
    aligned_vector_extend(&target->output->vector, target->count + (header_required));

    if(header_required) {
        apply_poly_header(_glSubmissionTargetHeader(target), target->output);
        STATE_DIRTY = GL_FALSE;
    }

    generateQuads(target, first, count);
}

void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    TRACE();
    submitVertices(mode, first, count);
}

void APIENTRY glVertexPointer(GLint size, GLenum type,  GLsizei stride,  const GLvoid * pointer) {
    VERTEX_PTR    = pointer;
    VERTEX_STRIDE = stride;
}
