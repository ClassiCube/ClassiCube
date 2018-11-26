#ifndef CC_GFXAPI_H
#define CC_GFXAPI_H
#include "PackedCol.h"
#include "Vectors.h"
#include "GameStructs.h"
#include "Bitmap.h"
#include "VertexStructs.h"

/* Abstracts a 3D graphics rendering API.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct Stream;

typedef enum CompareFunc_ {
	COMPARE_FUNC_ALWAYS, COMPARE_FUNC_NOTEQUAL,  COMPARE_FUNC_NEVER,
	COMPARE_FUNC_LESS,   COMPARE_FUNC_LESSEQUAL, COMPARE_FUNC_EQUAL,
	COMPARE_FUNC_GREATEREQUAL, COMPARE_FUNC_GREATER
} CompareFunc;
typedef enum BlendFunc_ {
	BLEND_FUNC_ZERO,          BLEND_FUNC_ONE,       BLEND_FUNC_SRC_ALPHA,
	BLEND_FUNC_INV_SRC_ALPHA, BLEND_FUNC_DST_ALPHA, BLEND_FUNC_INV_DST_ALPHA
} BlendFunc;

typedef enum VertexFormat_ {
	VERTEX_FORMAT_P3FC4B, VERTEX_FORMAT_P3FT2FC4B
} VertexFormat;
typedef enum FogFunc_ {
	FOG_LINEAR, FOG_EXP, FOG_EXP2
} FogFunc;
typedef enum MatrixType_ {
	MATRIX_PROJECTION, MATRIX_VIEW, MATRIX_TEXTURE
} MatrixType;

void Gfx_Init(void);
void Gfx_Free(void);

extern int Gfx_MaxTexWidth, Gfx_MaxTexHeight;
extern float Gfx_MinZNear;
extern bool Gfx_LostContext;
extern bool Gfx_Mipmaps;
extern bool Gfx_CustomMipmapsLevels;
extern struct Matrix Gfx_View, Gfx_Projection;

extern String Gfx_ApiInfo[7];
extern GfxResourceID Gfx_defaultIb;
extern GfxResourceID Gfx_quadVb, Gfx_texVb;

#define ICOUNT(verticesCount) (((verticesCount) >> 2) * 6)
#define GFX_MAX_INDICES (65536 / 4 * 6)
#define GFX_MAX_VERTICES 65536

/* Callback invoked when the context is lost. Repeatedly invoked until a context can be retrieved. */
extern ScheduledTaskCallback Gfx_LostContextFunction;

/* Creates a new texture. (and also generates mipmaps if mipmaps) */
/* NOTE: Only set mipmaps to true if Gfx_Mipmaps is also true, because whether textures
use mipmapping may be either a per-texture or global state depending on the backend. */
GfxResourceID Gfx_CreateTexture(Bitmap* bmp, bool managedPool, bool mipmaps);
/* Updates a region of the given texture. (and mipmapped regions if mipmaps) */
void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, Bitmap* part, bool mipmaps);
/* Sets the currently active texture. */
void Gfx_BindTexture(GfxResourceID texId);
/* Deletes the given texture, then sets it to GFX_NULL. */
void Gfx_DeleteTexture(GfxResourceID* texId);
/* Sets whether texture colour is used when rendering vertices. */
void Gfx_SetTexturing(bool enabled);
/* Turns on mipmapping. (if Gfx_Mipmaps is enabled) */
/* NOTE: You must have created textures with mipmaps true for this to work. */
void Gfx_EnableMipmaps(void);
/* Turns off mipmapping. (if GfX_Mipmaps is enabled) */
/* NOTE: You must have created textures with mipmaps true for this to work. */
void Gfx_DisableMipmaps(void);

/* Returns whether fog blending is enabled. */
bool Gfx_GetFog(void);
/* Sets whether fog blending is enabled. */
void Gfx_SetFog(bool enabled);
/* Sets the colour of the blended fog. */
void Gfx_SetFogCol(PackedCol col);
/* Sets thickness of fog for FOG_EXP/FOG_EXP2 modes. */
void Gfx_SetFogDensity(float value);
/* Sets extent/end of fog for FOG_LINEAR mode. */
void Gfx_SetFogEnd(float value);
/* Sets in what way fog is blended. */
void Gfx_SetFogMode(FogFunc func);

/* Sets whether backface culling is performed. */
void Gfx_SetFaceCulling(bool enabled);
/* Sets whether new pixels may be discarded based on their alpha. */
void Gfx_SetAlphaTest(bool enabled);
/* Sets in what way pixels may be discarded based on their alpha. */
void Gfx_SetAlphaTestFunc(CompareFunc func, float refValue);
/* Sets whether existing and new pixels are blended together. */
void Gfx_SetAlphaBlending(bool enabled);
/* Sets in what way existing and new pixels are blended. */
void Gfx_SetAlphaBlendFunc(BlendFunc srcFunc, BlendFunc dstFunc);
/* Sets whether blending between the alpha components of texture and vertex colour is performed. */
void Gfx_SetAlphaArgBlend(bool enabled);

/* Clears the colour and depth buffer to default. */
void Gfx_Clear(void);
/* Sets the colour that the colour buffer is cleared to. */
void Gfx_ClearCol(PackedCol col);
/* Sets whether pixels may be discard based on z/depth. */
void Gfx_SetDepthTest(bool enabled);
/* Sets in what may pixels may be discarded based on z/depth. */
void Gfx_SetDepthTestFunc(CompareFunc func);
/* Sets whether R/G/B/A of pixels are actually written to the colour buffer channels. */
void Gfx_SetColWriteMask(bool r, bool g, bool b, bool a);
/* Sets whether z/depth of pixels is actually written to the depth buffer. */
void Gfx_SetDepthWrite(bool enabled);

/* Creates a new dynamic vertex buffer, whose contents can be updated later. */
GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices);
/* Creates a new vertex buffer and fills out its contents. */
GfxResourceID Gfx_CreateVb(void* vertices, VertexFormat fmt, int count);
/* Creates a new index buffer and fills out its contents. */
GfxResourceID Gfx_CreateIb(void* indices, int indicesCount);
/* Sets the currently active vertex buffer. */
void Gfx_BindVb(GfxResourceID vb);
/* Sets the currently active index buffer. */
void Gfx_BindIb(GfxResourceID ib);
/* Deletes the given vertex buffer, then sets it to GFX_NULL. */
void Gfx_DeleteVb(GfxResourceID* vb);
/* Deletes the given index buffer, then sets it to GFX_NULL. */
void Gfx_DeleteIb(GfxResourceID* ib);

/* Sets the format of the rendered vertices. */
void Gfx_SetVertexFormat(VertexFormat fmt);
/* Updates the data of a dynamic vertex buffer. */
void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount);
/* Renders vertices from the currently bound vertex buffer as lines. */
void Gfx_DrawVb_Lines(int verticesCount);
/* Renders vertices from the currently bound vertex and index buffer as triangles. */
/* NOTE: Offsets each index by startVertex. */
void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex);
/* Renders vertices from the currently bound vertex and index buffer as triangles. */
void Gfx_DrawVb_IndexedTris(int verticesCount);
/* Special case Gfx_DrawVb_IndexedTris_Range for map renderer */
void Gfx_DrawIndexedVb_TrisT2fC4b(int verticesCount, int startVertex);

/* Loads the given matrix over the currently active matrix. */
void Gfx_LoadMatrix(MatrixType type, struct Matrix* matrix);
/* Loads the identity matrix over the currently active matrix. */
void Gfx_LoadIdentityMatrix(MatrixType type);
/* Calculates an orthographic matrix suitable with this backend. (usually for 2D) */
void Gfx_CalcOrthoMatrix(float width, float height, struct Matrix* matrix);
/* Calculates a projection matrix suitable with this backend. (usually for 3D) */
void Gfx_CalcPerspectiveMatrix(float fov, float aspect, float zNear, float zFar, struct Matrix* matrix);

/* Outputs a .png screenshot of the backbuffer. */
ReturnCode Gfx_TakeScreenshot(struct Stream* output, int width, int height);
/* Warns in chat if the backend has problems with the user's GPU. /*
/* Returns whether legacy rendering mode for borders/sky/clouds is needed. */
bool Gfx_WarnIfNecessary(void);
/* Sets up state for rendering a new frame. */
void Gfx_BeginFrame(void);
/* Finishes rendering a frame, and swaps it with the back buffer. */
void Gfx_EndFrame(void);
/* Sets whether to synchronise with monitor refresh to avoid tearing. */
/* NOTE: This setting may be unsupported or just ignored. */
void Gfx_SetVSync(bool value);
/* Updates state when the window's dimensions have changed. */
/* NOTE: This may require recreating the context depending on the backend. */
void Gfx_OnWindowResize(void);
/* Fills Gfx_ApiInfo with information about the backend state and the user's GPU. */
void Gfx_MakeApiInfo(void);
/* Updates Gfx_ApiInfo with current backend state information. */
/* NOTE: This is information such as current free memory, etc. */
void Gfx_UpdateApiInfo(void);

/* Raises ContextLost event and updates state for lost contexts. */
void Gfx_LoseContext(const char* reason);
/* Raises ContextRecreated event and restores state from lost contexts. */
void Gfx_RecreateContext(void);

/* Binds and draws the specified subset of the vertices in the current dynamic vertex buffer. */
/* NOTE: This replaces the dynamic vertex buffer's data first with the given vertices before drawing. */
void Gfx_UpdateDynamicVb_Lines(GfxResourceID vb, void* vertices, int vCount);
/* Binds and draws the specified subset of the vertices in the current dynamic vertex buffer. */
/* NOTE: This replaces the dynamic vertex buffer's data first with the given vertices before drawing. */
void Gfx_UpdateDynamicVb_IndexedTris(GfxResourceID vb, void* vertices, int vCount);

/* Renders a 2D flat coloured rectangle. */
void Gfx_Draw2DFlat(int x, int y, int width, int height, PackedCol col);
/* Renders a 2D flat vertical gradient rectangle. */
void Gfx_Draw2DGradient(int x, int y, int width, int height, PackedCol top, PackedCol bottom);
/* Renders a 2D coloured texture. */
void Gfx_Draw2DTexture(const struct Texture* tex, PackedCol col);
/* Fills out the vertices for rendering a 2D coloured texture. */
void Gfx_Make2DQuad(const struct Texture* tex, PackedCol col, VertexP3fT2fC4b** vertices);

/* Switches state to be suitable for drawing 2D graphics. */
/* NOTE: This means turning off fog/depth test, changing matrices, etc.*/
void Gfx_Mode2D(int width, int height);
/* Switches state to be suitable for drawing 3D graphics. */
/* NOTE: This means restoring fog/depth test, restoring matrices, etc. */
void Gfx_Mode3D(void);

/* Fills out indices array with {0,1,2} {2,3,0}, {4,5,6} {6,7,4} etc. */
void Gfx_MakeIndices(uint16_t* indices, int iCount);
/* Sets appropriate alpha test/blending for given block draw type. */
void Gfx_SetupAlphaState(uint8_t draw);
/* Undoes changes to alpha test/blending state by Gfx_SetupAlphaState. */
void Gfx_RestoreAlphaState(uint8_t draw);
/* Generates the next mipmaps level bitmap for the given bitmap. */
void Gfx_GenMipmaps(int width, int height, uint8_t* lvlScan0, uint8_t* scan0);
/* Returns the maximum number of mipmaps levels used for given size. */
int Gfx_MipmapsLevels(int width, int height);

/* Statically initialises the position and dimensions of this texture */
#define Tex_Rect(x,y, width,height) x,y,width,height
/* Statically initialises the texture coordinate corners of this texture */
#define Tex_UV(u1,v1, u2,v2)        u1,v1,u2,v2
/* Sets the position and dimensions of this texture */
#define Tex_SetRect(tex, x,y, width, height) tex.X = x; tex.Y = y; tex.Width = width; tex.Height = height;
/* Sets texture coordinate corners of this texture */
/* Useful to only draw a sub-region of the texture's pixels */
#define Tex_SetUV(tex, u1,v1, u2,v2) tex.uv.U1 = u1; tex.uv.V1 = v1; tex.uv.U2 = u2; tex.uv.V2 = v2;

/* Binds then renders the given texture. */
void Texture_Render(const struct Texture* tex);
/* Binds then renders the given texture. */
void Texture_RenderShaded(const struct Texture* tex, PackedCol shadeCol);
#endif
