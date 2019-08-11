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

#include <algorithm>
#include <sstream>
#include <map>
#include <iostream>
#include <vector>
#include "Xavier.h"


namespace Xavier
{
	// According to the vulkan specification.
	std::map<VkAccessFlagBits, VkPipelineStageFlags> pipelineStagesForAccess =
	{
		{ VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT },
		{ VK_ACCESS_INDEX_READ_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT },
		{ VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT},
		{ VK_ACCESS_UNIFORM_READ_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT },
		{ VK_ACCESS_SHADER_READ_BIT,  VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT },
		{ VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT }, 
		{ VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT },
		{ VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
		{ VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
		{ VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT },
		{ VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT },
		{ VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT },
		{ VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT },
		{ VK_ACCESS_HOST_READ_BIT, VK_PIPELINE_STAGE_HOST_BIT },
		{ VK_ACCESS_HOST_WRITE_BIT, VK_PIPELINE_STAGE_HOST_BIT },
	};

    bool XRender::onWindowSizeChanged()
    {
        return true;
    } 

    bool XRender::Prepare(SDLWindow &win)
    {
        // Initialization for Vulkan Infrastructure.
        winParam = win.getSDLWindowParameters();
        if (!prepareVulkan()) { return false; }
         
        // Prepare all the things before we get into the render loop. 
        // Including:
        // Load Resource Data;
        // Create Buffers and Images for the data.
        // Create Render Passes;
        // Create Descriptor Sets (Layouts?);
        // Create Pipelines;
        // Record the commands into command buffer.
        if (!prepareRenderSample()) { return false; }
        return true;
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
        
        if (!createCommandPool()) { return false; }
        if (!createCopyCommand()) { return false; }
		
		prepareContextParams();
        return true;
    }

    bool XRender::createCommandPool()
    {
        VkCommandPoolCreateInfo cmdPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        cmdPoolCreateInfo.queueFamilyIndex = xParams.xGraphicFamilyQueueIndex;
        cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        ZV_VK_CHECK(vkCreateCommandPool(xParams.xDevice, &cmdPoolCreateInfo, nullptr, &xParams.xRenderCmdPool));
         
        return true;
    }

    bool XRender::createCopyCommand()
    {
        VkCommandBufferAllocateInfo cpyCmdAllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        cpyCmdAllocInfo.commandBufferCount = 1;
        cpyCmdAllocInfo.pNext = nullptr;
        cpyCmdAllocInfo.commandPool = xParams.xRenderCmdPool;
        cpyCmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ZV_VK_CHECK(vkAllocateCommandBuffers(xParams.xDevice, &cpyCmdAllocInfo, &xParams.xCopyCmd));

        VkFenceCreateInfo cpyFenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        cpyFenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        cpyFenceCreateInfo.pNext = nullptr;
        ZV_VK_CHECK(vkCreateFence(xParams.xDevice, &cpyFenceCreateInfo, nullptr, &xParams.xCopyFence));

        return true;
    }
    bool XRender::createInstance()
    {
        // Get WSI extensions from SDL (we can add more if we like - we just can't remove these)
        unsigned extension_count = 0;
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

#if defined(_DEBUG)
        // Use validation layers if this is a debug build.
           xParams.xLayers.push_back("VK_LAYER_LUNARG_standard_validation");
           xParams.xLayers.push_back("VK_LAYER_RENDERDOC_Capture");
           xParams.xInstanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif
        // Enumerate all layers supported by Vulkan.
        uint32_t layerCount = 0;
        ZV_VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

        std::vector<VkLayerProperties> supportedLayers(layerCount);
        ZV_VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, supportedLayers.data()));

        // Check If the layers we need are all supported by the Vulkan.
        for (size_t i = 0; i < xParams.xLayers.size(); ++i)
        {
            if (!checkInstanceLayersSupport(supportedLayers, xParams.xLayers[i]))
            {
                std::cout << "Layer: " << xParams.xLayers[i] << "is not supported by Vulkan." << std::endl;
                return false;
            }
        }
        
        // Enumerate all instance extensions supported by Vulkan.
        uint32_t instanceExtensionsCount = 0;
        ZV_VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionsCount, nullptr));

        std::vector<VkExtensionProperties> instanceExtensions(instanceExtensionsCount);
        ZV_VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionsCount, instanceExtensions.data()));

        // Check If the instance extensions we need are all supported by the Vulkan.
        for (size_t i = 0; i < xParams.xInstanceExtensions.size(); ++i)
        {
            if (!checkInstanceExtensionsSupport(instanceExtensions, xParams.xInstanceExtensions[i]))
            {
                std::cout << "Instance Extensions: " << xParams.xInstanceExtensions[i] << "is not supported by the Vulkan !" << std::endl;
                return false;
            }
        }

        // VkApplicationInfo allows the programmer to specify basic information about the
        // program, which can be useful for layers and tools to provide more debug information.
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = NULL;
        appInfo.pApplicationName = "Zavi's Demo";
        appInfo.applicationVersion = 1;
        appInfo.pEngineName = "LunarG SDK";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

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
        int32_t                                      msgCode,
        const char *                                 pLayerPrefix,
        const char *                                 pMsg,
        void *                                       pUserData)
    {
        std::stringstream debugInfo;
        
        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        {
            debugInfo << "[ERROR]";
        }
        // Warnings may hint at unexpected / non-spec API usage
        if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        {
            debugInfo << "[WARNING]";
        }
        // May indicate sub-optimal usage of the API
        if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
        {
            debugInfo << "[PERFORMANCE]";
        }
        // Informal messages that may become handy during debugging
        if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
        {
            debugInfo << "[INFO]";
        }
        // Diagnostic info from the Vulkan loader and layers
        // Usually not helpful in terms of API usage, but may help to debug layer and loader problems 
        if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
        {
            debugInfo << "[DEBUG]";
        }

        // Display message to default output (console/logcat)
        std::stringstream debugMessage;
        debugInfo << " {" << pLayerPrefix << "} Code " << msgCode << " : " << pMsg;

        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        {
            std::cerr << debugInfo.str() << std::endl;
        }
        else
        {
            std::cout << debugInfo.str() << std::endl;
        }
        
        fflush(stdout);


        return VK_FALSE;
    }

    bool XRender::initDebug()
    {
        PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallback = NULL;
        PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallback = NULL;
        if (!(vkCreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(xParams.xInstance, "vkCreateDebugReportCallbackEXT")))
        {
            std::cout << "Can not get function: vkCreateDebugReportCallbackEXT" << std::endl;
            return false;
        }

        if (!(vkDestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(xParams.xInstance, "vkDestroyDebugReportCallbackEXT")))
        {
            std::cout << "Can not get function: vkDestroyDebugReportCallbackEXT" << std::endl;
            return false;
        }

        VkDebugReportFlagsEXT interestedMask = VK_DEBUG_REPORT_INFORMATION_BIT_EXT 
            | VK_DEBUG_REPORT_WARNING_BIT_EXT
            | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
            | VK_DEBUG_REPORT_ERROR_BIT_EXT
            | VK_DEBUG_REPORT_DEBUG_BIT_EXT;
        VkDebugReportCallbackCreateInfoEXT debugReportCallbackInfo = {};
        debugReportCallbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        debugReportCallbackInfo.pNext = nullptr;
        debugReportCallbackInfo.flags = interestedMask;
        debugReportCallbackInfo.pfnCallback = static_cast<PFN_vkDebugReportCallbackEXT>(zvDebugReportCallback);
        debugReportCallbackInfo.pUserData = "X-Renderer is using debug report extension !!";

        if (vkCreateDebugReportCallback(xParams.xInstance, &debugReportCallbackInfo, nullptr, &xParams.xReportCallback) != VK_SUCCESS)
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
        uint32_t computeQueueFamilyIndex = UINT32_MAX;
        for (VkPhysicalDevice physicalDevice : physical_devices)
        {
            if (!checkQueueFamilySupport(physicalDevice, graphicQueueFamilyIndex, presentQueueFamilyIndex))
            {
                std::cout << "Physical Device: " << physicalDevice << "not satisfied for graphics or presenting" << std::endl;
                return false;
            }
            else if (!checkQueueFamilySupportForCompute(physicalDevice, computeQueueFamilyIndex))
            {
                std::cout << "Physical Device: " << physicalDevice << "not satisfied for computing" << std::endl;
                return false;
            }
            else
            {
                xParams.xPhysicalDevice = physicalDevice;
                break;
            }
        }
        std::vector<float> queuePriorities = { 1.0f };
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        VkDeviceQueueCreateInfo graphicQueueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        // Graphic queue.
        graphicQueueCreateInfo.pNext = nullptr;
        graphicQueueCreateInfo.flags = 0;
        graphicQueueCreateInfo.queueFamilyIndex = graphicQueueFamilyIndex;
        graphicQueueCreateInfo.queueCount = queuePriorities.size();
        graphicQueueCreateInfo.pQueuePriorities = queuePriorities.data();
        queueCreateInfos.push_back(graphicQueueCreateInfo);

        // Present queue.
        if (presentQueueFamilyIndex != graphicQueueFamilyIndex)
        {
            VkDeviceQueueCreateInfo presentQueueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
            presentQueueCreateInfo.pNext = nullptr;
            presentQueueCreateInfo.flags = 0;
            presentQueueCreateInfo.queueFamilyIndex = presentQueueFamilyIndex;
            presentQueueCreateInfo.queueCount = queuePriorities.size();
            presentQueueCreateInfo.pQueuePriorities = queuePriorities.data();
            queueCreateInfos.push_back(presentQueueCreateInfo);
        }

        // Compute queue.
        if (computeQueueFamilyIndex != graphicQueueFamilyIndex && computeQueueFamilyIndex != presentQueueFamilyIndex)
        {
            VkDeviceQueueCreateInfo computeQueueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
            computeQueueCreateInfo.pNext = nullptr;
            computeQueueCreateInfo.flags = 0;
            computeQueueCreateInfo.queueFamilyIndex = presentQueueFamilyIndex;
            computeQueueCreateInfo.queueCount = queuePriorities.size();
            computeQueueCreateInfo.pQueuePriorities = queuePriorities.data();
            queueCreateInfos.push_back(computeQueueCreateInfo);
        }

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext = nullptr;
        deviceCreateInfo.flags = 0;
        deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfos[0];
        deviceCreateInfo.enabledLayerCount = 0;
        deviceCreateInfo.ppEnabledLayerNames = nullptr;
        deviceCreateInfo.enabledExtensionCount = xParams.xDeviceExtensions.size();
        deviceCreateInfo.ppEnabledExtensionNames = xParams.xDeviceExtensions.data();
        deviceCreateInfo.pEnabledFeatures = &xParams.xPhysicalDeviceFeatures;

        if (vkCreateDevice(xParams.xPhysicalDevice, &deviceCreateInfo, nullptr, &xParams.xDevice) != VK_SUCCESS)
        {
            std::cout << "Creation for the Device Failed !" << std::endl;
            return false;
        }

        xParams.xGraphicFamilyQueueIndex = graphicQueueFamilyIndex;
        xParams.xPresentFamilyQueueIndex = presentQueueFamilyIndex;
        xParams.xComputeFamilyQueueIndex = computeQueueFamilyIndex;
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
            uint32_t extensionCount = 0;
            if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr) != VK_SUCCESS ||
                extensionCount == 0)
            {
                std::cout << "Physical Device: " << physicalDevice << " Extension Enumeration Failed" << std::endl;
                return false;
            }

            std::vector<VkExtensionProperties> physicalDeviceExtensionProperties(extensionCount);
            if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, &physicalDeviceExtensionProperties[0]) != VK_SUCCESS ||
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
                if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, j, xParams.xSurface, &queueFamiliesSurfaceSupport[j]) != VK_SUCCESS)
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

    bool XRender::checkQueueFamilySupportForCompute(VkPhysicalDevice physicalDevice, uint32_t &computeQueueFamilyIndex)
    {
        uint32_t queueFamilyPropertyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);
        
        if (queueFamilyPropertyCount == 0)
        {
            std::cout << "queueFamilyCount error !" << std::endl;
            return false;
        }

        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

        computeQueueFamilyIndex = UINT32_MAX;
        for (uint32_t i = 0; i < queueFamilyPropertyCount; i++)
        {
            VkQueueFamilyProperties prop = queueFamilyProperties[i];
           if(prop.queueFlags & VK_QUEUE_COMPUTE_BIT && prop.queueFlags & VK_QUEUE_GRAPHICS_BIT == 0)
           {
               computeQueueFamilyIndex = i;
               return true;
           }
        }

        if (computeQueueFamilyIndex == UINT32_MAX)
        {
            for (uint32_t i = 0; i < queueFamilyPropertyCount; i++)
            {
                VkQueueFamilyProperties prop = queueFamilyProperties[i];
                if (prop.queueFlags & VK_QUEUE_COMPUTE_BIT)
                {
                    computeQueueFamilyIndex = i;
                    return true;
                }
            }
        }

        return false;
    }

    bool XRender::checkInstanceLayersSupport(const std::vector<VkLayerProperties>& layersProperties, const char * targetLayer)
    {
        for (VkLayerProperties layerProp : layersProperties)
        {
            if (strcmp(targetLayer, layerProp.layerName) == 0)
                return true;
        }
        return false;
    }

    bool XRender::checkInstanceExtensionsSupport(const std::vector<VkExtensionProperties>& extensionProperties, const char * targetExtension)
    {
        for (VkExtensionProperties extensionProp : extensionProperties)
        {
            if (strcmp(targetExtension, extensionProp.extensionName) == 0)
            return true;
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

	bool XRender::prepareContextParams()
	{
		VkPhysicalDeviceProperties deviceProp;
		vkGetPhysicalDeviceProperties(xParams.xPhysicalDevice, &deviceProp);
		// sample number limit
		VkSampleCountFlagBits commonMax;
		std::vector<int> counts;

		counts.push_back(xContextParams.frameBufferColorAttachImageMaxSampleNumber = static_cast<VkSampleCountFlagBits>(utils::maxFlagBit(deviceProp.limits.framebufferColorSampleCounts)));
		counts.push_back(xContextParams.frameBufferDepthStencilAttachImageMaxSampleNumber = static_cast<VkSampleCountFlagBits>(utils::maxFlagBit(deviceProp.limits.framebufferDepthSampleCounts)));
		counts.push_back(xContextParams.storageImageMaxSampleNumber = static_cast<VkSampleCountFlagBits>(utils::maxFlagBit(deviceProp.limits.storageImageSampleCounts)));
		int max = 0;
		for (int count : counts)
		{
			if (count > max)
				max = count;
		}
		xContextParams.storageImageMaxSampleNumber = xContextParams.frameBufferDepthStencilAttachImageMaxSampleNumber = xContextParams.frameBufferColorAttachImageMaxSampleNumber = static_cast<VkSampleCountFlagBits>(max);

		// format of images for depth buffer 
		std::vector<VkFormat> depthFormats = {
			VK_FORMAT_D32_SFLOAT,
		};

		xContextParams.depthStencilFormat = VK_FORMAT_MAX_ENUM;
		for (auto format : depthFormats)
		{
			VkFormatProperties formatProps;
			vkGetPhysicalDeviceFormatProperties(xParams.xPhysicalDevice, format, &formatProps);
			if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				xContextParams.depthStencilFormat = format;
				break;
			}
		}

		if (xContextParams.depthStencilFormat == VK_FORMAT_MAX_ENUM)
		{
			std::cout << "Not Format used for depth and stencil" << std::endl;
			return false;
		}

		// minImageTransferGranularity
		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(xParams.xPhysicalDevice, &queueFamilyCount, nullptr);
		if ( queueFamilyCount == 0 )
		{
			std::cout << "Fail to get Queue family properties" << std::endl;
			return false;
		}
		std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(xParams.xPhysicalDevice, &queueFamilyCount, queueFamilyProps.data());

		xContextParams.minImageTransferGranularityGraphics = queueFamilyProps[xParams.xGraphicFamilyQueueIndex].minImageTransferGranularity;
		xContextParams.minImageTransferGranularityPresent = queueFamilyProps[xParams.xPresentFamilyQueueIndex].minImageTransferGranularity;
		xContextParams.minImageTransferGranularityCompute = queueFamilyProps[xParams.xComputeFamilyQueueIndex].minImageTransferGranularity;
		
		return true;
	}

	bool XRender::prepareVirtualFrames()
	{
		xVirtualFrames.resize(10);

		// Create Virtual Frame including:
		// 2 semaphores, 1 fence and 1 command buffer
		for (size_t i = 0; i < xVirtualFrames.size(); ++i)
		{
			VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
			ZV_VK_CHECK(vkCreateSemaphore(xParams.xDevice, &semaphoreCreateInfo, nullptr, &xVirtualFrames[i].swapchainImageAvailableSemphore));
			ZV_VK_CHECK(vkCreateSemaphore(xParams.xDevice, &semaphoreCreateInfo, nullptr, &xVirtualFrames[i].renderFinishedSemphore));

			VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
			fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			ZV_VK_CHECK(vkCreateFence(xParams.xDevice, &fenceCreateInfo, nullptr, &xVirtualFrames[i].CPUGetChargeOfCmdBufFence));

			// Allocating a command buffer for the virtual frame.
			VkCommandBufferAllocateInfo cmdbufAllocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
			cmdbufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmdbufAllocateInfo.commandBufferCount = 1;
			cmdbufAllocateInfo.commandPool = xParams.xRenderCmdPool;

			ZV_VK_CHECK(vkAllocateCommandBuffers(xParams.xDevice, &cmdbufAllocateInfo, &xVirtualFrames[i].commandBuffer));

			ZV_VK_VALIDATE(createDepthBuffer(xVirtualFrames[i].depthImage), "something goes wrong when create Depth buffer");
		}
		return true;
	}

	bool XRender::createDepthBuffer(ImageParameters & depthImage)
	{
		VkImageCreateInfo depthCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		depthCreateInfo.pNext = nullptr;
		depthCreateInfo.flags = 0;
		depthCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		depthCreateInfo.format = xContextParams.depthStencilFormat;
		depthCreateInfo.extent = { xParams.xSwapchain.extent.width, xParams.xSwapchain.extent.height, 1 };
		depthCreateInfo.mipLevels = 1;
		depthCreateInfo.arrayLayers = 1;
		depthCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		depthCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		depthCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depthCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		depthCreateInfo.queueFamilyIndexCount = 0;
		depthCreateInfo.pQueueFamilyIndices = nullptr;
		depthCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		ZV_VK_CHECK(vkCreateImage(xParams.xDevice, &depthCreateInfo, nullptr, &depthImage.handle));

		ZV_VK_VALIDATE(allocateImageMemory(depthImage.handle, &depthImage.memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), "MEMORY ALLOCATION FOR DEPTH BUFFER FAILED !");

		ZV_VK_CHECK(vkBindImageMemory(xParams.xDevice, depthImage.handle, depthImage.memory, 0));

		// Create Image view
		VkImageViewCreateInfo depthImageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		depthImageViewCreateInfo.pNext = nullptr;
		depthImageViewCreateInfo.flags = 0;
		depthImageViewCreateInfo.image = depthImage.handle;
		depthImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthImageViewCreateInfo.format = xContextParams.depthStencilFormat;
		/*depthImageViewCreateInfo.components;*/
		depthImageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

		ZV_VK_CHECK(vkCreateImageView(xParams.xDevice, &depthImageViewCreateInfo, nullptr, &depthImage.view))

		return true;
	}

	bool XRender::createGlobalStagingBuffer()
	{
		// Setting size of gobal staging buffer
		xParams.globalStagingBuffer.size = 0x8000000; // 8M bytes
		xParams.globalStagingBuffer.isStaging = true;
		xParams.globalStagingBuffer.usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		return createBuffer(xParams.globalStagingBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	}

	bool XRender::createBuffer(BufferParameters &buff, VkMemoryPropertyFlags flags)
	{
		if ( buff.size == 0 )
		{
			std::cout << "Buffer Creating Failed: NoZero-Size buffer can be created !"
				<< std::endl;
			return false;
		}
		// Create Buffer object
		VkBufferCreateInfo vertStageBufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		vertStageBufCreateInfo.flags = 0;
		vertStageBufCreateInfo.pNext = nullptr;
		vertStageBufCreateInfo.queueFamilyIndexCount = 0;
		vertStageBufCreateInfo.pQueueFamilyIndices = nullptr;
		vertStageBufCreateInfo.usage = buff.usageFlags;
		vertStageBufCreateInfo.size = buff.size;
		vertStageBufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		ZV_VK_CHECK(vkCreateBuffer(xParams.xDevice, &vertStageBufCreateInfo, nullptr, &buff.handle));

		ZV_VK_VALIDATE(allocateBufferMemory(buff.handle, &buff.memory, flags), "CREATION FOR VERTEX STAGING BUFFER FAILED !");

		ZV_VK_CHECK(vkBindBufferMemory(xParams.xDevice, buff.handle, buff.memory, 0));

		return true;
	}

	bool XRender::getDataForImage2D(void *data, uint32_t dataSize, ImageParameters &imageParams, VkPipelineStageFlags consumingStages)
	{
		VkAccessFlags accessFlags = getImageAccessTypes(imageParams.usageFlags, consumingStages);
		if ( 0 == accessFlags )
		{
			std::cout << "Getting Data Failed: image usage cannot match may any consuming stage specified by the parameter: consumingStages"
				<< std::endl;
			return false;
		}
		if ( VK_ACCESS_FLAG_BITS_MAX_ENUM == accessFlags )
		{
			std::cout << "Getting Data Failed: Dst access flags getting failed"
				<< std::endl;
			return false;
		}
		if ( xParams.globalStagingBuffer.size < dataSize &&
			xContextParams.minImageTransferGranularityGraphics.width == 0 &&
			xContextParams.minImageTransferGranularityGraphics.height == 0 &&
			xContextParams.minImageTransferGranularityGraphics.depth == 0 )
		{
			std::cout << "Getting Data Failed: Image data'size exceed staging buffer's capacity "
				"and the graphics queue family do not support fragment transfer !"
				<< std::endl;
			return false;
		}
		
		uint32_t texelSize = dataSize / imageParams.extent.height / imageParams.extent.width;
		uint32_t rows = imageParams.extent.height;
		uint32_t rowPerBatch = xParams.globalStagingBuffer.size / (texelSize * imageParams.extent.width);

		if ( xParams.globalStagingBuffer.size < dataSize )
		{
			// Implying that granulartiy's each dimension(Aw,Ah,Ad) is not zero 
			// Rounding rowPerBatch down to the integer multiple of height granularity.
			rowPerBatch = rowPerBatch / xContextParams.minImageTransferGranularityGraphics.height * xContextParams.minImageTransferGranularityGraphics.height;
		}
		if ( 0 == rowPerBatch )
		{
			std::cout << "Getting Data Failed: Staging buffer's capacity is too small to stage an { image.width, granularity.height, 1 } iamge" 
			<< std::endl;
			return false;
		}
		// Image Layout transition
		ZV_VK_VALIDATE(imagePipelineBarrier(imageParams, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, VK_ACCESS_TRANSFER_WRITE_BIT,
			imageParams.layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		), "Image memory barrier failed");

		uint32_t batch = ( imageParams.extent.height + rowPerBatch - 1 ) / rowPerBatch;

		int32_t rowTransfered = 0;

		for ( int i = 0; i < batch; ++i )
		{
			uint32_t rowsToTransfer = std::min(rowPerBatch, rows - i * rowPerBatch);
			uint32_t batchDataSize = rowsToTransfer * texelSize * imageParams.extent.width;

			// Host Writes to the staging buffer
			void *stagingAddress;
			ZV_VK_CHECK(vkMapMemory(xParams.xDevice, xParams.globalStagingBuffer.memory, 0, xParams.globalStagingBuffer.size, 0, &stagingAddress));
			memcpy(stagingAddress, data, batchDataSize);
			VkMappedMemoryRange range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE	};
			range.memory = xParams.globalStagingBuffer.memory;
			range.size = batchDataSize;
			
			ZV_VK_CHECK(vkFlushMappedMemoryRanges(xParams.xDevice, 1, &range));
			vkUnmapMemory(xParams.xDevice, xParams.globalStagingBuffer.memory);

			// Make the host writes visible for the access performed by device's transfering
			ZV_VK_VALIDATE(wholeBufferPipelineBarrier(xParams.globalStagingBuffer,
				VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT
			), "buffer memory barrier barrier failed");

			// COPY COMMAND and SUBMIT
			VkCommandBufferBeginInfo cmdBI = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
			cmdBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			ZV_VK_CHECK(vkWaitForFences(xParams.xDevice, 1, &xParams.xCopyFence, VK_FALSE, 1000000000UL));
			ZV_VK_CHECK(vkResetFences(xParams.xDevice, 1, &xParams.xCopyFence));

			ZV_VK_CHECK(vkBeginCommandBuffer(xParams.xCopyCmd, &cmdBI));

			VkBufferImageCopy bufImageCopy = { };
			bufImageCopy.bufferRowLength = imageParams.extent.width;
			bufImageCopy.bufferImageHeight = rowsToTransfer;
			bufImageCopy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
			bufImageCopy.imageOffset = { 0, rowTransfered, 0};
			bufImageCopy.imageExtent = { imageParams.extent.width, rowsToTransfer, 1 };
			vkCmdCopyBufferToImage(xParams.xCopyCmd, xParams.globalStagingBuffer.handle, imageParams.handle, imageParams.layout, 1, &bufImageCopy );
			
			ZV_VK_CHECK(vkEndCommandBuffer(xParams.xCopyCmd));

			VkSubmitInfo SI = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
			SI.commandBufferCount = 1;
			SI.pCommandBuffers = &xParams.xCopyCmd;
			ZV_VK_CHECK(vkQueueSubmit(xParams.xGraphicQueue, 1, &SI, xParams.xCopyFence));

			// Make the transfer read for staging buffer visible to host.
			ZV_VK_VALIDATE(wholeBufferPipelineBarrier(xParams.globalStagingBuffer,
				VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_HOST_WRITE_BIT
			), "buffer memory image barrier failed");
			
			rowTransfered += rowsToTransfer;
			data = (char *)data + batchDataSize;
		}
		// Image Layout Transition
		ZV_VK_VALIDATE(imagePipelineBarrier(imageParams,
			VK_PIPELINE_STAGE_TRANSFER_BIT, consumingStages, 
			VK_ACCESS_TRANSFER_WRITE_BIT, accessFlags,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageParams.layout
		), "Image memory barrier failed");

		return true;
	}

	VkAccessFlags XRender::getBufferAccessTypes(VkBufferUsageFlags flags, VkPipelineStageFlags consumingStages)
	{
		if ( consumingStages & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT )
		{
			std::cout << "ALL COMMANDS STAGE FLAG CANNOT BE USED TO SPECIFY ACCESS FLAGS"
				"SINCE THE PROPERTY OF THE QUEUE FAMILY WHICH WILL PERFORMING ON THE IMAGE IS UNKNOWN, "
				"PLEASE INPUT SPECIFIC CONSUMING STAGE FLAGS WHICH THE QUEUE FAMILY SUPPORTS !"
				<< std::endl;
			return VK_ACCESS_FLAG_BITS_MAX_ENUM;
		}
		if ( consumingStages & VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT )
		{
			consumingStages |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
				| VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT
				| VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
				| VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
				| VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT
				| VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT
				| VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT
				| VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
				| VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
				| VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
				| VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}

		VkAccessFlags ret = 0;
		VkPipelineStageFlags consumingStages0;

#define CHECK_ACCESS(access) \
	consumingStages0 = pipelineStagesForAccess[access];\
	if(consumingStages & consumingStages0)\
	{\
		ret |= access;\
	}
		if ( flags & VK_BUFFER_USAGE_INDEX_BUFFER_BIT )
		{
			CHECK_ACCESS(VK_ACCESS_INDEX_READ_BIT)
		}
		if ( flags & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT )
		{
			CHECK_ACCESS(VK_ACCESS_INDIRECT_COMMAND_READ_BIT)
		}
		if ( flags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT || flags & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT )
		{
			CHECK_ACCESS(VK_ACCESS_SHADER_READ_BIT)
			CHECK_ACCESS(VK_ACCESS_SHADER_WRITE_BIT)
		}
		if ( flags & VK_BUFFER_USAGE_TRANSFER_SRC_BIT )
		{
			CHECK_ACCESS(VK_ACCESS_TRANSFER_READ_BIT)
		}
		if ( flags & VK_BUFFER_USAGE_TRANSFER_DST_BIT )
		{
			CHECK_ACCESS(VK_ACCESS_TRANSFER_WRITE_BIT)
		}
		if ( flags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT || flags & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT )
		{
			CHECK_ACCESS(VK_ACCESS_UNIFORM_READ_BIT)
		}
		if ( flags & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT )
		{
			CHECK_ACCESS(VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)
		}
#undef CHECK_ACCESS
		return ret;
	}

	bool XRender::wholeBufferPipelineBarrier(BufferParameters &bufferParams, VkPipelineStageFlags srcStageFlags, VkPipelineStageFlags dstStageFlags, VkAccessFlags srcAccess, VkAccessFlags dstAccess)
	{
		VkCommandBufferBeginInfo cmdbufferBI = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		cmdbufferBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		ZV_VK_CHECK(vkWaitForFences(xParams.xDevice, 1, &xParams.xCopyFence, VK_FALSE, 1000000UL));
		ZV_VK_CHECK(vkResetFences(xParams.xDevice, 1, &xParams.xCopyFence));
		ZV_VK_CHECK(vkBeginCommandBuffer(xParams.xCopyCmd, &cmdbufferBI));
		VkBufferMemoryBarrier bufMemBarrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
		bufMemBarrier.buffer = bufferParams.handle;
		bufMemBarrier.offset = 0;
		bufMemBarrier.size = bufferParams.size;
		bufMemBarrier.srcAccessMask = srcAccess;
		bufMemBarrier.dstAccessMask = dstAccess;
		bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		
		vkCmdPipelineBarrier(xParams.xCopyCmd, srcStageFlags, dstStageFlags, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &bufMemBarrier, 0, nullptr);
		ZV_VK_CHECK(vkEndCommandBuffer(xParams.xCopyCmd));

		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &xParams.xCopyCmd;
		ZV_VK_CHECK(vkQueueSubmit(xParams.xGraphicQueue, 1, &submitInfo, xParams.xCopyFence));

		return true;
	}

	VkAccessFlags XRender::getImageAccessTypes(VkImageUsageFlags flags, VkPipelineStageFlags consumingStages)
	{
		if ( consumingStages & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT )
		{
			std::cout << "ALL COMMANDS STAGE FLAG CANNOT BE USED TO SPECIFY ACCESS FLAGS"
				"SINCE THE PROPERTY OF THE QUEUE FAMILY WHICH WILL PERFORMING ON THE IMAGE IS UNKNOWN, "
				"PLEASE INPUT SPECIFIC CONSUMING STAGE FLAGS WHICH THE QUEUE FAMILY SUPPORTS"
				<< std::endl;
			return VK_ACCESS_FLAG_BITS_MAX_ENUM;
		}
		if ( consumingStages & VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT )
		{
			consumingStages |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
				| VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT
				| VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
				| VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
				| VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT
				| VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT
				| VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT
				| VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
				| VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
				| VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
				| VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}

		VkAccessFlags ret = 0;
		VkPipelineStageFlags consumingStages0;

#define CHECK_ACCESS(access) \
	consumingStages0 = pipelineStagesForAccess[access];\
	if(consumingStages & consumingStages0)\
	{\
		ret |= access;\
	}
		if ( flags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
		{
			CHECK_ACCESS(VK_ACCESS_COLOR_ATTACHMENT_READ_BIT)
			CHECK_ACCESS(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
		}
		if ( flags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT )
		{
			CHECK_ACCESS(VK_ACCESS_TRANSFER_READ_BIT)
		}
		if ( flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT )
		{
			CHECK_ACCESS(VK_ACCESS_TRANSFER_WRITE_BIT)
		}
		if ( flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT )
		{
			CHECK_ACCESS(VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT)
			CHECK_ACCESS(VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
		}
		if ( flags & VK_IMAGE_USAGE_SAMPLED_BIT )
		{
			CHECK_ACCESS(VK_ACCESS_SHADER_READ_BIT)
		}
		if ( flags & VK_IMAGE_USAGE_STORAGE_BIT )
		{
			CHECK_ACCESS(VK_ACCESS_SHADER_READ_BIT)
			CHECK_ACCESS(VK_ACCESS_SHADER_WRITE_BIT)
		}
		if ( flags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT )
		{
			CHECK_ACCESS(VK_ACCESS_INPUT_ATTACHMENT_READ_BIT)
		}
#undef CHECK_ACCESS
		return ret;
	}

	bool XRender::imagePipelineBarrier(ImageParameters &image, VkPipelineStageFlags srcStageFlags, VkPipelineStageFlags dstStageFlags, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBufferBeginInfo cmdbufferBI = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		cmdbufferBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		ZV_VK_CHECK(vkWaitForFences(xParams.xDevice, 1, &xParams.xCopyFence, VK_FALSE, 1000000UL));
		ZV_VK_CHECK(vkResetFences(xParams.xDevice, 1, &xParams.xCopyFence));

		ZV_VK_CHECK(vkBeginCommandBuffer(xParams.xCopyCmd, &cmdbufferBI));
		VkImageMemoryBarrier imgMemBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		imgMemBarrier.image = image.handle;
		imgMemBarrier.oldLayout = oldLayout;
		imgMemBarrier.newLayout = newLayout;
		imgMemBarrier.srcAccessMask = srcAccess;
		imgMemBarrier.dstAccessMask = dstAccess;
		imgMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imgMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkCmdPipelineBarrier(xParams.xCopyCmd, srcStageFlags, dstStageFlags, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &imgMemBarrier);
		ZV_VK_CHECK(vkEndCommandBuffer(xParams.xCopyCmd));

		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &xParams.xCopyCmd;
		ZV_VK_CHECK(vkQueueSubmit(xParams.xGraphicQueue, 1, &submitInfo, xParams.xCopyFence));

		return true;
	}

	bool XRender::getDataForBuffer(void *data, uint32_t dataSize, BufferParameters &bufferParams, VkPipelineStageFlags consumingStages)
	{
		// buffer usage and consumingStages match checking
		VkAccessFlags accessFlags = getBufferAccessTypes(bufferParams.usageFlags, consumingStages);
		if ( 0 == accessFlags )
		{
			std::cout << "Data Getting Failed: Usages of the Dst buffer cannot match any available consuming stage" 
				<< std::endl;
			return false;
		}
		if ( VK_ACCESS_FLAG_BITS_MAX_ENUM == accessFlags )
		{
			std::cout << "Data Getting Failed: dst Access flags getting failed !"
				<< std::endl;
			return false;
		}
		if ( dataSize <= bufferParams.size )
		{
			std::cout << "Data Getting Failed: Data size exceeds the Dst buffer's capacity" 
				<< std::endl;
			return false;
		}

		uint32_t batchOffset = 0;
		do
		{
			uint32_t batchTransferSize = std::min(dataSize, xParams.globalStagingBuffer.size);
			void *srcData = (char *)data + batchOffset;

			// Map memory for staging buffer and write data into it.
			void *stagingAddress;
			ZV_VK_CHECK(vkMapMemory(xParams.xDevice, xParams.globalStagingBuffer.memory, 0, xParams.globalStagingBuffer.size, 0, &stagingAddress));
			memcpy(stagingAddress, srcData, batchTransferSize);

			VkMappedMemoryRange range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
			range.memory = xParams.globalStagingBuffer.memory;
			range.size = batchTransferSize;

			ZV_VK_CHECK(vkFlushMappedMemoryRanges(xParams.xDevice, 1, &range));
			vkUnmapMemory(xParams.xDevice, xParams.globalStagingBuffer.memory);

			// Make host write visiable to device's transfering.
			ZV_VK_VALIDATE(wholeBufferPipelineBarrier(xParams.globalStagingBuffer,
				VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT
			), "buffer memory barrier failed");

			// COPY COMMAND
			VkCommandBufferBeginInfo copyCmdBI = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
			copyCmdBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			ZV_VK_CHECK(vkWaitForFences(xParams.xDevice, 1, &xParams.xCopyFence, VK_FALSE, 10000000UL));
			ZV_VK_CHECK(vkResetFences(xParams.xDevice, 1, &xParams.xCopyFence));

			ZV_VK_CHECK(vkBeginCommandBuffer(xParams.xCopyCmd, &copyCmdBI));

			VkBufferCopy bufferCopy = {};
			bufferCopy.size = batchTransferSize;
			bufferCopy.srcOffset = 0;
			bufferCopy.dstOffset = batchOffset;
			vkCmdCopyBuffer(xParams.xCopyCmd, xParams.globalStagingBuffer.handle, bufferParams.handle, 1, &bufferCopy);
			ZV_VK_CHECK(vkEndCommandBuffer(xParams.xCopyCmd));

			VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &xParams.xCopyCmd;
			ZV_VK_CHECK(vkQueueSubmit(xParams.xGraphicQueue, 1, &submitInfo, xParams.xCopyFence));

			// Make transfer read visiable to host.
			ZV_VK_VALIDATE(wholeBufferPipelineBarrier(xParams.globalStagingBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_HOST_WRITE_BIT
			), "buffer memory barrier failed");

			batchOffset += batchTransferSize;
			dataSize -= batchTransferSize;
		} while ( dataSize > 0 );

		// Make the transfer write to the dst buffer visible to the following access of it.
		ZV_VK_VALIDATE(wholeBufferPipelineBarrier(bufferParams,
			VK_PIPELINE_STAGE_TRANSFER_BIT, consumingStages,
			VK_ACCESS_TRANSFER_WRITE_BIT, accessFlags
		), "Memory Syn Failed After data transfer for buffer !");
		
		return true;
	}


    bool XRender::allocateBufferMemory(VkBuffer buffer, VkDeviceMemory * memory, VkMemoryPropertyFlags addtionProps)
    {
        VkMemoryRequirements memRequires;
        vkGetBufferMemoryRequirements(xParams.xDevice, buffer, &memRequires);

        VkPhysicalDeviceMemoryProperties phyiscalDeviceMemProp;
        vkGetPhysicalDeviceMemoryProperties(xParams.xPhysicalDevice, &phyiscalDeviceMemProp);

        // Look up all types of memory heap, and try to find one, whose type is required, from which the buffer will be allocated.
        for (size_t i = 0; i < phyiscalDeviceMemProp.memoryTypeCount; ++i)
        {

            if (memRequires.memoryTypeBits & (1 << i) // make sure the type is what the buffer requires. 
                && phyiscalDeviceMemProp.memoryTypes[i].propertyFlags & addtionProps) // make sure the memory with this type is visible to us.
            {
                VkMemoryAllocateInfo memAllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
                memAllocInfo.pNext = nullptr;
                memAllocInfo.allocationSize = memRequires.size;
                memAllocInfo.memoryTypeIndex = i;

                if (vkAllocateMemory(xParams.xDevice, &memAllocInfo, nullptr, memory) == VK_SUCCESS)
                    return true;
            }
        }
        return false;
    }

    bool XRender::allocateImageMemory(VkImage image, VkDeviceMemory *memory, VkMemoryPropertyFlags addtionProps)
    {
        VkMemoryRequirements memRequires;
        vkGetImageMemoryRequirements(xParams.xDevice, image, &memRequires);

        VkPhysicalDeviceMemoryProperties phyiscalDeviceMemProp;
        vkGetPhysicalDeviceMemoryProperties(xParams.xPhysicalDevice, &phyiscalDeviceMemProp);

        // Look up all types of memory heap, and try to find one, whose type is required, from which the buffer will be allocated.
        for (size_t i = 0; i < phyiscalDeviceMemProp.memoryTypeCount; ++i)
        {

            if (memRequires.memoryTypeBits & (1 << i) // make sure the type is what the buffer requires. 
                && phyiscalDeviceMemProp.memoryTypes[i].propertyFlags & addtionProps) // make sure the memory with this type is visible to us.
            {
                VkMemoryAllocateInfo memAllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
                memAllocInfo.pNext = nullptr;
                memAllocInfo.allocationSize = memRequires.size;
                memAllocInfo.memoryTypeIndex = i;

                if (vkAllocateMemory(xParams.xDevice, &memAllocInfo, nullptr, memory) == VK_SUCCESS)
                    return true;
            }
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
        swapchainCreateInfo.clipped = VK_TRUE;
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
        xParams.xSwapchain.extent = imagesExtent;

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
