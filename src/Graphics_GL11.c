#include "Core.h"
#if CC_GFX_BACKEND == CC_GFX_BACKEND_GL11
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Window.h"

/* 
  This is the OpenGL 1.1 specific backend, that uses display lists for static geometry.
  Although the normal OpenGL can fallback to a 1.1 mode, it uses client side vertex arrays.
  So this backend may potentially be faster on a small number of old GPUs.

  NOTE: Make sure when changing this or Graphics_GL1.c, to keep things in sync
*/
#include "../misc/opengl/GLCommon.h"

/* e.g. GLAPI void APIENTRY glFunction(int value); */
#define GL_FUNC(retType, name, args) GLAPI retType APIENTRY name args;
#include "../misc/opengl/GL1Funcs.h"

static GLuint activeList;
#define gl_DYNAMICLISTID 1234567891
static void* dynamicListData;
static cc_uint16 gl_indices[GFX_MAX_INDICES];

#include "../misc/opengl/GL1Macros.h"
#include "_GLShared.h"

typedef void (*GL_SetupVBFunc)(void);
typedef void (*GL_SetupVBRangeFunc)(int startVertex);
static GL_SetupVBFunc gfx_setupVBFunc;
static GL_SetupVBRangeFunc gfx_setupVBRangeFunc;

static void CheckSupport(void) {
	static const cc_string bgraExt = String_FromConst("GL_EXT_bgra");
	cc_string extensions = String_FromReadonly((const char*)_glGetString(GL_EXTENSIONS));
	const GLubyte* ver   = _glGetString(GL_VERSION);

	/* Version string is always: x.y. (and whatever afterwards) */
	int major = ver[0] - '0', minor = ver[2] - '0';
	/* Some old IRIX cards don't support BGRA */
	convert_rgba = major == 1 && minor <= 1 && !String_CaselessContains(&extensions, &bgraExt);
}

void Gfx_Create(void) {
	GLContext_Create();
	GL_InitCommon();

	MakeIndices(gl_indices, GFX_MAX_INDICES, NULL);
	Gfx_RestoreState();
	GLContext_SetVSync(gfx_vsync);
	CheckSupport();
}


/*########################################################################################################################*
*-------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) { return 0; }
void Gfx_BindIb(GfxResourceID ib) { }
void Gfx_DeleteIb(GfxResourceID* ib) { }


/*########################################################################################################################*
*------------------------------------------------------Vertex buffers-----------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) { 
	return uint_to_ptr(glGenLists(1)); 
}

void Gfx_BindVb(GfxResourceID vb) { 
	activeList = ptr_to_uint(vb); 
}

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

static cc_bool UnlockVb(GfxResourceID vb) {
	UpdateDisplayList(ptr_to_uint(vb), tmpData, tmpFormat, tmpCount);
	return true;
}

GfxResourceID Gfx_CreateVb2(void* vertices, VertexFormat fmt, int count) {
	GLuint list = glGenLists(1);
	UpdateDisplayList(list, vertices, fmt, count);
	return uint_to_ptr(list);
}


/*########################################################################################################################*
*--------------------------------------------------Dynamic vertex buffers-------------------------------------------------*
*#########################################################################################################################*/
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


/*########################################################################################################################*
*----------------------------------------------------------Drawing--------------------------------------------------------*
*#########################################################################################################################*/
#define VB_PTR ((cc_uint8*)dynamicListData)

static void GL_SetupVbColoured(void) {
	GLpointer ptr = (GLpointer)VB_PTR;
	glVertexPointer(3, GL_FLOAT,        SIZEOF_VERTEX_COLOURED, ptr +  0);
	glColorPointer(4, GL_UNSIGNED_BYTE, SIZEOF_VERTEX_COLOURED, ptr + 12);
}

static void GL_SetupVbTextured(void) {
	GLpointer ptr = (GLpointer)VB_PTR;
	glVertexPointer(3, GL_FLOAT,        SIZEOF_VERTEX_TEXTURED, ptr +  0);
	glColorPointer(4, GL_UNSIGNED_BYTE, SIZEOF_VERTEX_TEXTURED, ptr + 12);
	glTexCoordPointer(2, GL_FLOAT,      SIZEOF_VERTEX_TEXTURED, ptr + 16);
}

static void GL_SetupVbColoured_Range(int startVertex) {
	GLpointer ptr = (GLpointer)VB_PTR + startVertex * SIZEOF_VERTEX_COLOURED;
	glVertexPointer(3, GL_FLOAT,          SIZEOF_VERTEX_COLOURED, ptr +  0);
	glColorPointer(4, GL_UNSIGNED_BYTE,   SIZEOF_VERTEX_COLOURED, ptr + 12);
}

static void GL_SetupVbTextured_Range(int startVertex) {
	GLpointer ptr = (GLpointer)VB_PTR + startVertex * SIZEOF_VERTEX_TEXTURED;
	glVertexPointer(3,  GL_FLOAT,         SIZEOF_VERTEX_TEXTURED, ptr +  0);
	glColorPointer(4, GL_UNSIGNED_BYTE,   SIZEOF_VERTEX_TEXTURED, ptr + 12);
	glTexCoordPointer(2, GL_FLOAT,        SIZEOF_VERTEX_TEXTURED, ptr + 16);
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

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	if (activeList != gl_DYNAMICLISTID) { glCallList(activeList); return; }

	gfx_setupVBRangeFunc(startVertex);
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, gl_indices);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	if (activeList != gl_DYNAMICLISTID) { glCallList(activeList); return; }

	gfx_setupVBFunc();
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, gl_indices);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) { 
	glCallList(activeList); 
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_BindTexture(GfxResourceID texId) { setTexture(texId); }


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gfx_fogColor;
static float gfx_fogEnd, gfx_fogDensity;
static int gfx_fogMode;

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

	glFogi(GL_FOG_MODE, modes[func]);
	gfx_fogMode = func;
}

static void SetAlphaTest(cc_bool enabled) {
	if (enabled) { glEnable(GL_ALPHA_TEST); } else { glDisable(GL_ALPHA_TEST); }
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	cc_bool enabled = !depthOnly;
	SetColorWrite(enabled & gfx_colorMask[0], enabled & gfx_colorMask[1], 
				  enabled & gfx_colorMask[2], enabled & gfx_colorMask[3]);
	
	if (enabled) { glEnable(GL_TEXTURE_2D); } else { glDisable(GL_TEXTURE_2D); }
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static GLenum matrix_modes[] = { GL_PROJECTION, GL_MODELVIEW, GL_TEXTURE };
static int lastMatrix;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type != lastMatrix) { lastMatrix = type; glMatrixMode(matrix_modes[type]); }

	if (matrix == &Matrix_Identity) {
		glLoadIdentity();
	} else {
		glLoadMatrixf((const float*)matrix);
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
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	gfx_format = -1;
	lastMatrix = -1;

	gfx_clearColor = 0;
	gfx_fogColor   = 0;
	gfx_fogEnd     = -1.0f;
	gfx_fogDensity = -1.0f;
	gfx_fogMode    = -1;

	glAlphaFunc(GL_GREATER, 0.5f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LEQUAL);
}

cc_bool Gfx_WarnIfNecessary(void) {
	Chat_AddRaw("&cYou are using the very outdated OpenGL backend.");
	Chat_AddRaw("&cAs such you may experience poor performance.");
	Chat_AddRaw("&cIt is likely you need to install video card drivers.");
	return false;
}

cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }

void Gfx_GetApiInfo(cc_string* info) {
	int pointerSize = sizeof(void*) * 8;

	String_Format1(info, "-- Using OpenGL (%i bit) --\n", &pointerSize);
	GetGLApiInfo(info);
}
#endif
