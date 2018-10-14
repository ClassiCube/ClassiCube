#include "GraphicsCommon.h"
#include "Graphics.h"
#include "Platform.h"
#include "Block.h"
#include "Event.h"
#include "Funcs.h"
#include "ExtMath.h"

char Gfx_ApiBuffer[7][STRING_SIZE];
String Gfx_ApiInfo[7] = {
	String_FromArray(Gfx_ApiBuffer[0]), String_FromArray(Gfx_ApiBuffer[1]),
	String_FromArray(Gfx_ApiBuffer[2]), String_FromArray(Gfx_ApiBuffer[3]),
	String_FromArray(Gfx_ApiBuffer[4]), String_FromArray(Gfx_ApiBuffer[5]),
	String_FromArray(Gfx_ApiBuffer[6]),
};

void GfxCommon_Init(void) {
	GfxCommon_quadVb = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FC4B, 4);
	GfxCommon_texVb  = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FT2FC4B, 4);

	uint16_t indices[GFX_MAX_INDICES];
	GfxCommon_MakeIndices(indices, GFX_MAX_INDICES);
	GfxCommon_defaultIb = Gfx_CreateIb(indices, GFX_MAX_INDICES);
}

void GfxCommon_Free(void) {
	Gfx_DeleteVb(&GfxCommon_quadVb);
	Gfx_DeleteVb(&GfxCommon_texVb);
	Gfx_DeleteIb(&GfxCommon_defaultIb);
}

void GfxCommon_LoseContext(const char* reason) {
	Gfx_LostContext = true;
	Platform_Log1("Lost graphics context: %c", reason);

	Event_RaiseVoid(&GfxEvents_ContextLost);
	GfxCommon_Free();
}

void GfxCommon_RecreateContext(void) {
	Gfx_LostContext = false;
	Platform_LogConst("Recreating graphics context");

	Event_RaiseVoid(&GfxEvents_ContextRecreated);
	GfxCommon_Init();
}


void GfxCommon_UpdateDynamicVb_Lines(GfxResourceID vb, void* vertices, int vCount) {
	Gfx_SetDynamicVbData(vb, vertices, vCount);
	Gfx_DrawVb_Lines(vCount);
}

void GfxCommon_UpdateDynamicVb_IndexedTris(GfxResourceID vb, void* vertices, int vCount) {
	Gfx_SetDynamicVbData(vb, vertices, vCount);
	Gfx_DrawVb_IndexedTris(vCount);
}

void GfxCommon_Draw2DFlat(int x, int y, int width, int height, PackedCol col) {
	VertexP3fC4b verts[4];
	VertexP3fC4b v; v.Z = 0.0f; v.Col = col;

	v.X = (float)x;           v.Y = (float)y;            verts[0] = v;
	v.X = (float)(x + width);                            verts[1] = v;
	                          v.Y = (float)(y + height); verts[2] = v;
	v.X = (float)x;                                      verts[3] = v;

	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FC4B);
	GfxCommon_UpdateDynamicVb_IndexedTris(GfxCommon_quadVb, verts, 4);
}

void GfxCommon_Draw2DGradient(int x, int y, int width, int height, PackedCol top, PackedCol bottom) {
	VertexP3fC4b verts[4];
	VertexP3fC4b v; v.Z = 0.0f;

	v.X = (float)x;           v.Y = (float)y;            v.Col = top;    verts[0] = v;
	v.X = (float)(x + width);                                            verts[1] = v;
	                          v.Y = (float)(y + height); v.Col = bottom; verts[2] = v;
	v.X = (float)x;                                                      verts[3] = v;

	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FC4B);
	GfxCommon_UpdateDynamicVb_IndexedTris(GfxCommon_quadVb, verts, 4);
}

void GfxCommon_Draw2DTexture(const struct Texture* tex, PackedCol col) {
	VertexP3fT2fC4b texVerts[4];
	VertexP3fT2fC4b* ptr = texVerts;
	GfxCommon_Make2DQuad(tex, col, &ptr);
	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	GfxCommon_UpdateDynamicVb_IndexedTris(GfxCommon_texVb, texVerts, 4);
}

void GfxCommon_Make2DQuad(const struct Texture* tex, PackedCol col, VertexP3fT2fC4b** vertices) {
	float x1 = (float)tex->X, x2 = (float)(tex->X + tex->Width);
	float y1 = (float)tex->Y, y2 = (float)(tex->Y + tex->Height);
#ifdef CC_BUILD_D3D9
	/* NOTE: see "https://msdn.microsoft.com/en-us/library/windows/desktop/bb219690(v=vs.85).aspx", */
	/* i.e. the msdn article called "Directly Mapping Texels to Pixels (Direct3D 9)" for why we have to do this. */
	x1 -= 0.5f; x2 -= 0.5f;
	y1 -= 0.5f; y2 -= 0.5f;
#endif

	VertexP3fT2fC4b* ptr = *vertices;
	VertexP3fT2fC4b v; v.Z = 0.0f; v.Col = col;
	v.X = x1; v.Y = y1; v.U = tex->U1; v.V = tex->V1; ptr[0] = v;
	v.X = x2;           v.U = tex->U2;                ptr[1] = v;
	v.Y = y2;                          v.V = tex->V2; ptr[2] = v;
	v.X = x1;           v.U = tex->U1;                ptr[3] = v;
	*vertices += 4;
}

bool gfx_hadFog;
void GfxCommon_Mode2D(int width, int height) {
	Gfx_SetMatrixMode(MATRIX_TYPE_PROJECTION);
	struct Matrix ortho;
	Gfx_CalcOrthoMatrix((float)width, (float)height, &ortho);
	Gfx_LoadMatrix(&ortho);
	Gfx_SetMatrixMode(MATRIX_TYPE_VIEW);
	Gfx_LoadIdentityMatrix();

	Gfx_SetDepthTest(false);
	Gfx_SetAlphaBlending(true);
	gfx_hadFog = Gfx_GetFog();
	if (gfx_hadFog) Gfx_SetFog(false);
}

void GfxCommon_Mode3D(void) {
	Gfx_SetMatrixMode(MATRIX_TYPE_PROJECTION);
	Gfx_LoadMatrix(&Gfx_Projection);
	Gfx_SetMatrixMode(MATRIX_TYPE_VIEW);
	Gfx_LoadMatrix(&Gfx_View);

	Gfx_SetDepthTest(true);
	Gfx_SetAlphaBlending(false);
	if (gfx_hadFog) Gfx_SetFog(true);
}

void GfxCommon_MakeIndices(uint16_t* indices, int iCount) {
	int element = 0, i;

	for (i = 0; i < iCount; i += 6) {
		indices[0] = (uint16_t)(element + 0);
		indices[1] = (uint16_t)(element + 1);
		indices[2] = (uint16_t)(element + 2);

		indices[3] = (uint16_t)(element + 2);
		indices[4] = (uint16_t)(element + 3);
		indices[5] = (uint16_t)(element + 0);

		indices += 6; element += 4;
	}
}

void GfxCommon_SetupAlphaState(uint8_t draw) {
	if (draw == DRAW_TRANSLUCENT)       Gfx_SetAlphaBlending(true);
	if (draw == DRAW_TRANSPARENT)       Gfx_SetAlphaTest(true);
	if (draw == DRAW_TRANSPARENT_THICK) Gfx_SetAlphaTest(true);
	if (draw == DRAW_SPRITE)            Gfx_SetAlphaTest(true);
}

void GfxCommon_RestoreAlphaState(uint8_t draw) {
	if (draw == DRAW_TRANSLUCENT)       Gfx_SetAlphaBlending(false);
	if (draw == DRAW_TRANSPARENT)       Gfx_SetAlphaTest(false);
	if (draw == DRAW_TRANSPARENT_THICK) Gfx_SetAlphaTest(false);
	if (draw == DRAW_SPRITE)            Gfx_SetAlphaTest(false);
}


#define alphaMask ((uint32_t)0xFF000000UL)
/* Quoted from http://www.realtimerendering.com/blog/gpus-prefer-premultiplication/
   The short version: if you want your renderer to properly handle textures with alphas when using
   bilinear interpolation or mipmapping, you need to premultiply your PNG color data by their (unassociated) alphas. */
static uint32_t GfxCommon_Average(uint32_t p1, uint32_t p2) {
	uint32_t a1 = ((p1 & alphaMask) >> 24) & 0xFF;
	uint32_t a2 = ((p2 & alphaMask) >> 24) & 0xFF;
	uint32_t aSum = (a1 + a2);
	aSum = aSum > 0 ? aSum : 1; /* avoid divide by 0 below */

	/* Convert RGB to pre-multiplied form */
	uint32_t r1 = ((p1 >> 16) & 0xFF) * a1, g1 = ((p1 >> 8) & 0xFF) * a1, b1 = (p1 & 0xFF) * a1;
	uint32_t r2 = ((p2 >> 16) & 0xFF) * a2, g2 = ((p2 >> 8) & 0xFF) * a2, b2 = (p2 & 0xFF) * a2;

	/* https://stackoverflow.com/a/347376
	   We need to convert RGB back from the pre-multiplied average into normal form
	   ((r1 + r2) / 2) / ((a1 + a2) / 2)
	   but we just cancel out the / 2*/
	uint32_t aAve = aSum >> 1;
	uint32_t rAve = (r1 + r2) / aSum;
	uint32_t gAve = (g1 + g2) / aSum;
	uint32_t bAve = (b1 + b2) / aSum;

	return (aAve << 24) | (rAve << 16) | (gAve << 8) | bAve;
}

void GfxCommon_GenMipmaps(int width, int height, uint8_t* lvlScan0, uint8_t* scan0) {
	uint32_t* baseSrc = (uint32_t*)scan0;
	uint32_t* baseDst = (uint32_t*)lvlScan0;
	int srcWidth = width << 1;

	int x, y;
	for (y = 0; y < height; y++) {
		int srcY = (y << 1);
		uint32_t* src0 = baseSrc + srcY * srcWidth;
		uint32_t* src1 = src0 + srcWidth;
		uint32_t* dst = baseDst + y * width;

		for (x = 0; x < width; x++) {
			int srcX = (x << 1);
			uint32_t src00 = src0[srcX], src01 = src0[srcX + 1];
			uint32_t src10 = src1[srcX], src11 = src1[srcX + 1];

			/* bilinear filter this mipmap */
			uint32_t ave0 = GfxCommon_Average(src00, src01);
			uint32_t ave1 = GfxCommon_Average(src10, src11);
			dst[x] = GfxCommon_Average(ave0, ave1);
		}
	}
}

int GfxCommon_MipmapsLevels(int width, int height) {
	int lvlsWidth = Math_Log2(width), lvlsHeight = Math_Log2(height);
	if (Gfx_CustomMipmapsLevels) {
		int lvls = min(lvlsWidth, lvlsHeight);
		return min(lvls, 4);
	} else {
		return max(lvlsWidth, lvlsHeight);
	}
}

void Texture_Render(const struct Texture* tex) {
	Gfx_BindTexture(tex->ID);
	PackedCol white = PACKEDCOL_WHITE;
	GfxCommon_Draw2DTexture(tex, white);
}

void Texture_RenderShaded(const struct Texture* tex, PackedCol shadeCol) {
	Gfx_BindTexture(tex->ID);
	GfxCommon_Draw2DTexture(tex, shadeCol);
}
