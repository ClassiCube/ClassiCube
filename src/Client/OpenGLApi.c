#include "GraphicsAPI.h"
#include "ErrorHandler.h"
#include "GraphicsEnums.h"
#include "Platform.h"
#include "Window.h"
#include "GraphicsCommon.h"
#include "Funcs.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <GL/gl.h>

#if !USE_DX
/* Extensions from later than OpenGL 1.1 */
#define GL_TEXTURE_MAX_LEVEL          0x813D
#define GL_ARRAY_BUFFER               0x8892
#define GL_ELEMENT_ARRAY_BUFFER       0x8893
#define GL_STATIC_DRAW                0x88E4
#define GL_DYNAMIC_DRAW               0x88E8
typedef void (APIENTRY *FN_GLBINDBUFFER) (GLenum target, GLuint buffer);
typedef void (APIENTRY *FN_GLDELETEBUFFERS) (GLsizei n, const GLuint *buffers);
typedef void (APIENTRY *FN_GLGENBUFFERS) (GLsizei n, GLuint *buffers);
typedef void (APIENTRY *FN_GLBUFFERDATA) (GLenum target, const void* size, const void *data, GLenum usage);
typedef void (APIENTRY *FN_GLBUFFERSUBDATA) (GLenum target, const void* offset, const void* size, const void *data);
/* TODO: NEED TO ASSIGN THESE TO VALID PROC ADDRESSES */
FN_GLBINDBUFFER glBindBuffer;
FN_GLDELETEBUFFERS glDeleteBuffers;
FN_GLGENBUFFERS glGenBuffers;
FN_GLBUFFERDATA glBufferData;
FN_GLBUFFERSUBDATA glBufferSubData;


bool gl_lists = false;
Int32 gl_activeList = -1;
#define gl_DYNAMICLISTID 1234567891
void* gl_dynamicListData;

Int32 gl_blend[6] = { GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA };
Int32 gl_compare[8] = { GL_ALWAYS, GL_NOTEQUAL, GL_NEVER, GL_LESS, GL_LEQUAL, GL_EQUAL, GL_GEQUAL, GL_GREATER };
Int32 gl_fogModes[3] = { GL_LINEAR, GL_EXP, GL_EXP2 };
Int32 gl_matrixModes[3] = { GL_PROJECTION, GL_MODELVIEW, GL_TEXTURE };

void GL_CheckVboSupport(void) {
	String extensions = String_FromReadonly(glGetString(GL_EXTENSIONS));
	String version = String_FromReadonly(glGetString(GL_VERSION));
	Int32 major = (Int32)(version.buffer[0] - '0'); /* x.y. (and so forth) */
	Int32 minor = (Int32)(version.buffer[2] - '0');

	/* Supported in core since 1.5 */
	if ((major > 1) || (major == 1 && minor >= 5)) {
		glBindBuffer = (FN_GLBINDBUFFER)GLContext_GetAddress("glBindBuffer");
		glDeleteBuffers = (FN_GLDELETEBUFFERS)GLContext_GetAddress("glDeleteBuffers");
		glGenBuffers = (FN_GLGENBUFFERS)GLContext_GetAddress("glGenBuffers");
		glBufferData = (FN_GLBUFFERDATA)GLContext_GetAddress("glBufferData");
		glBufferSubData = (FN_GLBUFFERSUBDATA)GLContext_GetAddress("glBufferSubData");
		return;
	}

	String vboExt = String_FromConst("GL_ARB_vertex_buffer_object");
	if (String_ContainsString(&extensions, &vboExt)) {
		glBindBuffer = (FN_GLBINDBUFFER)GLContext_GetAddress("glBindBufferARB");
		glDeleteBuffers = (FN_GLDELETEBUFFERS)GLContext_GetAddress("glDeleteBuffersARB");
		glGenBuffers = (FN_GLGENBUFFERS)GLContext_GetAddress("glGenBuffersARB");
		glBufferData = (FN_GLBUFFERDATA)GLContext_GetAddress("glBufferDataARB");
		glBufferSubData = (FN_GLBUFFERSUBDATA)GLContext_GetAddress("glBufferSubDataARB");
	} else {
		gl_lists = true;
		Gfx_CustomMipmapsLevels = false;
	}
}

void Gfx_Init(void) {
	GraphicsMode mode = GraphicsMode_MakeDefault();
	GLContext_Init(mode);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &Gfx_MaxTextureDimensions);
	gl_lists = Options_GetBool(OptionsKey_ForceOldOpenGL, false);
	Gfx_CustomMipmapsLevels = !gl_lists;

	GL_CheckVboSupport();
	GfxCommon_Init();
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
}

void Gfx_Free(void) {
	GfxCommon_Free();
	GLContext_Free();
}

#define GL_TOGGLE(cap)\
if (enabled) {\
glEnable(cap);\
} else {\
glDisable(cap);\
}

void GL_DoMipmaps(GfxResourceID texId, Int32 x, Int32 y, Bitmap* bmp, bool partial) {
	UInt8* prev = bmp->Scan0;
	Int32 lvls = GfxCommon_MipmapsLevels(bmp->Width, bmp->Height);
	Int32 lvl, width = bmp->Width, height = bmp->Height;

	for (lvl = 1; lvl <= lvls; lvl++) {
		x /= 2; y /= 2;
		if (width > 1) width /= 2;
		if (height > 1) height /= 2;
		UInt32 size = Bitmap_DataSize(width, height);

		UInt8* cur = Platform_MemAlloc(size);
		if (cur == NULL) ErrorHandler_Fail("Allocating memory for mipmaps");
		GfxCommon_GenMipmaps(width, height, cur, prev);

		if (partial) {
			glTexSubImage2D(GL_TEXTURE_2D, lvl, x, y, width, height,
				GL_BGRA_EXT, GL_UNSIGNED_BYTE, cur);
		} else {
			glTexImage2D(GL_TEXTURE_2D, lvl, GL_RGBA, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, cur);
		}

		if (prev != bmp->Scan0) Platform_MemFree(prev);
		prev = cur;
	}
	if (prev != bmp->Scan0) Platform_MemFree(prev);
}

GfxResourceID Gfx_CreateTexture(Bitmap* bmp, bool managedPool, bool mipmaps) {
	UInt32 texId;
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if (mipmaps) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		if (Gfx_CustomMipmapsLevels) {
			Int32 lvls = GfxCommon_MipmapsLevels(bmp->Width, bmp->Height);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, lvls);
		}
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bmp->Width, bmp->Height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, bmp->Scan0);

	if (mipmaps) GL_DoMipmaps(texId, 0, 0, bmp, false);
	return texId;
}

void Gfx_UpdateTexturePart(GfxResourceID texId, Int32 x, Int32 y, Bitmap* part, bool mipmaps) {
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, part->Width, part->Height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, part->Scan0);
	if (mipmaps) GL_DoMipmaps(texId, x, y, part, true);
}

void Gfx_BindTexture(GfxResourceID texId) {
	glBindTexture(GL_TEXTURE_2D, texId);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	if (*texId <= 0) return;
	glDeleteTextures(1, texId);
	*texId = NULL;
}

void Gfx_SetTexturing(bool enabled) { GL_TOGGLE(GL_TEXTURE_2D); }
void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }


bool gl_fogEnable;
bool Gfx_GetFog(void) { return gl_fogEnable; }
void Gfx_SetFog(bool enabled) {
	gl_fogEnable = enabled;
	GL_TOGGLE(GL_FOG);
}

PackedCol gl_lastFogCol;
void Gfx_SetFogColour(PackedCol col) {
	if (PackedCol_Equals(col, gl_lastFogCol)) return;
	Real32 colRGBA[4] = { col.R / 255.0f, col.G / 255.0f, col.B / 255.0f, col.A / 255.0f };
	glFogfv(GL_FOG_COLOR, colRGBA);
	gl_lastFogCol = col;
}

Real32 gl_lastFogEnd = -1, gl_lastFogDensity = -1;
void Gfx_SetFogDensity(Real32 value) {
	if (value == gl_lastFogDensity) return;
	glFogf(GL_FOG_DENSITY, gl_lastFogDensity);
	gl_lastFogDensity = value;
}

void Gfx_SetFogStart(Real32 value) {
	glFogf(GL_FOG_START, value);
}

void Gfx_SetFogEnd(Real32 value) {
	if (value == gl_lastFogEnd) return;
	glFogf(GL_FOG_END, gl_lastFogEnd);
	gl_lastFogEnd = value;
}

Fog gl_lastFogMode = -1;
void Gfx_SetFogMode(Fog mode) {
	if (mode == gl_lastFogMode) return;
	glFogi(GL_FOG_MODE, gl_fogModes[mode]);
	gl_lastFogMode = mode;
}


void Gfx_SetFaceCulling(bool enabled) { GL_TOGGLE(GL_CULL_FACE); }
void Gfx_SetAlphaTest(bool enabled) { GL_TOGGLE(GL_ALPHA_TEST); }
void Gfx_SetAlphaTestFunc(CompareFunc func, Real32 value) {
	glAlphaFunc(gl_compare[func], value);
}

void Gfx_SetAlphaBlending(bool enabled) { GL_TOGGLE(GL_BLEND); }
void Gfx_SetAlphaBlendFunc(BlendFunc srcFunc, BlendFunc dstFunc) {
	glBlendFunc(gl_blend[srcFunc], gl_blend[dstFunc]);
}
void Gfx_SetAlphaArgBlend(bool enabled) { }


void Gfx_Clear(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

PackedCol gl_lastClearCol;
void Gfx_ClearColour(PackedCol col) {
	if (PackedCol_Equals(col, gl_lastClearCol)) return;
	glClearColor(col.R / 255.0f, col.G / 255.0f, col.B / 255.0f, col.A / 255.0f);
	gl_lastClearCol = col;
}

void Gfx_SetColourWrite(bool enabled) {
	glColorMask(enabled, enabled, enabled, enabled);
}

void Gfx_SetDepthWrite(bool enabled) {
	glDepthMask(enabled);
}

void Gfx_SetDepthTest(bool enabled) { GL_TOGGLE(GL_DEPTH_TEST); }
void Gfx_SetDepthTestFunc(CompareFunc compareFunc) {
	glDepthFunc(gl_compare[compareFunc]);
}


GfxResourceID GL_GenAndBind(GLenum target) {
	Int32 id;
	glGenBuffers(1, &id);
	glBindBuffer(target, id);
	return id;
}

GfxResourceID Gfx_CreateDynamicVb(VertexFormat vertexFormat, Int32 maxVertices) {
	if (gl_lists) return gl_DYNAMICLISTID;
	Int32 id = GL_GenAndBind(GL_ARRAY_BUFFER);
	UInt32 sizeInBytes = maxVertices * Gfx_strideSizes[vertexFormat];
	glBufferData(GL_ARRAY_BUFFER, (void*)sizeInBytes, NULL, GL_DYNAMIC_DRAW);
	return id;
}

#define gl_MAXINDICES ICOUNT(65536)
GfxResourceID Gfx_CreateVb(void* vertices, VertexFormat vertexFormat, Int32 count) {
	if (gl_lists) {
		Int32 list = glGenLists(1);
		glNewList(list, GL_COMPILE);

		UInt16 indices[GFX_MAX_INDICES];
		GfxCommon_MakeIndices(indices, ICOUNT(count));

		Int32 stride = vertexFormat == VertexFormat_P3fT2fC4b ? VertexP3fT2fC4b_Size : VertexP3fC4b_Size;
		glVertexPointer(3, GL_FLOAT, stride, vertices);
		glColorPointer(4, GL_UNSIGNED_BYTE, stride, (void*)((UInt8*)vertices + 12));
		if (vertexFormat == VertexFormat_P3fT2fC4b) {
			glTexCoordPointer(2, GL_FLOAT, stride, (void*)((UInt8*)vertices + 16));
		}

		glDrawElements(GL_TRIANGLES, ICOUNT(count), GL_UNSIGNED_SHORT, indices);
		glEndList();
		return list;
	}

	Int32 id = GL_GenAndBind(GL_ARRAY_BUFFER);
	UInt32 sizeInBytes = count * Gfx_strideSizes[vertexFormat];
	glBufferData(GL_ARRAY_BUFFER, (void*)sizeInBytes, vertices, GL_STATIC_DRAW);
	return id;
}

GfxResourceID Gfx_CreateIb(void* indices, Int32 indicesCount) {
	if (gl_lists) return 0;
	Int32 id = GL_GenAndBind(GL_ELEMENT_ARRAY_BUFFER);
	UInt32 sizeInBytes = indicesCount * sizeof(UInt16);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (void*)sizeInBytes, indices, GL_STATIC_DRAW);
	return id;
}

void Gfx_BindVb(GfxResourceID vb) {
	if (gl_lists) { 
		gl_activeList = vb; 
	} else { 
		glBindBuffer(GL_ARRAY_BUFFER, vb);
	}
}

void Gfx_BindIb(GfxResourceID ib) {
	if (gl_lists) return;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
}

void Gfx_DeleteVb(GfxResourceID* vb) {
	if (*vb <= 0) return;

	if (gl_lists) { 
		if (*vb != gl_DYNAMICLISTID) glDeleteLists(*vb, 1); 
	} else { 
		glDeleteBuffers(1, vb); 
	}
	*vb = NULL;
}

void Gfx_DeleteIb(GfxResourceID* ib) {
	if (gl_lists || *ib <= 0) return;
	glDeleteBuffers(1, ib);
	*ib = NULL;
}


typedef void (*GL_SetupVBFunc)(void);
typedef void (*GL_SetupVBRangeFunc)(Int32 startVertex);
GL_SetupVBFunc gl_setupVBFunc;
GL_SetupVBRangeFunc gl_setupVBRangeFunc;
Int32 gl_batchStride;
VertexFormat gl_batchFormat = -1;

void GL_SetupVbPos3fCol4b(void) {
	glVertexPointer(3, GL_FLOAT, VertexP3fC4b_Size, (void*)0);
	glColorPointer(4, GL_UNSIGNED_BYTE, VertexP3fC4b_Size, (void*)12);
}

void GL_SetupVbPos3fTex2fCol4b(void) {
	glVertexPointer(3, GL_FLOAT, VertexP3fT2fC4b_Size, (void*)0);
	glColorPointer(4, GL_UNSIGNED_BYTE, VertexP3fT2fC4b_Size, (void*)12);
	glTexCoordPointer(2, GL_FLOAT, VertexP3fT2fC4b_Size, (void*)16);
}

void GL_SetupVbPos3fCol4b_Range(Int32 startVertex) {
	UInt32 offset = startVertex * VertexP3fC4b_Size;
	glVertexPointer(3, GL_FLOAT, VertexP3fC4b_Size, (void*)(offset));
	glColorPointer(4, GL_UNSIGNED_BYTE, VertexP3fC4b_Size, (void*)(offset + 12));
}

void GL_SetupVbPos3fTex2fCol4b_Range(Int32 startVertex) {
	UInt32 offset = startVertex * VertexP3fT2fC4b_Size;
	glVertexPointer(3, GL_FLOAT, VertexP3fT2fC4b_Size, (void*)(offset));
	glColorPointer(4, GL_UNSIGNED_BYTE, VertexP3fT2fC4b_Size, (void*)(offset + 12));
	glTexCoordPointer(2, GL_FLOAT, VertexP3fT2fC4b_Size, (void*)(offset + 16));
}

void Gfx_SetBatchFormat(VertexFormat vertexFormat) {
	if (vertexFormat == gl_batchFormat) return;

	if (gl_batchFormat == VertexFormat_P3fT2fC4b) {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	gl_batchFormat = vertexFormat;
	gl_batchStride = Gfx_strideSizes[vertexFormat];

	if (vertexFormat == VertexFormat_P3fT2fC4b) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		gl_setupVBFunc = GL_SetupVbPos3fTex2fCol4b;
		gl_setupVBRangeFunc = GL_SetupVbPos3fTex2fCol4b_Range;
	} else {
		gl_setupVBFunc = GL_SetupVbPos3fCol4b;
		gl_setupVBRangeFunc = GL_SetupVbPos3fCol4b_Range;
	}
}

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, Int32 vCount) {
	if (gl_lists) {
		gl_activeList = gl_DYNAMICLISTID;
		gl_dynamicListData = vertices;
		return;
	}

	glBindBuffer(GL_ARRAY_BUFFER, vb);
	UInt32 sizeInBytes = vCount * gl_batchStride;
	glBufferSubData(GL_ARRAY_BUFFER, NULL, (void*)sizeInBytes, vertices);
}

void GL_V16(VertexP3fC4b v) {
	glColor4ub(v.Col.R, v.Col.G, v.Col.B, v.Col.A);
	glVertex3f(v.X, v.Y, v.Z);
}

void GL_V24(VertexP3fT2fC4b v) {
	glColor4ub(v.Col.R, v.Col.G, v.Col.B, v.Col.A);
	glTexCoord2f(v.U, v.V);
	glVertex3f(v.X, v.Y, v.Z);
}

void GL_DrawDynamicLines(Int32 verticesCount) {
	glBegin(GL_LINES);
	Int32 i;
	if (gl_batchFormat == VertexFormat_P3fT2fC4b) {
		VertexP3fT2fC4b* ptr = (VertexP3fT2fC4b*)gl_dynamicListData;
		for (i = 0; i < verticesCount; i += 2) {
			GL_V24(ptr[i + 0]); GL_V24(ptr[i + 1]);
		}
	} else {
		VertexP3fC4b* ptr = (VertexP3fC4b*)gl_dynamicListData;
		for (i = 0; i < verticesCount; i += 2) {
			GL_V16(ptr[i + 0]); GL_V16(ptr[i + 1]);
		}
	}
	glEnd();
}

void GL_DrawDynamicTriangles(Int32 verticesCount, Int32 startVertex) {
	glBegin(GL_TRIANGLES);
	Int32 i;
	if (gl_batchFormat == VertexFormat_P3fT2fC4b) {
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

void Gfx_DrawVb_Lines(Int32 verticesCount) {
	if (gl_lists) { GL_DrawDynamicLines(verticesCount); return; }

	gl_setupVBFunc();
	glDrawArrays(GL_LINES, 0, verticesCount);
}

void Gfx_DrawVb_IndexedTris_Range(Int32 verticesCount, Int32 startVertex) {
	if (gl_lists) {
		if (gl_activeList != gl_DYNAMICLISTID) { glCallList(gl_activeList); }
		else { GL_DrawDynamicTriangles(verticesCount, startVertex); }
		return;
	}

	gl_setupVBRangeFunc(startVertex);
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, NULL);
}

void Gfx_DrawVb_IndexedTris(Int32 verticesCount) {
	if (gl_lists) {
		if (gl_activeList != gl_DYNAMICLISTID) { glCallList(gl_activeList); }
		else { GL_DrawDynamicTriangles(verticesCount, 0); }
		return;
	}

	gl_setupVBFunc();
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, NULL);
}

Int32 gl_lastPartialList = -1;
void Gfx_DrawIndexedVb_TrisT2fC4b(Int32 verticesCount, Int32 startVertex) {
	/* TODO: This renders the whole map, bad performance!! FIX FIX */
	if (gl_lists) {
		if (gl_activeList != gl_lastPartialList) {
			glCallList(gl_activeList); 
			gl_lastPartialList = gl_activeList;
		}
		return;
	}

	UInt32 offset = startVertex * VertexP3fT2fC4b_Size;
	glVertexPointer(3, GL_FLOAT, VertexP3fT2fC4b_Size, (void*)(offset));
	glColorPointer(4, GL_UNSIGNED_BYTE, VertexP3fT2fC4b_Size, (void*)(offset + 12));
	glTexCoordPointer(2, GL_FLOAT, VertexP3fT2fC4b_Size, (void*)(offset + 16));
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, NULL);
}


MatrixType gl_lastMatrixType = 0;
void Gfx_SetMatrixMode(MatrixType matrixType) {
	if (matrixType == gl_lastMatrixType) return;
	glMatrixMode(gl_matrixModes[matrixType]);
	gl_lastMatrixType = matrixType;
}

void Gfx_LoadMatrix(Matrix* matrix) { glLoadMatrixf((Real32*)matrix); }
void Gfx_LoadIdentityMatrix(void) { glLoadIdentity(); }
void Gfx_MultiplyMatrix(Matrix* matrix) { glMultMatrixf((Real32*)matrix); }
void Gfx_PushMatrix(void) { glPushMatrix(); }
void Gfx_PopMatrix(void) { glPopMatrix(); }

void Gfx_LoadOrthoMatrix(Real32 width, Real32 height) {
	Matrix matrix;
	Matrix_OrthographicOffCenter(&matrix, 0.0f, width, height, 0.0f, -10000.0f, 10000.0f);
	Gfx_LoadMatrix(&matrix);
}


void Gfx_BeginFrame(void) { }
void Gfx_EndFrame(void) {
	GLContext_SwapBuffers();
	gl_activeList = -1;
}

void Gfx_OnWindowResize(void) {
	glViewport(0, 0, Game_Width, Game_Height);
}
#endif