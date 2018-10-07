#ifndef CC_GFXCOMMON_H
#define CC_GFXCOMMON_H
#include "VertexStructs.h"

/* Provides common/shared methods for a 3D graphics rendering API.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
extern String Gfx_ApiInfo[7];
GfxResourceID GfxCommon_defaultIb;

/* Contains the information necessary to describe a 2D textured quad. */
struct Texture {
	GfxResourceID ID;
	int16_t X, Y; uint16_t Width, Height;
	float U1, V1, U2, V2;
};
#define TEX_RECT(x,y, width,height) x,y,width,height
#define TEX_UV(u1,v1, u2,v2)        u1,v1,u2,v2

void GfxCommon_Init(void);
void GfxCommon_Free(void);
void GfxCommon_LoseContext(const char* reason);
void GfxCommon_RecreateContext(void);

/* Binds and draws the specified subset of the vertices in the current dynamic vertex buffer
This method also replaces the dynamic vertex buffer's data first with the given vertices before drawing. */
void GfxCommon_UpdateDynamicVb_Lines(GfxResourceID vb, void* vertices, int vCount);
/*Binds and draws the specified subset of the vertices in the current dynamic vertex buffer
This method also replaces the dynamic vertex buffer's data first with the given vertices before drawing. */
void GfxCommon_UpdateDynamicVb_IndexedTris(GfxResourceID vb, void* vertices, int vCount);

GfxResourceID GfxCommon_quadVb, GfxCommon_texVb;
void GfxCommon_Draw2DFlat(int x, int y, int width, int height, PackedCol col);
void GfxCommon_Draw2DGradient(int x, int y, int width, int height, PackedCol top, PackedCol bottom);
void GfxCommon_Draw2DTexture(struct Texture* tex, PackedCol col);
void GfxCommon_Make2DQuad(struct Texture* tex, PackedCol col, VertexP3fT2fC4b** vertices);

void GfxCommon_Mode2D(int width, int height);
void GfxCommon_Mode3D(void);

void GfxCommon_MakeIndices(uint16_t* indices, int iCount);
void GfxCommon_SetupAlphaState(uint8_t draw);
void GfxCommon_RestoreAlphaState(uint8_t draw);

void GfxCommon_GenMipmaps(int width, int height, uint8_t* lvlScan0, uint8_t* scan0);
int GfxCommon_MipmapsLevels(int width, int height);
void Texture_Render(struct Texture* tex);
void Texture_RenderShaded(struct Texture* tex, PackedCol shadeCol);
#endif
