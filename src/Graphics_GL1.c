/* Silence deprecation warnings on modern macOS/iOS */
#define GL_SILENCE_DEPRECATION
#define GLES_SILENCE_DEPRECATION

#include "Core.h"
#if CC_GFX_BACKEND == CC_GFX_BACKEND_GL1
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Window.h"

#if defined CC_BUILD_WIN
	#define CC_BUILD_GL11_FALLBACK
#endif

/* The OpenGL backend is a bit of a mess, since it's really 2 backends in one:
 * - OpenGL 1.1 (completely lacking GPU, fallbacks to say Windows built-in software rasteriser)
 * - OpenGL 1.5 or OpenGL 1.2 + GL_ARB_vertex_buffer_object (default desktop backend)
*/
#include "../misc/opengl/GLCommon.h"

/* e.g. GLAPI void APIENTRY glFunction(int value); */
#define GL_FUNC(retType, name, args) GLAPI retType APIENTRY name args;
#include "../misc/opengl/GL1Funcs.h"

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


#if defined CC_BUILD_GL11_FALLBACK && !defined CC_BUILD_GL11
/* Note the following about calling OpenGL functions on Windows */
/*  1) wglGetProcAddress returns a context specific address */
/*  2) dllimport functions are implemented using indirect function pointers */
/*     https://web.archive.org/web/20080321171626/http://blogs.msdn.com/oldnewthing/archive/2006/07/20/672695.aspx */
/*     https://web.archive.org/web/20071016185327/http://blogs.msdn.com/oldnewthing/archive/2006/07/27/680250.aspx */
/* Therefore one layer of indirection can be avoided by calling wglGetProcAddress functions instead */
/*  e.g. if _glDrawElements = wglGetProcAddress("glDrawElements") */
/*    call [glDrawElements]  --> opengl32.dll thunk--> GL driver thunk --> GL driver implementation */
/*    call [_glDrawElements] --> GL driver thunk --> GL driver implementation */

/* e.g. typedef void (APIENTRY *FP_glFunction)(int value); */
#undef  GL_FUNC
#define GL_FUNC(retType, name, args) typedef retType (APIENTRY *FP_ ## name)args;
#include "../misc/opengl/GL1Funcs.h"

/* e.g. static void (APIENTRY *_glFunction)(int value); */
#undef  GL_FUNC
#define GL_FUNC(retType, name, args) static retType (APIENTRY *_ ## name)args;
#include "../misc/opengl/GL1Funcs.h"

static const struct DynamicLibSym coreFuncs[] = {
	/* e.g. { DYNAMICLIB_QUOTE(name), (void**)&_ ## name }, */
	#undef  GL_FUNC
	#define GL_FUNC(retType, name, args) { DYNAMICLIB_QUOTE(name), (void**)&_ ## name },
	#include "../misc/opengl/GL1Funcs.h"
};
#else
	#include "../misc/opengl/GL1Macros.h"
#endif

typedef void (*GL_SetupVBFunc)(void);
typedef void (*GL_SetupVBRangeFunc)(int startVertex);
static GL_SetupVBFunc gfx_setupVBFunc;
static GL_SetupVBRangeFunc gfx_setupVBRangeFunc;

#include "_GLShared.h"
static void GLBackend_Init(void);

void Gfx_Create(void) {
	GLContext_Create();
#ifdef CC_BUILD_GL11_FALLBACK
	GLContext_GetAll(coreFuncs, Array_Elems(coreFuncs));
#endif
	customMipmapsLevels = true;
	Gfx.BackendType     = CC_GFX_BACKEND_GL1;

	GL_InitCommon();
	GLBackend_Init();
	Gfx_RestoreState();
	GLContext_SetVSync(gfx_vsync);
}


/*########################################################################################################################*
*-------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
#ifndef CC_BUILD_GL11
GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	cc_uint16 indices[GFX_MAX_INDICES];
	GfxResourceID id = NULL;
	cc_uint32 size   = count * sizeof(cc_uint16);

	_glGenBuffers(1, (GLuint*)&id);
	fillFunc(indices, count, obj);
	_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
	_glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
	return id;
}

void Gfx_BindIb(GfxResourceID ib) { _glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib); }

void Gfx_DeleteIb(GfxResourceID* ib) {
	GfxResourceID id = *ib;
	if (!id) return;

	_glDeleteBuffers(1, (GLuint*)&id);
	*ib = 0;
}
#else
GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) { return 0; }
void Gfx_BindIb(GfxResourceID ib) { }
void Gfx_DeleteIb(GfxResourceID* ib) { }
#endif


/*########################################################################################################################*
*------------------------------------------------------Vertex buffers-----------------------------------------------------*
*#########################################################################################################################*/
#ifndef CC_BUILD_GL11
static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	GfxResourceID id = NULL;
	_glGenBuffers(1, (GLuint*)&id);
	_glBindBuffer(GL_ARRAY_BUFFER, id);
	return id;
}

void Gfx_BindVb(GfxResourceID vb) { 
	_glBindBuffer(GL_ARRAY_BUFFER, vb); 
}

void Gfx_DeleteVb(GfxResourceID* vb) {
	GfxResourceID id = *vb;
	if (id) _glDeleteBuffers(1, (GLuint*)&id);
	*vb = 0;
}

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return FastAllocTempMem(count * strideSizes[fmt]);
}

void Gfx_UnlockVb(GfxResourceID vb) {
	_glBufferData(GL_ARRAY_BUFFER, tmpSize, tmpData, GL_STATIC_DRAW);
}
#else
static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) { 
	return glGenLists(1); 
}
void Gfx_BindVb(GfxResourceID vb) { activeList = ptr_to_uint(vb); }

void Gfx_DeleteVb(GfxResourceID* vb) {
	GLuint id = ptr_to_uint(*vb);
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
static GfxResourceID Gfx_AllocDynamicVb(VertexFormat fmt, int maxVertices) {
	GfxResourceID id = NULL;
	cc_uint32 size   = maxVertices * strideSizes[fmt];

	_glGenBuffers(1, (GLuint*)&id);
	_glBindBuffer(GL_ARRAY_BUFFER, id);
	_glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
	return id;
}

void Gfx_BindDynamicVb(GfxResourceID vb) {
	_glBindBuffer(GL_ARRAY_BUFFER, vb); 
}

void Gfx_DeleteDynamicVb(GfxResourceID* vb) {
	GfxResourceID id = *vb;
	if (id) _glDeleteBuffers(1, (GLuint*)&id);
	*vb = 0;
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
static GfxResourceID Gfx_AllocDynamicVb(VertexFormat fmt, int maxVertices) {
	return (GfxResourceID)Mem_TryAlloc(maxVertices, strideSizes[fmt]);
}

void Gfx_BindDynamicVb(GfxResourceID vb) {
	activeList      = gl_DYNAMICLISTID;
	dynamicListData = vb;
}

void Gfx_DeleteDynamicVb(GfxResourceID* vb) {
	void* addr = *vb;
	if (addr) Mem_Free(addr);
	*vb = 0;
}

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) { return vb; }
void  Gfx_UnlockDynamicVb(GfxResourceID vb) { Gfx_BindDynamicVb(vb); }

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	Gfx_BindDynamicVb(vb);
	Mem_Copy(vb, vertices, vCount * gfx_stride);
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
	_glVertexPointer(3, GL_FLOAT,        SIZEOF_VERTEX_COLOURED, VB_PTR +  0);
	_glColorPointer(4, GL_UNSIGNED_BYTE, SIZEOF_VERTEX_COLOURED, VB_PTR + 12);
}

static void GL_SetupVbTextured(void) {
	_glVertexPointer(3, GL_FLOAT,        SIZEOF_VERTEX_TEXTURED, VB_PTR +  0);
	_glColorPointer(4, GL_UNSIGNED_BYTE, SIZEOF_VERTEX_TEXTURED, VB_PTR + 12);
	_glTexCoordPointer(2, GL_FLOAT,      SIZEOF_VERTEX_TEXTURED, VB_PTR + 16);
}

static void GL_SetupVbColoured_Range(int startVertex) {
	cc_uint32 offset = startVertex * SIZEOF_VERTEX_COLOURED;
	_glVertexPointer(3, GL_FLOAT,          SIZEOF_VERTEX_COLOURED, VB_PTR + offset +  0);
	_glColorPointer(4, GL_UNSIGNED_BYTE,   SIZEOF_VERTEX_COLOURED, VB_PTR + offset + 12);
}

static void GL_SetupVbTextured_Range(int startVertex) {
	cc_uint32 offset = startVertex * SIZEOF_VERTEX_TEXTURED;
	_glVertexPointer(3,  GL_FLOAT,         SIZEOF_VERTEX_TEXTURED, VB_PTR + offset +  0);
	_glColorPointer(4, GL_UNSIGNED_BYTE,   SIZEOF_VERTEX_TEXTURED, VB_PTR + offset + 12);
	_glTexCoordPointer(2, GL_FLOAT,        SIZEOF_VERTEX_TEXTURED, VB_PTR + offset + 16);
}

void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];

	if (fmt == VERTEX_FORMAT_TEXTURED) {
		_glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		_glEnable(GL_TEXTURE_2D);

		gfx_setupVBFunc      = GL_SetupVbTextured;
		gfx_setupVBRangeFunc = GL_SetupVbTextured_Range;
	} else {
		_glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		_glDisable(GL_TEXTURE_2D);

		gfx_setupVBFunc      = GL_SetupVbColoured;
		gfx_setupVBRangeFunc = GL_SetupVbColoured_Range;
	}
}

void Gfx_DrawVb_Lines(int verticesCount) {
	gfx_setupVBFunc();
	_glDrawArrays(GL_LINES, 0, verticesCount);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
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
	_glVertexPointer(3, GL_FLOAT,        SIZEOF_VERTEX_TEXTURED, VB_PTR + offset +  0);
	_glColorPointer(4, GL_UNSIGNED_BYTE, SIZEOF_VERTEX_TEXTURED, VB_PTR + offset + 12);
	_glTexCoordPointer(2, GL_FLOAT,      SIZEOF_VERTEX_TEXTURED, VB_PTR + offset + 16);
	_glDrawElements(GL_TRIANGLES,        ICOUNT(verticesCount),  GL_UNSIGNED_SHORT, IB_PTR);
}
#endif /* !CC_BUILD_GL11 */


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_BindTexture(GfxResourceID texId) {
	_glBindTexture(GL_TEXTURE_2D, ptr_to_uint(texId));
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gfx_fogColor;
static float gfx_fogEnd = -1.0f, gfx_fogDensity = -1.0f;
static int gfx_fogMode  = -1;

void Gfx_SetFog(cc_bool enabled) {
	gfx_fogEnabled = enabled;
	if (enabled) { _glEnable(GL_FOG); } else { _glDisable(GL_FOG); }
}

void Gfx_SetFogCol(PackedCol color) {
	float rgba[4];
	if (color == gfx_fogColor) return;

	rgba[0] = PackedCol_R(color) / 255.0f; 
	rgba[1] = PackedCol_G(color) / 255.0f;
	rgba[2] = PackedCol_B(color) / 255.0f; 
	rgba[3] = PackedCol_A(color) / 255.0f;

	_glFogfv(GL_FOG_COLOR, rgba);
	gfx_fogColor = color;
}

void Gfx_SetFogDensity(float value) {
	if (value == gfx_fogDensity) return;
	_glFogf(GL_FOG_DENSITY, value);
	gfx_fogDensity = value;
}

void Gfx_SetFogEnd(float value) {
	if (value == gfx_fogEnd) return;
	_glFogf(GL_FOG_END, value);
	gfx_fogEnd = value;
}

void Gfx_SetFogMode(FogFunc func) {
	static GLint modes[3] = { GL_LINEAR, GL_EXP, GL_EXP2 };
	if (func == gfx_fogMode) return;

#ifdef CC_BUILD_GLES
	/* OpenGL ES doesn't support glFogi, so use glFogf instead */
	/*  https://www.khronos.org/registry/OpenGL-Refpages/es1.1/xhtml/ */
	_glFogf(GL_FOG_MODE, modes[func]);
#else
	_glFogi(GL_FOG_MODE, modes[func]);
#endif
	gfx_fogMode = func;
}

static void SetAlphaTest(cc_bool enabled) {
	if (enabled) { _glEnable(GL_ALPHA_TEST); } else { _glDisable(GL_ALPHA_TEST); }
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	cc_bool enabled = !depthOnly;
	SetColorWrite(enabled & gfx_colorMask[0], enabled & gfx_colorMask[1], 
				  enabled & gfx_colorMask[2], enabled & gfx_colorMask[3]);
	
	if (enabled) { _glEnable(GL_TEXTURE_2D); } else { _glDisable(GL_TEXTURE_2D); }
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static GLenum matrix_modes[] = { GL_PROJECTION, GL_MODELVIEW, GL_TEXTURE };
static int lastMatrix;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type != lastMatrix) { lastMatrix = type; _glMatrixMode(matrix_modes[type]); }

	if (matrix == &Matrix_Identity) {
		_glLoadIdentity();
	} else {
		_glLoadMatrixf((const float*)matrix);
	}
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	Gfx_LoadMatrix(MATRIX_VIEW, view);
	Gfx_LoadMatrix(MATRIX_PROJ, proj);
	Matrix_Mul(mvp, view, proj);
}

static struct Matrix texMatrix = Matrix_IdentityValue;
void Gfx_EnableTextureOffset(float x, float y) {
	texMatrix.row4.x = x; texMatrix.row4.y = y;
	Gfx_LoadMatrix(2, &texMatrix);
}

void Gfx_DisableTextureOffset(void) { Gfx_LoadMatrix(2, &Matrix_Identity); }


/*########################################################################################################################*
*-------------------------------------------------------State setup-------------------------------------------------------*
*#########################################################################################################################*/
static void Gfx_FreeState(void) { FreeDefaultResources(); }
static void Gfx_RestoreState(void) {
	InitDefaultResources();
	_glEnableClientState(GL_VERTEX_ARRAY);
	_glEnableClientState(GL_COLOR_ARRAY);
	gfx_format = -1;

	_glHint(GL_FOG_HINT, GL_NICEST);
	_glAlphaFunc(GL_GREATER, 0.5f);
	_glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	_glDepthFunc(GL_LEQUAL);
}

cc_bool Gfx_WarnIfNecessary(void) {
	cc_string renderer = String_FromReadonly((const char*)_glGetString(GL_RENDERER));
	
#ifdef CC_BUILD_GL11
	Chat_AddRaw("&cYou are using the very outdated OpenGL backend.");
	Chat_AddRaw("&cAs such you may experience poor performance.");
	Chat_AddRaw("&cIt is likely you need to install video card drivers.");
#endif

	if (String_ContainsConst(&renderer, "llvmpipe")) {
		Chat_AddRaw("&cSoftware rendering is being used, performance will greatly suffer.");
		Chat_AddRaw("&cVSync may not work, and you may see disappearing clouds and map edges.");
		Chat_AddRaw("&cYou may need to install video card drivers.");

		Gfx.Limitations |= GFX_LIMIT_VERTEX_ONLY_FOG;
		return true;
	}
	if (String_ContainsConst(&renderer, "Intel")) {
		Chat_AddRaw("&cIntel graphics cards are known to have issues with the OpenGL build.");
		Chat_AddRaw("&cVSync may not work, and you may see disappearing clouds and map edges.");
		#ifdef CC_BUILD_WIN
		Chat_AddRaw("&cTry downloading the Direct3D 9 build instead.");
		#endif

		Gfx.Limitations |= GFX_LIMIT_VERTEX_ONLY_FOG;
		return true;
	}
	return false;
}
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }


/*########################################################################################################################*
*-------------------------------------------------------Compatibility-----------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_GL11
static void GLBackend_Init(void) { MakeIndices(gl_indices, GFX_MAX_INDICES, NULL); }
#else

#ifdef CC_BUILD_GL11_FALLBACK
static FP_glDrawElements    _realDrawElements;
static FP_glColorPointer    _realColorPointer;
static FP_glTexCoordPointer _realTexCoordPointer;
static FP_glVertexPointer   _realVertexPointer;

/* On Windows, can replace the GL function drawing with these 1.1 fallbacks */
/* fake vertex buffer objects by using client side pointers instead */
typedef struct legacy_buffer { cc_uint8* data; } legacy_buffer;
static legacy_buffer* cur_ib;
static legacy_buffer* cur_vb;
#define legacy_GetBuffer(target) (target == GL_ELEMENT_ARRAY_BUFFER ? &cur_ib : &cur_vb);

static void APIENTRY legacy_genBuffer(GLsizei n, GLuint* buffer) {
	GfxResourceID* dst = (GfxResourceID*)buffer;
	*dst = Mem_TryAllocCleared(1, sizeof(legacy_buffer));
}

static void APIENTRY legacy_deleteBuffer(GLsizei n, const GLuint* buffer) {
	GfxResourceID* dst = (GfxResourceID*)buffer;
	Mem_Free(*dst);
}

static void APIENTRY legacy_bindBuffer(GLenum target, GfxResourceID src) {
	legacy_buffer** buffer = legacy_GetBuffer(target);
	*buffer = (legacy_buffer*)src;
}

static void APIENTRY legacy_bufferData(GLenum target, cc_uintptr size, const GLvoid* data, GLenum usage) {
	legacy_buffer* buffer = *legacy_GetBuffer(target);
	Mem_Free(buffer->data);

	buffer->data = Mem_TryAlloc(size, 1);
	if (data) Mem_Copy(buffer->data, data, size);
}

static void APIENTRY legacy_bufferSubData(GLenum target, cc_uintptr offset, cc_uintptr size, const GLvoid* data) {
	legacy_buffer* buffer = *legacy_GetBuffer(target);
	Mem_Copy(buffer->data, data, size);
}


struct GL10Texture {
	int width, height;
	unsigned char* pixels;
};
static struct GL10Texture* gl10_tex;

static void APIENTRY gl10_bindTexture(GLenum target, GLuint texture) {
	gl10_tex = (struct GL10Texture*)texture;
	if (gl10_tex && gl10_tex->pixels) {
		_glTexImage2D(GL_TEXTURE_2D, 0, 4, gl10_tex->width, gl10_tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, gl10_tex->pixels);
	} else {
		BitmapCol pixel = BITMAPCOLOR_WHITE;
		_glTexImage2D(GL_TEXTURE_2D, 0, 4, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixel);
	}
}

static void APIENTRY gl10_deleteTexture(GLsizei n, const GLuint* textures) {
	struct GL10Texture* tex = (struct GL10Texture*)textures[0];
	if (tex->pixels) Mem_Free(tex->pixels);
	if (tex) Mem_Free(tex);
}

static void APIENTRY gl10_genTexture(GLsizei n, GLuint* textures) {
	textures[0] = (GLuint)Mem_AllocCleared(1, sizeof(struct GL10Texture), "GL 1.0 texture");
}

static void APIENTRY gl10_texImage(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels) {
	int i;
	gl10_tex->width  = width;
	gl10_tex->height = height;
	gl10_tex->pixels = Mem_Alloc(width * height, 4, "GL 1.0 pixels");

	Mem_Copy(gl10_tex->pixels, pixels, width * height * 4);
	for (i = 0; i < width * height * 4; i += 4) 
	{
		cc_uint8 t = gl10_tex->pixels[i + 2];
		gl10_tex->pixels[i + 2] = gl10_tex->pixels[i + 0];
		gl10_tex->pixels[i + 0] = t;
	}
}

static void APIENTRY gl10_texSubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels) {
	/* TODO */
}

static void APIENTRY gl10_disableClientState(GLenum target) { }

static void APIENTRY gl10_enableClientState(GLenum target) { }

static cc_uint8* gl10_vb;
static void APIENTRY gl10_drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices) {
	/* TODO */
	int i;
	_glBegin(GL_QUADS);
	count = (count * 4) / 6;

	if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		struct VertexTextured* src = (struct VertexTextured*)gl10_vb;
		for (i = 0; i < count; i++, src++) 
		{
			_glColor4ub(PackedCol_R(src->Col), PackedCol_G(src->Col), PackedCol_B(src->Col), PackedCol_A(src->Col));
			_glTexCoord2f(src->U, src->V);
			_glVertex3f(src->x, src->y, src->z);
		}
	} else {
		struct VertexColoured* src = (struct VertexColoured*)gl10_vb;
		for (i = 0; i < count; i++, src++) 
		{
			_glColor4ub(PackedCol_R(src->Col), PackedCol_G(src->Col), PackedCol_B(src->Col), PackedCol_A(src->Col));
			_glVertex3f(src->x, src->y, src->z);
		}
	}

	_glEnd();
}
static void APIENTRY gl10_colorPointer(GLint size, GLenum type, GLsizei stride, GLpointer offset) {
}
static void APIENTRY gl10_texCoordPointer(GLint size, GLenum type, GLsizei stride, GLpointer offset) {
}
static void APIENTRY gl10_vertexPointer(GLint size, GLenum type, GLsizei stride, GLpointer offset) {
	gl10_vb = cur_vb->data + offset;
}


static void APIENTRY gl11_drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices) {
	_realDrawElements(mode, count, type, (cc_uintptr)indices + cur_ib->data);
}
static void APIENTRY gl11_colorPointer(GLint size, GLenum type, GLsizei stride, GLpointer offset) {
	_realColorPointer(size,    type, stride, (cc_uintptr)cur_vb->data + offset);
}
static void APIENTRY gl11_texCoordPointer(GLint size, GLenum type, GLsizei stride, GLpointer offset) {
	_realTexCoordPointer(size, type, stride, (cc_uintptr)cur_vb->data + offset);
}
static void APIENTRY gl11_vertexPointer(GLint size, GLenum type, GLsizei stride, GLpointer offset) {
	_realVertexPointer(size,   type, stride, (cc_uintptr)cur_vb->data + offset);
}


static void FallbackOpenGL(void) {
	Window_ShowDialog("Performance warning",
		"Your system only supports only OpenGL 1.1\n" \
		"This is usually caused by graphics drivers not being installed\n\n" \
		"As such you will likely experience very poor performance");
	customMipmapsLevels = false;
		
	_glGenBuffers    = legacy_genBuffer;
	_glDeleteBuffers = legacy_deleteBuffer;
	_glBindBuffer    = legacy_bindBuffer;
	_glBufferData    = legacy_bufferData;
	_glBufferSubData = legacy_bufferSubData;

	_realDrawElements    = _glDrawElements;    _realColorPointer  = _glColorPointer;
	_realTexCoordPointer = _glTexCoordPointer; _realVertexPointer = _glVertexPointer;

	_glDrawElements    = gl11_drawElements;    _glColorPointer  = gl11_colorPointer;
	_glTexCoordPointer = gl11_texCoordPointer; _glVertexPointer = gl11_vertexPointer;

	/* OpenGL 1.0 fallback support */
	if (_realDrawElements) return;
	Window_ShowDialog("Performance warning", "OpenGL 1.0 only support, expect awful performance");

	_glDrawElements    = gl10_drawElements;    _glColorPointer  = gl10_colorPointer;
	_glTexCoordPointer = gl10_texCoordPointer; _glVertexPointer = gl10_vertexPointer;

	_glBindTexture    = gl10_bindTexture;
	_glGenTextures    = gl10_genTexture;
	_glDeleteTextures = gl10_deleteTexture;
	_glTexImage2D     = gl10_texImage;
	_glTexSubImage2D  = gl10_texSubImage;

	_glDisableClientState = gl10_disableClientState;
	_glEnableClientState  = gl10_enableClientState;
}
#else
/* No point in even trying for other systems */
static void FallbackOpenGL(void) {
	Logger_FailToStart("Only OpenGL 1.1 supported.\n\n" \
		"Compile the game with CC_BUILD_GL11, or ask on the ClassiCube forums for it");
}
#endif

static void GLBackend_Init(void) {
	static const struct DynamicLibSym coreVboFuncs[] = {
		DynamicLib_ReqSym2("glBindBuffer",    glBindBuffer), DynamicLib_ReqSym2("glDeleteBuffers", glDeleteBuffers),
		DynamicLib_ReqSym2("glGenBuffers",    glGenBuffers), DynamicLib_ReqSym2("glBufferData",    glBufferData),
		DynamicLib_ReqSym2("glBufferSubData", glBufferSubData)
	};
	static const struct DynamicLibSym arbVboFuncs[] = {
		DynamicLib_ReqSym2("glBindBufferARB",    glBindBuffer), DynamicLib_ReqSym2("glDeleteBuffersARB", glDeleteBuffers),
		DynamicLib_ReqSym2("glGenBuffersARB",    glGenBuffers), DynamicLib_ReqSym2("glBufferDataARB",    glBufferData),
		DynamicLib_ReqSym2("glBufferSubDataARB", glBufferSubData)
	};
	static const cc_string vboExt = String_FromConst("GL_ARB_vertex_buffer_object");
	cc_string extensions = String_FromReadonly((const char*)_glGetString(GL_EXTENSIONS));
	const GLubyte* ver   = _glGetString(GL_VERSION);

	/* Version string is always: x.y. (and whatever afterwards) */
	int major = ver[0] - '0', minor = ver[2] - '0';

	/* Supported in core since 1.5 */
	if (major > 1 || (major == 1 && minor >= 5)) {
		GLContext_GetAll(coreVboFuncs, Array_Elems(coreVboFuncs));
	} else if (String_CaselessContains(&extensions, &vboExt)) {
		GLContext_GetAll(arbVboFuncs,  Array_Elems(arbVboFuncs));
	} else {
		FallbackOpenGL();
	}
}
#endif
#endif
