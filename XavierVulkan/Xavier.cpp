/*
 * Vulkan Windowed Program
 *
 * Copyright (C) 2016, 2018 Valve Corporation
 * Copyright (C) 2016, 2018 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
Vulkan Windowed Project Template
Create and destroy a Vulkan surface on an SDL window.
*/

// Enable the WSI extensions
#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <iostream>
#include <vector>
#include "Xavier.h"


namespace Xavier
{
    bool XRender::Draw()
    {
        
    }

    bool XRender::onWindowSizeChanged()
    {
        
    }

    bool XRender::prepareXRenderer(SDLWindow &win)
    {
        // Initialization for Vulkan Infrastructure.
        winParam = win.getSDLWindowParameters();
        prepareVulkan();
        // Prepare all the things before we get into the render loop. 
        // Including:
        // Load Resource Data;
        // Create Buffers and Images for the data.
        // Create Render Passes;
        // Create Descriptor Sets (Layouts?);
        // Create Pipelines;
        // Record the commands into command buffer.
        prepareRenderSample();
        
    }

    bool XRender::prepareVulkan()
    {

        if (!createInstance()) { return false; }
#if defined (_DEBUG)
        if (!initDebug()) { return false; }
#endif 
        if (!createSurface()) { return false; }
        if (!createDevice()) { return false; }
        if (!createDeviceQueue()) { return false; }
        if (!createSwapChain()) { return false; }

        return true;
    }

    bool XRender::createInstance()
    {
        // Get WSI extensions from SDL (we can add more if we like - we just can't remove these)
        unsigned extension_count;
        if (!SDL_Vulkan_GetInstanceExtensions(winParam.pWindow, &extension_count, NULL))
        {
            std::cout << "Could not get the number of required instance extensions from SDL." << std::endl;
            return false;
        }

        xParams.xInstanceExtensions.resize(extension_count);
        if (!SDL_Vulkan_GetInstanceExtensions(winParam.pWindow, &extension_count, xParams.xInstanceExtensions.data()))
        {
            std::cout << "Could not get the names of required instance extensions from SDL." << std::endl;
            return false;
        }

        // Use validation layers if this is a debug build
#if defined(_DEBUG)
        xParams.xLayers.push_back("VK_LAYER_LUNARG_standard_validation");
#endif

        // VkApplicationInfo allows the programmer to specify basic information about the
        // program, which can be useful for layers and tools to provide more debug information.
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = NULL;
        appInfo.pApplicationName = "Vulkan Program Template";
        appInfo.applicationVersion = 1;
        appInfo.pEngineName = "LunarG SDK";
        appInfo.engineVersion = 1;
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // VkInstanceCreateInfo is where the programmer specifies the layers and/or extensions that
        // are needed.
        VkInstanceCreateInfo instInfo = {};
        instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instInfo.pNext = NULL;
        instInfo.flags = 0;
        instInfo.pApplicationInfo = &appInfo;
        instInfo.enabledExtensionCount = static_cast<uint32_t>(xParams.xInstanceExtensions.size());
        instInfo.ppEnabledExtensionNames = xParams.xInstanceExtensions.data();
        instInfo.enabledLayerCount = static_cast<uint32_t>(xParams.xLayers.size());
        instInfo.ppEnabledLayerNames = xParams.xLayers.data();

        // Create the Vulkan instance.
        VkResult result = vkCreateInstance(&instInfo, NULL, &xParams.xInstance);
        switch (result)
        {
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            std::cout << "Unable to find a compatible Vulkan Driver." << std::endl;
            return false;
        case VK_SUCCESS:
            break;
        default:
            std::cout << "Could not create a Vulkan instance (for unknown reasons)." << std::endl;
            return false;
        }
        return true;
    }

#if defined(_DEBUG)
    VKAPI_ATTR VkBool32 VKAPI_PTR zvDebugReportCallback(
        VkDebugReportFlagsEXT                        flags,
        VkDebugReportObjectTypeEXT                   objectType,
        uint64_t                                     object,
        size_t                                       location,
        int32_t                                      messageCode,
        const char *                                 pLayerPrefix,
        const char *                                 pMessage,
        void *                                       pUserData)
    {
        std::cout << "FlagsEXT: " << flags << "  "
            << "ObjectTypeEXT: " << objectType << "  "
            << "obj: " << object << "  "
            << "location: " << location << "  "
            << "messageCode: " << messageCode << "  "
            << "Layer Prefix: " << pLayerPrefix << "  "
            << "Message:" << pMessage << "  "
            ;
        return VK_FALSE;
    }

    bool XRender::initDebug()
    {
        VkDebugReportFlagsEXT interestedMask = VK_DEBUG_REPORT_INFORMATION_BIT_EXT 
            & VK_DEBUG_REPORT_WARNING_BIT_EXT
            & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
            & VK_DEBUG_REPORT_ERROR_BIT_EXT
            & VK_DEBUG_REPORT_DEBUG_BIT_EXT;
        VkDebugReportCallbackCreateInfoEXT debugReportCallbackInfo = {};
        debugReportCallbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        debugReportCallbackInfo.pNext = nullptr;
        debugReportCallbackInfo.flags = interestedMask;
        debugReportCallbackInfo.pfnCallback = static_cast<PFN_vkDebugReportCallbackEXT>(zvDebugReportCallback);
        debugReportCallbackInfo.pUserData = "X-Renderer is using debug report extension !!";

        if (vkCreateDebugReportCallbackEXT(xParams.xInstance, &debugReportCallbackInfo, nullptr, &xParams.xReportCallback) != VK_SUCCESS)
        {
            std::cout << "Creating DebugCallback Failed !" << std::endl;
            return false;
        }
        return true;
    }
#endif

    bool XRender::createSurface()
    {
        if (!SDL_Vulkan_CreateSurface(winParam.pWindow, xParams.xInstance, &xParams.xSurface)) {
            std::cout << "Could not create a Vulkan surface." << std::endl;
            return false;
        }
        return true;
    }

    bool XRender::createDevice()
    {
        // Enumerate all physical devices.
        unsigned num_physical_devices;
        if (vkEnumeratePhysicalDevices(xParams.xInstance, &num_physical_devices, nullptr) != VK_SUCCESS ||
            num_physical_devices == 0)
        {
            std::cout << "Enumerating Physical Device Failed !";
            return false;
        }

        std::vector<VkPhysicalDevice> physical_devices(num_physical_devices);
        if (vkEnumeratePhysicalDevices(xParams.xInstance, &num_physical_devices, physical_devices.data()) != VK_SUCCESS ||
            num_physical_devices == 0)
        {
            std::cout << "Enumerating Physical Device Failed !";
            return false;
        }

        xParams.xDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        uint32_t graphicQueueFamilyIndex = UINT32_MAX;
        uint32_t presentQueueFamilyIndex = UINT32_MAX;
        for (VkPhysicalDevice physicalDevice : physical_devices)
        {
            if (!checkQueueFamilySupport(physicalDevice, graphicQueueFamilyIndex, presentQueueFamilyIndex))
            {
                std::cout << "Physical Device: " << physicalDevice << "not satisfied" << std::endl;
                return false;
            }
            else
            {
                xParams.xPhysicalDevice = physicalDevice;
                break;
            }
        }
        std::vector<float> queuePriorities = { 1.0f };
        VkDeviceQueueCreateInfo queueCreateInfos[2] = {};

        // Graphic queue.
        queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[0].pNext = nullptr;
        queueCreateInfos[0].flags = 0;
        queueCreateInfos[0].queueFamilyIndex = graphicQueueFamilyIndex;
        queueCreateInfos[0].queueCount = queuePriorities.size();
        queueCreateInfos[0].pQueuePriorities = queuePriorities.data();

        // Present queue.
        queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[1].pNext = nullptr;
        queueCreateInfos[1].flags = 0;
        queueCreateInfos[1].queueFamilyIndex = presentQueueFamilyIndex;
        queueCreateInfos[1].queueCount = queuePriorities.size();
        queueCreateInfos[1].pQueuePriorities = queuePriorities.data();

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext = nullptr;
        deviceCreateInfo.flags = 0;
        deviceCreateInfo.queueCreateInfoCount = sizeof(queueCreateInfos) / sizeof(VkDeviceQueueCreateInfo);
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfos[0];
        deviceCreateInfo.enabledLayerCount = 0;
        deviceCreateInfo.ppEnabledLayerNames = nullptr;
        deviceCreateInfo.enabledExtensionCount = xParams.xDeviceExtensions.size();
        deviceCreateInfo.ppEnabledExtensionNames = xParams.xDeviceExtensions.data();
        deviceCreateInfo.pEnabledFeatures = nullptr;

        if (!vkCreateDevice(xParams.xPhysicalDevice, &deviceCreateInfo, nullptr, &xParams.xDevice) != VK_SUCCESS)
        {
            std::cout << "Creation for the Device Failed !" << std::endl;
            return false;
        }

        xParams.xGraphicFamilyQueueIndex = graphicQueueFamilyIndex;
        xParams.xPresentFamilyQueueIndex = presentQueueFamilyIndex;
        return true;
    }

    bool XRender::checkQueueFamilySupport(VkPhysicalDevice physicalDevice, uint32_t &graphicQueueFamilyIndex, uint32_t &presentQueueFamilyIndex)
    {
        // Device Properties Check.
        {
            VkPhysicalDeviceFeatures physicalDeviceFeatures;
            VkPhysicalDeviceProperties physicalDeviceProperties;

            vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);
            vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

            uint32_t majorVersionPhysicalDeviceSupported = VK_VERSION_MAJOR(physicalDeviceProperties.apiVersion);
            if (majorVersionPhysicalDeviceSupported < 1 ||
                physicalDeviceProperties.limits.maxImageDimension2D < 4096)
            {
                // It's not the physical device we want.
                std::cout << "Physical Device:" << physicalDevice << "'s Properties don't meet the the need." << std::endl;
                return false;
            }
        }
        // Device Extension Support Check.
        {
            uint32_t extensionCount;
            if (vkEnumerateDeviceExtensionProperties(xParams.xPhysicalDevice, nullptr, &extensionCount, nullptr) != VK_SUCCESS ||
                extensionCount == 0)
            {
                std::cout << "Physical Device: " << physicalDevice << " Extension Enumeration Failed" << std::endl;
                return false;
            }

            std::vector<VkExtensionProperties> physicalDeviceExtensionProperties(extensionCount);
            if (vkEnumerateDeviceExtensionProperties(xParams.xPhysicalDevice, nullptr, &extensionCount, &physicalDeviceExtensionProperties[0]) != VK_SUCCESS ||
                extensionCount == 0)
            {
                std::cout << "Physical Device: " << physicalDevice << " Extension Enumeration Failed" << std::endl;
                return false;
            }

            for (const char *cstr : xParams.xDeviceExtensions)
            {
                if (!checkPhysicalDeviceExtensionsSupport(physicalDeviceExtensionProperties, cstr))
                {
                    std::cout << "Physical Device: " << physicalDevice << " do not support extensions: " << cstr << std::endl;
                    return false;
                }
            }
        }

        // Queue Family Present and Graphic Support Check.
        {
            uint32_t queueFamiliesCount;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, nullptr);
            if (queueFamiliesCount == 0)
            {
                std::cout << "Enumerating Queue Family Properties Failed !" << std::endl;
                return false;
            }

            std::vector<VkBool32> queueFamiliesSurfaceSupport(queueFamiliesCount);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamiliesCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, queueFamilies.data());

            uint32_t selectedGraphicQueueFamilyIndex = UINT32_MAX;
            uint32_t selectedPresentQueueFamilyIndex = UINT32_MAX;
            for (uint32_t j = 0; j < queueFamiliesCount; ++j)
            {
                if (!vkGetPhysicalDeviceSurfaceSupportKHR(xParams.xPhysicalDevice, j, xParams.xSurface, &queueFamiliesSurfaceSupport[j]) != VK_SUCCESS)
                {
                    std::cout << "Can not get whether queue family: " << j << " in Physical Device: " << physicalDevice << " supports surface extension !" << std::endl;
                    return false;
                }
                // Search for the queue family that support graphic.
                // When we've already found a graphic queue family, we still try to test if it support both graphic and present if we've not fount the one that support both.
                if (queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT && queueFamilies[j].queueCount > 0)
                {
                    if (selectedGraphicQueueFamilyIndex == UINT32_MAX)
                        selectedGraphicQueueFamilyIndex = j;
                    if (queueFamiliesSurfaceSupport[j])
                    {
                        selectedPresentQueueFamilyIndex = j;
                        break;
                    }
                }
                // If it's not the one which support both, just register it as the present one if supported.
                if (selectedPresentQueueFamilyIndex == UINT32_MAX && queueFamiliesSurfaceSupport[j])
                    selectedPresentQueueFamilyIndex = j;
            }

            if (selectedGraphicQueueFamilyIndex != UINT32_MAX && selectedPresentQueueFamilyIndex != UINT32_MAX)
            {
                graphicQueueFamilyIndex = selectedGraphicQueueFamilyIndex;
                presentQueueFamilyIndex = selectedPresentQueueFamilyIndex;
                return true;
            }
        }
        return false;
    }

    bool XRender::checkPhysicalDeviceExtensionsSupport(const std::vector<VkExtensionProperties> &extensions, const char *targetExtension)
    {
        for (size_t i = 0; i < extensions.size(); ++i)
        {
            const char *ext = extensions[i].extensionName;
            if (strcmp(ext, targetExtension) == 0)
                return true;
        }
        return false;
    }

    bool XRender::createDeviceQueue()
    {
        vkGetDeviceQueue(xParams.xDevice, xParams.xGraphicFamilyQueueIndex, 0, &xParams.xGraphicQueue);
        vkGetDeviceQueue(xParams.xDevice, xParams.xPresentFamilyQueueIndex, 0, &xParams.xPresentQueue);
        return true;
    }

    bool XRender::createSwapChain()
    {
        // This function will be called not only when we prepare the Vulkan for rendering, but also recreate one due to the window size changing,
        // So we must make sure the device is in the idle state and clear all the structure about the old swapchain before when we do it.

        if (xParams.xDevice != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(xParams.xDevice);
        }

        for (ImageParameters &img : xParams.xSwapchain.images)
        {
            if (img.view != VK_NULL_HANDLE)
            {
                vkDestroyImageView(xParams.xDevice, img.view, nullptr);
                img.view = VK_NULL_HANDLE;
            }
        }
        xParams.xSwapchain.images.clear();

        // Get the capacities of the surface.
        VkSurfaceCapabilitiesKHR surfaceCapacities;
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(xParams.xPhysicalDevice, xParams.xSurface, &surfaceCapacities) != VK_SUCCESS)
        {
            std::cout << "Could not check Presentation surface capacities !" << std::endl;
            return false;
        }

        // Get the formats of the surface.
        uint32_t formatsCount;
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(xParams.xPhysicalDevice, xParams.xSurface, &formatsCount, nullptr) != VK_SUCCESS
            || formatsCount == 0)
        {
            std::cout << "Error occurred during presentation surface formats enumeration!" << std::endl;
            return false;
        }

        std::vector<VkSurfaceFormatKHR> surfaceFormats(formatsCount);
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(xParams.xPhysicalDevice, xParams.xSurface, &formatsCount, &surfaceFormats[0]) != VK_SUCCESS)
        {
            std::cout << "Error occurred during presentation surface formats enumeration!" << std::endl;
            return false;
        }

        // Get the present mode of the surface.
        uint32_t presentModesCount;
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(xParams.xPhysicalDevice, xParams.xSurface, &presentModesCount, nullptr) != VK_SUCCESS
            || presentModesCount == 0)
        {
            std::cout << "Error occurred during presentation surface present modes enumeration!" << std::endl;
            return false;
        }

        std::vector<VkPresentModeKHR> surfacePresentModes(presentModesCount);
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(xParams.xPhysicalDevice, xParams.xSurface, &presentModesCount, &surfacePresentModes[0]) != VK_SUCCESS)
        {
            std::cout << "Error occurred during presentation surface present modes enumeration!" << std::endl;
            return false;
        }

        uint32_t imagesCountInSwapchain = xGetImagesCountForSwapchain(surfaceCapacities);
        VkSurfaceFormatKHR imagesFormat = xGetImageFormatForSwapchain(surfaceFormats);
        VkExtent2D imagesExtent = xGetImageExtent(surfaceCapacities);
        VkImageUsageFlags imagesUsage = xGetImageUsage(surfaceCapacities);
        VkSurfaceTransformFlagBitsKHR imagesTransform = xGetImageTransform(surfaceCapacities);
        VkPresentModeKHR swapchainPresentMode = xGetSurfacePresentMode(surfacePresentModes);
        VkSwapchainKHR oldSwapchain = xParams.xSwapchain.handle;

        VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.pNext = nullptr;
        swapchainCreateInfo.flags = 0;
        swapchainCreateInfo.surface = xParams.xSurface;
        swapchainCreateInfo.minImageCount = imagesCountInSwapchain;
        swapchainCreateInfo.imageFormat = imagesFormat.format;
        swapchainCreateInfo.imageColorSpace = imagesFormat.colorSpace;
        swapchainCreateInfo.imageExtent = imagesExtent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = imagesUsage;
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = nullptr;
        swapchainCreateInfo.preTransform = imagesTransform;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = swapchainPresentMode;
        swapchainCreateInfo.clipped = VK_FALSE;
        swapchainCreateInfo.oldSwapchain = oldSwapchain;

        if (vkCreateSwapchainKHR(xParams.xDevice, &swapchainCreateInfo, nullptr, &xParams.xSwapchain.handle) != VK_SUCCESS)
        {
            std::cout << "Could not create swapchain !" << std::endl;
            return false;
        }

        if (oldSwapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(xParams.xDevice, oldSwapchain, nullptr);
        }

        xParams.xSwapchain.format = imagesFormat.format;
        
        uint32_t imagesCount = 0;
        if (vkGetSwapchainImagesKHR(xParams.xDevice, xParams.xSwapchain.handle, &imagesCount, nullptr) != VK_SUCCESS
            || imagesCount == 0)
        {
            std::cout << "Could not get swap chain images !" << std::endl;
            return false;
        }
        
        std::vector<VkImage> images(imagesCount);
        if (vkGetSwapchainImagesKHR(xParams.xDevice, xParams.xSwapchain.handle, &imagesCount, images.data()) != VK_SUCCESS)
        {
            std::cout << "Could not get swap chain images !" << std::endl;
            return false;
        }

        xParams.xSwapchain.images.resize(imagesCount);
        for (size_t i = 0; i < imagesCount; ++i)
        {
            xParams.xSwapchain.images[i].handle = images[i];

            // Create image view for each image.
            VkImageViewCreateInfo imageViewCreateInfo = {};
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCreateInfo.pNext = nullptr;
            imageViewCreateInfo.flags = 0;
            imageViewCreateInfo.image = xParams.xSwapchain.images[i].handle;
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.format = xParams.xSwapchain.format;
            imageViewCreateInfo.components = {
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY 
            };
            imageViewCreateInfo.subresourceRange = {
                VK_IMAGE_ASPECT_COLOR_BIT,
                0,
                1,
                0,
                1
            };

            if (vkCreateImageView(xParams.xDevice, &imageViewCreateInfo, nullptr, &xParams.xSwapchain.images[i].view) != VK_SUCCESS)
            {
                std::cout << "Could not create image view for the image: No." << i << " !" << std::endl;
                return false;
            }

        }
        return true;
    }

}

int main()
{
    // Create a Vulkan surface for rendering

    // This is where most initialization for a program should be performed

    // Poll for user input.
    bool stillRunning = true;
    while (stillRunning) {

        SDL_Event event;
        while (SDL_PollEvent(&event)) {

            switch (event.type) {

            case SDL_QUIT:
                stillRunning = false;
                break;

            default:
                // Do nothing. 
                std::cout << "Xavier" << std::endl;
                break;
            }
        }


    }

    return 0;
}