#pragma once
#include <fstream>
#include <iostream>
#include "XavierBase.h"
#include <vector>

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

namespace Xavier {

    struct ImageParameters {
        VkImage handle;
        VkImageView view;
        VkSampler sampler;
        VkDeviceMemory memory;

        ImageParameters() :
            handle(VK_NULL_HANDLE),
            view(VK_NULL_HANDLE),
            sampler(VK_NULL_HANDLE),
            memory(VK_NULL_HANDLE)
        {   }

    };

    struct BufferParameters
    {
        VkBuffer handle;
        VkDeviceMemory memory;
        uint32_t size;
        BufferParameters() :
            handle(VK_NULL_HANDLE),
            memory(VK_NULL_HANDLE),
            size(0u)
        {   }
    };

    struct XSwapchain {
        VkSwapchainKHR handle;
        std::vector<ImageParameters> images;
        VkExtent2D extent;
        VkFormat format;
        XSwapchain() :
            handle(VK_NULL_HANDLE),
            images(),
            extent(),
            format(VK_FORMAT_UNDEFINED)
        { }
    };

    struct XVertexData {
        struct Position {
            float x, y, z; 
        } pos;

        struct Color {
            float r, g, b, a;
        } color;
    };
    
    struct XRenderParas {

        std::vector<const char *> xLayers;
        std::vector<const char *> xInstanceExtensions;
        std::vector<const char *> xDeviceExtensions;

        VkInstance xInstance;
        VkPhysicalDevice xPhysicalDevice;

        VkDevice xDevice;

        uint32_t xPresentFamilyQueueIndex;
        uint32_t xGraphicFamilyQueueIndex;

        VkQueue xGraphicQueue;
        VkQueue xPresentQueue;

        VkSurfaceKHR xSurface;

        XSwapchain xSwapchain;

        VkCommandPool xRenderCmdPool;
        
        VkCommandBuffer xCopyCmd;
        VkFence xCopyFence;

        VkFramebuffer xFramebuffer;

        VkRenderPass xRenderPass;

        VkPipeline xPipeline;
        VkPipelineLayout xPipelineLayout;

        BufferParameters xVertexBuffer;

        BufferParameters xIndexBuffer;

        VkDescriptorPool xDescriptorPool;
        

#if defined (_DEBUG)
        VkDebugReportCallbackEXT xReportCallback;
#endif
        
    };

    struct XVirtualFrameData {
        VkFence CPUGetChargeOfCmdBufFence;
        VkSemaphore swapchainImageAvailableSemphore;
        VkSemaphore renderFinishedSemphore;
        VkCommandBuffer commandBuffer;
        VkFramebuffer frameBuffer;
    };

    class XRender : public XRenderBase
    {
        // Methods
    public:
        bool onWindowSizeChanged() override;
        bool Prepare(SDLWindow &win);

        virtual bool prepareRenderSample() = 0;

    private:
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
        bool checkInstanceLayersSupport(const std::vector<VkLayerProperties> &layersProperties, const char *targetLayer);
        bool checkInstanceExtensionsSupport(const std::vector<VkExtensionProperties> &extensionProperties, const char *targetExtension);
        bool checkPhysicalDeviceExtensionsSupport(const std::vector<VkExtensionProperties> &extensionProperties, const char *targetExtension);

        
        // helper functions.
        uint32_t xGetImagesCountForSwapchain(const VkSurfaceCapabilitiesKHR &surfaceCap)
        {
            uint32_t rst = surfaceCap.minImageCount + 2;
            if (surfaceCap.maxImageCount > 0 && rst > surfaceCap.maxImageCount)
                rst = surfaceCap.maxImageCount;
            return rst;
        }

        VkSurfaceFormatKHR xGetImageFormatForSwapchain(const std::vector<VkSurfaceFormatKHR> &surfaceSupportFormats)
        {
            VkSurfaceFormatKHR rst;
            if (surfaceSupportFormats.size() == 1 && surfaceSupportFormats[0].format == VK_FORMAT_UNDEFINED)
            {
                rst.format = VK_FORMAT_R8G8B8A8_UNORM;
                rst.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
                return rst;
            }

            for (const VkSurfaceFormatKHR &format : surfaceSupportFormats)
            {
                if (format.format == VK_FORMAT_R8G8B8A8_UNORM)
                    return format;
            }
            return surfaceSupportFormats[0];
        }

        VkExtent2D xGetImageExtent(const VkSurfaceCapabilitiesKHR &surfaceCap)
        {
            VkExtent2D currentExtent = surfaceCap.currentExtent;
            if (currentExtent.height == -1)
            {
                return surfaceCap.maxImageExtent;
            }
            return currentExtent;
        }

        VkImageUsageFlags xGetImageUsage(const VkSurfaceCapabilitiesKHR &surfaceCap)
        {
            VkImageUsageFlags surfaceSupportUsage = surfaceCap.supportedUsageFlags;
            if (surfaceSupportUsage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
                return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            std::cout << "The images which the surface provides can not be used for color attachment." << std::endl;
            return static_cast<VkImageUsageFlags>(-1);
        }

        VkSurfaceTransformFlagBitsKHR xGetImageTransform(const VkSurfaceCapabilitiesKHR &surfaceCap)
        {
            if (surfaceCap.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
            {
                return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            }
            return surfaceCap.currentTransform;
        }

        VkPresentModeKHR xGetSurfacePresentMode(const std::vector<VkPresentModeKHR> &surfaceSupportPresentModes)
        {
            for (VkPresentModeKHR mode : surfaceSupportPresentModes)
            {
                if (mode & VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    return mode;
                }
            }

            for (VkPresentModeKHR mode : surfaceSupportPresentModes)
            {
                if (mode & VK_PRESENT_MODE_IMMEDIATE_KHR)
                {
                    return mode;
                }
            }

            for (VkPresentModeKHR mode : surfaceSupportPresentModes)
            {
                if (mode & VK_PRESENT_MODE_FIFO_KHR)
                {
                    return mode;
                }
            }

            std::cout << "FIFO present mode is not supported by the surface" << std::endl;
            return static_cast<VkPresentModeKHR>(-1);
        }

    protected:
        std::vector<char> m_getFileBinaryContent(const std::string &fileName)
        {
            std::vector<char> bytes;
            std::ifstream file(fileName, std::ios::binary);
            if (file.fail())
            {
                std::cout << "Open File: " << fileName.c_str() << "Failed !" << std::endl;
            }
            else
            {
                std::streampos begin, end;
                begin = file.tellg();
                file.seekg(0, std::ios::end);
                end = file.tellg();

                // Read the file from begin to end.
                bytes.resize(end - begin);
                file.seekg(0, std::ios::beg);
                file.read(bytes.data(), static_cast<size_t>(end - begin));
            }
            return bytes;
        }

        std::string m_getAssetPath()
        {
            return SAMPLE_DATA_DIR;
        }

        std::string m_getTexturePath()
        {
            return m_getAssetPath() + "/teapot/";
        }

        bool allocateBufferMemory(VkBuffer buffer, VkDeviceMemory *memory, VkMemoryPropertyFlags addtionProps);
        bool allocateImageMemory(VkImage image, VkDeviceMemory * memory, VkMemoryPropertyFlags addtionProps);

      protected:
        // 9 Attributes
        SDLWindowPara winParam;
        XRenderParas xParams;

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
        const char *                                 pLayerPrefix,
        const char *                                 pMessage,
        void *                                       pUserData);

#endif

}