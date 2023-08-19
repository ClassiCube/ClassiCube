#include "Core.h"
#if defined CC_BUILD_PSVITA
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"
#include <vitasdk.h>
/* Current format and size of vertices */
static int gfx_stride, gfx_format = -1;
static int frontBufferIndex, backBufferIndex;

#define DISPLAY_WIDTH   960
#define DISPLAY_HEIGHT  544
#define DISPLAY_STRIDE 1024

#define NUM_DISPLAY_BUFFERS 3 // TODO: or just 2?
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

static SceGxmShaderPatcher *gxm_shader_patcher;
static const int shader_patcher_buffer_size = 64 * 1024;;
static SceUID gxm_shader_patcher_buffer_uid;
static void*  gxm_shader_patcher_buffer_addr;

static const int shader_patcher_vertex_usse_size = 64 * 1024;
static SceUID gxm_shader_patcher_vertex_usse_uid;
static void*  gxm_shader_patcher_vertex_usse_addr;
static unsigned int shader_patcher_vertex_usse_offset;

static const int shader_patcher_fragment_usse_size = 64 * 1024;
static SceUID gxm_shader_patcher_fragment_usse_uid;
static void*  gxm_shader_patcher_fragment_usse_addr;
static unsigned int shader_patcher_fragment_usse_offset;

static SceGxmShaderPatcherId gxm_clear_vertex_program_id;
static SceGxmShaderPatcherId gxm_clear_fragment_program_id;
static const SceGxmProgramParameter* gxm_clear_vertex_program_position_param;
static const SceGxmProgramParameter* gxm_clear_fragment_program_clear_color_param;
static SceGxmVertexProgram* gxm_clear_vertex_program_patched;
static SceGxmFragmentProgram* gxm_clear_fragment_program_patched;

#include "../misc/vita/clear_fs.h"
#include "../misc/vita/clear_vs.h"
static SceGxmProgram* gxm_program_clear_vs = (SceGxmProgram *)&clear_vs;
static SceGxmProgram* gxm_program_clear_fs = (SceGxmProgram *)&clear_fs;

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

static void FreeGPUMemory(SceUID uid) {
	void *addr;

	if (sceKernelGetMemBlockBase(uid, &addr) < 0)
		return;

	sceGxmUnmapMemory(addr);
	sceKernelFreeMemBlock(uid);
}

static void* AllocShaderPatcherMem(void* user_data, unsigned int size) {
	return Mem_TryAlloc(1, size);
}

static void FreeShaderPatcherMem(void* user_data, void* mem) {
	Mem_Free(mem);
}


/*########################################################################################################################*
*-----------------------------------------------------Initialisation------------------------------------------------------*
*#########################################################################################################################*/
struct DQCallbackData { void* addr; };

static void DQCallback(const void *callback_data) {
	SceDisplayFrameBuf fb = { 0 }; 

	fb.size   = sizeof(SceDisplayFrameBuf);
	fb.base   = ((struct DQCallbackData*)callback_data)->addr;
	fb.pitch  = DISPLAY_STRIDE;
	fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
	fb.width  = DISPLAY_WIDTH;
	fb.height = DISPLAY_HEIGHT;

	sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
	
	if (gfx_vsync) sceDisplayWaitVblankStart();
}

static void InitGXM(void) {
	SceGxmInitializeParams params = { 0 };
	
	params.displayQueueMaxPendingCount = MAX_PENDING_SWAPS;
	params.displayQueueCallback = DQCallback;
	params.displayQueueCallbackDataSize = sizeof(struct DQCallbackData);
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
	
	params.hostMem     = Mem_TryAlloc(1, SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE);
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

static void AllocShaderPatcherMemory(void) {
	gxm_shader_patcher_buffer_addr = AllocGPUMemory(shader_patcher_buffer_size, 
		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
		&gxm_shader_patcher_buffer_uid);

	gxm_shader_patcher_vertex_usse_addr = AllocGPUVertexUSSE(
		shader_patcher_vertex_usse_size, &gxm_shader_patcher_vertex_usse_uid,
		&shader_patcher_vertex_usse_offset);

	gxm_shader_patcher_fragment_usse_addr = AllocGPUFragmentUSSE(
		shader_patcher_fragment_usse_size, &gxm_shader_patcher_fragment_usse_uid,
		&shader_patcher_fragment_usse_offset);
}

static void AllocShaderPatcher(void) {
	SceGxmShaderPatcherParams params = { 0 };
	params.hostAllocCallback = AllocShaderPatcherMem;
	params.hostFreeCallback  = FreeShaderPatcherMem;
	
	params.bufferMem     = gxm_shader_patcher_buffer_addr;
	params.bufferMemSize = shader_patcher_buffer_size;
	
	params.vertexUsseMem     = gxm_shader_patcher_vertex_usse_addr;
	params.vertexUsseMemSize = shader_patcher_vertex_usse_size;
	params.vertexUsseOffset  = shader_patcher_vertex_usse_offset;
	
	params.fragmentUsseMem     = gxm_shader_patcher_fragment_usse_addr;
	params.fragmentUsseMemSize = shader_patcher_fragment_usse_size;
	params.fragmentUsseOffset  = shader_patcher_fragment_usse_offset;

	sceGxmShaderPatcherCreate(&params, &gxm_shader_patcher);
}

struct clear_vertex {
	float x, y, z;
};

static void AllocClearShader(void) {
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_clear_vs,
		&gxm_clear_vertex_program_id);
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_clear_fs,
		&gxm_clear_fragment_program_id);

	const SceGxmProgram *clear_vertex_program =
		sceGxmShaderPatcherGetProgramFromId(gxm_clear_vertex_program_id);
	const SceGxmProgram *clear_fragment_program =
		sceGxmShaderPatcherGetProgramFromId(gxm_clear_fragment_program_id);

	gxm_clear_vertex_program_position_param = sceGxmProgramFindParameterByName(
		clear_vertex_program, "position");

	gxm_clear_fragment_program_clear_color_param = sceGxmProgramFindParameterByName(
		clear_fragment_program, "clearColor");

	SceGxmVertexAttribute clear_vertex_attribute;
	SceGxmVertexStream clear_vertex_stream;
	clear_vertex_attribute.streamIndex = 0;
	clear_vertex_attribute.offset = 0;
	clear_vertex_attribute.format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	clear_vertex_attribute.componentCount = 2;
	clear_vertex_attribute.regIndex = sceGxmProgramParameterGetResourceIndex(
		gxm_clear_vertex_program_position_param);
	clear_vertex_stream.stride = sizeof(struct clear_vertex);
	clear_vertex_stream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
		gxm_clear_vertex_program_id, &clear_vertex_attribute,
		1, &clear_vertex_stream, 1, &gxm_clear_vertex_program_patched);

	sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
		gxm_clear_fragment_program_id, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		SCE_GXM_MULTISAMPLE_NONE, NULL, clear_fragment_program,
		&gxm_clear_fragment_program_patched);
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
	Platform_LogConst("clear shader 1");
	AllocRingBuffers();
	Platform_LogConst("clear shader 2");
	AllocGXMContext();
	Platform_LogConst("clear shader 3");
	
	AllocRenderTarget();
	Platform_LogConst("clear shader4");
	for (int i = 0; i < NUM_DISPLAY_BUFFERS; i++) 
	{
		AllocColorBuffer(i);
	}
	Platform_LogConst("clear shader 5");
	AllocDepthBuffer();
	Platform_LogConst("clear shader 6");
	AllocShaderPatcherMemory();
	Platform_LogConst("clear shader 7");
	AllocShaderPatcher();
	Platform_LogConst("clear shader 8");
	AllocClearShader();
	Platform_LogConst("clear shader 9");
	
	
	InitDefaultResources();
	Platform_LogConst("clear shader 10");
	
	frontBufferIndex = NUM_DISPLAY_BUFFERS - 1;
	backBufferIndex  = 0;
	
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

static float clear_color[4];
void Gfx_ClearCol(PackedCol color) {
	clear_color[0] = PackedCol_R(color) / 255.0f;
	clear_color[1] = PackedCol_G(color) / 255.0f;
	clear_color[2] = PackedCol_B(color) / 255.0f;
	clear_color[3] = PackedCol_A(color) / 255.0f;
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
	sceGxmBeginScene(gxm_context,
			0, gxm_render_target,
			NULL, NULL,
			gxm_sync_objects[backBufferIndex],
			&gxm_color_surfaces[backBufferIndex],
			&gxm_depth_stencil_surface);
}

void Gfx_EndFrame(void) {
	Platform_LogConst("SWAP BUFFERS");
	sceGxmEndScene(gxm_context, NULL, NULL);

	struct DQCallbackData cb_data;
	cb_data.addr = gxm_color_surfaces_addr[backBufferIndex];

	sceGxmDisplayQueueAddEntry(gxm_sync_objects[frontBufferIndex],
			gxm_sync_objects[backBufferIndex], &cb_data);

	// Cycle through to next buffer pair
	frontBufferIndex = backBufferIndex;
	backBufferIndex  = (backBufferIndex + 1) % NUM_DISPLAY_BUFFERS;
		
	if (gfx_minFrameMs) LimitFPS();
}

void Gfx_OnWindowResize(void) { }


/*########################################################################################################################*
*----------------------------------------------------------Buffers--------------------------------------------------------*
*#########################################################################################################################*/
struct GPUBuffer {
	void* data;
	SceUID uid;
};

struct GPUBuffer* GPUBuffer_Alloc(int size) {
	struct GPUBuffer* buffer = Mem_Alloc(1, sizeof(struct GPUBuffer), "GPU buffer");
	
	buffer->data = AllocGPUMemory(size, 
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_READ,
		&buffer->uid);
	return buffer;
}

static void GPUBuffer_Free(GfxResourceID* resource) {
	GfxResourceID raw = *resource;
	if (!raw) return;
	
	struct GPUBuffer* buffer = (struct GPUBuffer*)raw;
	FreeGPUMemory(buffer->uid);
	Mem_Free(buffer);
	*resource = NULL;
}


static void* gfx_indices;

GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	struct GPUBuffer* buffer = GPUBuffer_Alloc(count * 2);
	fillFunc(buffer->data, count, obj);
	return buffer;
}

void Gfx_BindIb(GfxResourceID ib) { 
	// TODO
	gfx_indices = ib;
}

void Gfx_DeleteIb(GfxResourceID* ib) { GPUBuffer_Free(ib); }


static void* gfx_vertices;
GfxResourceID Gfx_CreateVb(VertexFormat fmt, int count) {
	return GPUBuffer_Alloc(count * strideSizes[fmt]);
}

void Gfx_BindVb(GfxResourceID vb) { gfx_vertices = vb; }

void Gfx_DeleteVb(GfxResourceID* vb) { GPUBuffer_Free(vb); }

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	struct GPUBuffer* buffer = (struct GPUBuffer*)vb;
	return buffer->data;
}

void Gfx_UnlockVb(GfxResourceID vb) { 
	gfx_vertices = vb; 
	// TODO
}


GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices) {
	return GPUBuffer_Alloc(maxVertices * strideSizes[fmt]);
}

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	struct GPUBuffer* buffer = (struct GPUBuffer*)vb;
	return buffer->data;
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) { 
	gfx_vertices = vb;
	// TODO
}

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	struct GPUBuffer* buffer = (struct GPUBuffer*)vb;
	gfx_vertices = vb;
	Mem_Copy(buffer->data, vertices, vCount * gfx_stride);
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






void Gfx_Clear(void) {
	static struct GPUBuffer* clear_vertices;
	static struct GPUBuffer* clear_indices;
	
	if (!clear_vertices) {
		clear_vertices = GPUBuffer_Alloc(4 * sizeof(struct clear_vertex));
		clear_indices  = GPUBuffer_Alloc(4 * sizeof(unsigned short));
		
		struct clear_vertex* clear_vertices_data = clear_vertices->data;
		clear_vertices_data[0] = (struct clear_vertex){-1.0f, -1.0f};
		clear_vertices_data[1] = (struct clear_vertex){ 1.0f, -1.0f};
		clear_vertices_data[2] = (struct clear_vertex){-1.0f,  1.0f};
		clear_vertices_data[3] = (struct clear_vertex){ 1.0f,  1.0f};

		unsigned short* clear_indices_data = clear_indices->data;
		clear_indices_data[0] = 0;
		clear_indices_data[1] = 1;
		clear_indices_data[2] = 2;
		clear_indices_data[3] = 3;
	}
	
	sceGxmSetVertexProgram(gxm_context,   gxm_clear_vertex_program_patched);
	sceGxmSetFragmentProgram(gxm_context, gxm_clear_fragment_program_patched);

	void *uniform_buffer;
	sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &uniform_buffer);
	sceGxmSetUniformDataF(uniform_buffer, gxm_clear_fragment_program_clear_color_param,
		0, sizeof(clear_color) / sizeof(float), clear_color);

	sceGxmSetVertexStream(gxm_context, 0, clear_vertices->data);
	sceGxmDraw(gxm_context, SCE_GXM_PRIMITIVE_TRIANGLE_STRIP,
			SCE_GXM_INDEX_FORMAT_U16, clear_indices->data, 4);
}
#endif
