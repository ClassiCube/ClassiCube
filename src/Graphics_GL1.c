#include "Core.h"
#if defined CC_BUILD_GL && !defined CC_BUILD_GLMODERN
#include "_GraphicsBase.h"
#include "Chat.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"
/* The OpenGL backend is a bit of a mess, since it's really 3 backends in one:
 * - OpenGL 1.1 (completely lacking GPU, fallbacks to say Windows built-in software rasteriser)
 * - OpenGL 1.5 or OpenGL 1.2 + GL_ARB_vertex_buffer_object (default desktop backend)
 * - OpenGL 2.0 (alternative modern-ish backend)
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
#define GL_BGRA_EXT              0x80E1

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
#define _GL_ARRAY_BUFFER         0x8892
#define _GL_ELEMENT_ARRAY_BUFFER 0x8893
#define _GL_STATIC_DRAW          0x88E4
#define _GL_DYNAMIC_DRAW         0x88E8
#define _GL_TEXTURE_MAX_LEVEL    0x813D

GLAPI void APIENTRY glAlphaFunc(GLenum func, GLfloat ref);
GLAPI void APIENTRY glBindTexture(GLenum target, GLuint texture);
GLAPI void APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor);
GLAPI void APIENTRY glCallList(GLuint list);
GLAPI void APIENTRY glClear(GLuint mask);
GLAPI void APIENTRY glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
GLAPI void APIENTRY glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
GLAPI void APIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLAPI void APIENTRY glDeleteLists(GLuint list, GLsizei range);
GLAPI void APIENTRY glDeleteTextures(GLsizei n, const GLuint *textures);
GLAPI void APIENTRY glDepthFunc(GLenum func);
GLAPI void APIENTRY glDepthMask(GLboolean flag);
GLAPI void APIENTRY glDisable(GLenum cap);
GLAPI void APIENTRY glDisableClientState(GLenum array);
GLAPI void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count);
GLAPI void APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
GLAPI void APIENTRY glEnable(GLenum cap);
GLAPI void APIENTRY glEnableClientState(GLenum array);
GLAPI void APIENTRY glEndList(void);
GLAPI void APIENTRY glFogf(GLenum pname, GLfloat param);
GLAPI void APIENTRY glFogfv(GLenum pname, const GLfloat *params);
GLAPI void APIENTRY glFogi(GLenum pname, GLint param);
GLAPI void APIENTRY glFogiv(GLenum pname, const GLint *params);
GLAPI GLuint APIENTRY glGenLists(GLsizei range);
GLAPI void APIENTRY glGenTextures(GLsizei n, GLuint *textures);
GLAPI GLenum APIENTRY glGetError(void);
GLAPI void APIENTRY glGetFloatv(GLenum pname, GLfloat *params);
GLAPI void APIENTRY glGetIntegerv(GLenum pname, GLint *params);
GLAPI const GLubyte * APIENTRY glGetString(GLenum name);
GLAPI void APIENTRY glHint(GLenum target, GLenum mode);
GLAPI void APIENTRY glLoadIdentity(void);
GLAPI void APIENTRY glLoadMatrixf(const GLfloat *m);
GLAPI void APIENTRY glMatrixMode(GLenum mode);
GLAPI void APIENTRY glNewList(GLuint list, GLenum mode);
GLAPI void APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
GLAPI void APIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLAPI void APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
GLAPI void APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param);
GLAPI void APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
GLAPI void APIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLAPI void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
/* === END OPENGL HEADERS === */


#if defined CC_BUILD_GL11
static GLuint activeList;
#define gl_DYNAMICLISTID 1234567891
static void* dynamicListData;
static cc_uint16 gl_indices[GFX_MAX_INDICES];
#else
/* OpenGL functions use stdcall instead of cdecl on Windows */
static void (APIENTRY *_glBindBuffer)(GLenum target, GLuint buffer);
static void (APIENTRY *_glDeleteBuffers)(GLsizei n, const GLuint *buffers);
static void (APIENTRY *_glGenBuffers)(GLsizei n, GLuint *buffers);
static void (APIENTRY *_glBufferData)(GLenum target, cc_uintptr size, const GLvoid* data, GLenum usage);
static void (APIENTRY *_glBufferSubData)(GLenum target, cc_uintptr offset, cc_uintptr size, const GLvoid* data);
#endif

#if defined CC_BUILD_WEB || defined CC_BUILD_ANDROID
#define PIXEL_FORMAT GL_RGBA
#else
#define PIXEL_FORMAT 0x80E1 /* GL_BGRA_EXT */
#endif

#if defined CC_BIG_ENDIAN
/* Pixels are stored in memory as A,R,G,B but GL_UNSIGNED_BYTE will interpret as B,G,R,A */
/* So use GL_UNSIGNED_INT_8_8_8_8_REV instead to remedy this */
#define TRANSFER_FORMAT GL_UNSIGNED_INT_8_8_8_8_REV
#else
/* Pixels are stored in memory as B,G,R,A and GL_UNSIGNED_BYTE will interpret as B,G,R,A */
/* So fine to just use GL_UNSIGNED_BYTE here */
#define TRANSFER_FORMAT GL_UNSIGNED_BYTE
#endif

typedef void (*GL_SetupVBFunc)(void);
typedef void (*GL_SetupVBRangeFunc)(int startVertex);
static GL_SetupVBFunc gfx_setupVBFunc;
static GL_SetupVBRangeFunc gfx_setupVBRangeFunc;
/* Current format and size of vertices */
static int gfx_stride, gfx_format = -1;

static void GL_UpdateVsync(void) {
	GLContext_SetFpsLimit(gfx_vsync, gfx_minFrameMs);
}

static void GL_CheckSupport(void);
void Gfx_Create(void) {
	GLContext_Create();
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &Gfx.MaxTexWidth);
	Gfx.MaxTexHeight = Gfx.MaxTexWidth;
	Gfx.Created      = true;

	GL_CheckSupport();
	Gfx_RestoreState();
	GL_UpdateVsync();
}

cc_bool Gfx_TryRestoreContext(void) {
	return GLContext_TryRestore();
}

void Gfx_Free(void) {
	Gfx_FreeState();
	GLContext_Free();
}

#define gl_Toggle(cap) if (enabled) { glEnable(cap); } else { glDisable(cap); }
static void* tmpData;
static int tmpSize;

static void* FastAllocTempMem(int size) {
	if (size > tmpSize) {
		Mem_Free(tmpData);
		tmpData = Mem_Alloc(size, 1, "Gfx_AllocTempMemory");
	}

	tmpSize = size;
	return tmpData;
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static void Gfx_DoMipmaps(int x, int y, struct Bitmap* bmp, int rowWidth, cc_bool partial) {
	BitmapCol* prev = bmp->scan0;
	BitmapCol* cur;

	int lvls = CalcMipmapsLevels(bmp->width, bmp->height);
	int lvl, width = bmp->width, height = bmp->height;

	for (lvl = 1; lvl <= lvls; lvl++) {
		x /= 2; y /= 2;
		if (width > 1)  width /= 2;
		if (height > 1) height /= 2;

		cur = (BitmapCol*)Mem_Alloc(width * height, 4, "mipmaps");
		GenMipmaps(width, height, cur, prev, rowWidth);

		if (partial) {
			glTexSubImage2D(GL_TEXTURE_2D, lvl, x, y, width, height, PIXEL_FORMAT, TRANSFER_FORMAT, cur);
		} else {
			glTexImage2D(GL_TEXTURE_2D, lvl, GL_RGBA, width, height, 0, PIXEL_FORMAT, TRANSFER_FORMAT, cur);
		}

		if (prev != bmp->scan0) Mem_Free(prev);
		prev    = cur;
		rowWidth = width;
	}
	if (prev != bmp->scan0) Mem_Free(prev);
}

GfxResourceID Gfx_CreateTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	GLuint texId;
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if (!Math_IsPowOf2(bmp->width) || !Math_IsPowOf2(bmp->height)) {
		Logger_Abort("Textures must have power of two dimensions");
	}
	if (Gfx.LostContext) return 0;

	if (mipmaps) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		if (customMipmapsLevels) {
			int lvls = CalcMipmapsLevels(bmp->width, bmp->height);
			glTexParameteri(GL_TEXTURE_2D, _GL_TEXTURE_MAX_LEVEL, lvls);
		}
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bmp->width, bmp->height, 0, PIXEL_FORMAT, TRANSFER_FORMAT, bmp->scan0);

	if (mipmaps) Gfx_DoMipmaps(0, 0, bmp, bmp->width, false);
	return texId;
}

#define UPDATE_FAST_SIZE (64 * 64)
static CC_NOINLINE void UpdateTextureSlow(int x, int y, struct Bitmap* part, int rowWidth) {
	BitmapCol buffer[UPDATE_FAST_SIZE];
	void* ptr = (void*)buffer;
	int count = part->width * part->height;

	/* cannot allocate memory on the stack for very big updates */
	if (count > UPDATE_FAST_SIZE) {
		ptr = Mem_Alloc(count, 4, "Gfx_UpdateTexture temp");
	}

	CopyTextureData(ptr, part->width << 2, part, rowWidth << 2);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, part->width, part->height, PIXEL_FORMAT, TRANSFER_FORMAT, ptr);
	if (count > UPDATE_FAST_SIZE) Mem_Free(ptr);
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	glBindTexture(GL_TEXTURE_2D, (GLuint)texId);
	/* TODO: Use GL_UNPACK_ROW_LENGTH for Desktop OpenGL */

	if (part->width == rowWidth) {
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, part->width, part->height, PIXEL_FORMAT, TRANSFER_FORMAT, part->scan0);
	} else {
		UpdateTextureSlow(x, y, part, rowWidth);
	}
	if (mipmaps) Gfx_DoMipmaps(x, y, part, rowWidth, true);
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
	Gfx_UpdateTexture(texId, x, y, part, part->width, mipmaps);
}

void Gfx_BindTexture(GfxResourceID texId) {
	glBindTexture(GL_TEXTURE_2D, (GLuint)texId);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	GLuint id = (GLuint)(*texId);
	if (!id) return;
	glDeleteTextures(1, &id);
	*texId = 0;
}

void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gfx_clearColor, gfx_fogColor;
static float gfx_fogEnd = -1.0f, gfx_fogDensity = -1.0f;
static int gfx_fogMode  = -1;

void Gfx_SetFaceCulling(cc_bool enabled)   { gl_Toggle(GL_CULL_FACE); }
void Gfx_SetAlphaBlending(cc_bool enabled) { gl_Toggle(GL_BLEND); }
void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

static void GL_ClearCol(PackedCol col) {
	glClearColor(PackedCol_R(col) / 255.0f, PackedCol_G(col) / 255.0f,
				 PackedCol_B(col) / 255.0f, PackedCol_A(col) / 255.0f);
}
void Gfx_ClearCol(PackedCol col) {
	if (col == gfx_clearColor) return;
	GL_ClearCol(col);
	gfx_clearColor = col;
}

void Gfx_SetColWriteMask(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	glColorMask(r, g, b, a);
}

void Gfx_SetDepthWrite(cc_bool enabled) { glDepthMask(enabled); }
void Gfx_SetDepthTest(cc_bool enabled) { gl_Toggle(GL_DEPTH_TEST); }

void Gfx_CalcOrthoMatrix(float width, float height, struct Matrix* matrix) {
	Matrix_Orthographic(matrix, 0.0f, width, 0.0f, height, ORTHO_NEAR, ORTHO_FAR);
}
void Gfx_CalcPerspectiveMatrix(float fov, float aspect, float zFar, struct Matrix* matrix) {
	float zNear = 0.1f;
	Matrix_PerspectiveFieldOfView(matrix, fov, aspect, zNear, zFar);
}


/*########################################################################################################################*
*-------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
#ifndef CC_BUILD_GL11
static GLuint GL_GenAndBind(GLenum target) {
	GLuint id;
	_glGenBuffers(1, &id);
	_glBindBuffer(target, id);
	return id;
}

GfxResourceID Gfx_CreateIb(void* indices, int indicesCount) {
	GLuint id     = GL_GenAndBind(_GL_ELEMENT_ARRAY_BUFFER);
	cc_uint32 size = indicesCount * 2;
	_glBufferData(_GL_ELEMENT_ARRAY_BUFFER, size, indices, _GL_STATIC_DRAW);
	return id;
}

void Gfx_BindIb(GfxResourceID ib) { _glBindBuffer(_GL_ELEMENT_ARRAY_BUFFER, (GLuint)ib); }

void Gfx_DeleteIb(GfxResourceID* ib) {
	GLuint id = (GLuint)(*ib);
	if (!id) return;
	_glDeleteBuffers(1, &id);
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
	return GL_GenAndBind(_GL_ARRAY_BUFFER);
}

void Gfx_BindVb(GfxResourceID vb) { _glBindBuffer(_GL_ARRAY_BUFFER, (GLuint)vb); }

void Gfx_DeleteVb(GfxResourceID* vb) {
	GLuint id = (GLuint)(*vb);
	if (!id) return;
	_glDeleteBuffers(1, &id);
	*vb = 0;
}

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return FastAllocTempMem(count * strideSizes[fmt]);
}

void Gfx_UnlockVb(GfxResourceID vb) {
	_glBufferData(_GL_ARRAY_BUFFER, tmpSize, tmpData, _GL_STATIC_DRAW);
}
#else
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

GfxResourceID Gfx_CreateVb(VertexFormat fmt, int count) { return glGenLists(1); }
void Gfx_BindVb(GfxResourceID vb) { activeList = (GLuint)vb; }

void Gfx_DeleteVb(GfxResourceID* vb) {
	GLuint id = (GLuint)(*vb);
	if (id) glDeleteLists(id, 1);
	*vb = 0;
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
	GLuint id;
	cc_uint32 size;
	if (Gfx.LostContext) return 0;

	id = GL_GenAndBind(_GL_ARRAY_BUFFER);
	size = maxVertices * strideSizes[fmt];
	_glBufferData(_GL_ARRAY_BUFFER, size, NULL, _GL_DYNAMIC_DRAW);
	return id;
}

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return FastAllocTempMem(count * strideSizes[fmt]);
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) {
	_glBindBuffer(_GL_ARRAY_BUFFER, (GLuint)vb);
	_glBufferSubData(_GL_ARRAY_BUFFER, 0, tmpSize, tmpData);
}

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	cc_uint32 size = vCount * gfx_stride;
	_glBindBuffer(_GL_ARRAY_BUFFER, (GLuint)vb);
	_glBufferSubData(_GL_ARRAY_BUFFER, 0, size, vertices);
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
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
static BitmapCol* GL_GetRow(struct Bitmap* bmp, int y) { 
	/* OpenGL stores bitmap in bottom-up order, so flip order when saving */
	return Bitmap_GetRow(bmp, (bmp->height - 1) - y); 
}
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	struct Bitmap bmp;
	cc_result res;
	GLint vp[4];
	
	glGetIntegerv(GL_VIEWPORT, vp); /* { x, y, width, height } */
	bmp.width  = vp[2]; 
	bmp.height = vp[3];

	bmp.scan0  = (BitmapCol*)Mem_TryAlloc(bmp.width * bmp.height, 4);
	if (!bmp.scan0) return ERR_OUT_OF_MEMORY;
	glReadPixels(0, 0, bmp.width, bmp.height, PIXEL_FORMAT, TRANSFER_FORMAT, bmp.scan0);

	res = Png_Encode(&bmp, output, GL_GetRow, false);
	Mem_Free(bmp.scan0);
	return res;
}

static void AppendVRAMStats(cc_string* info) {
	static const cc_string memExt = String_FromConst("GL_NVX_gpu_memory_info");
	GLint totalKb, curKb;
	float total, cur;

	/* NOTE: glGetString returns UTF8, but I just treat it as code page 437 */
	cc_string exts = String_FromReadonly((const char*)glGetString(GL_EXTENSIONS));
	if (!String_CaselessContains(&exts, &memExt)) return;

	glGetIntegerv(0x9048, &totalKb);
	glGetIntegerv(0x9049, &curKb);
	if (totalKb <= 0 || curKb <= 0) return;

	total = totalKb / 1024.0f; cur = curKb / 1024.0f;
	String_Format2(info, "Video memory: %f2 MB total, %f2 free\n", &total, &cur);
}

void Gfx_GetApiInfo(cc_string* info) {
	GLint depthBits;
	int pointerSize = sizeof(void*) * 8;

	glGetIntegerv(GL_DEPTH_BITS, &depthBits);
	String_Format1(info, "-- Using OpenGL (%i bit) --\n", &pointerSize);
	String_Format1(info, "Vendor: %c\n",     glGetString(GL_VENDOR));
	String_Format1(info, "Renderer: %c\n",   glGetString(GL_RENDERER));
	String_Format1(info, "GL version: %c\n", glGetString(GL_VERSION));
	AppendVRAMStats(info);
	String_Format2(info, "Max texture size: (%i, %i)\n", &Gfx.MaxTexWidth, &Gfx.MaxTexHeight);
	String_Format1(info, "Depth buffer bits: %i\n",      &depthBits);
	GLContext_GetApiInfo(info);
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
	if (Gfx.Created) GL_UpdateVsync();
}

void Gfx_BeginFrame(void) { frameStart = Stopwatch_Measure(); }
void Gfx_Clear(void) { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }
void Gfx_EndFrame(void) { 
	if (!GLContext_SwapBuffers()) Gfx_LoseContext("GLContext lost");
	if (gfx_minFrameMs) LimitFPS();
}

void Gfx_OnWindowResize(void) {
	GLContext_Update();
	/* In case GLContext_Update changes window bounds */
	/* TODO: Eliminate this nasty hack.. */
	Game_UpdateDimensions();
	glViewport(0, 0, Game.Width, Game.Height);
}


/*########################################################################################################################*
*------------------------------------------------------OpenGL legacy------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFog(cc_bool enabled) {
	gfx_fogEnabled = enabled;
	gl_Toggle(GL_FOG);
}

void Gfx_SetFogCol(PackedCol col) {
	float rgba[4];
	if (col == gfx_fogColor) return;

	rgba[0] = PackedCol_R(col) / 255.0f; rgba[1] = PackedCol_G(col) / 255.0f;
	rgba[2] = PackedCol_B(col) / 255.0f; rgba[3] = PackedCol_A(col) / 255.0f;

	glFogfv(GL_FOG_COLOR, rgba);
	gfx_fogColor = col;
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

void Gfx_SetTexturing(cc_bool enabled) { gl_Toggle(GL_TEXTURE_2D); }
void Gfx_SetAlphaTest(cc_bool enabled) { gl_Toggle(GL_ALPHA_TEST); }

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
static void GL_CheckSupport(void) { MakeIndices(gl_indices, GFX_MAX_INDICES); }
#else
/* fake vertex buffer objects with client side pointers */
typedef struct fake_buffer { cc_uint8* data; } fake_buffer;
static fake_buffer* cur_ib;
static fake_buffer* cur_vb;
#define fake_GetBuffer(target) (target == _GL_ELEMENT_ARRAY_BUFFER ? &cur_ib : &cur_vb);

static void APIENTRY fake_glBindBuffer(GLenum target, GLuint src) {
	fake_buffer** buffer = fake_GetBuffer(target);
	*buffer = (fake_buffer*)src;
}

static void APIENTRY fake_glDeleteBuffers(GLsizei n, const GLuint *buffers) {
	Mem_Free((void*)buffers[0]);
}

static void APIENTRY fake_glGenBuffers(GLsizei n, GLuint *buffers) {
	fake_buffer* buffer = (fake_buffer*)Mem_TryAlloc(1, sizeof(fake_buffer));
	buffer->data = NULL;
	buffers[0]   = (GLuint)buffer;
}

static void APIENTRY fake_glBufferData(GLenum target, cc_uintptr size, const GLvoid* data, GLenum usage) {
	fake_buffer* buffer = *fake_GetBuffer(target);
	Mem_Free(buffer->data);

	buffer->data = Mem_TryAlloc(size, 1);
	if (data) Mem_Copy(buffer->data, data, size);
}
static void APIENTRY fake_glBufferSubData(GLenum target, cc_uintptr offset, cc_uintptr size, const GLvoid* data) {
	fake_buffer* buffer = *fake_GetBuffer(target);
	Mem_Copy(buffer->data, data, size);
}

static void GL_CheckSupport(void) {
	static const struct DynamicLibSym coreVboFuncs[5] = {
		DynamicLib_Sym2("glBindBuffer",    glBindBuffer), DynamicLib_Sym2("glDeleteBuffers", glDeleteBuffers),
		DynamicLib_Sym2("glGenBuffers",    glGenBuffers), DynamicLib_Sym2("glBufferData",    glBufferData),
		DynamicLib_Sym2("glBufferSubData", glBufferSubData)
	};
	static const struct DynamicLibSym arbVboFuncs[5] = {
		DynamicLib_Sym2("glBindBufferARB",    glBindBuffer), DynamicLib_Sym2("glDeleteBuffersARB", glDeleteBuffers),
		DynamicLib_Sym2("glGenBuffersARB",    glGenBuffers), DynamicLib_Sym2("glBufferDataARB",    glBufferData),
		DynamicLib_Sym2("glBufferSubDataARB", glBufferSubData)
	};
	static const cc_string vboExt = String_FromConst("GL_ARB_vertex_buffer_object");
	cc_string extensions = String_FromReadonly((const char*)glGetString(GL_EXTENSIONS));
	const GLubyte* ver   = glGetString(GL_VERSION);

	/* Version string is always: x.y. (and whatever afterwards) */
	int major = ver[0] - '0', minor = ver[2] - '0';

	/* Supported in core since 1.5 */
	if (major > 1 || (major == 1 && minor >= 5)) {
		GLContext_GetAll(coreVboFuncs, Array_Elems(coreVboFuncs));
	} else if (String_CaselessContains(&extensions, &vboExt)) {
		GLContext_GetAll(arbVboFuncs,  Array_Elems(arbVboFuncs));
	} else {
		Logger_Abort("Only OpenGL 1.1 supported.\n\n" \
			"Compile the game with CC_BUILD_GL11, or ask on the ClassiCube forums for it");

		_glBindBuffer = fake_glBindBuffer; _glDeleteBuffers = fake_glDeleteBuffers;
		_glGenBuffers = fake_glGenBuffers; _glBufferData    = fake_glBufferData;
		_glBufferSubData = fake_glBufferSubData;
	}
	customMipmapsLevels = true;
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
	glVertexPointer(3, GL_FLOAT,        SIZEOF_VERTEX_COLOURED, (void*)(VB_PTR + 0));
	glColorPointer(4, GL_UNSIGNED_BYTE, SIZEOF_VERTEX_COLOURED, (void*)(VB_PTR + 12));
}

static void GL_SetupVbTextured(void) {
	glVertexPointer(3, GL_FLOAT,        SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + 0));
	glColorPointer(4, GL_UNSIGNED_BYTE, SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + 12));
	glTexCoordPointer(2, GL_FLOAT,      SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + 16));
}

static void GL_SetupVbColoured_Range(int startVertex) {
	cc_uint32 offset = startVertex * SIZEOF_VERTEX_COLOURED;
	glVertexPointer(3, GL_FLOAT,          SIZEOF_VERTEX_COLOURED, (void*)(VB_PTR + offset));
	glColorPointer(4, GL_UNSIGNED_BYTE,   SIZEOF_VERTEX_COLOURED, (void*)(VB_PTR + offset + 12));
}

static void GL_SetupVbTextured_Range(int startVertex) {
	cc_uint32 offset = startVertex * SIZEOF_VERTEX_TEXTURED;
	glVertexPointer(3,  GL_FLOAT,         SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset));
	glColorPointer(4, GL_UNSIGNED_BYTE,   SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset + 12));
	glTexCoordPointer(2, GL_FLOAT,        SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset + 16));
}

void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];

	if (fmt == VERTEX_FORMAT_TEXTURED) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		gfx_setupVBFunc      = GL_SetupVbTextured;
		gfx_setupVBRangeFunc = GL_SetupVbTextured_Range;
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
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
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, IB_PTR);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
#ifdef CC_BUILD_GL11
	if (activeList != gl_DYNAMICLISTID) { glCallList(activeList); return; }
#endif
	gfx_setupVBFunc();
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, IB_PTR);
}

#ifdef CC_BUILD_GL11
void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) { glCallList(activeList); }
#else
void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	cc_uint32 offset = startVertex * SIZEOF_VERTEX_TEXTURED;
	glVertexPointer(3, GL_FLOAT,        SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset));
	glColorPointer(4, GL_UNSIGNED_BYTE, SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset + 12));
	glTexCoordPointer(2, GL_FLOAT,      SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset + 16));
	glDrawElements(GL_TRIANGLES,        ICOUNT(verticesCount),   GL_UNSIGNED_SHORT, IB_PTR);
}
#endif /* !CC_BUILD_GL11 */
#endif
