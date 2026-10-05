#define CC_DYNAMIC_VBS_ARE_STATIC
#define CC_SCRATCH_VBS_ARE_DYNAMIC
#include "../_GraphicsBase.h"
#include "../Errors.h"
#include "../Logger.h"
#include "../Window.h"

#include <vitasdk.h>

// TODO track last frame used on
static cc_bool gfx_R = true, gfx_G = true, gfx_B = true, gfx_A = true;
static int frontBufferIndex, backBufferIndex;
// Inspired from
// https://github.com/xerpi/gxmfun/blob/master/source/main.c
// https://github.com/vitasdk/samples/blob/6337766482561cf28092d21082202c0f01e3542b/gxm/textured_cube/src/main.c

#define DISPLAY_WIDTH   960
#define DISPLAY_HEIGHT  544
#define DISPLAY_STRIDE 1024

#define NUM_DISPLAY_BUFFERS 3 // TODO: or just 2?
#define MAX_PENDING_SWAPS   (NUM_DISPLAY_BUFFERS - 1)

static void GPUBuffers_DeleteUnreferenced(void);
static void GPUTextures_DeleteUnreferenced(void);
static cc_uint32 frameCounter;
static cc_bool in_scene;

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
static const int shader_patcher_buffer_size = 64 * 1024;
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


/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
void* AllocGPUMemory(int size, int type, int gpu_access, SceUID* ret_uid, const char* memType) {
	char buffer[128];
	cc_string str;
	void* addr;
	
	if (type == SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW) {
		size = CC_ALIGNUP(size, 256 * 1024);
	} else {
		size = CC_ALIGNUP(size, 4 * 1024);
	}
	String_InitArray_NT(str, buffer);
	
	// https://wiki.henkaku.xyz/vita/SceSysmem
	SceUID uid = sceKernelAllocMemBlock(memType, type, size, NULL);
	if (uid < 0) {
		String_Format2(&str, "Failed to allocate GPU memory block for %c (%i bytes)%N", memType, &size);
		Process_Abort2(uid, buffer);
	}
		
	int res1 = sceKernelGetMemBlockBase(uid, &addr);
	if (res1 < 0) {
		String_Format1(&str, "Failed to get base of GPU memory block for %c%N", memType);
		Process_Abort2(res1, buffer);
	}
		
	int res2 = sceGxmMapMemory(addr, size, gpu_access);
	if (res2 < 0) {
		String_Format2(&str, "Failed to map memory for GPU usage for %c (%i bytes)%N", memType, &size);
		Process_Abort2(res2, buffer);
	}
	// https://wiki.henkaku.xyz/vita/GPU
	
	*ret_uid = uid;
	return addr;
}

void* AllocGPUVertexUSSE(size_t size, SceUID* ret_uid, unsigned int* ret_usse_offset) {
	SceUID uid;
	void *addr;

	size = CC_ALIGNUP(size, 4 * 1024);

	uid = sceKernelAllocMemBlock("GPU vertex USSE",
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, NULL);
	if (uid < 0) Process_Abort2(uid, "Failed to allocate vertex USSE block");

	int res1 = sceKernelGetMemBlockBase(uid, &addr);
	if (res1 < 0) Process_Abort2(res1, "Failed to get base of vertex USSE memory block");

	int res2 = sceGxmMapVertexUsseMemory(addr, size, ret_usse_offset);
	if (res1 < 0) Process_Abort2(res2, "Failed to map vertex USSE memory");

	*ret_uid = uid;
	return addr;
}

void* AllocGPUFragmentUSSE(size_t size, SceUID* ret_uid, unsigned int* ret_usse_offset) {
	SceUID uid;
	void *addr;

	size = CC_ALIGNUP(size, 4 * 1024);

	uid = sceKernelAllocMemBlock("GPU fragment USSE",
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, NULL);
	if (uid < 0) Process_Abort2(uid, "Failed to allocate fragment USSE block");

	int res1 = sceKernelGetMemBlockBase(uid, &addr);
	if (res1 < 0) Process_Abort2(res1, "Failed to get base of fragment USSE memory block");

	int res2 = sceGxmMapFragmentUsseMemory(addr, size, ret_usse_offset);
	if (res1 < 0) Process_Abort2(res2, "Failed to map fragment USSE memory");

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
*-----------------------------------------------------Vertex shaders------------------------------------------------------*
*#########################################################################################################################*/
#define VS_INPUT_REG_POS 0
#define VS_INPUT_REG_COL 4
#define VS_INPUT_REG_TEX 8

static SceGxmVertexProgram* VP_BuildColoured(const uint8_t* src) {
	const SceGxmProgram* prog = (const SceGxmProgram*)src;
	SceGxmShaderPatcherId programID;
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, prog, &programID);

	SceGxmVertexAttribute attribs[2];
	SceGxmVertexStream vertex_stream;
	
	attribs[0].streamIndex    = 0;
	attribs[0].offset         = offsetof(struct VertexColoured, x);
	attribs[0].format         = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	attribs[0].componentCount = 3;
	attribs[0].regIndex       = VS_INPUT_REG_POS;
		
	attribs[1].streamIndex    = 0;
	attribs[1].offset         = offsetof(struct VertexColoured, Col);
	attribs[1].format         = SCE_GXM_ATTRIBUTE_FORMAT_U8N;
	attribs[1].componentCount = 4;
	attribs[1].regIndex       = VS_INPUT_REG_COL;
		
	vertex_stream.stride      = SIZEOF_VERTEX_COLOURED;
	vertex_stream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	SceGxmVertexProgram* programPatched = NULL;
	sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
		programID, attribs, Array_Elems(attribs),
		&vertex_stream, 1, &programPatched);
	return programPatched;
}

static SceGxmVertexProgram* VP_BuildTextured(const uint8_t* src) {
	const SceGxmProgram* prog = (const SceGxmProgram*)src;
	SceGxmShaderPatcherId programID;
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, prog, &programID);

	SceGxmVertexAttribute attribs[3];
	SceGxmVertexStream vertex_stream;
	
	attribs[0].streamIndex    = 0;

	attribs[0].offset         = offsetof(struct VertexTextured, x);
	attribs[0].format         = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	attribs[0].componentCount = 3;
	attribs[0].regIndex       = VS_INPUT_REG_POS;
		
	attribs[1].streamIndex    = 0;
	attribs[1].offset         = offsetof(struct VertexTextured, Col);
	attribs[1].format         = SCE_GXM_ATTRIBUTE_FORMAT_U8N;
	attribs[1].componentCount = 4;
	attribs[1].regIndex       = VS_INPUT_REG_COL;
		
	attribs[2].streamIndex    = 0;
	attribs[2].offset         = offsetof(struct VertexTextured, U);
	attribs[2].format         = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	attribs[2].componentCount = 2;
	attribs[2].regIndex       = VS_INPUT_REG_TEX;
		
	vertex_stream.stride      = SIZEOF_VERTEX_TEXTURED;
	vertex_stream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	SceGxmVertexProgram* programPatched = NULL;
	sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
		programID, attribs, Array_Elems(attribs),
		&vertex_stream, 1, &programPatched);
	return programPatched;
}

static const uint8_t coloured_v_gxp[] = {
	#embed "../../misc/vita/colored_v.gxp"
};
static const uint8_t textured_v_gxp[] = {
	#embed "../../misc/vita/textured_v.gxp"
};
static const uint8_t offset___v_gxp[] = {
	#embed "../../misc/vita/offset_v.gxp"
};


static SceGxmVertexProgram* VP_list[3];
static SceGxmVertexProgram* VP_Active;

static void VP_BuildPrograms(void) {
	VP_list[0] = VP_BuildColoured(coloured_v_gxp);
	VP_list[1] = VP_BuildTextured(textured_v_gxp);
	VP_list[2] = VP_BuildTextured( offset___v_gxp);
}

static float transposed_mvp[4*4] CC_ALIGNED(64);
static struct { float x, y; } texOffset;

static void VP_UpdateUniforms(void) {
	SceGxmVertexProgram* VP = VP_Active;
	// Calling sceGxmReserveVertexDefaultUniformBuffer when not in a scene
	//   results in SCE_GXM_ERROR_NOT_WITHIN_SCENE on real hardware
	if (!VP || !in_scene) return;
	void *uniform_buffer = NULL;
		
	int ret = sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &uniform_buffer);
	if (ret) Process_Abort2(ret, "Reserving uniform buffer");

	Mem_Copy(uniform_buffer, transposed_mvp, sizeof(transposed_mvp));
	if (VP == VP_list[2]) {
		Mem_Copy((char*)uniform_buffer + 64, &texOffset, sizeof(texOffset));
	}
}

static void VP_SwitchActive(void) {
	int index = 0;
	if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		index = (texOffset.x == 0.0f && texOffset.y == 0.0f) ? 1 : 2;
	}
	
	SceGxmVertexProgram* VP = VP_list[index];
	if (VP == VP_Active) return;
	VP_Active = VP;
	
	sceGxmSetVertexProgram(gxm_context, VP);
	VP_UpdateUniforms(); // TODO: really need to update uniforms after switching program?
}


/*########################################################################################################################*
*----------------------------------------------------Fragment shaders-----------------------------------------------------*
*#########################################################################################################################*/
static SceGxmFragmentProgram* FP_BuildProgram(const SceGxmBlendInfo* blend_mode, const uint8_t* src) {
	const SceGxmProgram* prog = (const SceGxmProgram*)src;
	SceGxmShaderPatcherId programID;
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, prog, &programID);

	SceGxmFragmentProgram* programPatched = NULL;
	sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
		programID, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		SCE_GXM_MULTISAMPLE_NONE, blend_mode, NULL,
		&programPatched);
	return programPatched;
}

static const uint8_t coloured_f_gxp[] = {
	#embed "../../misc/vita/colored_f.gxp"
};

static const uint8_t textured_none_f_gxp[] = {
	#embed "../../misc/vita/textured_none_f.gxp"
};
static const uint8_t textured_linr_f_gxp[] = {
	#embed "../../misc/vita/textured_linear_f.gxp"
};

static const uint8_t coloured_alpha_f_gxp[] = {
	#embed "../../misc/vita/colored_alpha_f.gxp"
};

static const uint8_t textured_alpha_f_gxp[] = {
	#embed "../../misc/vita/textured_alpha_f.gxp"
};

#define FP_WRITE_R (1 << 0)
#define FP_WRITE_G (1 << 1)
#define FP_WRITE_B (1 << 2)
#define FP_WRITE_A (1 << 3)
#define FP_BLEND   (1 << 4)

#define FP_STATES 32

static SceGxmFragmentProgram* FP_list[5 * FP_STATES];
static SceGxmFragmentProgram* FP_Active;

static void FP_BuildAll(int states) {
	SceGxmBlendInfo blend;
	int blending = states & FP_BLEND;

	blend.colorMask =
    	((states & FP_WRITE_R) ? SCE_GXM_COLOR_MASK_R : 0) |
    	((states & FP_WRITE_G) ? SCE_GXM_COLOR_MASK_G : 0) |
    	((states & FP_WRITE_B) ? SCE_GXM_COLOR_MASK_B : 0) |
    	((states & FP_WRITE_A) ? SCE_GXM_COLOR_MASK_A : 0);

	blend.colorFunc = blending ? SCE_GXM_BLEND_FUNC_ADD  : SCE_GXM_BLEND_FUNC_NONE;
	blend.alphaFunc = blending ? SCE_GXM_BLEND_FUNC_ADD  : SCE_GXM_BLEND_FUNC_NONE;
	blend.colorSrc  = blending ? SCE_GXM_BLEND_FACTOR_SRC_ALPHA : SCE_GXM_BLEND_FACTOR_ONE;
	blend.alphaSrc  = blending ? SCE_GXM_BLEND_FACTOR_SRC_ALPHA : SCE_GXM_BLEND_FACTOR_ZERO;
	blend.colorDst  = blending ? SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : SCE_GXM_BLEND_FACTOR_ONE;
	blend.alphaDst  = blending ? SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : SCE_GXM_BLEND_FACTOR_ONE;

	FP_list[ 0*FP_STATES + states] = FP_BuildProgram(&blend, coloured_f_gxp);
	FP_list[ 1*FP_STATES + states] = FP_BuildProgram(&blend, textured_none_f_gxp);
	FP_list[ 2*FP_STATES + states] = FP_BuildProgram(&blend, coloured_alpha_f_gxp);
	FP_list[ 3*FP_STATES + states] = FP_BuildProgram(&blend, textured_alpha_f_gxp);
	FP_list[ 4*FP_STATES + states] = FP_BuildProgram(&blend, textured_linr_f_gxp);
}

static void FP_UpdateUniforms(void) {
	SceGxmFragmentProgram* FP = FP_Active;
	// Calling sceGxmReserveFragmentDefaultUniformBuffer when not in a scene
	//   results in SCE_GXM_ERROR_NOT_WITHIN_SCENE on real hardware
	if (!FP || !in_scene) return;
	void *uniform_buffer = NULL;
		
	int ret = sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &uniform_buffer);
	if (!uniform_buffer) return; // non-fog shaders have no uniform buffer
	if (ret) Process_Abort2(ret, "Reserving uniform buffer");

	float* buf = uniform_buffer;
	buf[0] = PackedCol_R(gfx_fogColor) / 255.0f;
	buf[1] = PackedCol_G(gfx_fogColor) / 255.0f;
	buf[2] = PackedCol_B(gfx_fogColor) / 255.0f;
	buf[3] = gfx_fogMode == FOG_LINEAR ? gfx_fogEnd : gfx_fogDensity;
}

static void FP_SwitchActive(void) {
	int shdr = gfx_format == VERTEX_FORMAT_TEXTURED ? 1 : 0;
	if (gfx_alphaTest) shdr += 2;
	//if (gfx_fogEnabled) index = 12; // TODO: fix

	// TODO still not working properly?
    int states =
    	(gfx_R          ? FP_WRITE_R : 0) |
    	(gfx_G          ? FP_WRITE_G : 0) |
    	(gfx_B          ? FP_WRITE_B : 0) |
    	(gfx_A          ? FP_WRITE_A : 0) |
    	(gfx_alphaBlend ? FP_BLEND   : 0);
	
	int index = shdr * FP_STATES + states;
	if (FP_list[index] == NULL) FP_BuildAll(states);

	SceGxmFragmentProgram* FP = FP_list[index];
	if (FP == FP_Active) return;
	FP_Active = FP;
	
	sceGxmSetFragmentProgram(gxm_context, FP);
	FP_UpdateUniforms(); // TODO: need to update uniforms after switching program?
}


/*########################################################################################################################*
*-----------------------------------------------------Initialisation------------------------------------------------------*
*#########################################################################################################################*/
struct DQCallbackData { void* addr; };
void (*DQ_OnNextFrame)(void* fb);

static void DQ_OnNextFrame3D(void* fb) {
	if (gfx_vsync) sceDisplayWaitVblankStart();
	
	GPUBuffers_DeleteUnreferenced();
	GPUTextures_DeleteUnreferenced();
	frameCounter++;
}

static void DQCallback(const void *callback_data) {
	SceDisplayFrameBuf fb = { 0 }; 

	fb.size   = sizeof(SceDisplayFrameBuf);
	fb.base   = ((struct DQCallbackData*)callback_data)->addr;
	fb.pitch  = DISPLAY_STRIDE;
	fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
	fb.width  = DISPLAY_WIDTH;
	fb.height = DISPLAY_HEIGHT;

	sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
	DQ_OnNextFrame(fb.base);
}

void Gfx_InitGXM(void) { // called from Window_Init
	SceGxmInitializeParams params = { 0 };
	
	params.displayQueueMaxPendingCount  = MAX_PENDING_SWAPS;
	params.displayQueueCallback         = DQCallback;
	params.displayQueueCallbackDataSize = sizeof(struct DQCallbackData);
	params.parameterBufferSize = SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE;
	
	sceGxmInitialize(&params);
}

static void AllocRingBuffers(void) {
	vdm_ring_buffer_addr = AllocGPUMemory(SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE,
			SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
			&vdm_ring_buffer_uid, "VDM ring buffer");

	vertex_ring_buffer_addr = AllocGPUMemory(SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE,
			SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
			&vertex_ring_buffer_uid, "Vertex ring buffer");

	fragment_ring_buffer_addr = AllocGPUMemory(SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE,
			SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
			&fragment_ring_buffer_uid, "Fragment ring buffer");

	fragment_usse_ring_buffer_addr = AllocGPUFragmentUSSE(SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE,
			&fragment_ring_buffer_uid, &fragment_usse_offset);
}

static void AllocGXMContext(void) {
	SceGxmContextParams params = { 0 };
	
	params.hostMem     = Mem_Alloc(1, SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE, "Host memory");
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
	int size = CC_ALIGNUP(4 * DISPLAY_STRIDE * DISPLAY_HEIGHT, 1 * 1024 * 1024);
	
	gxm_color_surfaces_addr[i] = AllocGPUMemory(size, SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
									SCE_GXM_MEMORY_ATTRIB_RW, &gxm_color_surfaces_uid[i], "color buffer");

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
	int width   = CC_ALIGNUP(DISPLAY_WIDTH,  SCE_GXM_TILE_SIZEX);
	int height  = CC_ALIGNUP(DISPLAY_HEIGHT, SCE_GXM_TILE_SIZEY);
	int samples = width * height;

	gxm_depth_stencil_surface_addr = AllocGPUMemory(4 * samples, SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
										SCE_GXM_MEMORY_ATTRIB_RW, &gxm_depth_stencil_surface_uid, "depth buffer");

	sceGxmDepthStencilSurfaceInit(&gxm_depth_stencil_surface,
		SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24,
		SCE_GXM_DEPTH_STENCIL_SURFACE_TILED,
		width, gxm_depth_stencil_surface_addr, NULL);
}

static void AllocShaderPatcherMemory(void) {
	gxm_shader_patcher_buffer_addr = AllocGPUMemory(shader_patcher_buffer_size, 
		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
		&gxm_shader_patcher_buffer_uid, "shader patcher");

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

/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID white_square;
static void SetDefaultStates(void) {
	sceGxmSetFrontDepthFunc(gxm_context, SCE_GXM_DEPTH_FUNC_LESS_EQUAL);
	sceGxmSetBackDepthFunc(gxm_context,  SCE_GXM_DEPTH_FUNC_LESS_EQUAL);
}

void Gfx_AllocFramebuffers(void) { // called from Window_Init
	for (int i = 0; i < NUM_DISPLAY_BUFFERS; i++) 
	{
		AllocColorBuffer(i);
	}
	AllocDepthBuffer();
	
	frontBufferIndex = NUM_DISPLAY_BUFFERS - 1;
	backBufferIndex  = 0;
}

static void InitGPU(void) {
	AllocRingBuffers();
	AllocGXMContext();
	
	AllocRenderTarget();
	AllocShaderPatcherMemory();
	AllocShaderPatcher();

	VP_BuildPrograms();
}

void Gfx_Create(void) {
	DQ_OnNextFrame = DQ_OnNextFrame3D;
	if (!Gfx.Created) InitGPU();
	in_scene = false;
	
	Gfx.MaxTexWidth  = 1024;
	Gfx.MaxTexHeight = 1024;
	Gfx.MaxTexSize   = 512 * 512;
	Gfx.Created      = true;
	gfx_vsync        = true;
	
	Gfx_SetVertexFormat(VERTEX_FORMAT_COLOURED);

	Gfx.NonPowTwoTexturesSupport = GFX_NONPOW2_UPLOAD;
}

void Gfx_Free(void) { 
	Gfx_FreeState();
}

cc_bool Gfx_TryRestoreContext(void) { return true; }

static void Gfx_RestoreState(void) {
	InitDefaultResources();
	
	// 1x1 dummy white texture
	struct Bitmap bmp;
	BitmapCol pixels[1] = { BITMAPCOLOR_WHITE };
	Bitmap_Init(bmp, 1, 1, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
}

static void Gfx_FreeState(void) {
	FreeDefaultResources(); 
	Gfx_DeleteTexture(&white_square);
}


/*########################################################################################################################*
*--------------------------------------------------------GPU Textures-----------------------------------------------------*
*#########################################################################################################################*/
struct GPUTexture;
struct GPUTexture {
	cc_uint32* data;
	SceUID uid;
	SceGxmTexture texture;
	struct GPUTexture* next;
	cc_uint32 lastFrame;
};
static struct GPUTexture* del_textures_head;
static struct GPUTexture* del_textures_tail;

struct GPUTexture* GPUTexture_Alloc(int size) {
	struct GPUTexture* tex = Mem_AllocCleared(1, sizeof(struct GPUTexture), "GPU texture");
	
	tex->data = AllocGPUMemory(size, 
		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
		&tex->uid, "texture");
	return tex;
}

// can't delete textures until not used in any frames
static void GPUTexture_Unref(GfxResourceID* resource) {
	struct GPUTexture* tex = (struct GPUTexture*)(*resource);
	if (!tex) return;
	*resource = NULL;

	LinkedList_Append(tex, del_textures_head, del_textures_tail);
}

static void GPUTexture_Free(struct GPUTexture* tex) {
	FreeGPUMemory(tex->uid);
	Mem_Free(tex);
}

static void GPUTextures_DeleteUnreferenced(void) {
	if (!del_textures_head) return;
	
	struct GPUTexture* tex;
	struct GPUTexture* next;
	struct GPUTexture* prev = NULL;
	
	for (tex = del_textures_head; tex != NULL; tex = next)
	{
		next = tex->next;
		
		if (tex->lastFrame + 4 > frameCounter) {
			// texture was used within last 4 fames
			prev = tex;
		} else {
			// advance the head of the linked list
			if (del_textures_head == tex) 
				del_textures_head = next;

			// update end of linked list if necessary
			if (del_textures_tail == tex)
				del_textures_tail = prev;
			
			// unlink this texture from the linked list
			if (prev) prev->next = next;
			
			GPUTexture_Free(tex);
		}
	}
}


/*########################################################################################################################*
*---------------------------------------------------------Swizzling-------------------------------------------------------*
*#########################################################################################################################*/
// See Graphics_Dreamcast.c for twiddling explanation
static CC_INLINE void TwiddleCalcFactors(unsigned w, unsigned h, unsigned* maskX, unsigned* maskY) {
	*maskX = 0;
	*maskY = 0;
	int shift = 0;

	for (; w > 1 || h > 1; w >>= 1, h >>= 1)
	{
		if (w > 1 && h > 1) {
			// Add interleaved X and Y bits
			*maskY += 0x01 << shift;
			*maskX += 0x02 << shift;
			shift  += 2;
		} else if (w > 1) {
			// Add a linear X bit
			*maskX += 0x01 << shift;
			shift  += 1;		
		} else if (h > 1) {
			// Add a linear Y bit
			*maskY += 0x01 << shift;
			shift  += 1;		
		}
	}
}

static CC_INLINE void UploadFullTexture(struct Bitmap* bmp, int rowWidth, cc_uint32* dst, int dst_w, int dst_h) {
	int src_w = bmp->width, src_h = bmp->height;
	unsigned maskX, maskY;
	unsigned X = 0, Y = 0;
	TwiddleCalcFactors(dst_w, dst_h, &maskX, &maskY);
	
	for (int y = 0; y < src_h; y++)
	{
		cc_uint32* src = bmp->scan0 + y * rowWidth;
		X = 0;
		
		for (int x = 0; x < src_w; x++, src++)
		{
			dst[X | Y] = *src;
			X = (X - maskX) & maskX;
		}
		Y = (Y - maskY) & maskY;
	}
}

static CC_INLINE void UploadPartialTexture(struct Bitmap* part, int rowWidth, cc_uint32* dst, int dst_w, int dst_h,
						int originX, int originY) {
	int src_w = part->width, src_h = part->height;
	unsigned maskX, maskY;
	unsigned X = 0, Y = 0;
	TwiddleCalcFactors(dst_w, dst_h, &maskX, &maskY);

	// Calculate start twiddled X and Y values
	for (int x = 0; x < originX; x++) { X = (X - maskX) & maskX; }
	for (int y = 0; y < originY; y++) { Y = (Y - maskY) & maskY; }
	unsigned startX = X;
	
	for (int y = 0; y < src_h; y++)
	{
		cc_uint32* src = part->scan0 + y * rowWidth;
		X = startX;
		
		for (int x = 0; x < src_w; x++, src++)
		{
			dst[X | Y] = *src;
			X = (X - maskX) & maskX;
		}
		Y = (Y - maskY) & maskY;
	}
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	int dst_w = Math_NextPowOf2(bmp->width);
	int dst_h = Math_NextPowOf2(bmp->height);
	int size  = dst_w * dst_h * 4;

	struct GPUTexture* tex = GPUTexture_Alloc(size);
	cc_uint32* dst = tex->data;
	UploadFullTexture(bmp, rowWidth, dst, dst_w, dst_h);
            
	sceGxmTextureInitSwizzled(&tex->texture, dst,
		SCE_GXM_TEXTURE_FORMAT_A8B8G8R8, dst_w, dst_h, 0);
		
	sceGxmTextureSetUAddrMode(&tex->texture, SCE_GXM_TEXTURE_ADDR_REPEAT);
	sceGxmTextureSetVAddrMode(&tex->texture, SCE_GXM_TEXTURE_ADDR_REPEAT);
	return tex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int originX, int originY, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	struct GPUTexture* tex = (struct GPUTexture*)texId;
	int texWidth   = sceGxmTextureGetWidth(&tex->texture);
	int texHeight  = sceGxmTextureGetHeight(&tex->texture);

	cc_uint32* dst = tex->data;
	UploadPartialTexture(part, rowWidth, dst, texWidth, texHeight, originX, originY);

	// TODO: is it necessary to invalidate? probably just everything?
	//sceKernelDcacheWritebackInvalidateRange(dst, (tex->width * tex->height) * 4);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	GPUTexture_Unref(texId);
}

void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }

void Gfx_BindTexture(GfxResourceID texId) {
	if (!texId) texId = white_square;
 
 	struct GPUTexture* tex = (struct GPUTexture*)texId;
 	tex->lastFrame = frameCounter;
	sceGxmSetFragmentTexture(gxm_context, 0, &tex->texture);
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	// Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glortho
	//   The simplified calculation below uses: L = 0, R = width, T = 0, B = height
	// NOTE: Shared with OpenGL. might be wrong to do that though?
	*matrix = Matrix_Identity;

	matrix->row1.x =  2.0f / width;
	matrix->row2.y = -2.0f / height;
	matrix->row3.z = -2.0f / (zFar - zNear);

	matrix->row4.x = -1.0f;
	matrix->row4.y =  1.0f;
	matrix->row4.z = -(zFar + zNear) / (zFar - zNear);
}

static float Cotangent(float x) { return Math_CosF(x) / Math_SinF(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	float zNear = 0.1f;
	float c = Cotangent(0.5f * fov);

	// Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glfrustum
	// For a FOV based perspective matrix, left/right/top/bottom are calculated as:
	//   left = -c * aspect, right = c * aspect, bottom = -c, top = c
	// Calculations are simplified because of left/right and top/bottom symmetry
	*matrix = Matrix_Identity;

	matrix->row1.x =  c / aspect;
	matrix->row2.y =  c;
	matrix->row3.z = -(zFar + zNear) / (zFar - zNear);
	matrix->row3.w = -1.0f;
	matrix->row4.z = -(2.0f * zFar * zNear) / (zFar - zNear);
	matrix->row4.w =  0.0f;
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using PS Vita --\n");
	PrintMaxTextureInfo(info);
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
}

void Gfx_BeginFrame(void) {
	in_scene = true;
	sceGxmBeginScene(gxm_context,
			0, gxm_render_target,
			NULL, NULL,
			gxm_sync_objects[backBufferIndex],
			&gxm_color_surfaces[backBufferIndex],
			&gxm_depth_stencil_surface);
}

void Gfx_UpdateCommonDialogBuffers(void) {
	SceCommonDialogUpdateParam param = { 0 };
	param.renderTarget.colorFormat   = SCE_GXM_COLOR_FORMAT_A8B8G8R8;
	param.renderTarget.surfaceType   = SCE_GXM_COLOR_SURFACE_LINEAR;
	param.renderTarget.width         = DISPLAY_WIDTH;
	param.renderTarget.height        = DISPLAY_HEIGHT;
	param.renderTarget.strideInPixels   = DISPLAY_STRIDE;
	param.renderTarget.colorSurfaceData = gxm_color_surfaces_addr[backBufferIndex];
	param.renderTarget.depthSurfaceData = gxm_depth_stencil_surface.depthData;
	param.displaySyncObject = gxm_sync_objects[backBufferIndex];
	
	sceCommonDialogUpdate(&param);
}

void Gfx_NextFramebuffer(void) {
	struct DQCallbackData cb_data;
	cb_data.addr = gxm_color_surfaces_addr[backBufferIndex];

	sceGxmDisplayQueueAddEntry(gxm_sync_objects[frontBufferIndex],
			gxm_sync_objects[backBufferIndex], &cb_data);

	// Cycle through to next buffer pair
	frontBufferIndex = backBufferIndex;
	backBufferIndex  = (backBufferIndex + 1) % NUM_DISPLAY_BUFFERS;
}

void Gfx_EndFrame(void) {
	in_scene = false;
	sceGxmEndScene(gxm_context, NULL, NULL);

	Gfx_NextFramebuffer();
}

void Gfx_OnWindowResize(int width, int height) { }

// NOTE: beginScene resets viewport and clip region
void Gfx_SetViewport(int x, int y, int w, int h) { 
	sceGxmSetViewport(
		gxm_context,
		x + 0.5f * w,  0.5f * w,
		y + 0.5f * h, -0.5f * h,
		0.5f, 0.5f);
}

void Gfx_SetScissor (int x, int y, int w, int h) {
	sceGxmSetRegionClip(gxm_context, SCE_GXM_REGION_CLIP_OUTSIDE, 
						x, y, x + w - 1, y + h - 1);
}


/*########################################################################################################################*
*--------------------------------------------------------GPU Buffers------------------------------------------------------*
*#########################################################################################################################*/
struct GPUBuffer;
struct GPUBuffer {
	void* data;
	SceUID uid;
	cc_uint32 lastFrame;
	struct GPUBuffer* next;
};
static struct GPUBuffer* del_buffers_head;
static struct GPUBuffer* del_buffers_tail;

struct GPUBuffer* GPUBuffer_Alloc(int size) {
	struct GPUBuffer* buffer = Mem_AllocCleared(1, sizeof(struct GPUBuffer), "GPU buffer");
	
	buffer->data = AllocGPUMemory(size, 
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_READ,
		&buffer->uid, "buffer");
	return buffer;
}


// can't delete buffers until not used in any frames
static void GPUBuffer_Unref(GfxResourceID* resource) {
	struct GPUBuffer* buf = (struct GPUBuffer*)(*resource);
	if (!buf) return;
	*resource = NULL;
	
	LinkedList_Append(buf, del_buffers_head, del_buffers_tail);
}

static void GPUBuffer_Free(struct GPUBuffer* buf) {
	FreeGPUMemory(buf->uid);
	Mem_Free(buf);
}

static void GPUBuffers_DeleteUnreferenced(void) {
	if (!del_buffers_head) return;
	
	struct GPUBuffer* buf;
	struct GPUBuffer* next;
	struct GPUBuffer* prev = NULL;
	
	for (buf = del_buffers_head; buf != NULL; buf = next)
	{
		next = buf->next;
		
		if (buf->lastFrame + 4 > frameCounter) {
			// texture was used within last 4 fames
			prev = buf;
		} else {
			// advance the head of the linked list
			if (del_buffers_head == buf) 
				del_buffers_head = next;

			// update end of linked list if necessary
			if (del_buffers_tail == buf)
				del_buffers_tail = prev;
			
			// unlink this texture from the linked list
			if (prev) prev->next = next;
			
			GPUBuffer_Free(buf);
		}
	}
}


/*########################################################################################################################*
*-------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
static cc_uint16* gfx_indices;

GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	struct GPUBuffer* buffer = GPUBuffer_Alloc(count * 2);
	fillFunc(buffer->data, count, obj);
	return buffer;
}

void Gfx_BindIb(GfxResourceID ib) {
	struct GPUBuffer* buffer = (struct GPUBuffer*)ib;
	gfx_indices = buffer->data;
}

void Gfx_DeleteIb(GfxResourceID* ib) { GPUBuffer_Unref(ib); }


/*########################################################################################################################*
*-------------------------------------------------------Vertex buffers----------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	return GPUBuffer_Alloc(count * strideSizes[fmt]);
}

void Gfx_BindVb(GfxResourceID vb) { 
	struct GPUBuffer* buffer = (struct GPUBuffer*)vb;
	buffer->lastFrame = frameCounter;
	sceGxmSetVertexStream(gxm_context, 0, buffer->data);
}

void Gfx_DeleteVb(GfxResourceID* vb) { GPUBuffer_Unref(vb); }

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	struct GPUBuffer* buffer = (struct GPUBuffer*)vb;
	return buffer->data;
}

void Gfx_UnlockVb(GfxResourceID vb) { }


/*########################################################################################################################*
*------------------------------------------------------------Fog----------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFog(cc_bool enabled) {
	gfx_fogEnabled = enabled;
	FP_SwitchActive();
}

static void SetFogColor(PackedCol color) { FP_UpdateUniforms(); }

static void SetFogDensity(float value)   { FP_UpdateUniforms(); }

static void SetFogEnd(float value)       { FP_UpdateUniforms(); }

static void SetFogMode(FogFunc func)     { FP_SwitchActive(); }


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static void SetAlphaTest(cc_bool enabled) {
	FP_SwitchActive();
}
 
static void SetAlphaBlend(cc_bool enabled) {
	FP_SwitchActive();
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
    DefaultDepthOnlyRendering(depthOnly);
}

void Gfx_SetFaceCulling(cc_bool enabled) { 
	sceGxmSetCullMode(gxm_context, enabled ? SCE_GXM_CULL_CW : SCE_GXM_CULL_NONE);
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
    gfx_R = r; gfx_G = g; gfx_B = b; gfx_A = a;
	FP_SwitchActive();
}

static void SetDepthWrite(cc_bool enabled) {
	int mode = enabled ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED;
	sceGxmSetFrontDepthWriteEnable(gxm_context, mode);
	sceGxmSetBackDepthWriteEnable(gxm_context,  mode);
}

static void SetDepthTest(cc_bool enabled) {
	int func = enabled ? SCE_GXM_DEPTH_FUNC_LESS_EQUAL : SCE_GXM_DEPTH_FUNC_ALWAYS;
	sceGxmSetFrontDepthFunc(gxm_context, func);
	sceGxmSetBackDepthFunc(gxm_context,  func);
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static struct Matrix _view, _proj;

static void UpdateMVP(void) {
	struct Matrix mvp CC_ALIGNED(64);
	Matrix_Mul(&mvp, &_view, &_proj);
	float* m = (float*)&mvp;
	
	// Transpose matrix
	for (int i = 0; i < 4; i++)
	{
		transposed_mvp[i * 4 + 0] = m[0  + i];
		transposed_mvp[i * 4 + 1] = m[4  + i];
		transposed_mvp[i * 4 + 2] = m[8  + i];
		transposed_mvp[i * 4 + 3] = m[12 + i];
	}

	VP_UpdateUniforms();
}

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW) _view = *matrix;
	if (type == MATRIX_PROJ) _proj = *matrix;

	UpdateMVP();
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	_view = *view;
	_proj = *proj;

	UpdateMVP();
	Matrix_Mul(mvp, view, proj);
}

void Gfx_EnableTextureOffset(float x, float y) {
	texOffset.x = x; texOffset.y  = y;
	VP_SwitchActive();
}

void Gfx_DisableTextureOffset(void) {
	texOffset.x = 0; texOffset.y = 0;
	VP_SwitchActive();
}


/*########################################################################################################################*
*---------------------------------------------------------Drawing---------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Gfx_WarnIfNecessary(void) { return false; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }

void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
	
	VP_SwitchActive();
	FP_SwitchActive();
}

void Gfx_DrawVb_Lines(int verticesCount) {
 // TODO
}

// TODO probably wrong to offset index buffer
void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	//Platform_Log2("DRAW1: %i, %i", &verticesCount, &startVertex); Thread_Sleep(100);
	sceGxmDraw(gxm_context, SCE_GXM_PRIMITIVE_TRIANGLES,
			SCE_GXM_INDEX_FORMAT_U16, gfx_indices + ICOUNT(startVertex), ICOUNT(verticesCount));
}

// TODO probably wrong to offset index buffer
void Gfx_DrawVb_IndexedTris(int verticesCount) {
	//Platform_Log1("DRAW2: %i", &verticesCount); Thread_Sleep(100);
	sceGxmDraw(gxm_context, SCE_GXM_PRIMITIVE_TRIANGLES,
			SCE_GXM_INDEX_FORMAT_U16, gfx_indices, ICOUNT(verticesCount));
}

// TODO probably wrong to offset index buffer
void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex, DrawHints hints) {
	//Platform_Log2("DRAW3: %i, %i", &verticesCount, &startVertex); Thread_Sleep(100);
	sceGxmDraw(gxm_context, SCE_GXM_PRIMITIVE_TRIANGLES,
			SCE_GXM_INDEX_FORMAT_U16, gfx_indices + ICOUNT(startVertex), ICOUNT(verticesCount));
}


/*########################################################################################################################*
*--------------------------------------------------------Clearing---------------------------------------------------------*
*#########################################################################################################################*/
static PackedCol clear_color;
void Gfx_ClearColor(PackedCol color) {
	clear_color = color;
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
	// TODO clear only some buffers
	static struct GPUBuffer* clearVB;
	if (!clearVB) {
		clearVB = GPUBuffer_Alloc(4 * sizeof(struct VertexColoured));
	}

	int clear_z = (buffers & GFX_BUFFER_DEPTH) != 0;
	int clear_c = (buffers & GFX_BUFFER_COLOR) != 0;
	
	struct VertexColoured* clear_vertices = clearVB->data;
	clear_vertices[0] = (struct VertexColoured){-1.0f, -1.0f, 1.0f, clear_color };
	clear_vertices[1] = (struct VertexColoured){ 1.0f, -1.0f, 1.0f, clear_color };
	clear_vertices[2] = (struct VertexColoured){ 1.0f,  1.0f, 1.0f, clear_color };
	clear_vertices[3] = (struct VertexColoured){-1.0f,  1.0f, 1.0f, clear_color };

	cc_bool alpha_test  = gfx_alphaTest;
	cc_bool alpha_blend = gfx_alphaBlend;
	cc_bool depth_test  = gfx_depthTest;
	cc_bool depth_write = gfx_depthWrite;
	struct Matrix view  = _view;
	struct Matrix proj  = _proj;
	
	Gfx_SetVertexFormat(VERTEX_FORMAT_COLOURED);

	Gfx_SetAlphaTest    (false);
	Gfx_SetAlphaBlending(false);
	Gfx_SetDepthTest    (false);
	Gfx_SetDepthWrite   (clear_z);
	SetColorWrite(clear_c, clear_c, clear_c, clear_c);
	Gfx_LoadMatrix(MATRIX_VIEW, &Matrix_Identity);
	Gfx_LoadMatrix(MATRIX_PROJ, &Matrix_Identity);

	Gfx_BindVb(clearVB);
	Gfx_DrawVb_IndexedTris(4);
	
	Gfx_SetAlphaTest    (alpha_test);
	Gfx_SetAlphaBlending(alpha_blend);
	Gfx_SetDepthTest    (depth_test);
	Gfx_SetDepthWrite   (depth_write);
	DefaultDepthOnlyRendering(false);
	Gfx_LoadMatrix(MATRIX_VIEW, &view);
	Gfx_LoadMatrix(MATRIX_PROJ, &proj);
}

