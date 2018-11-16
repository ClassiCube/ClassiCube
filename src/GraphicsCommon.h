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
	TextureRec uv;
};
/* Statically initialises the position and dimensions of this texture */
#define Tex_Rect(x,y, width,height) x,y,width,height
/* Statically initialises the texture coordinate corners of this texture */
#define Tex_UV(u1,v1, u2,v2)        u1,v1,u2,v2
/* Sets the position and dimensions of this texture */
#define Tex_SetRect(tex, x,y, width, height) tex.X = x; tex.Y = y; tex.Width = width; tex.Height = height;
/* Sets texture coordinate corners of this texture */
/* Useful to only draw a sub-region of the texture's pixels */
#define Tex_SetUV(tex, u1,v1, u2,v2) tex.uv.U1 = u1; tex.uv.V1 = v1; tex.uv.U2 = u2; tex.uv.V2 = v2;

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
void GfxCommon_Draw2DTexture(const struct Texture* tex, PackedCol col);
void GfxCommon_Make2DQuad(const struct Texture* tex, PackedCol col, VertexP3fT2fC4b** vertices);

void GfxCommon_Mode2D(int width, int height);
void GfxCommon_Mode3D(void);

void GfxCommon_MakeIndices(uint16_t* indices, int iCount);
void GfxCommon_SetupAlphaState(uint8_t draw);
void GfxCommon_RestoreAlphaState(uint8_t draw);

void GfxCommon_GenMipmaps(int width, int height, uint8_t* lvlScan0, uint8_t* scan0);
int GfxCommon_MipmapsLevels(int width, int height);
void Texture_Render(const struct Texture* tex);
void Texture_RenderShaded(const struct Texture* tex, PackedCol shadeCol);
#endif
