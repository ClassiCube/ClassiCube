#define CC_SCRATCH_VBS_ARE_DYNAMIC
#include "Core.h"
#if CC_GFX_BACKEND == CC_GFX_BACKEND_VULKAN
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Window.h"

/* Vulkan 1.1 backend (Windows only for now) */

/* Avoid pointless includes */
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#include <windows.h>

/* Vulkan types/functions are declared manually, so the Vulkan SDK isn't needed to compile */
#include "../misc/vulkan/VulkanCommon.h"
#include "../misc/vulkan/VulkanShaders.h"

/* Functions must be dynamically loaded from vulkan-1.dll */
/* e.g. static VkResult (VKAPI_PTR *vkCreateInstance)(const VkInstanceCreateInfo* pCreateInfo, ...); */
#define VK_FUNC(retType, name, args) static retType (VKAPI_PTR *name) args;
#include "../misc/vulkan/VulkanFuncs.h"

static const struct DynamicLibSym vk_funcs[] = {
	/* e.g. { "vkCreateInstance", (void**)&vkCreateInstance, true }, */
	#undef  VK_FUNC
	#define VK_FUNC(retType, name, args) { DYNAMICLIB_QUOTE(name), (void**)&name, true },
	#include "../misc/vulkan/VulkanFuncs.h"
};

/* Max number of frames the CPU can get ahead of the GPU */
#define FRAMES_IN_FLIGHT 2
#define MAX_SWAP_IMAGES  16
/* Buffers and textures are sub-allocated from large blocks of device memory, */
/*  since some drivers only support up to 4096 allocations in total */
#define MEM_BLOCK_SIZE   (64 * 1024 * 1024)
/* Staging data and dynamic vertices are written into a per frame ring buffer */
#define RING_CHUNK_SIZE  (4  * 1024 * 1024)
#define RING_MAX_KEEP    (32 * 1024 * 1024)
#define RING_MAX_CHUNKS  32
#define DESC_POOL_SETS   512
#define PIPELINE_KEYS    8192
#define WAIT_FOREVER     ((cc_uint64)-1)

#define GARBAGE_BUFFER  0
#define GARBAGE_TEXTURE 1

struct ShaderDesc { const unsigned int* data; int len; };

/* Free region within a block of device memory */
struct MemRange { struct MemRange* next; VkDeviceSize offset, size; };
struct MemBlock {
	struct MemBlock* next;
	struct MemRange* free;
	VkDeviceMemory memory;
	VkBuffer buffer;
	VkDeviceSize size;
	cc_uint32 type;
};
struct MemPool  { struct MemBlock* blocks; VkFlags usage; };
struct MemAlloc { struct MemBlock* block; VkDeviceSize offset, size; };

struct RingChunk { VkBuffer buffer; VkDeviceMemory memory; cc_uint8* data; VkDeviceSize size, used; };
struct Garbage   { int type; void* obj; };

/* Resources are only reused once the GPU has finished rendering the frame they were used in */
struct Frame {
	VkCommandPool pool;
	VkCommandBuffer uploadCmd, drawCmd;
	VkFence fence;
	VkSemaphore acquireSem;
	struct RingChunk chunks[RING_MAX_CHUNKS];
	int numChunks;
	struct Garbage* garbage;
	int numGarbage, maxGarbage;
};

struct VulkanTexture {
	VkImage image;
	VkImageView view;
	struct MemAlloc alloc;
	VkDescriptorPool pool;
	VkDescriptorSet sets[2];
	cc_uint32 levels;
};

/* Dynamic VBs keep a CPU side copy of the vertices, since the game expects the */
/*  contents to persist across frames even when the VB isn't locked again */
struct DynamicVb {
	cc_uint8* data;
	cc_uint32 size, used;
	VkBuffer buffer;
	VkDeviceSize offset;
	cc_uint32 frameId;
};

/* NOTE: Layout must match the push constant blocks in misc/vulkan/VulkanShaderGen.c */
static struct PushConstants {
	struct Matrix mvp;
	float texX, texY, _pad1, _pad2;
	float fogR, fogG, fogB, fogEnd;
	float fogDensity, _pad3, _pad4, _pad5;
} pc;

static VkInstance instance;
static VkPhysicalDevice physDevice;
static VkDevice device;
static VkQueue queue;
static cc_uint32 queueFamily;
static VkSurfaceKHR surface;
static VkPhysicalDeviceProperties props;
static VkPhysicalDeviceMemoryProperties memProps;

static VkFormat colorFormat, depthFormat;
static VkColorSpaceKHR colorSpace;
/* resumePass keeps existing contents, for continuing a frame after taking a screenshot */
static VkRenderPass renderPass, resumePass;

static VkSwapchainKHR swapchain;
static VkExtent2D swapExtent;
static VkFlags swapUsage;
static cc_uint32 swapCount, imageIndex;
static VkImage swapImages[MAX_SWAP_IMAGES];
static VkImageView swapViews[MAX_SWAP_IMAGES];
static VkFramebuffer framebuffers[MAX_SWAP_IMAGES];
static VkSemaphore renderSems[MAX_SWAP_IMAGES];
static VkImage depthImage;
static VkImageView depthView;
static struct MemAlloc depthAlloc;
static cc_bool swapchainDirty;

static struct Frame frames[FRAMES_IN_FLIGHT];
static struct Frame* frame;
static int frameIndex;
static cc_uint32 frameCounter;
static cc_bool frameOpen, imageAcquired, acquirePending, renderPassActive, uploadBegun, drawBegun;

static struct MemPool bufferPool, imagePool;

static VkDescriptorSetLayout setLayout;
static VkPipelineLayout pipelineLayout;
static VkSampler samplers[4];
static VkDescriptorPool* descPools;
static int numDescPools;
static VkShaderModule vsModules[2], fsModules[12];
static VkPipeline pipelines[PIPELINE_KEYS];

static GfxResourceID white_square;
static cc_bool inited;
static float clearColor[4];
static int colorMask = 0x0F;
static cc_bool cullFaces, mipmapsOn;
static VkViewport viewport;
static VkRect2D scissor;
static VkBuffer vbBuffer, ibBuffer;
static VkDeviceSize vbOffset, ibOffset;
static struct VulkanTexture* curTex;
static int boundPipeline;
static cc_bool dirtyViewport, dirtyScissor, dirtyConstants, dirtyTexture, dirtyVb, dirtyIb;
static struct Matrix _view, _proj;

static void RecreateSwapchain(void);
static void Garbage_Collect(struct Frame* f);


static void HandleError(VkResult res, const char* place) {
	if (res == VK_ERROR_DEVICE_LOST) {
		Gfx_LoseContext(" (Vulkan device lost)");
	} else if (res == VK_ERROR_SURFACE_LOST_KHR) {
		Gfx_LoseContext(" (Vulkan surface lost)");
	} else {
		Process_Abort2(res, place);
	}
}


/*########################################################################################################################*
*------------------------------------------------------Device memory------------------------------------------------------*
*#########################################################################################################################*/
/* Tries to find a memory type with required and preferred flags, then just required flags */
static int FindMemoryType(cc_uint32 typeBits, VkFlags required, VkFlags preferred) {
	VkFlags flags, wanted = required | preferred;
	cc_uint32 i;

	for (i = 0; i < memProps.memoryTypeCount; i++)
	{
		flags = memProps.memoryTypes[i].propertyFlags;
		if ((typeBits & (1u << i)) && (flags & wanted) == wanted) return (int)i;
	}
	for (i = 0; i < memProps.memoryTypeCount; i++)
	{
		flags = memProps.memoryTypes[i].propertyFlags;
		if ((typeBits & (1u << i)) && (flags & required) == required) return (int)i;
	}
	return -1;
}


static struct MemBlock* MemBlock_Create(struct MemPool* pool, VkDeviceSize size, cc_uint32 typeBits) {
	VkBufferCreateInfo info   = { 0 };
	VkMemoryAllocateInfo alloc = { 0 };
	VkMemoryRequirements reqs;
	VkDeviceMemory memory = 0;
	VkBuffer buffer = 0;
	struct MemBlock* block;
	int type;

	reqs.size           = size;
	reqs.memoryTypeBits = typeBits;

	if (pool->usage) {
		info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.size        = size;
		info.usage       = pool->usage;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (vkCreateBuffer(device, &info, NULL, &buffer)) return NULL;
		vkGetBufferMemoryRequirements(device, buffer, &reqs);
	}

	type = FindMemoryType(reqs.memoryTypeBits, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc.allocationSize  = reqs.size;
	alloc.memoryTypeIndex = type;

	if (type < 0 || vkAllocateMemory(device, &alloc, NULL, &memory)) {
		if (buffer) vkDestroyBuffer(device, buffer, NULL);
		return NULL;
	}
	if (buffer) vkBindBufferMemory(device, buffer, memory, 0);

	block = (struct MemBlock*)Mem_Alloc(1, sizeof(struct MemBlock), "Vulkan memory block");
	block->free = (struct MemRange*)Mem_Alloc(1, sizeof(struct MemRange), "Vulkan memory range");
	block->free->next   = NULL;
	block->free->offset = 0;
	block->free->size   = size;

	block->memory = memory;
	block->buffer = buffer;
	block->size   = size;
	block->type   = type;
	block->next   = pool->blocks;
	pool->blocks  = block;
	return block;
}

static void MemBlock_Destroy(struct MemBlock* block) {
	struct MemRange* range;
	struct MemRange* next;

	for (range = block->free; range; range = next)
	{
		next = range->next;
		Mem_Free(range);
	}
	if (block->buffer) vkDestroyBuffer(device, block->buffer, NULL);
	vkFreeMemory(device, block->memory, NULL);
	Mem_Free(block);
}

/* Simple first fit allocator */
static cc_bool MemBlock_Alloc(struct MemBlock* block, VkDeviceSize size, VkDeviceSize align, struct MemAlloc* alloc) {
	struct MemRange** link = &block->free;
	struct MemRange* range;
	struct MemRange* tail;
	VkDeviceSize start, end;

	for (range = *link; range; link = &range->next, range = *link)
	{
		start = (range->offset + align - 1) & ~(align - 1);
		end   = range->offset + range->size;
		if (start + size > end) continue;

		if (start + size < end && start == range->offset) {
			range->offset = start + size;
			range->size   = end - range->offset;
		} else if (start + size < end) {
			tail = (struct MemRange*)Mem_Alloc(1, sizeof(struct MemRange), "Vulkan memory range");
			tail->offset = start + size;
			tail->size   = end - tail->offset;
			tail->next   = range->next;
			range->next  = tail;
			range->size  = start - range->offset;
		} else if (start == range->offset) {
			*link = range->next;
			Mem_Free(range);
		} else {
			range->size = start - range->offset;
		}

		alloc->block  = block;
		alloc->offset = start;
		alloc->size   = size;
		return true;
	}
	return false;
}

static cc_bool MemPool_Alloc(struct MemPool* pool, const VkMemoryRequirements* reqs, struct MemAlloc* alloc) {
	VkDeviceSize align = reqs->alignment ? reqs->alignment : 1;
	struct MemBlock* block;

	for (block = pool->blocks; block; block = block->next)
	{
		if (!(reqs->memoryTypeBits & (1u << block->type))) continue;
		if (MemBlock_Alloc(block, reqs->size, align, alloc)) return true;
	}

	block = MemBlock_Create(pool, max(reqs->size, MEM_BLOCK_SIZE), reqs->memoryTypeBits);
	return block && MemBlock_Alloc(block, reqs->size, align, alloc);
}

/* Returns the range to the free list, merging it with any adjacent free ranges */
static void MemAlloc_Free(struct MemPool* pool, struct MemAlloc* alloc) {
	struct MemBlock* block = alloc->block;
	VkDeviceSize offset = alloc->offset, size = alloc->size;
	struct MemBlock** link;
	struct MemRange* prev = NULL;
	struct MemRange* next;
	struct MemRange* range;

	if (!block) return;
	alloc->block = NULL;

	for (next = block->free; next && next->offset < offset; next = next->next) prev = next;

	if (prev && prev->offset + prev->size == offset) {
		prev->size += size;
		if (next && next->offset == offset + size) {
			prev->size += next->size;
			prev->next  = next->next;
			Mem_Free(next);
		}
		range = prev;
	} else if (next && next->offset == offset + size) {
		next->offset = offset;
		next->size  += size;
		range = next;
	} else {
		range = (struct MemRange*)Mem_Alloc(1, sizeof(struct MemRange), "Vulkan memory range");
		range->offset = offset;
		range->size   = size;
		range->next   = next;
		if (prev) { prev->next = range; } else { block->free = range; }
	}

	/* Oversized blocks only ever hold one big resource, so release them once it is freed */
	if (block->size <= MEM_BLOCK_SIZE || range->offset || range->size != block->size) return;
	for (link = &pool->blocks; *link != block; link = &(*link)->next) { }
	*link = block->next;
	MemBlock_Destroy(block);
}

static void MemPool_Destroy(struct MemPool* pool) {
	struct MemBlock* block;
	struct MemBlock* next;

	for (block = pool->blocks; block; block = next)
	{
		next = block->next;
		MemBlock_Destroy(block);
	}
	pool->blocks = NULL;
}

/* Deleted resources are only freed after the GPU is done with them, */
/*  so when out of memory, try again after waiting for the GPU to finish */
static cc_bool AllocMemory(struct MemPool* pool, const VkMemoryRequirements* reqs, struct MemAlloc* alloc) {
	int i;
	if (MemPool_Alloc(pool, reqs, alloc)) return true;

	vkDeviceWaitIdle(device);
	for (i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		if (&frames[i] != frame) Garbage_Collect(&frames[i]);
	}
	return MemPool_Alloc(pool, reqs, alloc);
}


/*########################################################################################################################*
*----------------------------------------------------Frame ring buffers---------------------------------------------------*
*#########################################################################################################################*/
static cc_bool RingChunk_Create(struct RingChunk* chunk, VkDeviceSize size) {
	VkBufferCreateInfo info    = { 0 };
	VkMemoryAllocateInfo alloc = { 0 };
	VkMemoryRequirements reqs;
	void* data;
	int type;

	info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size        = size;
	info.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(device, &info, NULL, &chunk->buffer)) return false;

	vkGetBufferMemoryRequirements(device, chunk->buffer, &reqs);
	type = FindMemoryType(reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0);
	alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc.allocationSize  = reqs.size;
	alloc.memoryTypeIndex = type;

	if (type < 0 || vkAllocateMemory(device, &alloc, NULL, &chunk->memory)) {
		vkDestroyBuffer(device, chunk->buffer, NULL);
		return false;
	}
	vkBindBufferMemory(device, chunk->buffer, chunk->memory, 0);

	if (vkMapMemory(device, chunk->memory, 0, size, 0, &data)) {
		vkDestroyBuffer(device, chunk->buffer, NULL);
		vkFreeMemory(device, chunk->memory, NULL);
		return false;
	}
	chunk->data = (cc_uint8*)data;
	chunk->size = size;
	chunk->used = 0;
	return true;
}

static void RingChunk_Destroy(struct RingChunk* chunk) {
	vkDestroyBuffer(device, chunk->buffer, NULL);
	vkFreeMemory(device, chunk->memory, NULL);
}

/* Allocates temp memory that stays valid until the GPU has finished rendering this frame */
static void* Ring_Alloc(VkDeviceSize size, VkBuffer* buffer, VkDeviceSize* offset) {
	struct RingChunk* chunk = NULL;
	VkDeviceSize start = 0;

	if (frame->numChunks) {
		chunk = &frame->chunks[frame->numChunks - 1];
		start = (chunk->used + 15) & ~(VkDeviceSize)15;
		if (start + size > chunk->size) chunk = NULL;
	}

	if (!chunk) {
		if (frame->numChunks == RING_MAX_CHUNKS) return NULL;
		chunk = &frame->chunks[frame->numChunks];
		if (!RingChunk_Create(chunk, max(size, RING_CHUNK_SIZE))) return NULL;
		frame->numChunks++;
		start = 0;
	}

	chunk->used = start + size;
	*buffer     = chunk->buffer;
	*offset     = start;
	return chunk->data + start;
}

/* Replaces multiple chunks with one bigger chunk, to avoid allocating new chunks every frame */
static void Ring_Reset(struct Frame* f) {
	VkDeviceSize used = 0;
	int i;

	if (f->numChunks <= 1) {
		if (f->numChunks) f->chunks[0].used = 0;
		return;
	}

	for (i = 0; i < f->numChunks; i++)
	{
		used += f->chunks[i].used;
		RingChunk_Destroy(&f->chunks[i]);
	}
	f->numChunks = 0;

	if (used > RING_MAX_KEEP) return;
	if (RingChunk_Create(&f->chunks[0], max(used, RING_CHUNK_SIZE))) f->numChunks = 1;
}


/*########################################################################################################################*
*----------------------------------------------------Deferred deletion----------------------------------------------------*
*#########################################################################################################################*/
/* Resources might still be in use by the GPU when the game deletes them, so they are */
/*  only actually destroyed once the frame they were deleted in has finished rendering */
static void FreeTexture(struct VulkanTexture* tex) {
	vkFreeDescriptorSets(device, tex->pool, 2, tex->sets);
	vkDestroyImageView(device, tex->view, NULL);
	vkDestroyImage(device, tex->image, NULL);
	MemAlloc_Free(&imagePool, &tex->alloc);
	Mem_Free(tex);
}

static void Garbage_Add(int type, void* obj) {
	struct Frame* f = frame;

	if (f->numGarbage == f->maxGarbage) {
		f->maxGarbage = f->maxGarbage ? f->maxGarbage * 2 : 64;
		f->garbage    = f->garbage ?
			(struct Garbage*)Mem_Realloc(f->garbage, f->maxGarbage, sizeof(struct Garbage), "Vulkan garbage") :
			(struct Garbage*)Mem_Alloc(f->maxGarbage, sizeof(struct Garbage), "Vulkan garbage");
	}
	f->garbage[f->numGarbage].type = type;
	f->garbage[f->numGarbage].obj  = obj;
	f->numGarbage++;
}

static void Garbage_Collect(struct Frame* f) {
	struct MemAlloc* alloc;
	int i;

	for (i = 0; i < f->numGarbage; i++)
	{
		if (f->garbage[i].type == GARBAGE_TEXTURE) {
			FreeTexture((struct VulkanTexture*)f->garbage[i].obj);
		} else {
			alloc = (struct MemAlloc*)f->garbage[i].obj;
			MemAlloc_Free(&bufferPool, alloc);
			Mem_Free(alloc);
		}
	}
	f->numGarbage = 0;
}


/*########################################################################################################################*
*-----------------------------------------------------Command buffers-----------------------------------------------------*
*#########################################################################################################################*/
static void BeginCmd(VkCommandBuffer cmd) {
	VkCommandBufferBeginInfo info = { 0 };
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &info);
}

/* Copies are recorded into a separate command buffer, since textures/buffers can be */
/*  updated at any time (even in the middle of a render pass). This command buffer */
/*  is submitted before the draw command buffer, so copies always happen before drawing */
static VkCommandBuffer UploadCmd(void) {
	if (uploadBegun) return frame->uploadCmd;
	uploadBegun = true;

	BeginCmd(frame->uploadCmd);
	vkCmdPipelineBarrier(frame->uploadCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
							0, 0, NULL, 0, NULL, 0, NULL);
	return frame->uploadCmd;
}

static void EndUploadCmd(void) {
	VkMemoryBarrier barrier = { 0 };
	barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(frame->uploadCmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
							VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
							0, 1, &barrier, 0, NULL, 0, NULL);
	vkEndCommandBuffer(frame->uploadCmd);
}

static void ImageBarrier(VkCommandBuffer cmd, VkImage image, cc_uint32 levels,
						VkImageLayout oldLayout, VkImageLayout newLayout,
						VkFlags srcStage, VkFlags srcAccess, VkFlags dstStage, VkFlags dstAccess) {
	VkImageMemoryBarrier barrier = { 0 };
	barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask       = srcAccess;
	barrier.dstAccessMask       = dstAccess;
	barrier.oldLayout           = oldLayout;
	barrier.newLayout           = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image               = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = levels;
	barrier.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, NULL, 0, NULL, 1, &barrier);
}


/*########################################################################################################################*
*------------------------------------------------------Initialisation-----------------------------------------------------*
*#########################################################################################################################*/
static void LoadVulkanLibrary(void) {
	static const cc_string path = String_FromConst("vulkan-1.dll");
	void* lib;

	if (DynamicLib_LoadAll(&path, vk_funcs, Array_Elems(vk_funcs), &lib)) return;
	Logger_FailToStart("Failed to load vulkan-1.dll. You may need to update your graphics drivers.\n\nNOTE: Vulkan requires a GPU with Vulkan 1.1 support\nYou may need to use the Direct3D9 version instead.\n");
}

static void CreateInstance(void) {
	static const char* const extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
	VkApplicationInfo app     = { 0 };
	VkInstanceCreateInfo info = { 0 };
	VkResult res;

	app.sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app.pApplicationName = "ClassiCube";
	app.pEngineName      = "ClassiCube";
	/* 1.1 is needed for negative viewport height (see Gfx_SetViewport) */
	app.apiVersion       = VK_API_VERSION_1_1;

	info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	info.pApplicationInfo        = &app;
	info.enabledExtensionCount   = Array_Elems(extensions);
	info.ppEnabledExtensionNames = extensions;

	res = vkCreateInstance(&info, NULL, &instance);
	if (res == VK_ERROR_INCOMPATIBLE_DRIVER)
		Logger_FailToStart("Your graphics drivers do not support Vulkan 1.1.\n\nYou may need to use the Direct3D9 version instead.\n");
	if (res) Process_Abort2(res, "Failed to create Vulkan instance");
}

/* TODO: Support other window backends besides Win32 */
static void CreateSurface(void) {
	VkWin32SurfaceCreateInfoKHR info = { 0 };
	VkResult res;

	info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	info.hinstance = GetModuleHandleW(NULL);
	info.hwnd      = Window_Main.Handle.ptr;

	res = vkCreateWin32SurfaceKHR(instance, &info, NULL, &surface);
	if (res) Process_Abort2(res, "Failed to create Vulkan surface");
}

static cc_bool FindQueueFamily(VkPhysicalDevice dev, cc_uint32* family) {
	VkQueueFamilyProperties families[16];
	cc_uint32 i, count = Array_Elems(families);
	VkBool32 present;

	vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, families);
	for (i = 0; i < count; i++)
	{
		if (!(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) continue;

		present = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &present);
		if (!present) continue;

		*family = i;
		return true;
	}
	return false;
}

/* Dedicated GPUs are preferred over integrated GPUs */
static int RateDevice(VkPhysicalDevice dev) {
	VkPhysicalDeviceProperties p;
	cc_uint32 family;

	vkGetPhysicalDeviceProperties(dev, &p);
	if (p.apiVersion < VK_API_VERSION_1_1) return 0;
	if (!FindQueueFamily(dev, &family))    return 0;

	if (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)   return 3;
	if (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) return 2;
	return 1;
}

static void PickPhysicalDevice(void) {
	VkPhysicalDevice devices[16];
	cc_uint32 i, count = Array_Elems(devices);
	int score, best = 0;

	vkEnumeratePhysicalDevices(instance, &count, devices);
	for (i = 0; i < count; i++)
	{
		score = RateDevice(devices[i]);
		if (score <= best) continue;

		best       = score;
		physDevice = devices[i];
	}

	if (!best) Logger_FailToStart("Failed to find a GPU that supports Vulkan 1.1.\n\nYou may need to use the Direct3D9 version instead.\n");
	FindQueueFamily(physDevice, &queueFamily);
	vkGetPhysicalDeviceProperties(physDevice, &props);
	vkGetPhysicalDeviceMemoryProperties(physDevice, &memProps);
}

static void CreateDevice(void) {
	static const char* const extensions[] = { "VK_KHR_swapchain" };
	static const float priority = 1.0f;
	VkDeviceQueueCreateInfo queueInfo = { 0 };
	VkDeviceCreateInfo info = { 0 };
	VkResult res;

	queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.queueFamilyIndex = queueFamily;
	queueInfo.queueCount       = 1;
	queueInfo.pQueuePriorities = &priority;

	info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	info.queueCreateInfoCount    = 1;
	info.pQueueCreateInfos       = &queueInfo;
	info.enabledExtensionCount   = Array_Elems(extensions);
	info.ppEnabledExtensionNames = extensions;

	res = vkCreateDevice(physDevice, &info, NULL, &device);
	if (res) Process_Abort2(res, "Failed to create Vulkan device");
	vkGetDeviceQueue(device, queueFamily, 0, &queue);
}

/* B8G8R8A8 is preferred since it matches BitmapCol, so screenshots don't need converting */
/* For depth, 32 bit float gives the best precision with a reversed depth buffer */
static void ChooseFormats(void) {
	static const VkFormat depthFormats[] = {
		VK_FORMAT_D32_SFLOAT, VK_FORMAT_X8_D24_UNORM_PACK32, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM
	};
	VkSurfaceFormatKHR formats[64];
	VkFormatProperties fmtProps;
	cc_uint32 i, count = Array_Elems(formats);

	vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &count, formats);
	colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
	colorSpace  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

	if (count && formats[0].format != VK_FORMAT_UNDEFINED) {
		colorFormat = formats[0].format;
		colorSpace  = formats[0].colorSpace;
	}

	for (i = 0; i < count; i++)
	{
		if (formats[i].colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) continue;
		if (formats[i].format != VK_FORMAT_B8G8R8A8_UNORM && formats[i].format != VK_FORMAT_R8G8B8A8_UNORM) continue;

		colorFormat = formats[i].format;
		colorSpace  = formats[i].colorSpace;
		if (colorFormat == VK_FORMAT_B8G8R8A8_UNORM) break;
	}

	for (i = 0; i < Array_Elems(depthFormats); i++)
	{
		vkGetPhysicalDeviceFormatProperties(physDevice, depthFormats[i], &fmtProps);
		if (!(fmtProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)) continue;

		depthFormat = depthFormats[i];
		return;
	}
	Process_Abort("No supported Vulkan depth buffer format");
}

/* Normal render pass doesn't load existing contents, since Gfx_ClearBuffers is always called first */
/*  Resume render pass loads existing contents, for continuing a frame after taking a screenshot */
static VkRenderPass CreateRenderPass(cc_bool resume) {
	VkAttachmentDescription attachments[2] = { { 0 } };
	VkSubpassDependency deps[2]       = { { 0 } };
	VkSubpassDescription subpass      = { 0 };
	VkRenderPassCreateInfo info       = { 0 };
	VkAttachmentReference colorRef, depthRef;
	VkRenderPass pass;
	VkResult res;

	attachments[0].format         = colorFormat;
	attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp         = resume ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout  = resume ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachments[1].format         = depthFormat;
	attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp         = resume ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout  = resume ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	colorRef.attachment = 0;
	colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	depthRef.attachment = 1;
	depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount    = 1;
	subpass.pColorAttachments       = &colorRef;
	subpass.pDepthStencilAttachment = &depthRef;

	/* Depth buffer is shared between frames, so wait for previous frame to finish writing to it */
	deps[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
	deps[0].dstSubpass    = 0;
	deps[0].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	deps[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	deps[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
							VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	/* Allows copying the swapchain image afterwards for screenshots */
	deps[1].srcSubpass    = 0;
	deps[1].dstSubpass    = VK_SUBPASS_EXTERNAL;
	deps[1].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	deps[1].dstStageMask  = VK_PIPELINE_STAGE_TRANSFER_BIT;
	deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	deps[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = 2;
	info.pAttachments    = attachments;
	info.subpassCount    = 1;
	info.pSubpasses      = &subpass;
	info.dependencyCount = 2;
	info.pDependencies   = deps;

	res = vkCreateRenderPass(device, &info, NULL, &pass);
	if (res) Process_Abort2(res, "Failed to create Vulkan render pass");
	return pass;
}

static void CreateFrames(void) {
	VkCommandPoolCreateInfo poolInfo       = { 0 };
	VkCommandBufferAllocateInfo allocInfo  = { 0 };
	VkFenceCreateInfo fenceInfo            = { 0 };
	VkSemaphoreCreateInfo semInfo          = { 0 };
	VkCommandBuffer cmds[2];
	struct Frame* f;
	VkResult res;
	int i;

	poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	poolInfo.queueFamilyIndex = queueFamily;

	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 2;

	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	semInfo.sType   = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		f   = &frames[i];
		res = vkCreateCommandPool(device, &poolInfo, NULL, &f->pool);
		if (res) Process_Abort2(res, "Failed to create Vulkan command pool");

		allocInfo.commandPool = f->pool;
		res = vkAllocateCommandBuffers(device, &allocInfo, cmds);
		if (res) Process_Abort2(res, "Failed to allocate Vulkan command buffers");
		f->uploadCmd = cmds[0];
		f->drawCmd   = cmds[1];

		res = vkCreateFence(device, &fenceInfo, NULL, &f->fence);
		if (res) Process_Abort2(res, "Failed to create Vulkan fence");
		res = vkCreateSemaphore(device, &semInfo, NULL, &f->acquireSem);
		if (res) Process_Abort2(res, "Failed to create Vulkan semaphore");
	}

	frameIndex = 0;
	frame      = &frames[0];
	/* Fences start signalled so NextFrame doesn't wait forever, except for the current frame */
	vkWaitForFences(device, 1, &frame->fence, true, WAIT_FOREVER);
	vkResetFences(device, 1, &frame->fence);
}

static void DestroyFrame(struct Frame* f) {
	int i;
	Garbage_Collect(f);
	for (i = 0; i < f->numChunks; i++) RingChunk_Destroy(&f->chunks[i]);

	vkDestroyCommandPool(device, f->pool, NULL);
	vkDestroyFence(device, f->fence, NULL);
	vkDestroySemaphore(device, f->acquireSem, NULL);
	Mem_Free(f->garbage);
	Mem_Set(f, 0, sizeof(struct Frame));
}


/*########################################################################################################################*
*--------------------------------------------------------Swapchain--------------------------------------------------------*
*#########################################################################################################################*/
static void DestroySwapchainResources(void) {
	cc_uint32 i;
	for (i = 0; i < swapCount; i++)
	{
		vkDestroyFramebuffer(device, framebuffers[i], NULL);
		vkDestroyImageView(device, swapViews[i], NULL);
		vkDestroySemaphore(device, renderSems[i], NULL);
	}
	swapCount = 0;

	/* Depth buffer doesn't exist when the window was minimised */
	if (depthView)  vkDestroyImageView(device, depthView, NULL);
	if (depthImage) vkDestroyImage(device, depthImage, NULL);
	MemAlloc_Free(&imagePool, &depthAlloc);
	depthView  = 0;
	depthImage = 0;
}

/* Immediate is preferred over mailbox when not using vsync, since mailbox */
/*  is still capped to a multiple of the refresh rate by some drivers */
static VkPresentModeKHR ChoosePresentMode(void) {
	VkPresentModeKHR modes[16];
	VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;
	cc_uint32 i, count = Array_Elems(modes);
	if (gfx_vsync) return mode;

	vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &count, modes);
	for (i = 0; i < count; i++)
	{
		if (modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) return modes[i];
		if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)   mode = modes[i];
	}
	return mode;
}

static void CreateDepthBuffer(void) {
	VkImageCreateInfo info         = { 0 };
	VkImageViewCreateInfo viewInfo = { 0 };
	VkMemoryRequirements reqs;
	VkResult res;

	info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.imageType     = VK_IMAGE_TYPE_2D;
	info.format        = depthFormat;
	info.extent.width  = swapExtent.width;
	info.extent.height = swapExtent.height;
	info.extent.depth  = 1;
	info.mipLevels     = 1;
	info.arrayLayers   = 1;
	info.samples       = VK_SAMPLE_COUNT_1_BIT;
	info.tiling        = VK_IMAGE_TILING_OPTIMAL;
	info.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	res = vkCreateImage(device, &info, NULL, &depthImage);
	if (res) Process_Abort2(res, "Failed to create Vulkan depth buffer");

	vkGetImageMemoryRequirements(device, depthImage, &reqs);
	if (!AllocMemory(&imagePool, &reqs, &depthAlloc)) Process_Abort("Out of video memory! (allocating depth buffer)");
	vkBindImageMemory(device, depthImage, depthAlloc.block->memory, depthAlloc.offset);

	viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image    = depthImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format   = depthFormat;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;
	if (depthFormat == VK_FORMAT_D24_UNORM_S8_UINT) viewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

	res = vkCreateImageView(device, &viewInfo, NULL, &depthView);
	if (res) Process_Abort2(res, "Failed to create Vulkan depth buffer view");
}

static void CreateSwapchain(void) {
	VkSwapchainCreateInfoKHR info  = { 0 };
	VkImageViewCreateInfo viewInfo = { 0 };
	VkFramebufferCreateInfo fbInfo = { 0 };
	VkSemaphoreCreateInfo semInfo  = { 0 };
	VkSwapchainKHR old = swapchain;
	VkSurfaceCapabilitiesKHR caps;
	VkImageView attachments[2];
	cc_uint32 i, count;
	VkResult res;

	swapchainDirty = false;
	swapchain      = 0;

	res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &caps);
	if (res) {
		if (old) vkDestroySwapchainKHR(device, old, NULL);
		HandleError(res, "Failed to query Vulkan surface");
		return;
	}

	swapExtent = caps.currentExtent;
	/* 0xFFFFFFFF means the swapchain size determines the surface size */
	if (swapExtent.width == 0xFFFFFFFFu) {
		swapExtent.width  = Game.Width;
		swapExtent.height = Game.Height;
		Math_Clamp(swapExtent.width,  caps.minImageExtent.width,  caps.maxImageExtent.width);
		Math_Clamp(swapExtent.height, caps.minImageExtent.height, caps.maxImageExtent.height);
	}
	/* Swapchain can't be created while the window is minimised */
	if (!swapExtent.width || !swapExtent.height) {
		if (old) vkDestroySwapchainKHR(device, old, NULL);
		return;
	}

	count = caps.minImageCount + 1;
	if (caps.maxImageCount && count > caps.maxImageCount) count = caps.maxImageCount;
	swapUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | (caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

	info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface          = surface;
	info.minImageCount    = count;
	info.imageFormat      = colorFormat;
	info.imageColorSpace  = colorSpace;
	info.imageExtent      = swapExtent;
	info.imageArrayLayers = 1;
	info.imageUsage       = swapUsage;
	info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.preTransform     = caps.currentTransform;
	info.compositeAlpha   = (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) ?
								VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	info.presentMode      = ChoosePresentMode();
	info.clipped          = true;
	info.oldSwapchain     = old;

	res = vkCreateSwapchainKHR(device, &info, NULL, &swapchain);
	/* NOTE: Some implicit layers (e.g. Bandicam) crash if a NULL swapchain is destroyed, */
	/*  even though destroying NULL handles is valid in Vulkan */
	if (old) vkDestroySwapchainKHR(device, old, NULL);
	if (res) { swapchain = 0; HandleError(res, "Failed to create Vulkan swapchain"); return; }

	count = MAX_SWAP_IMAGES;
	vkGetSwapchainImagesKHR(device, swapchain, &count, swapImages);
	CreateDepthBuffer();

	viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format   = colorFormat;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;

	fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbInfo.renderPass      = renderPass;
	fbInfo.attachmentCount = 2;
	fbInfo.pAttachments    = attachments;
	fbInfo.width           = swapExtent.width;
	fbInfo.height          = swapExtent.height;
	fbInfo.layers          = 1;
	semInfo.sType          = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (i = 0; i < count; i++)
	{
		viewInfo.image = swapImages[i];
		res = vkCreateImageView(device, &viewInfo, NULL, &swapViews[i]);
		if (res) Process_Abort2(res, "Failed to create Vulkan swapchain view");

		attachments[0] = swapViews[i];
		attachments[1] = depthView;
		res = vkCreateFramebuffer(device, &fbInfo, NULL, &framebuffers[i]);
		if (res) Process_Abort2(res, "Failed to create Vulkan framebuffer");

		res = vkCreateSemaphore(device, &semInfo, NULL, &renderSems[i]);
		if (res) Process_Abort2(res, "Failed to create Vulkan semaphore");
	}
	swapCount = count;
}

static void RecreateSwapchain(void) {
	vkDeviceWaitIdle(device);
	DestroySwapchainResources();
	CreateSwapchain();
}


/*########################################################################################################################*
*--------------------------------------------------------Pipelines--------------------------------------------------------*
*#########################################################################################################################*/
/* SPIR-V is generated using misc/vulkan/VulkanShaderGen.c */
static const struct ShaderDesc vs_descs[] = {
	{ vs_colored,  sizeof(vs_colored)  },
	{ vs_textured, sizeof(vs_textured) },
};
static const struct ShaderDesc fs_descs[] = {
	{ fs_colored,               sizeof(fs_colored)               },
	{ fs_textured,              sizeof(fs_textured)              },
	{ fs_colored_test,          sizeof(fs_colored_test)          },
	{ fs_textured_test,         sizeof(fs_textured_test)         },
	{ fs_colored_linear,        sizeof(fs_colored_linear)        },
	{ fs_textured_linear,       sizeof(fs_textured_linear)       },
	{ fs_colored_test_linear,   sizeof(fs_colored_test_linear)   },
	{ fs_textured_test_linear,  sizeof(fs_textured_test_linear)  },
	{ fs_colored_density,       sizeof(fs_colored_density)       },
	{ fs_textured_density,      sizeof(fs_textured_density)      },
	{ fs_colored_test_density,  sizeof(fs_colored_test_density)  },
	{ fs_textured_test_density, sizeof(fs_textured_test_density) },
};

static VkShaderModule CreateShader(const struct ShaderDesc* desc) {
	VkShaderModuleCreateInfo info = { 0 };
	VkShaderModule module;
	VkResult res;

	info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	info.codeSize = desc->len;
	info.pCode    = desc->data;

	res = vkCreateShaderModule(device, &info, NULL, &module);
	if (res) Process_Abort2(res, "Failed to create Vulkan shader");
	return module;
}

static void CreatePipelineObjects(void) {
	VkDescriptorSetLayoutBinding binding   = { 0 };
	VkDescriptorSetLayoutCreateInfo setInfo = { 0 };
	VkPipelineLayoutCreateInfo layoutInfo  = { 0 };
	VkSamplerCreateInfo samplerInfo        = { 0 };
	VkPushConstantRange range;
	VkResult res;
	int i;

	for (i = 0; i < Array_Elems(vsModules); i++) vsModules[i] = CreateShader(&vs_descs[i]);
	for (i = 0; i < Array_Elems(fsModules); i++) fsModules[i] = CreateShader(&fs_descs[i]);

	binding.binding         = 0;
	binding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding.descriptorCount = 1;
	binding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

	setInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setInfo.bindingCount = 1;
	setInfo.pBindings    = &binding;

	res = vkCreateDescriptorSetLayout(device, &setInfo, NULL, &setLayout);
	if (res) Process_Abort2(res, "Failed to create Vulkan descriptor set layout");

	range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	range.offset     = 0;
	range.size       = sizeof(pc);

	layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount         = 1;
	layoutInfo.pSetLayouts            = &setLayout;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges    = &range;

	res = vkCreatePipelineLayout(device, &layoutInfo, NULL, &pipelineLayout);
	if (res) Process_Abort2(res, "Failed to create Vulkan pipeline layout");

	samplerInfo.sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.addressModeU  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.compareOp     = VK_COMPARE_OP_NEVER;
	samplerInfo.maxLod        = VK_LOD_CLAMP_NONE;
	samplerInfo.borderColor   = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

	for (i = 0; i < Array_Elems(samplers); i++)
	{
		/* 0 = nearest, 1 = nearest + mipmaps, 2 = bilinear, 3 = bilinear + mipmaps */
		samplerInfo.magFilter  = (i & 2) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		samplerInfo.minFilter  = (i & 2) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		samplerInfo.mipmapMode = (i & 1) ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;

		res = vkCreateSampler(device, &samplerInfo, NULL, &samplers[i]);
		if (res) Process_Abort2(res, "Failed to create Vulkan sampler");
	}
}

static void DestroyPipelineObjects(void) {
	int i;
	for (i = 0; i < PIPELINE_KEYS; i++)
	{
		if (pipelines[i]) vkDestroyPipeline(device, pipelines[i], NULL);
		pipelines[i] = 0;
	}
	for (i = 0; i < Array_Elems(vsModules); i++) vkDestroyShaderModule(device, vsModules[i], NULL);
	for (i = 0; i < Array_Elems(fsModules); i++) vkDestroyShaderModule(device, fsModules[i], NULL);
	for (i = 0; i < Array_Elems(samplers);  i++) vkDestroySampler(device, samplers[i], NULL);
	for (i = 0; i < numDescPools; i++) vkDestroyDescriptorPool(device, descPools[i], NULL);

	Mem_Free(descPools);
	descPools    = NULL;
	numDescPools = 0;
	vkDestroyPipelineLayout(device, pipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(device, setLayout, NULL);
}

/* Pipelines are created on demand for each combination of render state (see CalcPipelineKey) */
static VkPipeline CreatePipeline(int key) {
	static const VkDynamicState dynamics[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineShaderStageCreateInfo stages[2]            = { { 0 } };
	VkVertexInputAttributeDescription attribs[3]         = { { 0 } };
	VkVertexInputBindingDescription binding              = { 0 };
	VkPipelineVertexInputStateCreateInfo vertexInput     = { 0 };
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = { 0 };
	VkPipelineViewportStateCreateInfo viewportState      = { 0 };
	VkPipelineRasterizationStateCreateInfo raster        = { 0 };
	VkPipelineMultisampleStateCreateInfo multisample     = { 0 };
	VkPipelineDepthStencilStateCreateInfo depth          = { 0 };
	VkPipelineColorBlendAttachmentState blendAttachment  = { 0 };
	VkPipelineColorBlendStateCreateInfo blend            = { 0 };
	VkPipelineDynamicStateCreateInfo dynamic             = { 0 };
	VkGraphicsPipelineCreateInfo info                    = { 0 };
	VkPipeline pipeline;
	VkResult res;
	int fmt = key & 1;

	stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = vsModules[fmt];
	stages[0].pName  = "main";
	stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = fsModules[key & 0x0F];
	stages[1].pName  = "main";

	binding.stride    = strideSizes[fmt];
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	attribs[0].location = 0; attribs[0].format = VK_FORMAT_R32G32B32_SFLOAT; attribs[0].offset = 0;
	attribs[1].location = 1; attribs[1].format = VK_FORMAT_R8G8B8A8_UNORM;   attribs[1].offset = 12;
	attribs[2].location = 2; attribs[2].format = VK_FORMAT_R32G32_SFLOAT;    attribs[2].offset = 16;

	vertexInput.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount   = 1;
	vertexInput.pVertexBindingDescriptions      = &binding;
	vertexInput.vertexAttributeDescriptionCount = fmt ? 3 : 2;
	vertexInput.pVertexAttributeDescriptions    = attribs;

	inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = (key & 0x1000) ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount  = 1;

	raster.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster.polygonMode = VK_POLYGON_MODE_FILL;
	raster.cullMode    = (key & 0x0800) ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
	raster.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	raster.lineWidth   = 1.0f;

	multisample.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	depth.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth.depthTestEnable  = (key >> 9)  & 1;
	depth.depthWriteEnable = (key >> 10) & 1;
	/* Reversed depth buffer, same as Direct3D11 backend */
	depth.depthCompareOp   = VK_COMPARE_OP_GREATER_OR_EQUAL;

	blendAttachment.blendEnable         = (key >> 4) & 1;
	blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
	blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
	blendAttachment.colorWriteMask      = (key >> 5) & 0x0F;

	blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend.attachmentCount = 1;
	blend.pAttachments    = &blendAttachment;

	dynamic.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic.dynamicStateCount = Array_Elems(dynamics);
	dynamic.pDynamicStates    = dynamics;

	info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	info.stageCount          = 2;
	info.pStages             = stages;
	info.pVertexInputState   = &vertexInput;
	info.pInputAssemblyState = &inputAssembly;
	info.pViewportState      = &viewportState;
	info.pRasterizationState = &raster;
	info.pMultisampleState   = &multisample;
	info.pDepthStencilState  = &depth;
	info.pColorBlendState    = &blend;
	info.pDynamicState       = &dynamic;
	info.layout              = pipelineLayout;
	info.renderPass          = renderPass;
	info.basePipelineIndex   = -1;

	res = vkCreateGraphicsPipelines(device, 0, 1, &info, NULL, &pipeline);
	if (res) Process_Abort2(res, "Failed to create Vulkan pipeline");
	return pipeline;
}


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_Create(void) {
	LoadVulkanLibrary();
	CreateInstance();
	CreateSurface();
	PickPhysicalDevice();
	CreateDevice();
	ChooseFormats();

	renderPass = CreateRenderPass(false);
	resumePass = CreateRenderPass(true);
	CreateFrames();
	CreatePipelineObjects();

	bufferPool.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	imagePool.usage  = 0;
	CreateSwapchain();

	Gfx.MaxTexWidth  = props.limits.maxImageDimension2D;
	Gfx.MaxTexHeight = props.limits.maxImageDimension2D;
	Gfx.Created         = true;
	Gfx.BackendType     = CC_GFX_BACKEND_VULKAN;
	customMipmapsLevels = true;
}

void Gfx_Free(void) {
	int i;
	Gfx_FreeState();
	if (!device) return;
	vkDeviceWaitIdle(device);

	for (i = 0; i < FRAMES_IN_FLIGHT; i++) DestroyFrame(&frames[i]);
	frame = NULL;

	DestroySwapchainResources();
	if (swapchain) vkDestroySwapchainKHR(device, swapchain, NULL);
	swapchain = 0;

	DestroyPipelineObjects();
	vkDestroyRenderPass(device, renderPass, NULL);
	vkDestroyRenderPass(device, resumePass, NULL);
	MemPool_Destroy(&bufferPool);
	MemPool_Destroy(&imagePool);

	vkDestroyDevice(device, NULL);
	vkDestroySurfaceKHR(instance, surface, NULL);
	vkDestroyInstance(instance, NULL);
	device   = NULL;
	surface  = 0;
	instance = NULL;

	frameOpen        = false;
	imageAcquired    = false;
	acquirePending   = false;
	uploadBegun      = false;
	drawBegun        = false;
	swapchainDirty   = false;
	renderPassActive = false;
	vbBuffer = 0;
	ibBuffer = 0;
	curTex   = NULL;
}

cc_bool Gfx_TryRestoreContext(void) {
	Gfx_Free();
	Gfx_Create();
	return true;
}

static void Gfx_FreeState(void) {
	if (!inited) return;
	inited = false;

	FreeDefaultResources();
	Gfx_DeleteTexture(&white_square);
}

static void Gfx_RestoreState(void) {
	struct Bitmap bmp;
	BitmapCol pixels[1] = { BITMAPCOLOR_WHITE };
	if (inited) return;
	inited = true;

	InitDefaultResources();
	gfx_format = -1;

	/* 1x1 dummy white texture */
	Bitmap_Init(bmp, 1, 1, pixels);
	Gfx_RecreateTexture(&white_square, &bmp, 0, false);
	curTex = NULL;

	Gfx_SetViewport(0, 0, Game.Width, Game.Height);
	Gfx_SetScissor( 0, 0, Game.Width, Game.Height);
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static void CreateDescriptorPool(void) {
	VkDescriptorPoolCreateInfo info = { 0 };
	VkDescriptorPoolSize size;
	VkDescriptorPool pool;
	VkResult res;

	size.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	size.descriptorCount = DESC_POOL_SETS;

	info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	info.maxSets       = DESC_POOL_SETS;
	info.poolSizeCount = 1;
	info.pPoolSizes    = &size;

	res = vkCreateDescriptorPool(device, &info, NULL, &pool);
	if (res) Process_Abort2(res, "Failed to create Vulkan descriptor pool");

	descPools = descPools ?
		(VkDescriptorPool*)Mem_Realloc(descPools, numDescPools + 1, sizeof(VkDescriptorPool), "Vulkan descriptor pools") :
		(VkDescriptorPool*)Mem_Alloc(1, sizeof(VkDescriptorPool), "Vulkan descriptor pools");
	descPools[numDescPools++] = pool;
}

/* Each texture has one descriptor set for normal sampling, and one for mipmapped sampling */
static void AllocTextureSets(struct VulkanTexture* tex, int filter) {
	VkDescriptorSetAllocateInfo info = { 0 };
	VkWriteDescriptorSet writes[2]   = { { 0 } };
	VkDescriptorImageInfo images[2];
	VkDescriptorSetLayout layouts[2];
	VkResult res;
	int i;

	layouts[0] = setLayout;
	layouts[1] = setLayout;
	info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info.descriptorSetCount = 2;
	info.pSetLayouts        = layouts;

	for (i = numDescPools - 1; i >= 0; i--)
	{
		info.descriptorPool = descPools[i];
		if (!vkAllocateDescriptorSets(device, &info, tex->sets)) break;
	}

	/* All existing pools are full */
	if (i < 0) {
		CreateDescriptorPool();
		i = numDescPools - 1;
		info.descriptorPool = descPools[i];

		res = vkAllocateDescriptorSets(device, &info, tex->sets);
		if (res) Process_Abort2(res, "Failed to allocate Vulkan descriptor sets");
	}
	tex->pool = descPools[i];

	for (i = 0; i < 2; i++)
	{
		images[i].sampler     = samplers[filter + i];
		images[i].imageView   = tex->view;
		images[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		writes[i].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[i].dstSet          = tex->sets[i];
		writes[i].descriptorCount = 1;
		writes[i].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[i].pImageInfo      = &images[i];
	}
	vkUpdateDescriptorSets(device, 2, writes, 0, NULL);
}

static void CopyTextureLevel(VkCommandBuffer cmd, struct VulkanTexture* tex, int level,
							int x, int y, int width, int height, BitmapCol* pixels, int rowWidth) {
	VkBufferImageCopy region = { 0 };
	VkDeviceSize offset;
	VkBuffer buffer;
	void* data = Ring_Alloc((VkDeviceSize)width * height * BITMAPCOLOR_SIZE, &buffer, &offset);
	if (!data) return;

	CopyPixels(data, width * BITMAPCOLOR_SIZE, pixels, rowWidth * BITMAPCOLOR_SIZE, width, height);
	region.bufferOffset = offset;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel   = level;
	region.imageSubresource.layerCount = 1;
	region.imageOffset.x      = x;
	region.imageOffset.y      = y;
	region.imageExtent.width  = width;
	region.imageExtent.height = height;
	region.imageExtent.depth  = 1;

	vkCmdCopyBufferToImage(cmd, buffer, tex->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

static void UploadTexture(struct VulkanTexture* tex, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps, cc_bool created) {
	VkCommandBuffer cmd = UploadCmd();
	BitmapCol* prev = part->scan0;
	BitmapCol* cur;
	int lvl, lvls = 0;
	int width = part->width, height = part->height;

	ImageBarrier(cmd, tex->image, tex->levels,
		created ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		created ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
	CopyTextureLevel(cmd, tex, 0, x, y, width, height, part->scan0, rowWidth);

	if (mipmaps) lvls = min(CalcMipmapsLevels(width, height), (int)tex->levels - 1);
	for (lvl = 1; lvl <= lvls; lvl++)
	{
		x /= 2; y /= 2;
		if (width  > 1) width  /= 2;
		if (height > 1) height /= 2;

		cur = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "mipmaps");
		GenMipmaps(width, height, cur, prev, rowWidth);
		CopyTextureLevel(cmd, tex, lvl, x, y, width, height, cur, width);

		if (prev != part->scan0) Mem_Free(prev);
		prev     = cur;
		rowWidth = width;
	}
	if (prev != part->scan0) Mem_Free(prev);

	ImageBarrier(cmd, tex->image, tex->levels,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
}

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	VkImageCreateInfo info         = { 0 };
	VkImageViewCreateInfo viewInfo = { 0 };
	VkMemoryRequirements reqs;
	struct VulkanTexture* tex;
	VkResult res;

	tex = (struct VulkanTexture*)Mem_TryAllocCleared(1, sizeof(struct VulkanTexture));
	if (!tex) return NULL;
	tex->levels = mipmaps ? 1 + CalcMipmapsLevels(bmp->width, bmp->height) : 1;

	info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.imageType     = VK_IMAGE_TYPE_2D;
	info.format        = VK_FORMAT_B8G8R8A8_UNORM;
	info.extent.width  = bmp->width;
	info.extent.height = bmp->height;
	info.extent.depth  = 1;
	info.mipLevels     = tex->levels;
	info.arrayLayers   = 1;
	info.samples       = VK_SAMPLE_COUNT_1_BIT;
	info.tiling        = VK_IMAGE_TILING_OPTIMAL;
	info.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (vkCreateImage(device, &info, NULL, &tex->image)) { Mem_Free(tex); return NULL; }
	vkGetImageMemoryRequirements(device, tex->image, &reqs);

	while (!AllocMemory(&imagePool, &reqs, &tex->alloc))
	{
		if (Game_ReduceVRAM()) continue;
		vkDestroyImage(device, tex->image, NULL);
		Mem_Free(tex);
		return NULL;
	}
	vkBindImageMemory(device, tex->image, tex->alloc.block->memory, tex->alloc.offset);

	viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image    = tex->image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format   = VK_FORMAT_B8G8R8A8_UNORM;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.levelCount = tex->levels;
	viewInfo.subresourceRange.layerCount = 1;

	res = vkCreateImageView(device, &viewInfo, NULL, &tex->view);
	if (res) Process_Abort2(res, "Failed to create Vulkan texture view");

	AllocTextureSets(tex, (flags & TEXTURE_FLAG_BILINEAR) ? 2 : 0);
	UploadTexture(tex, 0, 0, bmp, rowWidth, mipmaps, true);
	return tex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	struct VulkanTexture* tex = (struct VulkanTexture*)texId;
	if (tex) UploadTexture(tex, x, y, part, rowWidth, mipmaps, false);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	struct VulkanTexture* tex = (struct VulkanTexture*)(*texId);

	if (tex) {
		if (tex == curTex) { curTex = NULL; dirtyTexture = true; }
		Garbage_Add(GARBAGE_TEXTURE, tex);
	}
	*texId = NULL;
}

/* White texture is used when no texture is bound */
void Gfx_BindTexture(GfxResourceID texId) {
	struct VulkanTexture* tex = (struct VulkanTexture*)(texId ? texId : white_square);
	if (tex == curTex) return;

	curTex       = tex;
	dirtyTexture = true;
}

void Gfx_EnableMipmaps(void) {
	if (!Gfx.Mipmaps) return;
	mipmapsOn    = true;
	dirtyTexture = true;
}

void Gfx_DisableMipmaps(void) {
	if (!Gfx.Mipmaps) return;
	mipmapsOn    = false;
	dirtyTexture = true;
}


/*########################################################################################################################*
*------------------------------------------------------Index buffers------------------------------------------------------*
*#########################################################################################################################*/
/* Static vertex and index buffers are sub-allocated from big device local VkBuffers */
static struct MemAlloc* AllocBuffer(VkDeviceSize size) {
	struct MemAlloc* alloc = (struct MemAlloc*)Mem_TryAllocCleared(1, sizeof(struct MemAlloc));
	VkMemoryRequirements reqs;
	if (!alloc) return NULL;

	reqs.size           = max(size, 16);
	reqs.alignment      = 16;
	reqs.memoryTypeBits = ~0u;
	if (AllocMemory(&bufferPool, &reqs, alloc)) return alloc;

	Mem_Free(alloc);
	return NULL;
}

static void DeleteBuffer(GfxResourceID* id) {
	if (*id) Garbage_Add(GARBAGE_BUFFER, *id);
	*id = NULL;
}

static void CopyToBuffer(VkBuffer src, VkDeviceSize srcOffset, struct MemAlloc* dst, VkDeviceSize size) {
	VkBufferCopy region;
	if (!size) return;

	region.srcOffset = srcOffset;
	region.dstOffset = dst->offset;
	region.size      = size;
	vkCmdCopyBuffer(UploadCmd(), src, dst->block->buffer, 1, &region);
}

GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	VkDeviceSize size = (VkDeviceSize)count * sizeof(cc_uint16);
	struct MemAlloc* ib = AllocBuffer(size);
	VkDeviceSize offset;
	VkBuffer src;
	void* data;

	if (!ib) Process_Abort("Failed to create index buffer");
	data = Ring_Alloc(size, &src, &offset);
	if (!data) Process_Abort("Failed to create index buffer");

	fillFunc((cc_uint16*)data, count, obj);
	CopyToBuffer(src, offset, ib, size);
	return ib;
}

void Gfx_BindIb(GfxResourceID ib) {
	struct MemAlloc* alloc = (struct MemAlloc*)ib;
	ibBuffer = alloc ? alloc->block->buffer : 0;
	ibOffset = alloc ? alloc->offset : 0;
	dirtyIb  = true;
}

void Gfx_DeleteIb(GfxResourceID* ib) { DeleteBuffer(ib); }


/*########################################################################################################################*
*------------------------------------------------------Vertex buffers-----------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	return AllocBuffer((VkDeviceSize)count * strideSizes[fmt]);
}

void Gfx_BindVb(GfxResourceID vb) {
	struct MemAlloc* alloc = (struct MemAlloc*)vb;
	vbBuffer = alloc ? alloc->block->buffer : 0;
	vbOffset = alloc ? alloc->offset : 0;
	dirtyVb  = true;
}

void Gfx_DeleteVb(GfxResourceID* vb) { DeleteBuffer(vb); }

static VkBuffer lockBuffer;
static VkDeviceSize lockOffset, lockSize;

/* Vertices are written to staging memory, then copied into the VB when unlocked */
void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	void* data;
	lockSize = (VkDeviceSize)count * strideSizes[fmt];
	data     = Ring_Alloc(lockSize, &lockBuffer, &lockOffset);

	if (!data) lockSize = 0;
	return data;
}

void Gfx_UnlockVb(GfxResourceID vb) {
	struct MemAlloc* alloc = (struct MemAlloc*)vb;
	CopyToBuffer(lockBuffer, lockOffset, alloc, min(lockSize, alloc->size));
}


/*########################################################################################################################*
*--------------------------------------------------Dynamic vertex buffers-------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Gfx_AllocDynamicVb(VertexFormat fmt, int maxVertices) {
	struct DynamicVb* vb = (struct DynamicVb*)Mem_TryAllocCleared(1, sizeof(struct DynamicVb));
	if (!vb) return NULL;

	vb->size    = maxVertices * strideSizes[fmt];
	vb->frameId = frameCounter - 1;
	vb->data    = (cc_uint8*)Mem_TryAlloc(max(vb->size, 16), 1);
	if (vb->data) return vb;

	Mem_Free(vb);
	return NULL;
}

void Gfx_DeleteDynamicVb(GfxResourceID* vb) {
	struct DynamicVb* dyn = (struct DynamicVb*)(*vb);
	if (dyn) { Mem_Free(dyn->data); Mem_Free(dyn); }
	*vb = NULL;
}

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	struct DynamicVb* dyn = (struct DynamicVb*)vb;
	dyn->used = min((cc_uint32)(count * strideSizes[fmt]), dyn->size);
	return dyn->data;
}

static void UploadDynamicVb(struct DynamicVb* dyn) {
	void* dst = Ring_Alloc(max(dyn->used, 16), &dyn->buffer, &dyn->offset);
	if (!dst) { dyn->buffer = 0; return; }

	Mem_Copy(dst, dyn->data, dyn->used);
	dyn->frameId = frameCounter;
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) {
	UploadDynamicVb((struct DynamicVb*)vb);
	Gfx_BindDynamicVb(vb);
}

void Gfx_BindDynamicVb(GfxResourceID vb) {
	struct DynamicVb* dyn = (struct DynamicVb*)vb;
	if (!dyn) return;
	/* Data from a previous frame may have been overwritten in the ring buffer already */
	if (dyn->frameId != frameCounter) UploadDynamicVb(dyn);

	vbBuffer = dyn->buffer;
	vbOffset = dyn->offset;
	dirtyVb  = true;
}


/*########################################################################################################################*
*------------------------------------------------------Vertex drawing-----------------------------------------------------*
*#########################################################################################################################*/
/* Bound state is lost when starting a new command buffer */
static void InvalidateState(void) {
	boundPipeline  = -1;
	dirtyViewport  = true;
	dirtyScissor   = true;
	dirtyConstants = true;
	dirtyTexture   = true;
	dirtyVb        = true;
	dirtyIb        = true;
}

static void BeginRenderPass(VkRenderPass pass) {
	VkRenderPassBeginInfo info = { 0 };
	BeginCmd(frame->drawCmd);
	drawBegun = true;
	InvalidateState();

	info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.renderPass        = pass;
	info.framebuffer       = framebuffers[imageIndex];
	info.renderArea.extent = swapExtent;

	vkCmdBeginRenderPass(frame->drawCmd, &info, VK_SUBPASS_CONTENTS_INLINE);
	renderPassActive = true;
}

/* Swapchain image is only acquired once something actually needs to be drawn */
static cc_bool AcquireImage(void) {
	VkResult res;
	if (swapchainDirty) RecreateSwapchain();
	if (!swapchain) return false;

	res = vkAcquireNextImageKHR(device, swapchain, WAIT_FOREVER, frame->acquireSem, 0, &imageIndex);
	if (res == VK_ERROR_OUT_OF_DATE_KHR) {
		RecreateSwapchain();
		if (!swapchain) return false;
		res = vkAcquireNextImageKHR(device, swapchain, WAIT_FOREVER, frame->acquireSem, 0, &imageIndex);
	}

	if (res == VK_SUBOPTIMAL_KHR) { swapchainDirty = true; res = VK_SUCCESS; }
	if (res) { HandleError(res, "Failed to acquire Vulkan swapchain image"); return false; }

	imageAcquired  = true;
	acquirePending = true;
	return true;
}

static cc_bool EnsureRenderPass(void) {
	if (renderPassActive) return true;
	if (Gfx.LostContext)  return false;
	if (!imageAcquired && !AcquireImage()) return false;

	BeginRenderPass(renderPass);
	return true;
}

/* Bit 0 = format, 1 = alpha test, 2-3 = fog, 4 = blending, 5-8 = colour mask, */
/*  9 = depth test, 10 = depth write, 11 = culling, 12 = lines */
static int CalcPipelineKey(cc_bool lines) {
	int fog = 0;
	if (gfx_fogEnabled && gfx_fogMode >= 0) fog = gfx_fogMode == FOG_LINEAR ? 1 : 2;

	return gfx_format | (gfx_alphaTest << 1) | (fog << 2) | (gfx_alphaBlend << 4) | (colorMask << 5) |
		(gfx_depthTest << 9) | (gfx_depthWrite << 10) | (cullFaces << 11) | (lines << 12);
}

static cc_bool PrepareDraw(cc_bool lines) {
	struct VulkanTexture* tex = curTex ? curTex : (struct VulkanTexture*)white_square;
	cc_bool textured = gfx_format == VERTEX_FORMAT_TEXTURED;
	VkCommandBuffer cmd;
	int key;

	if (gfx_format < 0 || !vbBuffer)  return false;
	if (!lines && !ibBuffer)         return false;
	if (textured && !tex)            return false;
	if (!EnsureRenderPass())         return false;
	cmd = frame->drawCmd;

	key = CalcPipelineKey(lines);
	if (key != boundPipeline) {
		if (!pipelines[key]) pipelines[key] = CreatePipeline(key);
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[key]);
		boundPipeline = key;
	}

	if (dirtyViewport) {
		vkCmdSetViewport(cmd, 0, 1, &viewport);
		dirtyViewport = false;
	}
	if (dirtyScissor) {
		vkCmdSetScissor(cmd, 0, 1, &scissor);
		dirtyScissor = false;
	}
	if (dirtyConstants) {
		vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
							0, sizeof(pc), &pc);
		dirtyConstants = false;
	}
	if (dirtyTexture && textured) {
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
								0, 1, &tex->sets[mipmapsOn], 0, NULL);
		dirtyTexture = false;
	}
	if (dirtyVb) {
		vkCmdBindVertexBuffers(cmd, 0, 1, &vbBuffer, &vbOffset);
		dirtyVb = false;
	}
	if (dirtyIb && !lines) {
		vkCmdBindIndexBuffer(cmd, ibBuffer, ibOffset, VK_INDEX_TYPE_UINT16);
		dirtyIb = false;
	}
	return true;
}

void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
}

void Gfx_DrawVb_Lines(int verticesCount) {
	if (!PrepareDraw(true)) return;
	vkCmdDraw(frame->drawCmd, verticesCount, 1, 0, 0);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	if (!PrepareDraw(false)) return;
	vkCmdDrawIndexed(frame->drawCmd, ICOUNT(verticesCount), 1, 0, 0, 0);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	if (!PrepareDraw(false)) return;
	vkCmdDrawIndexed(frame->drawCmd, ICOUNT(verticesCount), 1, 0, startVertex, 0);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex, DrawHints hints) {
	if (!PrepareDraw(false)) return;
	vkCmdDrawIndexed(frame->drawCmd, ICOUNT(verticesCount), 1, 0, startVertex, 0);
}


/*########################################################################################################################*
*-----------------------------------------------------Vertex transform----------------------------------------------------*
*#########################################################################################################################*/
/* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixorthooffcenterrh */
/*   The simplified calculation below uses: L = 0, R = width, T = 0, B = height */
/* NOTE: This calculation is shared with Direct3D 9/11 backends */
void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	*matrix = Matrix_Identity;

	matrix->row1.x =  2.0f / width;
	matrix->row2.y = -2.0f / height;
	matrix->row3.z =  1.0f / (zNear - zFar);

	matrix->row4.x = -1.0f;
	matrix->row4.y =  1.0f;
	matrix->row4.z = zNear / (zNear - zFar);
}

static float Cotangent(float x) { return Math_CosF(x) / Math_SinF(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	/* Deliberately swap zNear/zFar in projection matrix calculation to produce */
	/*  a projection matrix that results in a reversed depth buffer */
	/* https://developer.nvidia.com/content/depth-precision-visualized */
	float zNear_ = zFar;
	float zFar_  = Reversed_CalcZNear(fov, 24);
	float c = Cotangent(0.5f * fov);
	*matrix = Matrix_Identity;

	matrix->row1.x =  c / aspect;
	matrix->row2.y =  c;
	matrix->row3.z = zFar_ / (zNear_ - zFar_);
	matrix->row3.w = -1.0f;
	matrix->row4.z = (zNear_ * zFar_) / (zNear_ - zFar_);
	matrix->row4.w =  0.0f;
}

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW) _view = *matrix;
	if (type == MATRIX_PROJ) _proj = *matrix;

	Matrix_Mul(&pc.mvp, &_view, &_proj);
	dirtyConstants = true;
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	Gfx_LoadMatrix(MATRIX_VIEW, view);
	Gfx_LoadMatrix(MATRIX_PROJ, proj);
	Matrix_Mul(mvp, view, proj);
}

void Gfx_EnableTextureOffset(float x, float y) {
	pc.texX = x;
	pc.texY = y;
	dirtyConstants = true;
}

void Gfx_DisableTextureOffset(void) {
	pc.texX = 0;
	pc.texY = 0;
	dirtyConstants = true;
}

/* Negative height flips the viewport vertically, so the same projection matrices as */
/*  Direct3D can be used (since Vulkan clip space Y points down instead of up) */
void Gfx_SetViewport(int x, int y, int w, int h) {
	viewport.x        = (float)x;
	viewport.y        = (float)(y + h);
	viewport.width    = (float)w;
	viewport.height   = -(float)h;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	dirtyViewport     = true;
}

void Gfx_SetScissor(int x, int y, int w, int h) {
	scissor.offset.x      = max(x, 0);
	scissor.offset.y      = max(y, 0);
	scissor.extent.width  = max(w, 0);
	scissor.extent.height = max(h, 0);
	dirtyScissor          = true;
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
/* State is applied when drawing, by selecting the matching pipeline */
static void SetAlphaTest(cc_bool enabled)  { }
static void SetAlphaBlend(cc_bool enabled) { }
static void SetDepthTest(cc_bool enabled)  { }
static void SetDepthWrite(cc_bool enabled) { }
void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	colorMask = r | (g << 1) | (b << 2) | (a << 3);
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	DefaultDepthOnlyRendering(depthOnly);
}

void Gfx_SetFaceCulling(cc_bool enabled) { cullFaces = enabled; }


/*########################################################################################################################*
*-----------------------------------------------------------Fog-----------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFog(cc_bool enabled) { gfx_fogEnabled = enabled; }

static void SetFogColor(PackedCol color) {
	pc.fogR = PackedCol_R(color) / 255.0f;
	pc.fogG = PackedCol_G(color) / 255.0f;
	pc.fogB = PackedCol_B(color) / 255.0f;
	dirtyConstants = true;
}

static void SetFogDensity(float value) {
	/* avoid doing - in fragment shader for density fog */
	pc.fogDensity  = -value;
	dirtyConstants = true;
}

static void SetFogEnd(float value) {
	/* avoid doing division in fragment shader for linear fog */
	pc.fogEnd      = 1.0f / value;
	dirtyConstants = true;
}

static void SetFogMode(FogFunc func) { }


/*########################################################################################################################*
*-----------------------------------------------------Buffer clearing-----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_ClearColor(PackedCol color) {
	clearColor[0] = PackedCol_R(color) / 255.0f;
	clearColor[1] = PackedCol_G(color) / 255.0f;
	clearColor[2] = PackedCol_B(color) / 255.0f;
	clearColor[3] = PackedCol_A(color) / 255.0f;
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
	VkClearAttachment attachments[2];
	VkClearRect rect = { { { 0 } } };
	cc_uint32 count = 0;

	if (!EnsureRenderPass()) return;

	if (buffers & GFX_BUFFER_COLOR) {
		attachments[count].aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
		attachments[count].colorAttachment = 0;
		Mem_Copy(attachments[count].clearValue.color.float32, clearColor, sizeof(clearColor));
		count++;
	}
	if (buffers & GFX_BUFFER_DEPTH) {
		attachments[count].aspectMask      = VK_IMAGE_ASPECT_DEPTH_BIT;
		attachments[count].colorAttachment = 0;
		/* 0 since using a reversed depth buffer */
		attachments[count].clearValue.depthStencil.depth   = 0.0f;
		attachments[count].clearValue.depthStencil.stencil = 0;
		count++;
	}
	if (!count) return;

	rect.rect.extent = swapExtent;
	rect.layerCount  = 1;
	vkCmdClearAttachments(frame->drawCmd, count, attachments, 1, &rect);
}


/*########################################################################################################################*
*-----------------------------------------------------Frame management----------------------------------------------------*
*#########################################################################################################################*/
static VkResult SubmitFrame(cc_bool present) {
	static const VkFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submit = { 0 };
	VkCommandBuffer cmds[2];
	cc_uint32 count = 0;

	if (uploadBegun) { EndUploadCmd(); cmds[count++] = frame->uploadCmd; }
	if (drawBegun)   { vkEndCommandBuffer(frame->drawCmd); cmds[count++] = frame->drawCmd; }

	submit.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.commandBufferCount = count;
	submit.pCommandBuffers    = cmds;

	if (acquirePending) {
		submit.waitSemaphoreCount = 1;
		submit.pWaitSemaphores    = &frame->acquireSem;
		submit.pWaitDstStageMask  = &waitStage;
	}
	if (present) {
		submit.signalSemaphoreCount = 1;
		submit.pSignalSemaphores    = &renderSems[imageIndex];
	}

	uploadBegun    = false;
	drawBegun      = false;
	acquirePending = false;
	return vkQueueSubmit(queue, 1, &submit, frame->fence);
}

static void PresentFrame(void) {
	VkPresentInfoKHR info = { 0 };
	VkResult res;

	info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores    = &renderSems[imageIndex];
	info.swapchainCount     = 1;
	info.pSwapchains        = &swapchain;
	info.pImageIndices      = &imageIndex;

	res = vkQueuePresentKHR(queue, &info);
	if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
		swapchainDirty = true;
	} else if (res) {
		HandleError(res, "Failed to present Vulkan swapchain image");
	}
}

/* Waits until the GPU has finished with the next frame's resources, then resets them */
static void NextFrame(void) {
	frameIndex = (frameIndex + 1) % FRAMES_IN_FLIGHT;
	frame      = &frames[frameIndex];
	frameCounter++;

	vkWaitForFences(device, 1, &frame->fence, true, WAIT_FOREVER);
	vkResetFences(device, 1, &frame->fence);
	vkResetCommandPool(device, frame->pool, 0);
	Garbage_Collect(frame);
	Ring_Reset(frame);
}

/* Game doesn't call Gfx_EndFrame when the window is inactive (see Game_RenderFrame) */
void Gfx_BeginFrame(void) {
	if (frameOpen) Gfx_EndFrame();
	frameOpen = true;
}

void Gfx_EndFrame(void) {
	cc_bool present = imageAcquired;
	VkResult res;

	if (renderPassActive) {
		vkCmdEndRenderPass(frame->drawCmd);
		renderPassActive = false;
	}

	res = SubmitFrame(present);
	if (res) {
		HandleError(res, "Failed to submit Vulkan commands");
	} else if (present) {
		PresentFrame();
	}

	imageAcquired = false;
	frameOpen     = false;
	NextFrame();

	/* run at reduced FPS when minimised */
	if (!swapchain) { TickReducedPerformance(); return; }
	EndReducedPerformance();
}

void Gfx_SetVSync(cc_bool vsync) {
	if (gfx_vsync == vsync) return;
	gfx_vsync      = vsync;
	swapchainDirty = swapchain != 0;
}

/* Swapchain is recreated when the next swapchain image is acquired */
void Gfx_OnWindowResize(int width, int height) {
	swapchainDirty = true;
	Gfx_SetViewport(0, 0, width, height);
	Gfx_SetScissor( 0, 0, width, height);
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
static BitmapCol* Vulkan_GetRow(struct Bitmap* bmp, int y, void* ctx) {
	return (BitmapCol*)((cc_uint8*)ctx + (cc_uint32)y * bmp->width * BITMAPCOLOR_SIZE);
}

/* Screenshots are taken in the middle of a frame, so rendering has to be paused to copy */
/*  the swapchain image, then resumed afterwards with the resume render pass */
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	VkBufferCreateInfo info    = { 0 };
	VkMemoryAllocateInfo alloc = { 0 };
	VkBufferImageCopy region   = { 0 };
	VkMemoryBarrier barrier    = { 0 };
	VkMemoryRequirements reqs;
	VkDeviceMemory memory = 0;
	VkBuffer buffer = 0;
	VkCommandBuffer cmd;
	VkImage image;
	struct Bitmap bmp;
	cc_uint8* data;
	cc_uint8 tmp;
	cc_uint32 i, pixels;
	void* mapped;
	cc_result res;
	int type;

	if (!(swapUsage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) || !EnsureRenderPass()) return ERR_NOT_SUPPORTED;
	pixels = swapExtent.width * swapExtent.height;

	info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size        = (VkDeviceSize)pixels * 4;
	info.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(device, &info, NULL, &buffer)) return ERR_OUT_OF_MEMORY;

	vkGetBufferMemoryRequirements(device, buffer, &reqs);
	type = FindMemoryType(reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
							VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
	alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc.allocationSize  = reqs.size;
	alloc.memoryTypeIndex = type;

	if (type < 0 || vkAllocateMemory(device, &alloc, NULL, &memory)) {
		vkDestroyBuffer(device, buffer, NULL);
		return ERR_OUT_OF_MEMORY;
	}
	vkBindBufferMemory(device, buffer, memory, 0);

	cmd   = frame->drawCmd;
	image = swapImages[imageIndex];
	vkCmdEndRenderPass(cmd);
	renderPassActive = false;

	ImageBarrier(cmd, image, 1, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent.width  = swapExtent.width;
	region.imageExtent.height = swapExtent.height;
	region.imageExtent.depth  = 1;
	vkCmdCopyImageToBuffer(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);

	ImageBarrier(cmd, image, 1, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, NULL, 0, NULL);

	res = SubmitFrame(false);
	if (!res) res = vkWaitForFences(device, 1, &frame->fence, true, WAIT_FOREVER);
	vkResetFences(device, 1, &frame->fence);
	vkResetCommandPool(device, frame->pool, 0);
	BeginRenderPass(resumePass);

	if (!res) res = vkMapMemory(device, memory, 0, info.size, 0, &mapped);
	if (!res) {
		data = (cc_uint8*)mapped;
		/* Swapchain format might be RGBA instead of BGRA */
		if (colorFormat == VK_FORMAT_R8G8B8A8_UNORM) {
			for (i = 0; i < pixels; i++)
			{
				tmp             = data[i * 4 + 0];
				data[i * 4 + 0] = data[i * 4 + 2];
				data[i * 4 + 2] = tmp;
			}
		}

		Bitmap_Init(bmp, swapExtent.width, swapExtent.height, NULL);
		res = Png_Encode(&bmp, output, Vulkan_GetRow, false, data);
	}

	vkDestroyBuffer(device, buffer, NULL);
	vkFreeMemory(device, memory, NULL);
	return res;
}

cc_bool Gfx_WarnIfNecessary(void) { return false; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }

void Gfx_GetApiInfo(cc_string* info) {
	int pointerSize = sizeof(void*) * 8;
	int major = VK_VERSION_MAJOR(props.apiVersion);
	int minor = VK_VERSION_MINOR(props.apiVersion);
	int patch = VK_VERSION_PATCH(props.apiVersion);
	VkDeviceSize vram = 0;
	float vram_;
	cc_uint32 i;

	String_Format1(info, "-- Using Vulkan (%i bit) --\n", &pointerSize);
	String_Format1(info, "Adapter: %c\n", props.deviceName);
	String_Format3(info, "Vulkan version: %i.%i.%i\n", &major, &minor, &patch);

	for (i = 0; i < memProps.memoryHeapCount; i++)
	{
		if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) vram += memProps.memoryHeaps[i].size;
	}
	vram_ = vram / (1024.0f * 1024.0f);
	String_Format1(info, "Graphics memory: %f2 MB VRAM\n", &vram_);
	PrintMaxTextureInfo(info);
}
#endif
