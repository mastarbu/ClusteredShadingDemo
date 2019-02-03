#pragma once
#include "Xavier.h"

#include <vector>


namespace Xavier {
    class XSampleA : public XRender {
    public:
        XSampleA() : XRender() {}
        ~XSampleA() {}

        virtual bool Draw();
        virtual bool prepareRenderSample();

    private:
        bool prepareVirutalFrames();
        bool prepareRenderPasses();
        bool prepareGraphicsPipeline();
        bool prepareVertexInput();

        bool allocateBufferMemory(VkBuffer buffer, VkDeviceMemory *memory, VkMemoryPropertyFlags addtionProps);
        bool prepareFrameBuffer();
     
    };

}  