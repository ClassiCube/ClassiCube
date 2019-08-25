#ifndef CC_GFXAPI_H
#define CC_GFXAPI_H
#include "PackedCol.h"
#include "Vectors.h"
#include "GameStructs.h"
#include "Bitmap.h"
#include "VertexStructs.h"

/* Abstracts a 3D graphics rendering API.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/
struct Stream;

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

CC_VAR extern struct _GfxData {
	/* Maximum dimensions textures can be created up to. (usually 1024 to 16384) */
	int MaxTexWidth, MaxTexHeight;
	float MinZNear;
	/* Whether context graphics has been lost (all creation/render fails) */
	bool LostContext;
	/* Whether some textures are created with mipmaps. */
	bool Mipmaps;
	/* Whether mipmaps must be created for all dimensions down to 1x1 or not. */
	bool CustomMipmapsLevels;
	/* Whether Gfx_Init has been called to initialise state. */
	bool Initialised;
	struct Matrix View, Projection;
} Gfx;

#define GFX_APIINFO_LINES 7
extern GfxResourceID Gfx_defaultIb;
extern GfxResourceID Gfx_quadVb, Gfx_texVb;

#define ICOUNT(verticesCount) (((verticesCount) >> 2) * 6)
#define GFX_MAX_INDICES (65536 / 4 * 6)
#define GFX_MAX_VERTICES 65536

/* Creates a new texture. (and also generates mipmaps if mipmaps) */
/* NOTE: Only set mipmaps to true if Gfx_Mipmaps is also true, because whether textures
use mipmapping may be either a per-texture or global state depending on the backend. */
CC_API GfxResourceID Gfx_CreateTexture(Bitmap* bmp, bool managedPool, bool mipmaps);
/* Updates a region of the given texture. (and mipmapped regions if mipmaps) */
CC_API void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, Bitmap* part, bool mipmaps);
/* Sets the currently active texture. */
CC_API void Gfx_BindTexture(GfxResourceID texId);
/* Deletes the given texture, then sets it to GFX_NULL. */
CC_API void Gfx_DeleteTexture(GfxResourceID* texId);
/* Sets whether texture colour is used when rendering vertices. */
CC_API void Gfx_SetTexturing(bool enabled);
/* Turns on mipmapping. (if Gfx_Mipmaps is enabled) */
/* NOTE: You must have created textures with mipmaps true for this to work. */
CC_API void Gfx_EnableMipmaps(void);
/* Turns off mipmapping. (if GfX_Mipmaps is enabled) */
/* NOTE: You must have created textures with mipmaps true for this to work. */
CC_API void Gfx_DisableMipmaps(void);

/* Returns whether fog blending is enabled. */
CC_API bool Gfx_GetFog(void);
/* Sets whether fog blending is enabled. */
CC_API void Gfx_SetFog(bool enabled);
/* Sets the colour of the blended fog. */
CC_API void Gfx_SetFogCol(PackedCol col);
/* Sets thickness of fog for FOG_EXP/FOG_EXP2 modes. */
CC_API void Gfx_SetFogDensity(float value);
/* Sets extent/end of fog for FOG_LINEAR mode. */
CC_API void Gfx_SetFogEnd(float value);
/* Sets in what way fog is blended. */
CC_API void Gfx_SetFogMode(FogFunc func);

/* Sets whether backface culling is performed. */
CC_API void Gfx_SetFaceCulling(bool enabled);
/* Sets whether pixels with an alpha of less than 128 are discarded. */
CC_API void Gfx_SetAlphaTest(bool enabled);
/* Sets whether existing and new pixels are blended together. */
CC_API void Gfx_SetAlphaBlending(bool enabled);
/* Sets whether blending between the alpha components of texture and vertex colour is performed. */
CC_API void Gfx_SetAlphaArgBlend(bool enabled);

/* Clears the colour and depth buffer to default. */
CC_API void Gfx_Clear(void);
/* Sets the colour that the colour buffer is cleared to. */
CC_API void Gfx_ClearCol(PackedCol col);
/* Sets whether pixels may be discard based on z/depth. */
CC_API void Gfx_SetDepthTest(bool enabled);
/* Sets whether R/G/B/A of pixels are actually written to the colour buffer channels. */
CC_API void Gfx_SetColWriteMask(bool r, bool g, bool b, bool a);
/* Sets whether z/depth of pixels is actually written to the depth buffer. */
CC_API void Gfx_SetDepthWrite(bool enabled);

/* Creates a new dynamic vertex buffer, whose contents can be updated later. */
CC_API GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices);
/* Creates a new vertex buffer and fills out its contents. */
CC_API GfxResourceID Gfx_CreateVb(void* vertices, VertexFormat fmt, int count);
/* Creates a new index buffer and fills out its contents. */
CC_API GfxResourceID Gfx_CreateIb(void* indices, int indicesCount);
/* Sets the currently active vertex buffer. */
CC_API void Gfx_BindVb(GfxResourceID vb);
/* Sets the currently active index buffer. */
CC_API void Gfx_BindIb(GfxResourceID ib);
/* Deletes the given vertex buffer, then sets it to GFX_NULL. */
CC_API void Gfx_DeleteVb(GfxResourceID* vb);
/* Deletes the given index buffer, then sets it to GFX_NULL. */
CC_API void Gfx_DeleteIb(GfxResourceID* ib);

/* Sets the format of the rendered vertices. */
CC_API void Gfx_SetVertexFormat(VertexFormat fmt);
/* Updates the data of a dynamic vertex buffer. */
CC_API void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount);
/* Renders vertices from the currently bound vertex buffer as lines. */
CC_API void Gfx_DrawVb_Lines(int verticesCount);
/* Renders vertices from the currently bound vertex and index buffer as triangles. */
/* NOTE: Offsets each index by startVertex. */
CC_API void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex);
/* Renders vertices from the currently bound vertex and index buffer as triangles. */
CC_API void Gfx_DrawVb_IndexedTris(int verticesCount);
/* Special case Gfx_DrawVb_IndexedTris_Range for map renderer */
void Gfx_DrawIndexedVb_TrisT2fC4b(int verticesCount, int startVertex);

/* Loads the given matrix over the currently active matrix. */
CC_API void Gfx_LoadMatrix(MatrixType type, struct Matrix* matrix);
/* Loads the identity matrix over the currently active matrix. */
CC_API void Gfx_LoadIdentityMatrix(MatrixType type);
/* Calculates an orthographic matrix suitable with this backend. (usually for 2D) */
void Gfx_CalcOrthoMatrix(float width, float height, struct Matrix* matrix);
/* Calculates a projection matrix suitable with this backend. (usually for 3D) */
void Gfx_CalcPerspectiveMatrix(float fov, float aspect, float zNear, float zFar, struct Matrix* matrix);

/* Outputs a .png screenshot of the backbuffer. */
ReturnCode Gfx_TakeScreenshot(struct Stream* output);
/* Warns in chat if the backend has problems with the user's GPU. */
/* Returns whether legacy rendering mode for borders/sky/clouds is needed. */
bool Gfx_WarnIfNecessary(void);
/* Sets up state for rendering a new frame. */
void Gfx_BeginFrame(void);
/* Finishes rendering a frame, and swaps it with the back buffer. */
void Gfx_EndFrame(void);
/* Sets whether to synchronise with monitor refresh to avoid tearing, and maximum frame rate. */
/* NOTE: VSync setting may be unsupported or just ignored. */
void Gfx_SetFpsLimit(bool value, float minFrameMillis);
/* Updates state when the window's dimensions have changed. */
/* NOTE: This may require recreating the context depending on the backend. */
void Gfx_OnWindowResize(void);
/* Gets information about the user's GPU and current backend state. */
/* Backend state may include depth buffer bits, free memory, etc. */
/* NOTE: lines must be an array of at least GFX_APIINFO_LINES */
void Gfx_GetApiInfo(String* lines);

/* Raises ContextLost event and updates state for lost contexts. */
void Gfx_LoseContext(const char* reason);
/* Attempts to restore a lost context. Raises ContextRecreated event on success. */
bool Gfx_TryRestoreContext(void);

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
void Gfx_MakeIndices(cc_uint16* indices, int iCount);
/* Sets appropriate alpha test/blending for given block draw type. */
void Gfx_SetupAlphaState(cc_uint8 draw);
/* Undoes changes to alpha test/blending state by Gfx_SetupAlphaState. */
void Gfx_RestoreAlphaState(cc_uint8 draw);
/* Generates the next mipmaps level bitmap for the given bitmap. */
void Gfx_GenMipmaps(int width, int height, cc_uint8* lvlScan0, cc_uint8* scan0);
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
