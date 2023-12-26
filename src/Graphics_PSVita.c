#include "Core.h"
#if defined CC_BUILD_PSVITA
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"
#include <vitasdk.h>

// TODO track last frame used on
static cc_bool gfx_depthOnly;
static cc_bool gfx_alphaTesting, gfx_alphaBlending;
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


#include "../misc/vita/colored_f.h"
#include "../misc/vita/colored_v.h"
static SceGxmProgram* gxm_colored_VP = (SceGxmProgram *)&colored_v;
static SceGxmProgram* gxm_colored_FP = (SceGxmProgram *)&colored_f;

#include "../misc/vita/textured_f.h"
#include "../misc/vita/textured_v.h"
static SceGxmProgram* gxm_textured_VP = (SceGxmProgram *)&textured_v;
static SceGxmProgram* gxm_textured_FP = (SceGxmProgram *)&textured_f;

#include "../misc/vita/colored_alpha_f.h"
static SceGxmProgram* gxm_colored_alpha_FP = (SceGxmProgram *)&colored_alpha_f;
#include "../misc/vita/textured_alpha_f.h"
static SceGxmProgram* gxm_textured_alpha_FP = (SceGxmProgram *)&textured_alpha_f;


typedef struct CCVertexProgram {
	SceGxmVertexProgram* programPatched;
	const SceGxmProgramParameter* param_in_position;
	const SceGxmProgramParameter* param_in_color;
	const SceGxmProgramParameter* param_in_texcoord;
	const SceGxmProgramParameter* param_uni_mvp;
	int dirtyUniforms;
} VertexProgram;
#define VP_UNI_MATRIX 0x01

typedef struct CCFragmentProgram {
	SceGxmFragmentProgram* programPatched;
} FragmentProgram;


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
*-----------------------------------------------------Vertex shaders------------------------------------------------------*
*#########################################################################################################################*/
static VertexProgram VP_list[2];
static VertexProgram* VP_Active;
static float transposed_mvp[4*4] __attribute__((aligned(64)));

static void VP_DirtyUniform(int uniform) {
	for (int i = 0; i < Array_Elems(VP_list); i++) 
	{
		VP_list[i].dirtyUniforms |= uniform;
	}
}

static void VP_ReloadUniforms(void) {
	VertexProgram* VP = VP_Active;
	// Calling sceGxmReserveVertexDefaultUniformBuffer when not in a scene
	//   results in SCE_GXM_ERROR_NOT_WITHIN_SCENE on real hardware
	if (!VP || !in_scene) return;
	int ret;

	if (VP->dirtyUniforms & VP_UNI_MATRIX) {
		void *uniform_buffer = NULL;
		
		ret = sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &uniform_buffer);
		if (ret) Logger_Abort2(ret, "Reserving uniform buffer");
		ret = sceGxmSetUniformDataF(uniform_buffer, VP->param_uni_mvp, 0, 4 * 4, transposed_mvp);
		if (ret) Logger_Abort2(ret, "Updating uniform buffer");
			
		VP->dirtyUniforms &= ~VP_UNI_MATRIX;
	}
}

static void VP_SwitchActive(void) {
	int index = gfx_format == VERTEX_FORMAT_TEXTURED ? 1 : 0;
	
	VertexProgram* VP = &VP_list[index];
	if (VP == VP_Active) return;
	VP_Active = VP;
	
	VP->dirtyUniforms |= VP_UNI_MATRIX; // Need to update uniforms after switching program
	sceGxmSetVertexProgram(gxm_context, VP->programPatched);
	VP_ReloadUniforms();
}


/*########################################################################################################################*
*----------------------------------------------------Fragment shaders-----------------------------------------------------*
*#########################################################################################################################*/
static FragmentProgram FP_list[4 * 3];
static FragmentProgram* FP_Active;

static void FP_SwitchActive(void) {
	int index = gfx_format == VERTEX_FORMAT_TEXTURED ? 3 : 0;
	
	// [normal rendering, blend rendering, no rendering]
	if (gfx_depthOnly) {
		index += 2;
	} else if (gfx_alphaBlending) {
		index += 1;
	}
	
	if (gfx_alphaTesting) index += 2 * 3;
	
	FragmentProgram* FP = &FP_list[index];
	if (FP == FP_Active) return;
	FP_Active = FP;
	
	sceGxmSetFragmentProgram(gxm_context, FP->programPatched);
}


static const SceGxmBlendInfo no_blending = {
	SCE_GXM_COLOR_MASK_ALL,
	SCE_GXM_BLEND_FUNC_NONE,  SCE_GXM_BLEND_FUNC_NONE,
	SCE_GXM_BLEND_FACTOR_ONE, SCE_GXM_BLEND_FACTOR_ZERO,
	SCE_GXM_BLEND_FACTOR_ONE, SCE_GXM_BLEND_FACTOR_ZERO
};
static const SceGxmBlendInfo yes_blending = {
	SCE_GXM_COLOR_MASK_ALL,
	SCE_GXM_BLEND_FUNC_ADD,   SCE_GXM_BLEND_FUNC_ADD,
	SCE_GXM_BLEND_FACTOR_SRC_ALPHA, SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	SCE_GXM_BLEND_FACTOR_SRC_ALPHA, SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA
};
static const SceGxmBlendInfo no_rendering = {
	SCE_GXM_COLOR_MASK_NONE,
	SCE_GXM_BLEND_FUNC_NONE,  SCE_GXM_BLEND_FUNC_NONE,
	SCE_GXM_BLEND_FACTOR_ONE, SCE_GXM_BLEND_FACTOR_ZERO,
	SCE_GXM_BLEND_FACTOR_ONE, SCE_GXM_BLEND_FACTOR_ZERO
};
static const SceGxmBlendInfo* blend_modes[] = { &no_blending, &yes_blending, &no_rendering };

static void CreateFragmentPrograms(int index, const SceGxmProgram* fragProgram, const SceGxmProgram* vertexProgram) {
	SceGxmShaderPatcherId programID;
	
	for (int i = 0; i < Array_Elems(blend_modes); i++)
	{
		FragmentProgram* FP = &FP_list[index + i];
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, fragProgram, &programID);
		
		const SceGxmProgram* prog = sceGxmShaderPatcherGetProgramFromId(programID); // TODO just use original program directly?

		sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
			programID, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
			SCE_GXM_MULTISAMPLE_NONE, blend_modes[i], vertexProgram,
			&FP->programPatched);
	}
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

static void AllocColouredVertexProgram(int index) {
	SceGxmShaderPatcherId programID;
	VertexProgram* VP = &VP_list[index];
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_colored_VP, &programID);

	const SceGxmProgram* prog = sceGxmShaderPatcherGetProgramFromId(programID);

	VP->param_in_position = sceGxmProgramFindParameterByName(prog, "in_position");
	VP->param_in_color    = sceGxmProgramFindParameterByName(prog, "in_color");
	VP->param_uni_mvp     = sceGxmProgramFindParameterByName(prog, "mvp_matrix");

	SceGxmVertexAttribute attribs[2];
	SceGxmVertexStream vertex_stream;
	
	attribs[0].streamIndex    = 0;
	attribs[0].offset         = 0;
	attribs[0].format         = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	attribs[0].componentCount = 3;
	attribs[0].regIndex       = sceGxmProgramParameterGetResourceIndex(VP->param_in_position);
		
	attribs[1].streamIndex    = 0;
	attribs[1].offset         = 3 * sizeof(float);
	attribs[1].format         = SCE_GXM_ATTRIBUTE_FORMAT_U8N;
	attribs[1].componentCount = 4;
	attribs[1].regIndex       = sceGxmProgramParameterGetResourceIndex(VP->param_in_color);
		
	vertex_stream.stride      = SIZEOF_VERTEX_COLOURED;
	vertex_stream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
		programID, attribs, 2,
		&vertex_stream, 1, &VP->programPatched);
}

static void AllocTexturedVertexProgram(int index) {
	SceGxmShaderPatcherId programID;
	VertexProgram* VP = &VP_list[index];
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_textured_VP, &programID);

	const SceGxmProgram* prog = sceGxmShaderPatcherGetProgramFromId(programID);

	VP->param_in_position = sceGxmProgramFindParameterByName(prog, "in_position");
	VP->param_in_color    = sceGxmProgramFindParameterByName(prog, "in_color");
	VP->param_in_texcoord = sceGxmProgramFindParameterByName(prog, "in_texcoord");
	VP->param_uni_mvp     = sceGxmProgramFindParameterByName(prog, "mvp_matrix");

	SceGxmVertexAttribute attribs[3];
	SceGxmVertexStream vertex_stream;
	
	attribs[0].streamIndex    = 0;
	attribs[0].offset         = 0;
	attribs[0].format         = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	attribs[0].componentCount = 3;
	attribs[0].regIndex       = sceGxmProgramParameterGetResourceIndex(VP->param_in_position);
		
	attribs[1].streamIndex    = 0;
	attribs[1].offset         = 3 * sizeof(float);
	attribs[1].format         = SCE_GXM_ATTRIBUTE_FORMAT_U8N;
	attribs[1].componentCount = 4;
	attribs[1].regIndex       = sceGxmProgramParameterGetResourceIndex(VP->param_in_color);
		
	attribs[2].streamIndex    = 0;
	attribs[2].offset         = 3 * sizeof(float) + 4 * sizeof(char);
	attribs[2].format         = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	attribs[2].componentCount = 2;
	attribs[2].regIndex       = sceGxmProgramParameterGetResourceIndex(VP->param_in_texcoord);
		
	vertex_stream.stride      = SIZEOF_VERTEX_TEXTURED;
	vertex_stream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
		programID, attribs, 3,
		&vertex_stream, 1, &VP->programPatched);
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
	
	AllocColouredVertexProgram(0);
	CreateFragmentPrograms(0, gxm_colored_FP,        gxm_colored_VP);
	CreateFragmentPrograms(6, gxm_colored_alpha_FP,  gxm_colored_VP);
	AllocTexturedVertexProgram(1);
	CreateFragmentPrograms(3, gxm_textured_FP,       gxm_textured_VP);
	CreateFragmentPrograms(9, gxm_textured_alpha_FP, gxm_textured_VP);
}

void Gfx_Create(void) {
	DQ_OnNextFrame = DQ_OnNextFrame3D;
	if (!Gfx.Created) InitGPU();
	in_scene = false;
	
	Gfx.MaxTexWidth  = 512;
	Gfx.MaxTexHeight = 512;
	Gfx.Created      = true;
	gfx_vsync        = true;
	
	Gfx_SetDepthTest(true);
	Gfx_SetVertexFormat(VERTEX_FORMAT_COLOURED);
	Gfx_RestoreState();
}

void Gfx_Free(void) { 
	Gfx_FreeState();
}

cc_bool Gfx_TryRestoreContext(void) { return true; }

void Gfx_RestoreState(void) {
	InitDefaultResources();
	
	// 1x1 dummy white texture
	struct Bitmap bmp;
	BitmapCol pixels[1] = { BITMAPCOLOR_WHITE };
	Bitmap_Init(bmp, 1, 1, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
	// TODO
}

void Gfx_FreeState(void) {
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
		&tex->uid);
	return tex;
}

// can't delete textures until not used in any frames
static void GPUTexture_Unref(GfxResourceID* resource) {
	struct GPUTexture* tex = (struct GPUTexture*)(*resource);
	if (!tex) return;
	*resource = NULL;
	
	cc_uintptr addr = tex;
	Platform_Log1("TEX UNREF %h", &addr);
	LinkedList_Append(tex, del_textures_head, del_textures_tail);
}

static void GPUTexture_Free(struct GPUTexture* tex) {
	cc_uintptr addr = tex;
	Platform_Log1("TEX DELETE %h", &addr);
	FreeGPUMemory(tex->uid);
	Mem_Free(tex);
}

static void GPUTextures_DeleteUnreferenced(void) {
	if (!del_textures_head) return;
	del_textures_tail = NULL;
	
	struct GPUTexture* tex;
	struct GPUTexture* next;
	struct GPUTexture* prev = NULL;
	
	for (tex = del_textures_head; tex != NULL; tex = next)
	{
		next = tex->next;
		
		if (tex->lastFrame + 4 > frameCounter) {
			// texture was used within last 4 fames
			prev              = tex;
			del_textures_tail = tex; // update end of linked list
			
			cc_uintptr addr = tex;
			Platform_Log1("TEX CHECK %h", &addr);
		} else {
			// advance the head of the linked list
			if (del_textures_head == tex) 
				del_textures_head = next;
			
			// unlink this texture from the linked list
			if (prev) prev->next = next;
			
			GPUTexture_Free(tex);
		}
	}
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	int size = bmp->width * bmp->height * 4;
	struct GPUTexture* tex = GPUTexture_Alloc(size);
	Mem_Copy(tex->data, bmp->scan0, size);
            
	sceGxmTextureInitLinear(&tex->texture, tex->data,
		SCE_GXM_TEXTURE_FORMAT_A8B8G8R8, bmp->width, bmp->height, 0);
		
	sceGxmTextureSetUAddrMode(&tex->texture, SCE_GXM_TEXTURE_ADDR_REPEAT);
	sceGxmTextureSetVAddrMode(&tex->texture, SCE_GXM_TEXTURE_ADDR_REPEAT);
	return tex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	struct GPUTexture* tex = (struct GPUTexture*)texId;
	int texWidth = sceGxmTextureGetWidth(&tex->texture);
	
	// NOTE: Only valid for LINEAR textures
	cc_uint32* dst = (tex->data + x) + y * texWidth;
	
	CopyTextureData(dst, texWidth * 4, part, rowWidth << 2);
	// TODO: Do line by line and only invalidate the actually changed parts of lines?
	//sceKernelDcacheWritebackInvalidateRange(dst, (tex->width * part->height) * 4);
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
	Gfx_UpdateTexture(texId, x, y, part, part->width, mipmaps);
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

static double Cotangent(double x) { return Math_Cos(x) / Math_Sin(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	float zNear = 0.1f;
	float c = (float)Cotangent(0.5f * fov);

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

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
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
	if (gfx_minFrameMs) LimitFPS();
}

void Gfx_OnWindowResize(void) { }


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
		&buffer->uid);
		
	cc_uintptr addr = buffer->data;
	Platform_Log2("VB ALLOC %h = %i bytes", &addr, &size);
	return buffer;
}


// can't delete buffers until not used in any frames
static void GPUBuffer_Unref(GfxResourceID* resource) {
	struct GPUBuffer* buf = (struct GPUBuffer*)(*resource);
	if (!buf) return;
	*resource = NULL;
	
	cc_uintptr addr = buf;
	Platform_Log1("VB UNREF %h", &addr);
	LinkedList_Append(buf, del_buffers_head, del_buffers_tail);
}

static void GPUBuffer_Free(struct GPUBuffer* buf) {
	cc_uintptr addr = buf;
	Platform_Log1("VB DELETE %h", &addr);
	FreeGPUMemory(buf->uid);
	Mem_Free(buf);
}

static void GPUBuffers_DeleteUnreferenced(void) {
	if (!del_buffers_head) return;
	del_buffers_tail = NULL;
	
	struct GPUBuffer* buf;
	struct GPUBuffer* next;
	struct GPUBuffer* prev = NULL;
	
	for (buf = del_buffers_head; buf != NULL; buf = next)
	{
		next = buf->next;
		
		if (buf->lastFrame + 4 > frameCounter) {
			// texture was used within last 4 fames
			prev             = buf;
			del_buffers_tail = buf; // update end of linked list
			
			cc_uintptr addr = buf;
			Platform_Log1("VB CHECK %h", &addr);
		} else {
			// advance the head of the linked list
			if (del_buffers_head == buf) 
				del_buffers_head = next;
			
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

void Gfx_UnlockVb(GfxResourceID vb) { Gfx_BindVb(vb); }


static GfxResourceID Gfx_AllocDynamicVb(VertexFormat fmt, int maxVertices) {
	return GPUBuffer_Alloc(maxVertices * strideSizes[fmt]);
}

void Gfx_BindDynamicVb(GfxResourceID vb) { Gfx_BindVb(vb); }

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	struct GPUBuffer* buffer = (struct GPUBuffer*)vb;
	return buffer->data;
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) { Gfx_BindVb(vb); }

void Gfx_DeleteDynamicVb(GfxResourceID* vb) { Gfx_DeleteVb(vb); }


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

void Gfx_SetAlphaTest(cc_bool enabled) {
	gfx_alphaTesting = enabled;
	FP_SwitchActive();
}
 
void Gfx_SetAlphaBlending(cc_bool enabled) {
	gfx_alphaBlending = enabled;
	FP_SwitchActive();
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	// TODO
	gfx_depthOnly = depthOnly;
	FP_SwitchActive();
}

void Gfx_SetFaceCulling(cc_bool enabled) { 
	sceGxmSetCullMode(gxm_context, enabled ? SCE_GXM_CULL_CW : SCE_GXM_CULL_NONE);
}


void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

static PackedCol clear_color;
void Gfx_ClearCol(PackedCol color) {
	clear_color = color;
}

void Gfx_SetColWriteMask(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
 // TODO
}

static cc_bool depth_write = true, depth_test = true;
static void UpdateDepthWrite(void) {
	// match Desktop behaviour, where disabling depth testing also disables depth writing
	// TODO do we actually need to & here?
	cc_bool enabled = depth_write & depth_test;
	
	int mode = enabled ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED;
	sceGxmSetFrontDepthWriteEnable(gxm_context, mode);
	sceGxmSetBackDepthWriteEnable(gxm_context,  mode);
}

static void UpdateDepthFunction(void) {
	int func = depth_test ? SCE_GXM_DEPTH_FUNC_LESS_EQUAL : SCE_GXM_DEPTH_FUNC_ALWAYS;
	sceGxmSetFrontDepthFunc(gxm_context, func);
	sceGxmSetBackDepthFunc(gxm_context,  func);
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	depth_write = enabled;
	UpdateDepthWrite();
}

void Gfx_SetDepthTest(cc_bool enabled) {
	depth_test = enabled;
	UpdateDepthWrite();
	UpdateDepthFunction();
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static struct Matrix _view, _proj;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW)       _view = *matrix;
	if (type == MATRIX_PROJECTION) _proj = *matrix;

	struct Matrix mvp __attribute__((aligned(64)));	
	Matrix_Mul(&mvp, &_view, &_proj);
	float* m = &mvp;
	
	// Transpose matrix
	for (int i = 0; i < 4; i++)
	{
		transposed_mvp[i * 4 + 0] = m[0  + i];
		transposed_mvp[i * 4 + 1] = m[4  + i];
		transposed_mvp[i * 4 + 2] = m[8  + i];
		transposed_mvp[i * 4 + 3] = m[12 + i];
	}
	
	VP_DirtyUniform(VP_UNI_MATRIX);
	VP_ReloadUniforms();
}

void Gfx_LoadIdentityMatrix(MatrixType type) { 
	Gfx_LoadMatrix(type, &Matrix_Identity);
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
	
	VP_SwitchActive();
	FP_SwitchActive();
}

void Gfx_DrawVb_Lines(int verticesCount) {
 // TODO
}

// TODO probably wrong to offset index buffer
void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
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
void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	//Platform_Log2("DRAW3: %i, %i", &verticesCount, &startVertex); Thread_Sleep(100);
	sceGxmDraw(gxm_context, SCE_GXM_PRIMITIVE_TRIANGLES,
			SCE_GXM_INDEX_FORMAT_U16, gfx_indices + ICOUNT(startVertex), ICOUNT(verticesCount));
}


void Gfx_Clear(void) {
	static struct GPUBuffer* clearVB;
	if (!clearVB) {
		clearVB = GPUBuffer_Alloc(4 * sizeof(struct VertexColoured));
	}
	
	struct VertexColoured* clear_vertices = clearVB->data;
	clear_vertices[0] = (struct VertexColoured){-1.0f, -1.0f, 1.0f, clear_color };
	clear_vertices[1] = (struct VertexColoured){ 1.0f, -1.0f, 1.0f, clear_color };
	clear_vertices[2] = (struct VertexColoured){ 1.0f,  1.0f, 1.0f, clear_color };
	clear_vertices[3] = (struct VertexColoured){-1.0f,  1.0f, 1.0f, clear_color };
	
	Gfx_SetAlphaTest(false);
	// can't use Gfx_SetDepthTest because that also affects depth writing
	depth_test = false; UpdateDepthFunction();
	
	Gfx_SetVertexFormat(VERTEX_FORMAT_COLOURED);
	Gfx_LoadIdentityMatrix(MATRIX_VIEW);
	Gfx_LoadIdentityMatrix(MATRIX_PROJECTION);
	Gfx_BindVb(clearVB);
	Gfx_DrawVb_IndexedTris(4);
	
	depth_test = true; UpdateDepthFunction();
}
#endif
