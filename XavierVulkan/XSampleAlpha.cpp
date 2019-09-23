#include "XSampleAlpha.h"
namespace Xavier {

    bool Xavier::XSampleA::prepareRenderSample()
    {
        /// Create everything before we start the render loop.
        prepareVirutalFrames();

        prepareRenderPasses();

        prepareGraphicsPipeline();

        prepareVertexInput();

        return true;
    }

    bool XSampleA::Draw()
    {
        // Initialization for the CurrentFrameData.
        static CurrentFrameData currentFrameData = {
            0,
            xVirtualFrames.size(),
            &xVirtualFrames[0],
            &xParams.xSwapchain,
            0,
        };

        //Move the index Forward by 1.
        currentFrameData.frameIndex = (currentFrameData.frameIndex + 1) % currentFrameData.frameCount;
        currentFrameData.virtualFrameData = &xVirtualFrames.at(currentFrameData.frameIndex);

        VkCommandBufferBeginInfo cmdBufBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        cmdBufBeginInfo.pNext = nullptr;
        // Only useful when record a secondary command buffer.
        cmdBufBeginInfo.pInheritanceInfo = nullptr;
        // Wait for the point when command buffer exits pending state.
        vkWaitForFences(xParams.xDevice, 1, &currentFrameData.virtualFrameData->CPUGetChargeOfCmdBufFence, VK_TRUE, 200000ll);

        // Reset the fence
        vkResetFences(xParams.xDevice, 1, &currentFrameData.virtualFrameData->CPUGetChargeOfCmdBufFence);
        // Acquire Image.
        VkResult rst = vkAcquireNextImageKHR(xParams.xDevice, xParams.xSwapchain.handle, 200, currentFrameData.virtualFrameData->swapchainImageAvailableSemphore, VkFence(), &currentFrameData.swapChainImageIndex);

        std::cout << "rst:" << rst << std::endl;
        // Create FrameBuffer

        if (currentFrameData.virtualFrameData->frameBuffer != VK_NULL_HANDLE)
            vkDestroyFramebuffer(xParams.xDevice, currentFrameData.virtualFrameData->frameBuffer, nullptr);

        VkFramebufferCreateInfo framebufferCreateInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };

        framebufferCreateInfo.pNext = nullptr;
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.renderPass = xParams.xRenderPass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = &xParams.xSwapchain.images[currentFrameData.swapChainImageIndex].view;
        framebufferCreateInfo.width = xParams.xSwapchain.extent.width;
        framebufferCreateInfo.height = xParams.xSwapchain.extent.height;
        framebufferCreateInfo.layers = 1;
        ZV_VK_CHECK(vkCreateFramebuffer(xParams.xDevice, &framebufferCreateInfo, nullptr, &currentFrameData.virtualFrameData->frameBuffer));

        std::vector<float> colorClearValue = { 0.5f, 0.5f, 0.5f, 0.0f };
        VkClearValue clearColor;
        memcpy(&clearColor, colorClearValue.data(), sizeof(float) * colorClearValue.size());

        // Begin to record command buffer.
        vkBeginCommandBuffer(currentFrameData.virtualFrameData->commandBuffer, &cmdBufBeginInfo);
        VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        renderPassBeginInfo.renderPass = xParams.xRenderPass;
        renderPassBeginInfo.framebuffer = currentFrameData.virtualFrameData->frameBuffer;
        renderPassBeginInfo.renderArea = {
            VkOffset2D {0, 0},
            VkExtent2D {xParams.xSwapchain.extent.width, xParams.xSwapchain.extent.height},
        };
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &clearColor;

        // recording starts.
        vkCmdBeginRenderPass(currentFrameData.virtualFrameData->commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
        // 1. Bind my pipeline to the graphic binding point.
        vkCmdBindPipeline(currentFrameData.virtualFrameData->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, xParams.xPipeline);
        // 2. Set the dynamic state for the pipeline.
        // 3. Bind Vertex Input Buffer.
        std::vector<VkDeviceSize> offsets = { 0ll };
        vkCmdBindVertexBuffers(currentFrameData.virtualFrameData->commandBuffer, 0, 1, &xParams.xVertexBuffer.handle, offsets.data());
        // 4. Bind Descriptor Set.
        // 5. Push Constants.
        // 5. Draw Call.
        vkCmdDraw(currentFrameData.virtualFrameData->commandBuffer, 6, 1, 0, 0);
        vkCmdEndRenderPass(currentFrameData.virtualFrameData->commandBuffer);

        vkEndCommandBuffer(currentFrameData.virtualFrameData->commandBuffer);
        
        // Submit the command buffer.
        VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.pNext = nullptr;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &currentFrameData.virtualFrameData->swapchainImageAvailableSemphore;
        submitInfo.pWaitDstStageMask = &waitDstStageMask;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &currentFrameData.virtualFrameData->commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &currentFrameData.virtualFrameData->renderFinishedSemphore;
        ZV_VK_CHECK(vkQueueSubmit(xParams.xGraphicQueue, 1, &submitInfo, currentFrameData.virtualFrameData->CPUGetChargeOfCmdBufFence));  

        // Present the image.
        VkResult presentResult;
        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.pNext = nullptr;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &currentFrameData.virtualFrameData->renderFinishedSemphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &xParams.xSwapchain.handle;
        presentInfo.pImageIndices = &currentFrameData.swapChainImageIndex;
        presentInfo.pResults = &presentResult;

        ZV_VK_CHECK(vkQueuePresentKHR(xParams.xPresentQueue, &presentInfo));
        return true;
    }

    bool XSampleA::prepareVirutalFrames()
    {
        xVirtualFrames.resize(3);

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
        }
    }

    bool XSampleA::prepareRenderPasses()
    {
        // Attachment Descriptions
        VkAttachmentDescription attachmentDescriptions[1] = { };

        /// Color Attachment.
        attachmentDescriptions[0].flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
        attachmentDescriptions[0].format = xParams.xSwapchain.format;
        attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        /// The only color attachment reference
        VkAttachmentReference colorAttachmentRef[1] = {};
        colorAttachmentRef[0].attachment = 0;
        colorAttachmentRef[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        /// The only subpass
        VkSubpassDescription subpassDecription[1] = {};
        subpassDecription[0].flags = 0;
        subpassDecription[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDecription[0].inputAttachmentCount = 0;
        subpassDecription[0].pInputAttachments = nullptr;
        subpassDecription[0].colorAttachmentCount = 1;
        subpassDecription[0].pColorAttachments = &colorAttachmentRef[0];
        subpassDecription[0].pResolveAttachments = nullptr;
        subpassDecription[0].pDepthStencilAttachment = nullptr;
        subpassDecription[0].preserveAttachmentCount = 0;
        subpassDecription[0].pPreserveAttachments = nullptr;

        /// Dependencies
        VkSubpassDependency subpassDependency[2] = {};
        subpassDependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependency[0].dstSubpass = 0;
        subpassDependency[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subpassDependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependency[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        subpassDependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpassDependency[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        subpassDependency[1].srcSubpass = 0;
        subpassDependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependency[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subpassDependency[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpassDependency[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        subpassDependency[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        /// Create Render Pass
        VkRenderPassCreateInfo renderPassCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        renderPassCreateInfo.pNext = nullptr;
        renderPassCreateInfo.flags = 0;
        renderPassCreateInfo.attachmentCount = 1;
        renderPassCreateInfo.pAttachments = &attachmentDescriptions[0];
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpassDecription[0];
        renderPassCreateInfo.dependencyCount = 2;
        renderPassCreateInfo.pDependencies = &subpassDependency[0];
        ZV_VK_CHECK(vkCreateRenderPass(xParams.xDevice, &renderPassCreateInfo, nullptr, &xParams.xRenderPass));
    }

    bool XSampleA::prepareGraphicsPipeline()
    {
        /// Create Pipeline Layout.
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipelineLayoutCreateInfo.pNext = nullptr;
        pipelineLayoutCreateInfo.flags = 0;
        pipelineLayoutCreateInfo.setLayoutCount = 0;
        pipelineLayoutCreateInfo.pSetLayouts = nullptr;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

        ZV_VK_CHECK(vkCreatePipelineLayout(xParams.xDevice, &pipelineLayoutCreateInfo, nullptr, &xParams.xPipelineLayout));

        /// Pipeline Shader stages
        VkPipelineShaderStageCreateInfo shaderStages[2] = {};

        VkShaderModule shaderModule[2] = {};
        {
            VkShaderModuleCreateInfo shaderModuleCreateInfo[2] = {};

            /// Create shader module of vertex shader.
            std::vector<char> vertexShaderData(m_getFileBinaryContent("Data/" SAMPLE_NUMBER "/main.vert.spv"));
            shaderModuleCreateInfo[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shaderModuleCreateInfo[0].pNext = nullptr;
            shaderModuleCreateInfo[0].flags = 0;
            shaderModuleCreateInfo[0].pCode = (uint32_t *)vertexShaderData.data();
            shaderModuleCreateInfo[0].codeSize = vertexShaderData.size();
            ZV_VK_CHECK(vkCreateShaderModule(xParams.xDevice, &shaderModuleCreateInfo[0], nullptr, &shaderModule[0]));

            /// Create shader module of fragment shader.
            std::vector<char> fragmentShaderData(m_getFileBinaryContent("Data/" SAMPLE_NUMBER "/main.frag.spv"));
            shaderModuleCreateInfo[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shaderModuleCreateInfo[1].pNext = nullptr;
            shaderModuleCreateInfo[1].flags = 0;
            shaderModuleCreateInfo[1].pCode = (uint32_t *)fragmentShaderData.data();
            shaderModuleCreateInfo[1].codeSize = fragmentShaderData.size();
            ZV_VK_CHECK(vkCreateShaderModule(xParams.xDevice, &shaderModuleCreateInfo[1], nullptr, &shaderModule[1]));

            shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[0].pNext = nullptr;
            shaderStages[0].flags = 0;
            shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            shaderStages[0].module = shaderModule[0];
            shaderStages[0].pName = "main";
            shaderStages[0].pSpecializationInfo = nullptr;

            shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[1].pNext = nullptr;
            shaderStages[1].flags = 0;
            shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            shaderStages[1].module = shaderModule[1];
            shaderStages[1].pName = "main";
            shaderStages[1].pSpecializationInfo = nullptr;
        }
        /// Input Attribute Binding.
        VkVertexInputBindingDescription vertexInputBind[1] = {};
        vertexInputBind[0].binding = 0;
        vertexInputBind[0].stride = sizeof(XVertexData);
        vertexInputBind[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription vertexInputAttr[2] = {};
        vertexInputAttr[0].binding = 0;
        vertexInputAttr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexInputAttr[0].location = 0;
        vertexInputAttr[0].offset = offsetof(XVertexData, pos);

        vertexInputAttr[1].binding = 0;
        vertexInputAttr[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        vertexInputAttr[1].location = 1;
        vertexInputAttr[1].offset = offsetof(XVertexData, color);

        VkPipelineVertexInputStateCreateInfo vertexInput = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertexInput.pNext = nullptr;
        vertexInput.flags = 0;
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &vertexInputBind[0];
        vertexInput.vertexAttributeDescriptionCount = 2;
        vertexInput.pVertexAttributeDescriptions = &vertexInputAttr[0];

        /// Vertex Input Assembly state.
        VkPipelineInputAssemblyStateCreateInfo vertexInputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        vertexInputAssembly.pNext = nullptr;
        vertexInputAssembly.flags = 0;
        vertexInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        /* This is a flag that is used to allow strip and fan primitive topologies
           to be cut and restarted. Without this, each strip or fan would need to be a separate draw. When you
           use restarts, many strips or fans can be combined into a single draw. Restarts take effect only when
           indexed draws are used because the point at which to restart the strip is marked using a special,
           reserved value in the index buffer.
        */
        vertexInputAssembly.primitiveRestartEnable = VK_FALSE;

        /// Tessellation State
        // [ Reserved ]

        /// Viewport state.
        VkViewport viewport = {};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = (float)xParams.xSwapchain.extent.width;
        viewport.height = (float)xParams.xSwapchain.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.extent = xParams.xSwapchain.extent;
        scissor.offset = VkOffset2D{ 0, 0 };

        /// Viewport state
        VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewportState.pNext = nullptr;
        viewportState.flags = 0;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        /// Rasterization state.
        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterizationStateCreateInfo.pNext = nullptr;
        rasterizationStateCreateInfo.flags = 0;
        /*
            The depthClampEnable field is used to turn depth clamping on or off. Depth clamping causes
            fragments that would have been clipped away by the near or far planes to instead be projected onto
            those planes and can be used to fill holes in geometry that would be caused by clipping.
        */
        rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
        /*
            Rasterizer DiscardEnable is used to turn off rasterization altogether. When this flag is set,
            the rasterizer will not run, and no fragments will be produced.
        */
        rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        // The depth bias feature.
        rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
        rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
        rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
        rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

        /*
            lineWidth sets the width of line primitives, in pixels. This applies to all lines rasterized
            with the pipeline. This includes pipelines in which the primitive topology is one of the line
            primitives, the geometry or tessellation shaders turn the input primitives into lines, and the polygon
            mode (set by polygonMode) is VK_POLYGON_MODE_LINE.
        */
        rasterizationStateCreateInfo.lineWidth = 1.0f;

        /// Multi-sample state.
        VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisampleState.pNext = nullptr;
        multisampleState.flags = 0;
        multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Parameter specifying that shading should occur per sample (when enabled) instead of per fragment (when disabled).
        multisampleState.sampleShadingEnable = VK_FALSE;
        multisampleState.minSampleShading = 1.0;
        multisampleState.pSampleMask = nullptr;
        multisampleState.alphaToOneEnable = VK_FALSE;
        multisampleState.alphaToCoverageEnable = VK_FALSE;

        //VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        //depthStencilState.pNext = nullptr;
        //depthStencilState.flags = 0;
        //depthStencilState.depthTestEnable = VK_TRUE;
        //// depthWriteEnable decides whether the depth value needs to be replaced with the new value when it pass the depth test.
        //depthStencilState.depthWriteEnable = VK_TRUE;
        //depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

        /*
            The depth-bounds test is a special, additional test that can be performed as part of depth testing. The
            value stored in the depth buffer for the current fragment is compared with a specified range of values
            that is part of the pipeline. If the value in the depth buffer falls within the specified range, then the
            test passes; otherwise, it fails.
        */
        /*
        depthStencilState.depthBoundsTestEnable = VK_FALSE;
        depthStencilState.minDepthBounds = 0.0f;
        depthStencilState.maxDepthBounds = 0.0f;
        depthStencilState.stencilTestEnable = VK_FALSE;
        depthStencilState.front = PVkStencilOpState();
        depthStencilState.back = VkStencilOpState();
        */

        /// Color Blend Attachment State.
        VkPipelineColorBlendAttachmentState colorBlendAttachment[1] = {};
        /*
            If the colorBlendEnable member of each VkPipelineColorBlendAttachmentState
            structure is VK_TRUE, then blending is applied to the corresponding color attachment.
        */
        colorBlendAttachment[0].blendEnable = VK_FALSE;
        colorBlendAttachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment[0].colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment[0].alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        /// Color Blend State.
        VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        colorBlendState.pNext = nullptr;
        colorBlendState.flags = 0;
        colorBlendState.logicOpEnable = VK_FALSE;
        colorBlendState.logicOp = VK_LOGIC_OP_COPY;
        colorBlendState.attachmentCount = 1;
        colorBlendState.pAttachments = &colorBlendAttachment[0];
        /*colorBlendState.blendConstants[4];*/

        /// Create the Graphic Pipeline.
        VkGraphicsPipelineCreateInfo graphicPipelineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        graphicPipelineCreateInfo.flags = 0;
        // shader info
        graphicPipelineCreateInfo.stageCount = 2;
        graphicPipelineCreateInfo.pStages = &shaderStages[0];
        // vertex input info
        graphicPipelineCreateInfo.pVertexInputState = &vertexInput;
        graphicPipelineCreateInfo.pInputAssemblyState = &vertexInputAssembly;
        graphicPipelineCreateInfo.pTessellationState = nullptr;
        graphicPipelineCreateInfo.pViewportState = &viewportState;
        graphicPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
        graphicPipelineCreateInfo.pMultisampleState = &multisampleState;
        graphicPipelineCreateInfo.pDepthStencilState = nullptr;
        graphicPipelineCreateInfo.pColorBlendState = &colorBlendState;
        graphicPipelineCreateInfo.pDynamicState = nullptr;
        graphicPipelineCreateInfo.layout = xParams.xPipelineLayout;
        graphicPipelineCreateInfo.renderPass = xParams.xRenderPass;
        graphicPipelineCreateInfo.subpass = 0;
        graphicPipelineCreateInfo.basePipelineHandle;
        graphicPipelineCreateInfo.basePipelineIndex;

        ZV_VK_CHECK(vkCreateGraphicsPipelines(xParams.xDevice, VK_NULL_HANDLE, 1, &graphicPipelineCreateInfo, nullptr, &xParams.xPipeline));
    }

    bool XSampleA::prepareVertexInput()
    {
        XVertexData xTriangleData[6] = {
        { { 1.0f, 1.0f, 1.0f, }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 1.0f, -1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
        { { -1.0f, -1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },

        { { 1.0f, 1.0f, 1.0f, }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { -1.0f, -1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 0.0f, 1.0f } },
        };

        // Create Buffer Object.
        xParams.xVertexBuffer.size = sizeof(xTriangleData);
        VkBufferCreateInfo bufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferCreateInfo.pNext = nullptr;
        bufferCreateInfo.flags = 0;
        bufferCreateInfo.size = sizeof(xTriangleData);
        bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferCreateInfo.queueFamilyIndexCount = 0;
        bufferCreateInfo.pQueueFamilyIndices = nullptr;

        ZV_VK_CHECK(vkCreateBuffer(xParams.xDevice, &bufferCreateInfo, nullptr, &xParams.xVertexBuffer.handle));

        // Allocate Memory for the Buffer.
        ZV_VK_VALIDATE(allocateBufferMemory(xParams.xVertexBuffer.handle, &xParams.xVertexBuffer.memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), "Cannnot Allocate Memory for the Buffer");

        // Bind the Memory to the Buffer.
        ZV_VK_CHECK(vkBindBufferMemory(xParams.xDevice, xParams.xVertexBuffer.handle, xParams.xVertexBuffer.memory, 0));

        // Copy Data to the Memory.
        void *mappedAddress;
        ZV_VK_CHECK(vkMapMemory(xParams.xDevice, xParams.xVertexBuffer.memory, 0, xParams.xVertexBuffer.size, 0, &mappedAddress));

        memcpy(mappedAddress, static_cast<void *>(&xTriangleData), sizeof(xTriangleData));
        
        VkMappedMemoryRange mappedMemRange = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
        mappedMemRange.pNext = nullptr;
        mappedMemRange.memory = xParams.xVertexBuffer.memory;
        mappedMemRange.offset = 0;
        mappedMemRange.size = VK_WHOLE_SIZE;
        ZV_VK_CHECK(vkFlushMappedMemoryRanges(xParams.xDevice, 1, &mappedMemRange));

        vkUnmapMemory(xParams.xDevice, xParams.xVertexBuffer.memory);

        return true;
    }

    bool XSampleA::prepareFrameBuffer()
    {
        return true;
    }
}