#include "GraphicsAPI.h"
#ifndef CC_BUILD_D3D9
#include "ErrorHandler.h"
#include "Platform.h"
#include "Window.h"
#include "GraphicsCommon.h"
#include "Funcs.h"
#include "Chat.h"
#include "Game.h"
#include "ExtMath.h"
#include "Bitmap.h"

#ifdef CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#include <windows.h>
#else
#define APIENETRY
#endif
#include <GL/gl.h>

/* Extensions from later than OpenGL 1.1 */
#define GL_TEXTURE_MAX_LEVEL    0x813D
#define GL_ARRAY_BUFFER         0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW          0x88E4
#define GL_DYNAMIC_DRAW         0x88E8
#define GL_BGRA_EXT             0x80E1

#ifdef CC_BUILD_GL11
GfxResourceID gl_activeList;
#define gl_DYNAMICLISTID 1234567891
void* gl_dynamicListData;
#else
typedef void (APIENTRY *FUNC_GLBINDBUFFER) (GLenum target, GLuint buffer);
typedef void (APIENTRY *FUNC_GLDELETEBUFFERS) (GLsizei n, const GLuint *buffers);
typedef void (APIENTRY *FUNC_GLGENBUFFERS) (GLsizei n, GLuint *buffers);
typedef void (APIENTRY *FUNC_GLBUFFERDATA) (GLenum target, const void* size, const void *data, GLenum usage);
typedef void (APIENTRY *FUNC_GLBUFFERSUBDATA) (GLenum target, const void* offset, const void* size, const void *data);
FUNC_GLBINDBUFFER    glBindBuffer;
FUNC_GLDELETEBUFFERS glDeleteBuffers;
FUNC_GLGENBUFFERS    glGenBuffers;
FUNC_GLBUFFERDATA    glBufferData;
FUNC_GLBUFFERSUBDATA glBufferSubData;
#endif

Int32 Gfx_strideSizes[2] = GFX_STRIDE_SIZES;
bool gl_vsync;

int gl_blend[6] = { GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA };
int gl_compare[8] = { GL_ALWAYS, GL_NOTEQUAL, GL_NEVER, GL_LESS, GL_LEQUAL, GL_EQUAL, GL_GEQUAL, GL_GREATER };
int gl_fogModes[3] = { GL_LINEAR, GL_EXP, GL_EXP2 };
int gl_matrixModes[3] = { GL_PROJECTION, GL_MODELVIEW, GL_TEXTURE };

typedef void (*GL_SetupVBFunc)(void);
typedef void (*GL_SetupVBRangeFunc)(int startVertex);
GL_SetupVBFunc gl_setupVBFunc;
GL_SetupVBRangeFunc gl_setupVBRangeFunc;
int gl_batchStride, gl_batchFormat = -1;

#ifndef CC_BUILD_GL11
static void GL_CheckVboSupport(void) {
	String extensions = String_FromReadonly(glGetString(GL_EXTENSIONS));
	String version    = String_FromReadonly(glGetString(GL_VERSION));
	String vboExt     = String_FromConst("GL_ARB_vertex_buffer_object");

	int major = (int)(version.buffer[0] - '0'); /* x.y. (and so forth) */
	int minor = (int)(version.buffer[2] - '0');

	/* Supported in core since 1.5 */
	if ((major > 1) || (major == 1 && minor >= 5)) {
		glBindBuffer    = (FUNC_GLBINDBUFFER)GLContext_GetAddress("glBindBuffer");
		glDeleteBuffers = (FUNC_GLDELETEBUFFERS)GLContext_GetAddress("glDeleteBuffers");
		glGenBuffers    = (FUNC_GLGENBUFFERS)GLContext_GetAddress("glGenBuffers");
		glBufferData    = (FUNC_GLBUFFERDATA)GLContext_GetAddress("glBufferData");
		glBufferSubData = (FUNC_GLBUFFERSUBDATA)GLContext_GetAddress("glBufferSubData");
		return;
	} else if (String_CaselessContains(&extensions, &vboExt)) {
		glBindBuffer    = (FUNC_GLBINDBUFFER)GLContext_GetAddress("glBindBufferARB");
		glDeleteBuffers = (FUNC_GLDELETEBUFFERS)GLContext_GetAddress("glDeleteBuffersARB");
		glGenBuffers    = (FUNC_GLGENBUFFERS)GLContext_GetAddress("glGenBuffersARB");
		glBufferData    = (FUNC_GLBUFFERDATA)GLContext_GetAddress("glBufferDataARB");
		glBufferSubData = (FUNC_GLBUFFERSUBDATA)GLContext_GetAddress("glBufferSubDataARB");
	} else {
		ErrorHandler_Fail("Only OpenGL 1.1 supported.\r\n\r\n" \
			"Compile the game with CC_BUILD_GL11, or ask on the classicube forums for it");
	}
}
#endif

void Gfx_Init(void) {
	Gfx_MinZNear = 0.1f;
	struct GraphicsMode mode = GraphicsMode_MakeDefault();
	GLContext_Init(mode);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &Gfx_MaxTexWidth);
	Gfx_MaxTexHeight = Gfx_MaxTexWidth;

#ifndef CC_BUILD_GL11
	Gfx_CustomMipmapsLevels = true;
	GL_CheckVboSupport();
#else
	Gfx_CustomMipmapsLevels = false;
#endif
	
	GfxCommon_Init();
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
}

void Gfx_Free(void) {
	GfxCommon_Free();
	GLContext_Free();
}

#define gl_Toggle(cap) if (enabled) { glEnable(cap); } else { glDisable(cap); }

static void GL_DoMipmaps(GfxResourceID texId, int x, int y, Bitmap* bmp, bool partial) {
	uint8_t* prev = bmp->Scan0;
	int lvls = GfxCommon_MipmapsLevels(bmp->Width, bmp->Height);
	int lvl, width = bmp->Width, height = bmp->Height;

	for (lvl = 1; lvl <= lvls; lvl++) {
		x /= 2; y /= 2;
		if (width > 1) width /= 2;
		if (height > 1) height /= 2;

		uint8_t* cur = Mem_Alloc(width * height, BITMAP_SIZEOF_PIXEL, "mipmaps");
		GfxCommon_GenMipmaps(width, height, cur, prev);

		if (partial) {
			glTexSubImage2D(GL_TEXTURE_2D, lvl, x, y, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, cur);
		} else {
			glTexImage2D(GL_TEXTURE_2D, lvl, GL_RGBA, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, cur);
		}

		if (prev != bmp->Scan0) Mem_Free(prev);
		prev = cur;
	}
	if (prev != bmp->Scan0) Mem_Free(prev);
}

GfxResourceID Gfx_CreateTexture(Bitmap* bmp, bool managedPool, bool mipmaps) {
	UInt32 texId;
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if (!Math_IsPowOf2(bmp->Width) || !Math_IsPowOf2(bmp->Height)) {
		ErrorHandler_Fail("Textures must have power of two dimensions");
	}

	if (mipmaps) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		if (Gfx_CustomMipmapsLevels) {
			int lvls = GfxCommon_MipmapsLevels(bmp->Width, bmp->Height);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, lvls);
		}
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bmp->Width, bmp->Height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, bmp->Scan0);

	if (mipmaps) GL_DoMipmaps(texId, 0, 0, bmp, false);
	return texId;
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, Bitmap* part, bool mipmaps) {
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, part->Width, part->Height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, part->Scan0);
	if (mipmaps) GL_DoMipmaps(texId, x, y, part, true);
}

void Gfx_BindTexture(GfxResourceID texId) {
	glBindTexture(GL_TEXTURE_2D, texId);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	if (texId == NULL || *texId == NULL) return;
	glDeleteTextures(1, texId);
	*texId = NULL;
}

void Gfx_SetTexturing(bool enabled) { gl_Toggle(GL_TEXTURE_2D); }
void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }


bool gl_fogEnable;
bool Gfx_GetFog(void) { return gl_fogEnable; }
void Gfx_SetFog(bool enabled) {
	gl_fogEnable = enabled;
	gl_Toggle(GL_FOG);
}

PackedCol gl_lastFogCol;
void Gfx_SetFogCol(PackedCol col) {
	if (PackedCol_Equals(col, gl_lastFogCol)) return;
	float colRGBA[4] = { col.R / 255.0f, col.G / 255.0f, col.B / 255.0f, col.A / 255.0f };
	glFogfv(GL_FOG_COLOR, colRGBA);
	gl_lastFogCol = col;
}

float gl_lastFogEnd = -1, gl_lastFogDensity = -1;
void Gfx_SetFogDensity(float value) {
	if (value == gl_lastFogDensity) return;
	glFogf(GL_FOG_DENSITY, value);
	gl_lastFogDensity = value;
}

void Gfx_SetFogEnd(float value) {
	if (value == gl_lastFogEnd) return;
	glFogf(GL_FOG_END, value);
	gl_lastFogEnd = value;
}

int gl_lastFogMode = -1;
void Gfx_SetFogMode(int mode) {
	if (mode == gl_lastFogMode) return;
	glFogi(GL_FOG_MODE, gl_fogModes[mode]);
	gl_lastFogMode = mode;
}


void Gfx_SetFaceCulling(bool enabled) { gl_Toggle(GL_CULL_FACE); }
void Gfx_SetAlphaTest(bool enabled) { gl_Toggle(GL_ALPHA_TEST); }
void Gfx_SetAlphaTestFunc(int func, float value) {
	glAlphaFunc(gl_compare[func], value);
}

void Gfx_SetAlphaBlending(bool enabled) { gl_Toggle(GL_BLEND); }
void Gfx_SetAlphaBlendFunc(int srcFunc, int dstFunc) {
	glBlendFunc(gl_blend[srcFunc], gl_blend[dstFunc]);
}
void Gfx_SetAlphaArgBlend(bool enabled) { }


void Gfx_Clear(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

PackedCol gl_lastClearCol;
void Gfx_ClearCol(PackedCol col) {
	if (PackedCol_Equals(col, gl_lastClearCol)) return;
	glClearColor(col.R / 255.0f, col.G / 255.0f, col.B / 255.0f, col.A / 255.0f);
	gl_lastClearCol = col;
}

void Gfx_SetColourWriteMask(bool r, bool g, bool b, bool a) {
	glColorMask(r, g, b, a);
}

void Gfx_SetDepthWrite(bool enabled) {
	glDepthMask(enabled);
}

void Gfx_SetDepthTest(bool enabled) { gl_Toggle(GL_DEPTH_TEST); }
void Gfx_SetDepthTestFunc(int compareFunc) {
	glDepthFunc(gl_compare[compareFunc]);
}


#ifndef CC_BUILD_GL11
GfxResourceID GL_GenAndBind(GLenum target) {
	GfxResourceID id;
	glGenBuffers(1, &id);
	glBindBuffer(target, id);
	return id;
}
#endif

GfxResourceID Gfx_CreateDynamicVb(int vertexFormat, int maxVertices) {
#ifndef CC_BUILD_GL11
	GfxResourceID id = GL_GenAndBind(GL_ARRAY_BUFFER);
	UInt32 sizeInBytes = maxVertices * Gfx_strideSizes[vertexFormat];
	glBufferData(GL_ARRAY_BUFFER, (void*)sizeInBytes, NULL, GL_DYNAMIC_DRAW);
	return id;
#else
	return gl_DYNAMICLISTID;
#endif
}

#define gl_MAXINDICES ICOUNT(65536)
GfxResourceID Gfx_CreateVb(void* vertices, int vertexFormat, int count) {
#ifndef CC_BUILD_GL11
	GfxResourceID id = GL_GenAndBind(GL_ARRAY_BUFFER);
	UInt32 sizeInBytes = count * Gfx_strideSizes[vertexFormat];
	glBufferData(GL_ARRAY_BUFFER, (void*)sizeInBytes, vertices, GL_STATIC_DRAW);
	return id;
#else
	/* We need to setup client state properly when building the list */
	int curFormat = gl_batchFormat;
	Gfx_SetBatchFormat(vertexFormat);
	GfxResourceID list = glGenLists(1);
	glNewList(list, GL_COMPILE);
	count &= ~0x01; /* Need to get rid of the 1 extra element, see comment in chunk mesh builder for why */

	uint16_t indices[GFX_MAX_INDICES];
	GfxCommon_MakeIndices(indices, ICOUNT(count));

	int stride = vertexFormat == VERTEX_FORMAT_P3FT2FC4B ? sizeof(VertexP3fT2fC4b) : sizeof(VertexP3fC4b);
	glVertexPointer(3, GL_FLOAT, stride, vertices);
	glColorPointer(4, GL_UNSIGNED_BYTE, stride, (void*)((uint8_t*)vertices + 12));
	if (vertexFormat == VERTEX_FORMAT_P3FT2FC4B) {
		glTexCoordPointer(2, GL_FLOAT, stride, (void*)((uint8_t*)vertices + 16));
	}

	glDrawElements(GL_TRIANGLES, ICOUNT(count), GL_UNSIGNED_SHORT, indices);
	glEndList();
	Gfx_SetBatchFormat(curFormat);
	return list;
#endif
}

GfxResourceID Gfx_CreateIb(void* indices, int indicesCount) {
#ifndef CC_BUILD_GL11
	GfxResourceID id = GL_GenAndBind(GL_ELEMENT_ARRAY_BUFFER);
	UInt32 sizeInBytes = indicesCount * 2;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (void*)sizeInBytes, indices, GL_STATIC_DRAW);
	return id;
#else
	return NULL;
#endif
}

void Gfx_BindVb(GfxResourceID vb) {
#ifndef CC_BUILD_GL11
	glBindBuffer(GL_ARRAY_BUFFER, vb);
#else
	gl_activeList = vb;
#endif
}

void Gfx_BindIb(GfxResourceID ib) {
#ifndef CC_BUILD_GL11
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
#else
	return;
#endif
}

void Gfx_DeleteVb(GfxResourceID* vb) {
	if (vb == NULL || *vb == NULL) return;
#ifndef CC_BUILD_GL11
	glDeleteBuffers(1, vb);
#else
	if (*vb != gl_DYNAMICLISTID) glDeleteLists(*vb, 1);
#endif
	*vb = NULL;
}

void Gfx_DeleteIb(GfxResourceID* ib) {
#ifndef CC_BUILD_GL11
	if (ib == NULL || *ib == NULL) return;
	glDeleteBuffers(1, ib);
	*ib = NULL;
#else
	return;
#endif
}


void GL_SetupVbPos3fCol4b(void) {
	glVertexPointer(3, GL_FLOAT,        sizeof(VertexP3fC4b), (void*)0);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexP3fC4b), (void*)12);
}

void GL_SetupVbPos3fTex2fCol4b(void) {
	glVertexPointer(3, GL_FLOAT,        sizeof(VertexP3fT2fC4b), (void*)0);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexP3fT2fC4b), (void*)12);
	glTexCoordPointer(2, GL_FLOAT,      sizeof(VertexP3fT2fC4b), (void*)16);
}

void GL_SetupVbPos3fCol4b_Range(int startVertex) {
	UInt32 offset = startVertex * (UInt32)sizeof(VertexP3fC4b);
	glVertexPointer(3, GL_FLOAT,          sizeof(VertexP3fC4b), (void*)(offset));
	glColorPointer(4, GL_UNSIGNED_BYTE,   sizeof(VertexP3fC4b), (void*)(offset + 12));
}

void GL_SetupVbPos3fTex2fCol4b_Range(int startVertex) {
	UInt32 offset = startVertex * (UInt32)sizeof(VertexP3fT2fC4b);
	glVertexPointer(3,  GL_FLOAT,         sizeof(VertexP3fT2fC4b), (void*)(offset));
	glColorPointer(4, GL_UNSIGNED_BYTE,   sizeof(VertexP3fT2fC4b), (void*)(offset + 12));
	glTexCoordPointer(2, GL_FLOAT,        sizeof(VertexP3fT2fC4b), (void*)(offset + 16));
}

void Gfx_SetBatchFormat(int vertexFormat) {
	if (vertexFormat == gl_batchFormat) return;

	if (gl_batchFormat == VERTEX_FORMAT_P3FT2FC4B) {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	gl_batchFormat = vertexFormat;
	gl_batchStride = Gfx_strideSizes[vertexFormat];

	if (vertexFormat == VERTEX_FORMAT_P3FT2FC4B) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		gl_setupVBFunc = GL_SetupVbPos3fTex2fCol4b;
		gl_setupVBRangeFunc = GL_SetupVbPos3fTex2fCol4b_Range;
	} else {
		gl_setupVBFunc = GL_SetupVbPos3fCol4b;
		gl_setupVBRangeFunc = GL_SetupVbPos3fCol4b_Range;
	}
}

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
#ifndef CC_BUILD_GL11
	glBindBuffer(GL_ARRAY_BUFFER, vb);
	UInt32 sizeInBytes = vCount * gl_batchStride;
	glBufferSubData(GL_ARRAY_BUFFER, NULL, (void*)sizeInBytes, vertices);
#else
	gl_activeList = gl_DYNAMICLISTID;
	gl_dynamicListData = vertices;
#endif	
}

#ifdef CC_BUILD_GL11
static void GL_V16(VertexP3fC4b v) {
	glColor4ub(v.Col.R, v.Col.G, v.Col.B, v.Col.A);
	glVertex3f(v.X, v.Y, v.Z);
}

static void GL_V24(VertexP3fT2fC4b v) {
	glColor4ub(v.Col.R, v.Col.G, v.Col.B, v.Col.A);
	glTexCoord2f(v.U, v.V);
	glVertex3f(v.X, v.Y, v.Z);
}

static void GL_DrawDynamicTriangles(int verticesCount, int startVertex) {
	glBegin(GL_TRIANGLES);
	int i;
	if (gl_batchFormat == VERTEX_FORMAT_P3FT2FC4B) {
		VertexP3fT2fC4b* ptr = (VertexP3fT2fC4b*)gl_dynamicListData;
		for (i = startVertex; i < startVertex + verticesCount; i += 4) {
			GL_V24(ptr[i + 0]); GL_V24(ptr[i + 1]); GL_V24(ptr[i + 2]);
			GL_V24(ptr[i + 2]); GL_V24(ptr[i + 3]); GL_V24(ptr[i + 0]);
		}
	} else {
		VertexP3fC4b* ptr = (VertexP3fC4b*)gl_dynamicListData;
		for (i = startVertex; i < startVertex + verticesCount; i += 4) {
			GL_V16(ptr[i + 0]); GL_V16(ptr[i + 1]); GL_V16(ptr[i + 2]);
			GL_V16(ptr[i + 2]); GL_V16(ptr[i + 3]); GL_V16(ptr[i + 0]);
		}
	}
	glEnd();
}
#endif

void Gfx_DrawVb_Lines(int verticesCount) {
#ifndef CC_BUILD_GL11
	gl_setupVBFunc();
	glDrawArrays(GL_LINES, 0, verticesCount);
#else
	glBegin(GL_LINES);
	int i;
	if (gl_batchFormat == VERTEX_FORMAT_P3FT2FC4B) {
		VertexP3fT2fC4b* ptr = (VertexP3fT2fC4b*)gl_dynamicListData;
		for (i = 0; i < verticesCount; i += 2) { GL_V24(ptr[i + 0]); GL_V24(ptr[i + 1]); }
	} else {
		VertexP3fC4b* ptr = (VertexP3fC4b*)gl_dynamicListData;
		for (i = 0; i < verticesCount; i += 2) { GL_V16(ptr[i + 0]); GL_V16(ptr[i + 1]); }
	}
	glEnd();
#endif
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
#ifndef CC_BUILD_GL11
	gl_setupVBRangeFunc(startVertex);
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, NULL);
#else
	if (gl_activeList != gl_DYNAMICLISTID) { glCallList(gl_activeList); }
	else { GL_DrawDynamicTriangles(verticesCount, startVertex); }
#endif
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
#ifndef CC_BUILD_GL11
	gl_setupVBFunc();
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, NULL);
#else
	if (gl_activeList != gl_DYNAMICLISTID) { glCallList(gl_activeList); }
	else { GL_DrawDynamicTriangles(verticesCount, 0); }
#endif
}

GfxResourceID gl_lastPartialList;
void Gfx_DrawIndexedVb_TrisT2fC4b(int verticesCount, int startVertex) {
	
#ifndef CC_BUILD_GL11
	UInt32 offset = startVertex * (UInt32)sizeof(VertexP3fT2fC4b);
	glVertexPointer(3, GL_FLOAT,          sizeof(VertexP3fT2fC4b), (void*)(offset));
	glColorPointer(4, GL_UNSIGNED_BYTE,   sizeof(VertexP3fT2fC4b), (void*)(offset + 12));
	glTexCoordPointer(2, GL_FLOAT,        sizeof(VertexP3fT2fC4b), (void*)(offset + 16));
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, NULL);
#else
	/* TODO: This renders the whole map, bad performance!! FIX FIX */
	if (gl_activeList != gl_lastPartialList) {
		glCallList(gl_activeList);
		gl_lastPartialList = gl_activeList;
	}
#endif
}


int gl_lastMatrixType;
void Gfx_SetMatrixMode(int matrixType) {
	if (matrixType == gl_lastMatrixType) return;
	glMatrixMode(gl_matrixModes[matrixType]);
	gl_lastMatrixType = matrixType;
}

void Gfx_LoadMatrix(struct Matrix* matrix) { glLoadMatrixf((float*)matrix); }
void Gfx_LoadIdentityMatrix(void) { glLoadIdentity(); }

void Gfx_CalcOrthoMatrix(float width, float height, struct Matrix* matrix) {
	Matrix_OrthographicOffCenter(matrix, 0.0f, width, height, 0.0f, -10000.0f, 10000.0f);
}
void Gfx_CalcPerspectiveMatrix(float fov, float aspect, float zNear, float zFar, struct Matrix* matrix) {
	Matrix_PerspectiveFieldOfView(matrix, fov, aspect, zNear, zFar);
}


ReturnCode Gfx_TakeScreenshot(struct Stream* output, int width, int height) {
	Bitmap bmp; Bitmap_Allocate(&bmp, width, height);
	glReadPixels(0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, bmp.Scan0);
	uint8_t tmp[PNG_MAX_DIMS * BITMAP_SIZEOF_PIXEL];

	/* flip vertically around y */
	int x, y;
	UInt32 stride = (UInt32)(bmp.Width) * BITMAP_SIZEOF_PIXEL;
	for (y = 0; y < height / 2; y++) {
		UInt32* src = Bitmap_GetRow(&bmp, y);
		UInt32* dst = Bitmap_GetRow(&bmp, (height - 1) - y);

		Mem_Copy(tmp, src, stride);
		Mem_Copy(src, dst, stride);
		Mem_Copy(dst, tmp, stride);
		/*for (x = 0; x < bmp.Width; x++) {
			UInt32 temp = dst[x]; dst[x] = src[x]; src[x] = temp;
		}*/
	}

	ReturnCode res = Bitmap_EncodePng(&bmp, output);
	Mem_Free(bmp.Scan0);
	return res;
}

bool nv_mem;
void Gfx_MakeApiInfo(void) {
	int depthBits = 0;
	glGetIntegerv(GL_DEPTH_BITS, &depthBits);

	String_AppendConst(&Gfx_ApiInfo[0],"-- Using OpenGL --");
	String_Format1(&Gfx_ApiInfo[1], "Vendor: %c",     glGetString(GL_VENDOR));
	String_Format1(&Gfx_ApiInfo[2], "Renderer: %c",   glGetString(GL_RENDERER));
	String_Format1(&Gfx_ApiInfo[3], "GL version: %c", glGetString(GL_VERSION));
	/* Memory usage goes here */
	String_Format2(&Gfx_ApiInfo[5], "Max texture size: (%i, %i)", &Gfx_MaxTexWidth, &Gfx_MaxTexHeight);
	String_Format1(&Gfx_ApiInfo[6], "Depth buffer bits: %i", &depthBits);

	String extensions = String_FromReadonly(glGetString(GL_EXTENSIONS));
	String memExt     = String_FromConst("GL_NVX_gpu_memory_info");
	nv_mem = String_CaselessContains(&extensions, &memExt);
}

void Gfx_UpdateApiInfo(void) {
	if (!nv_mem) return;
	int totalKb = 0, curKb = 0;
	glGetIntegerv(0x9048, &totalKb);
	glGetIntegerv(0x9049, &curKb);

	if (totalKb <= 0 || curKb <= 0) return;
	Gfx_ApiInfo[4].length = 0;
	float total = totalKb / 1024.0f, cur = curKb / 1024.0f;
	String_Format2(&Gfx_ApiInfo[4], "Video memory: %f2 MB total, %f2 free", &total, &cur);
}

bool Gfx_WarnIfNecessary(void) {
#ifdef CC_BUILD_GL11
	Chat_AddRaw("&cYou are using the very outdated OpenGL backend.");
	Chat_AddRaw("&cAs such you may experience poor performance.");
	Chat_AddRaw("&cIt is likely you need to install video card drivers.");
#endif

	String renderer = String_FromReadonly(glGetString(GL_RENDERER));
	String intel = String_FromConst("Intel");
	if (!String_ContainsString(&renderer, &intel)) return false;

	Chat_AddRaw("&cIntel graphics cards are known to have issues with the OpenGL build.");
	Chat_AddRaw("&cVSync may not work, and you may see disappearing clouds and map edges.");
	Chat_AddRaw("&cFor Windows, try downloading the Direct3D 9 build instead.");
	return true;
}


void Gfx_SetVSync(bool value) {
	if (gl_vsync == value) return;
	gl_vsync = value;
	GLContext_SetVSync(value);
}

void Gfx_BeginFrame(void) { }
void Gfx_EndFrame(void) {
	GLContext_SwapBuffers();
#ifdef CC_BUILD_GL11
	gl_activeList = NULL;
#endif
}

void Gfx_OnWindowResize(void) {
	glViewport(0, 0, Game_Width, Game_Height);
}
#endif
