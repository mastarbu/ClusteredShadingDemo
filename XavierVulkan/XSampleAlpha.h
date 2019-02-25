#pragma once
#include "Xavier.h"

#include <vector>


namespace Xavier {
    class XSampleA : public XRender {
    public:
        XSampleA() : XRender() {}
        ~XSampleA() {}

        struct CurrentFrameData {
            size_t frameIndex;
            size_t frameCount;
            XVirtualFrameData *virtualFrameData;
            XSwapchain *swapChain;
            uint32_t swapChainImageIndex;
        };

        virtual bool Draw();
        virtual bool prepareRenderSample();

    private:
        bool prepareVirutalFrames();
        bool prepareRenderPasses();
        bool prepareGraphicsPipeline();
        bool prepareVertexInput();

        bool prepareFrameBuffer();
     
    };
}  