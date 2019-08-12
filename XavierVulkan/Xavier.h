#pragma once

#include "XStruct.h"
#include "XavierBase.h"
#include "XShader.h"

#include <vector>
#include <fstream>
#include <iostream>
// Macro for return value checking of the Vulkan API calls.
#define ZV_VK_CHECK( x ) { \
	VkResult ret = x; \
	if ( ret != VK_SUCCESS ) { \
        std::cout << "VK: " << #x << ", error code: " << ret << std::endl; \
        return false; \
    } \
}

#define ZV_VK_VALIDATE( x, msg ) { \
	if ( !( x ) ) { \
        std::cout << "VK: " << msg << " - " << #x << std::endl; \
        return false; \
    } \
}

namespace Xavier
{

	class XRender : public XRenderBase
	{
		// Methods
	public:
		bool onWindowSizeChanged() override;
		bool Prepare(SDLWindow &win);

		virtual bool prepareRenderSample() = 0;

	protected:
		bool prepareVulkan();
		bool createInstance();
#if defined (_DEBUG)
		bool initDebug();
#endif 
		bool createSurface();
		bool createDevice();
		bool createDeviceQueue();
		bool createSwapChain();
		bool createCommandPool();
		bool createCopyCommand();

		bool checkQueueFamilySupport(VkPhysicalDevice physicalDevice, uint32_t &graphicQueueFamilyIndex, uint32_t &presentQueueFamilyIndex);
		bool checkQueueFamilySupportForCompute(VkPhysicalDevice physicalDevice, uint32_t &computeQueueFamilyIndex);
		bool checkInstanceLayersSupport(const std::vector<VkLayerProperties> &layersProperties, const char *targetLayer);
		bool checkInstanceExtensionsSupport(const std::vector<VkExtensionProperties> &extensionProperties, const char *targetExtension);
		bool checkPhysicalDeviceExtensionsSupport(const std::vector<VkExtensionProperties> &extensionProperties, const char *targetExtension);

		bool prepareContextParams();

		bool prepareVirtualFrames();
		bool createBuffer(BufferParameters &buff, VkMemoryPropertyFlags flags);
		bool createDepthBuffer(ImageParameters &depthImage);
		bool createGlobalStagingBuffer();

		VkAccessFlags getBufferAccessTypes(VkBufferUsageFlags flags, VkPipelineStageFlags consumingStages);
		bool wholeBufferPipelineBarrier(BufferParameters &bufferParams, VkPipelineStageFlags srcStageFlags, VkPipelineStageFlags dstStageFlags, VkAccessFlags srcAccess, VkAccessFlags dstAccess);
		bool getDataForBuffer(void *data, uint32_t size,
			BufferParameters &bufferParams,
			VkPipelineStageFlags consumingStage);
	
		VkAccessFlags getImageAccessTypes(VkImageUsageFlags flags, VkPipelineStageFlags consumingStages);
		bool imagePipelineBarrier(ImageParameters &image, VkPipelineStageFlags srcStageFlags, VkPipelineStageFlags dstStageFlags, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkImageLayout oldLayout, VkImageLayout newLayout);
		bool getDataForImage2D(void *data, uint32_t size,
			ImageParameters &bufferParams,
			VkPipelineStageFlags consuingStages);

		// helper functions.
		uint32_t xGetImagesCountForSwapchain(const VkSurfaceCapabilitiesKHR &surfaceCap)
		{
			uint32_t rst = surfaceCap.minImageCount + 2;
			if ( surfaceCap.maxImageCount > 0 && rst > surfaceCap.maxImageCount )
				rst = surfaceCap.maxImageCount;
			return rst;
		}

		VkSurfaceFormatKHR xGetImageFormatForSwapchain(const std::vector<VkSurfaceFormatKHR> &surfaceSupportFormats)
		{
			VkSurfaceFormatKHR rst;
			if ( surfaceSupportFormats.size() == 1 && surfaceSupportFormats[0].format == VK_FORMAT_UNDEFINED )
			{
				rst.format = VK_FORMAT_R8G8B8A8_UNORM;
				rst.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
				return rst;
			}

			for ( const VkSurfaceFormatKHR &format : surfaceSupportFormats )
			{
				if ( format.format == VK_FORMAT_R8G8B8A8_UNORM )
					return format;
			}
			return surfaceSupportFormats[0];
		}

		VkExtent2D xGetImageExtent(const VkSurfaceCapabilitiesKHR & surfaceCap)
		{
			VkExtent2D currentExtent = surfaceCap.currentExtent;
			if ( currentExtent.height == -1 )
			{
				return surfaceCap.maxImageExtent;
			}
			return currentExtent;
		}

		VkImageUsageFlags xGetImageUsage(const VkSurfaceCapabilitiesKHR & surfaceCap)
		{
			VkImageUsageFlags surfaceSupportUsage = surfaceCap.supportedUsageFlags;
			if ( surfaceSupportUsage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
				return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			std::cout << "The images which the surface provides can not be used for color attachment." << std::endl;
			return static_cast<VkImageUsageFlags>(-1);
		}

		VkSurfaceTransformFlagBitsKHR xGetImageTransform(const VkSurfaceCapabilitiesKHR & surfaceCap)
		{
			if ( surfaceCap.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR )
			{
				return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			}
			return surfaceCap.currentTransform;
		}

		VkPresentModeKHR xGetSurfacePresentMode(const std::vector<VkPresentModeKHR> & surfaceSupportPresentModes)
		{
			for ( VkPresentModeKHR mode : surfaceSupportPresentModes )
			{
				if ( mode == VK_PRESENT_MODE_MAILBOX_KHR )
				{
					return mode;
				}
			}

			for ( VkPresentModeKHR mode : surfaceSupportPresentModes )
			{
				if ( mode == VK_PRESENT_MODE_IMMEDIATE_KHR )
				{
					return mode;
				}
			}

			for ( VkPresentModeKHR mode : surfaceSupportPresentModes )
			{
				if ( mode == VK_PRESENT_MODE_FIFO_KHR )
				{
					return mode;
				}
			}

			std::cout << "FIFO present mode is not supported by the surface" << std::endl;
			return static_cast<VkPresentModeKHR>(-1);
		}

	protected:
		std::vector<char> m_getFileBinaryContent(const std::string & fileName)
		{
			return utils::m_getFileBinaryContent(fileName);
		}

		std::string m_getAssetPath()
		{
			return "Data/";
		}

		std::string m_getTexturePath()
		{
			return m_getAssetPath() + "teapot/";
		}

		bool allocateBufferMemory(VkBuffer buffer, VkDeviceMemory * memory, VkMemoryPropertyFlags addtionProps);
		bool allocateImageMemory(VkImage image, VkDeviceMemory * memory, VkMemoryPropertyFlags addtionProps);

	protected:
		// 9 Attributes
		SDLWindowPara winParam;
		XRenderParas xParams;
		ContextParams xContextParams;

		std::vector<XVirtualFrameData> xVirtualFrames;
		size_t currentFrameIndex;

	};

#if defined(_DEBUG)
	VKAPI_ATTR VkBool32 VKAPI_PTR zvDebugReportCallback(
		VkDebugReportFlagsEXT                        flags,
		VkDebugReportObjectTypeEXT                   objectType,
		uint64_t                                     object,
		size_t                                       location,
		int32_t                                      messageCode,
		const char *pLayerPrefix,
		const char *pMessage,
		void *pUserData);

#endif

}