#include "GraphicsCommon.h"
#include "GraphicsAPI.h"
#include "GraphicsEnums.h"
#include "Platform.h"
#include "Block.h"
#include "Events.h"
#include "Funcs.h"
#include "ExtMath.h"

void GfxCommon_Init(void) {
	GfxCommon_quadVb = Gfx_CreateDynamicVb(VertexFormat_P3fC4b, 4);
	GfxCommon_texVb = Gfx_CreateDynamicVb(VertexFormat_P3fT2fC4b, 4);
}

void GfxCommon_Free(void) {
	Gfx_DeleteVb(&GfxCommon_quadVb);
	Gfx_DeleteVb(&GfxCommon_texVb);
}

void GfxCommon_LoseContext(STRING_PURE String* reason) {
	Gfx_LostContext = true;
	String logMsg = String_FromConst("Lost graphics context:");
	Platform_Log(&logMsg);
	Platform_Log(reason);

	Event_RaiseVoid(&GfxEvents_ContextLost);
	GfxCommon_Free();
}

void GfxCommon_RecreateContext(void) {
	Gfx_LostContext = false;
	String logMsg = String_FromConst("Recreating graphics context");
	Platform_Log(&logMsg);

	Event_RaiseVoid(&GfxEvents_ContextRecreated);
	GfxCommon_Init();
}


void GfxCommon_UpdateDynamicVb_Lines(GfxResourceID vb, void* vertices, Int32 vCount) {
	Gfx_SetDynamicVbData(vb, vertices, vCount);
	Gfx_DrawVb_Lines(vCount);
}

void GfxCommon_UpdateDynamicVb_IndexedTris(GfxResourceID vb, void* vertices, Int32 vCount) {
	Gfx_SetDynamicVbData(vb, vertices, vCount);
	Gfx_DrawVb_IndexedTris(vCount);
}

void GfxCommon_Draw2DFlat(Int32 x, Int32 y, Int32 width, Int32 height, PackedCol col) {
	VertexP3fC4b quadVerts[4];
	VertexP3fC4b v; v.Z = 0.0f; v.Col = col;

	v.X = (Real32)x;           v.Y = (Real32)y;            quadVerts[0] = v;
	v.X = (Real32)(x + width);                             quadVerts[1] = v;
	                           v.Y = (Real32)(y + height); quadVerts[2] = v;
	v.X = (Real32)x;                                       quadVerts[3] = v;

	Gfx_SetBatchFormat(VertexFormat_P3fC4b);
	GfxCommon_UpdateDynamicVb_IndexedTris(GfxCommon_quadVb, quadVerts, 4);
}

void GfxCommon_Draw2DGradient(Int32 x, Int32 y, Int32 width, Int32 height, PackedCol topCol, PackedCol bottomCol) {
	VertexP3fC4b quadVerts[4];
	VertexP3fC4b v; v.Z = 0.0f;

	v.X = (Real32)x;           v.Y = (Real32)y;            v.Col = topCol;    quadVerts[0] = v;
	v.X = (Real32)(x + width);                                                quadVerts[1] = v;
	                           v.Y = (Real32)(y + height); v.Col = bottomCol; quadVerts[2] = v;
	v.X = (Real32)x;                                                          quadVerts[3] = v;

	Gfx_SetBatchFormat(VertexFormat_P3fC4b);
	GfxCommon_UpdateDynamicVb_IndexedTris(GfxCommon_quadVb, quadVerts, 4);
}

void GfxCommon_Draw2DTexture(Texture* tex, PackedCol col) {
	VertexP3fT2fC4b texVerts[4];
	VertexP3fT2fC4b* ptr = texVerts;
	GfxCommon_Make2DQuad(tex, col, &ptr);
	Gfx_SetBatchFormat(VertexFormat_P3fT2fC4b);
	GfxCommon_UpdateDynamicVb_IndexedTris(GfxCommon_texVb, texVerts, 4);
}

void GfxCommon_Make2DQuad(Texture* tex, PackedCol col, VertexP3fT2fC4b** vertices) {
	Real32 x1 = (Real32)tex->X, x2 = (Real32)(tex->X + tex->Width);
	Real32 y1 = (Real32)tex->Y, y2 = (Real32)(tex->Y + tex->Height);
#if USE_DX
	/* NOTE: see "https://msdn.microsoft.com/en-us/library/windows/desktop/bb219690(v=vs.85).aspx", */
	/* i.e. the msdn article called "Directly Mapping Texels to Pixels (Direct3D 9)" for why we have to do this. */
	x1 -= 0.5f; x2 -= 0.5f;
	y1 -= 0.5f; y2 -= 0.5f;
#endif

	VertexP3fT2fC4b* ptr = *vertices;
	VertexP3fT2fC4b v; v.Z = 0.0f; v.Col = col;
	v.X = x1; v.Y = y1; v.U = tex->U1; v.V = tex->V1; ptr[0] = v;
	v.X = x2;           v.U = tex->U2;                ptr[1] = v;
	v.Y = y2;                         v.V = tex->V2;  ptr[2] = v;
	v.X = x1;           v.U = tex->U1;                ptr[3] = v;
	vertices += 4;
}

bool gfx_hadFog;
void GfxCommon_Mode2D(Int32 width, Int32 height) {
	Gfx_SetMatrixMode(MatrixType_Projection);
	Gfx_PushMatrix();
	Gfx_LoadOrthoMatrix((Real32)width, (Real32)height);
	Gfx_SetMatrixMode(MatrixType_Modelview);
	Gfx_PushMatrix();
	Gfx_LoadIdentityMatrix();

	Gfx_SetDepthTest(false);
	Gfx_SetAlphaBlending(true);
	gfx_hadFog = Gfx_GetFog();
	if (gfx_hadFog) Gfx_SetFog(false);
}

void GfxCommon_Mode3D(void) {
	Gfx_SetMatrixMode(MatrixType_Projection);
	Gfx_PopMatrix(); /* Get rid of orthographic 2D matrix. */
	Gfx_SetMatrixMode(MatrixType_Modelview);
	Gfx_PopMatrix();

	Gfx_SetDepthTest(false);
	Gfx_SetAlphaBlending(false);
	if (gfx_hadFog) Gfx_SetFog(true);
}

GfxResourceID GfxCommon_MakeDefaultIb(void) {
	UInt16 indices[GFX_MAX_INDICES];
	GfxCommon_MakeIndices(indices, GFX_MAX_INDICES);
	return Gfx_CreateIb(indices, GFX_MAX_INDICES);
}

void GfxCommon_MakeIndices(UInt16* indices, Int32 iCount) {
	Int32 element = 0, i;

	for (i = 0; i < iCount; i += 6) {
		indices[0] = (UInt16)(element + 0);
		indices[1] = (UInt16)(element + 1);
		indices[2] = (UInt16)(element + 2);

		indices[3] = (UInt16)(element + 2);
		indices[4] = (UInt16)(element + 3);
		indices[5] = (UInt16)(element + 0);

		indices += 6; element += 4;
	}
}

void GfxCommon_SetupAlphaState(UInt8 draw) {
	if (draw == DrawType_Translucent)      Gfx_SetAlphaBlending(true);
	if (draw == DrawType_Transparent)      Gfx_SetAlphaTest(true);
	if (draw == DrawType_TransparentThick) Gfx_SetAlphaTest(true);
	if (draw == DrawType_Sprite)           Gfx_SetAlphaTest(true);
}

void GfxCommon_RestoreAlphaState(UInt8 draw) {
	if (draw == DrawType_Translucent)      Gfx_SetAlphaBlending(false);
	if (draw == DrawType_Transparent)      Gfx_SetAlphaTest(false);
	if (draw == DrawType_TransparentThick) Gfx_SetAlphaTest(false);
	if (draw == DrawType_Sprite)           Gfx_SetAlphaTest(false);
}


#define alphaMask ((UInt32)0xFF000000UL)
/* Quoted from http://www.realtimerendering.com/blog/gpus-prefer-premultiplication/
   The short version: if you want your renderer to properly handle textures with alphas when using
   bilinear interpolation or mipmapping, you need to premultiply your PNG color data by their (unassociated) alphas. */
UInt32 GfxCommon_Average(UInt32 p1, UInt32 p2) {
	UInt32 a1 = ((p1 & alphaMask) >> 24) & 0xFF;
	UInt32 a2 = ((p2 & alphaMask) >> 24) & 0xFF;
	UInt32 aSum = (a1 + a2);
	aSum = aSum > 0 ? aSum : 1; // avoid divide by 0 below

	/* Convert RGB to pre-multiplied form */
	UInt32 r1 = ((p1 >> 16) & 0xFF) * a1, g1 = ((p1 >> 8) & 0xFF) * a1, b1 = (p1 & 0xFF) * a1;
	UInt32 r2 = ((p2 >> 16) & 0xFF) * a2, g2 = ((p2 >> 8) & 0xFF) * a2, b2 = (p2 & 0xFF) * a2;

	/* https://stackoverflow.com/a/347376
	   We need to convert RGB back from the pre-multiplied average into normal form
	   ((r1 + r2) / 2) / ((a1 + a2) / 2)
	   but we just cancel out the / 2*/
	UInt32 aAve = aSum >> 1;
	UInt32 rAve = (r1 + r2) / aSum;
	UInt32 gAve = (g1 + g2) / aSum;
	UInt32 bAve = (b1 + b2) / aSum;

	return (aAve << 24) | (rAve << 16) | (gAve << 8) | bAve;
}

void GfxCommon_GenMipmaps(Int32 width, Int32 height, UInt8* lvlScan0, UInt8* scan0) {
	UInt32* baseSrc = (UInt32*)scan0;
	UInt32* baseDst = (UInt32*)lvlScan0;
	Int32 srcWidth = width << 1;

	Int32 x, y;
	for (y = 0; y < height; y++) {
		Int32 srcY = (y << 1);
		UInt32* src0 = baseSrc + srcY * srcWidth;
		UInt32* src1 = src0 + srcWidth;
		UInt32* dst = baseDst + y * width;

		for (x = 0; x < width; x++) {
			Int32 srcX = (x << 1);
			UInt32 src00 = src0[srcX], src01 = src0[srcX + 1];
			UInt32 src10 = src1[srcX], src11 = src1[srcX + 1];

			/* bilinear filter this mipmap */
			UInt32 ave0 = GfxCommon_Average(src00, src01);
			UInt32 ave1 = GfxCommon_Average(src10, src11);
			dst[x] = GfxCommon_Average(ave0, ave1);
		}
	}
}

Int32 GfxCommon_MipmapsLevels(Int32 width, Int32 height) {
	Int32 lvlsWidth = Math_Log2(width), lvlsHeight = Math_Log2(height);
	if (Gfx_CustomMipmapsLevels) {
		Int32 lvls = min(lvlsWidth, lvlsHeight);
		return min(lvls, 4);
	} else {
		return max(lvlsWidth, lvlsHeight);
	}
}