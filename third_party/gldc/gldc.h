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

/* Scissor box */
#define GL_SCISSOR_TEST     0x0008      /* capability bit */

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

#define GLushort unsigned short
#define GLuint   unsigned int
#define GLenum   unsigned int
#define GLubyte  unsigned char
#define GLboolean  unsigned char

#define GL_FALSE   0
#define GL_TRUE    1

#define GLAPI extern
#define APIENTRY
#define GL_NEARZ_CLIPPING_KOS                       0xEEFA

/* Depth Testing */
GLAPI void glClearDepth(float depth);

GLAPI GLuint gldcGenTexture(void);
GLAPI void   gldcDeleteTexture(GLuint texture);
GLAPI void   gldcBindTexture(GLuint texture);

/* Loads texture from SH4 RAM into PVR VRAM */
GLAPI int  gldcAllocTexture(int w, int h, int format);
GLAPI void gldcGetTexture(void** data, int* width, int* height);

GLAPI void glViewport(int x, int y, int width, int height);

GLAPI void glScissor(int x, int y, int width, int height);


/* Initialize the GL pipeline. GL will initialize the PVR. */
GLAPI void glKosInit();
GLAPI void glKosSwapBuffers();

/* Memory allocation extension (GL_KOS_texture_memory_management) */
GLAPI void glDefragmentTextureMemory_KOS(void);

__END_DECLS

#endif /* !__GL_GL_H */
