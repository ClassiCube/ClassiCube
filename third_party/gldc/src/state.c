#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "private.h"


static struct {
    GLboolean is_dirty;

/* We can't just use the GL_CONTEXT for this state as the two
 * GL states are combined, so we store them separately and then
 * calculate the appropriate PVR state from them. */
    GLenum depth_func;
    GLboolean depth_test_enabled;
    GLenum cull_face;
    GLenum front_face;
    GLboolean culling_enabled;
    GLboolean znear_clipping_enabled;
    GLboolean alpha_test_enabled;
    GLboolean scissor_test_enabled;
    GLboolean fog_enabled;
    GLboolean depth_mask_enabled;

    struct {
        GLint x;
        GLint y;
        GLsizei width;
        GLsizei height;
        GLboolean applied;
    } scissor_rect;

    GLenum blend_sfactor;
    GLenum blend_dfactor;
    GLboolean blend_enabled;

    GLenum shade_model;
} GPUState = {
    .is_dirty = GL_TRUE,
    .depth_func = GL_LESS,
    .depth_test_enabled = GL_FALSE,
    .cull_face = GL_BACK,
    .front_face = GL_CCW,
    .culling_enabled = GL_FALSE,
    .znear_clipping_enabled = GL_TRUE,
    .alpha_test_enabled = GL_FALSE,
    .scissor_test_enabled = GL_FALSE,
    .fog_enabled = GL_FALSE,
    .depth_mask_enabled = GL_FALSE,
    .scissor_rect = {0, 0, 640, 480, false},
    .blend_sfactor = GL_ONE,
    .blend_dfactor = GL_ZERO,
    .blend_enabled = GL_FALSE,
    .shade_model = GL_SMOOTH
};

void _glGPUStateMarkClean() {
    GPUState.is_dirty = GL_FALSE;
}

void _glGPUStateMarkDirty() {
    GPUState.is_dirty = GL_TRUE;
}

GLboolean _glGPUStateIsDirty() {
    return GPUState.is_dirty;
}

GLboolean _glIsDepthTestEnabled() {
    return GPUState.depth_test_enabled;
}

GLenum _glGetDepthFunc() {
    return GPUState.depth_func;
}

GLboolean _glIsDepthWriteEnabled() {
    return GPUState.depth_mask_enabled;
}

GLenum _glGetShadeModel() {
    return GPUState.shade_model;
}

GLboolean _glIsBlendingEnabled() {
    return GPUState.blend_enabled;
}

GLboolean _glIsAlphaTestEnabled() {
    return GPUState.alpha_test_enabled;
}

GLboolean _glIsCullingEnabled() {
    return GPUState.culling_enabled;
}

GLenum _glGetCullFace() {
    return GPUState.cull_face;
}

GLenum _glGetFrontFace() {
    return GPUState.front_face;
}

GLboolean _glIsFogEnabled() {
    return GPUState.fog_enabled;
}

GLboolean _glIsScissorTestEnabled() {
    return GPUState.scissor_test_enabled;
}

GLboolean _glNearZClippingEnabled() {
    return GPUState.znear_clipping_enabled;
}

void _glApplyScissor(bool force);

GLenum _glGetBlendSourceFactor() {
    return GPUState.blend_sfactor;
}

GLenum _glGetBlendDestFactor() {
    return GPUState.blend_dfactor;
}


GLboolean _glCheckValidEnum(GLint param, GLint* values, const char* func) {
    GLubyte found = 0;
    while(*values != 0) {
        if(*values == param) {
            found++;
            break;
        }
        values++;
    }

    if(!found) {
        _glKosThrowError(GL_INVALID_ENUM, func);
        return GL_TRUE;
    }

    return GL_FALSE;
}

GLboolean TEXTURES_ENABLED = GL_FALSE;

void _glUpdatePVRTextureContext(PolyContext *context, GLshort textureUnit) {
    const TextureObject *tx1 = TEXTURE_ACTIVE;

    /* Disable all texturing to start with */
    context->txr.enable = GPU_TEXTURE_DISABLE;
    context->txr2.enable = GPU_TEXTURE_DISABLE;
    context->txr2.alpha = GPU_TXRALPHA_DISABLE;

    if(!TEXTURES_ENABLED || !tx1 || !tx1->data) {
        context->txr.base = NULL;
        return;
    }

    context->txr.alpha = (GPUState.blend_enabled || GPUState.alpha_test_enabled) ? GPU_TXRALPHA_ENABLE : GPU_TXRALPHA_DISABLE;

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

void _glInitContext() {
    const VideoMode* mode = GetVideoMode();

    GPUState.scissor_rect.x = 0;
    GPUState.scissor_rect.y = 0;
    GPUState.scissor_rect.width = mode->width;
    GPUState.scissor_rect.height = mode->height;

    glClearDepth(1.0f);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
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
                GPUState.is_dirty = GL_TRUE;
            }
        break;
        case GL_CULL_FACE: {
            if(GPUState.culling_enabled != GL_TRUE) {
                GPUState.culling_enabled = GL_TRUE;
                GPUState.is_dirty = GL_TRUE;
            }

        } break;
        case GL_DEPTH_TEST: {
            if(GPUState.depth_test_enabled != GL_TRUE) {
                GPUState.depth_test_enabled = GL_TRUE;
                GPUState.is_dirty = GL_TRUE;
            }
        } break;
        case GL_BLEND: {
            if(GPUState.blend_enabled != GL_TRUE) {
                GPUState.blend_enabled = GL_TRUE;
                GPUState.is_dirty = GL_TRUE;
            }
        } break;
        case GL_SCISSOR_TEST: {
            if(GPUState.scissor_test_enabled != GL_TRUE) {
                GPUState.scissor_test_enabled = GL_TRUE;
                GPUState.is_dirty = GL_TRUE;
            }
        } break;
        case GL_FOG:
            if(GPUState.fog_enabled != GL_TRUE) {
                GPUState.fog_enabled = GL_TRUE;
                GPUState.is_dirty = GL_TRUE;
            }
        break;
        case GL_ALPHA_TEST: {
            if(GPUState.alpha_test_enabled != GL_TRUE) {
                GPUState.alpha_test_enabled = GL_TRUE;
                GPUState.is_dirty = GL_TRUE;
            }
        } break;
        case GL_NEARZ_CLIPPING_KOS:
            if(GPUState.znear_clipping_enabled != GL_TRUE) {
                GPUState.znear_clipping_enabled = GL_TRUE;
                GPUState.is_dirty = GL_TRUE;
            }
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
                GPUState.is_dirty = GL_TRUE;
            }
        break;
        case GL_CULL_FACE: {
            if(GPUState.culling_enabled != GL_FALSE) {
                GPUState.culling_enabled = GL_FALSE;
                GPUState.is_dirty = GL_TRUE;
            }

        } break;
        case GL_DEPTH_TEST: {
            if(GPUState.depth_test_enabled != GL_FALSE) {
                GPUState.depth_test_enabled = GL_FALSE;
                GPUState.is_dirty = GL_TRUE;
            }
        } break;
        case GL_BLEND: {
            if(GPUState.blend_enabled != GL_FALSE) {
                GPUState.blend_enabled = GL_FALSE;
                GPUState.is_dirty = GL_TRUE;
            }
        } break;
        case GL_SCISSOR_TEST: {
            if(GPUState.scissor_test_enabled != GL_FALSE) {
                GPUState.scissor_test_enabled = GL_FALSE;
                GPUState.is_dirty = GL_TRUE;
            }
        } break;
        case GL_FOG:
            if(GPUState.fog_enabled != GL_FALSE) {
                GPUState.fog_enabled = GL_FALSE;
                GPUState.is_dirty = GL_TRUE;
            }
        break;
        case GL_ALPHA_TEST: {
            if(GPUState.alpha_test_enabled != GL_FALSE) {
                GPUState.alpha_test_enabled = GL_FALSE;
                GPUState.is_dirty = GL_TRUE;
            }
        } break;
        case GL_NEARZ_CLIPPING_KOS:
            if(GPUState.znear_clipping_enabled != GL_FALSE) {
                GPUState.znear_clipping_enabled = GL_FALSE;
                GPUState.is_dirty = GL_TRUE;
            }
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
    if(GPUState.depth_mask_enabled != flag) {
        GPUState.depth_mask_enabled = flag;
        GPUState.is_dirty = GL_TRUE;
    }
}

GLAPI void APIENTRY glDepthFunc(GLenum func) {
    if(GPUState.depth_func != func) {
        GPUState.depth_func = func;
        GPUState.is_dirty = GL_TRUE;
    }
}

/* Culling */
GLAPI void APIENTRY glFrontFace(GLenum mode) {
    if(GPUState.front_face != mode) {
        GPUState.front_face = mode;
        GPUState.is_dirty = GL_TRUE;
    }
}

GLAPI void APIENTRY glCullFace(GLenum mode) {
    if(GPUState.cull_face != mode) {
        GPUState.cull_face = mode;
        GPUState.is_dirty = GL_TRUE;
    }
}

/* Shading - Flat or Goraud */
GLAPI void APIENTRY glShadeModel(GLenum mode) {
    if(GPUState.shade_model != mode) {
        GPUState.shade_model = mode;
        GPUState.is_dirty = GL_TRUE;
    }
}

/* Blending */
GLAPI void APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor) {
    if(GPUState.blend_dfactor != dfactor || GPUState.blend_sfactor != sfactor) {
        GPUState.blend_sfactor = sfactor;
        GPUState.blend_dfactor = dfactor;
        GPUState.is_dirty = GL_TRUE;
    }
}


GLAPI void APIENTRY glAlphaFunc(GLenum func, GLclampf ref) {
    GLubyte val = (GLubyte)(ref * 255.0f);
    GPUSetAlphaCutOff(val);
}

void APIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {

    if(GPUState.scissor_rect.x == x &&
        GPUState.scissor_rect.y == y &&
        GPUState.scissor_rect.width == width &&
        GPUState.scissor_rect.height == height) {
        return;
    }

    GPUState.scissor_rect.x = x;
    GPUState.scissor_rect.y = y;
    GPUState.scissor_rect.width = width;
    GPUState.scissor_rect.height = height;
    GPUState.scissor_rect.applied = false;
    GPUState.is_dirty = GL_TRUE; // FIXME: do we need this?

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
    if(!GPUState.scissor_test_enabled) {
        return;
    }

    /* Don't apply if we already applied - nothing changed */
    if(GPUState.scissor_rect.applied && !force) {
        return;
    }

    PVRTileClipCommand c;

    GLint miny, maxx, maxy;

    const VideoMode* vid_mode = GetVideoMode();

    GLsizei scissor_width = MAX(MIN(GPUState.scissor_rect.width, vid_mode->width), 0);
    GLsizei scissor_height = MAX(MIN(GPUState.scissor_rect.height, vid_mode->height), 0);

    /* force the origin to the lower left-hand corner of the screen */
    miny = (vid_mode->height - scissor_height) - GPUState.scissor_rect.y;
    maxx = (scissor_width + GPUState.scissor_rect.x);
    maxy = (scissor_height + miny);

    /* load command structure while mapping screen coords to TA tiles */
    c.flags = GPU_CMD_USERCLIP;
    c.d1 = c.d2 = c.d3 = 0;

    uint16_t vw = vid_mode->width >> 5;
    uint16_t vh = vid_mode->height >> 5;

    c.sx = CLAMP(GPUState.scissor_rect.x >> 5, 0, vw);
    c.sy = CLAMP(miny >> 5, 0, vh);
    c.ex = CLAMP((maxx >> 5) - 1, 0, vw);
    c.ey = CLAMP((maxy >> 5) - 1, 0, vh);

    aligned_vector_push_back(&_glOpaquePolyList()->vector, &c, 1);
    aligned_vector_push_back(&_glPunchThruPolyList()->vector, &c, 1);
    aligned_vector_push_back(&_glTransparentPolyList()->vector, &c, 1);

    GPUState.scissor_rect.applied = true;
}

void APIENTRY glGetIntegerv(GLenum pname, GLint *params) {
    switch(pname) {
        case GL_MAX_TEXTURE_SIZE:
            *params = MAX_TEXTURE_SIZE;
        break;
        case GL_TEXTURE_FREE_MEMORY_ATI:
        case GL_FREE_TEXTURE_MEMORY_KOS:
            *params = _glFreeTextureMemory();
        break;
        case GL_USED_TEXTURE_MEMORY_KOS:
            *params = _glUsedTextureMemory();
        break;
        case GL_FREE_CONTIGUOUS_TEXTURE_MEMORY_KOS:
            *params = _glFreeContiguousTextureMemory();
        break;
    default:
        _glKosThrowError(GL_INVALID_ENUM, __func__);
        break;
    }
}

const GLubyte *glGetString(GLenum name) {
    switch(name) {
        case GL_VENDOR:
            return (const GLubyte*) "KallistiOS / Kazade";

        case GL_RENDERER:
            return (const GLubyte*) "PowerVR2 CLX2 100mHz";
    }

    return (const GLubyte*) "GL_KOS_ERROR: ENUM Unsupported\n";
}
