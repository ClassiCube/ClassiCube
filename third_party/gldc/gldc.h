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

/* Texture Environment */
#define GL_TEXTURE_ENV_MODE 0x2200
#define GL_REPLACE          0x1E01
#define GL_MODULATE         0x2100
#define GL_DECAL            0x2101

/* TextureMagFilter */
#define GL_NEAREST                      0x2600
#define GL_LINEAR                       0x2601

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
#define GLubyte  unsigned char
#define GLboolean  unsigned char
#define GL_FALSE   0
#define GL_TRUE    1

#define GLAPI extern
#define APIENTRY

/* Clear Caps */
GLAPI void glClear(GLuint mode);

/* Depth Testing */
GLAPI void glClearDepth(GLfloat depth);

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

GLAPI void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

GLAPI void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);


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
GLAPI void glKosSwapBuffers();

typedef struct {
    /* If GL_TRUE, enables pvr autosorting, this *will* break glDepthFunc/glDepthTest */
    GLboolean autosort_enabled;

    /* If GL_TRUE, enables the PVR FSAA */
    GLboolean fsaa_enabled;
} GLdcConfig;


/* Memory allocation extension (GL_KOS_texture_memory_management) */
GLAPI void glDefragmentTextureMemory_KOS(void);

__END_DECLS

#endif /* !__GL_GL_H */