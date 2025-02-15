#ifndef CC_GFXAPI_H
#define CC_GFXAPI_H
#include "Vectors.h"
#include "PackedCol.h"
CC_BEGIN_HEADER

/* 
SUMMARY:
- Provides a low level abstraction over a 3D graphics rendering API.
- Because of the numerous possible graphics backends, only a small number of
   functions are provided so that the available functionality behaves the same
   regardless of the graphics backend being used. (as much as reasonably possible)
- Most code using Graphics.h therefore doesn't need to care about the graphics backend being used

IMPLEMENTATION NOTES:
- By default, a reasonable graphics backend is automatically selected in Core.h
- The selected graphics backend can be altered in two ways:
  * explicitly defining CC_GFX_BACKEND in the compilation flags (recommended)
  * altering DEFAULT_GFX_BACKEND for the platform in Core.h
- graphics backends are implemented in Graphics_GL1.c, Graphics_D3D9.c etc
   
Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct Bitmap;
struct Stream;
struct IGameComponent;
struct MenuOptionsScreen;
extern struct IGameComponent Gfx_Component;

typedef enum VertexFormat_ {
	VERTEX_FORMAT_COLOURED, VERTEX_FORMAT_TEXTURED
} VertexFormat;

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
	/* Whether graphics context has been lost (all creation/render calls fail) */
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
	/* Limitations of the graphics backend, see GFX_LIMIT values */
	cc_bool Limitations;
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

/* Whether the graphics backend supports U/V that don't occupy whole texture */
/*   e.g. Saturn, 3D0 systems don't support it */
#define GFX_LIMIT_NO_UV_SUPPORT   0x01
/* Whether the graphics backend requires very large quads to be broken */
/*  up into smaller quads, to reduce fog interpolation artifacts */
#define GFX_LIMIT_VERTEX_ONLY_FOG 0x02

extern const cc_string Gfx_LowPerfMessage;

#define ICOUNT(verticesCount) (((verticesCount) >> 2) * 6)
#define GFX_MAX_INDICES (65536 / 4 * 6)
#define GFX_MAX_VERTICES 65536

void Gfx_RecreateTexture(GfxResourceID* tex, struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps);


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
/*
SUMMARY:
	Textures are used to store a bitmap which can then later be used when drawing
*/
/* Texture should persist across gfx context loss (if backend supports ManagedTextures) */
#define TEXTURE_FLAG_MANAGED     0x01
/* Texture should allow updating via Gfx_UpdateTexture */
#define TEXTURE_FLAG_DYNAMIC     0x02
/* Texture is deliberately (and not accidentally) being created with non power of two dimensions */
#define TEXTURE_FLAG_NONPOW2     0x04
/* Texture can fallback to fewer BPP when necessary (most backends don't do this) */
#define TEXTURE_FLAG_LOWRES      0x08
/* Texture should be rendered using bilinear filtering if possible */
#define TEXTURE_FLAG_BILINEAR    0x10

cc_bool Gfx_CheckTextureSize(int width, int height, cc_uint8 flags);
/* Creates a new texture. (and also generates mipmaps if mipmaps) */
/*   See TEXTURE_FLAG values for supported flags */
/* NOTE: Only set mipmaps to true if Gfx_Mipmaps is also true, because whether textures
use mipmapping may be either a per-texture or global state depending on the backend */
CC_API GfxResourceID Gfx_CreateTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps);
GfxResourceID Gfx_CreateTexture2(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps);
/* NOTE: Should not be used directly, use Gfx_CreateTexture or Gfx_CreateTexture2 instead */
GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps);
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


/*########################################################################################################################*
*------------------------------------------------------Frame management---------------------------------------------------*
*#########################################################################################################################*//*
SUMMARY:
	The frame management functions manage frame related functionality
	(beginning frames, displaying framebuffers, and resetting rendering buffers)
USAGE NOTES:
	There is usually no need to call these functions
*/
typedef enum GfxBuffers_ {
	GFX_BUFFER_COLOR = 1,
	GFX_BUFFER_DEPTH = 2
} GfxBuffers;

/* Clears the given rendering buffer(s) to their default values. */
/* buffers can be either GFX_BUFFER_COLOR or GFX_BUFFER_DEPTH, or both */
CC_API void Gfx_ClearBuffers(GfxBuffers buffers);
/* Sets the default colour that the colour buffer is cleared to */
CC_API void Gfx_ClearColor(PackedCol color);

/* Sets up state for rendering a new frame */
void Gfx_BeginFrame(void);
/* Finishes rendering a frame, and swaps it with the back buffer */
void Gfx_EndFrame(void);
/* Sets whether to synchronise rendering with monitor refresh to avoid tearing */
/* NOTE: VSync setting may be unsupported or just ignored */
void Gfx_SetVSync(cc_bool vsync);

enum Screen3DS { TOP_SCREEN, BOTTOM_SCREEN };
#ifdef CC_BUILD_DUALSCREEN
/* Selects which screen/display to render to */
void Gfx_3DS_SetRenderScreen(enum Screen3DS screen);
#else
/* Selects which screen/display to render to */
static CC_INLINE void Gfx_3DS_SetRenderScreen(enum Screen3DS screen) { }
#endif


/*########################################################################################################################*
*---------------------------------------------------------Fog state-------------------------------------------------------*
*#########################################################################################################################*/
/*
SUMMARY:
	Fog can be used to adjust the colour of pixels based on their distance from the camera
IMPLEMENTATION NOTES:
	Some console ports rendering backends do not support fog
USAGE NOTES:
	There is rarely a need to use the fog functions
*/

typedef enum FogFunc_ {
	FOG_LINEAR, FOG_EXP, FOG_EXP2
} FogFunc;

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


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
/*
SUMMARY:
	The state management functions control how pixels are:
	- potentially skipped (alpha testing, depth testing, face culling)
	- written to the framebuffer (alpha blending, color write)
	- written to the depth buffer (depth write)
IMPLEMENTATION NOTES:
	Some rendering backends do not support some state management functions
	For example, Gfx_SetColorWrite may not be supported by the underlying GPU
USAGE NOTES:
	Alpha testing and blending are the main functions used
*/
/* Sets whether backface culling is performed */
CC_API void Gfx_SetFaceCulling(cc_bool enabled);
/* Sets whether pixels with an alpha of less than 128 are discarded */
CC_API void Gfx_SetAlphaTest(cc_bool enabled);
/* Sets whether existing and new pixels are blended together */
CC_API void Gfx_SetAlphaBlending(cc_bool enabled);
/* Sets whether blending between the alpha components of texture and vertex colour is performed */
CC_API void Gfx_SetAlphaArgBlend(cc_bool enabled);

/* Sets whether pixels may be discard based on z/depth */
CC_API void Gfx_SetDepthTest(cc_bool enabled);
/* Sets whether z/depth of pixels is actually written to the depth buffer */
CC_API void Gfx_SetDepthWrite(cc_bool enabled);
/* Sets whether R/G/B/A of pixels are actually written to the colour buffer channels */
CC_API void Gfx_SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a);
/* Sets whether the game should only write output to depth buffer */
/*  NOTE: Implicitly calls Gfx_SetColorWrite */
CC_API void Gfx_DepthOnlyRendering(cc_bool depthOnly);


/*########################################################################################################################*
*------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
/*
SUMMARY:
	Index buffers are used to select the triangle vertices from the active vertex buffer
	  when rendering using the Gfx_DrawVb_IndexedTris/Gfx_DrawVb_IndexedTris_Range APIs
IMPLEMENTATION NOTES:
	All of the OpenGL and Direct3D rendering backends fully support index buffers
	However, most console ports rendering backends do not support index buffers
USAGE NOTES:
	The default index buffer selects groups of 4 vertices (1 quad) to produce 2 triangles
	  ( (0,1,2) (2,3,0)  (4,5,6) (6,7,4)  etc)
	ClassiCube itself never uses a different index buffer, as it draws everything using quads
	However, plugins might alter the index buffer to draw geometry that doesn't use quads
*/

/* Callback function to initialise/fill out the contents of an index buffer */
typedef void (*Gfx_FillIBFunc)(cc_uint16* indices, int count, void* obj);
/* Creates a new index buffer and fills out its contents */
CC_API GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj);
/* Sets the currently active index buffer */
CC_API void Gfx_BindIb(GfxResourceID ib);
/* Deletes the given index buffer, then sets it to 0 */
CC_API void Gfx_DeleteIb(GfxResourceID* ib);


/*########################################################################################################################*
*------------------------------------------------------Vertex buffers-----------------------------------------------------*
*#########################################################################################################################*/
/*
SUMMARY:
	Vertex buffers are used to store a group of vertices which can then latered be rendered
IMPLEMENTATION NOTES:
	Vertex buffers may be stored in VRAM or converted into a more efficient internal format
	For example, the PS1 and DS ports convert XYZ floats into fixed point integers
USAGE NOTES:
	Static vertex buffers should be used for data that very rarely changes
	Dynamic vertex buffers should be used for frequently changing data
*/
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


/*########################################################################################################################*
*------------------------------------------------------Vertex drawing-----------------------------------------------------*
*#########################################################################################################################*/
/*
SUMMARY:
	Vertex drawing functions draw lines or triangles from the currently active vertex buffer
IMPLEMENTATION NOTES:
	Not all rendering backends support lines
	Some rendering backends will always draw quads for the triangle drawing functions
USAGE NOTES:
	The appropriate vertex format must be set before drawing/rendering
	With the triangle drawing functions, the default bound index buffer
	  is setup to draw groups of 2 triangles from 4 vertices (1 quad)
*/

typedef enum DrawHints_ {
	DRAW_HINT_NONE   = 0,
	DRAW_HINT_SPRITE = 9,
} DrawHints;

/* Sets the format of the rendered vertices */
CC_API void Gfx_SetVertexFormat(VertexFormat fmt);
/* Renders vertices from the currently bound vertex buffer as lines */
CC_API void Gfx_DrawVb_Lines(int verticesCount);
/* Renders vertices from the currently bound vertex and index buffer as triangles */
/* NOTE: Offsets each index by startVertex */
CC_API void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints);
/* Renders vertices from the currently bound vertex and index buffer as triangles */
CC_API void Gfx_DrawVb_IndexedTris(int verticesCount);
/* Special case Gfx_DrawVb_IndexedTris_Range for map renderer */
void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex);


/*########################################################################################################################*
*-----------------------------------------------------Vertex transform----------------------------------------------------*
*#########################################################################################################################*/
/*
SUMMARY:
	The vertex transform pipeline transforms 3D coordinates into 2D window coordinates
	The vertex transform pipeline consists of the following stages
	- transform by model matrix
	- transform by view matrix
	- transform by projection matrix
	- transform by viewport
IMPLEMENTATION NOTES:
	The model and view matrices are combined into one matrix
	The combined matrix can be calculated by matrix multiplication
USAGE NOTES:
	Most rendering code does not alter vertex transform matrices.
	Some examples of code that does however:
	- entities use custom model matrices
	- sky may use a model matrix to transform coordinates upwards when you fly into the sky
	  (this avoids needing to recreate the vertex buffer in this case)
	- skybox uses a custom view matrix so it is always drawn at a constant distance
	Altering the viewport (and scissor) changes which part of the window
	  that everything is rendered to. It can be used for e.g. splitscreen mode
*/

typedef enum MatrixType_ {
	MATRIX_PROJ, /* Projection matrix */
	MATRIX_VIEW  /* Combined model view matrix */
} MatrixType;

/* Sets the currently active matrix projection or modelview matrix */
CC_API void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix);
/* Sets the texture U/V translation (not normally used) */
CC_API void Gfx_EnableTextureOffset(float x, float y);
/* Disables texture U/V translation */
CC_API void Gfx_DisableTextureOffset(void);
/* Loads given modelview and projection matrices, then calculates the combined MVP matrix */
void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp);

/* Calculates an orthographic projection matrix suitable with this backend. (usually for 2D) */
void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar);
/* Calculates a perspective projection matrix suitable with this backend. (usually for 3D) */
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar);
/* NOTE: The projection matrix calculation can slightly vary depending on the graphics backend */
/*  (e.g. OpenGL uses a Z clip space range of [-1, 1], whereas Direct3D9 uses [0, 1]) */

/* Sets the region where transformed vertices are drawn in */
/*  By default this region has origin 0,0 and size is window width/height */
/*  This region should normally be the same as the scissor region */
CC_API void Gfx_SetViewport(int x, int y, int w, int h);
/* Sets the region where pixels can be drawn in (pixels outside this region are discarded) */
/*  By default this region has origin 0,0 and size is window width/height */
/*  This region should normally be the same as the viewport region */
CC_API void Gfx_SetScissor (int x, int y, int w, int h);


/*########################################################################################################################*
*------------------------------------------------------Misc utilities-----------------------------------------------------*
*#########################################################################################################################*/
/* Outputs a .png screenshot of the backbuffer */
cc_result Gfx_TakeScreenshot(struct Stream* output);
/* Warns in chat if the graphics backend has problems with the user's GPU */
/* Returns whether legacy rendering mode for borders/sky/clouds is needed */
cc_bool Gfx_WarnIfNecessary(void);
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s);
/* Gets information about the user's GPU and graphics backend state */
/* Backend state may include depth buffer bits, total free memory, etc */
/* NOTE: Each line is separated by \n */
void Gfx_GetApiInfo(cc_string* info);

/* Updates state when the window's dimensions have changed */
/* NOTE: This may require recreating the context depending on the backend */
void Gfx_OnWindowResize(void);

/* Anaglyph 3D rendering support */
void Gfx_Set3DLeft( struct Matrix* proj, struct Matrix* view);
void Gfx_Set3DRight(struct Matrix* proj, struct Matrix* view);
void Gfx_End3D(     struct Matrix* proj, struct Matrix* view);

/* Raises ContextLost event and updates state for lost contexts */
void Gfx_LoseContext(const char* reason);
/* Raises ContextRecreated event and restores internal state */
void Gfx_RecreateContext(void);
/* Attempts to restore a lost context */
cc_bool Gfx_TryRestoreContext(void);

/* Sets appropriate alpha test/blending for given block draw type */
void Gfx_SetupAlphaState(cc_uint8 draw);
/* Undoes changes to alpha test/blending state by Gfx_SetupAlphaState */
void Gfx_RestoreAlphaState(cc_uint8 draw);


/*########################################################################################################################*
*------------------------------------------------------2D rendering------------------------------------------------------*
*#########################################################################################################################*/
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
