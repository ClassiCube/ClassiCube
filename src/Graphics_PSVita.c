#include "Core.h"
#if defined CC_BUILD_PSVITA
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"
#include <vitasdk.h>
/* Current format and size of vertices */
static int gfx_stride, gfx_format = -1;

#define DISPLAY_WIDTH   960
#define DISPLAY_HEIGHT  544
#define DISPLAY_STRIDE 1024

#define NUM_DISPLAY_BUFFERS 2
#define MAX_PENDING_SWAPS   (NUM_DISPLAY_BUFFERS - 1)

static SceGxmContext* gxm_context;

static SceUID vdm_ring_buffer_uid;
static void*  vdm_ring_buffer_addr;
static SceUID vertex_ring_buffer_uid;
static void*  vertex_ring_buffer_addr;
static SceUID fragment_ring_buffer_uid;
static void*  fragment_ring_buffer_addr;
static SceUID fragment_usse_ring_buffer_uid;
static void*  fragment_usse_ring_buffer_addr;
static unsigned int fragment_usse_offset;

static SceGxmRenderTarget* gxm_render_target;
static SceGxmColorSurface gxm_color_surfaces[NUM_DISPLAY_BUFFERS];
static SceUID gxm_color_surfaces_uid[NUM_DISPLAY_BUFFERS];
static void*  gxm_color_surfaces_addr[NUM_DISPLAY_BUFFERS];
static SceGxmSyncObject* gxm_sync_objects[NUM_DISPLAY_BUFFERS];

static SceUID gxm_depth_stencil_surface_uid;
static void*  gxm_depth_stencil_surface_addr;
static SceGxmDepthStencilSurface gxm_depth_stencil_surface;

/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
#define ALIGNUP(size, a) (((size) + ((a) - 1)) & ~((a) - 1))

void* AllocGPUMemory(int size, int type, int gpu_access, SceUID* ret_uid) {
	SceUID uid;
	void* addr;
	
	if (type == SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW) {
		size = ALIGNUP(size, 256 * 1024);
	} else {
		size = ALIGNUP(size, 4 * 1024);
	}
	
	// https://wiki.henkaku.xyz/vita/SceSysmem
	uid = sceKernelAllocMemBlock("GPU memory", type, size, NULL);
	if (uid < 0) Logger_Abort2(uid, "Failed to allocate GPU memory block");
		
	int res1 = sceKernelGetMemBlockBase(uid, &addr);
	if (res1 < 0) Logger_Abort2(res1, "Failed to get base of GPU memory block");
		
	int res2 = sceGxmMapMemory(addr, size, gpu_access);
	if (res1 < 0) Logger_Abort2(res2, "Failed to map memory for GPU usage");
	// https://wiki.henkaku.xyz/vita/GPU
	
	*ret_uid = uid;
	return addr;
}

void* AllocGPUVertexUSSE(size_t size, SceUID* ret_uid, unsigned int* ret_usse_offset) {
	SceUID uid;
	void *addr;

	size = ALIGNUP(size, 4 * 1024);

	uid = sceKernelAllocMemBlock("GPU vertex USSE",
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, NULL);
	if (uid < 0) Logger_Abort2(uid, "Failed to allocate vertex USSE block");

	int res1 = sceKernelGetMemBlockBase(uid, &addr);
	if (res1 < 0) Logger_Abort2(res1, "Failed to get base of vertex USSE memory block");

	int res2 = sceGxmMapVertexUsseMemory(addr, size, ret_usse_offset);
	if (res1 < 0) Logger_Abort2(res2, "Failed to map vertex USSE memory");

	*ret_uid = uid;
	return addr;
}

void* AllocGPUFragmentUSSE(size_t size, SceUID* ret_uid, unsigned int* ret_usse_offset) {
	SceUID uid;
	void *addr;

	size = ALIGNUP(size, 4 * 1024);

	uid = sceKernelAllocMemBlock("GPU fragment USSE",
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, NULL);
	if (uid < 0) Logger_Abort2(uid, "Failed to allocate fragment USSE block");

	int res1 = sceKernelGetMemBlockBase(uid, &addr);
	if (res1 < 0) Logger_Abort2(res1, "Failed to get base of fragment USSE memory block");

	int res2 = sceGxmMapFragmentUsseMemory(addr, size, ret_usse_offset);
	if (res1 < 0) Logger_Abort2(res2, "Failed to map fragment USSE memory");

	*ret_uid = uid;
	return addr;
}


/*########################################################################################################################*
*-----------------------------------------------------Initialisation------------------------------------------------------*
*#########################################################################################################################*/
static void DisplayQueueCallback(const void *callback_data) {
	SceDisplayFrameBuf fb = { 0 };
	void* addr = *((void **)callback_data);

	fb.size   = sizeof(SceDisplayFrameBuf);
	fb.base   = addr;
	fb.pitch  = DISPLAY_STRIDE;
	fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
	fb.width  = DISPLAY_WIDTH;
	fb.height = DISPLAY_HEIGHT;

	sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
}

static void InitGXM(void) {
	SceGxmInitializeParams params = { 0 };
	
	params.displayQueueMaxPendingCount = MAX_PENDING_SWAPS;
	params.displayQueueCallback = DisplayQueueCallback;
	params.displayQueueCallbackDataSize = sizeof(void*);
	params.parameterBufferSize = SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE;
	
	sceGxmInitialize(&params);
}

static void AllocRingBuffers(void) {
	vdm_ring_buffer_addr = AllocGPUMemory(SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE,
			SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
			&vdm_ring_buffer_uid);

	vertex_ring_buffer_addr = AllocGPUMemory(SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE,
			SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
			&vertex_ring_buffer_uid);

	fragment_ring_buffer_addr = AllocGPUMemory(SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE,
			SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
			&fragment_ring_buffer_uid);

	fragment_usse_ring_buffer_addr = AllocGPUFragmentUSSE(SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE,
			&fragment_ring_buffer_uid, &fragment_usse_offset);
}

static void AllocGXMContext(void) {
	SceGxmContextParams params = { 0 };
	
	params.hostMem     = Mem_TryAlloc(SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE, 1);
	params.hostMemSize = SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE;
	
	params.vdmRingBufferMem     = vdm_ring_buffer_addr;
	params.vdmRingBufferMemSize = SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE;
	
	params.vertexRingBufferMem     = vertex_ring_buffer_addr;
	params.vertexRingBufferMemSize = SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE;
	
	params.fragmentRingBufferMem     = fragment_ring_buffer_addr;
	params.fragmentRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE;
	
	params.fragmentUsseRingBufferMem     = fragment_usse_ring_buffer_addr;
	params.fragmentUsseRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE;
	params.fragmentUsseRingBufferOffset  = fragment_usse_offset;

	sceGxmCreateContext(&params, &gxm_context);
}

static void AllocRenderTarget(void) {
	SceGxmRenderTargetParams params = { 0 };
	
	params.width  = DISPLAY_WIDTH;
	params.height = DISPLAY_HEIGHT;
	params.scenesPerFrame = 1;
	params.driverMemBlock = -1;

	sceGxmCreateRenderTarget(&params, &gxm_render_target);
}

static void AllocColorBuffer(int i) {
	int size = ALIGNUP(4 * DISPLAY_STRIDE * DISPLAY_HEIGHT, 1 * 1024 * 1024);
	
	gxm_color_surfaces_addr[i] = AllocGPUMemory(size, SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
									SCE_GXM_MEMORY_ATTRIB_RW, &gxm_color_surfaces_uid[i]);

	sceGxmColorSurfaceInit(&gxm_color_surfaces[i],
		SCE_GXM_COLOR_FORMAT_A8B8G8R8,
		SCE_GXM_COLOR_SURFACE_LINEAR,
		SCE_GXM_COLOR_SURFACE_SCALE_NONE,
		SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
		DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_STRIDE,
		gxm_color_surfaces_addr[i]);

	sceGxmSyncObjectCreate(&gxm_sync_objects[i]);
}

static void AllocDepthBuffer(void) {
	int width   = ALIGNUP(DISPLAY_WIDTH,  SCE_GXM_TILE_SIZEX);
	int height  = ALIGNUP(DISPLAY_HEIGHT, SCE_GXM_TILE_SIZEY);
	int samples = width * height;

	gxm_depth_stencil_surface_addr = AllocGPUMemory(4 * samples, SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
										SCE_GXM_MEMORY_ATTRIB_RW, &gxm_depth_stencil_surface_uid);

	sceGxmDepthStencilSurfaceInit(&gxm_depth_stencil_surface,
		SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24,
		SCE_GXM_DEPTH_STENCIL_SURFACE_TILED,
		width, gxm_depth_stencil_surface_addr, NULL);
}


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID white_square;
void Gfx_Create(void) {
	Gfx.MaxTexWidth  = 512;
	Gfx.MaxTexHeight = 512;
	Gfx.Created      = true;
	
	InitGXM();
	AllocRingBuffers();
	AllocGXMContext();
	
	AllocRenderTarget();
	for (int i = 0; i < NUM_DISPLAY_BUFFERS; i++) 
	{
		AllocColorBuffer(i);
	}
	AllocDepthBuffer();
	
	InitDefaultResources();
	
	// 1x1 dummy white texture
	struct Bitmap bmp;
	BitmapCol pixels[1] = { BITMAPCOLOR_WHITE };
	Bitmap_Init(bmp, 1, 1, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
	// TODO
}

void Gfx_Free(void) { 
	FreeDefaultResources(); 
	Gfx_DeleteTexture(&white_square);
}

cc_bool Gfx_TryRestoreContext(void) { return true; }
void Gfx_RestoreState(void) { }
void Gfx_FreeState(void) { }

/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
GfxResourceID Gfx_CreateTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	return (void*)1; // TODO
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	 // TODO
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
	Gfx_UpdateTexture(texId, x, y, part, part->width, mipmaps);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
 // TODO
	*texId = NULL;
}

void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }

void Gfx_BindTexture(GfxResourceID texId) {
 // TODO
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFaceCulling(cc_bool enabled)   { }  // TODO
void Gfx_SetAlphaBlending(cc_bool enabled) { } // TODO
void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

void Gfx_ClearCol(PackedCol color) {
 // TODO
}

void Gfx_SetColWriteMask(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
 // TODO
}

void Gfx_SetDepthWrite(cc_bool enabled) {
 // TODO
}
void Gfx_SetDepthTest(cc_bool enabled)  { }  // TODO

/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	// Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glortho
	//   The simplified calculation below uses: L = 0, R = width, T = 0, B = height
	// NOTE: Shared with OpenGL. might be wrong to do that though?
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

	// Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glfrustum
	// For a FOV based perspective matrix, left/right/top/bottom are calculated as:
	//   left = -c * aspect, right = c * aspect, bottom = -c, top = c
	// Calculations are simplified because of left/right and top/bottom symmetry
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

	String_Format1(info, "-- Using PS VITA (%i bit) --\n", &pointerSize);
	String_Format2(info, "Max texture size: (%i, %i)\n", &Gfx.MaxTexWidth, &Gfx.MaxTexHeight);
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
}

void Gfx_BeginFrame(void) {
 // TODO
}
void Gfx_Clear(void) { } // TODO

void Gfx_EndFrame(void) {
	Platform_LogConst("SWAP BUFFERS");
 // TODO
	if (gfx_minFrameMs) LimitFPS();
}

void Gfx_OnWindowResize(void) { }


/*########################################################################################################################*
*-------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	//fillFunc(gfx_indices, count, obj);
	return 1;
}

void Gfx_BindIb(GfxResourceID ib) { }
void Gfx_DeleteIb(GfxResourceID* ib) { }


/*########################################################################################################################*
*----------------------------------------------------------Buffers--------------------------------------------------------*
*#########################################################################################################################*/
static void* gfx_vertices;
static int vb_size;

GfxResourceID Gfx_CreateVb(VertexFormat fmt, int count) {
	void* data = memalign(16, count * strideSizes[fmt]);
	if (!data) Logger_Abort("Failed to allocate memory for GFX VB");
	return data;
	// TODO
}

void Gfx_BindVb(GfxResourceID vb) { gfx_vertices = vb; }

void Gfx_DeleteVb(GfxResourceID* vb) {
	GfxResourceID data = *vb;
	if (data) Mem_Free(data);
	*vb = 0;
	// TODO
}

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	vb_size = count * strideSizes[fmt];
	return vb;
	// TODO
}

void Gfx_UnlockVb(GfxResourceID vb) { 
	gfx_vertices = vb; 
	// TODO
}


GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices) {
	void* data = memalign(16, maxVertices * strideSizes[fmt]);
	if (!data) Logger_Abort("Failed to allocate memory for GFX VB");
	return data;
	// TODO
}

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	vb_size = count * strideSizes[fmt];
	return vb; 
	// TODO
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) { 
	gfx_vertices = vb; 
	// TODO
}

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	gfx_vertices = vb;
	Mem_Copy(vb, vertices, vCount * gfx_stride);
	// TODO
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFog(cc_bool enabled) {
 // TODO
}

void Gfx_SetFogCol(PackedCol color) {
 // TODO
}

void Gfx_SetFogDensity(float value) {
 // TODO
}

void Gfx_SetFogEnd(float value) {
 // TODO
}

void Gfx_SetFogMode(FogFunc func) {
 // TODO
}

void Gfx_SetAlphaTest(cc_bool enabled) { } 
 // TODO

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
 // TODO
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
 // TODO
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
 // TODO
}

void Gfx_EnableTextureOffset(float x, float y) {
 // TODO
}

void Gfx_DisableTextureOffset(void) { 
 // TODO
}



/*########################################################################################################################*
*---------------------------------------------------------Drawing---------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Gfx_WarnIfNecessary(void) { return false; }

void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
 // TODO
}

void Gfx_DrawVb_Lines(int verticesCount) {
 // TODO
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
 // TODO
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
 // TODO
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
 // TODO
}
#endif
