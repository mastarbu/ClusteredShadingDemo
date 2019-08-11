#include "XSampleTeapot.h"
#include "assimp\Importer.hpp"
#include "assimp\postProcess.h"
#include "assimp\scene.h"
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

bool Xavier::XSampleTeapot::Draw()
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

    // Acquire Image.
    VkResult rst = vkAcquireNextImageKHR(xParams.xDevice, xParams.xSwapchain.handle, 200, currentFrameData.virtualFrameData->swapchainImageAvailableSemphore, VkFence(), &currentFrameData.swapChainImageIndex);

    vkWaitForFences(xParams.xDevice, 1, &currentFrameData.virtualFrameData->CPUGetChargeOfCmdBufFence, VK_TRUE, 1000000000ll);

    // Reset the fence
    vkResetFences(xParams.xDevice, 1, &currentFrameData.virtualFrameData->CPUGetChargeOfCmdBufFence);

    // Create FrameBuffer
    if (currentFrameData.virtualFrameData->frameBuffer != VK_NULL_HANDLE)
        vkDestroyFramebuffer(xParams.xDevice, currentFrameData.virtualFrameData->frameBuffer, nullptr);


    std::vector<VkImageView> attachments = { xParams.xSwapchain.images[currentFrameData.swapChainImageIndex].view,
        currentFrameData.virtualFrameData->depthImage.view };

    prepareFrameBuffer(attachments, currentFrameData.virtualFrameData->frameBuffer);

    // Begin to record command buffer.  
    ZV_VK_VALIDATE(buildCommandBuffer(currentFrameData.virtualFrameData->commandBuffer, currentFrameData.virtualFrameData->frameBuffer), "something goes wrong when building command buufer !");
   

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
    vkQueueSubmit(xParams.xGraphicQueue, 1, &submitInfo, currentFrameData.virtualFrameData->CPUGetChargeOfCmdBufFence);

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

bool Xavier::XSampleTeapot::prepareRenderSample()
{
    ZV_VK_VALIDATE(prepareDescriptorSetLayout(), "Prepare descriptorset layout failed !");
    ZV_VK_VALIDATE(prepareRenderPasses(), "something goes wrong when preparing renderpass");

    ZV_VK_VALIDATE(loadAsserts(), "something goes wrong when loading assets !");
    // ZV_VK_VALIDATE(prepareVertexInput(), "something goes wrong when preparing vertex input");
    ZV_VK_VALIDATE(prepareCameraAndLights(), "something goes wrong when preparing camera and lights");
    ZV_VK_VALIDATE(prepareVirutalFrames(), "something goes wrong when preparing virtual frames");

    ZV_VK_VALIDATE(prepareGraphicsPipeline(), "something goes wrong when preparing graphic pipelines! ");
    return true;
}

bool Xavier::XSampleTeapot::loadAsserts()
{
    Assimp::Importer sceneImporter;
    unsigned int flag = aiProcess_PreTransformVertices | aiProcess_Triangulate | aiProcess_GenNormals;

    // Import the obj file.
    sceneImporter.ReadFile(m_getAssetPath() + "/teapot/teapot.obj", flag);

    // Get the Scene.
    const aiScene *scene = sceneImporter.GetScene();
    if (scene)
    {
        aiNode *rootNode = scene->mRootNode;
        uint32_t indexBase = 0;
        for (unsigned int i = 0; i < rootNode->mNumMeshes; ++i)
        {
            // Since we import the file with "aiProcess_PreTransformVertices" flag, all meshes will correspond to only one childNode.
            unsigned int meshIndex = rootNode->mMeshes[i];
            aiMesh *mesh = scene->mMeshes[meshIndex];
            bool rest = loadMesh(mesh, indexBase);
        }

        // Create VertexBuffer
        xParams.xVertexBuffer.size = xParamsTeapot.vertices.size() * sizeof(XVertex);
        VkBufferCreateInfo vertBufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        vertBufCreateInfo.flags = 0;
        vertBufCreateInfo.pNext = nullptr;
        vertBufCreateInfo.queueFamilyIndexCount = 0;
        vertBufCreateInfo.pQueueFamilyIndices = nullptr;
        vertBufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        vertBufCreateInfo.size = xParams.xVertexBuffer.size;
        vertBufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ZV_VK_CHECK(vkCreateBuffer(xParams.xDevice, &vertBufCreateInfo, nullptr, &xParams.xVertexBuffer.handle));

        ZV_VK_VALIDATE(allocateBufferMemory(xParams.xVertexBuffer.handle, &xParams.xVertexBuffer.memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), "CREATION FOR VERTEX BUFFER FAILED !");

        ZV_VK_CHECK(vkBindBufferMemory(xParams.xDevice, xParams.xVertexBuffer.handle, xParams.xVertexBuffer.memory, 0));

        // Create staging buffer for vertex buffer.
        BufferParameters vertexStageBuffer;
        vertexStageBuffer.size = xParams.xVertexBuffer.size;
        VkBufferCreateInfo vertStageBufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };

        vertStageBufCreateInfo.flags = 0;
        vertStageBufCreateInfo.pNext = nullptr;
        vertStageBufCreateInfo.queueFamilyIndexCount = 0;
        vertStageBufCreateInfo.pQueueFamilyIndices = nullptr;
        vertStageBufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        vertStageBufCreateInfo.size = vertexStageBuffer.size;
        vertStageBufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ZV_VK_CHECK(vkCreateBuffer(xParams.xDevice, &vertStageBufCreateInfo, nullptr, &vertexStageBuffer.handle));

        ZV_VK_VALIDATE(allocateBufferMemory(vertexStageBuffer.handle, &vertexStageBuffer.memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), "CREATION FOR VERTEX STAGING BUFFER FAILED !");

        ZV_VK_CHECK(vkBindBufferMemory(xParams.xDevice, vertexStageBuffer.handle, vertexStageBuffer.memory, 0));

        // Load data for vertex input.
        void *mapAddress;
        ZV_VK_CHECK(vkMapMemory(xParams.xDevice, vertexStageBuffer.memory, 0, vertexStageBuffer.size, 0, &mapAddress));

        memcpy(mapAddress, xParamsTeapot.vertices.data(), vertexStageBuffer.size);

        VkMappedMemoryRange memRange = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
        memRange.memory = vertexStageBuffer.memory;
        memRange.pNext = nullptr;
        memRange.offset = 0;
        memRange.size = vertexStageBuffer.size;
        ZV_VK_CHECK(vkFlushMappedMemoryRanges(xParams.xDevice, 1, &memRange));

        // Copy from staging buffer to vertex buffer.
        ZV_VK_CHECK(vkWaitForFences(xParams.xDevice, 1, &xParams.xCopyFence, VK_TRUE, 1000000000ull));
        ZV_VK_CHECK(vkResetFences(xParams.xDevice, 1, &xParams.xCopyFence));
        VkCommandBufferBeginInfo cmdBufInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        cmdBufInfo.pNext = nullptr;
        cmdBufInfo.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(xParams.xCopyCmd, &cmdBufInfo);
        VkBufferCopy cpyRegion = { };
        cpyRegion.srcOffset = 0;
        cpyRegion.dstOffset = 0;
        cpyRegion.size = vertexStageBuffer.size;

        vkCmdCopyBuffer(xParams.xCopyCmd, vertexStageBuffer.handle, xParams.xVertexBuffer.handle, 1, &cpyRegion);
        
        VkBufferMemoryBarrier vertexBufMemBarrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        vertexBufMemBarrier.buffer = xParams.xVertexBuffer.handle;
        vertexBufMemBarrier.pNext = nullptr;
        vertexBufMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vertexBufMemBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        vertexBufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vertexBufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vertexBufMemBarrier.offset = 0;
        vertexBufMemBarrier.size = xParams.xVertexBuffer.size;
        
        vkCmdPipelineBarrier(xParams.xCopyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT,
            0, nullptr, 1, &vertexBufMemBarrier, 0, nullptr);
        vkEndCommandBuffer(xParams.xCopyCmd);

        VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit.pNext = nullptr;
        submit.waitSemaphoreCount = 0;
        submit.pWaitSemaphores = nullptr;
        submit.pWaitDstStageMask = nullptr;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &xParams.xCopyCmd;
        submit.signalSemaphoreCount = 0;
        submit.pSignalSemaphores = nullptr;
        vkQueueSubmit(xParams.xGraphicQueue, 1, &submit, xParams.xCopyFence);

        // Create Index buffer.
        xParams.xIndexBuffer.size = xParamsTeapot.indices.size() * sizeof(uint32_t);
        VkBufferCreateInfo indexBufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };;
        indexBufCreateInfo.flags = 0;
        indexBufCreateInfo.pNext = nullptr;
        indexBufCreateInfo.queueFamilyIndexCount = 0;
        indexBufCreateInfo.pQueueFamilyIndices = nullptr;
        indexBufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        indexBufCreateInfo.size = xParams.xIndexBuffer.size;
        indexBufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ZV_VK_CHECK(vkCreateBuffer(xParams.xDevice, &indexBufCreateInfo, nullptr, &xParams.xIndexBuffer.handle));

        ZV_VK_VALIDATE(allocateBufferMemory(xParams.xIndexBuffer.handle, &xParams.xIndexBuffer.memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), "CREATION FOR INDEX BUFFER FAILED !") ;

        ZV_VK_CHECK(vkBindBufferMemory(xParams.xDevice, xParams.xIndexBuffer.handle, xParams.xIndexBuffer.memory, 0));

        // Create Staging buffer for index buffer.
        BufferParameters indexStageBuffer;
        indexStageBuffer.size = xParams.xIndexBuffer.size;
        VkBufferCreateInfo indexStageBufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };;
        indexStageBufCreateInfo.flags = 0;
        indexStageBufCreateInfo.pNext = nullptr;
        indexStageBufCreateInfo.queueFamilyIndexCount = 0;
        indexStageBufCreateInfo.pQueueFamilyIndices = nullptr;
        indexStageBufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        indexStageBufCreateInfo.size = indexStageBuffer.size;
        indexStageBufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ZV_VK_CHECK(vkCreateBuffer(xParams.xDevice, &indexStageBufCreateInfo, nullptr, &indexStageBuffer.handle));

        ZV_VK_VALIDATE(allocateBufferMemory(indexStageBuffer.handle, &indexStageBuffer.memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), "CREATION FOR INDEX STAGING BUFFER FAILED !");

        ZV_VK_CHECK(vkBindBufferMemory(xParams.xDevice, indexStageBuffer.handle, indexStageBuffer.memory, 0));

        // Load data for index input.
        ZV_VK_CHECK(vkMapMemory(xParams.xDevice, indexStageBuffer.memory, 0, indexStageBuffer.size, 0, &mapAddress));

        memcpy(mapAddress, xParamsTeapot.indices.data(), indexStageBuffer.size);

        memRange = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
        memRange.memory = indexStageBuffer.memory;
        memRange.pNext = nullptr;
        memRange.offset = 0;
        memRange.size = indexStageBuffer.size;
        ZV_VK_CHECK(vkFlushMappedMemoryRanges(xParams.xDevice, 1, &memRange));

        // Copy from staging buffer to vertex buffer.
        ZV_VK_CHECK(vkWaitForFences(xParams.xDevice, 1, &xParams.xCopyFence, VK_TRUE, 1000000000ull));
        ZV_VK_CHECK(vkResetFences(xParams.xDevice, 1, &xParams.xCopyFence));
        cmdBufInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        cmdBufInfo.pNext = nullptr;
        cmdBufInfo.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(xParams.xCopyCmd, &cmdBufInfo);
        cpyRegion = { };
        cpyRegion.srcOffset = 0;
        cpyRegion.dstOffset = 0;
        cpyRegion.size = indexStageBuffer.size;

        vkCmdCopyBuffer(xParams.xCopyCmd, indexStageBuffer.handle, xParams.xIndexBuffer.handle, 1, &cpyRegion);

        VkBufferMemoryBarrier indexBufMemBarrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        indexBufMemBarrier.buffer = xParams.xIndexBuffer.handle;
        indexBufMemBarrier.pNext = nullptr;
        indexBufMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        indexBufMemBarrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
        indexBufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        indexBufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        indexBufMemBarrier.offset = 0;
        indexBufMemBarrier.size = xParams.xIndexBuffer.size;

        vkCmdPipelineBarrier(xParams.xCopyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT,
            0, nullptr, 1, &indexBufMemBarrier, 0, nullptr);
        vkEndCommandBuffer(xParams.xCopyCmd);

        submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit.pNext = nullptr;
        submit.waitSemaphoreCount = 0;
        submit.pWaitSemaphores = nullptr;
        submit.pWaitDstStageMask = nullptr;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &xParams.xCopyCmd;
        submit.signalSemaphoreCount = 0;
        submit.pSignalSemaphores = nullptr;
        vkQueueSubmit(xParams.xGraphicQueue, 1, &submit, xParams.xCopyFence);
        // Destroy two staging buffers.
        vkQueueWaitIdle(xParams.xGraphicQueue);
        std::cout << "f";
        vkFreeMemory(xParams.xDevice, vertexStageBuffer.memory, nullptr);
        std::cout << "u";
        vkDestroyBuffer(xParams.xDevice, vertexStageBuffer.handle, nullptr);
        std::cout << "c";
        vkFreeMemory(xParams.xDevice, indexStageBuffer.memory, nullptr);
        std::cout << "k";
        vkDestroyBuffer(xParams.xDevice, indexStageBuffer.handle, nullptr);
        std::cout << "!";



        // Load Texture Materials
        for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
        {
            aiMaterial *material = scene->mMaterials[i];
            XMaterialTeapot xMat = {};
            std::cout << material->GetName().C_Str() << std::endl;

            // Load Diffuse Texture.
            if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
            {
                aiString texFileName;
                if (aiReturn_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texFileName))
                {
                    XTexture *xTex;
                    if (loadTextureFromFile(texFileName.C_Str(), m_getTexturePath(), xTex))
                    {
                        xMat.texDiffuse = xTex;
                    }

                    else {
                        // Load dummy texture
                        // However we dont have dummy texture to load 
                        // [TODO]
                        std::cout << "Texture loading failed, I dont know what to do next !" << std::endl;
                        return false;
                    }
                }
            }
            else {
                xMat.texDiffuse = nullptr;
                std::cout << "The material does not contain diffuse texture !" << std::endl;
            }

            // Load Material Properties.
            aiColor4D kADSE[4];
            material->Get(AI_MATKEY_COLOR_AMBIENT, kADSE[0]);
            material->Get(AI_MATKEY_COLOR_DIFFUSE, kADSE[1]);
            material->Get(AI_MATKEY_COLOR_SPECULAR, kADSE[2]);
            material->Get(AI_MATKEY_COLOR_EMISSIVE, kADSE[3]);

            xMat.matProps.kAmbient = glm::make_vec4(&kADSE[0].r);
            xMat.matProps.kDiffuse = glm::make_vec4(&kADSE[1].r);
            xMat.matProps.kSpecular = glm::make_vec4(&kADSE[2].r);
            xMat.matProps.kEmissive = glm::make_vec4(&kADSE[3].r);

            // Create uniform buffer
            xMat.propUniformBuffer.size = sizeof(xMat.matProps);

            VkBufferCreateInfo uniformBufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            uniformBufferCreateInfo.pNext = nullptr;
            uniformBufferCreateInfo.flags = 0;
            uniformBufferCreateInfo.size = xMat.propUniformBuffer.size;
            uniformBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            uniformBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            uniformBufferCreateInfo.queueFamilyIndexCount = 0;
            uniformBufferCreateInfo.pQueueFamilyIndices = nullptr;
            ZV_VK_CHECK(vkCreateBuffer(xParams.xDevice, &uniformBufferCreateInfo, nullptr, &xMat.propUniformBuffer.handle));
            ZV_VK_VALIDATE(allocateBufferMemory(xMat.propUniformBuffer.handle, &xMat.propUniformBuffer.memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), "MEMORY ALLOCATION FOR UNIFORM BUFFER FAILED !");
            ZV_VK_CHECK(vkBindBufferMemory(xParams.xDevice, xMat.propUniformBuffer.handle, xMat.propUniformBuffer.memory, 0));

            // Create stageBuffer and write uniform data to it.
            BufferParameters stageBuffer;

            stageBuffer.size = xMat.propUniformBuffer.size;
            VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bufCreateInfo.pNext = nullptr;
            bufCreateInfo.flags = 0;
            bufCreateInfo.size = stageBuffer.size;
            bufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            bufCreateInfo.queueFamilyIndexCount = 0;
            bufCreateInfo.pQueueFamilyIndices = nullptr;

            ZV_VK_CHECK(vkCreateBuffer(xParams.xDevice, &bufCreateInfo, nullptr, &stageBuffer.handle));
            ZV_VK_VALIDATE(allocateBufferMemory(stageBuffer.handle, &stageBuffer.memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), "Cannot allocate memory for staging buffer !");
            ZV_VK_CHECK(vkBindBufferMemory(xParams.xDevice, stageBuffer.handle, stageBuffer.memory, 0));

            void *mapAddress;
            ZV_VK_CHECK(vkMapMemory(xParams.xDevice, stageBuffer.memory, 0, stageBuffer.size, 0, &mapAddress));

            memcpy(mapAddress, &xMat.matProps, stageBuffer.size);

            VkMappedMemoryRange memRange = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
            memRange.memory = stageBuffer.memory;
            memRange.pNext = nullptr;
            memRange.offset = 0;
            memRange.size = stageBuffer.size;
            ZV_VK_CHECK(vkFlushMappedMemoryRanges(xParams.xDevice, 1, &memRange));

            // create command for transfer uniform data from staging buffer to uniform buffer.
            VkCommandBuffer copyCmd;
            VkCommandBufferAllocateInfo cmdbufAllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
            cmdbufAllocInfo.pNext = nullptr;
            cmdbufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdbufAllocInfo.commandPool = xParams.xRenderCmdPool;
            cmdbufAllocInfo.commandBufferCount = 1;
            ZV_VK_CHECK(vkAllocateCommandBuffers(xParams.xDevice, &cmdbufAllocInfo, &copyCmd));

            VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            beginInfo.pInheritanceInfo = nullptr;
            ZV_VK_CHECK(vkBeginCommandBuffer(copyCmd, &beginInfo));

            VkBufferCopy bufCpy = {};
            bufCpy.srcOffset = 0;
            bufCpy.dstOffset = 0;
            bufCpy.size = stageBuffer.size;
            
            vkCmdCopyBuffer(copyCmd, stageBuffer.handle, xMat.propUniformBuffer.handle, 1, &bufCpy);

            VkBufferMemoryBarrier bufferMemoryBarrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
            bufferMemoryBarrier.pNext = nullptr;
            bufferMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            bufferMemoryBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
            bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferMemoryBarrier.buffer = xMat.propUniformBuffer.handle;
            bufferMemoryBarrier.offset = 0;
            bufferMemoryBarrier.size = VK_WHOLE_SIZE;
            vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr);

            ZV_VK_CHECK(vkEndCommandBuffer(copyCmd));

            // submit the copy command to the graphic queue.
            VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submit.pNext = nullptr;
            submit.waitSemaphoreCount = 0;
            submit.pWaitSemaphores = nullptr;
            submit.pWaitDstStageMask = nullptr;
            submit.commandBufferCount = 1;
            submit.pCommandBuffers = &copyCmd;
            submit.signalSemaphoreCount = 0;
            submit.pSignalSemaphores = nullptr;

            VkFence fence;
            VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            fenceCreateInfo.pNext = nullptr;
            ZV_VK_CHECK(vkCreateFence(xParams.xDevice, &fenceCreateInfo, nullptr, &fence));
            ZV_VK_CHECK(vkResetFences(xParams.xDevice, 1, &fence));
            ZV_VK_CHECK(vkQueueSubmit(xParams.xGraphicQueue, 1, &submit, fence));
            ZV_VK_CHECK(vkWaitForFences(xParams.xDevice, 1, &fence, VK_TRUE, 1000000000ull));
            ZV_VK_CHECK(vkQueueWaitIdle(xParams.xGraphicQueue));
            vkDestroyFence(xParams.xDevice, fence, nullptr);
            vkFreeMemory(xParams.xDevice, stageBuffer.memory, nullptr);
            vkFreeCommandBuffers(xParams.xDevice, xParams.xRenderCmdPool, 1, &copyCmd);
            vkDestroyBuffer(xParams.xDevice, stageBuffer.handle, nullptr);
            
            xParamsTeapot.materials.push_back(xMat);

        }
         
        // Create and update descriptors for all materials.
        VkDescriptorPoolSize poolSize[2] = {};
        // Uniform descriptor size.
        poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize[0].descriptorCount = xParamsTeapot.materials.size();
        // Sampled-Image descriptor size.
        poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize[1].descriptorCount = xParamsTeapot.materials.size();

        // write the texture to the descriptor.
        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        descriptorPoolCreateInfo.flags = 0;
        descriptorPoolCreateInfo.pNext = nullptr;
        descriptorPoolCreateInfo.maxSets = xParamsTeapot.materials.size();
        descriptorPoolCreateInfo.poolSizeCount = 2;
        descriptorPoolCreateInfo.pPoolSizes = &poolSize[0];

        ZV_VK_CHECK(vkCreateDescriptorPool(xParams.xDevice, &descriptorPoolCreateInfo, nullptr, &xParams.xDescriptorPool));

        for (size_t i = 0; i < xParamsTeapot.materials.size(); ++i)
        {
            XMaterialTeapot &xMat = xParamsTeapot.materials[i];
            VkDescriptorSetAllocateInfo descSetAllocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
            descSetAllocInfo.pNext = nullptr;
            descSetAllocInfo.descriptorSetCount = 1;
            descSetAllocInfo.pSetLayouts = (xMat.texDiffuse ? &xParamsTeapot.xTexDescSetLayout : &xParamsTeapot.xBoldDescSetLayout);
            descSetAllocInfo.descriptorPool = xParams.xDescriptorPool;
            ZV_VK_CHECK(vkAllocateDescriptorSets(xParams.xDevice, &descSetAllocInfo, &xMat.descriptorSet));

            std::vector<VkWriteDescriptorSet> writeDescSets;
            // Update descriptor for uniform buffer. 
            VkDescriptorBufferInfo descBufferInfo = {};
            descBufferInfo.buffer = xMat.propUniformBuffer.handle;
            descBufferInfo.offset = 0;
            descBufferInfo.range = VK_WHOLE_SIZE;

            VkWriteDescriptorSet writeDescSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            writeDescSet.pNext = nullptr;
            writeDescSet.dstSet = xMat.descriptorSet;
            writeDescSet.dstBinding = 0;
            writeDescSet.dstArrayElement = 0;
            writeDescSet.descriptorCount = 1;
            writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writeDescSet.pImageInfo = nullptr;
            writeDescSet.pBufferInfo = &descBufferInfo;
            writeDescSet.pTexelBufferView = nullptr;
            writeDescSets.push_back(std::move(writeDescSet));

            // Update descriptor for sampled-image.
            VkDescriptorImageInfo textureImageInfo = { };
            if (xMat.texDiffuse)
            {
                textureImageInfo.imageView = xMat.texDiffuse->view;
                textureImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                textureImageInfo.sampler = xMat.texDiffuse->sampler;

                VkWriteDescriptorSet writeDescSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                writeDescSet.pNext = nullptr;
                writeDescSet.dstSet = xMat.descriptorSet;
                writeDescSet.dstBinding = 1;
                writeDescSet.dstArrayElement = 0;
                writeDescSet.descriptorCount = 1;
                writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writeDescSet.pImageInfo = &textureImageInfo;
                writeDescSet.pBufferInfo = nullptr;
                writeDescSet.pTexelBufferView = nullptr;
                writeDescSets.push_back(std::move(writeDescSet));
            }
            vkUpdateDescriptorSets(xParams.xDevice, xMat.texDiffuse ? 2 : 1, writeDescSets.data(), 0, nullptr);
        }
    }
    else {
        return false;
    }

    return true;
}

bool Xavier::XSampleTeapot::loadTextureFromFile(const std::string &file, const std::string &dir, Xavier::XTexture *&xTex)
{
    if (!xTex)
    {
        std::cout << "Input Texture pointer error !";
        return false;
    }

    if (file.find(".png") == std::string::npos)
    {
        std::cout << "Image format can not be loaded !" << std::endl;
        return false;
    }

    // if the tex is already in the lib, return the pointer.
    std::string path = dir + file;

    std::map<std::string, XTexture>::iterator found = xParamsTeapot.xTextureLib.find(file);
    if (found == xParamsTeapot.xTextureLib.end())
    {
        int width, height, channels;

        //We want to always get RGBA format data from reading the 
        const unsigned char* imageData = stbi_load(path.c_str(), &width, &height, &channels, 4);

        if (!imageData || width == 0 || height == 0)
        {
            std::cout << "Image file reading failed !" << std::endl;
            return false;
        }

        xTex = &xParamsTeapot.xTextureLib[file];
        switch (channels)
        {
        case 3:
            // We can know that the image is opaque.
            break;
        case 4:
            break;
        }

        xTex->format = VK_FORMAT_R8G8B8A8_UNORM;
        xTex->mipmapLevels = std::log2f((float)std::min(width, height));
        
        // Create Staging Buffer.
        BufferParameters stageBuffer;
            
        stageBuffer.size = width * height * 4;
        VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufCreateInfo.pNext = nullptr;
        bufCreateInfo.flags = 0;
        bufCreateInfo.size = stageBuffer.size;
        bufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufCreateInfo.queueFamilyIndexCount = 0;
        bufCreateInfo.pQueueFamilyIndices = nullptr;

        ZV_VK_CHECK(vkCreateBuffer(xParams.xDevice, &bufCreateInfo, nullptr, &stageBuffer.handle));

        ZV_VK_VALIDATE(allocateBufferMemory(stageBuffer.handle, &stageBuffer.memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), "Cannot allocate memory for staging buffer !");

        ZV_VK_CHECK(vkBindBufferMemory(xParams.xDevice, stageBuffer.handle, stageBuffer.memory, 0));

        void *mapAddress;
        ZV_VK_CHECK(vkMapMemory(xParams.xDevice, stageBuffer.memory, 0, stageBuffer.size, 0, &mapAddress));

        memcpy(mapAddress, imageData, stageBuffer.size);

        VkMappedMemoryRange memRange = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
        memRange.memory = stageBuffer.memory;
        memRange.pNext = nullptr;
        memRange.offset = 0;
        memRange.size = stageBuffer.size;
        ZV_VK_CHECK(vkFlushMappedMemoryRanges(xParams.xDevice, 1, &memRange));

        // Create texture vkImage
        VkImageCreateInfo imgCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imgCreateInfo.pNext = nullptr;
        imgCreateInfo.flags = 0;
        imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imgCreateInfo.format = xTex->format;
        imgCreateInfo.extent = { (uint32_t)width, (uint32_t)height, 1u };
        imgCreateInfo.mipLevels = xTex->mipmapLevels;
        imgCreateInfo.arrayLayers = 1;
        imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imgCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imgCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imgCreateInfo.queueFamilyIndexCount = 1;
        imgCreateInfo.pQueueFamilyIndices = &xParams.xGraphicFamilyQueueIndex;
        imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        ZV_VK_CHECK(vkCreateImage(xParams.xDevice, &imgCreateInfo, nullptr, &xTex->handle));

        // Allocate memory for the image.
        ZV_VK_VALIDATE(allocateImageMemory(xTex->handle, &xTex->memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), "MEMORY ALLOCATION FOR THE IMAGE FAILED !");
        
        ZV_VK_CHECK(vkBindImageMemory(xParams.xDevice, xTex->handle, xTex->memory, 0));



        VkCommandBuffer copyCmd;

        VkCommandBufferAllocateInfo cmdbufAllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        cmdbufAllocInfo.pNext = nullptr;
        cmdbufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdbufAllocInfo.commandPool = xParams.xRenderCmdPool;
        cmdbufAllocInfo.commandBufferCount = 1;
        ZV_VK_CHECK(vkAllocateCommandBuffers(xParams.xDevice, &cmdbufAllocInfo, &copyCmd));

        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.pInheritanceInfo = nullptr;
        ZV_VK_CHECK(vkBeginCommandBuffer(copyCmd, &beginInfo));

        // Change the layout of the sampled-image from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        VkImageMemoryBarrier imgMemBarrier1 = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        imgMemBarrier1.pNext = nullptr;
        imgMemBarrier1.srcAccessMask = 0;
        imgMemBarrier1.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imgMemBarrier1.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imgMemBarrier1.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imgMemBarrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imgMemBarrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imgMemBarrier1.image = xTex->handle;
        imgMemBarrier1.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &imgMemBarrier1);

        VkBufferImageCopy bufImgCpy;
        bufImgCpy.bufferOffset = 0;
        bufImgCpy.bufferRowLength = 0;
        bufImgCpy.bufferImageHeight = 0;
        bufImgCpy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        bufImgCpy.imageOffset = { 0,0,0 };
        bufImgCpy.imageExtent = { (uint32_t)width, (uint32_t)height, 1 };
        vkCmdCopyBufferToImage(copyCmd, stageBuffer.handle, xTex->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufImgCpy);

        VkImageMemoryBarrier imgMemBarrier2 = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        imgMemBarrier2.pNext = nullptr;
        imgMemBarrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imgMemBarrier2.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imgMemBarrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imgMemBarrier2.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imgMemBarrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imgMemBarrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imgMemBarrier2.image = xTex->handle;
        imgMemBarrier2.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &imgMemBarrier2);

        // Generate Mipmap
        for (uint32_t i = 1; i < xTex->mipmapLevels; i++)
        {
            VkImageMemoryBarrier nextLevelLayoutTransferDst = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            nextLevelLayoutTransferDst.pNext = nullptr;
            nextLevelLayoutTransferDst.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            nextLevelLayoutTransferDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            nextLevelLayoutTransferDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            nextLevelLayoutTransferDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            nextLevelLayoutTransferDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            nextLevelLayoutTransferDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            nextLevelLayoutTransferDst.image = xTex->handle;
            nextLevelLayoutTransferDst.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, 1 };
            vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &nextLevelLayoutTransferDst);
            
            VkImageBlit imgBlit = { };
            imgBlit.srcOffsets[0] = { 0, 0, 0 };
            imgBlit.srcOffsets[1] = { width >> (i - 1), height >> (i - 1), 1};
            imgBlit.dstOffsets[0] = { 0, 0, 0 };
            imgBlit.dstOffsets[1] = { width >> (i), height >> (i), 1 };
            imgBlit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1 };
            imgBlit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1 };

            vkCmdBlitImage(copyCmd, xTex->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, xTex->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgBlit, VK_FILTER_LINEAR);

            VkImageMemoryBarrier nextLevelLayoutTransferSrc = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            nextLevelLayoutTransferSrc.pNext = nullptr;
            nextLevelLayoutTransferSrc.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            nextLevelLayoutTransferSrc.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            nextLevelLayoutTransferSrc.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            nextLevelLayoutTransferSrc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            nextLevelLayoutTransferSrc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            nextLevelLayoutTransferSrc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            nextLevelLayoutTransferSrc.image = xTex->handle;
            nextLevelLayoutTransferSrc.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, 1 };
            vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &nextLevelLayoutTransferSrc);
        }
        
        // After generation for mipmap, we shall set the whole image layout for shader reading.
        VkImageMemoryBarrier wholeImageLayoutShaderReading = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        wholeImageLayoutShaderReading.pNext = nullptr;
        wholeImageLayoutShaderReading.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        wholeImageLayoutShaderReading.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        wholeImageLayoutShaderReading.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        wholeImageLayoutShaderReading.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        wholeImageLayoutShaderReading.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        wholeImageLayoutShaderReading.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        wholeImageLayoutShaderReading.image = xTex->handle;
        wholeImageLayoutShaderReading.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, xTex->mipmapLevels, 0, 1 };
        vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &wholeImageLayoutShaderReading);

        ZV_VK_CHECK(vkEndCommandBuffer(copyCmd));

        // submit the copy command to the graphic queue.
        VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit.pNext = nullptr;
        submit.waitSemaphoreCount = 0;
        submit.pWaitSemaphores = nullptr;
        submit.pWaitDstStageMask = nullptr;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &copyCmd;
        submit.signalSemaphoreCount = 0;
        submit.pSignalSemaphores = nullptr;

        VkFence fence;
        VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        fenceCreateInfo.pNext = nullptr;
        ZV_VK_CHECK(vkCreateFence(xParams.xDevice, &fenceCreateInfo, nullptr, &fence));
        ZV_VK_CHECK(vkResetFences(xParams.xDevice, 1, &fence));
        ZV_VK_CHECK(vkQueueSubmit(xParams.xGraphicQueue, 1, &submit, fence));
        ZV_VK_CHECK(vkWaitForFences(xParams.xDevice, 1, &fence, VK_TRUE, 1000000000ull));
        ZV_VK_CHECK(vkQueueWaitIdle(xParams.xGraphicQueue));
        vkDestroyFence(xParams.xDevice, fence, nullptr);
        vkFreeMemory(xParams.xDevice, stageBuffer.memory, nullptr);
        vkDestroyBuffer(xParams.xDevice, stageBuffer.handle, nullptr);
        vkFreeCommandBuffers(xParams.xDevice, xParams.xRenderCmdPool, 1, &copyCmd);

        // Create Sampler for the texture
        VkSamplerCreateInfo smlCreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        smlCreateInfo.flags = 0;
        smlCreateInfo.pNext = nullptr;
        smlCreateInfo.magFilter = VK_FILTER_NEAREST;
        smlCreateInfo.minFilter = VK_FILTER_NEAREST;
        smlCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        smlCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        smlCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        smlCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        // Close all the additional features, and handle them someday later 
        // [TODO]
        smlCreateInfo.mipLodBias = 0.0f;
        smlCreateInfo.anisotropyEnable = VK_FALSE;
        smlCreateInfo.maxAnisotropy = 1.0F;
        smlCreateInfo.compareEnable = VK_FALSE;
        smlCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
        smlCreateInfo.minLod = 4.0f;
        smlCreateInfo.maxLod = 4.0f;
        smlCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        smlCreateInfo.unnormalizedCoordinates = VK_FALSE;
        ZV_VK_CHECK(vkCreateSampler(xParams.xDevice, &smlCreateInfo, nullptr, &xTex->sampler));
        
        // Create Image View
        VkImageViewCreateInfo imgViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        imgViewCreateInfo.pNext = nullptr;
        imgViewCreateInfo.flags = 0;
        imgViewCreateInfo.image = xTex->handle;
        imgViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imgViewCreateInfo.format = xTex->format;
        imgViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        imgViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, xTex->mipmapLevels, 0, 1 };
        ZV_VK_CHECK(vkCreateImageView(xParams.xDevice, &imgViewCreateInfo, nullptr, &xTex->view));

    }
    else
    {
        xTex = &found->second;
    }
    // Else read the file
    return true;
}

bool Xavier::XSampleTeapot::loadMesh(aiMesh * mesh, uint32_t &indexBase)
{
    if (!mesh)
    {
        std::cout << "Mesh ptr null !" << std::endl;
        return false;
    }
    // Record Vertex Buffer.
    XMesh xMesh;
    xMesh.indexBase = indexBase;

    // Record Index 
    uint32_t numFace = mesh->mNumFaces;
    for (uint32_t i = 0; i < numFace; ++i)
    {
        aiFace *face = &mesh->mFaces[i];
        if (face->mNumIndices != 3)
        {
            std::cout << "Face: " << i << "Not a triangle !" << std::endl;
            continue;
        }

        xParamsTeapot.indices.push_back(face->mIndices[0]);
        xParamsTeapot.indices.push_back(face->mIndices[1]);
        xParamsTeapot.indices.push_back(face->mIndices[2]);
    }

    // Record Vertex
    // Ensure that the component of UV Components is 2
    if (mesh->mNumUVComponents[0] != 2)
    {
        std::cout << "The component of UV Components is not 2 !" << std::endl;
        return false;
    }

    uint32_t numVertex = mesh->mNumVertices;

    for (uint32_t i = 0; i < numVertex; ++i)
    {
        XVertex vert;

        vert.pos.r = mesh->mVertices[i].x;
        vert.pos.g = - mesh->mVertices[i].y;
        vert.pos.b = mesh->mVertices[i].z;

        vert.uv.r = mesh->mTextureCoords[0][i].x;
        vert.uv.g = 1.0f - mesh->mTextureCoords[0][i].y;

        vert.norm.r = mesh->mNormals[i].x;
        vert.norm.g = - mesh->mNormals[i].y;
        vert.norm.b = mesh->mNormals[i].z;

        xParamsTeapot.vertices.push_back(std::move(vert));
    }


    // update indexBase in Vertex Buffer
    xMesh.vertexCount = numVertex;
    xMesh.indexBase = indexBase;
    xMesh.indexCount = 3 * numFace;
    indexBase += xMesh.indexCount;

    xMesh.materialIndex = mesh->mMaterialIndex;
    xParamsTeapot.Meshes.push_back(xMesh);

    // Material 
    //uint32_t materialIndex = mesh->mMaterialIndex;
    //aiMaterial *material = scene->mMaterials[materialIndex];
    //aiString diffuseName;
    //material->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseName);

    //struct MaterialProterties
    //{
    //    aiColor3D       diffuse;
    //    aiColor3D       specular;
    //    aiColor3D       ambient;
    //    aiColor3D       emissive;
    //    aiColor3D       transparent;
    //    int             isWireframe;
    //    int             twoSided;
    //    aiShadingMode   shading;
    //    aiBlendMode     blend_func;
    //    float           opacity;
    //    float           shininess;
    //    float           shininessStength;
    //    float           refracti;

    //}properties;
    //aiString name;
    //material->Get(AI_MATKEY_COLOR_DIFFUSE, properties.diffuse);
    //material->Get(AI_MATKEY_COLOR_SPECULAR, properties.specular);
    //material->Get(AI_MATKEY_COLOR_AMBIENT, properties.ambient);
    //material->Get(AI_MATKEY_COLOR_EMISSIVE, properties.emissive);
    //material->Get(AI_MATKEY_COLOR_TRANSPARENT, properties.transparent);
    //material->Get(AI_MATKEY_ENABLE_WIREFRAME, properties.isWireframe);
    //material->Get(AI_MATKEY_TWOSIDED, properties.twoSided);
    //material->Get(AI_MATKEY_SHADING_MODEL, properties.shading);
    //material->Get(AI_MATKEY_BLEND_FUNC, properties.blend_func);
    //material->Get(AI_MATKEY_OPACITY, properties.opacity);
    //material->Get(AI_MATKEY_SHININESS, properties.shininess);
    //material->Get(AI_MATKEY_SHININESS_STRENGTH, properties.shininessStength);
    //material->Get(AI_MATKEY_REFRACTI, properties.refracti);
    //  

    return true;
}

bool Xavier::XSampleTeapot::prepareCameraAndLights()
{
    struct common {
        glm::mat4 MVP = {};
        glm::vec4 lightPos;
        glm::vec4 view ;
    } cameraAndLights;
    // calculate the data;
    glm::mat4 id;
    glm::mat4 model = glm::translate(id, glm::vec3{0.0f, 3.5f, 0.0f}) * glm::scale(id, glm::vec3{ 0.1f, 0.1f, 0.1f });
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)xParams.xSwapchain.extent.width / (float)xParams.xSwapchain.extent.height, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(glm::vec3{ 0.0f, 0.0f, 10.0f }, glm::vec3{ 0.0f,0.0f,0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f });

    cameraAndLights.MVP = proj * view * model;
    cameraAndLights.lightPos = { 0.0f,10.0f,0.0f, 0.0f };
    cameraAndLights.view = { 0.0f, 0.0f, 10.0f ,0.0f };

    BufferParameters staging;
    staging.size = sizeof(cameraAndLights);
    VkBufferCreateInfo stageBufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    stageBufCreateInfo.pNext = nullptr;
    stageBufCreateInfo.flags = 0;
    stageBufCreateInfo.size = staging.size;
    stageBufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stageBufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    stageBufCreateInfo.queueFamilyIndexCount = 0;
    stageBufCreateInfo.pQueueFamilyIndices = nullptr;
    
    ZV_VK_CHECK(vkCreateBuffer(xParams.xDevice, &stageBufCreateInfo, nullptr, &staging.handle));

    ZV_VK_VALIDATE(allocateBufferMemory(staging.handle, &staging.memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), "ERROR !");

    ZV_VK_CHECK(vkBindBufferMemory(xParams.xDevice, staging.handle, staging.memory, 0));

    void *mappedAddr;
    ZV_VK_CHECK(vkMapMemory(xParams.xDevice, staging.memory, 0, staging.size, 0, &mappedAddr));

    memcpy(mappedAddr, &cameraAndLights, staging.size);

    VkMappedMemoryRange mappedRange = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
    mappedRange.pNext = nullptr;
    mappedRange.memory = staging.memory;
    mappedRange.offset = 0;
    mappedRange.size = staging.size;
    ZV_VK_CHECK(vkFlushMappedMemoryRanges(xParams.xDevice, 1, &mappedRange));

    // Create common uniform buffer.
    xParamsTeapot.commonUniform.size = sizeof(cameraAndLights);
    VkBufferCreateInfo commonUniformBufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    commonUniformBufCreateInfo.pNext = nullptr;
    commonUniformBufCreateInfo.flags = 0;
    commonUniformBufCreateInfo.size = xParamsTeapot.commonUniform.size;
    commonUniformBufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT| VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    commonUniformBufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    commonUniformBufCreateInfo.queueFamilyIndexCount = 0;
    commonUniformBufCreateInfo.pQueueFamilyIndices = nullptr;

    ZV_VK_CHECK(vkCreateBuffer(xParams.xDevice, &commonUniformBufCreateInfo, nullptr, &xParamsTeapot.commonUniform.handle));

    ZV_VK_VALIDATE(allocateBufferMemory(xParamsTeapot.commonUniform.handle, &xParamsTeapot.commonUniform.memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), "ERROR !");

    ZV_VK_CHECK(vkBindBufferMemory(xParams.xDevice, xParamsTeapot.commonUniform.handle, xParamsTeapot.commonUniform.memory, 0));

    // COPY DATA
    vkWaitForFences(xParams.xDevice, 1, &xParams.xCopyFence, VK_TRUE, 1000000000ull);
    vkResetFences(xParams.xDevice, 1, &xParams.xCopyFence);
    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.pNext = nullptr;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    vkBeginCommandBuffer(xParams.xCopyCmd, &beginInfo);
    VkBufferCopy bufCopy = {};
    bufCopy.dstOffset = 0;
    bufCopy.srcOffset = 0;
    bufCopy.size = xParamsTeapot.commonUniform.size;
    vkCmdCopyBuffer(xParams.xCopyCmd, staging.handle, xParamsTeapot.commonUniform.handle, 1, &bufCopy);

    VkBufferMemoryBarrier barrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
    barrier.pNext = nullptr;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = xParamsTeapot.commonUniform.handle;
    barrier.offset = 0;
    barrier.size = xParamsTeapot.commonUniform.size;

    vkCmdPipelineBarrier(xParams.xCopyCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &barrier, 0, nullptr);

    vkEndCommandBuffer(xParams.xCopyCmd);
    VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.pNext = nullptr;
    submit.waitSemaphoreCount = 0;
    submit.pWaitSemaphores = nullptr;
    submit.pWaitDstStageMask = nullptr;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &xParams.xCopyCmd;
    submit.signalSemaphoreCount = 0;
    submit.pSignalSemaphores = nullptr;
    ZV_VK_CHECK(vkQueueSubmit(xParams.xGraphicQueue, 1, &submit, xParams.xCopyFence));

    // Create Descriptor pool for the common uniform
    VkDescriptorPoolSize poolSize = {};
    poolSize.descriptorCount = 1;
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    VkDescriptorPoolCreateInfo commonPoolCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    commonPoolCreateInfo.pNext = nullptr;
    commonPoolCreateInfo.flags = 0;
    commonPoolCreateInfo.maxSets = 1;
    commonPoolCreateInfo.poolSizeCount = 1;
    commonPoolCreateInfo.pPoolSizes = &poolSize;
    ZV_VK_CHECK(vkCreateDescriptorPool(xParams.xDevice, &commonPoolCreateInfo, nullptr, &xParamsTeapot.commonPool));

    // Create DescriptorSet layout 
    VkDescriptorSetLayoutBinding binding = {};
    binding.binding = 0;
    binding.descriptorCount = 1;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo commonLayoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    commonLayoutCreateInfo.pNext = nullptr;
    commonLayoutCreateInfo.flags = 0;
    commonLayoutCreateInfo.bindingCount = 1;
    commonLayoutCreateInfo.pBindings = &binding;
    ZV_VK_CHECK(vkCreateDescriptorSetLayout(xParams.xDevice, &commonLayoutCreateInfo, nullptr, &xParamsTeapot.commonSetLayout));

    VkDescriptorSetAllocateInfo setAllocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    setAllocateInfo.pNext = nullptr;
    setAllocateInfo.descriptorPool = xParamsTeapot.commonPool;
    setAllocateInfo.descriptorSetCount = 1;
    setAllocateInfo.pSetLayouts = &xParamsTeapot.commonSetLayout;
    ZV_VK_CHECK(vkAllocateDescriptorSets(xParams.xDevice, &setAllocateInfo, &xParamsTeapot.commonSet));

    VkDescriptorBufferInfo descBufferInfo = {};
    descBufferInfo.buffer = xParamsTeapot.commonUniform.handle;
    descBufferInfo.offset = 0;
    descBufferInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet commonSetWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    commonSetWrite.pNext = nullptr;
    commonSetWrite.dstSet = xParamsTeapot.commonSet;
    commonSetWrite.dstBinding = 0;
    commonSetWrite.dstArrayElement = 0;
    commonSetWrite.descriptorCount = 1;
    commonSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    commonSetWrite.pImageInfo = nullptr;
    commonSetWrite.pBufferInfo = &descBufferInfo;
    commonSetWrite.pTexelBufferView = nullptr;
    vkUpdateDescriptorSets(xParams.xDevice, 1, &commonSetWrite, 0, nullptr);

    ZV_VK_CHECK(vkQueueWaitIdle(xParams.xGraphicQueue));
    vkFreeMemory(xParams.xDevice, staging.memory, nullptr);
    vkDestroyBuffer(xParams.xDevice, staging.handle, nullptr);

     return true;
}

bool Xavier::XSampleTeapot::prepareVirutalFrames()
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

bool Xavier::XSampleTeapot::prepareRenderPasses()
{
    // Remember we shall take the depth attachment into consideration.
    VkAttachmentDescription attachments[2] = {};
    attachments[0].flags = 0;
    attachments[0].format = xParams.xSwapchain.format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachments[1].flags = 0;
    attachments[1].format = VK_FORMAT_D16_UNORM;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDependency subpassDependencies[2] = {};
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[0].dstSubpass = 0;
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    subpassDependencies[1].srcSubpass = 0;
    subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Attachment descs for the Subpass 0.
    VkAttachmentReference attachRef[2] = {};
    attachRef[0].attachment = 0;
    attachRef[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachRef[1].attachment = 1;
    attachRef[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Description for subpass 0.
    VkSubpassDescription subpassDesc = { };
    subpassDesc.flags = 0;
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.inputAttachmentCount = 0;
    subpassDesc.pInputAttachments= nullptr;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &attachRef[0];
    subpassDesc.pResolveAttachments = nullptr;
    subpassDesc.pDepthStencilAttachment = &attachRef[1];
    subpassDesc.preserveAttachmentCount = 0;
    subpassDesc.pPreserveAttachments = nullptr;

    VkRenderPassCreateInfo renderPassCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    renderPassCreateInfo.pNext = nullptr;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = 2;
    renderPassCreateInfo.pAttachments = &attachments[0];
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDesc;
    renderPassCreateInfo.dependencyCount = 2;
    renderPassCreateInfo.pDependencies = &subpassDependencies[0];

    ZV_VK_CHECK(vkCreateRenderPass(xParams.xDevice, &renderPassCreateInfo, nullptr, &xParams.xRenderPass));
    return true;
}

bool Xavier::XSampleTeapot::prepareGraphicsPipeline()
{
    // Shader changed, Blend added, BindDescriptorSetLayout, Depth Test;
     /// Create Pipeline Layout.
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    VkDescriptorSetLayout layouts[2] = { xParamsTeapot.commonSetLayout };
   
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = 2;
    pipelineLayoutCreateInfo.pSetLayouts = &layouts[0];
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    // Create Pipeline for tex-teapot 
    layouts[1] = xParamsTeapot.xTexDescSetLayout;
    ZV_VK_CHECK(vkCreatePipelineLayout(xParams.xDevice, &pipelineLayoutCreateInfo, nullptr, &xParamsTeapot.xTexPipelineLayout));

    //// Create pipeline for non-tex teapot
    //layouts[1] = xParamsTeapot.xBoldDescSetLayout;
    //ZV_VK_CHECK(vkCreatePipelineLayout(xParams.xDevice, &pipelineLayoutCreateInfo, nullptr, &xParamsTeapot.xBoldPipelineLayout));

    /// Pipeline Shader stages
    VkPipelineShaderStageCreateInfo shaderStages[4] = {};

    VkShaderModule shaderModule[4] = {};
    {
        VkShaderModuleCreateInfo shaderModuleCreateInfo[4] = {};

        /// Create shader module of vertex shader.
        std::vector<char> texTeapotVertexShaderData(m_getFileBinaryContent("Data/" SAMPLE_NUMBER "/texTeapot.vert.spv"));
        shaderModuleCreateInfo[0].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo[0].pNext = nullptr;
        shaderModuleCreateInfo[0].flags = 0;
        shaderModuleCreateInfo[0].pCode = (uint32_t *)texTeapotVertexShaderData.data();
        shaderModuleCreateInfo[0].codeSize = texTeapotVertexShaderData.size();
        ZV_VK_CHECK(vkCreateShaderModule(xParams.xDevice, &shaderModuleCreateInfo[0], nullptr, &shaderModule[0]));


        /// Create shader module of fragment shader.
        std::vector<char> texTeapotFragmentShaderData(m_getFileBinaryContent("Data/" SAMPLE_NUMBER "/texTeapot.frag.spv"));
        shaderModuleCreateInfo[1].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo[1].pNext = nullptr;
        shaderModuleCreateInfo[1].flags = 0;
        shaderModuleCreateInfo[1].pCode = (uint32_t *)texTeapotFragmentShaderData.data();
        shaderModuleCreateInfo[1].codeSize = texTeapotFragmentShaderData.size();
        ZV_VK_CHECK(vkCreateShaderModule(xParams.xDevice, &shaderModuleCreateInfo[1], nullptr, &shaderModule[1]));
        
        /// Create shader module of fragment shader.
        std::vector<char> noneTexTeapotVertexShaderData(m_getFileBinaryContent("Data/" SAMPLE_NUMBER "/noneTexTeapot.vert.spv"));
        shaderModuleCreateInfo[2].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo[2].pNext = nullptr;
        shaderModuleCreateInfo[2].flags = 0;
        shaderModuleCreateInfo[2].pCode = (uint32_t *)noneTexTeapotVertexShaderData.data();
        shaderModuleCreateInfo[2].codeSize = noneTexTeapotVertexShaderData.size();
        ZV_VK_CHECK(vkCreateShaderModule(xParams.xDevice, &shaderModuleCreateInfo[2], nullptr, &shaderModule[2]));
        /// Create shader module of fragment shader.
        std::vector<char> noneTexTeapotFragmentShaderData(m_getFileBinaryContent("Data/" SAMPLE_NUMBER "/noneTexTeapot.frag.spv"));
        shaderModuleCreateInfo[3].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo[3].pNext = nullptr;
        shaderModuleCreateInfo[3].flags = 0;
        shaderModuleCreateInfo[3].pCode = (uint32_t *)noneTexTeapotFragmentShaderData.data();
        shaderModuleCreateInfo[3].codeSize = noneTexTeapotFragmentShaderData.size();
        ZV_VK_CHECK(vkCreateShaderModule(xParams.xDevice, &shaderModuleCreateInfo[3], nullptr, &shaderModule[3]));

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

        shaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[2].pNext = nullptr;
        shaderStages[2].flags = 0;
        shaderStages[2].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[2].module = shaderModule[2];
        shaderStages[2].pName = "main";
        shaderStages[2].pSpecializationInfo = nullptr;

        shaderStages[3].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[3].pNext = nullptr;
        shaderStages[3].flags = 0;
        shaderStages[3].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[3].module = shaderModule[3];
        shaderStages[3].pName = "main";
        shaderStages[3].pSpecializationInfo = nullptr;
    }
    /// Input Attribute Binding.
    VkVertexInputBindingDescription vertexInputBind[1] = {};
    vertexInputBind[0].binding = 0;
    vertexInputBind[0].stride = sizeof(XVertex);
    vertexInputBind[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertexInputAttr[3] = {};
    vertexInputAttr[0].binding = 0;
    vertexInputAttr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttr[0].location = 0;
    vertexInputAttr[0].offset = offsetof(XVertex, pos);

    vertexInputAttr[1].binding = 0;
    vertexInputAttr[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttr[1].location = 1;
    vertexInputAttr[1].offset = offsetof(XVertex, norm);

    vertexInputAttr[2].binding = 0;
    vertexInputAttr[2].format = VK_FORMAT_R32G32_SFLOAT;
    vertexInputAttr[2].location = 2;
    vertexInputAttr[2].offset = offsetof(XVertex, uv);

    VkPipelineVertexInputStateCreateInfo vertexInput = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertexInput.pNext = nullptr;
    vertexInput.flags = 0;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &vertexInputBind[0];
    vertexInput.vertexAttributeDescriptionCount = 3;
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

    VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depthStencilState.pNext = nullptr;
    depthStencilState.flags = 0;
    depthStencilState.depthTestEnable = VK_TRUE;
    // depthWriteEnable decides whether the depth value needs to be replaced with the new value when it pass the depth test.
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    
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

    /// Gathering all the state info.
    VkGraphicsPipelineCreateInfo graphicPipelineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    graphicPipelineCreateInfo.flags = 0;
  
    graphicPipelineCreateInfo.stageCount = 2;    
    // vertex input info
    graphicPipelineCreateInfo.pVertexInputState = &vertexInput;
    graphicPipelineCreateInfo.pInputAssemblyState = &vertexInputAssembly;
    graphicPipelineCreateInfo.pTessellationState = nullptr;
    graphicPipelineCreateInfo.pViewportState = &viewportState;
    graphicPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    graphicPipelineCreateInfo.pMultisampleState = &multisampleState;
    graphicPipelineCreateInfo.pDepthStencilState = &depthStencilState;
    graphicPipelineCreateInfo.pColorBlendState = &colorBlendState;
    graphicPipelineCreateInfo.pDynamicState = nullptr;
    
    graphicPipelineCreateInfo.renderPass = xParams.xRenderPass;
    graphicPipelineCreateInfo.subpass = 0;
    graphicPipelineCreateInfo.basePipelineHandle;
    graphicPipelineCreateInfo.basePipelineIndex;

    graphicPipelineCreateInfo.layout = xParamsTeapot.xTexPipelineLayout;
    graphicPipelineCreateInfo.pStages = &shaderStages[0];
    ZV_VK_CHECK(vkCreateGraphicsPipelines(xParams.xDevice, VK_NULL_HANDLE, 1, &graphicPipelineCreateInfo, nullptr, &xParamsTeapot.xTexPipeline));

    /*graphicPipelineCreateInfo.layout = xParamsTeapot.xBoldPipelineLayout;
    graphicPipelineCreateInfo.pStages = &shaderStages[2];
    ZV_VK_CHECK(vkCreateGraphicsPipelines(xParams.xDevice, VK_NULL_HANDLE, 1, &graphicPipelineCreateInfo, nullptr, &xParamsTeapot.xBoldPipeline));*/

    return true;
}

bool Xavier::XSampleTeapot::prepareDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding bindings[2] = {};
    // Material Properties binding
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    // Texture binding
    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descBoldSetLayoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    descBoldSetLayoutCreateInfo.pNext = nullptr;
    // We will update the descriptor set created with this before we bind it with pipeline.
    descBoldSetLayoutCreateInfo.flags = 0;
    descBoldSetLayoutCreateInfo.bindingCount = 1;
    descBoldSetLayoutCreateInfo.pBindings = &bindings[0];
    ZV_VK_CHECK(vkCreateDescriptorSetLayout(xParams.xDevice, &descBoldSetLayoutCreateInfo, nullptr, &xParamsTeapot.xBoldDescSetLayout));

    VkDescriptorSetLayoutCreateInfo descTexSetLayoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    descTexSetLayoutCreateInfo.pNext = nullptr;
    // We will update the descriptor set created with this before we bind it with pipeline.
    descTexSetLayoutCreateInfo.flags = 0;
    descTexSetLayoutCreateInfo.bindingCount = 2;
    descTexSetLayoutCreateInfo.pBindings = &bindings[0];
    ZV_VK_CHECK(vkCreateDescriptorSetLayout(xParams.xDevice, &descTexSetLayoutCreateInfo, nullptr, &xParamsTeapot.xTexDescSetLayout));

    return true;
}

bool Xavier::XSampleTeapot::createDepthBuffer(Xavier::ImageParameters &depthImage)
{
    VkImageCreateInfo depthCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    depthCreateInfo.pNext = nullptr;
    depthCreateInfo.flags = 0;
    depthCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    depthCreateInfo.format = VK_FORMAT_D16_UNORM;
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
    depthImageViewCreateInfo.format = VK_FORMAT_D16_UNORM;
    /*depthImageViewCreateInfo.components;*/
    depthImageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

    ZV_VK_CHECK(vkCreateImageView(xParams.xDevice, &depthImageViewCreateInfo, nullptr, &depthImage.view))

    return true;
}

bool Xavier::XSampleTeapot::prepareFrameBuffer(std::vector<VkImageView> & view, VkFramebuffer & framebuffer)
{
    VkFramebufferCreateInfo framebufferCreateInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    framebufferCreateInfo.pNext = nullptr;
    framebufferCreateInfo.flags = 0;
    framebufferCreateInfo.renderPass = xParams.xRenderPass;
    framebufferCreateInfo.attachmentCount = 2;
    framebufferCreateInfo.pAttachments = view.data();
    framebufferCreateInfo.width = xParams.xSwapchain.extent.width;
    framebufferCreateInfo.height = xParams.xSwapchain.extent.height;
    framebufferCreateInfo.layers = 1;
    ZV_VK_CHECK(vkCreateFramebuffer(xParams.xDevice, &framebufferCreateInfo, nullptr, &framebuffer));
    return true;
}

bool Xavier::XSampleTeapot::buildCommandBuffer(VkCommandBuffer &xRenderCmdBuffer, VkFramebuffer &currentFrameBuffer)
{
    // Command buffer begin
    VkCommandBufferBeginInfo cmdBufBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cmdBufBeginInfo.pNext = nullptr;
    cmdBufBeginInfo.pInheritanceInfo = nullptr;
    
    ZV_VK_CHECK(vkBeginCommandBuffer(xRenderCmdBuffer, &cmdBufBeginInfo));

    std::vector<float> colorClearValue = { 0.2f, 0.2f, 0.2f, 0.0f };
    std::vector<float> depthClearValue = { 1.0f , 0};
    VkClearValue clearColor[2] = {};
    memcpy(&clearColor[0].color.float32[0], colorClearValue.data(), sizeof(float) * colorClearValue.size());
    memcpy(&clearColor[1].depthStencil.depth, depthClearValue.data(), sizeof(float) * depthClearValue.size());

    VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = xParams.xRenderPass;
    renderPassBeginInfo.framebuffer = currentFrameBuffer;
    renderPassBeginInfo.renderArea = { { 0, 0 }, { xParams.xSwapchain.extent.width, xParams.xSwapchain.extent.height } };
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = &clearColor[0];

    std::vector<VkDescriptorSet> descSets[2];
    descSets[0].push_back(xParamsTeapot.commonSet);
    descSets[1].push_back(xParamsTeapot.commonSet);

    descSets[0].push_back(xParamsTeapot.materials[0].texDiffuse? xParamsTeapot.materials[0].descriptorSet: xParamsTeapot.materials[1].descriptorSet);
    descSets[1].push_back(xParamsTeapot.materials[0].texDiffuse? xParamsTeapot.materials[1].descriptorSet: xParamsTeapot.materials[0].descriptorSet);
    vkCmdBeginRenderPass(xRenderCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    uint32_t vertexOffset = 0;
    for (uint32_t i = 0; i < xParamsTeapot.Meshes.size(); ++i)
    {

        uint32_t matIndex = xParamsTeapot.Meshes[i].materialIndex;
        if (xParamsTeapot.materials[matIndex].texDiffuse != nullptr)
        {
            vkCmdBindPipeline(xRenderCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, xParamsTeapot.xTexPipeline);
            vkCmdBindDescriptorSets(xRenderCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, xParamsTeapot.xTexPipelineLayout,
                0, 2, descSets[0].data(), 0, nullptr);
        }
        else
        {
            vkCmdBindPipeline(xRenderCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, xParamsTeapot.xBoldPipeline);
            vkCmdBindDescriptorSets(xRenderCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, xParamsTeapot.xBoldPipelineLayout,
                0, 2, descSets[1].data(), 0, nullptr);
        }

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(xRenderCmdBuffer, 0, 1, &xParams.xVertexBuffer.handle, &offset);
        vkCmdBindIndexBuffer(xRenderCmdBuffer, xParams.xIndexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(xRenderCmdBuffer, xParamsTeapot.Meshes[i].indexCount, 1, xParamsTeapot.Meshes[i].indexBase, vertexOffset, 0);
        // vkCmdDraw(xRenderCmdBuffer, 3, 1, 0, 0);
        vertexOffset += xParamsTeapot.Meshes[i].vertexCount;
    }
    vkCmdEndRenderPass(xRenderCmdBuffer);

    vkEndCommandBuffer(xRenderCmdBuffer);
    return true;
}