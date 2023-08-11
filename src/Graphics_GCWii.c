#include "Core.h"
#if defined CC_BUILD_GCWII
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"
#include <malloc.h>
#include <string.h>
#include <gccore.h>

static void* fifo_buffer;
#define FIFO_SIZE (256 * 1024)
extern void* Window_XFB;
static void* xfbs[2];
static  int curFB;
static GfxResourceID white_square;
// https://wiibrew.org/wiki/Developer_tips
// https://devkitpro.org/wiki/libogc/GX


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
static void InitGX(void) {
	GXRModeObj* mode = VIDEO_GetPreferredMode(NULL);
	fifo_buffer = MEM_K0_TO_K1(memalign(32, FIFO_SIZE));
	memset(fifo_buffer, 0, FIFO_SIZE);

	GX_Init(fifo_buffer, FIFO_SIZE);
	GX_SetViewport(0, 0, mode->fbWidth, mode->efbHeight, 0, 1);
	GX_SetDispCopyYScale((f32)mode->xfbHeight / (f32)mode->efbHeight);
	GX_SetScissor(0, 0, mode->fbWidth, mode->efbHeight);
	GX_SetDispCopySrc(0, 0, mode->fbWidth, mode->efbHeight);
	GX_SetDispCopyDst(mode->fbWidth, mode->xfbHeight);
	GX_SetCopyFilter(mode->aa, mode->sample_pattern,
					 GX_TRUE, mode->vfilter);
	GX_SetFieldMode(mode->field_rendering,
					((mode->viHeight==2*mode->xfbHeight)?GX_ENABLE:GX_DISABLE));

	GX_SetCullMode(GX_CULL_NONE);
	GX_SetDispCopyGamma(GX_GM_1_0);
	GX_InvVtxCache();
	
	GX_SetNumChans(1);
	
	xfbs[0] = Window_XFB;
	xfbs[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(mode));
}

void Gfx_Create(void) {
	Gfx.MaxTexWidth  = 512;
	Gfx.MaxTexHeight = 512;
	Gfx.Created      = true;
	
	InitGX();
	Gfx_RestoreState();
}

void Gfx_Free(void) { 
	Gfx_FreeState();
	GX_AbortFrame();
	//GX_Flush(); // TODO needed?
	VIDEO_Flush();
}
cc_bool Gfx_TryRestoreContext(void) { return true; }

void Gfx_RestoreState(void) { 
	InitDefaultResources();

	// 4x4 dummy white texture (textures must be at least 1 4x4 tile)
	struct Bitmap bmp;
	BitmapCol pixels[4 * 4];
	Mem_Set(pixels, 0xFF, sizeof(pixels));
	Bitmap_Init(bmp, 4, 4, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
}

void Gfx_FreeState(void) { 
	FreeDefaultResources();
	Gfx_DeleteTexture(&white_square);
}

/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
typedef struct CCTexture_ {
	GXTexObj obj;
	cc_uint32 pixels[];
} CCTexture;

// ClassiCube RGBA8 bitmaps
// - store pixels in simple linear order
//    i.e. pixels are ordered as (x=0,y=0), (x=1,y=0), (x=2,y=0) ... (x=0,y=1) ...
// - store colour components in interleaved order
//    i.e. pixels are stored in memory as ARGB ARGB ARGB ARGB ...
// GX RGBA8 textures
// - store pixels in 4x4 tiles
// - store all of the AR values of the tile's pixels, then store all of the GB values
static void ReorderPixels(CCTexture* tex, struct Bitmap* bmp, 
			int originX, int originY, int rowWidth) {
	int stride = GX_GetTexObjWidth(&tex->obj) * 4;
	// TODO not really right
	// TODO originX ignored
	originX &= ~0x03;
	originY &= ~0x03;
	
	// http://hitmen.c02.at/files/yagcd/yagcd/chap15.html
	//  section 15.35  TPL (Texture Palette)
	// "RGBA8 (4x4 tiles in two cache lines - first is AR and second is GB"
	uint8_t *src = (uint8_t*)bmp->scan0;
	uint8_t *dst = (uint8_t*)tex->pixels + stride * originY;
	int srcWidth = bmp->width, srcHeight = bmp->height;
	
	for (int tileY = 0; tileY < srcHeight; tileY += 4)
		for (int tileX = 0; tileX < srcWidth; tileX += 4) 
	{
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				uint32_t idx = (((tileY + y) * rowWidth) + tileX + x) << 2;

				*dst++ = src[idx + 0]; // A
				*dst++ = src[idx + 1]; // R
			}
		}
		
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				uint32_t idx = (((tileY + y) * rowWidth) + tileX + x) << 2;

				*dst++ = src[idx + 2]; // G
				*dst++ = src[idx + 3]; // B
			}
		}
	}
}

GfxResourceID Gfx_CreateTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	if (bmp->width < 4 || bmp->height < 4) {
		Platform_LogConst("ERROR: Tried to create texture smaller than 4x4");
		return 0;
	}
	
	int size = bmp->width * bmp->height * 4;
	CCTexture* tex = (CCTexture*)memalign(32, 32 + size);
	
	GX_InitTexObj(&tex->obj, tex->pixels, bmp->width, bmp->height,
			GX_TF_RGBA8, GX_REPEAT, GX_REPEAT, GX_FALSE);
	GX_InitTexObjFilterMode(&tex->obj, GX_NEAR, GX_NEAR);
			
	ReorderPixels(tex, bmp, 0, 0, bmp->width);
	DCFlushRange(tex->pixels, size);
	return tex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	CCTexture* tex = (CCTexture*)texId;
	// TODO: wrong behaviour if x/y/part isn't multiple of 4 pixels
	ReorderPixels(tex, part, x, y, rowWidth);
	GX_InvalidateTexAll();
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
	Gfx_UpdateTexture(texId, x, y, part, part->width, mipmaps);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	GfxResourceID data = *texId;
	if (data) Mem_Free(data);
	*texId = NULL;
}

void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }

void Gfx_BindTexture(GfxResourceID texId) {
	CCTexture* tex = (CCTexture*)texId;
	if (!tex) tex = white_square;

	GX_LoadTexObj(&tex->obj, GX_TEXMAP0);
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static GXColor gfx_clearColor = { 0, 0, 0, 255 };

void Gfx_SetFaceCulling(cc_bool enabled) { 
	// NOTE: seems like ClassiCube's triangle ordering is opposite of what GX considers front facing
	GX_SetCullMode(enabled ? GX_CULL_FRONT : GX_CULL_NONE);
}

void Gfx_SetAlphaBlending(cc_bool enabled) {
	if (enabled) {
		GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	} else {
		GX_SetBlendMode(GX_BM_NONE,  GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
	}
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { 
}

void Gfx_ClearCol(PackedCol color) {
	gfx_clearColor.r = PackedCol_R(color);
	gfx_clearColor.g = PackedCol_G(color);
	gfx_clearColor.b = PackedCol_B(color);
	
	GX_SetCopyClear(gfx_clearColor, 0x00ffffff); // TODO: use GX_MAX_Z24 
}

void Gfx_SetColWriteMask(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
}

static cc_bool depth_write = true, depth_test = true;
static void UpdateDepthState(void) {
	// match Desktop behaviour, where disabling depth testing also disables depth writing
	// TODO do we actually need to & here?
	GX_SetZMode(depth_test, GX_LEQUAL, depth_write & depth_test);
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	depth_write = enabled;
	UpdateDepthState();
}

void Gfx_SetDepthTest(cc_bool enabled) {
	depth_test = enabled;
	UpdateDepthState();
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_GetApiInfo(cc_string* info) {
	String_Format1(info, "-- Using GC/WII --", NULL);
	String_Format2(info, "Max texture size: (%i, %i)\n", &Gfx.MaxTexWidth, &Gfx.MaxTexHeight);
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
}

void Gfx_BeginFrame(void) {
}

void Gfx_Clear(void) {
}

void Gfx_EndFrame(void) {
	curFB ^= 1; // swap "front" and "back" buffers
	GX_CopyDisp(xfbs[curFB], GX_TRUE);
	GX_DrawDone();
	
	VIDEO_SetNextFramebuffer(xfbs[curFB]);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if (gfx_minFrameMs) LimitFPS();
}

void Gfx_OnWindowResize(void) { }

cc_bool Gfx_WarnIfNecessary(void) { return false; }


/*########################################################################################################################*
*-------------------------------------------------------Index Buffers-----------------------------------------------------*
*#########################################################################################################################*/
//static cc_uint16 __attribute__((aligned(16))) gfx_indices[GFX_MAX_INDICES];

GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	//fillFunc(gfx_indices, count, obj);
	// not used since render using GX_QUADS anyways
	return 1;
}

void Gfx_BindIb(GfxResourceID ib) { }
void Gfx_DeleteIb(GfxResourceID* ib) { }


/*########################################################################################################################*
*------------------------------------------------------Vertex Buffers-----------------------------------------------------*
*#########################################################################################################################*/
static cc_uint8* gfx_vertices;
static int vb_size;

GfxResourceID Gfx_CreateVb(VertexFormat fmt, int count) {
	void* data = memalign(16, count * strideSizes[fmt]);
	if (!data) Logger_Abort("Failed to allocate memory for GFX VB");
	return data;
}

void Gfx_BindVb(GfxResourceID vb) { gfx_vertices = vb; }

void Gfx_DeleteVb(GfxResourceID* vb) {
	GfxResourceID data = *vb;
	if (data) Mem_Free(data);
	*vb = 0;
}

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	vb_size = count * strideSizes[fmt];
	return vb;
}

void Gfx_UnlockVb(GfxResourceID vb) { 
	gfx_vertices = vb; 
	DCFlushRange(vb, vb_size);
}


GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices) {
	void* data = memalign(16, maxVertices * strideSizes[fmt]);
	if (!data) Logger_Abort("Failed to allocate memory for GFX VB");
	return data;
}

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	vb_size = count * strideSizes[fmt];
	return vb; 
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) { 
	gfx_vertices = vb;
	DCFlushRange(vb, vb_size);
}

// Current size of vertices
static int gfx_stride; // TODO move down to Drawing area ??
void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	gfx_vertices = vb;
	Mem_Copy(vb, vertices, vCount * gfx_stride);
	DCFlushRange(vertices, vCount * gfx_stride);
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gfx_fogColor;
static float gfx_fogEnd = -1.0f, gfx_fogDensity = -1.0f;
static int gfx_fogMode  = -1;

void Gfx_SetFog(cc_bool enabled) {
	gfx_fogEnabled = enabled;
}

void Gfx_SetFogCol(PackedCol color) {
	if (color == gfx_fogColor) return;
	gfx_fogColor = color;
}

void Gfx_SetFogDensity(float value) {
}

void Gfx_SetFogEnd(float value) {
	if (value == gfx_fogEnd) return;
	gfx_fogEnd = value;
}

void Gfx_SetFogMode(FogFunc func) {
}

void Gfx_SetAlphaTest(cc_bool enabled) {
	if (enabled) {
		GX_SetAlphaCompare(GX_GREATER, 127, GX_AOP_AND, GX_ALWAYS, 0);
	} else {
		GX_SetAlphaCompare(GX_ALWAYS,    0, GX_AOP_AND, GX_ALWAYS, 0);
	}
	
	// See explanation from libGX headers
	//   Normally, Z buffering should happen before texturing, as this enables better performance by not texturing pixels that are not visible; 
	//   however, when alpha compare is used, Z buffering must be done after texturing
	// Parameter[0] = Enables Z-buffering before texturing when set to GX_TRUE; otherwise, Z-buffering takes place after texturing.
	GX_SetZCompLoc(!enabled);
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	GX_SetColorUpdate(!depthOnly);
	GX_SetAlphaUpdate(!depthOnly);
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	// Transposed, source guOrtho https://github.com/devkitPro/libogc/blob/master/libogc/gu.c
	//   The simplified calculation below uses: L = 0, R = width, T = 0, B = height
	*matrix = Matrix_Identity;
	
	matrix->row1.X =  2.0f / width;
	matrix->row2.Y = -2.0f / height;
	matrix->row3.Z = -1.0f / (zFar - zNear);

	matrix->row4.X = -1.0f;
	matrix->row4.Y =  1.0f;
	matrix->row4.Z = -zFar / (zFar - zNear);
}

static double Cotangent(double x) { return Math_Cos(x) / Math_Sin(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	float zNear = 0.1f;
	float c = (float)Cotangent(0.5f * fov);
	
	// Transposed, source guPersepctive https://github.com/devkitPro/libogc/blob/master/libogc/gu.c
	*matrix = Matrix_Identity;
	matrix->row1.X = c / aspect;
	matrix->row2.Y = c;
	matrix->row3.Z = -(zNear)        / (zFar - zNear);
	matrix->row3.W = -1.0f;
	matrix->row4.Z = -(zFar * zNear) / (zFar - zNear);
	matrix->row4.W =  0.0f;
}

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	const float* m = (const float*)matrix;
	float tmp[16];
	
	// Transpose matrix
	for (int i = 0; i < 4; i++)
	{
		tmp[i * 4 + 0] = m[0  + i];
		tmp[i * 4 + 1] = m[4  + i];
		tmp[i * 4 + 2] = m[8  + i];
		tmp[i * 4 + 3] = m[12 + i];
	}
		
	if (type == MATRIX_PROJECTION) {
		GX_LoadProjectionMtx(tmp,
			tmp[3*4+3] == 0.0f ? GX_PERSPECTIVE : GX_ORTHOGRAPHIC);
	} else {
		GX_LoadPosMtxImm(tmp, GX_PNMTX0);
	}
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
	Gfx_LoadMatrix(type, &Matrix_Identity);
}
static float texOffsetX, texOffsetY;
static void UpdateTexCoordGen(void) {
	if (texOffsetX || texOffsetY) {
		Mtx mat   = { 0 };
		// https://registry.khronos.org/OpenGL-Refpages/gl2.1/xhtml/glTranslate.xml
		mat[0][0] = 1; mat[0][3] = texOffsetX;
		mat[1][1] = 1; mat[1][3] = texOffsetY;
		
		GX_LoadTexMtxImm(mat, GX_TEXMTX0, GX_MTX2x4);
		GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0);
	} else {
		GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
	}
}

void Gfx_EnableTextureOffset(float x, float y) {
	texOffsetX = x; texOffsetY = y;
	UpdateTexCoordGen();
}

void Gfx_DisableTextureOffset(void) {
	texOffsetX = 0; texOffsetY = 0;
	UpdateTexCoordGen();
}



/*########################################################################################################################*
*---------------------------------------------------------Drawing---------------------------------------------------------*
*#########################################################################################################################*/
// Current format and size of vertices
static int gfx_format = -1;

void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];

	GX_ClearVtxDesc();
	if (fmt == VERTEX_FORMAT_TEXTURED) {
		GX_SetVtxDesc(GX_VA_POS,  GX_DIRECT);
		GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,  GX_POS_XYZ,  GX_F32,   0);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST,   GX_F32,   0);

		GX_SetNumTexGens(1);
		// TODO avoid calling here. only call in Gfx_Init and Enable/Disable Texture Offset ???
		UpdateTexCoordGen();
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
	} else {
		GX_SetVtxDesc(GX_VA_POS,  GX_DIRECT);
		GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,  GX_POS_XYZ,  GX_F32,   0);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

		GX_SetNumTexGens(0);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
	}
}

void Gfx_DrawVb_Lines(int verticesCount) {
}


static void Draw_ColouredTriangles(int verticesCount, int startVertex) {
	GX_Begin(GX_QUADS, GX_VTXFMT0, verticesCount);
	// TODO: Ditch indexed rendering and use GX_QUADS instead ??
	for (int i = 0; i < verticesCount; i++) 
	{
		struct VertexColoured* v = (struct VertexColoured*)gfx_vertices + startVertex + i;
		
		GX_Position3f32(v->X, v->Y, v->Z);
		GX_Color4u8(PackedCol_R(v->Col), PackedCol_G(v->Col), PackedCol_B(v->Col), PackedCol_A(v->Col));
	}
	GX_End();
}

static void Draw_TexturedTriangles(int verticesCount, int startVertex) {
	GX_Begin(GX_QUADS, GX_VTXFMT0, verticesCount);
	for (int i = 0; i < verticesCount; i++) 
	{
		struct VertexTextured* v = (struct VertexTextured*)gfx_vertices + startVertex + i;
		
		GX_Position3f32(v->X, v->Y, v->Z);
		GX_Color4u8(PackedCol_R(v->Col), PackedCol_G(v->Col), PackedCol_B(v->Col), PackedCol_A(v->Col));
		GX_TexCoord2f32(v->U, v->V);
	}
	GX_End();
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		Draw_TexturedTriangles(verticesCount, startVertex);
	} else {
		Draw_ColouredTriangles(verticesCount, startVertex);
	}
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		Draw_TexturedTriangles(verticesCount, 0);
	} else {
		Draw_ColouredTriangles(verticesCount, 0);
	}
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	Draw_TexturedTriangles(verticesCount, startVertex);
}
#endif