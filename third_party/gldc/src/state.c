#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "private.h"

GLboolean STATE_DIRTY = GL_TRUE;

GLboolean DEPTH_TEST_ENABLED = GL_FALSE;
GLboolean DEPTH_MASK_ENABLED = GL_FALSE;

GLboolean CULLING_ENABLED = GL_FALSE;

GLboolean FOG_ENABLED        = GL_FALSE;
GLboolean ALPHA_TEST_ENABLED = GL_FALSE;

GLboolean SCISSOR_TEST_ENABLED = GL_FALSE;
GLenum SHADE_MODEL = GL_SMOOTH;
GLboolean ZNEAR_CLIPPING_ENABLED = GL_FALSE;

GLboolean BLEND_ENABLED = GL_FALSE;

GLboolean TEXTURES_ENABLED = GL_FALSE;

extern GLboolean AUTOSORT_ENABLED;


static struct {
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    GLboolean applied;
} scissor_rect = {0, 0, 640, 480, false};

void _glInitContext() {
    scissor_rect.x = 0;
    scissor_rect.y = 0;
    scissor_rect.width  = vid_mode->width;
    scissor_rect.height = vid_mode->height;

    glClearDepth(1.0f);
    glDepthMask(GL_TRUE);
    glShadeModel(GL_SMOOTH);

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
}

GLAPI void APIENTRY glEnable(GLenum cap) {
    switch(cap) {
        case GL_TEXTURE_2D:
            if(TEXTURES_ENABLED != GL_TRUE) {
                TEXTURES_ENABLED = GL_TRUE;
                STATE_DIRTY = GL_TRUE;
            }
        break;
        case GL_CULL_FACE: {
            CULLING_ENABLED = GL_TRUE;
            STATE_DIRTY = GL_TRUE;
        } break;
        case GL_DEPTH_TEST: {
            if(DEPTH_TEST_ENABLED != GL_TRUE) {
                DEPTH_TEST_ENABLED = GL_TRUE;
                STATE_DIRTY = GL_TRUE;
            }
        } break;
        case GL_BLEND: {
            if(BLEND_ENABLED != GL_TRUE) {
                BLEND_ENABLED = GL_TRUE;
                STATE_DIRTY = GL_TRUE;
            }
        } break;
        case GL_SCISSOR_TEST: {
            SCISSOR_TEST_ENABLED = GL_TRUE;
            STATE_DIRTY = GL_TRUE;
        } break;
        case GL_FOG:
            if(FOG_ENABLED != GL_TRUE) {
                FOG_ENABLED = GL_TRUE;
                STATE_DIRTY = GL_TRUE;
            }
        break;
        case GL_ALPHA_TEST: {
            if(ALPHA_TEST_ENABLED != GL_TRUE) {
                ALPHA_TEST_ENABLED = GL_TRUE;
                STATE_DIRTY = GL_TRUE;
            }
        } break;
        case GL_NEARZ_CLIPPING_KOS:
            ZNEAR_CLIPPING_ENABLED = GL_TRUE;
            STATE_DIRTY = GL_TRUE;
        break;
    default:
        break;
    }
}

GLAPI void APIENTRY glDisable(GLenum cap) {
    switch(cap) {
        case GL_TEXTURE_2D:
            if(TEXTURES_ENABLED != GL_FALSE) {
                TEXTURES_ENABLED = GL_FALSE;
                STATE_DIRTY = GL_TRUE;
            }
        break;
        case GL_CULL_FACE: {
            CULLING_ENABLED = GL_FALSE;
            STATE_DIRTY = GL_TRUE;
        } break;
        case GL_DEPTH_TEST: {
            if(DEPTH_TEST_ENABLED != GL_FALSE) {
                DEPTH_TEST_ENABLED = GL_FALSE;
                STATE_DIRTY = GL_TRUE;
            }
        } break;
        case GL_BLEND: {
            if(BLEND_ENABLED != GL_FALSE) {
                BLEND_ENABLED = GL_FALSE;
                STATE_DIRTY = GL_TRUE;
            }
        } break;
        case GL_SCISSOR_TEST: {
            SCISSOR_TEST_ENABLED = GL_FALSE;
            STATE_DIRTY = GL_TRUE;
        } break;
        case GL_FOG:
            if(FOG_ENABLED != GL_FALSE) {
                FOG_ENABLED = GL_FALSE;
                STATE_DIRTY = GL_TRUE;
            }
        break;
        case GL_ALPHA_TEST: {
            if(ALPHA_TEST_ENABLED != GL_FALSE) {
                ALPHA_TEST_ENABLED = GL_FALSE;
                STATE_DIRTY = GL_TRUE;
            }
        } break;
        case GL_NEARZ_CLIPPING_KOS:
            ZNEAR_CLIPPING_ENABLED = GL_FALSE;
            STATE_DIRTY = GL_TRUE;
        break;
    default:
        break;
    }
}

/* Depth Testing */
GLAPI void APIENTRY glClearDepthf(GLfloat depth) {
    glClearDepth(depth);
}

GLAPI void APIENTRY glClearDepth(GLfloat depth) {
    /* We reverse because using invW means that farther Z == lower number */
    GPUSetClearDepth(MIN(1.0f - depth, PVR_MIN_Z));
}

GLAPI void APIENTRY glDepthMask(GLboolean flag) {
    if(DEPTH_MASK_ENABLED != flag) {
        DEPTH_MASK_ENABLED = flag;
        STATE_DIRTY = GL_TRUE;
    }
}

/* Shading - Flat or Goraud */
GLAPI void APIENTRY glShadeModel(GLenum mode) {
    SHADE_MODEL = mode;
    STATE_DIRTY = GL_TRUE;
}

/* Blending */
GLAPI void APIENTRY glAlphaFunc(GLenum func, GLclampf ref) {
    GLubyte val = (GLubyte)(ref * 255.0f);
    GPUSetAlphaCutOff(val);
}

void APIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {

    if(scissor_rect.x == x &&
        scissor_rect.y == y &&
        scissor_rect.width == width &&
        scissor_rect.height == height) {
        return;
    }

    scissor_rect.x = x;
    scissor_rect.y = y;
    scissor_rect.width = width;
    scissor_rect.height = height;
    scissor_rect.applied = false;
    STATE_DIRTY = GL_TRUE; // FIXME: do we need this?

    _glApplyScissor(false);
}

/* Setup the hardware user clip rectangle.

   The minimum clip rectangle is a 32x32 area which is dependent on the tile
   size use by the tile accelerator. The PVR swithes off rendering to tiles
   outside or inside the defined rectangle dependant upon the 'clipmode'
   bits in the polygon header.

   Clip rectangles therefore must have a size that is some multiple of 32.

    glScissor(0, 0, 32, 32) allows only the 'tile' in the lower left
    hand corner of the screen to be modified and glScissor(0, 0, 0, 0)
    disallows modification to all 'tiles' on the screen.

    We call this in the following situations:

     - glEnable(GL_SCISSOR_TEST) is called
     - glScissor() is called
     - After glKosSwapBuffers()

    This ensures that a clip command is added to every vertex list
    at the right place, either when enabling the scissor test, or
    when the scissor test changes.
*/
void _glApplyScissor(bool force) {
    /* Don't do anyting if clipping is disabled */
    if(!SCISSOR_TEST_ENABLED) {
        return;
    }

    /* Don't apply if we already applied - nothing changed */
    if(scissor_rect.applied && !force) {
        return;
    }

    PVRTileClipCommand c;

    GLint miny, maxx, maxy;

    GLsizei scissor_width  = MAX(MIN(scissor_rect.width,  vid_mode->width),  0);
    GLsizei scissor_height = MAX(MIN(scissor_rect.height, vid_mode->height), 0);

    /* force the origin to the lower left-hand corner of the screen */
    miny = (vid_mode->height - scissor_height) - scissor_rect.y;
    maxx = (scissor_width + scissor_rect.x);
    maxy = (scissor_height + miny);

    /* load command structure while mapping screen coords to TA tiles */
    c.flags = GPU_CMD_USERCLIP;
    c.d1 = c.d2 = c.d3 = 0;

    uint16_t vw = vid_mode->width >> 5;
    uint16_t vh = vid_mode->height >> 5;

    c.sx = CLAMP(scissor_rect.x >> 5, 0, vw);
    c.sy = CLAMP(miny >> 5, 0, vh);
    c.ex = CLAMP((maxx >> 5) - 1, 0, vw);
    c.ey = CLAMP((maxy >> 5) - 1, 0, vh);

    aligned_vector_push_back(&OP_LIST.vector, &c, 1);
    aligned_vector_push_back(&PT_LIST.vector, &c, 1);
    aligned_vector_push_back(&TR_LIST.vector, &c, 1);

    scissor_rect.applied = true;
}

void APIENTRY glGetIntegerv(GLenum pname, GLint *params) {
    switch(pname) {
        case GL_FREE_TEXTURE_MEMORY_KOS:
            *params = _glFreeTextureMemory();
        break;
        case GL_USED_TEXTURE_MEMORY_KOS:
            *params = _glUsedTextureMemory();
        break;
        case GL_FREE_CONTIGUOUS_TEXTURE_MEMORY_KOS:
            *params = _glFreeContiguousTextureMemory();
        break;
    }
}


Viewport VIEWPORT = {
    0, 0, 640, 480, 320.0f, 240.0f, 320.0f, 240.0f
};

void _glInitMatrices() {
    glViewport(0, 0, vid_mode->width, vid_mode->height);
}

/* Set the GL viewport */
void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    VIEWPORT.x = x;
    VIEWPORT.y = y;
    VIEWPORT.width   = width;
    VIEWPORT.height  = height;
    VIEWPORT.hwidth  = ((GLfloat) VIEWPORT.width) * 0.5f;
    VIEWPORT.hheight = ((GLfloat) VIEWPORT.height) * 0.5f;
    VIEWPORT.x_plus_hwidth  = VIEWPORT.x + VIEWPORT.hwidth;
    VIEWPORT.y_plus_hheight = VIEWPORT.y + VIEWPORT.hheight;
}


GL_FORCE_INLINE void _updatePVRTextureContext(PolyContext *context, GLshort textureUnit) {
    const TextureObject *tx1 = TEXTURE_ACTIVE;

    /* Disable all texturing to start with */
    context->txr.enable = GPU_TEXTURE_DISABLE;
    context->txr2.enable = GPU_TEXTURE_DISABLE;
    context->txr2.alpha = GPU_TXRALPHA_DISABLE;

    if(!TEXTURES_ENABLED || !tx1 || !tx1->data) {
        context->txr.base = NULL;
        return;
    }

    context->txr.alpha = (BLEND_ENABLED || ALPHA_TEST_ENABLED) ? GPU_TXRALPHA_ENABLE : GPU_TXRALPHA_DISABLE;

    GLuint filter = GPU_FILTER_NEAREST;

    if(tx1->minFilter == GL_LINEAR && tx1->magFilter == GL_LINEAR) {
        filter = GPU_FILTER_BILINEAR;
    }

    if(tx1->data) {
        context->txr.enable = GPU_TEXTURE_ENABLE;
        context->txr.filter = filter;
        context->txr.width = tx1->width;
        context->txr.height = tx1->height;
        context->txr.mipmap = GL_FALSE;
        context->txr.mipmap_bias = tx1->mipmap_bias;
        
	context->txr.base = tx1->data;
        context->txr.format = tx1->color;
        context->txr.env = tx1->env;
        context->txr.uv_flip = GPU_UVFLIP_NONE;
        context->txr.uv_clamp = GPU_UVCLAMP_NONE;
    }
}

GL_FORCE_INLINE int _calc_pvr_face_culling() {
    if(!CULLING_ENABLED) {
        return GPU_CULLING_SMALL;
    } else {
        return GPU_CULLING_CW;
    }
}

GL_FORCE_INLINE void _updatePVRBlend(PolyContext* context) {
    if(BLEND_ENABLED || ALPHA_TEST_ENABLED) {
        context->gen.alpha = GPU_ALPHA_ENABLE;
    } else {
        context->gen.alpha = GPU_ALPHA_DISABLE;
    }

    context->blend.src = PVR_BLEND_SRCALPHA;
    context->blend.dst = PVR_BLEND_INVSRCALPHA;
}

void apply_poly_header(PolyHeader* header, PolyList* activePolyList) {
    TRACE();

    // Compile the header
    PolyContext ctx;
    memset(&ctx, 0, sizeof(PolyContext));

    ctx.list_type = activePolyList->list_type;
    ctx.fmt.color = GPU_CLRFMT_ARGBPACKED;
    ctx.fmt.uv = GPU_UVFMT_32BIT;
    ctx.gen.color_clamp = GPU_CLRCLAMP_DISABLE;

    ctx.gen.culling = _calc_pvr_face_culling();
    ctx.depth.comparison = DEPTH_TEST_ENABLED ? GPU_DEPTHCMP_GEQUAL : GPU_DEPTHCMP_ALWAYS;
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

    _updatePVRTextureContext(&ctx, 0);

    CompilePolyHeader(header, &ctx);

    /* Force bits 18 and 19 on to switch to 6 triangle strips */
    header->cmd |= 0xC0000;
}