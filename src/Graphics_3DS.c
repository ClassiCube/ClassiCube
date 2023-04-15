#include "Core.h"

// https://gbatemp.net/threads/homebrew-development.360646/page-245
// 3DS defaults to stack size of *32 KB*.. way too small
unsigned int __stacksize__ = 256 * 1024;

//#define CC_BUILD_GL
//#include "Graphics_GL1.c"

//#undef CC_BUILD_3DS
#if defined CC_BUILD_3DS
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"
#include <3ds.h>
#include <citro3d.h>
#include "_3DS_textured_shbin.h"// auto generated from .v.pica files by Makefile
#include "_3DS_coloured_shbin.h"
#include "_3DS_offset_shbin.h"

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

/* Current format and size of vertices */
static int gfx_stride, gfx_format = -1;
	
	
/*########################################################################################################################*
*------------------------------------------------------Vertex shaders-----------------------------------------------------*
*#########################################################################################################################*/
#define UNI_MVP_MATRIX (1 << 0)

/* cached uniforms (cached for multiple programs */
static struct Matrix _view, _proj, _mvp;

static struct CCShader {
	DVLB_s* dvlb;
	shaderProgram_s program;
	int uniforms;     /* which associated uniforms need to be resent to GPU */
	int locations[1]; /* location of uniforms (not constant) */
};
static struct CCShader* gfx_activeShader;
static struct CCShader shaders[2];

static void Shader_Alloc(struct CCShader* shader, u8* binData, int binSize) {
	shader->dvlb = DVLB_ParseFile((u32*)binData, binSize);
	shaderProgramInit(&shader->program);
	shaderProgramSetVsh(&shader->program, &shader->dvlb->DVLE[0]);
	
	shader->locations[0] = shaderInstanceGetUniformLocation(shader->program.vertexShader, "MVP");
}

static void Shader_Free(struct CCShader* shader) {
	shaderProgramFree(&shader->program);
	DVLB_Free(shader->dvlb);
}

/* Marks a uniform as changed on all programs */
static void DirtyUniform(int uniform) {
	int i;
	for (int i = 0; i < Array_Elems(shaders); i++) {
		shaders[i].uniforms |= uniform;
	}
}

/* Sends changed uniforms to the GPU for current program */
static void ReloadUniforms(void) {
	struct CCShader* s = gfx_activeShader;
	if (!s) return; /* NULL if context is lost */

	if (s->uniforms & UNI_MVP_MATRIX) {
		C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, s->locations[0], &_mvp);
		s->uniforms &= ~UNI_MVP_MATRIX;
	}
}

/* Switches program to one that can render current vertex format and state */
/* Compiles program and reloads uniforms if needed */
static void SwitchProgram(void) {
	struct CCShader* shader;
	int index = 0;

	if (gfx_format == VERTEX_FORMAT_TEXTURED) index++;

	shader = &shaders[index];
	if (shader != gfx_activeShader) {
		gfx_activeShader = shader;
		C3D_BindProgram(&shader->program);
	}
	ReloadUniforms();
}


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
static C3D_RenderTarget* target;

static void AllocShaders(void) {
	Shader_Alloc(&shaders[0], (u32*)_3DS_coloured_shbin, _3DS_coloured_shbin_size);
	Shader_Alloc(&shaders[1], (u32*)_3DS_textured_shbin, _3DS_textured_shbin_size);
}

static void FreeShaders(void) {
	for (int i = 0; i < Array_Elems(shaders); i++) {
		Shader_Free(&shaders[i]);
	}
}

void Gfx_Create(void) {
	Platform_LogConst("BEGIN..");
	Gfx.MaxTexWidth  = 512;
	Gfx.MaxTexHeight = 512;
	Gfx.Created      = true;
	
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	InitDefaultResources();
	AllocShaders();
	
	// Configure the first fragment shading substage to just pass through the vertex color
	// See https://www.opengl.org/sdk/docs/man2/xhtml/glTexEnv.xml for more insight
	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);
	C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
}

cc_bool Gfx_TryRestoreContext(void) { return true; }

void Gfx_Free(void) { FreeShaders(); }
void Gfx_RestoreState(void) { }
void Gfx_FreeState(void) { }

/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
GfxResourceID Gfx_CreateTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	return 0;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
}

void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }

void Gfx_BindTexture(GfxResourceID texId) {
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFaceCulling(cc_bool enabled)   { }
void Gfx_SetAlphaBlending(cc_bool enabled) { }
void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

static PackedCol clear_color;
void Gfx_ClearCol(PackedCol color) { 
	//clear_color = color;
	// TODO find better way?
	clear_color = (PackedCol_R(color) << 24) | (PackedCol_G(color) << 16) | (PackedCol_B(color) << 8) | 0xFF;
}

void Gfx_SetColWriteMask(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
}

void Gfx_SetDepthWrite(cc_bool enabled) { }
void Gfx_SetDepthTest(cc_bool enabled)  { }


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_CalcOrthoMatrix(float width, float height, struct Matrix* matrix) {
	//Matrix_Orthographic(matrix, 0.0f, width, 0.0f, height, ORTHO_NEAR, ORTHO_FAR);
	Mtx_Ortho(matrix, 0.0f, width, 0.0f, height, ORTHO_NEAR, ORTHO_FAR, false);
}
void Gfx_CalcPerspectiveMatrix(float fov, float aspect, float zFar, struct Matrix* matrix) {
	float zNear = 0.1f;
	//Matrix_PerspectiveFieldOfView(matrix, fov, aspect, zNear, zFar);
	Mtx_Persp(matrix, fov, aspect, 0.1f, zFar, false);
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_GetApiInfo(cc_string* info) {
	int pointerSize = sizeof(void*) * 8;

	String_Format1(info, "-- Using 3DS (%i bit) --\n", &pointerSize);
	String_Format2(info, "Max texture size: (%i, %i)\n", &Gfx.MaxTexWidth, &Gfx.MaxTexHeight);
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
}

void Gfx_BeginFrame(void) {
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
}
void Gfx_Clear(void) {
	//Platform_Log1("CLEAR: %i", &clear_color);
	C3D_RenderTargetClear(target, C3D_CLEAR_ALL, clear_color, 0);
	C3D_FrameDrawOn(target);
}

void Gfx_EndFrame(void) {
	C3D_FrameEnd(0);
	//gfxFlushBuffers();
	//gfxSwapBuffers();

	//Platform_LogConst("FRAME!");
	//if (gfx_vsync) gspWaitForVBlank();
	if (gfx_minFrameMs) LimitFPS();
}

void Gfx_OnWindowResize(void) { }


/*########################################################################################################################*
*----------------------------------------------------------Buffers--------------------------------------------------------*
*#########################################################################################################################*/
static cc_uint8* gfx_vertices;
static cc_uint16* gfx_indices;

static void* AllocBuffer(int count, int elemSize) {
	void* ptr = linearAlloc(count * elemSize);
	cc_uintptr addr = ptr;
	Platform_Log3("BUFFER CREATE: %i X %i = %x", &count, &elemSize, &addr);
	
	if (!ptr) Logger_Abort("Failed to allocate memory for buffer");
	return ptr;
}

static void FreeBuffer(GfxResourceID* buffer) {
	GfxResourceID ptr = *buffer;
	if (!ptr) return;
	linearFree(ptr);
	*buffer = 0;
}

GfxResourceID Gfx_CreateIb(void* indices, int indicesCount) { 
	void* ptr = AllocBuffer(indicesCount, 2);
	Mem_Copy(ptr, indices, indicesCount * 2);
	return ptr;
}

void Gfx_BindIb(GfxResourceID ib)    { gfx_indices = ib; }

void Gfx_DeleteIb(GfxResourceID* ib) { FreeBuffer(ib); }


GfxResourceID Gfx_CreateVb(VertexFormat fmt, int count) {
	return AllocBuffer(count, strideSizes[fmt]);
}

void Gfx_BindVb(GfxResourceID vb) { 
	gfx_vertices = vb; // https://github.com/devkitPro/citro3d/issues/47
	// "Fyi the permutation specifies the order in which the attributes are stored in the buffer, LSB first. So 0x210 indicates attributes 0, 1 & 2."
	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
  	BufInfo_Init(bufInfo);

	if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		BufInfo_Add(bufInfo, vb, SIZEOF_VERTEX_TEXTURED, 3, 0x210);
	} else {
		BufInfo_Add(bufInfo, vb, SIZEOF_VERTEX_COLOURED, 2,  0x10);
	}
}

void Gfx_DeleteVb(GfxResourceID* vb) { FreeBuffer(vb); }

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return vb;
}

void Gfx_UnlockVb(GfxResourceID vb) { gfx_vertices = vb; }


GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices)  {
	return AllocBuffer(maxVertices, strideSizes[fmt]);
}

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) { 
	return vb; 
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) { gfx_vertices = vb; }

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	Mem_Copy(vb, vertices, vCount * gfx_stride);
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFog(cc_bool enabled) {
}

void Gfx_SetFogCol(PackedCol color) {
}

void Gfx_SetFogDensity(float value) {
}

void Gfx_SetFogEnd(float value) {
}

void Gfx_SetFogMode(FogFunc func) {
}

void Gfx_SetAlphaTest(cc_bool enabled) { 
	C3D_AlphaTest(enabled, GPU_GREATER, 0x7F);
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW)       _view = *matrix;
	if (type == MATRIX_PROJECTION) _proj = *matrix;

	Matrix_Mul(&_mvp, &_view, &_proj);
	DirtyUniform(UNI_MVP_MATRIX);
	ReloadUniforms();
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
	Gfx_LoadMatrix(type, &Matrix_Identity);
}

void Gfx_EnableTextureOffset(float x, float y) {
}

void Gfx_DisableTextureOffset(void) {
}



/*########################################################################################################################*
*---------------------------------------------------------Drawing---------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Gfx_WarnIfNecessary(void) { return false; }

void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
	SwitchProgram();
	
	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3);         // in_pos
	AttrInfo_AddLoader(attrInfo, 1, GPU_UNSIGNED_BYTE, 4); // in_col
	if (fmt == VERTEX_FORMAT_TEXTURED) {
		AttrInfo_AddLoader(attrInfo, 2, GPU_FLOAT, 2); // in_tex
	}
}

void Gfx_DrawVb_Lines(int verticesCount) {
}

static void SetVertexBuffer(int startVertex) {
	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
  	BufInfo_Init(bufInfo);

	if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		BufInfo_Add(bufInfo, (struct VertexTextured*)gfx_vertices + startVertex, SIZEOF_VERTEX_TEXTURED, 3, 0x210);
	} else {
		BufInfo_Add(bufInfo, (struct VertexColoured*)gfx_vertices + startVertex, SIZEOF_VERTEX_COLOURED, 2,  0x10);
	}
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	SetVertexBuffer(startVertex);
	C3D_DrawElements(GPU_TRIANGLES, ICOUNT(verticesCount), C3D_UNSIGNED_SHORT, gfx_indices);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	SetVertexBuffer(0);
	C3D_DrawElements(GPU_TRIANGLES, ICOUNT(verticesCount), C3D_UNSIGNED_SHORT, gfx_indices);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	SetVertexBuffer(startVertex);
	C3D_DrawElements(GPU_TRIANGLES, ICOUNT(verticesCount), C3D_UNSIGNED_SHORT, gfx_indices);
}


// this doesn't work properly, because (index buffer + offset) must be aligned to 16 bytes
/*void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	C3D_DrawElements(GPU_TRIANGLES, ICOUNT(verticesCount), C3D_UNSIGNED_SHORT, gfx_indices + startVertex);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	C3D_DrawElements(GPU_TRIANGLES, ICOUNT(verticesCount), C3D_UNSIGNED_SHORT, gfx_indices);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	C3D_DrawElements(GPU_TRIANGLES, ICOUNT(verticesCount), C3D_UNSIGNED_SHORT, gfx_indices + startVertex);
}*/
#endif