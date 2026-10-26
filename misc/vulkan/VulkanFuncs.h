/* Instance functions */
VK_FUNC(VkResult, vkCreateInstance,  (const VkInstanceCreateInfo* pCreateInfo, const void* pAllocator, VkInstance* pInstance))
VK_FUNC(void,     vkDestroyInstance, (VkInstance instance, const void* pAllocator))

/* Physical device functions */
VK_FUNC(VkResult, vkEnumeratePhysicalDevices,               (VkInstance instance, cc_uint32* pCount, VkPhysicalDevice* pDevices))
VK_FUNC(void,     vkGetPhysicalDeviceProperties,            (VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties))
VK_FUNC(void,     vkGetPhysicalDeviceMemoryProperties,      (VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pProperties))
VK_FUNC(void,     vkGetPhysicalDeviceQueueFamilyProperties, (VkPhysicalDevice physicalDevice, cc_uint32* pCount, VkQueueFamilyProperties* pProperties))
VK_FUNC(void,     vkGetPhysicalDeviceFormatProperties,      (VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties* pProperties))

/* Device functions */
VK_FUNC(VkResult, vkCreateDevice,   (VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const void* pAllocator, VkDevice* pDevice))
VK_FUNC(void,     vkDestroyDevice,  (VkDevice device, const void* pAllocator))
VK_FUNC(void,     vkGetDeviceQueue, (VkDevice device, cc_uint32 queueFamilyIndex, cc_uint32 queueIndex, VkQueue* pQueue))
VK_FUNC(VkResult, vkDeviceWaitIdle, (VkDevice device))

/* Surface functions (VK_KHR_surface, VK_KHR_win32_surface) */
VK_FUNC(VkResult, vkCreateWin32SurfaceKHR,                   (VkInstance instance, const VkWin32SurfaceCreateInfoKHR* pCreateInfo, const void* pAllocator, VkSurfaceKHR* pSurface))
VK_FUNC(void,     vkDestroySurfaceKHR,                       (VkInstance instance, VkSurfaceKHR surface, const void* pAllocator))
VK_FUNC(VkResult, vkGetPhysicalDeviceSurfaceSupportKHR,      (VkPhysicalDevice physicalDevice, cc_uint32 queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported))
VK_FUNC(VkResult, vkGetPhysicalDeviceSurfaceCapabilitiesKHR, (VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pCapabilities))
VK_FUNC(VkResult, vkGetPhysicalDeviceSurfaceFormatsKHR,      (VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, cc_uint32* pCount, VkSurfaceFormatKHR* pFormats))
VK_FUNC(VkResult, vkGetPhysicalDeviceSurfacePresentModesKHR, (VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, cc_uint32* pCount, VkPresentModeKHR* pModes))

/* Swapchain functions (VK_KHR_swapchain) */
VK_FUNC(VkResult, vkCreateSwapchainKHR,    (VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const void* pAllocator, VkSwapchainKHR* pSwapchain))
VK_FUNC(void,     vkDestroySwapchainKHR,   (VkDevice device, VkSwapchainKHR swapchain, const void* pAllocator))
VK_FUNC(VkResult, vkGetSwapchainImagesKHR, (VkDevice device, VkSwapchainKHR swapchain, cc_uint32* pCount, VkImage* pImages))
VK_FUNC(VkResult, vkAcquireNextImageKHR,   (VkDevice device, VkSwapchainKHR swapchain, cc_uint64 timeout, VkSemaphore semaphore, VkFence fence, cc_uint32* pImageIndex))
VK_FUNC(VkResult, vkQueuePresentKHR,       (VkQueue queue, const VkPresentInfoKHR* pPresentInfo))

/* Image functions */
VK_FUNC(VkResult, vkCreateImage,                (VkDevice device, const VkImageCreateInfo* pCreateInfo, const void* pAllocator, VkImage* pImage))
VK_FUNC(void,     vkDestroyImage,               (VkDevice device, VkImage image, const void* pAllocator))
VK_FUNC(void,     vkGetImageMemoryRequirements, (VkDevice device, VkImage image, VkMemoryRequirements* pRequirements))
VK_FUNC(VkResult, vkBindImageMemory,            (VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize offset))
VK_FUNC(VkResult, vkCreateImageView,            (VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const void* pAllocator, VkImageView* pView))
VK_FUNC(void,     vkDestroyImageView,           (VkDevice device, VkImageView view, const void* pAllocator))

/* Buffer functions */
VK_FUNC(VkResult, vkCreateBuffer,                (VkDevice device, const VkBufferCreateInfo* pCreateInfo, const void* pAllocator, VkBuffer* pBuffer))
VK_FUNC(void,     vkDestroyBuffer,               (VkDevice device, VkBuffer buffer, const void* pAllocator))
VK_FUNC(void,     vkGetBufferMemoryRequirements, (VkDevice device, VkBuffer buffer, VkMemoryRequirements* pRequirements))
VK_FUNC(VkResult, vkBindBufferMemory,            (VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize offset))

/* Memory functions */
VK_FUNC(VkResult, vkAllocateMemory, (VkDevice device, const VkMemoryAllocateInfo* pAllocateInfo, const void* pAllocator, VkDeviceMemory* pMemory))
VK_FUNC(void,     vkFreeMemory,     (VkDevice device, VkDeviceMemory memory, const void* pAllocator))
VK_FUNC(VkResult, vkMapMemory,      (VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkFlags flags, void** ppData))
VK_FUNC(void,     vkUnmapMemory,    (VkDevice device, VkDeviceMemory memory))

/* Render pass functions */
VK_FUNC(VkResult, vkCreateRenderPass,   (VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const void* pAllocator, VkRenderPass* pRenderPass))
VK_FUNC(void,     vkDestroyRenderPass,  (VkDevice device, VkRenderPass renderPass, const void* pAllocator))
VK_FUNC(VkResult, vkCreateFramebuffer,  (VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, const void* pAllocator, VkFramebuffer* pFramebuffer))
VK_FUNC(void,     vkDestroyFramebuffer, (VkDevice device, VkFramebuffer framebuffer, const void* pAllocator))

/* Pipeline functions */
VK_FUNC(VkResult, vkCreateShaderModule,      (VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const void* pAllocator, VkShaderModule* pModule))
VK_FUNC(void,     vkDestroyShaderModule,     (VkDevice device, VkShaderModule module, const void* pAllocator))
VK_FUNC(VkResult, vkCreatePipelineLayout,    (VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, const void* pAllocator, VkPipelineLayout* pLayout))
VK_FUNC(void,     vkDestroyPipelineLayout,   (VkDevice device, VkPipelineLayout layout, const void* pAllocator))
VK_FUNC(VkResult, vkCreateGraphicsPipelines, (VkDevice device, VkPipelineCache cache, cc_uint32 count, const VkGraphicsPipelineCreateInfo* pCreateInfos, const void* pAllocator, VkPipeline* pPipelines))
VK_FUNC(void,     vkDestroyPipeline,         (VkDevice device, VkPipeline pipeline, const void* pAllocator))

/* Descriptor and sampler functions */
VK_FUNC(VkResult, vkCreateDescriptorSetLayout,  (VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const void* pAllocator, VkDescriptorSetLayout* pLayout))
VK_FUNC(void,     vkDestroyDescriptorSetLayout, (VkDevice device, VkDescriptorSetLayout layout, const void* pAllocator))
VK_FUNC(VkResult, vkCreateDescriptorPool,       (VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo, const void* pAllocator, VkDescriptorPool* pPool))
VK_FUNC(void,     vkDestroyDescriptorPool,      (VkDevice device, VkDescriptorPool pool, const void* pAllocator))
VK_FUNC(VkResult, vkAllocateDescriptorSets,     (VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pSets))
VK_FUNC(VkResult, vkFreeDescriptorSets,         (VkDevice device, VkDescriptorPool pool, cc_uint32 count, const VkDescriptorSet* pSets))
VK_FUNC(void,     vkUpdateDescriptorSets,       (VkDevice device, cc_uint32 writeCount, const VkWriteDescriptorSet* pWrites, cc_uint32 copyCount, const void* pCopies))
VK_FUNC(VkResult, vkCreateSampler,              (VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const void* pAllocator, VkSampler* pSampler))
VK_FUNC(void,     vkDestroySampler,             (VkDevice device, VkSampler sampler, const void* pAllocator))

/* Command buffer functions */
VK_FUNC(VkResult, vkCreateCommandPool,      (VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const void* pAllocator, VkCommandPool* pPool))
VK_FUNC(void,     vkDestroyCommandPool,     (VkDevice device, VkCommandPool pool, const void* pAllocator))
VK_FUNC(VkResult, vkResetCommandPool,       (VkDevice device, VkCommandPool pool, VkFlags flags))
VK_FUNC(VkResult, vkAllocateCommandBuffers, (VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pBuffers))
VK_FUNC(VkResult, vkBeginCommandBuffer,     (VkCommandBuffer cmd, const VkCommandBufferBeginInfo* pBeginInfo))
VK_FUNC(VkResult, vkEndCommandBuffer,       (VkCommandBuffer cmd))

/* Synchronisation and submission functions */
VK_FUNC(VkResult, vkCreateFence,      (VkDevice device, const VkFenceCreateInfo* pCreateInfo, const void* pAllocator, VkFence* pFence))
VK_FUNC(void,     vkDestroyFence,     (VkDevice device, VkFence fence, const void* pAllocator))
VK_FUNC(VkResult, vkWaitForFences,    (VkDevice device, cc_uint32 count, const VkFence* pFences, VkBool32 waitAll, cc_uint64 timeout))
VK_FUNC(VkResult, vkResetFences,      (VkDevice device, cc_uint32 count, const VkFence* pFences))
VK_FUNC(VkResult, vkCreateSemaphore,  (VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, const void* pAllocator, VkSemaphore* pSemaphore))
VK_FUNC(void,     vkDestroySemaphore, (VkDevice device, VkSemaphore semaphore, const void* pAllocator))
VK_FUNC(VkResult, vkQueueSubmit,      (VkQueue queue, cc_uint32 count, const VkSubmitInfo* pSubmits, VkFence fence))

/* Command recording functions */
VK_FUNC(void, vkCmdBeginRenderPass,    (VkCommandBuffer cmd, const VkRenderPassBeginInfo* pBeginInfo, VkSubpassContents contents))
VK_FUNC(void, vkCmdEndRenderPass,      (VkCommandBuffer cmd))
VK_FUNC(void, vkCmdBindPipeline,       (VkCommandBuffer cmd, VkPipelineBindPoint bindPoint, VkPipeline pipeline))
VK_FUNC(void, vkCmdBindDescriptorSets, (VkCommandBuffer cmd, VkPipelineBindPoint bindPoint, VkPipelineLayout layout, cc_uint32 firstSet, cc_uint32 count, const VkDescriptorSet* pSets, cc_uint32 dynamicCount, const cc_uint32* pDynamicOffsets))
VK_FUNC(void, vkCmdBindVertexBuffers,  (VkCommandBuffer cmd, cc_uint32 firstBinding, cc_uint32 count, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets))
VK_FUNC(void, vkCmdBindIndexBuffer,    (VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType))
VK_FUNC(void, vkCmdPushConstants,      (VkCommandBuffer cmd, VkPipelineLayout layout, VkFlags stageFlags, cc_uint32 offset, cc_uint32 size, const void* pValues))
VK_FUNC(void, vkCmdSetViewport,        (VkCommandBuffer cmd, cc_uint32 first, cc_uint32 count, const VkViewport* pViewports))
VK_FUNC(void, vkCmdSetScissor,         (VkCommandBuffer cmd, cc_uint32 first, cc_uint32 count, const VkRect2D* pScissors))
VK_FUNC(void, vkCmdDraw,               (VkCommandBuffer cmd, cc_uint32 vertexCount, cc_uint32 instanceCount, cc_uint32 firstVertex, cc_uint32 firstInstance))
VK_FUNC(void, vkCmdDrawIndexed,        (VkCommandBuffer cmd, cc_uint32 indexCount, cc_uint32 instanceCount, cc_uint32 firstIndex, int vertexOffset, cc_uint32 firstInstance))
VK_FUNC(void, vkCmdClearAttachments,   (VkCommandBuffer cmd, cc_uint32 count, const VkClearAttachment* pAttachments, cc_uint32 rectCount, const VkClearRect* pRects))
VK_FUNC(void, vkCmdCopyBuffer,         (VkCommandBuffer cmd, VkBuffer src, VkBuffer dst, cc_uint32 count, const VkBufferCopy* pRegions))
VK_FUNC(void, vkCmdCopyBufferToImage,  (VkCommandBuffer cmd, VkBuffer src, VkImage dst, VkImageLayout dstLayout, cc_uint32 count, const VkBufferImageCopy* pRegions))
VK_FUNC(void, vkCmdCopyImageToBuffer,  (VkCommandBuffer cmd, VkImage src, VkImageLayout srcLayout, VkBuffer dst, cc_uint32 count, const VkBufferImageCopy* pRegions))
VK_FUNC(void, vkCmdPipelineBarrier,    (VkCommandBuffer cmd, VkFlags srcStageMask, VkFlags dstStageMask, VkFlags dependencyFlags, cc_uint32 memoryCount, const VkMemoryBarrier* pMemoryBarriers, cc_uint32 bufferCount, const void* pBufferBarriers, cc_uint32 imageCount, const VkImageMemoryBarrier* pImageBarriers))
