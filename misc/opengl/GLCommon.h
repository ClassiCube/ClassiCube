#if defined CC_BUILD_WIN
	/* Avoid pointless includes */
	#define WIN32_LEAN_AND_MEAN
	#define NOSERVICE
	#define NOMCX
	#define NOIME
	#include <windows.h>
	#define GLAPI WINGDIAPI
#elif defined CC_BUILD_SYMBIAN && !defined __WINSCW__
	#define GLAPI IMPORT_C
	#define APIENTRY
#else
	#define GLAPI extern
	#define APIENTRY
#endif

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef signed char GLbyte;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef void GLvoid;

/* NOTE: With the OpenGL 1.1 backend "pointer" arguments are actual pointers, */
/* but with VBOs they are just offsets instead */
#ifdef CC_BUILD_GL11
typedef const void* GLpointer;
#else
typedef cc_uintptr GLpointer;
#endif

#define GL_LEQUAL                0x0203
#define GL_GREATER               0x0204

#define GL_DEPTH_BUFFER_BIT      0x00000100
#define GL_COLOR_BUFFER_BIT      0x00004000

#define GL_LINES                 0x0001
#define GL_TRIANGLES             0x0004
#define GL_QUADS                 0x0007

#define GL_ZERO                  0
#define GL_ONE                   1
#define GL_BLEND                 0x0BE2
#define GL_SRC_ALPHA             0x0302
#define GL_ONE_MINUS_SRC_ALPHA   0x0303

#define GL_UNSIGNED_BYTE         0x1401
#define GL_UNSIGNED_SHORT        0x1403
#define GL_UNSIGNED_INT          0x1405
#define GL_FLOAT                 0x1406
#define GL_RGBA                  0x1908

#define GL_FOG                   0x0B60
#define GL_FOG_DENSITY           0x0B62
#define GL_FOG_END               0x0B64
#define GL_FOG_MODE              0x0B65
#define GL_FOG_COLOR             0x0B66
#define GL_LINEAR                0x2601
#define GL_EXP                   0x0800
#define GL_EXP2                  0x0801

#define GL_CULL_FACE             0x0B44
#define GL_DEPTH_TEST            0x0B71
#define GL_MATRIX_MODE           0x0BA0
#define GL_VIEWPORT              0x0BA2
#define GL_ALPHA_TEST            0x0BC0
#define GL_SCISSOR_TEST          0x0C11

#define GL_MAX_TEXTURE_SIZE      0x0D33
#define GL_DEPTH_BITS            0x0D56

#define GL_PERSPECTIVE_CORRECTION_HINT 0x0C50
#define GL_FOG_HINT              0x0C54
#define GL_NICEST                0x1102
#define GL_COMPILE               0x1300

#define GL_MODELVIEW             0x1700
#define GL_PROJECTION            0x1701
#define GL_TEXTURE               0x1702

#define GL_VENDOR                0x1F00
#define GL_RENDERER              0x1F01
#define GL_VERSION               0x1F02
#define GL_EXTENSIONS            0x1F03

#define GL_TEXTURE_2D            0x0DE1
#define GL_NEAREST               0x2600
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_TEXTURE_MAG_FILTER    0x2800
#define GL_TEXTURE_MIN_FILTER    0x2801

#define GL_VERTEX_ARRAY          0x8074
#define GL_COLOR_ARRAY           0x8076
#define GL_TEXTURE_COORD_ARRAY   0x8078

/* Not present in gl.h on Windows (only up to OpenGL 1.1) */
#define GL_ARRAY_BUFFER          0x8892
#define GL_ELEMENT_ARRAY_BUFFER  0x8893
#define GL_STATIC_DRAW           0x88E4
#define GL_DYNAMIC_DRAW          0x88E8

#define GL_FRAGMENT_SHADER       0x8B30
#define GL_VERTEX_SHADER         0x8B31
#define GL_COMPILE_STATUS        0x8B81
#define GL_LINK_STATUS           0x8B82
#define GL_INFO_LOG_LENGTH       0x8B84

#define GL_OUT_OF_MEMORY         0x0505
