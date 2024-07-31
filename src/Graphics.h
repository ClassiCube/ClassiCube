#ifndef CC_GFXAPI_H
#define CC_GFXAPI_H
#include "Vectors.h"
#include "PackedCol.h"
CC_BEGIN_HEADER

/* 
Abstracts a 3D graphics rendering API
Copyright 2014-2023 ClassiCube | Licensed under BSD-3
*/
struct Bitmap;
struct Stream;
struct IGameComponent;
struct MenuOptionsScreen;
extern struct IGameComponent Gfx_Component;

typedef enum VertexFormat_ {
	VERTEX_FORMAT_COLOURED, VERTEX_FORMAT_TEXTURED
} VertexFormat;

typedef enum FogFunc_ {
	FOG_LINEAR, FOG_EXP, FOG_EXP2
} FogFunc;

typedef enum MatrixType_ {
	MATRIX_PROJ, /* Projection matrix */
	MATRIX_VIEW  /* Combined model view matrix */
} MatrixType;

#define SIZEOF_VERTEX_COLOURED 16
#define SIZEOF_VERTEX_TEXTURED 24

#if defined CC_BUILD_PSP
/* 3 floats for position (XYZ), 4 bytes for colour */
struct VertexColoured { PackedCol Col; float x, y, z; };
/* 3 floats for position (XYZ), 2 floats for texture coordinates (UV), 4 bytes for colour */
struct VertexTextured { float U, V; PackedCol Col; float x, y, z; };
#else
/* 3 floats for position (XYZ), 4 bytes for colour */
struct VertexColoured { float x, y, z; PackedCol Col; };
/* 3 floats for position (XYZ), 2 floats for texture coordinates (UV), 4 bytes for colour */
struct VertexTextured { float x, y, z; PackedCol Col; float U, V; };
#endif

void Gfx_Create(void);
void Gfx_Free(void);

CC_VAR extern struct _GfxData {
	/* Maximum dimensions in pixels that a texture can be created up to */
	/* NOTE: usually 1024 to 16384 */
	int MaxTexWidth, MaxTexHeight;
	/* Maximum total size in pixels a texture can consist of */
	/* NOTE: Not all graphics backends specify a value for this */
	int MaxTexSize;
	/* Whether context graphics has been lost (all creation/render calls fail) */
	cc_bool LostContext;
	/* Whether some textures are created with mipmaps */
	cc_bool Mipmaps;
	/* Whether managed textures are actually supported */
	/* If not, you must free/create them just like normal textures */
	cc_bool ManagedTextures;
	/* Whether graphics context has been created */
	cc_bool Created;
	struct Matrix View, Projection;
	/* Whether the graphics backend supports non power of two textures */
	cc_bool SupportsNonPowTwoTextures;
	/* Whether the graphics backend supports U/V that don't occupy whole texture */
	/*   e.g. Saturn, 3D0 systems don't support it */
	cc_bool NoUVSupport;
	/* Type of the backend (e.g. OpenGL, Direct3D 9, etc)*/
	cc_uint8 BackendType;
	cc_bool __pad;
	/* Maximum total size in pixels a low resolution texture can consist of */
	/* NOTE: Not all graphics backends specify a value for this */
	int MaxLowResTexSize;
	/* Minimum dimensions in pixels that a texture must be */
	/* NOTE: Most graphics backends do not use this */
	int MinTexWidth, MinTexHeight;
	cc_bool  ReducedPerfMode;
	cc_uint8 ReducedPerfModeCooldown;
	/* Default index buffer for a triangle list representing quads */
	GfxResourceID DefaultIb;
} Gfx;

extern const cc_string Gfx_LowPerfMessage;

#define ICOUNT(verticesCount) (((verticesCount) >> 2) * 6)
#define GFX_MAX_INDICES (65536 / 4 * 6)
#define GFX_MAX_VERTICES 65536

typedef enum GfxBuffers_ {
	GFX_BUFFER_COLOR = 1,
	GFX_BUFFER_DEPTH = 2
} GfxBuffers;

/* Texture should persist across gfx context loss (if backend supports ManagedTextures) */
#define TEXTURE_FLAG_MANAGED  0x01
/* Texture should allow updating via Gfx_UpdateTexture */
#define TEXTURE_FLAG_DYNAMIC  0x02
/* Texture is deliberately (and not accidentally) being created with non power of two dimensions */
#define TEXTURE_FLAG_NONPOW2  0x04
/* Texture can fallback to 16 bpp when necessary (most backends don't do this) */
#define TEXTURE_FLAG_LOWRES   0x08
/* Texture should be rendered using bilinear filtering if possible */
#define TEXTURE_FLAG_BILINEAR 0x10

void  Gfx_RecreateTexture(GfxResourceID* tex, struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps);
void* Gfx_RecreateAndLockVb(GfxResourceID* vb, VertexFormat fmt, int count);

cc_bool Gfx_CheckTextureSize(int width, int height, cc_uint8 flags);
/* Creates a new texture. (and also generates mipmaps if mipmaps) */
/*   See TEXTURE_FLAG values for supported flags */
/* NOTE: Only set mipmaps to true if Gfx_Mipmaps is also true, because whether textures
use mipmapping may be either a per-texture or global state depending on the backend */
CC_API GfxResourceID Gfx_CreateTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps);
GfxResourceID Gfx_CreateTexture2(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps);
/* Updates a region of the given texture. (and mipmapped regions if mipmaps) */
CC_API void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps);
/* Updates a region of the given texture. (and mipmapped regions if mipmaps) */
/* NOTE: rowWidth is in pixels (so for normal bitmaps, rowWidth equals width) */ /* OBSOLETE */
void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps);
/* Sets the currently active texture */
CC_API void Gfx_BindTexture(GfxResourceID texId);
/* Deletes the given texture, then sets it to 0 */
CC_API void Gfx_DeleteTexture(GfxResourceID* texId);

/* NOTE: Completely useless now, and does nothing in all graphics backends */
/*  (used to set whether texture colour is used when rendering vertices) */
CC_API void Gfx_SetTexturing(cc_bool enabled);
/* Turns on mipmapping. (if Gfx_Mipmaps is enabled) */
/* NOTE: You must have created textures with mipmaps true for this to work */
CC_API void Gfx_EnableMipmaps(void);
/* Turns off mipmapping. (if Gfx_Mipmaps is enabled) */
/* NOTE: You must have created textures with mipmaps true for this to work */
CC_API void Gfx_DisableMipmaps(void);

/* Returns whether fog blending is enabled */
CC_API cc_bool Gfx_GetFog(void);
/* Sets whether fog blending is enabled */
CC_API void Gfx_SetFog(cc_bool enabled);
/* Sets the colour of the blended fog */
CC_API void Gfx_SetFogCol(PackedCol col);
/* Sets thickness of fog for FOG_EXP/FOG_EXP2 modes */
CC_API void Gfx_SetFogDensity(float value);
/* Sets extent/end of fog for FOG_LINEAR mode */
CC_API void Gfx_SetFogEnd(float value);
/* Sets in what way fog is blended */
CC_API void Gfx_SetFogMode(FogFunc func);

/* Sets whether backface culling is performed */
CC_API void Gfx_SetFaceCulling(cc_bool enabled);
/* Sets whether pixels with an alpha of less than 128 are discarded */
CC_API void Gfx_SetAlphaTest(cc_bool enabled);
/* Sets whether existing and new pixels are blended together */
CC_API void Gfx_SetAlphaBlending(cc_bool enabled);
/* Sets whether blending between the alpha components of texture and vertex colour is performed */
CC_API void Gfx_SetAlphaArgBlend(cc_bool enabled);

/* Clears the given rendering buffer(s) to default. */
/* buffers can be either GFX_BUFFER_COLOR or GFX_BUFFER_DEPTH, or both */
CC_API void Gfx_ClearBuffers(GfxBuffers buffers);
/* Sets the colour that the colour buffer is cleared to */
CC_API void Gfx_ClearColor(PackedCol color);
/* Sets whether pixels may be discard based on z/depth */
CC_API void Gfx_SetDepthTest(cc_bool enabled);
/* Sets whether z/depth of pixels is actually written to the depth buffer */
CC_API void Gfx_SetDepthWrite(cc_bool enabled);
/* Sets whether R/G/B/A of pixels are actually written to the colour buffer channels */
CC_API void Gfx_SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a);
/* Sets whether the game should only write output to depth buffer */
/*  NOTE: Implicitly calls Gfx_SetColorWrite */
CC_API void Gfx_DepthOnlyRendering(cc_bool depthOnly);

/* Anaglyph 3D rendering support */
void Gfx_Set3DLeft( struct Matrix* proj, struct Matrix* view);
void Gfx_Set3DRight(struct Matrix* proj, struct Matrix* view);
void Gfx_End3D(     struct Matrix* proj, struct Matrix* view);

/* Callback function to initialise/fill out the contents of an index buffer */
typedef void (*Gfx_FillIBFunc)(cc_uint16* indices, int count, void* obj);
/* Creates a new index buffer and fills out its contents */
CC_API GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj);
/* Sets the currently active index buffer */
CC_API void Gfx_BindIb(GfxResourceID ib);
/* Deletes the given index buffer, then sets it to 0 */
CC_API void Gfx_DeleteIb(GfxResourceID* ib);

/* Creates a new vertex buffer */
CC_API GfxResourceID Gfx_CreateVb(VertexFormat fmt, int count);
/* Sets the currently active vertex buffer */
CC_API void Gfx_BindVb(GfxResourceID vb);
/* Deletes the given vertex buffer, then sets it to 0 */
CC_API void Gfx_DeleteVb(GfxResourceID* vb);
/* Acquires temp memory for changing the contents of a vertex buffer */
CC_API void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count);
/* Submits the changed contents of a vertex buffer */
CC_API void  Gfx_UnlockVb(GfxResourceID vb);

/* TODO: How to make LockDynamicVb work with OpenGL 1.1 Builder stupidity. */
#ifdef CC_BUILD_GL11
/* Special case of Gfx_Create/LockVb for building chunks in Builder.c */
GfxResourceID Gfx_CreateVb2(void* vertices, VertexFormat fmt, int count);
#endif
#if CC_GFX_BACKEND == CC_GFX_BACKEND_GL2
/* Special case Gfx_BindVb for use with Gfx_DrawIndexedTris_T2fC4b */
void Gfx_BindVb_Textured(GfxResourceID vb);
#else
#define Gfx_BindVb_Textured Gfx_BindVb
#endif

/* Creates a new dynamic vertex buffer, whose contents can be updated later */
CC_API GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices);
/* Sets the active vertex buffer to the given dynamic vertex buffer */
CC_API void  Gfx_BindDynamicVb(GfxResourceID vb);
/* Deletes the given dynamic vertex buffer, then sets it to 0 */
CC_API void  Gfx_DeleteDynamicVb(GfxResourceID* vb);
/* Acquires temp memory for changing the contents of a dynamic vertex buffer */
CC_API void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count);
/* Binds then submits the changed contents of a dynamic vertex buffer */
CC_API void  Gfx_UnlockDynamicVb(GfxResourceID vb);

/* Updates the data of a dynamic vertex buffer */
CC_API void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount);

/* Sets the format of the rendered vertices */
CC_API void Gfx_SetVertexFormat(VertexFormat fmt);
/* Renders vertices from the currently bound vertex buffer as lines */
CC_API void Gfx_DrawVb_Lines(int verticesCount);
/* Renders vertices from the currently bound vertex and index buffer as triangles */
/* NOTE: Offsets each index by startVertex */
CC_API void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex);
/* Renders vertices from the currently bound vertex and index buffer as triangles */
CC_API void Gfx_DrawVb_IndexedTris(int verticesCount);
/* Special case Gfx_DrawVb_IndexedTris_Range for map renderer */
void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex);

/* Loads the given matrix over the currently active matrix */
CC_API void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix);
CC_API void Gfx_EnableTextureOffset(float x, float y);
CC_API void Gfx_DisableTextureOffset(void);
/* Loads given modelview and projection matrices, then calculates the combined MVP matrix */
void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp);

/* Calculates an orthographic projection matrix suitable with this backend. (usually for 2D) */
void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar);
/* Calculates a perspective projection matrix suitable with this backend. (usually for 3D) */
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar);
/* NOTE: Projection matrix calculation is here because it can depend the graphics backend */
/*  (e.g. OpenGL uses a Z clip space range of [-1, 1], whereas Direct3D9 uses [0, 1]) */

/* Outputs a .png screenshot of the backbuffer */
cc_result Gfx_TakeScreenshot(struct Stream* output);
/* Warns in chat if the backend has problems with the user's GPU */
/* Returns whether legacy rendering mode for borders/sky/clouds is needed */
cc_bool Gfx_WarnIfNecessary(void);
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s);
/* Sets up state for rendering a new frame */
void Gfx_BeginFrame(void);
/* Finishes rendering a frame, and swaps it with the back buffer */
void Gfx_EndFrame(void);
/* Sets whether to synchronise with monitor refresh to avoid tearing */
/* NOTE: VSync setting may be unsupported or just ignored */
void Gfx_SetVSync(cc_bool vsync);
/* Gets information about the user's GPU and current backend state */
/* Backend state may include depth buffer bits, free memory, etc */
/* NOTE: Each line is separated by \n */
void Gfx_GetApiInfo(cc_string* info);

/* Updates state when the window's dimensions have changed */
/* NOTE: This may require recreating the context depending on the backend */
void Gfx_OnWindowResize(void);
CC_API void Gfx_SetViewport(int x, int y, int w, int h);
CC_API void Gfx_SetScissor (int x, int y, int w, int h);

enum Screen3DS { TOP_SCREEN, BOTTOM_SCREEN };
#ifdef CC_BUILD_DUALSCREEN
/* Selects which screen on the 3DS to render to */
void Gfx_3DS_SetRenderScreen(enum Screen3DS screen);
#else
static CC_INLINE void Gfx_3DS_SetRenderScreen(enum Screen3DS screen) { }
#endif

/* Raises ContextLost event and updates state for lost contexts */
void Gfx_LoseContext(const char* reason);
/* Raises ContextRecreated event and restores internal state */
void Gfx_RecreateContext(void);
/* Attempts to restore a lost context */
cc_bool Gfx_TryRestoreContext(void);

/* Renders a 2D flat coloured rectangle */
void Gfx_Draw2DFlat(int x, int y, int width, int height, PackedCol color);
/* Renders a 2D flat vertical gradient rectangle */
void Gfx_Draw2DGradient(int x, int y, int width, int height, PackedCol top, PackedCol bottom);
/* Renders a 2D coloured texture */
void Gfx_Draw2DTexture(const struct Texture* tex, PackedCol color);
/* Fills out the vertices for rendering a 2D coloured texture */
void Gfx_Make2DQuad(const struct Texture* tex, PackedCol color, struct VertexTextured** vertices);

/* Switches state to be suitable for drawing 2D graphics */
/* NOTE: This means turning off fog/depth test, changing matrices, etc.*/
void Gfx_Begin2D(int width, int height);
/* Switches state to be suitable for drawing 3D graphics */
/* NOTE: This means restoring fog/depth test, restoring matrices, etc */
void Gfx_End2D(void);

/* Sets appropriate alpha test/blending for given block draw type */
void Gfx_SetupAlphaState(cc_uint8 draw);
/* Undoes changes to alpha test/blending state by Gfx_SetupAlphaState */
void Gfx_RestoreAlphaState(cc_uint8 draw);

/* Statically initialises the position and dimensions of this texture */
#define Tex_Rect(x,y, width,height) x,y,width,height
/* Statically initialises the texture coordinate corners of this texture */
#define Tex_UV(u1,v1, u2,v2)        { u1,v1,u2,v2 }
/* Sets the position and dimensions of this texture */
#define Tex_SetRect(tex, xVal,yVal, wVal, hVal) tex.x = xVal; tex.y = yVal; tex.width = wVal; tex.height = hVal;
/* Sets texture coordinate corners of this texture */
/* Useful to only draw a sub-region of the texture's pixels */
#define Tex_SetUV(tex, U1,V1, U2,V2) tex.uv.u1 = U1; tex.uv.v1 = V1; tex.uv.u2 = U2; tex.uv.v2 = V2;

/* Binds then renders the given texture */
void Texture_Render(const struct Texture* tex);
/* Binds then renders the given texture */
void Texture_RenderShaded(const struct Texture* tex, PackedCol shadeColor);

CC_END_HEADER
#endif
