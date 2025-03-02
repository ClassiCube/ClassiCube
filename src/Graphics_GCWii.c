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
static GXTexRegionCallback regionCB;
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
	Gfx_SetViewport(0, 0, mode->fbWidth, mode->efbHeight);
	Gfx_SetScissor( 0, 0, mode->fbWidth, mode->efbHeight);
	
	GX_SetDispCopyYScale((f32)mode->xfbHeight / (f32)mode->efbHeight);
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

	regionCB = GX_SetTexRegionCallback(NULL);
	GX_SetTexRegionCallback(regionCB);
}

void Gfx_Create(void) {
	if (!Gfx.Created) InitGX();
	
	Gfx.MaxTexWidth  = 1024;
	Gfx.MaxTexHeight = 1024;
	Gfx.MaxTexSize   = 512 * 512;
	
	Gfx.MinTexWidth  = 4;
	Gfx.MinTexHeight = 4;
	Gfx.Created      = true;
	gfx_vsync        = true;
	
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

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	int size = bmp->width * bmp->height * 4;
	CCTexture* tex = (CCTexture*)memalign(32, 32 + size);
	if (!tex) return NULL;
	
	GX_InitTexObj(&tex->obj, tex->pixels, bmp->width, bmp->height,
			GX_TF_RGBA8, GX_REPEAT, GX_REPEAT, GX_FALSE);
	GX_InitTexObjFilterMode(&tex->obj, GX_NEAR, GX_NEAR);
			
	ReorderPixels(tex, bmp, 0, 0, rowWidth);
	DCFlushRange(tex->pixels, size);
	return tex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	CCTexture* tex = (CCTexture*)texId;
	// TODO: wrong behaviour if x/y/part isn't multiple of 4 pixels
	ReorderPixels(tex, part, x, y, rowWidth);
	GX_InvalidateTexAll();
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

	GXTexRegion* reg = regionCB(&tex->obj, GX_TEXMAP0);
	GX_LoadTexObjPreloaded(&tex->obj, reg, GX_TEXMAP0);
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static GXColor gfx_clearColor = { 0, 0, 0, 255 };

void Gfx_SetFaceCulling(cc_bool enabled) { 
	// NOTE: seems like ClassiCube's triangle ordering is opposite of what GX considers front facing
	GX_SetCullMode(enabled ? GX_CULL_FRONT : GX_CULL_NONE);
}

static void SetAlphaBlend(cc_bool enabled) {
	if (enabled) {
		GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	} else {
		GX_SetBlendMode(GX_BM_NONE,  GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
	}
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { 
}

void Gfx_ClearColor(PackedCol color) {
	gfx_clearColor.r = PackedCol_R(color);
	gfx_clearColor.g = PackedCol_G(color);
	gfx_clearColor.b = PackedCol_B(color);
	
	GX_SetCopyClear(gfx_clearColor, 0x00ffffff); // TODO: use GX_MAX_Z24 
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	// TODO
}

static cc_bool depth_write = true, depth_test = true;
static void UpdateDepthState(void) {
	GX_SetZMode(depth_test, GX_LEQUAL, depth_write);
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
static BitmapCol* GCWii_GetRow(struct Bitmap* bmp, int y, void* ctx) {
	u8* buffer = (u8*)ctx;
	u8 a, r, g, b;
	int blockYStride = 4 * (bmp->width * 4); // tile row stride = 4 * row stride
	int blockXStride = (4 * 4) * 4; // 16 pixels per tile

	// Do the inverse of converting from 4x4 tiled to linear
	for (u32 x = 0; x < bmp->width; x++){
		int tileY = y >> 2, tileX = x >> 2;
		int locY  = y & 0x3, locX = x & 0x3;
		int idx   = (tileY * blockYStride) + (tileX * blockXStride) + ((locY << 2) + locX) * 2; 

		// All 16 pixels are stored with AR first, then GB
		//a = buffer[idx     ];
		r = buffer[idx +  1];
		g = buffer[idx + 32]; 
		b = buffer[idx + 33];

		bmp->scan0[x] = BitmapColor_RGB(r, g, b);
	}
	return bmp->scan0;
}

cc_result Gfx_TakeScreenshot(struct Stream* output) {
	BitmapCol tmp[1024];
	GXRModeObj* vmode = VIDEO_GetPreferredMode(NULL);
	int width  = vmode->fbWidth;
	int height = vmode->efbHeight;

	u8* buffer = memalign(32, width * height * 4);
	if (!buffer) return ERR_OUT_OF_MEMORY;

	GX_SetTexCopySrc(0, 0, width, height);
	GX_SetTexCopyDst(width, height, GX_TF_RGBA8, GX_FALSE);
	GX_CopyTex(buffer, GX_FALSE);
	GX_PixModeSync();
	GX_Flush();
	DCFlushRange(buffer, width * height * 4);

	struct Bitmap bmp;
	bmp.scan0  = tmp;
	bmp.width  = width; 
	bmp.height = height;

	cc_result res = Png_Encode(&bmp, output, GCWii_GetRow, false, buffer);
	free(buffer);
	return res;
}

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using GC/Wii --\n");
	PrintMaxTextureInfo(info);
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
}

void Gfx_BeginFrame(void) {
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
	// TODO clear only some buffers
}

void Gfx_EndFrame(void) {
	curFB ^= 1; // swap "front" and "back" buffers
	GX_CopyDisp(xfbs[curFB], GX_TRUE);
	GX_DrawDone();
	
	VIDEO_SetNextFramebuffer(xfbs[curFB]);
	VIDEO_Flush();
	
	if (gfx_vsync) VIDEO_WaitVSync();
}

void Gfx_OnWindowResize(void) { }

void Gfx_SetViewport(int x, int y, int w, int h) {
	GX_SetViewport(x, y, w, h, 0, 1);
}

void Gfx_SetScissor(int x, int y, int w, int h) {
	GX_SetScissor(x, y, w, h);
}

cc_bool Gfx_WarnIfNecessary(void) { return false; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }


/*########################################################################################################################*
*-------------------------------------------------------Index Buffers-----------------------------------------------------*
*#########################################################################################################################*/
//static cc_uint16 __attribute__((aligned(16))) gfx_indices[GFX_MAX_INDICES];

GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	//fillFunc(gfx_indices, count, obj);
	// not used since render using GX_QUADS anyways
	return (void*)1;
}

void Gfx_BindIb(GfxResourceID ib) { }
void Gfx_DeleteIb(GfxResourceID* ib) { }


/*########################################################################################################################*
*------------------------------------------------------Vertex Buffers-----------------------------------------------------*
*#########################################################################################################################*/
static cc_uint8* gfx_vertices;
static int vb_size;

static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	return memalign(16, count * strideSizes[fmt]);
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


static GfxResourceID Gfx_AllocDynamicVb(VertexFormat fmt, int maxVertices) {
	return memalign(16, maxVertices * strideSizes[fmt]);
}

void Gfx_BindDynamicVb(GfxResourceID vb) { Gfx_BindVb(vb); }

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	vb_size = count * strideSizes[fmt];
	return vb; 
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) { 
	gfx_vertices = vb;
	DCFlushRange(vb, vb_size);
}

void Gfx_DeleteDynamicVb(GfxResourceID* vb) { Gfx_DeleteVb(vb); }


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gfx_fogColor;
static float gfx_fogEnd = -1.0f, gfx_fogDensity = -1.0f;
static int gfx_fogMode  = -1;

static void UpdateFog(void) {
	float beg = 0.0f, end = 0.0f;
	float near = 0.1f, far = Game_ViewDistance;
	int mode = GX_FOG_NONE;

	GXColor color;
	color.r = PackedCol_R(gfx_fogColor);
	color.g = PackedCol_G(gfx_fogColor);
	color.b = PackedCol_B(gfx_fogColor);
	color.a = PackedCol_A(gfx_fogColor);

	// Fog end values based off https://github.com/devkitPro/opengx/blob/master/src/gc_gl.c#L1770
	if (!gfx_fogEnabled) {
		near = 0.0f;
		far  = 0.0f;
	} else if (gfx_fogMode == FOG_LINEAR) {
		mode = GX_FOG_LIN;
		end  = gfx_fogEnd;
	} else if (gfx_fogMode == FOG_EXP) {
		mode = GX_FOG_EXP;
		beg  = near;
		end  = 5.0f / gfx_fogDensity;
	} else if (gfx_fogMode == FOG_EXP2) {
		mode = GX_FOG_EXP2;
		beg  = near;
		end  = 2.0f / gfx_fogDensity;
	}
    GX_SetFog(mode, beg, end, near, far, color);
}

void Gfx_SetFog(cc_bool enabled) {
	gfx_fogEnabled = enabled;
	UpdateFog();
}

void Gfx_SetFogCol(PackedCol color) {
	if (color == gfx_fogColor) return;
	gfx_fogColor = color;
	UpdateFog();
}

void Gfx_SetFogDensity(float value) {
	if (value == gfx_fogDensity) return;
	gfx_fogDensity = value;
	UpdateFog();
}

void Gfx_SetFogEnd(float value) {
	if (value == gfx_fogEnd) return;
	gfx_fogEnd = value;
	UpdateFog();
}

void Gfx_SetFogMode(FogFunc func) {
	if (func == gfx_fogMode) return;
	gfx_fogMode = func;
	UpdateFog();
}

static void SetAlphaTest(cc_bool enabled) {
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
	
	matrix->row1.x =  2.0f / width;
	matrix->row2.y = -2.0f / height;
	matrix->row3.z = -1.0f / (zFar - zNear);

	matrix->row4.x = -1.0f;
	matrix->row4.y =  1.0f;
	matrix->row4.z = -zFar / (zFar - zNear);
}

static float Cotangent(float x) { return Math_CosF(x) / Math_SinF(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	float zNear = 0.1f;
	float c = Cotangent(0.5f * fov);
	
	// Transposed, source guPersepctive https://github.com/devkitPro/libogc/blob/master/libogc/gu.c
	*matrix = Matrix_Identity;
	matrix->row1.x = c / aspect;
	matrix->row2.y = c;
	matrix->row3.z = -(zNear)        / (zFar - zNear);
	matrix->row3.w = -1.0f;
	matrix->row4.z = -(zFar * zNear) / (zFar - zNear);
	matrix->row4.w =  0.0f;
}

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	const float* m = (const float*)matrix;
	Mtx44 dst;
	float* tmp = (float*)dst;
	
	// Transpose matrix
	for (int i = 0; i < 4; i++)
	{
		tmp[i * 4 + 0] = m[0  + i];
		tmp[i * 4 + 1] = m[4  + i];
		tmp[i * 4 + 2] = m[8  + i];
		tmp[i * 4 + 3] = m[12 + i];
	}
		
	if (type == MATRIX_PROJ) {
		GX_LoadProjectionMtx(dst,
			tmp[3*4+3] == 0.0f ? GX_PERSPECTIVE : GX_ORTHOGRAPHIC);
	} else {
		GX_LoadPosMtxImm(dst, GX_PNMTX0);
	}
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	Gfx_LoadMatrix(MATRIX_VIEW, view);
	Gfx_LoadMatrix(MATRIX_PROJ, proj);
	Matrix_Mul(mvp, view, proj);
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
		
		GX_Position3f32(v->x, v->y, v->z);
		GX_Color1u32(v->Col);
	}
	GX_End();
}

static void Draw_TexturedTriangles(int verticesCount, int startVertex) {
	GX_Begin(GX_QUADS, GX_VTXFMT0, verticesCount);
	for (int i = 0; i < verticesCount; i++) 
	{
		struct VertexTextured* v = (struct VertexTextured*)gfx_vertices + startVertex + i;
		
		GX_Position3f32(v->x, v->y, v->z);
		GX_Color1u32(v->Col);
		GX_TexCoord2f32(v->U, v->V);
	}
	GX_End();
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
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
