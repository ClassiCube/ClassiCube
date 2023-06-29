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

struct _GfxData Gfx;
GfxResourceID Gfx_defaultIb;
GfxResourceID Gfx_quadVb, Gfx_texVb;
const cc_string Gfx_LowPerfMessage = String_FromConst("&eRunning in reduced performance mode (game minimised or hidden)");

static const int strideSizes[2] = { SIZEOF_VERTEX_COLOURED, SIZEOF_VERTEX_TEXTURED };
/* Whether mipmaps must be created for all dimensions down to 1x1 or not */
static cc_bool customMipmapsLevels;

static cc_bool gfx_vsync, gfx_fogEnabled;
static float gfx_minFrameMs;
cc_bool Gfx_GetFog(void) { return gfx_fogEnabled; }

/* Initialises/Restores render state */
CC_NOINLINE static void Gfx_RestoreState(void);
/* Destroys render state, but can be restored later */
CC_NOINLINE static void Gfx_FreeState(void);

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

static void InitDefaultResources(void) {
	Gfx_defaultIb = Gfx_CreateIb2(GFX_MAX_INDICES, MakeIndices, NULL);

	Gfx_RecreateDynamicVb(&Gfx_quadVb, VERTEX_FORMAT_COLOURED, 4);
	Gfx_RecreateDynamicVb(&Gfx_texVb,  VERTEX_FORMAT_TEXTURED, 4);
}

static void FreeDefaultResources(void) {
	Gfx_DeleteDynamicVb(&Gfx_quadVb);
	Gfx_DeleteDynamicVb(&Gfx_texVb);
	Gfx_DeleteIb(&Gfx_defaultIb);
}

#ifdef CC_BUILD_WEB
static void LimitFPS(void) {
	/* Can't use Thread_Sleep on the web. (spinwaits instead of sleeping) */
	/* However this is not a problem, because GLContext_SetVsync */
	/*  makes the web browser manage the frame timing instead */
}
#else
static float gfx_targetTime, gfx_actualTime;

/* Examines difference between expected and actual frame times, */
/*  then sleeps if actual frame time is too fast */
static void LimitFPS(void) {
	cc_uint64 frameEnd, sleepEnd;
	
	frameEnd = Stopwatch_Measure();
	gfx_actualTime += Stopwatch_ElapsedMicroseconds(Game_FrameStart, frameEnd) / 1000.0f;
	gfx_targetTime += gfx_minFrameMs;

	/* going faster than FPS limit - sleep to slow down */
	if (gfx_actualTime < gfx_targetTime) {
		float cooldown = gfx_targetTime - gfx_actualTime;
		Thread_Sleep((int)(cooldown + 0.5f));

		/* also accumulate Thread_Sleep duration, as actual sleep */
		/*  duration can significantly deviate from requested time */ 
		/*  (e.g. requested 4ms, but actually slept for 8ms) */
		sleepEnd = Stopwatch_Measure();
		gfx_actualTime += Stopwatch_ElapsedMicroseconds(frameEnd, sleepEnd) / 1000.0f;
	}

	/* reset accumulated time to avoid excessive FPS drift */
	if (gfx_targetTime >= 1000) { gfx_actualTime = 0; gfx_targetTime = 0; }
}
#endif

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

cc_bool reducedPerformance;
static void TickReducedPerformance(void) {
	Thread_Sleep(100); /* 10 FPS */

	if (reducedPerformance) return;
	reducedPerformance = true;
	Chat_AddOf(&Gfx_LowPerfMessage, MSG_TYPE_EXTRASTATUS_2);
}

static void EndReducedPerformance(void) {
	if (!reducedPerformance) return;
	reducedPerformance = false;
	Chat_AddOf(&String_Empty,       MSG_TYPE_EXTRASTATUS_2);
	Chat_AddRaw(LOWPERF_EXIT_MESSAGE);
}


void Gfx_RecreateDynamicVb(GfxResourceID* vb, VertexFormat fmt, int maxVertices) {
	Gfx_DeleteDynamicVb(vb);
	*vb = Gfx_CreateDynamicVb(fmt, maxVertices);
}

void Gfx_RecreateTexture(GfxResourceID* tex, struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	Gfx_DeleteTexture(tex);
	*tex = Gfx_CreateTexture(bmp, flags, mipmaps);
}

void* Gfx_RecreateAndLockVb(GfxResourceID* vb, VertexFormat fmt, int count) {
	Gfx_DeleteVb(vb);
	*vb = Gfx_CreateVb(fmt, count);
	return Gfx_LockVb(*vb, fmt, count);
}

void Gfx_UpdateDynamicVb_IndexedTris(GfxResourceID vb, void* vertices, int vCount) {
	Gfx_SetDynamicVbData(vb, vertices, vCount);
	Gfx_DrawVb_IndexedTris(vCount);
}

void Gfx_Draw2DFlat(int x, int y, int width, int height, PackedCol color) {
	struct VertexColoured verts[4];
	struct VertexColoured* v = verts;

	v->X = (float)x;           v->Y = (float)y;            v->Z = 0; v->Col = color; v++;
	v->X = (float)(x + width); v->Y = (float)y;            v->Z = 0; v->Col = color; v++;
	v->X = (float)(x + width); v->Y = (float)(y + height); v->Z = 0; v->Col = color; v++;
	v->X = (float)x;           v->Y = (float)(y + height); v->Z = 0; v->Col = color; v++;

	Gfx_SetVertexFormat(VERTEX_FORMAT_COLOURED);
	Gfx_UpdateDynamicVb_IndexedTris(Gfx_quadVb, verts, 4);
}

void Gfx_Draw2DGradient(int x, int y, int width, int height, PackedCol top, PackedCol bottom) {
	struct VertexColoured verts[4];
	struct VertexColoured* v = verts;

	v->X = (float)x;           v->Y = (float)y;            v->Z = 0; v->Col = top; v++;
	v->X = (float)(x + width); v->Y = (float)y;            v->Z = 0; v->Col = top; v++;
	v->X = (float)(x + width); v->Y = (float)(y + height); v->Z = 0; v->Col = bottom; v++;
	v->X = (float)x;           v->Y = (float)(y + height); v->Z = 0; v->Col = bottom; v++;

	Gfx_SetVertexFormat(VERTEX_FORMAT_COLOURED);
	Gfx_UpdateDynamicVb_IndexedTris(Gfx_quadVb, verts, 4);
}

void Gfx_Draw2DTexture(const struct Texture* tex, PackedCol color) {
	struct VertexTextured texVerts[4];
	struct VertexTextured* ptr = texVerts;
	Gfx_Make2DQuad(tex, color, &ptr);
	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_UpdateDynamicVb_IndexedTris(Gfx_texVb, texVerts, 4);
}

void Gfx_Make2DQuad(const struct Texture* tex, PackedCol color, struct VertexTextured** vertices) {
	float x1 = (float)tex->X, x2 = (float)(tex->X + tex->Width);
	float y1 = (float)tex->Y, y2 = (float)(tex->Y + tex->Height);
	struct VertexTextured* v = *vertices;

#ifdef CC_BUILD_D3D9
	/* NOTE: see "https://msdn.microsoft.com/en-us/library/windows/desktop/bb219690(v=vs.85).aspx", */
	/* i.e. the msdn article called "Directly Mapping Texels to Pixels (Direct3D 9)" for why we have to do this. */
	x1 -= 0.5f; x2 -= 0.5f;
	y1 -= 0.5f; y2 -= 0.5f;
#endif

	v->X = x1; v->Y = y1; v->Z = 0; v->Col = color; v->U = tex->uv.U1; v->V = tex->uv.V1; v++;
	v->X = x2; v->Y = y1; v->Z = 0; v->Col = color; v->U = tex->uv.U2; v->V = tex->uv.V1; v++;
	v->X = x2; v->Y = y2; v->Z = 0; v->Col = color; v->U = tex->uv.U2; v->V = tex->uv.V2; v++;
	v->X = x1; v->Y = y2; v->Z = 0; v->Col = color; v->U = tex->uv.U1; v->V = tex->uv.V2; v++;
	*vertices = v;
}

static cc_bool gfx_hadFog;
void Gfx_Begin2D(int width, int height) {
	struct Matrix ortho;
	Gfx_CalcOrthoMatrix(&ortho, (float)width, (float)height, -10000.0f, 10000.0f);
	Gfx_LoadMatrix(MATRIX_PROJECTION, &ortho);
	Gfx_LoadIdentityMatrix(MATRIX_VIEW);

	Gfx_SetDepthTest(false);
	Gfx_SetAlphaBlending(true);
	gfx_hadFog = Gfx_GetFog();
	if (gfx_hadFog) Gfx_SetFog(false);
}

void Gfx_End2D(void) {
	Gfx_SetDepthTest(true);
	Gfx_SetAlphaBlending(false);
	if (gfx_hadFog) Gfx_SetFog(true);
}

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


static float Reversed_CalcZNear(float fov, int depthbufferBits) {
	/* With reversed z depth, near Z plane can be much closer (with sufficient depth buffer precision) */
	/*   This reduces clipping with high FOV without sacrificing depth precision for faraway objects */
	/*   However for low FOV, don't reduce near Z in order to gain a bit more depth precision */
	if (depthbufferBits < 24 || fov <= 70 * MATH_DEG2RAD) return 0.05f;
	if (fov <= 100 * MATH_DEG2RAD) return 0.025f;
	if (fov <= 150 * MATH_DEG2RAD) return 0.0125f;
	return 0.00390625f;
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static void CopyTextureData(void* dst, int dstStride, const struct Bitmap* src, int srcStride) {
	/* We need to copy scanline by scanline, as generally srcStride != dstStride */
	cc_uint8* src_ = (cc_uint8*)src->scan0;
	cc_uint8* dst_ = (cc_uint8*)dst;
	int y;

	for (y = 0; y < src->height; y++) {
		Mem_Copy(dst_, src_, src->width << 2);
		src_ += srcStride;
		dst_ += dstStride;
	}
}

/* Quoted from http://www.realtimerendering.com/blog/gpus-prefer-premultiplication/ */
/* The short version: if you want your renderer to properly handle textures with alphas when using */
/* bilinear interpolation or mipmapping, you need to premultiply your PNG color data by their (unassociated) alphas. */
static BitmapCol AverageCol(BitmapCol p1, BitmapCol p2) {
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
			dst[0] = AverageCol(*src, *(src + srcWidth));

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
			BitmapCol ave0 = AverageCol(src0[srcX], src0[srcX + 1]);
			BitmapCol ave1 = AverageCol(src1[srcX], src1[srcX + 1]);
			dst[x] = AverageCol(ave0, ave1);
		}
		src += (srcWidth << 1);
		dst += width;
	}
}

/* Returns the maximum number of mipmaps levels used for given size. */
static CC_NOINLINE int CalcMipmapsLevels(int width, int height) {
	int lvlsWidth = Math_Log2(width), lvlsHeight = Math_Log2(height);
	if (customMipmapsLevels) {
		int lvls = min(lvlsWidth, lvlsHeight);
		return min(lvls, 4);
	} else {
		return max(lvlsWidth, lvlsHeight);
	}
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
