#pragma once
#include "vulkan/vulkan.h"
#include <vector>
namespace Xavier
{
	struct ImageParameters
	{
		VkImage handle;
		VkImageView view;
		VkSampler sampler;
		VkDeviceMemory memory;
		VkFormat format;
		VkExtent3D extent;
		VkImageUsageFlags usageFlags;
		VkImageLayout layout;
		ImageParameters() :
			handle(VK_NULL_HANDLE),
			view(VK_NULL_HANDLE),
			sampler(VK_NULL_HANDLE),
			memory(VK_NULL_HANDLE),
			format(VK_FORMAT_MAX_ENUM),
			extent(),
			usageFlags(VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM),
			layout(VK_IMAGE_LAYOUT_UNDEFINED)
		{
		}

	};

	struct ContextParams
	{
		VkSampleCountFlagBits frameBufferColorAttachImageMaxSampleNumber;
		VkSampleCountFlagBits frameBufferDepthStencilAttachImageMaxSampleNumber;
		VkSampleCountFlagBits storageImageMaxSampleNumber;
		VkFormat depthStencilFormat;
		VkExtent3D minImageTransferGranularityGraphics;
		VkExtent3D minImageTransferGranularityPresent;
		VkExtent3D minImageTransferGranularityCompute;
	};

	struct BufferParameters
	{
		VkBuffer handle;
		VkDeviceMemory memory;
		VkDeviceSize memorySize;
		uint32_t size;
		VkBufferUsageFlags usageFlags;
		bool isStaging;
		BufferParameters() :
			handle(VK_NULL_HANDLE),
			memory(VK_NULL_HANDLE),
			size(0u),
			memorySize(0u),
			usageFlags(VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM),
			isStaging(false)
		{
		}

	};

	struct XSwapchain
	{
		VkSwapchainKHR handle;
		std::vector<ImageParameters> images;
		VkExtent2D extent;
		VkFormat format;
		XSwapchain() :
			handle(VK_NULL_HANDLE),
			images(),
			extent(),
			format(VK_FORMAT_UNDEFINED)
		{
		}
	};

	struct XVertexData
	{
		struct Position
		{
			float x, y, z;
		} pos;

		struct Color
		{
			float r, g, b, a;
		} color;
	};

	struct XRenderParas
	{

		std::vector<const char *> xLayers;
		std::vector<const char *> xInstanceExtensions;
		std::vector<const char *> xDeviceExtensions;

		VkInstance xInstance = VK_NULL_HANDLE;
		VkPhysicalDeviceFeatures xPhysicalDeviceFeatures = {};
		VkPhysicalDevice xPhysicalDevice = VK_NULL_HANDLE;

		VkDevice xDevice = VK_NULL_HANDLE;

		uint32_t xGraphicFamilyQueueIndex;
		uint32_t xPresentFamilyQueueIndex;
		uint32_t xComputeFamilyQueueIndex;

		VkQueue xGraphicQueue = VK_NULL_HANDLE;
		VkQueue xPresentQueue = VK_NULL_HANDLE;
		VkQueue xComputeQueue = VK_NULL_HANDLE;

		VkSurfaceKHR xSurface = VK_NULL_HANDLE;

		XSwapchain xSwapchain;

		VkCommandPool xRenderCmdPool = VK_NULL_HANDLE;

		VkCommandBuffer xCopyCmd = VK_NULL_HANDLE;
		VkFence xCopyFence = VK_NULL_HANDLE;

		VkFramebuffer xFramebuffer = VK_NULL_HANDLE;

		VkRenderPass xRenderPass = VK_NULL_HANDLE;

		VkPipeline xPipeline = VK_NULL_HANDLE;
		VkPipelineLayout xPipelineLayout = VK_NULL_HANDLE;

		BufferParameters xVertexBuffer;

		BufferParameters xIndexBuffer;

		VkDescriptorPool xDescriptorPool = VK_NULL_HANDLE;

		BufferParameters globalStagingBuffer;

#if defined (_DEBUG)
		VkDebugReportCallbackEXT xReportCallback = VK_NULL_HANDLE;
#endif

	};

	struct XVirtualFrameData
	{
		VkFence CPUGetChargeOfCmdBufFence;
		VkSemaphore swapchainImageAvailableSemphore;
		VkSemaphore renderFinishedSemphore;
		VkCommandBuffer commandBuffer;
		VkFramebuffer frameBuffer;
		ImageParameters depthImage;
	};
}