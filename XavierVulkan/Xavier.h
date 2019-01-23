#pragma once
#include "XavierBase.h"
#include <vector>

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

#if defined (_DEBUG)
        VkDebugReportCallbackEXT xReportCallback;
#endif
        
    };

    struct XVirtualFrameData {
        VkFence CPUGetChargeCmdBuff;
        VkSemaphore swapchainImageAvailableSignal;
        VkSemaphore renderFinishedSignal;
        VkCommandBuffer commandBuffer;

        VkPipeline pipeline;

    };

    class XRender : public XRenderBase
    {
        // Methods
    public:
        bool Draw() override;
        bool onWindowSizeChanged() override;
        bool prepareXRenderer(SDLWindow &win);

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

        bool checkQueueFamilySupport(VkPhysicalDevice physicalDevice, uint32_t &graphicQueueFamilyIndex, uint32_t &presentQueueFamilyIndex);
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