#pragma once
#include "vulkan/vulkan.h"
#include <vector>
#include "glm/glm.hpp"

namespace Xavier {

    
    struct XVertex {
        glm::vec3 pos;
        glm::vec3 norm;
        glm::vec2 uv;
    };
    
    class XMesh {
    public:
        //XMesh() {}
        //~XMesh() {}

        uint32_t materialIndex;

        uint32_t indexBase;

        uint32_t indexCount;
    };
}