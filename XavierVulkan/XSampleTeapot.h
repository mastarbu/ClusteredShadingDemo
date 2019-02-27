#pragma once
#include "Xavier.h"
#include "XModel.h"
#include "XTexture.h"
#include "assimp/scene.h"
#include <vector>
#include <string>

namespace Xavier {
    class XSampleTeapot : public XRender {
    public:
        XSampleTeapot() : XRender() {}
        ~XSampleTeapot() {}



        struct XMaterialTeapot {
            XTexture *texDiffuse;
            XMaterialProperties matProps;
            BufferParameters propUniformBuffer;
            VkDescriptorSet descriptorSet;
        };

        struct XTeapotParams {
            std::vector<XMaterialTeapot> materials;

            VkDescriptorSetLayout xTexDescSetLayout;
            VkPipelineLayout xTexPipelineLayout;
            VkPipeline xTexPipeline;

            VkDescriptorSetLayout xBoldDescSetLayout;
            VkPipelineLayout xBoldPipelineLayout;
            VkPipeline xBoldPipeline;

            ImageParameters depth;

            std::vector<XMesh> Meshes;
            std::vector<XVertex> vertices;
            std::vector<uint32_t> indices;
            
            VkDescriptorPool commonPool;
            VkDescriptorSetLayout commonSetLayout;
            VkDescriptorSet commonSet;
            BufferParameters commonUniform;


            std::map<std::string, XTexture> xTextureLib;
        } xParamsTeapot;

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
        // Load meshes and materials of the scene.
        bool loadAsserts();
        bool loadMesh(aiMesh * mesh, uint32_t &indexBase);
        bool loadTextureFromFile(const std::string &file, const std::string &dir, Xavier::XTexture *&xTex);

        bool prepareCameraAndLights();
        bool prepareVirutalFrames();
        bool prepareRenderPasses();
        bool prepareGraphicsPipeline();

        bool prepareDescriptorSetLayout();

        bool createDepthBuffer(ImageParameters &depthImage);

        bool prepareFrameBuffer(std::vector<VkImageView> & view, VkFramebuffer & framebuffer);
        bool buildCommandBuffer(VkCommandBuffer & xRenderCmdBuffer, VkFramebuffer &currentFrameBuffer);
    };
}