#include "Core.h"
#if defined CC_BUILD_GL && !defined CC_BUILD_GLMODERN
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"
/* The OpenGL backend is a bit of a mess, since it's really 2 backends in one:
 * - OpenGL 1.1 (completely lacking GPU, fallbacks to say Windows built-in software rasteriser)
 * - OpenGL 1.5 or OpenGL 1.2 + GL_ARB_vertex_buffer_object (default desktop backend)
*/

#if defined CC_BUILD_WIN
/* Avoid pointless includes */
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#include <windows.h>
#define GLAPI WINGDIAPI
#else
#define GLAPI extern
#define APIENTRY
#endif
/* === BEGIN OPENGL HEADERS === */
typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef signed char GLbyte;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef void GLvoid;

#define GL_LEQUAL                0x0203
#define GL_GREATER               0x0204

#define GL_DEPTH_BUFFER_BIT      0x00000100
#define GL_COLOR_BUFFER_BIT      0x00004000

#define GL_LINES                 0x0001
#define GL_TRIANGLES             0x0004

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
#define GL_MAX_TEXTURE_SIZE      0x0D33
#define GL_DEPTH_BITS            0x0D56

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

GLAPI void APIENTRY glAlphaFunc(GLenum func, GLfloat ref);
GLAPI void APIENTRY glBindTexture(GLenum target, GLuint texture);
GLAPI void APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor);
GLAPI void APIENTRY glCallList(GLuint list);
GLAPI void APIENTRY glClear(GLuint mask);
GLAPI void APIENTRY glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
GLAPI void APIENTRY glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
GLAPI void APIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
GLAPI void APIENTRY glDeleteLists(GLuint list, GLsizei range);
GLAPI void APIENTRY glDeleteTextures(GLsizei n, const GLuint* textures);
GLAPI void APIENTRY glDepthFunc(GLenum func);
GLAPI void APIENTRY glDepthMask(GLboolean flag);
GLAPI void APIENTRY glDisable(GLenum cap);
GLAPI void APIENTRY glDisableClientState(GLenum array);
GLAPI void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count);
GLAPI void APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
GLAPI void APIENTRY glEnable(GLenum cap);
GLAPI void APIENTRY glEnableClientState(GLenum array);
GLAPI void APIENTRY glEndList(void);
GLAPI void APIENTRY glFogf(GLenum pname, GLfloat param);
GLAPI void APIENTRY glFogfv(GLenum pname, const GLfloat* params);
GLAPI void APIENTRY glFogi(GLenum pname, GLint param);
GLAPI void APIENTRY glFogiv(GLenum pname, const GLint* params);
GLAPI GLuint APIENTRY glGenLists(GLsizei range);
GLAPI void APIENTRY   glGenTextures(GLsizei n, GLuint* textures);
GLAPI GLenum APIENTRY glGetError(void);
GLAPI void APIENTRY glGetFloatv(GLenum pname, GLfloat* params);
GLAPI void APIENTRY glGetIntegerv(GLenum pname, GLint* params);
GLAPI const GLubyte* APIENTRY glGetString(GLenum name);
GLAPI void APIENTRY glHint(GLenum target, GLenum mode);
GLAPI void APIENTRY glLoadIdentity(void);
GLAPI void APIENTRY glLoadMatrixf(const GLfloat* m);
GLAPI void APIENTRY glMatrixMode(GLenum mode);
GLAPI void APIENTRY glNewList(GLuint list, GLenum mode);
GLAPI void APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels);
GLAPI void APIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
GLAPI void APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
GLAPI void APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param);
GLAPI void APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels);
GLAPI void APIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
GLAPI void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
/* === END OPENGL HEADERS === */


#if defined CC_BUILD_GL11
static GLuint activeList;
#define gl_DYNAMICLISTID 1234567891
static void* dynamicListData;
static cc_uint16 gl_indices[GFX_MAX_INDICES];
#else
/* OpenGL functions use stdcall instead of cdecl on Windows */
static void (APIENTRY *_glBindBuffer)(GLenum target, GfxResourceID buffer); /* NOTE: buffer is actually a GLuint in OpenGL */
static void (APIENTRY *_glDeleteBuffers)(GLsizei n, const GLuint *buffers);
static void (APIENTRY *_glGenBuffers)(GLsizei n, GLuint *buffers);
static void (APIENTRY *_glBufferData)(GLenum target, cc_uintptr size, const GLvoid* data, GLenum usage);
static void (APIENTRY *_glBufferSubData)(GLenum target, cc_uintptr offset, cc_uintptr size, const GLvoid* data);
#endif
#include "_GLShared.h"

typedef void (*GL_SetupVBFunc)(void);
typedef void (*GL_SetupVBRangeFunc)(int startVertex);
static GL_SetupVBFunc gfx_setupVBFunc;
static GL_SetupVBRangeFunc gfx_setupVBRangeFunc;
/* Current format and size of vertices */
static int gfx_stride, gfx_format = -1;

#if defined CC_BUILD_WIN && !defined CC_BUILD_GL11
/* Note the following about calling OpenGL functions on Windows */
/*  1) wglGetProcAddress returns a context specific address */
/*  2) dllimport functions are implemented using indirect function pointers */
/*     https://web.archive.org/web/20080321171626/http://blogs.msdn.com/oldnewthing/archive/2006/07/20/672695.aspx */
/*     https://web.archive.org/web/20071016185327/http://blogs.msdn.com/oldnewthing/archive/2006/07/27/680250.aspx */
/* Therefore one layer of indirection can be avoided by calling wglGetProcAddress functions instead */
/*  e.g. if _glDrawElements = wglGetProcAddress("glDrawElements") */
/*    call [glDrawElements]  --> opengl32.dll thunk--> GL driver thunk --> GL driver implementation */
/*    call [_glDrawElements] --> GL driver thunk --> GL driver implementation */

static void (APIENTRY *_glColorPointer)(GLint size,    GLenum type, GLsizei stride, const GLvoid* pointer);
static void (APIENTRY *_glDrawElements)(GLenum mode, GLsizei count,    GLenum type, const GLvoid* indices);
static void (APIENTRY *_glTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
static void (APIENTRY *_glVertexPointer)(GLint size,   GLenum type, GLsizei stride, const GLvoid* pointer);

static const struct DynamicLibSym coreFuncs[] = {
	DynamicLib_Sym2("glColorPointer",    glColorPointer),
	DynamicLib_Sym2("glTexCoordPointer", glTexCoordPointer), DynamicLib_Sym2("glDrawElements", glDrawElements),
	DynamicLib_Sym2("glVertexPointer",  glVertexPointer)
};
static void LoadCoreFuncs(void) {
	GLContext_GetAll(coreFuncs, Array_Elems(coreFuncs));
}
#else
#define _glColorPointer    glColorPointer
#define _glDrawElements    glDrawElements
#define _glTexCoordPointer glTexCoordPointer
#define _glVertexPointer   glVertexPointer
#endif


/*########################################################################################################################*
*-------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
#ifndef CC_BUILD_GL11
static GfxResourceID GL_GenBuffer(void) {
	GLuint id;
	_glGenBuffers(1, &id);
	return id;
}

static void GL_DelBuffer(GfxResourceID id) {
	GLuint gl_id = (GLuint)id;
	_glDeleteBuffers(1, &gl_id);
}

static GfxResourceID (*_genBuffer)(void)    = GL_GenBuffer;
static void (*_delBuffer)(GfxResourceID id) = GL_DelBuffer;

GfxResourceID Gfx_CreateIb(void* indices, int indicesCount) {
	GfxResourceID id = _genBuffer();
	cc_uint32 size   = indicesCount * 2;

	_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
	_glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
	return id;
}

void Gfx_BindIb(GfxResourceID ib) { _glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib); }

void Gfx_DeleteIb(GfxResourceID* ib) {
	GfxResourceID id = *ib;
	if (!id) return;
	_delBuffer(id);
	*ib = 0;
}
#else
GfxResourceID Gfx_CreateIb(void* indices, int indicesCount) { return 0; }
void Gfx_BindIb(GfxResourceID ib) { }
void Gfx_DeleteIb(GfxResourceID* ib) { }
#endif


/*########################################################################################################################*
*------------------------------------------------------Vertex buffers-----------------------------------------------------*
*#########################################################################################################################*/
#ifndef CC_BUILD_GL11
GfxResourceID Gfx_CreateVb(VertexFormat fmt, int count) {
	GfxResourceID id = _genBuffer();
	_glBindBuffer(GL_ARRAY_BUFFER, id);
	return id;
}

void Gfx_BindVb(GfxResourceID vb) { _glBindBuffer(GL_ARRAY_BUFFER, vb); }

void Gfx_DeleteVb(GfxResourceID* vb) {
	GfxResourceID id = *vb;
	if (!id) return;
	_delBuffer(id);
	*vb = 0;
}

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return FastAllocTempMem(count * strideSizes[fmt]);
}

void Gfx_UnlockVb(GfxResourceID vb) {
	_glBufferData(GL_ARRAY_BUFFER, tmpSize, tmpData, GL_STATIC_DRAW);
}
#else
GfxResourceID Gfx_CreateVb(VertexFormat fmt, int count) { return glGenLists(1); }
void Gfx_BindVb(GfxResourceID vb) { activeList = (GLuint)vb; }

void Gfx_DeleteVb(GfxResourceID* vb) {
	GLuint id = (GLuint)(*vb);
	if (id) glDeleteLists(id, 1);
	*vb = 0;
}

static void UpdateDisplayList(GLuint list, void* vertices, VertexFormat fmt, int count) {
	/* We need to restore client state afer building the list */
	int realFormat = gfx_format;
	void* dyn_data = dynamicListData;
	Gfx_SetVertexFormat(fmt);
	dynamicListData = vertices;

	glNewList(list, GL_COMPILE);
	gfx_setupVBFunc();
	glDrawElements(GL_TRIANGLES, ICOUNT(count), GL_UNSIGNED_SHORT, gl_indices);
	glEndList();

	Gfx_SetVertexFormat(realFormat);
	dynamicListData = dyn_data;
}

/* NOTE! Building chunk in Builder.c relies on vb being ignored */
/* If that changes, you must fix Builder.c to properly call Gfx_LockVb */
static VertexFormat tmpFormat;
static int tmpCount;
void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	tmpFormat = fmt;
	tmpCount  = count;
	return FastAllocTempMem(count * strideSizes[fmt]);
}

void Gfx_UnlockVb(GfxResourceID vb) {
	UpdateDisplayList((GLuint)vb, tmpData, tmpFormat, tmpCount);
}

GfxResourceID Gfx_CreateVb2(void* vertices, VertexFormat fmt, int count) {
	GLuint list = glGenLists(1);
	UpdateDisplayList(list, vertices, fmt, count);
	return list;
}
#endif


/*########################################################################################################################*
*--------------------------------------------------Dynamic vertex buffers-------------------------------------------------*
*#########################################################################################################################*/
#ifndef CC_BUILD_GL11
GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices) {
	GfxResourceID id;
	cc_uint32 size;
	if (Gfx.LostContext) return 0;

	id   = _genBuffer();
	size = maxVertices * strideSizes[fmt];

	_glBindBuffer(GL_ARRAY_BUFFER, id);
	_glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
	return id;
}

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return FastAllocTempMem(count * strideSizes[fmt]);
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) {
	_glBindBuffer(GL_ARRAY_BUFFER, vb);
	_glBufferSubData(GL_ARRAY_BUFFER, 0, tmpSize, tmpData);
}

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	cc_uint32 size = vCount * gfx_stride;
	_glBindBuffer(GL_ARRAY_BUFFER, vb);
	_glBufferSubData(GL_ARRAY_BUFFER, 0, size, vertices);
}
#else
GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices) { 
	return (GfxResourceID)Mem_Alloc(maxVertices, strideSizes[fmt], "creating dynamic vb");
}

void Gfx_BindDynamicVb(GfxResourceID vb) {
	activeList      = gl_DYNAMICLISTID;
	dynamicListData = (void*)vb;
}

void Gfx_DeleteDynamicVb(GfxResourceID* vb) {
	void* addr = (void*)(*vb);
	if (addr) Mem_Free(addr);
	*vb = 0;
}

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) { return (void*)vb; }
void  Gfx_UnlockDynamicVb(GfxResourceID vb) { Gfx_BindDynamicVb(vb); }

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	Gfx_BindDynamicVb(vb);
	Mem_Copy((void*)vb, vertices, vCount * gfx_stride);
}
#endif


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_BindTexture(GfxResourceID texId) {
	glBindTexture(GL_TEXTURE_2D, (GLuint)texId);
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gfx_fogColor;
static float gfx_fogEnd = -1.0f, gfx_fogDensity = -1.0f;
static int gfx_fogMode  = -1;

void Gfx_SetFog(cc_bool enabled) {
	gfx_fogEnabled = enabled;
	if (enabled) { glEnable(GL_FOG); } else { glDisable(GL_FOG); }
}

void Gfx_SetFogCol(PackedCol color) {
	float rgba[4];
	if (color == gfx_fogColor) return;

	rgba[0] = PackedCol_R(color) / 255.0f; 
	rgba[1] = PackedCol_G(color) / 255.0f;
	rgba[2] = PackedCol_B(color) / 255.0f; 
	rgba[3] = PackedCol_A(color) / 255.0f;

	glFogfv(GL_FOG_COLOR, rgba);
	gfx_fogColor = color;
}

void Gfx_SetFogDensity(float value) {
	if (value == gfx_fogDensity) return;
	glFogf(GL_FOG_DENSITY, value);
	gfx_fogDensity = value;
}

void Gfx_SetFogEnd(float value) {
	if (value == gfx_fogEnd) return;
	glFogf(GL_FOG_END, value);
	gfx_fogEnd = value;
}

void Gfx_SetFogMode(FogFunc func) {
	static GLint modes[3] = { GL_LINEAR, GL_EXP, GL_EXP2 };
	if (func == gfx_fogMode) return;

#ifdef CC_BUILD_GLES
	/* OpenGL ES doesn't support glFogi, so use glFogf instead */
	/*  https://www.khronos.org/registry/OpenGL-Refpages/es1.1/xhtml/ */
	glFogf(GL_FOG_MODE, modes[func]);
#else
	glFogi(GL_FOG_MODE, modes[func]);
#endif
	gfx_fogMode = func;
}

void Gfx_SetTexturing(cc_bool enabled) { }

void Gfx_SetAlphaTest(cc_bool enabled) { 
	if (enabled) { glEnable(GL_ALPHA_TEST); } else { glDisable(GL_ALPHA_TEST); }
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	cc_bool enabled = !depthOnly;
	Gfx_SetColWriteMask(enabled, enabled, enabled, enabled);
	if (enabled) { glEnable(GL_TEXTURE_2D); } else { glDisable(GL_TEXTURE_2D); }
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static GLenum matrix_modes[3] = { GL_PROJECTION, GL_MODELVIEW, GL_TEXTURE };
static int lastMatrix;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type != lastMatrix) { lastMatrix = type; glMatrixMode(matrix_modes[type]); }
	glLoadMatrixf((const float*)matrix);
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
	if (type != lastMatrix) { lastMatrix = type; glMatrixMode(matrix_modes[type]); }
	glLoadIdentity();
}

static struct Matrix texMatrix = Matrix_IdentityValue;
void Gfx_EnableTextureOffset(float x, float y) {
	texMatrix.row4.X = x; texMatrix.row4.Y = y;
	Gfx_LoadMatrix(2, &texMatrix);
}

void Gfx_DisableTextureOffset(void) { Gfx_LoadIdentityMatrix(2); }


/*########################################################################################################################*
*-------------------------------------------------------State setup-------------------------------------------------------*
*#########################################################################################################################*/
static void Gfx_FreeState(void) { FreeDefaultResources(); }
static void Gfx_RestoreState(void) {
	InitDefaultResources();
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	gfx_format = -1;

	glHint(GL_FOG_HINT, GL_NICEST);
	glAlphaFunc(GL_GREATER, 0.5f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LEQUAL);
}

cc_bool Gfx_WarnIfNecessary(void) {
	cc_string renderer = String_FromReadonly((const char*)glGetString(GL_RENDERER));

#ifdef CC_BUILD_GL11
	Chat_AddRaw("&cYou are using the very outdated OpenGL backend.");
	Chat_AddRaw("&cAs such you may experience poor performance.");
	Chat_AddRaw("&cIt is likely you need to install video card drivers.");
#endif
	if (!String_ContainsConst(&renderer, "Intel")) return false;

	Chat_AddRaw("&cIntel graphics cards are known to have issues with the OpenGL build.");
	Chat_AddRaw("&cVSync may not work, and you may see disappearing clouds and map edges.");
#ifdef CC_BUILD_WIN
	Chat_AddRaw("&cTry downloading the Direct3D 9 build instead.");
#endif
	return true;
}


/*########################################################################################################################*
*-------------------------------------------------------Compatibility-----------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_GL11
static void GLBackend_Init(void) { MakeIndices(gl_indices, GFX_MAX_INDICES); }
#else

#if defined CC_BUILD_WIN
/* On 32 bit windows, can replace the gl function drawing with these 1.1 fallbacks  */
/*  (note that this only works on 32 bit system, as OpenGL IDs are 32 bit integers) */

/* fake vertex buffer objects with client side pointers */
typedef struct fake_buffer { cc_uint8* data; } fake_buffer;
static fake_buffer* cur_ib;
static fake_buffer* cur_vb;
#define fake_GetBuffer(target) (target == GL_ELEMENT_ARRAY_BUFFER ? &cur_ib : &cur_vb);

static void APIENTRY fake_bindBuffer(GLenum target, GfxResourceID src) {
	fake_buffer** buffer = fake_GetBuffer(target);
	*buffer = (fake_buffer*)src;
}

static GfxResourceID GenFakeBuffer(void) {
	return (GfxResourceID)Mem_TryAllocCleared(1, sizeof(fake_buffer));
}

static void DelFakeBuffer(GfxResourceID id) {
	Mem_Free((void*)id);
}

static void APIENTRY fake_bufferData(GLenum target, cc_uintptr size, const GLvoid* data, GLenum usage) {
	fake_buffer* buffer = *fake_GetBuffer(target);
	Mem_Free(buffer->data);

	buffer->data = Mem_TryAlloc(size, 1);
	if (data) Mem_Copy(buffer->data, data, size);
}
static void APIENTRY fake_bufferSubData(GLenum target, cc_uintptr offset, cc_uintptr size, const GLvoid* data) {
	fake_buffer* buffer = *fake_GetBuffer(target);
	Mem_Copy(buffer->data, data, size);
}

/* wglGetProcAddress doesn't work with OpenGL 1.1 software rasteriser, so call GL functions directly */
static void APIENTRY fake_drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices) {
	glDrawElements(mode, count, type, (cc_uintptr)indices + cur_ib->data);
}
static void APIENTRY fake_colorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer) {
	glColorPointer(size,    type, stride, (cc_uintptr)pointer + cur_vb->data);
}
static void APIENTRY fake_texCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer) {
	glTexCoordPointer(size, type, stride, (cc_uintptr)pointer + cur_vb->data);
}
static void APIENTRY fake_vertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer) {
	glVertexPointer(size,   type, stride, (cc_uintptr)pointer + cur_vb->data);
}

static void OpenGL11Fallback(void) {
	Window_ShowDialog("Performance warning",
		"Your system only supports only OpenGL 1.1\n" \
		"This is usually caused by graphics drivers not being installed\n\n" \
		"As such you will likely experience very poor performance");
	customMipmapsLevels = false;
		
	_glBindBuffer = fake_bindBuffer; _delBuffer    = DelFakeBuffer;
	_genBuffer    = GenFakeBuffer;   _glBufferData = fake_bufferData;
	_glBufferSubData = fake_bufferSubData;

	_glDrawElements    = fake_drawElements;    _glColorPointer  = fake_colorPointer;
	_glTexCoordPointer = fake_texCoordPointer; _glVertexPointer = fake_vertexPointer;
}
#else
/* No point in even trying for other systems */
static void OpenGL11Fallback(void) {
	Logger_FailToStart("Only OpenGL 1.1 supported.\n\n" \
		"Compile the game with CC_BUILD_GL11, or ask on the ClassiCube forums for it");
}
#endif

static void GLBackend_Init(void) {
	static const struct DynamicLibSym coreVboFuncs[] = {
		DynamicLib_Sym2("glBindBuffer",    glBindBuffer), DynamicLib_Sym2("glDeleteBuffers", glDeleteBuffers),
		DynamicLib_Sym2("glGenBuffers",    glGenBuffers), DynamicLib_Sym2("glBufferData",    glBufferData),
		DynamicLib_Sym2("glBufferSubData", glBufferSubData)
	};
	static const struct DynamicLibSym arbVboFuncs[] = {
		DynamicLib_Sym2("glBindBufferARB",    glBindBuffer), DynamicLib_Sym2("glDeleteBuffersARB", glDeleteBuffers),
		DynamicLib_Sym2("glGenBuffersARB",    glGenBuffers), DynamicLib_Sym2("glBufferDataARB",    glBufferData),
		DynamicLib_Sym2("glBufferSubDataARB", glBufferSubData)
	};
	static const cc_string vboExt = String_FromConst("GL_ARB_vertex_buffer_object");
	cc_string extensions = String_FromReadonly((const char*)glGetString(GL_EXTENSIONS));
	const GLubyte* ver   = glGetString(GL_VERSION);

	/* Version string is always: x.y. (and whatever afterwards) */
	int major = ver[0] - '0', minor = ver[2] - '0';
#ifdef CC_BUILD_WIN
	LoadCoreFuncs();
#endif
	customMipmapsLevels = true;

	/* Supported in core since 1.5 */
	if (major > 1 || (major == 1 && minor >= 5)) {
		GLContext_GetAll(coreVboFuncs, Array_Elems(coreVboFuncs));
	} else if (String_CaselessContains(&extensions, &vboExt)) {
		GLContext_GetAll(arbVboFuncs,  Array_Elems(arbVboFuncs));
	} else {
		OpenGL11Fallback();
	}
}
#endif


/*########################################################################################################################*
*----------------------------------------------------------Drawing--------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_GL11
/* point to client side dynamic array */
#define VB_PTR ((cc_uint8*)dynamicListData)
#define IB_PTR gl_indices
#else
/* no client side array, use vertex buffer object */
#define VB_PTR 0
#define IB_PTR NULL
#endif

static void GL_SetupVbColoured(void) {
	_glVertexPointer(3, GL_FLOAT,        SIZEOF_VERTEX_COLOURED, (void*)(VB_PTR + 0));
	_glColorPointer(4, GL_UNSIGNED_BYTE, SIZEOF_VERTEX_COLOURED, (void*)(VB_PTR + 12));
}

static void GL_SetupVbTextured(void) {
	_glVertexPointer(3, GL_FLOAT,        SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + 0));
	_glColorPointer(4, GL_UNSIGNED_BYTE, SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + 12));
	_glTexCoordPointer(2, GL_FLOAT,      SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + 16));
}

static void GL_SetupVbColoured_Range(int startVertex) {
	cc_uint32 offset = startVertex * SIZEOF_VERTEX_COLOURED;
	_glVertexPointer(3, GL_FLOAT,          SIZEOF_VERTEX_COLOURED, (void*)(VB_PTR + offset));
	_glColorPointer(4, GL_UNSIGNED_BYTE,   SIZEOF_VERTEX_COLOURED, (void*)(VB_PTR + offset + 12));
}

static void GL_SetupVbTextured_Range(int startVertex) {
	cc_uint32 offset = startVertex * SIZEOF_VERTEX_TEXTURED;
	_glVertexPointer(3,  GL_FLOAT,         SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset));
	_glColorPointer(4, GL_UNSIGNED_BYTE,   SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset + 12));
	_glTexCoordPointer(2, GL_FLOAT,        SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset + 16));
}

void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];

	if (fmt == VERTEX_FORMAT_TEXTURED) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);

		gfx_setupVBFunc      = GL_SetupVbTextured;
		gfx_setupVBRangeFunc = GL_SetupVbTextured_Range;
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);

		gfx_setupVBFunc      = GL_SetupVbColoured;
		gfx_setupVBRangeFunc = GL_SetupVbColoured_Range;
	}
}

void Gfx_DrawVb_Lines(int verticesCount) {
	gfx_setupVBFunc();
	glDrawArrays(GL_LINES, 0, verticesCount);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
#ifdef CC_BUILD_GL11
	if (activeList != gl_DYNAMICLISTID) { glCallList(activeList); return; }
#endif
	gfx_setupVBRangeFunc(startVertex);
	_glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, IB_PTR);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
#ifdef CC_BUILD_GL11
	if (activeList != gl_DYNAMICLISTID) { glCallList(activeList); return; }
#endif
	gfx_setupVBFunc();
	_glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, IB_PTR);
}

#ifdef CC_BUILD_GL11
void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) { glCallList(activeList); }
#else
void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	cc_uint32 offset = startVertex * SIZEOF_VERTEX_TEXTURED;
	_glVertexPointer(3, GL_FLOAT,        SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset));
	_glColorPointer(4, GL_UNSIGNED_BYTE, SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset + 12));
	_glTexCoordPointer(2, GL_FLOAT,      SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset + 16));
	_glDrawElements(GL_TRIANGLES,        ICOUNT(verticesCount),   GL_UNSIGNED_SHORT, IB_PTR);
}
#endif /* !CC_BUILD_GL11 */
#endif
