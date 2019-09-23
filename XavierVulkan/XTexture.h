#pragma once
#include <vector>
#include <map>
#include <memory>
#include "glm/glm.hpp"

namespace Xavier {
    struct XTexture
    {
		XTexture()
			: format(VK_FORMAT_UNDEFINED)
			, mipmapLevels(1)
			, handle(VK_NULL_HANDLE)
			, memory(VK_NULL_HANDLE)
			, view(VK_NULL_HANDLE)
			, sampler(VK_NULL_HANDLE)
		{ }

        VkFormat format;
        uint32_t mipmapLevels;
        VkImage handle;
        VkDeviceMemory memory;
        VkImageView view;
        VkSampler sampler;
    };

    struct XMaterialProperties {

		XMaterialProperties()
			: kAmbient()
			, kDiffuse()
			, kSpecular()
		    , kEmissive()
		{ }

        glm::vec4 kAmbient;
        glm::vec4 kDiffuse;
        glm::vec4 kSpecular;
        glm::vec4 kEmissive;
    };
}
