#include "Core.h"
#if defined CC_BUILD_PS3
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"
#include <malloc.h>
#include <rsx/rsx.h>
#include <sysutil/video.h>

/* Current format and size of vertices */
static int gfx_stride, gfx_format = -1;
static cc_bool renderingDisabled;
static gcmContextData* context;
static u32 cur_fb = 0;

#define CB_SIZE   0x100000 // TODO: smaller command buffer?
#define HOST_SIZE (32 * 1024 * 1024)


/*########################################################################################################################*
*---------------------------------------------------------- setup---------------------------------------------------------*
*#########################################################################################################################*/
static u32  color_pitch;
static u32  color_offset[2];
static u32* color_buffer[2];

static u32  depth_pitch;
static u32  depth_offset;
static u32* depth_buffer;

static void Gfx_FreeState(void) { FreeDefaultResources(); }
static void Gfx_RestoreState(void) {
	InitDefaultResources();
	gfx_format = -1;/* TODO */
	
	rsxSetColorMaskMrt(context, 0);
	rsxSetDepthFunc(context, GCM_LEQUAL);
	rsxSetClearDepthStencil(context, 0xFFFFFF);
}

static void CreateContext(void) {
	void* host_addr = memalign(1024 * 1024, HOST_SIZE);
	rsxInit(&context, CB_SIZE, HOST_SIZE, host_addr);
}

static void ConfigureVideo(void) {
	videoState state;
	videoGetState(0, 0, &state);

	videoConfiguration vconfig = { 0 };
	vconfig.resolution = state.displayMode.resolution;
	vconfig.format     = VIDEO_BUFFER_FORMAT_XRGB;
	vconfig.pitch      = DisplayInfo.Width * sizeof(u32);
	
	videoConfigure(0, &vconfig, NULL, 0);
}

static void SetupBlendingState(void) {
	rsxSetBlendFunc(context, GCM_SRC_ALPHA, GCM_ONE_MINUS_SRC_ALPHA, GCM_SRC_ALPHA, GCM_ONE_MINUS_SRC_ALPHA);
	rsxSetBlendEquation(context, GCM_FUNC_ADD, GCM_FUNC_ADD);
}

static void AllocColorSurface(u32 i) {
	color_pitch     = DisplayInfo.Width * 4;
	color_buffer[i] = (u32*)rsxMemalign(64, DisplayInfo.Height * color_pitch);
	
	rsxAddressToOffset(color_buffer[i], &color_offset[i]);
	gcmSetDisplayBuffer(i, color_offset[i], color_pitch,
		DisplayInfo.Width, DisplayInfo.Height);
}

static void AllocDepthSurface(void) {
	depth_pitch  = DisplayInfo.Width * 4;
	depth_buffer = (u32*)rsxMemalign(64, DisplayInfo.Height * depth_pitch);
	
	rsxAddressToOffset(depth_buffer, &depth_offset);
}


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
void SetRenderTarget(u32 index) {
	gcmSurface sf;

	sf.colorFormat		= GCM_SURFACE_X8R8G8B8;
	sf.colorTarget		= GCM_SURFACE_TARGET_0;
	sf.colorLocation[0]	= GCM_LOCATION_RSX;
	sf.colorOffset[0]	= color_offset[index];
	sf.colorPitch[0]	= color_pitch;

	sf.colorLocation[1]	= GCM_LOCATION_RSX;
	sf.colorLocation[2]	= GCM_LOCATION_RSX;
	sf.colorLocation[3]	= GCM_LOCATION_RSX;
	sf.colorOffset[1]	= 0;
	sf.colorOffset[2]	= 0;
	sf.colorOffset[3]	= 0;
	sf.colorPitch[1]	= 64;
	sf.colorPitch[2]	= 64;
	sf.colorPitch[3]	= 64;

	sf.depthFormat		= GCM_SURFACE_ZETA_Z24S8;
	sf.depthLocation	= GCM_LOCATION_RSX;
	sf.depthOffset		= depth_offset;
	sf.depthPitch		= depth_pitch;

	sf.type		= GCM_SURFACE_TYPE_LINEAR;
	sf.antiAlias		= GCM_SURFACE_CENTER_1;

	sf.width		= DisplayInfo.Width;
	sf.height		= DisplayInfo.Height;
	sf.x			= 0;
	sf.y			= 0;

	rsxSetSurface(context,&sf);
}

void Gfx_Create(void) {
	// TODO rethink all this
	if (Gfx.Created) return;
	Gfx.MaxTexWidth  = 1024;
	Gfx.MaxTexHeight = 1024;
	Gfx.Created      = true;
	
	// https://github.com/ps3dev/PSL1GHT/blob/master/ppu/include/rsx/rsx.h#L30
	CreateContext();
	ConfigureVideo();
	gcmSetFlipMode(GCM_FLIP_VSYNC);
	
	AllocColorSurface(0);
	AllocColorSurface(1);
	AllocDepthSurface();
	gcmResetFlipStatus();
	
	SetupBlendingState();
	Gfx_RestoreState();
	SetRenderTarget(cur_fb);
}

cc_bool Gfx_TryRestoreContext(void) { return true; }

cc_bool Gfx_WarnIfNecessary(void) { return false; }

void Gfx_Free(void) {
	Gfx_FreeState();
}

u32* Gfx_AllocImage(u32* offset, s32 w, s32 h) {
	u32* pixels = (u32*)rsxMemalign(64, w * h * 4);
	rsxAddressToOffset(pixels, offset);
	return pixels;
}

void Gfx_TransferImage(u32 offset, s32 w, s32 h) {
	rsxSetTransferImage(context, GCM_TRANSFER_LOCAL_TO_LOCAL,
		color_offset[cur_fb], color_pitch, -w/2, -h/2,
		offset, w * 4, 0, 0, w, h, 4);
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gfx_clearColor;
void Gfx_SetFaceCulling(cc_bool enabled) {
	rsxSetCullFaceEnable(context, enabled);
}

void Gfx_SetAlphaBlending(cc_bool enabled) {
	rsxSetBlendEnable(context, enabled);
}
void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

void Gfx_ClearCol(PackedCol color) {
        rsxSetClearColor(context, color);
}

void Gfx_SetColWriteMask(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	unsigned mask = 0;
	if (r) mask |= GCM_COLOR_MASK_R;
	if (g) mask |= GCM_COLOR_MASK_G;
	if (b) mask |= GCM_COLOR_MASK_B;
	if (a) mask |= GCM_COLOR_MASK_A;

	rsxSetColorMask(context, mask);
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	rsxSetDepthWriteEnable(context, enabled);
}

void Gfx_SetDepthTest(cc_bool enabled) {
	rsxSetDepthTestEnable(context, enabled);
}

void Gfx_SetTexturing(cc_bool enabled) { }

void Gfx_SetAlphaTest(cc_bool enabled) { /* TODO */ }

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {/* TODO */
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	/* Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glortho */
	/*   The simplified calculation below uses: L = 0, R = width, T = 0, B = height */
	*matrix = Matrix_Identity;

	matrix->row1.X =  2.0f / width;
	matrix->row2.Y = -2.0f / height;
	matrix->row3.Z = -2.0f / (zFar - zNear);

	matrix->row4.X = -1.0f;
	matrix->row4.Y =  1.0f;
	matrix->row4.Z = -(zFar + zNear) / (zFar - zNear);
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

	matrix->row1.X =  c / aspect;
	matrix->row2.Y =  c;
	matrix->row3.Z = -(zFar + zNear) / (zFar - zNear);
	matrix->row3.W = -1.0f;
	matrix->row4.Z = -(2.0f * zFar * zNear) / (zFar - zNear);
	matrix->row4.W =  0.0f;
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_GetApiInfo(cc_string* info) {
	int pointerSize = sizeof(void*) * 8;

	String_Format1(info, "-- Using PS3 (%i bit) --\n", &pointerSize);
	String_Format2(info, "Max texture size: (%i, %i)\n", &Gfx.MaxTexWidth, &Gfx.MaxTexHeight);
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
}


// https://github.com/ps3dev/PSL1GHT/blob/master/ppu/include/rsx/rsx.h#L30
static cc_bool everFlipped;
void Gfx_BeginFrame(void) {
	// TODO: remove everFlipped
	if (everFlipped) {
		while (gcmGetFlipStatus() != 0) usleep(200);
	}
	
	everFlipped = true;
	gcmResetFlipStatus();
}

void Gfx_Clear(void) {
	rsxClearSurface(context, GCM_CLEAR_R | GCM_CLEAR_G | GCM_CLEAR_B | GCM_CLEAR_A 
		| GCM_CLEAR_S | GCM_CLEAR_Z);
}

void Gfx_EndFrame(void) {
	gcmSetFlip(context, cur_fb);
	rsxFlushBuffer(context);
	gcmSetWaitFlip(context);

	cur_fb ^= 1;
	SetRenderTarget(cur_fb);
	
	if (gfx_minFrameMs) LimitFPS();
}

void Gfx_OnWindowResize(void) {/* TODO */
}


/*########################################################################################################################*
*-------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
static void* gfx_vertices;
static int vb_size;

GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	return 1;/* TODO */
}

void Gfx_BindIb(GfxResourceID ib) { }
void Gfx_DeleteIb(GfxResourceID* ib) { }


/*########################################################################################################################*
*------------------------------------------------------Vertex buffers-----------------------------------------------------*
*#########################################################################################################################*/
GfxResourceID Gfx_CreateVb(VertexFormat fmt, int count) {
	void* data = memalign(16, count * strideSizes[fmt]);
	if (!data) Logger_Abort("Failed to allocate memory for GFX VB");
	return data;
	//return Mem_Alloc(count, strideSizes[fmt], "gfx VB");/* TODO */
}

void Gfx_BindVb(GfxResourceID vb) { gfx_vertices = vb;/* TODO */ }

void Gfx_DeleteVb(GfxResourceID* vb) {
	GfxResourceID data = *vb;/* TODO */
	if (data) Mem_Free(data);
	*vb = 0;
}

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	vb_size = count * strideSizes[fmt];/* TODO */
	return vb;
}

void Gfx_UnlockVb(GfxResourceID vb) { 
	gfx_vertices = vb;/* TODO */
	//sceKernelDcacheWritebackInvalidateRange(vb, vb_size);
}


GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices) {
	void* data = memalign(16, maxVertices * strideSizes[fmt]);
	if (!data) Logger_Abort("Failed to allocate memory for GFX VB");
	return data;/* TODO */
	//return Mem_Alloc(maxVertices, strideSizes[fmt], "gfx VB");
}

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	vb_size = count * strideSizes[fmt];
	return vb; /* TODO */
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) { 
	gfx_vertices = vb; /* TODO */
	//dcache_flush_range(vb, vb_size);
}

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	gfx_vertices = vb;/* TODO */
	Mem_Copy(vb, vertices, vCount * gfx_stride);
	//dcache_flush_range(vertices, vCount * gfx_stride);
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_BindTexture(GfxResourceID texId) {
	/* TODO */
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	/* TODO */
}

void Gfx_EnableMipmaps(void)  { }
void Gfx_DisableMipmaps(void) { }

GfxResourceID Gfx_CreateTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	return 1;/* TODO */
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
/* TODO */
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
	Gfx_UpdateTexture(texId, x, y, part, part->width, mipmaps);
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gfx_fogColor;
static float gfx_fogEnd = 16.0f, gfx_fogDensity = 1.0f;
static FogFunc gfx_fogMode = -1;

void Gfx_SetFog(cc_bool enabled) {/* TODO */
}

void Gfx_SetFogCol(PackedCol color) {/* TODO */
}

void Gfx_SetFogDensity(float value) {/* TODO */
}

void Gfx_SetFogEnd(float value) {/* TODO */
}

void Gfx_SetFogMode(FogFunc func) {/* TODO */
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
/* TODO */
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
	Gfx_LoadMatrix(type, &Matrix_Identity);
}


void Gfx_EnableTextureOffset(float x, float y) {
/* TODO */
}

void Gfx_DisableTextureOffset(void) {
/* TODO */
}


/*########################################################################################################################*
*----------------------------------------------------------Drawing--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];/* TODO */
}

void Gfx_DrawVb_Lines(int verticesCount) {/* TODO */
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {/* TODO */
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {/* TODO */
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {/* TODO */
}
#endif