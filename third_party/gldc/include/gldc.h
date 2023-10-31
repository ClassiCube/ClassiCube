/* KallistiGL for KallistiOS ##version##

   libgl/gl.h
   Copyright (C) 2013-2014 Josh "PH3NOM" Pearson
   Copyright (C) 2014, 2016 Lawrence Sebald

   Some functionality adapted from the original KOS libgl:
   Copyright (C) 2001 Dan Potter
   Copyright (C) 2002 Benoit Miller

   This API implements much but not all of the OpenGL 1.1 for KallistiOS.
*/

#ifndef __GL_GL_H
#define __GL_GL_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <math.h>

/* Primitive Types taken from GL for compatability */
/* Not all types are implemented in Open GL DC V.1.0 */
#define GL_POINTS                               0x0000
#define GL_LINES                                0x0001
#define GL_TRIANGLES                            0x0004
#define GL_TRIANGLE_STRIP                       0x0005
#define GL_QUADS                                0x0007

/* Scissor box */
#define GL_SCISSOR_TEST     0x0008      /* capability bit */
#define GL_SCISSOR_BOX      0x0C10

/* Depth buffer */
#define GL_NEVER              0x0200
#define GL_LESS               0x0201
#define GL_EQUAL              0x0202
#define GL_LEQUAL             0x0203
#define GL_GREATER            0x0204
#define GL_NOTEQUAL           0x0205
#define GL_GEQUAL             0x0206
#define GL_ALWAYS             0x0207
#define GL_DEPTH_TEST         0x0B71
#define GL_DEPTH_BITS         0x0D56
#define GL_DEPTH_FUNC         0x0B74
#define GL_DEPTH_WRITEMASK    0x0B72

/* Blending */
#define GL_BLEND              0x0BE2 /* capability bit */

/* Misc texture constants */
#define GL_TEXTURE_2D           0x0001      /* capability bit */

/* Texture Environment */
#define GL_TEXTURE_ENV_MODE 0x2200
#define GL_REPLACE          0x1E01
#define GL_MODULATE         0x2100
#define GL_DECAL            0x2101

/* TextureMagFilter */
#define GL_NEAREST                      0x2600
#define GL_LINEAR                       0x2601

/* Fog */
#define GL_FOG              0x0004      /* capability bit */

#define GL_FRONT_AND_BACK                       0x0408
#define GL_FRONT                                0x0404
#define GL_BACK                                 0x0405

#define GL_SHADE_MODEL      0x0b54
#define GL_FLAT         0x1d00
#define GL_SMOOTH       0x1d01

/* Data types */
#define GL_BYTE                                 0x1400
#define GL_UNSIGNED_BYTE                        0x1401
#define GL_SHORT                                0x1402
#define GL_UNSIGNED_SHORT                       0x1403
#define GL_INT                                  0x1404
#define GL_UNSIGNED_INT                         0x1405
#define GL_FLOAT                                0x1406

/* ErrorCode */
#define GL_INVALID_VALUE                  0x0501
#define GL_OUT_OF_MEMORY                  0x0505

#define GL_UNSIGNED_SHORT_4_4_4_4       0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1       0x8034
#define GL_UNSIGNED_SHORT_5_6_5         0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV     0x8364
#define GL_UNSIGNED_SHORT_4_4_4_4_REV   0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV   0x8366

#define GL_RGBA                           0x1908

#define GL_CULL_FACE				0x0B44

#define GLbyte   char
#define GLshort  short
#define GLint    int
#define GLfloat  float
#define GLvoid   void
#define GLushort unsigned short
#define GLuint   unsigned int
#define GLenum   unsigned int
#define GLsizei  unsigned int
#define GLclampf float
#define GLclampd float
#define GLubyte  unsigned char
#define GLboolean  unsigned char
#define GL_FALSE   0
#define GL_TRUE    1

/* Stubs for portability */
#define GL_ALPHA_TEST                     0x0BC0

#define GLAPI extern
#define APIENTRY

/* Start Submission of Primitive Data */
/* Currently Supported Primitive Types:
   -GL_POINTS   ( does NOT work with glDrawArrays )( ZClipping NOT supported )
   -GL_TRIANGLES        ( works with glDrawArrays )( ZClipping supported )
   -GL_TRIANLGLE_STRIP  ( works with glDrawArrays )( ZClipping supported )
   -GL_QUADS            ( works with glDrawArrays )( ZClipping supported )
**/

/* Enable / Disable Capability */
/* Currently Supported Capabilities:
        GL_TEXTURE_2D
        GL_BLEND
        GL_DEPTH_TEST
        GL_LIGHTING
        GL_SCISSOR_TEST
        GL_FOG
        GL_CULL_FACE
        GL_KOS_NEARZ_CLIPPING
        GL_KOS_TEXTURE_MATRIX
*/
GLAPI void glEnable(GLenum cap);
GLAPI void glDisable(GLenum cap);

/* Clear Caps */
GLAPI void glClear(GLuint mode);

/* Depth Testing */
GLAPI void glClearDepth(GLfloat depth);
GLAPI void glClearDepthf(GLfloat depth);
GLAPI void glDepthMask(GLboolean flag);
GLAPI void glDepthFunc(GLenum func);
GLAPI void glDepthRange(GLclampf n, GLclampf f);
GLAPI void glDepthRangef(GLclampf n, GLclampf f);

/* Shading - Flat or Goraud */
GLAPI void glShadeModel(GLenum mode);

GLAPI GLuint gldcGenTexture(void);
GLAPI void   gldcDeleteTexture(GLuint texture);
GLAPI void   gldcBindTexture(GLuint texture);

/* Loads texture from SH4 RAM into PVR VRAM applying color conversion if needed */
/* internalformat must be one of the following constants:
     GL_RGB
     GL_RGBA

   format must be the same as internalformat

   if internal format is GL_RGBA, type must be one of the following constants:
     GL_BYTE
     GL_UNSIGNED_BYTE
     GL_SHORT
     GL_UNSIGNED_SHORT
     GL_FLOAT
     GL_UNSIGNED_SHORT_4_4_4_4
     GL_UNSIGNED_SHORT_4_4_4_4_TWID
     GL_UNSIGNED_SHORT_1_5_5_5
     GL_UNSIGNED_SHORT_1_5_5_5_TWID
 */                      
GLAPI int  gldcAllocTexture(GLsizei w, GLsizei h, GLenum format, GLenum type);
GLAPI void gldcGetTexture(GLvoid** data, GLsizei* width, GLsizei* height);


/* GL Array API - Only GL_TRIANGLES, GL_TRIANGLE_STRIP, and GL_QUADS are supported */
GLAPI void gldcVertexPointer(GLsizei stride, const GLvoid *pointer);

/* Array Data Submission */
GLAPI void glDrawArrays(GLenum mode, GLint first, GLsizei count);
GLAPI void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);

/* Transformation / Matrix Functions */

GLAPI void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

GLAPI void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);

/* glGet Functions */
GLAPI void glGetIntegerv(GLenum pname, GLint *params);

GLAPI void glAlphaFunc(GLenum func, GLclampf ref);


/*
 * Dreamcast specific compressed + twiddled formats.
 * We use constants from the range 0xEEE0 onwards
 * to avoid trampling any real GL constants (this is in the middle of the
 * any_vendor_future_use range defined in the GL enum.spec file.
*/
#define GL_UNSIGNED_SHORT_5_6_5_TWID_KOS            0xEEE0
#define GL_UNSIGNED_SHORT_1_5_5_5_REV_TWID_KOS      0xEEE2
#define GL_UNSIGNED_SHORT_4_4_4_4_REV_TWID_KOS      0xEEE3

#define GL_NEARZ_CLIPPING_KOS                       0xEEFA


/* Initialize the GL pipeline. GL will initialize the PVR. */
GLAPI void glKosInit();

typedef struct {
    /* If GL_TRUE, enables pvr autosorting, this *will* break glDepthFunc/glDepthTest */
    GLboolean autosort_enabled;

    /* If GL_TRUE, enables the PVR FSAA */
    GLboolean fsaa_enabled;

    /* Initial capacity of each of the OP, TR and PT lists in vertices */
    GLuint initial_op_capacity;
    GLuint initial_tr_capacity;
    GLuint initial_pt_capacity;

} GLdcConfig;


GLAPI void glKosInitConfig(GLdcConfig* config);

/* Usage:
 *
 * GLdcConfig config;
 * glKosInitConfig(&config);
 *
 * config.autosort_enabled = GL_TRUE;
 *
 * glKosInitEx(&config);
 */
GLAPI void glKosInitEx(GLdcConfig* config);
GLAPI void glKosSwapBuffers();
\

/* Memory allocation extension (GL_KOS_texture_memory_management) */
GLAPI void glDefragmentTextureMemory_KOS(void);

#define GL_FREE_TEXTURE_MEMORY_KOS                  0xEF3D
#define GL_USED_TEXTURE_MEMORY_KOS                  0xEF3E
#define GL_FREE_CONTIGUOUS_TEXTURE_MEMORY_KOS       0xEF3F

__END_DECLS

#endif /* !__GL_GL_H */
