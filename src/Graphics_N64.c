#include "Core.h"
#ifdef CC_BUILD_N64
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"
#include <libdragon.h>
#include <GL/gl.h>
#include <GL/gl_integration.h>

typedef void (*GL_SetupVBFunc)(void);
typedef void (*GL_SetupVBRangeFunc)(int startVertex);
static GL_SetupVBFunc gfx_setupVBFunc;
static GL_SetupVBRangeFunc gfx_setupVBRangeFunc;


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
static void GL_UpdateVsync(void) {
	//GLContext_SetFpsLimit(gfx_vsync, gfx_minFrameMs);
}
static surface_t zbuffer;

void Gfx_Create(void) {
    gl_init();
    //rdpq_debug_start(); // TODO debug
    //rdpq_debug_log(true);
    zbuffer = surface_alloc(FMT_RGBA16, display_get_width(), display_get_height());
    
	Gfx.MaxTexWidth  = 256;
	Gfx.MaxTexHeight = 256;
	Gfx.Created      = true;
	
	// TMEM only has 4 KB in it, which can be interpreted as
	// - 1024 32bpp pixels
	// - 2048 16bpp pixels 
	Gfx.MaxTexSize       = 1024;
	Gfx.MaxLowResTexSize = 2048;

	Gfx.SupportsNonPowTwoTextures = true;
	Gfx_RestoreState();
	GL_UpdateVsync();
}

cc_bool Gfx_TryRestoreContext(void) {
	return true;
}

void Gfx_Free(void) {
	Gfx_FreeState();
	gl_close();
}

#define gl_Toggle(cap) if (enabled) { glEnable(cap); } else { glDisable(cap); }


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
static color_t gfx_clearColor;

cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using Nintendo 64 --\n");
	String_AppendConst(info, "GPU: Nintendo 64 RDP (LibDragon OpenGL)\n");
	String_AppendConst(info, "T&L: Nintendo 64 RSP (LibDragon OpenGL)\n");
	PrintMaxTextureInfo(info);
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
	if (Gfx.Created) GL_UpdateVsync();
}

void Gfx_OnWindowResize(void) { }


void Gfx_BeginFrame(void) {
	surface_t* disp = display_get();
    rdpq_attach(disp, &zbuffer);
    
	gl_context_begin();
	Platform_LogConst("GFX ctx beg");
}

static color_t gfx_clearColor;
void Gfx_Clear(void) {
	__rdpq_autosync_change(AUTOSYNC_PIPE);
	rdpq_clear(gfx_clearColor);
    rdpq_clear_z(0xFFFC);
} 

void Gfx_ClearCol(PackedCol color) {
	gfx_clearColor.r = PackedCol_R(color);
	gfx_clearColor.g = PackedCol_G(color);
	gfx_clearColor.b = PackedCol_B(color);
	gfx_clearColor.a = PackedCol_A(color);
	//memcpy(&gfx_clearColor, &color, sizeof(color_t)); // TODO maybe use proper conversion from graphics.h ??
}

void Gfx_EndFrame(void) {
	Platform_LogConst("GFX ctx end");
	gl_context_end();
    rdpq_detach_show();
    
	if (gfx_minFrameMs) LimitFPS();
//Platform_LogConst("GFX END");
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
typedef struct CCTexture {
	surface_t surface;
	GLuint textureID;
} CCTexture;

#define ALIGNUP8(size) (((size) + 7) & ~0x07)

// A8 B8 G8 R8 > A1 B5 G5 B5
#define To16BitPixel(src) \
	((src & 0x80) >> 7) | ((src & 0xF800) >> 10) | ((src & 0xF80000) >> 13) | ((src & 0xF8000000) >> 16);	

static GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	cc_bool bit16  = flags & TEXTURE_FLAG_LOWRES;
	// rows are actually 8 byte aligned in TMEM https://github.com/DragonMinded/libdragon/blob/f360fa1bb1fb3ff3d98f4ab58692d40c828636c9/src/rdpq/rdpq_tex.c#L132
	// so even though width * height * pixel size may fit within 4096 bytes, after adjusting for 8 byte alignment, row pitch * height may exceed 4096 bytes
	int pitch = bit16 ? ALIGNUP8(bmp->width * 2) : ALIGNUP8(bmp->width * 4);
	if (pitch * bmp->height > 4096) return 0;
	
	CCTexture* tex = Mem_Alloc(1, sizeof(CCTexture), "texture");
	
	glGenTextures(1, &tex->textureID);
	glBindTexture(GL_TEXTURE_2D, tex->textureID);
	// NOTE: Enabling these fixes textures, but seems to break on cen64
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	tex->surface   = surface_alloc(bit16 ? FMT_RGBA16 : FMT_RGBA32, bmp->width, bmp->height);
	surface_t* fb  = &tex->surface;
	cc_uint32* src = (cc_uint32*)bmp->scan0;
	cc_uint8*  dst = (cc_uint8*)fb->buffer;
		
	if (bit16) {
		// 16 bpp requires reducing A8R8G8B8 to A1R5G5B5
		for (int y = 0; y < bmp->height; y++) 
		{	
			cc_uint32* src_row = src + y * bmp->width;
			cc_uint16* dst_row = (cc_uint16*)(dst + y * fb->stride);
			
			for (int x = 0; x < bmp->width; x++) 
			{
				dst_row[x] = To16BitPixel(src_row[x]);
			}
		}
	} else {
		// 32 bpp can just be copied straight across
		for (int y = 0; y < bmp->height; y++) 
		{
			Mem_Copy(dst + y * fb->stride,
				 	src + y * bmp->width,
				 	bmp->width * 4);
		}
	}
	
	
	rdpq_texparms_t params =
	{
        .s.repeats = (flags & TEXTURE_FLAG_NONPOW2) ? 1 : REPEAT_INFINITE,
        .t.repeats = (flags & TEXTURE_FLAG_NONPOW2) ? 1 : REPEAT_INFINITE,
    };

	// rdpq_tex_upload(TILE0, &tex->surface, &params);
	glSurfaceTexImageN64(GL_TEXTURE_2D, 0, fb, &params);
	return tex;
}

void Gfx_BindTexture(GfxResourceID texId) {
	CCTexture* tex = (CCTexture*)texId;
	GLuint glID = tex ? tex->textureID : 0;
	//Platform_Log1("BIND: %i", &glID);
	
    //rdpq_debug_log(true);
	glBindTexture(GL_TEXTURE_2D, glID);	
   // rdpq_debug_log(false);
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	// TODO: Just memcpying doesn't actually work. maybe due to glSurfaceTexImageN64 caching the RSQ upload block?
	// TODO: Is there a more optimised approach than just calling glSurfaceTexImageN64
	CCTexture* tex = (CCTexture*)texId;
	
	surface_t* fb  = &tex->surface;
	cc_uint32* src = (cc_uint32*)part->scan0 + x;
	cc_uint8*  dst = (cc_uint8*)fb->buffer  + (x * 4) + (y * fb->stride);

	for (int srcY = 0; srcY < part->height; srcY++) 
	{
		Mem_Copy(dst + srcY * fb->stride,
				 src + srcY * rowWidth,
				 part->width * 4);
	}
	
	
	glBindTexture(GL_TEXTURE_2D, tex->textureID);
	rdpq_texparms_t params = (rdpq_texparms_t){
        .s.repeats = REPEAT_INFINITE,
        .t.repeats = REPEAT_INFINITE,
    };
	// rdpq_tex_upload(TILE0, &tex->surface, &params);
	glSurfaceTexImageN64(GL_TEXTURE_2D, 0, fb, &params);
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
	Gfx_UpdateTexture(texId, x, y, part, part->width, mipmaps);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	CCTexture* tex = (CCTexture*)(*texId);
	if (!tex) return;
	
	glDeleteTextures(1, &tex->textureID);
	Mem_Free(tex);
	*texId = NULL;
}

void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFaceCulling(cc_bool enabled)   { gl_Toggle(GL_CULL_FACE); }
void Gfx_SetAlphaBlending(cc_bool enabled) { gl_Toggle(GL_BLEND); }
void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

void Gfx_SetColWriteMask(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	//glColorMask(r, g, b, a); TODO
}

void Gfx_SetDepthWrite(cc_bool enabled) { glDepthMask(enabled); }
void Gfx_SetDepthTest(cc_bool enabled) { gl_Toggle(GL_DEPTH_TEST); }

static void Gfx_FreeState(void) { FreeDefaultResources(); }
static void Gfx_RestoreState(void) {
	InitDefaultResources();
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	gfx_format = -1;

	glHint(GL_FOG_HINT, GL_NICEST);
	glAlphaFunc(GL_GREATER, 0.5f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LESS);
	//glEnable(GL_RDPQ_TEXTURING_N64);
}

cc_bool Gfx_WarnIfNecessary(void) { return false; }


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	/* Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glortho */
	/*   The simplified calculation below uses: L = 0, R = width, T = 0, B = height */
	*matrix = Matrix_Identity;

	matrix->row1.x =  2.0f / width;
	matrix->row2.y = -2.0f / height;
	matrix->row3.z = -2.0f / (zFar - zNear);

	matrix->row4.x = -1.0f;
	matrix->row4.y =  1.0f;
	matrix->row4.z = -(zFar + zNear) / (zFar - zNear);
}

static double Cotangent(double x) { return Math_Cos(x) / Math_Sin(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	float zNear = 0.1f;
	float c = (float)Cotangent(0.5f * fov);

	/* Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glfrustum */
	/* For a FOV based perspective matrix, left/right/top/bottom are calculated as: */
	/*   left = -c * aspect, right = c * aspect, bottom = -c, top = c */
	/* Calculations are simplified because of left/right and top/bottom symmetry */
	*matrix = Matrix_Identity;

	matrix->row1.x =  c / aspect;
	matrix->row2.y =  c;
	matrix->row3.z = -(zFar + zNear) / (zFar - zNear);
	matrix->row3.w = -1.0f;
	matrix->row4.z = -(2.0f * zFar * zNear) / (zFar - zNear);
	matrix->row4.w =  0.0f;
}


/*########################################################################################################################*
*----------------------------------------------------------Buffers--------------------------------------------------------*
*#########################################################################################################################*/
static cc_uint16 __attribute__((aligned(16))) gfx_indices[GFX_MAX_INDICES];
static void* gfx_vertices;
static int vb_size;

GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	fillFunc(gfx_indices, count, obj);
	return (void*)1;
}

void Gfx_BindIb(GfxResourceID ib)    { }
void Gfx_DeleteIb(GfxResourceID* ib) { }


static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	return Mem_TryAlloc(count, strideSizes[fmt]);
}

void Gfx_BindVb(GfxResourceID vb) { gfx_vertices = vb; }

void Gfx_DeleteVb(GfxResourceID* vb) {
	GfxResourceID data = *vb;
	if (data) Mem_Free(data);
	*vb = 0;
}

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	vb_size = count * strideSizes[fmt];
	return vb;
}

void Gfx_UnlockVb(GfxResourceID vb) { 
	gfx_vertices = vb;
}


static GfxResourceID Gfx_AllocDynamicVb(VertexFormat fmt, int maxVertices) {
	return Mem_TryAlloc(maxVertices, strideSizes[fmt]);
}

void Gfx_BindDynamicVb(GfxResourceID vb) { Gfx_BindVb(vb); }

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	vb_size = count * strideSizes[fmt];
	return vb; 
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) { 
	gfx_vertices = vb;
}

void Gfx_DeleteDynamicVb(GfxResourceID* vb) { Gfx_DeleteVb(vb); }


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static cc_bool depthOnlyRendering;

void Gfx_SetFog(cc_bool enabled) {
}

void Gfx_SetFogCol(PackedCol color) {
}

void Gfx_SetFogDensity(float value) {
}

void Gfx_SetFogEnd(float value) {
}

void Gfx_SetFogMode(FogFunc func) {
}

void Gfx_SetTexturing(cc_bool enabled) { }

void Gfx_SetAlphaTest(cc_bool enabled) { 
	if (enabled) { glEnable(GL_ALPHA_TEST); } else { glDisable(GL_ALPHA_TEST); }
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	depthOnlyRendering = depthOnly; // TODO: Better approach? maybe using glBlendFunc instead?
	cc_bool enabled    = !depthOnly;
	//Gfx_SetColWriteMask(enabled, enabled, enabled, enabled);
	if (enabled) { glEnable(GL_TEXTURE_2D); } else { glDisable(GL_TEXTURE_2D); }
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static GLenum matrix_modes[3] = { GL_PROJECTION, GL_MODELVIEW, GL_TEXTURE };
static int lastMatrix;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type != lastMatrix) { lastMatrix = type; glMatrixMode(matrix_modes[type]); }
	glLoadMatrixf((const float*)matrix);
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
	if (type != lastMatrix) { lastMatrix = type; glMatrixMode(matrix_modes[type]); }
	glLoadIdentity();
}

static struct Matrix texMatrix = Matrix_IdentityValue;
void Gfx_EnableTextureOffset(float x, float y) {
	texMatrix.row4.x = x; texMatrix.row4.y = y;
	Gfx_LoadMatrix(2, &texMatrix);
}

void Gfx_DisableTextureOffset(void) { Gfx_LoadIdentityMatrix(2); }


/*########################################################################################################################*
*--------------------------------------------------------Rendering--------------------------------------------------------*
*#########################################################################################################################*/
#define VB_PTR gfx_vertices
#define IB_PTR gfx_indices

static void GL_SetupVbColoured(void) {
	glVertexPointer(3, GL_FLOAT,        SIZEOF_VERTEX_COLOURED, (void*)(VB_PTR + 0));
	glColorPointer(4, GL_UNSIGNED_BYTE, SIZEOF_VERTEX_COLOURED, (void*)(VB_PTR + 12));
}

static void GL_SetupVbTextured(void) {
	glVertexPointer(3, GL_FLOAT,        SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + 0));
	glColorPointer(4, GL_UNSIGNED_BYTE, SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + 12));
	glTexCoordPointer(2, GL_FLOAT,      SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + 16));
}

static void GL_SetupVbColoured_Range(int startVertex) {
	cc_uint32 offset = startVertex * SIZEOF_VERTEX_COLOURED;
	glVertexPointer(3, GL_FLOAT,          SIZEOF_VERTEX_COLOURED, (void*)(VB_PTR + offset));
	glColorPointer(4, GL_UNSIGNED_BYTE,   SIZEOF_VERTEX_COLOURED, (void*)(VB_PTR + offset + 12));
}

static void GL_SetupVbTextured_Range(int startVertex) {
	cc_uint32 offset = startVertex * SIZEOF_VERTEX_TEXTURED;
	glVertexPointer(3,  GL_FLOAT,         SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset));
	glColorPointer(4, GL_UNSIGNED_BYTE,   SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset + 12));
	glTexCoordPointer(2, GL_FLOAT,        SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset + 16));
}

void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];

	if (fmt == VERTEX_FORMAT_TEXTURED) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);

		gfx_setupVBFunc      = GL_SetupVbTextured;
		gfx_setupVBRangeFunc = GL_SetupVbTextured_Range;
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);

		gfx_setupVBFunc      = GL_SetupVbColoured;
		gfx_setupVBRangeFunc = GL_SetupVbColoured_Range;
	}
}

void Gfx_DrawVb_Lines(int verticesCount) {
	gfx_setupVBFunc();
	glDrawArrays(GL_LINES, 0, verticesCount);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	gfx_setupVBRangeFunc(startVertex);
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, IB_PTR);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	gfx_setupVBFunc();
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, IB_PTR);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	cc_uint32 offset = startVertex * SIZEOF_VERTEX_TEXTURED;
	if (depthOnlyRendering) return;
	glVertexPointer(3, GL_FLOAT,        SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset));
	glColorPointer(4, GL_UNSIGNED_BYTE, SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset + 12));
	glTexCoordPointer(2, GL_FLOAT,      SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset + 16));
	glDrawElements(GL_TRIANGLES,        ICOUNT(verticesCount),   GL_UNSIGNED_SHORT, IB_PTR);
}
#endif
