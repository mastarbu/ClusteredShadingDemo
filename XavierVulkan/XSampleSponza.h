#pragma once
#include "Xavier.h"
#include "XModel.h"
#include "XTexture.h"
#include "XShader.h"
#include "assimp/scene.h"
#include <vector>
#include <string>

namespace Xavier {
    class XSampleSponza : public XRender {
    public:
        XSampleSponza() : XRender() {
			xParams.xPhysicalDeviceFeatures.shaderStorageImageMultisample = VK_TRUE;
		}
        ~XSampleSponza() {}

        struct XMaterialSponza {
            XTexture *texDiffuse;
            XMaterialProperties matProps;
            BufferParameters propUniformBuffer;
            VkDescriptorSet descriptorSet;
        };

        struct PipelineParams {
            VkPipelineLayout pipeLayout;
            VkPipeline pipeline;
        };

        struct Pipelines {
            PipelineParams preZGraphicsPipeline;
            PipelineParams clusterForwardShadingPipe;
            PipelineParams uniqueClustersFindingCompPipe;
            PipelineParams lightAssignmentCompPipe;
        };

        struct DescriptorSetLayouts {
            VkDescriptorSetLayout globalUniform;
            VkDescriptorSetLayout findUniqueCluster;
        };

		struct DescriptorSets
		{
			VkDescriptorSet globalUniform;
			VkDescriptorSet findUniqueCluster;
		};

		struct PipelineShaders
		{
			XShader prezVS;
			XShader prezFS;
			XShader findUniqueClusterCP;
		};
		
		struct DescriptorResources
		{
			VkImageView preZ; // an image view for depth image
			BufferParameters globalUBO;
			ImageParameters clusterIndexsForEachSampel;
			BufferParameters UniqueClusters;
		};

        struct XSponzaParams {
            std::vector<XMaterialSponza> materials;

            Pipelines pipelines;
			DescriptorSetLayouts descSetsLayouts;
			DescriptorSets descSets;
			DescriptorResources descSetRes;

			VkRenderPass trivialRenderPass;
			ImageParameters depth;

			PipelineShaders shaders;
            std::vector<XMesh> Meshes;
            std::vector<XVertex> vertices;
            std::vector<uint32_t> indices;

            VkDescriptorPool commonPool;
            VkDescriptorSetLayout commonSetLayout;
            VkDescriptorSet commonSet;
            BufferParameters commonUniform;

            VkQueue xComputeQueue;

            VkPipeline uniqueClustersFindingPipeline;
            VkPipelineLayout uniqueClustersFindingLayout;
            VkPipeline lightCullingPipeline;
            VkPipelineLayout lightCullingLayout;

            std::map<std::string, XTexture> xTextureLib;
        } xSponzaParams;

        struct CurrentFrameData {
            size_t frameIndex;
            size_t frameCount;
            XVirtualFrameData *virtualFrameData;
            XSwapchain *swapChain;
            uint32_t swapChainImageIndex;
        };

        virtual bool Draw();
        virtual bool prepareRenderSample();

        /* Overload*/
        bool prepareVulkan();

    private:
        // Load meshes and materials of the scene.
        bool loadAssets(const std::string & path);
        bool loadMesh(aiMesh * mesh, uint32_t &indexBase);
        bool loadTextureFromFile(const std::string &file, const std::string &dir, Xavier::XTexture *&xTex);

        bool prepareCameraAndLights();
        bool prepareRenderPasses();
        bool prepareGraphicsPipeline();
        bool prepareComputePipeline();
        
        bool prepareDescriptorSetLayout();
		bool preparePipelineLayout();
		bool prepareShaders();
        bool prepareFrameBuffer(std::vector<VkImageView> & view, VkFramebuffer & framebuffer);
        bool buildPreZCommandBuffer(VkCommandBuffer& cmdBuffer, VkFramebuffer& currentFrameBuffer);
		
		bool prepareResourcesForDescriptorSets();

    };
}