#pragma once
#include <vector>
#include <map>
#include <memory>
#include "glm/glm.hpp"

namespace Xavier {
    struct XTexture
    {
        VkFormat format;
        VkImage handle;
        VkDeviceMemory memory;
        VkImageView view;
        VkSampler sampler;
    };

    struct XMaterialProperties {
        glm::vec4 kAmbient;
        glm::vec4 kDiffuse;
        glm::vec4 kSpecular;
        glm::vec4 kEmissive;
    };
}
