#include "Graphics.h"
#include "String.h"
#include "Platform.h"
#include "Funcs.h"
#include "Game.h"
#include "ExtMath.h"
#include "Event.h"
#include "Block.h"
#include "Options.h"
#include "Bitmap.h"
#include "Chat.h"
#include "Logger.h"

struct _GfxData Gfx;
static GfxResourceID Gfx_quadVb, Gfx_texVb;
const cc_string Gfx_LowPerfMessage = String_FromConst("&eRunning in reduced performance mode (game minimised or hidden)");

static const int strideSizes[] = { SIZEOF_VERTEX_COLOURED, SIZEOF_VERTEX_TEXTURED };
/* Whether mipmaps must be created for all dimensions down to 1x1 or not */
static cc_bool customMipmapsLevels;
/* Current format and size of vertices */
static int gfx_stride, gfx_format = -1;

static cc_bool gfx_vsync, gfx_fogEnabled;
static cc_bool gfx_rendering2D;


/*########################################################################################################################*
*------------------------------------------------------State changes------------------------------------------------------*
*#########################################################################################################################*/
static cc_bool gfx_colorMask[4] = { true, true, true, true };
cc_bool Gfx_GetFog(void) { return gfx_fogEnabled; }
static cc_bool gfx_alphaTest, gfx_alphaBlend;

static void SetAlphaTest(cc_bool enabled);
void Gfx_SetAlphaTest(cc_bool enabled) {
	if (gfx_alphaTest == enabled) return;
	
	gfx_alphaTest = enabled;
	SetAlphaTest(enabled);
}

static void SetAlphaBlend(cc_bool enabled);
void Gfx_SetAlphaBlending(cc_bool enabled) {
	if (gfx_alphaBlend == enabled) return;
	
	gfx_alphaBlend = enabled;
	SetAlphaBlend(enabled);
}

/* Initialises/Restores render state */
CC_NOINLINE static void Gfx_RestoreState(void);
/* Destroys render state, but can be restored later */
CC_NOINLINE static void Gfx_FreeState(void);

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a);
void Gfx_SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	gfx_colorMask[0] = r;
	gfx_colorMask[1] = g;
	gfx_colorMask[2] = b;
	gfx_colorMask[3] = a;
	SetColorWrite(r, g, b, a);
}

void Gfx_SetTexturing(cc_bool enabled) { } /* useless */

#ifndef CC_BUILD_3DS
void Gfx_Set3DLeft(struct Matrix* proj, struct Matrix* view) {
	struct Matrix proj_left, view_left;

	/* Translation values according to values captured by */
	/*  analysing the OpenGL calls made by classic using gDEbugger */
	/* TODO these still aren't quite right, ghosting occurs */
	Matrix_Translate(&proj_left,   0.07f, 0, 0);
	Matrix_Mul(&Gfx.Projection, proj, &proj_left);
	Matrix_Translate(&view_left,  -0.10f, 0, 0);
	Matrix_Mul(&Gfx.View,       view, &view_left);

	Gfx_SetColorWrite(false, true, true, false);
}

void Gfx_Set3DRight(struct Matrix* proj, struct Matrix* view) {
	struct Matrix proj_right, view_right;

	Matrix_Translate(&proj_right, -0.07f, 0, 0);
	Matrix_Mul(&Gfx.Projection, proj, &proj_right);
	Matrix_Translate(&view_right,  0.10f, 0, 0);
	Matrix_Mul(&Gfx.View,       view, &view_right);

	Gfx_ClearBuffers(GFX_BUFFER_DEPTH);
	Gfx_SetColorWrite(true, false, false, false);
}

void Gfx_End3D(struct Matrix* proj, struct Matrix* view) {
	Gfx.Projection = *proj;

	Gfx_SetColorWrite(true, true, true, true);
}
#endif


/*########################################################################################################################*
*------------------------------------------------------Generic/Common-----------------------------------------------------*
*#########################################################################################################################*/
/* Fills out indices array with {0,1,2} {2,3,0}, {4,5,6} {6,7,4} etc */
static void MakeIndices(cc_uint16* indices, int count, void* obj) {
	int element = 0, i;

	for (i = 0; i < count; i += 6) {
		indices[0] = (cc_uint16)(element + 0);
		indices[1] = (cc_uint16)(element + 1);
		indices[2] = (cc_uint16)(element + 2);

		indices[3] = (cc_uint16)(element + 2);
		indices[4] = (cc_uint16)(element + 3);
		indices[5] = (cc_uint16)(element + 0);

		indices += 6; element += 4;
	}
}

static void RecreateDynamicVb(GfxResourceID* vb, VertexFormat fmt, int maxVertices) {
	Gfx_DeleteDynamicVb(vb);
	*vb = Gfx_CreateDynamicVb(fmt, maxVertices);
}

static void InitDefaultResources(void) {
	Gfx.DefaultIb = Gfx_CreateIb2(GFX_MAX_INDICES, MakeIndices, NULL);

	RecreateDynamicVb(&Gfx_quadVb, VERTEX_FORMAT_COLOURED, 4);
	RecreateDynamicVb(&Gfx_texVb,  VERTEX_FORMAT_TEXTURED, 4);
}

static void FreeDefaultResources(void) {
	Gfx_DeleteDynamicVb(&Gfx_quadVb);
	Gfx_DeleteDynamicVb(&Gfx_texVb);
	Gfx_DeleteIb(&Gfx.DefaultIb);
}


/*########################################################################################################################*
*------------------------------------------------------FPS and context----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_LoseContext(const char* reason) {
	if (Gfx.LostContext) return;
	Gfx.LostContext = true;
	Platform_Log1("Lost graphics context: %c", reason);
	Event_RaiseVoid(&GfxEvents.ContextLost);
}

void Gfx_RecreateContext(void) {
	Gfx.LostContext = false;
	Platform_LogConst("Recreating graphics context");
	Event_RaiseVoid(&GfxEvents.ContextRecreated);
}

static CC_INLINE void TickReducedPerformance(void) {
	Thread_Sleep(100); /* 10 FPS */

	if (Gfx.ReducedPerfMode) return;
	Gfx.ReducedPerfMode = true;
	Chat_AddOf(&Gfx_LowPerfMessage, MSG_TYPE_EXTRASTATUS_2);
}

static CC_INLINE void EndReducedPerformance(void) {
	if (!Gfx.ReducedPerfMode) return;

	Gfx.ReducedPerfModeCooldown = 2;
	Gfx.ReducedPerfMode         = false;
	Chat_AddOf(&String_Empty, MSG_TYPE_EXTRASTATUS_2);
}


/*########################################################################################################################*
*--------------------------------------------------------2D drawing-------------------------------------------------------*
*#########################################################################################################################*/
#ifndef CC_BUILD_3DS
void Gfx_Draw2DFlat(int x, int y, int width, int height, PackedCol color) {
	struct VertexColoured* v;

	Gfx_SetVertexFormat(VERTEX_FORMAT_COLOURED);
	v = (struct VertexColoured*)Gfx_LockDynamicVb(Gfx_quadVb, VERTEX_FORMAT_COLOURED, 4);

	v->x = (float)x;           v->y = (float)y;            v->z = 0; v->Col = color; v++;
	v->x = (float)(x + width); v->y = (float)y;            v->z = 0; v->Col = color; v++;
	v->x = (float)(x + width); v->y = (float)(y + height); v->z = 0; v->Col = color; v++;
	v->x = (float)x;           v->y = (float)(y + height); v->z = 0; v->Col = color; v++;

	Gfx_UnlockDynamicVb(Gfx_quadVb);
	Gfx_DrawVb_IndexedTris(4);
}

void Gfx_Draw2DGradient(int x, int y, int width, int height, PackedCol top, PackedCol bottom) {
	struct VertexColoured* v;

	Gfx_SetVertexFormat(VERTEX_FORMAT_COLOURED);
	v = (struct VertexColoured*)Gfx_LockDynamicVb(Gfx_quadVb, VERTEX_FORMAT_COLOURED, 4);

	v->x = (float)x;           v->y = (float)y;            v->z = 0; v->Col = top; v++;
	v->x = (float)(x + width); v->y = (float)y;            v->z = 0; v->Col = top; v++;
	v->x = (float)(x + width); v->y = (float)(y + height); v->z = 0; v->Col = bottom; v++;
	v->x = (float)x;           v->y = (float)(y + height); v->z = 0; v->Col = bottom; v++;

	Gfx_UnlockDynamicVb(Gfx_quadVb);
	Gfx_DrawVb_IndexedTris(4);
}

void Gfx_Draw2DTexture(const struct Texture* tex, PackedCol color) {
	struct VertexTextured* ptr;

	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	ptr = (struct VertexTextured*)Gfx_LockDynamicVb(Gfx_texVb, VERTEX_FORMAT_TEXTURED, 4);

	Gfx_Make2DQuad(tex, color, &ptr);

	Gfx_UnlockDynamicVb(Gfx_texVb);
	Gfx_DrawVb_IndexedTris(4);
}
#endif

void Gfx_Make2DQuad(const struct Texture* tex, PackedCol color, struct VertexTextured** vertices) {
	float x1 = (float)tex->x, x2 = (float)(tex->x + tex->width);
	float y1 = (float)tex->y, y2 = (float)(tex->y + tex->height);
	struct VertexTextured* v = *vertices;

#if CC_GFX_BACKEND == CC_GFX_BACKEND_D3D9
	/* NOTE: see "https://msdn.microsoft.com/en-us/library/windows/desktop/bb219690(v=vs.85).aspx", */
	/* i.e. the msdn article called "Directly Mapping Texels to Pixels (Direct3D 9)" for why we have to do this. */
	x1 -= 0.5f; x2 -= 0.5f;
	y1 -= 0.5f; y2 -= 0.5f;
#endif

	v->x = x1; v->y = y1; v->z = 0; v->Col = color; v->U = tex->uv.u1; v->V = tex->uv.v1; v++;
	v->x = x2; v->y = y1; v->z = 0; v->Col = color; v->U = tex->uv.u2; v->V = tex->uv.v1; v++;
	v->x = x2; v->y = y2; v->z = 0; v->Col = color; v->U = tex->uv.u2; v->V = tex->uv.v2; v++;
	v->x = x1; v->y = y2; v->z = 0; v->Col = color; v->U = tex->uv.u1; v->V = tex->uv.v2; v++;
	*vertices = v;
}

#ifndef CC_BUILD_PS1
static cc_bool gfx_hadFog;
void Gfx_Begin2D(int width, int height) {
	struct Matrix ortho;
	/* intentionally biased more towards positive Z to reduce 2D clipping issues on the DS */
	Gfx_CalcOrthoMatrix(&ortho, (float)width, (float)height, -100.0f, 1000.0f);
	Gfx_LoadMatrix(MATRIX_PROJ, &ortho);
	Gfx_LoadMatrix(MATRIX_VIEW, &Matrix_Identity);

	Gfx_SetDepthTest(false);
	Gfx_SetDepthWrite(false);
	Gfx_SetAlphaBlending(true);
	
	gfx_hadFog = Gfx_GetFog();
	if (gfx_hadFog) Gfx_SetFog(false);
	gfx_rendering2D = true;
}

void Gfx_End2D(void) {
	Gfx_SetDepthTest(true);
	Gfx_SetDepthWrite(true);
	Gfx_SetAlphaBlending(false);
	
	if (gfx_hadFog) Gfx_SetFog(true);
	gfx_rendering2D = false;
}
#endif


/*########################################################################################################################*
*--------------------------------------------------------Misc/Utils-------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetupAlphaState(cc_uint8 draw) {
	if (draw == DRAW_TRANSLUCENT)       Gfx_SetAlphaBlending(true);
	if (draw == DRAW_TRANSPARENT)       Gfx_SetAlphaTest(true);
	if (draw == DRAW_TRANSPARENT_THICK) Gfx_SetAlphaTest(true);
	if (draw == DRAW_SPRITE)            Gfx_SetAlphaTest(true);
}

void Gfx_RestoreAlphaState(cc_uint8 draw) {
	if (draw == DRAW_TRANSLUCENT)       Gfx_SetAlphaBlending(false);
	if (draw == DRAW_TRANSPARENT)       Gfx_SetAlphaTest(false);
	if (draw == DRAW_TRANSPARENT_THICK) Gfx_SetAlphaTest(false);
	if (draw == DRAW_SPRITE)            Gfx_SetAlphaTest(false);
}

static CC_INLINE float Reversed_CalcZNear(float fov, int depthbufferBits) {
	/* With reversed z depth, near Z plane can be much closer (with sufficient depth buffer precision) */
	/*   This reduces clipping with high FOV without sacrificing depth precision for faraway objects */
	/*   However for low FOV, don't reduce near Z in order to gain a bit more depth precision */
	if (depthbufferBits < 24 || fov <= 70 * MATH_DEG2RAD) return 0.05f;
	if (fov <= 100 * MATH_DEG2RAD) return 0.025f;
	if (fov <= 150 * MATH_DEG2RAD) return 0.0125f;
	return 0.00390625f;
}

static void PrintMaxTextureInfo(cc_string* info) {
	if (Gfx.MaxTexSize) {
		float maxSize = Gfx.MaxTexSize / (1024.0f * 1024.0f);
		String_Format3(info, "Max texture size: (%i, %i), up to %f3 MB\n", 
						&Gfx.MaxTexWidth, &Gfx.MaxTexHeight, &maxSize);
	} else {
		String_Format2(info, "Max texture size: (%i, %i)\n",
						&Gfx.MaxTexWidth, &Gfx.MaxTexHeight);
	}
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_RecreateTexture(GfxResourceID* tex, struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	Gfx_DeleteTexture(tex);
	*tex = Gfx_CreateTexture(bmp, flags, mipmaps);
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
	Gfx_UpdateTexture(texId, x, y, part, part->width, mipmaps);
}

static CC_INLINE void CopyTextureData(void* dst, int dstStride, const struct Bitmap* src, int srcStride) {
	cc_uint8* src_ = (cc_uint8*)src->scan0;
	cc_uint8* dst_ = (cc_uint8*)dst;
	int y;

	if (srcStride == dstStride) {
		Mem_Copy(dst_, src_, Bitmap_DataSize(src->width, src->height));
	} else {
		/* Have to copy scanline by scanline */
		for (y = 0; y < src->height; y++) {
			Mem_Copy(dst_, src_, src->width * BITMAPCOLOR_SIZE);
			src_ += srcStride;
			dst_ += dstStride;
		}
	}
}

/* Quoted from http://www.realtimerendering.com/blog/gpus-prefer-premultiplication/ */
/* The short version: if you want your renderer to properly handle textures with alphas when using */
/* bilinear interpolation or mipmapping, you need to premultiply your PNG color data by their (unassociated) alphas. */
static BitmapCol AverageColor(BitmapCol p1, BitmapCol p2) {
	cc_uint32 a1, a2, aSum;
	cc_uint32 b1, g1, r1;
	cc_uint32 b2, g2, r2;

	a1 = BitmapCol_A(p1); a2 = BitmapCol_A(p2);
	aSum = (a1 + a2);
	aSum = aSum > 0 ? aSum : 1; /* avoid divide by 0 below */

	/* Convert RGB to pre-multiplied form */
	/* TODO: Don't shift when multiplying/averaging */
	r1 = BitmapCol_R(p1) * a1; g1 = BitmapCol_G(p1) * a1; b1 = BitmapCol_B(p1) * a1;
	r2 = BitmapCol_R(p2) * a2; g2 = BitmapCol_G(p2) * a2; b2 = BitmapCol_B(p2) * a2;

	/* https://stackoverflow.com/a/347376 */
	/* We need to convert RGB back from the pre-multiplied average into normal form */
	/* ((r1 + r2) / 2) / ((a1 + a2) / 2) */
	/* but we just cancel out the / 2 */
	return BitmapCol_Make(
		(r1 + r2) / aSum, 
		(g1 + g2) / aSum,
		(b1 + b2) / aSum, 
		aSum >> 1);
}

/* Generates the next mipmaps level bitmap by downsampling from the given bitmap. */
static void GenMipmaps(int width, int height, BitmapCol* dst, BitmapCol* src, int srcWidth) {
	int x, y;
	/* Downsampling from a 1 pixel wide bitmap requires simpler filtering */
	if (srcWidth == 1) {
		for (y = 0; y < height; y++) {
			/* 1x2 bilinear filter */
			dst[0] = AverageColor(*src, *(src + srcWidth));

			dst += width;
			src += (srcWidth << 1);
		}
		return;
	}

	for (y = 0; y < height; y++) {
		BitmapCol* src0 = src;
		BitmapCol* src1 = src + srcWidth;

		for (x = 0; x < width; x++) {
			int srcX = (x << 1);
			/* 2x2 bilinear filter */
			BitmapCol ave0 = AverageColor(src0[srcX], src0[srcX + 1]);
			BitmapCol ave1 = AverageColor(src1[srcX], src1[srcX + 1]);
			dst[x] = AverageColor(ave0, ave1);
		}
		src += (srcWidth << 1);
		dst += width;
	}
}

/* Returns the maximum number of mipmaps levels used for given size. */
static CC_NOINLINE int CalcMipmapsLevels(int width, int height) {
	int lvlsWidth = Math_ilog2(width), lvlsHeight = Math_ilog2(height);
	if (customMipmapsLevels) {
		int lvls = min(lvlsWidth, lvlsHeight);
		return min(lvls, 4);
	} else {
		return max(lvlsWidth, lvlsHeight);
	}
}

cc_bool Gfx_CheckTextureSize(int width, int height, cc_uint8 flags) {
	int maxSize;
	if (width  > Gfx.MaxTexWidth)  return false;
	if (height > Gfx.MaxTexHeight) return false;
	
	if (Gfx.MinTexWidth  && width  < Gfx.MinTexWidth)  return false;
	if (Gfx.MinTexHeight && height < Gfx.MinTexHeight) return false;
	
	maxSize = Gfx.MaxTexSize;
	// low resolution textures may support higher sizes (e.g. Nintendo 64)
	if ((flags & TEXTURE_FLAG_LOWRES) && Gfx.MaxLowResTexSize)
		maxSize = Gfx.MaxLowResTexSize;

	return maxSize == 0 || (width * height <= maxSize);
}

static GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps);

GfxResourceID Gfx_CreateTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	return Gfx_CreateTexture2(bmp, bmp->width, flags, mipmaps);
}

GfxResourceID Gfx_CreateTexture2(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	if (Gfx.SupportsNonPowTwoTextures && (flags & TEXTURE_FLAG_NONPOW2)) {
		/* Texture is being deliberately created and can be successfully created */
		/* with non power of two dimensions. Typically used for UI textures */
	} else if (!Math_IsPowOf2(bmp->width) || !Math_IsPowOf2(bmp->height)) {
		Logger_Abort("Textures must have power of two dimensions");
	}

	if (Gfx.LostContext) return 0;
	if (!Gfx_CheckTextureSize(bmp->width, bmp->height, flags)) return 0;

	return Gfx_AllocTexture(bmp, rowWidth, flags, mipmaps);
}

void Texture_Render(const struct Texture* tex) {
	Gfx_BindTexture(tex->ID);
	Gfx_Draw2DTexture(tex, PACKEDCOL_WHITE);
}

void Texture_RenderShaded(const struct Texture* tex, PackedCol shadeColor) {
	Gfx_BindTexture(tex->ID);
	Gfx_Draw2DTexture(tex, shadeColor);
}


/*########################################################################################################################*
*------------------------------------------------------Vertex buffers-----------------------------------------------------*
*#########################################################################################################################*/
void* Gfx_RecreateAndLockVb(GfxResourceID* vb, VertexFormat fmt, int count) {
	Gfx_DeleteVb(vb);
	*vb = Gfx_CreateVb(fmt, count);
	return Gfx_LockVb(*vb, fmt, count);
}

static GfxResourceID Gfx_AllocStaticVb( VertexFormat fmt, int count);
static GfxResourceID Gfx_AllocDynamicVb(VertexFormat fmt, int maxVertices);

GfxResourceID Gfx_CreateVb(VertexFormat fmt, int count) {
	GfxResourceID vb;
	/* if (Gfx.LostContext) return 0; TODO check this ???? probably breaks things */

	for (;;)
	{
		if ((vb = Gfx_AllocStaticVb(fmt, count))) return vb;

		if (!Game_ReduceVRAM()) Logger_Abort("Out of video memory! (allocating static VB)");
	}
}

GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices) {
	GfxResourceID vb;
	if (Gfx.LostContext) return 0; 

	for (;;)
	{
		if ((vb = Gfx_AllocDynamicVb(fmt, maxVertices))) return vb;

		if (!Game_ReduceVRAM()) Logger_Abort("Out of video memory! (allocating dynamic VB)");
	}
}

#if CC_GFX_BACKEND_IS_GL() || (CC_GFX_BACKEND == CC_GFX_BACKEND_D3D9)
/* Slightly more efficient implementations are defined in the backends */
#else
void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	void* data = Gfx_LockDynamicVb(vb, gfx_format, vCount);
	Mem_Copy(data, vertices, vCount * gfx_stride);
	Gfx_UnlockDynamicVb(vb);
}
#endif


/*########################################################################################################################*
*----------------------------------------------------Graphics component---------------------------------------------------*
*#########################################################################################################################*/
static void OnContextLost(void* obj)      { Gfx_FreeState(); }
static void OnContextRecreated(void* obj) { Gfx_RestoreState(); }

static void OnInit(void) {
	Event_Register_(&GfxEvents.ContextLost,      NULL, OnContextLost);
	Event_Register_(&GfxEvents.ContextRecreated, NULL, OnContextRecreated);

	Gfx.Mipmaps = Options_GetBool(OPT_MIPMAPS, false);
	if (Gfx.LostContext) return;
	OnContextRecreated(NULL);
}

struct IGameComponent Gfx_Component = {
	OnInit /* Init */
	/* Can't use OnFree because then Gfx would wrongly be the */
	/* first component freed, even though it MUST be the last */
	/* Instead, Game.c calls Gfx_Free after first freeing all */
	/* the other game components. */
};
