/* Minimal subset of vulkan_core.h and vulkan_win32.h, with only what the Vulkan backend needs */
/* https://github.com/KhronosGroup/Vulkan-Headers */
#if defined _WIN32
	#define VKAPI_CALL __stdcall
	#define VKAPI_PTR  VKAPI_CALL
#else
	#define VKAPI_CALL
	#define VKAPI_PTR
#endif

/* Dispatchable handles are pointers, non-dispatchable handles are always 64 bit */
typedef struct VkInstance_T*       VkInstance;
typedef struct VkPhysicalDevice_T* VkPhysicalDevice;
typedef struct VkDevice_T*         VkDevice;
typedef struct VkQueue_T*          VkQueue;
typedef struct VkCommandBuffer_T*  VkCommandBuffer;

typedef cc_uint64 VkSurfaceKHR;
typedef cc_uint64 VkSwapchainKHR;
typedef cc_uint64 VkImage;
typedef cc_uint64 VkImageView;
typedef cc_uint64 VkBuffer;
typedef cc_uint64 VkDeviceMemory;
typedef cc_uint64 VkRenderPass;
typedef cc_uint64 VkFramebuffer;
typedef cc_uint64 VkShaderModule;
typedef cc_uint64 VkPipeline;
typedef cc_uint64 VkPipelineLayout;
typedef cc_uint64 VkPipelineCache;
typedef cc_uint64 VkDescriptorSetLayout;
typedef cc_uint64 VkDescriptorPool;
typedef cc_uint64 VkDescriptorSet;
typedef cc_uint64 VkSampler;
typedef cc_uint64 VkCommandPool;
typedef cc_uint64 VkFence;
typedef cc_uint64 VkSemaphore;

typedef cc_uint32 VkFlags;
typedef cc_uint32 VkBool32;
typedef cc_uint64 VkDeviceSize;
typedef int VkResult;
typedef int VkStructureType;
typedef int VkFormat;
typedef int VkColorSpaceKHR;
typedef int VkPresentModeKHR;
typedef int VkImageLayout;
typedef int VkPhysicalDeviceType;
typedef int VkIndexType;
typedef int VkPipelineBindPoint;
typedef int VkSubpassContents;
typedef int VkDynamicState;
typedef void (VKAPI_PTR *PFN_vkVoidFunction)(void);

/* Only the enum values/flags actually used by the Vulkan backend are defined */
#define VK_API_VERSION_1_1 ((1u << 22) | (1u << 12))
#define VK_VERSION_MAJOR(version) ((cc_uint32)(version) >> 22)
#define VK_VERSION_MINOR(version) (((cc_uint32)(version) >> 12) & 0x3FFu)
#define VK_VERSION_PATCH(version) ((cc_uint32)(version) & 0xFFFu)

#define VK_SUCCESS                         0
#define VK_SUBOPTIMAL_KHR                  1000001003
#define VK_ERROR_OUT_OF_HOST_MEMORY        (-1)
#define VK_ERROR_OUT_OF_DEVICE_MEMORY      (-2)
#define VK_ERROR_DEVICE_LOST               (-4)
#define VK_ERROR_INCOMPATIBLE_DRIVER       (-9)
#define VK_ERROR_SURFACE_LOST_KHR          (-1000000000)
#define VK_ERROR_OUT_OF_DATE_KHR           (-1000001004)

#define VK_STRUCTURE_TYPE_APPLICATION_INFO                          0
#define VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO                      1
#define VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO                  2
#define VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO                        3
#define VK_STRUCTURE_TYPE_SUBMIT_INFO                               4
#define VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO                      5
#define VK_STRUCTURE_TYPE_FENCE_CREATE_INFO                         8
#define VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO                     9
#define VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO                        12
#define VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO                         14
#define VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO                    15
#define VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO                 16
#define VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO         18
#define VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO   19
#define VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO 20
#define VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO       22
#define VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO  23
#define VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO    24
#define VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO  25
#define VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO    26
#define VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO        27
#define VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO             28
#define VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO               30
#define VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO                       31
#define VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO         32
#define VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO               33
#define VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO              34
#define VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET                      35
#define VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO                   37
#define VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO                   38
#define VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO                  39
#define VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO              40
#define VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO                 42
#define VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO                    43
#define VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER                      45
#define VK_STRUCTURE_TYPE_MEMORY_BARRIER                            46
#define VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR                 1000001000
#define VK_STRUCTURE_TYPE_PRESENT_INFO_KHR                          1000001001
#define VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR             1000009000
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT 1000267000

#define VK_FORMAT_UNDEFINED            0
#define VK_FORMAT_R8G8B8A8_UNORM       37
#define VK_FORMAT_B8G8R8A8_UNORM       44
#define VK_FORMAT_R32G32_SFLOAT        103
#define VK_FORMAT_R32G32B32_SFLOAT     106
#define VK_FORMAT_D16_UNORM            124
#define VK_FORMAT_X8_D24_UNORM_PACK32  125
#define VK_FORMAT_D32_SFLOAT           126
#define VK_FORMAT_D24_UNORM_S8_UINT    129

#define VK_COLOR_SPACE_SRGB_NONLINEAR_KHR 0

#define VK_PRESENT_MODE_IMMEDIATE_KHR 0
#define VK_PRESENT_MODE_MAILBOX_KHR   1
#define VK_PRESENT_MODE_FIFO_KHR      2

#define VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU 1
#define VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU   2

#define VK_QUEUE_GRAPHICS_BIT 0x00000001

#define VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT  0x00000001
#define VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  0x00000002
#define VK_MEMORY_PROPERTY_HOST_COHERENT_BIT 0x00000004
#define VK_MEMORY_PROPERTY_HOST_CACHED_BIT   0x00000008
#define VK_MEMORY_HEAP_DEVICE_LOCAL_BIT      0x00000001

#define VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT 0x00000200

#define VK_IMAGE_USAGE_TRANSFER_SRC_BIT             0x00000001
#define VK_IMAGE_USAGE_TRANSFER_DST_BIT             0x00000002
#define VK_IMAGE_USAGE_SAMPLED_BIT                  0x00000004
#define VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT         0x00000010
#define VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT 0x00000020

#define VK_BUFFER_USAGE_TRANSFER_SRC_BIT  0x00000001
#define VK_BUFFER_USAGE_TRANSFER_DST_BIT  0x00000002
#define VK_BUFFER_USAGE_INDEX_BUFFER_BIT  0x00000040
#define VK_BUFFER_USAGE_VERTEX_BUFFER_BIT 0x00000080

#define VK_SHARING_MODE_EXCLUSIVE 0

#define VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR          0x00000001
#define VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR  0x00000002
#define VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR 0x00000004
#define VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR         0x00000008

#define VK_IMAGE_TYPE_2D      1
#define VK_IMAGE_VIEW_TYPE_2D 1
#define VK_IMAGE_TILING_OPTIMAL 0
#define VK_SAMPLE_COUNT_1_BIT 0x00000001

#define VK_IMAGE_ASPECT_COLOR_BIT   0x00000001
#define VK_IMAGE_ASPECT_DEPTH_BIT   0x00000002
#define VK_IMAGE_ASPECT_STENCIL_BIT 0x00000004

#define VK_IMAGE_LAYOUT_UNDEFINED                        0
#define VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL         2
#define VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL 3
#define VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL         5
#define VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL             6
#define VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL             7
#define VK_IMAGE_LAYOUT_PRESENT_SRC_KHR                  1000001002

#define VK_ATTACHMENT_LOAD_OP_LOAD       0
#define VK_ATTACHMENT_LOAD_OP_DONT_CARE  2
#define VK_ATTACHMENT_STORE_OP_STORE     0
#define VK_ATTACHMENT_STORE_OP_DONT_CARE 1

#define VK_PIPELINE_BIND_POINT_GRAPHICS 0
#define VK_SUBPASS_EXTERNAL (~0u)
#define VK_SUBPASS_CONTENTS_INLINE 0
#define VK_QUEUE_FAMILY_IGNORED (~0u)

#define VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT             0x00000001
#define VK_PIPELINE_STAGE_VERTEX_INPUT_BIT            0x00000004
#define VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT         0x00000080
#define VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT    0x00000100
#define VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT     0x00000200
#define VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT 0x00000400
#define VK_PIPELINE_STAGE_TRANSFER_BIT                0x00001000
#define VK_PIPELINE_STAGE_HOST_BIT                    0x00004000
#define VK_PIPELINE_STAGE_ALL_COMMANDS_BIT            0x00010000

#define VK_ACCESS_INDEX_READ_BIT                     0x00000002
#define VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT          0x00000004
#define VK_ACCESS_SHADER_READ_BIT                    0x00000020
#define VK_ACCESS_COLOR_ATTACHMENT_READ_BIT          0x00000080
#define VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT         0x00000100
#define VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT  0x00000200
#define VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT 0x00000400
#define VK_ACCESS_TRANSFER_READ_BIT                  0x00000800
#define VK_ACCESS_TRANSFER_WRITE_BIT                 0x00001000
#define VK_ACCESS_HOST_READ_BIT                      0x00002000

#define VK_SHADER_STAGE_VERTEX_BIT   0x00000001
#define VK_SHADER_STAGE_FRAGMENT_BIT 0x00000010

#define VK_VERTEX_INPUT_RATE_VERTEX 0
#define VK_PRIMITIVE_TOPOLOGY_LINE_LIST     1
#define VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST 3
#define VK_POLYGON_MODE_FILL 0
#define VK_CULL_MODE_NONE     0
#define VK_CULL_MODE_BACK_BIT 0x00000002
#define VK_FRONT_FACE_COUNTER_CLOCKWISE 0
#define VK_COMPARE_OP_NEVER            0
#define VK_COMPARE_OP_GREATER_OR_EQUAL 6
#define VK_BLEND_FACTOR_SRC_ALPHA           6
#define VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA 7
#define VK_BLEND_OP_ADD 0
#define VK_DYNAMIC_STATE_VIEWPORT 0
#define VK_DYNAMIC_STATE_SCISSOR  1
#define VK_DYNAMIC_STATE_CULL_MODE_EXT 1000267000

#define VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER 1
#define VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT 0x00000001

#define VK_FILTER_NEAREST 0
#define VK_FILTER_LINEAR  1
#define VK_SAMPLER_MIPMAP_MODE_NEAREST 0
#define VK_SAMPLER_MIPMAP_MODE_LINEAR  1
#define VK_SAMPLER_ADDRESS_MODE_REPEAT 0
#define VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK 0
#define VK_LOD_CLAMP_NONE 1000.0f

#define VK_COMMAND_POOL_CREATE_TRANSIENT_BIT 0x00000001
#define VK_COMMAND_BUFFER_LEVEL_PRIMARY 0
#define VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT 0x00000001
#define VK_FENCE_CREATE_SIGNALED_BIT 0x00000001
#define VK_INDEX_TYPE_UINT16 0

typedef struct VkExtent2D { cc_uint32 width, height; } VkExtent2D;
typedef struct VkExtent3D { cc_uint32 width, height, depth; } VkExtent3D;
typedef struct VkOffset2D { int x, y; } VkOffset2D;
typedef struct VkOffset3D { int x, y, z; } VkOffset3D;
typedef struct VkRect2D { VkOffset2D offset; VkExtent2D extent; } VkRect2D;

typedef struct VkApplicationInfo {
	VkStructureType sType;
	const void* pNext;
	const char* pApplicationName;
	cc_uint32   applicationVersion;
	const char* pEngineName;
	cc_uint32   engineVersion;
	cc_uint32   apiVersion;
} VkApplicationInfo;

typedef struct VkInstanceCreateInfo {
	VkStructureType sType;
	const void* pNext;
	VkFlags     flags;
	const VkApplicationInfo* pApplicationInfo;
	cc_uint32   enabledLayerCount;
	const char* const* ppEnabledLayerNames;
	cc_uint32   enabledExtensionCount;
	const char* const* ppEnabledExtensionNames;
} VkInstanceCreateInfo;

/* NOTE: Only the first few fields are declared, the rest is just padding */
/*  so that the struct is always at least as large as the real one */
typedef struct VkPhysicalDeviceLimits {
	cc_uint32 maxImageDimension1D;
	cc_uint32 maxImageDimension2D;
	cc_uint32 maxImageDimension3D;
	cc_uint32 maxImageDimensionCube;
	cc_uint32 maxImageArrayLayers;
	cc_uint32 maxTexelBufferElements;
	cc_uint32 maxUniformBufferRange;
	cc_uint32 maxStorageBufferRange;
	cc_uint32 maxPushConstantsSize;
	cc_uint32 maxMemoryAllocationCount;
	cc_uint32 maxSamplerAllocationCount;
	VkDeviceSize bufferImageGranularity;
	VkDeviceSize _remaining[64];
} VkPhysicalDeviceLimits;

typedef struct VkPhysicalDeviceProperties {
	cc_uint32 apiVersion;
	cc_uint32 driverVersion;
	cc_uint32 vendorID;
	cc_uint32 deviceID;
	VkPhysicalDeviceType deviceType;
	char      deviceName[256];
	cc_uint8  pipelineCacheUUID[16];
	VkPhysicalDeviceLimits limits;
	VkBool32  sparseProperties[8];
} VkPhysicalDeviceProperties;

typedef struct VkMemoryType { VkFlags propertyFlags; cc_uint32 heapIndex; } VkMemoryType;
typedef struct VkMemoryHeap { VkDeviceSize size; VkFlags flags; } VkMemoryHeap;

typedef struct VkPhysicalDeviceMemoryProperties {
	cc_uint32    memoryTypeCount;
	VkMemoryType memoryTypes[32];
	cc_uint32    memoryHeapCount;
	VkMemoryHeap memoryHeaps[16];
} VkPhysicalDeviceMemoryProperties;

typedef struct VkQueueFamilyProperties {
	VkFlags    queueFlags;
	cc_uint32  queueCount;
	cc_uint32  timestampValidBits;
	VkExtent3D minImageTransferGranularity;
} VkQueueFamilyProperties;

typedef struct VkExtensionProperties {
	char      extensionName[256];
	cc_uint32 specVersion;
} VkExtensionProperties;

typedef struct VkPhysicalDeviceExtendedDynamicStateFeaturesEXT {
	VkStructureType sType;
	void*    pNext;
	VkBool32 extendedDynamicState;
} VkPhysicalDeviceExtendedDynamicStateFeaturesEXT;

typedef struct VkFormatProperties {
	VkFlags linearTilingFeatures;
	VkFlags optimalTilingFeatures;
	VkFlags bufferFeatures;
} VkFormatProperties;

typedef struct VkDeviceQueueCreateInfo {
	VkStructureType sType;
	const void*  pNext;
	VkFlags      flags;
	cc_uint32    queueFamilyIndex;
	cc_uint32    queueCount;
	const float* pQueuePriorities;
} VkDeviceQueueCreateInfo;

typedef struct VkDeviceCreateInfo {
	VkStructureType sType;
	const void* pNext;
	VkFlags     flags;
	cc_uint32   queueCreateInfoCount;
	const VkDeviceQueueCreateInfo* pQueueCreateInfos;
	cc_uint32   enabledLayerCount;
	const char* const* ppEnabledLayerNames;
	cc_uint32   enabledExtensionCount;
	const char* const* ppEnabledExtensionNames;
	const void* pEnabledFeatures;
} VkDeviceCreateInfo;

typedef struct VkWin32SurfaceCreateInfoKHR {
	VkStructureType sType;
	const void* pNext;
	VkFlags     flags;
	void*       hinstance;
	void*       hwnd;
} VkWin32SurfaceCreateInfoKHR;

typedef struct VkSurfaceCapabilitiesKHR {
	cc_uint32  minImageCount;
	cc_uint32  maxImageCount;
	VkExtent2D currentExtent;
	VkExtent2D minImageExtent;
	VkExtent2D maxImageExtent;
	cc_uint32  maxImageArrayLayers;
	VkFlags    supportedTransforms;
	VkFlags    currentTransform;
	VkFlags    supportedCompositeAlpha;
	VkFlags    supportedUsageFlags;
} VkSurfaceCapabilitiesKHR;

typedef struct VkSurfaceFormatKHR {
	VkFormat        format;
	VkColorSpaceKHR colorSpace;
} VkSurfaceFormatKHR;

typedef struct VkSwapchainCreateInfoKHR {
	VkStructureType  sType;
	const void*      pNext;
	VkFlags          flags;
	VkSurfaceKHR     surface;
	cc_uint32        minImageCount;
	VkFormat         imageFormat;
	VkColorSpaceKHR  imageColorSpace;
	VkExtent2D       imageExtent;
	cc_uint32        imageArrayLayers;
	VkFlags          imageUsage;
	int              imageSharingMode;
	cc_uint32        queueFamilyIndexCount;
	const cc_uint32* pQueueFamilyIndices;
	VkFlags          preTransform;
	VkFlags          compositeAlpha;
	VkPresentModeKHR presentMode;
	VkBool32         clipped;
	VkSwapchainKHR   oldSwapchain;
} VkSwapchainCreateInfoKHR;

typedef struct VkComponentMapping { int r, g, b, a; } VkComponentMapping;

typedef struct VkImageSubresourceRange {
	VkFlags   aspectMask;
	cc_uint32 baseMipLevel;
	cc_uint32 levelCount;
	cc_uint32 baseArrayLayer;
	cc_uint32 layerCount;
} VkImageSubresourceRange;

typedef struct VkImageViewCreateInfo {
	VkStructureType sType;
	const void* pNext;
	VkFlags     flags;
	VkImage     image;
	int         viewType;
	VkFormat    format;
	VkComponentMapping components;
	VkImageSubresourceRange subresourceRange;
} VkImageViewCreateInfo;

typedef struct VkImageCreateInfo {
	VkStructureType  sType;
	const void*      pNext;
	VkFlags          flags;
	int              imageType;
	VkFormat         format;
	VkExtent3D       extent;
	cc_uint32        mipLevels;
	cc_uint32        arrayLayers;
	VkFlags          samples;
	int              tiling;
	VkFlags          usage;
	int              sharingMode;
	cc_uint32        queueFamilyIndexCount;
	const cc_uint32* pQueueFamilyIndices;
	VkImageLayout    initialLayout;
} VkImageCreateInfo;

typedef struct VkMemoryRequirements {
	VkDeviceSize size;
	VkDeviceSize alignment;
	cc_uint32    memoryTypeBits;
} VkMemoryRequirements;

typedef struct VkMemoryAllocateInfo {
	VkStructureType sType;
	const void*  pNext;
	VkDeviceSize allocationSize;
	cc_uint32    memoryTypeIndex;
} VkMemoryAllocateInfo;

typedef struct VkBufferCreateInfo {
	VkStructureType  sType;
	const void*      pNext;
	VkFlags          flags;
	VkDeviceSize     size;
	VkFlags          usage;
	int              sharingMode;
	cc_uint32        queueFamilyIndexCount;
	const cc_uint32* pQueueFamilyIndices;
} VkBufferCreateInfo;

typedef struct VkAttachmentDescription {
	VkFlags       flags;
	VkFormat      format;
	VkFlags       samples;
	int           loadOp;
	int           storeOp;
	int           stencilLoadOp;
	int           stencilStoreOp;
	VkImageLayout initialLayout;
	VkImageLayout finalLayout;
} VkAttachmentDescription;

typedef struct VkAttachmentReference {
	cc_uint32     attachment;
	VkImageLayout layout;
} VkAttachmentReference;

typedef struct VkSubpassDescription {
	VkFlags     flags;
	VkPipelineBindPoint pipelineBindPoint;
	cc_uint32   inputAttachmentCount;
	const VkAttachmentReference* pInputAttachments;
	cc_uint32   colorAttachmentCount;
	const VkAttachmentReference* pColorAttachments;
	const VkAttachmentReference* pResolveAttachments;
	const VkAttachmentReference* pDepthStencilAttachment;
	cc_uint32   preserveAttachmentCount;
	const cc_uint32* pPreserveAttachments;
} VkSubpassDescription;

typedef struct VkSubpassDependency {
	cc_uint32 srcSubpass;
	cc_uint32 dstSubpass;
	VkFlags   srcStageMask;
	VkFlags   dstStageMask;
	VkFlags   srcAccessMask;
	VkFlags   dstAccessMask;
	VkFlags   dependencyFlags;
} VkSubpassDependency;

typedef struct VkRenderPassCreateInfo {
	VkStructureType sType;
	const void* pNext;
	VkFlags     flags;
	cc_uint32   attachmentCount;
	const VkAttachmentDescription* pAttachments;
	cc_uint32   subpassCount;
	const VkSubpassDescription* pSubpasses;
	cc_uint32   dependencyCount;
	const VkSubpassDependency* pDependencies;
} VkRenderPassCreateInfo;

typedef struct VkFramebufferCreateInfo {
	VkStructureType    sType;
	const void*        pNext;
	VkFlags            flags;
	VkRenderPass       renderPass;
	cc_uint32          attachmentCount;
	const VkImageView* pAttachments;
	cc_uint32          width;
	cc_uint32          height;
	cc_uint32          layers;
} VkFramebufferCreateInfo;

typedef struct VkShaderModuleCreateInfo {
	VkStructureType  sType;
	const void*      pNext;
	VkFlags          flags;
	cc_uintptr       codeSize;
	const cc_uint32* pCode;
} VkShaderModuleCreateInfo;

typedef struct VkPipelineShaderStageCreateInfo {
	VkStructureType sType;
	const void*    pNext;
	VkFlags        flags;
	VkFlags        stage;
	VkShaderModule module;
	const char*    pName;
	const void*    pSpecializationInfo;
} VkPipelineShaderStageCreateInfo;

typedef struct VkVertexInputBindingDescription {
	cc_uint32 binding;
	cc_uint32 stride;
	int       inputRate;
} VkVertexInputBindingDescription;

typedef struct VkVertexInputAttributeDescription {
	cc_uint32 location;
	cc_uint32 binding;
	VkFormat  format;
	cc_uint32 offset;
} VkVertexInputAttributeDescription;

typedef struct VkPipelineVertexInputStateCreateInfo {
	VkStructureType sType;
	const void* pNext;
	VkFlags     flags;
	cc_uint32   vertexBindingDescriptionCount;
	const VkVertexInputBindingDescription* pVertexBindingDescriptions;
	cc_uint32   vertexAttributeDescriptionCount;
	const VkVertexInputAttributeDescription* pVertexAttributeDescriptions;
} VkPipelineVertexInputStateCreateInfo;

typedef struct VkPipelineInputAssemblyStateCreateInfo {
	VkStructureType sType;
	const void* pNext;
	VkFlags     flags;
	int         topology;
	VkBool32    primitiveRestartEnable;
} VkPipelineInputAssemblyStateCreateInfo;

typedef struct VkViewport {
	float x, y, width, height, minDepth, maxDepth;
} VkViewport;

typedef struct VkPipelineViewportStateCreateInfo {
	VkStructureType   sType;
	const void*       pNext;
	VkFlags           flags;
	cc_uint32         viewportCount;
	const VkViewport* pViewports;
	cc_uint32         scissorCount;
	const VkRect2D*   pScissors;
} VkPipelineViewportStateCreateInfo;

typedef struct VkPipelineRasterizationStateCreateInfo {
	VkStructureType sType;
	const void* pNext;
	VkFlags     flags;
	VkBool32    depthClampEnable;
	VkBool32    rasterizerDiscardEnable;
	int         polygonMode;
	VkFlags     cullMode;
	int         frontFace;
	VkBool32    depthBiasEnable;
	float       depthBiasConstantFactor;
	float       depthBiasClamp;
	float       depthBiasSlopeFactor;
	float       lineWidth;
} VkPipelineRasterizationStateCreateInfo;

typedef struct VkPipelineMultisampleStateCreateInfo {
	VkStructureType  sType;
	const void*      pNext;
	VkFlags          flags;
	VkFlags          rasterizationSamples;
	VkBool32         sampleShadingEnable;
	float            minSampleShading;
	const cc_uint32* pSampleMask;
	VkBool32         alphaToCoverageEnable;
	VkBool32         alphaToOneEnable;
} VkPipelineMultisampleStateCreateInfo;

typedef struct VkStencilOpState {
	int       failOp;
	int       passOp;
	int       depthFailOp;
	int       compareOp;
	cc_uint32 compareMask;
	cc_uint32 writeMask;
	cc_uint32 reference;
} VkStencilOpState;

typedef struct VkPipelineDepthStencilStateCreateInfo {
	VkStructureType  sType;
	const void*      pNext;
	VkFlags          flags;
	VkBool32         depthTestEnable;
	VkBool32         depthWriteEnable;
	int              depthCompareOp;
	VkBool32         depthBoundsTestEnable;
	VkBool32         stencilTestEnable;
	VkStencilOpState front;
	VkStencilOpState back;
	float            minDepthBounds;
	float            maxDepthBounds;
} VkPipelineDepthStencilStateCreateInfo;

typedef struct VkPipelineColorBlendAttachmentState {
	VkBool32 blendEnable;
	int      srcColorBlendFactor;
	int      dstColorBlendFactor;
	int      colorBlendOp;
	int      srcAlphaBlendFactor;
	int      dstAlphaBlendFactor;
	int      alphaBlendOp;
	VkFlags  colorWriteMask;
} VkPipelineColorBlendAttachmentState;

typedef struct VkPipelineColorBlendStateCreateInfo {
	VkStructureType sType;
	const void* pNext;
	VkFlags     flags;
	VkBool32    logicOpEnable;
	int         logicOp;
	cc_uint32   attachmentCount;
	const VkPipelineColorBlendAttachmentState* pAttachments;
	float       blendConstants[4];
} VkPipelineColorBlendStateCreateInfo;

typedef struct VkPipelineDynamicStateCreateInfo {
	VkStructureType       sType;
	const void*           pNext;
	VkFlags               flags;
	cc_uint32             dynamicStateCount;
	const VkDynamicState* pDynamicStates;
} VkPipelineDynamicStateCreateInfo;

typedef struct VkGraphicsPipelineCreateInfo {
	VkStructureType sType;
	const void*     pNext;
	VkFlags         flags;
	cc_uint32       stageCount;
	const VkPipelineShaderStageCreateInfo*        pStages;
	const VkPipelineVertexInputStateCreateInfo*   pVertexInputState;
	const VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState;
	const void*                                   pTessellationState;
	const VkPipelineViewportStateCreateInfo*      pViewportState;
	const VkPipelineRasterizationStateCreateInfo* pRasterizationState;
	const VkPipelineMultisampleStateCreateInfo*   pMultisampleState;
	const VkPipelineDepthStencilStateCreateInfo*  pDepthStencilState;
	const VkPipelineColorBlendStateCreateInfo*    pColorBlendState;
	const VkPipelineDynamicStateCreateInfo*       pDynamicState;
	VkPipelineLayout layout;
	VkRenderPass     renderPass;
	cc_uint32        subpass;
	VkPipeline       basePipelineHandle;
	int              basePipelineIndex;
} VkGraphicsPipelineCreateInfo;

typedef struct VkPushConstantRange {
	VkFlags   stageFlags;
	cc_uint32 offset;
	cc_uint32 size;
} VkPushConstantRange;

typedef struct VkPipelineLayoutCreateInfo {
	VkStructureType sType;
	const void* pNext;
	VkFlags     flags;
	cc_uint32   setLayoutCount;
	const VkDescriptorSetLayout* pSetLayouts;
	cc_uint32   pushConstantRangeCount;
	const VkPushConstantRange* pPushConstantRanges;
} VkPipelineLayoutCreateInfo;

typedef struct VkDescriptorSetLayoutBinding {
	cc_uint32        binding;
	int              descriptorType;
	cc_uint32        descriptorCount;
	VkFlags          stageFlags;
	const VkSampler* pImmutableSamplers;
} VkDescriptorSetLayoutBinding;

typedef struct VkDescriptorSetLayoutCreateInfo {
	VkStructureType sType;
	const void* pNext;
	VkFlags     flags;
	cc_uint32   bindingCount;
	const VkDescriptorSetLayoutBinding* pBindings;
} VkDescriptorSetLayoutCreateInfo;

typedef struct VkDescriptorPoolSize {
	int       type;
	cc_uint32 descriptorCount;
} VkDescriptorPoolSize;

typedef struct VkDescriptorPoolCreateInfo {
	VkStructureType sType;
	const void* pNext;
	VkFlags     flags;
	cc_uint32   maxSets;
	cc_uint32   poolSizeCount;
	const VkDescriptorPoolSize* pPoolSizes;
} VkDescriptorPoolCreateInfo;

typedef struct VkDescriptorSetAllocateInfo {
	VkStructureType  sType;
	const void*      pNext;
	VkDescriptorPool descriptorPool;
	cc_uint32        descriptorSetCount;
	const VkDescriptorSetLayout* pSetLayouts;
} VkDescriptorSetAllocateInfo;

typedef struct VkDescriptorImageInfo {
	VkSampler     sampler;
	VkImageView   imageView;
	VkImageLayout imageLayout;
} VkDescriptorImageInfo;

typedef struct VkWriteDescriptorSet {
	VkStructureType sType;
	const void*     pNext;
	VkDescriptorSet dstSet;
	cc_uint32       dstBinding;
	cc_uint32       dstArrayElement;
	cc_uint32       descriptorCount;
	int             descriptorType;
	const VkDescriptorImageInfo* pImageInfo;
	const void*     pBufferInfo;
	const void*     pTexelBufferView;
} VkWriteDescriptorSet;

typedef struct VkSamplerCreateInfo {
	VkStructureType sType;
	const void* pNext;
	VkFlags     flags;
	int         magFilter;
	int         minFilter;
	int         mipmapMode;
	int         addressModeU;
	int         addressModeV;
	int         addressModeW;
	float       mipLodBias;
	VkBool32    anisotropyEnable;
	float       maxAnisotropy;
	VkBool32    compareEnable;
	int         compareOp;
	float       minLod;
	float       maxLod;
	int         borderColor;
	VkBool32    unnormalizedCoordinates;
} VkSamplerCreateInfo;

typedef struct VkCommandPoolCreateInfo {
	VkStructureType sType;
	const void* pNext;
	VkFlags     flags;
	cc_uint32   queueFamilyIndex;
} VkCommandPoolCreateInfo;

typedef struct VkCommandBufferAllocateInfo {
	VkStructureType sType;
	const void*   pNext;
	VkCommandPool commandPool;
	int           level;
	cc_uint32     commandBufferCount;
} VkCommandBufferAllocateInfo;

typedef struct VkCommandBufferBeginInfo {
	VkStructureType sType;
	const void* pNext;
	VkFlags     flags;
	const void* pInheritanceInfo;
} VkCommandBufferBeginInfo;

typedef struct VkFenceCreateInfo {
	VkStructureType sType;
	const void* pNext;
	VkFlags     flags;
} VkFenceCreateInfo;

typedef struct VkSemaphoreCreateInfo {
	VkStructureType sType;
	const void* pNext;
	VkFlags     flags;
} VkSemaphoreCreateInfo;

typedef struct VkSubmitInfo {
	VkStructureType        sType;
	const void*            pNext;
	cc_uint32              waitSemaphoreCount;
	const VkSemaphore*     pWaitSemaphores;
	const VkFlags*         pWaitDstStageMask;
	cc_uint32              commandBufferCount;
	const VkCommandBuffer* pCommandBuffers;
	cc_uint32              signalSemaphoreCount;
	const VkSemaphore*     pSignalSemaphores;
} VkSubmitInfo;

typedef struct VkPresentInfoKHR {
	VkStructureType       sType;
	const void*           pNext;
	cc_uint32             waitSemaphoreCount;
	const VkSemaphore*    pWaitSemaphores;
	cc_uint32             swapchainCount;
	const VkSwapchainKHR* pSwapchains;
	const cc_uint32*      pImageIndices;
	VkResult*             pResults;
} VkPresentInfoKHR;

typedef union VkClearColorValue {
	float     float32[4];
	int       int32[4];
	cc_uint32 uint32[4];
} VkClearColorValue;

typedef struct VkClearDepthStencilValue {
	float     depth;
	cc_uint32 stencil;
} VkClearDepthStencilValue;

typedef union VkClearValue {
	VkClearColorValue        color;
	VkClearDepthStencilValue depthStencil;
} VkClearValue;

typedef struct VkClearAttachment {
	VkFlags      aspectMask;
	cc_uint32    colorAttachment;
	VkClearValue clearValue;
} VkClearAttachment;

typedef struct VkClearRect {
	VkRect2D  rect;
	cc_uint32 baseArrayLayer;
	cc_uint32 layerCount;
} VkClearRect;

typedef struct VkRenderPassBeginInfo {
	VkStructureType sType;
	const void*     pNext;
	VkRenderPass    renderPass;
	VkFramebuffer   framebuffer;
	VkRect2D        renderArea;
	cc_uint32       clearValueCount;
	const VkClearValue* pClearValues;
} VkRenderPassBeginInfo;

typedef struct VkBufferCopy {
	VkDeviceSize srcOffset;
	VkDeviceSize dstOffset;
	VkDeviceSize size;
} VkBufferCopy;

typedef struct VkImageSubresourceLayers {
	VkFlags   aspectMask;
	cc_uint32 mipLevel;
	cc_uint32 baseArrayLayer;
	cc_uint32 layerCount;
} VkImageSubresourceLayers;

typedef struct VkBufferImageCopy {
	VkDeviceSize bufferOffset;
	cc_uint32    bufferRowLength;
	cc_uint32    bufferImageHeight;
	VkImageSubresourceLayers imageSubresource;
	VkOffset3D   imageOffset;
	VkExtent3D   imageExtent;
} VkBufferImageCopy;

typedef struct VkMemoryBarrier {
	VkStructureType sType;
	const void* pNext;
	VkFlags     srcAccessMask;
	VkFlags     dstAccessMask;
} VkMemoryBarrier;

typedef struct VkImageMemoryBarrier {
	VkStructureType sType;
	const void*     pNext;
	VkFlags         srcAccessMask;
	VkFlags         dstAccessMask;
	VkImageLayout   oldLayout;
	VkImageLayout   newLayout;
	cc_uint32       srcQueueFamilyIndex;
	cc_uint32       dstQueueFamilyIndex;
	VkImage         image;
	VkImageSubresourceRange subresourceRange;
} VkImageMemoryBarrier;
