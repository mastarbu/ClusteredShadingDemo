#include "XSampleSponza.h"
#include "assimp\Importer.hpp"
#include "assimp\postProcess.h"
#include "assimp\scene.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

bool Xavier::XSampleSponza::Draw()
{
	return true;
}

bool Xavier::XSampleSponza::prepareRenderSample()
{
	if (!loadAssets("/teapot/teapot.obj")) return false;
	if (!prepareShaders()) return false;
	if (!prepareDescriptorSetLayout()) return false;
	if (!preparePipelineLayout()) return false;
	if (!prepareRenderPasses()) return false;
	if (!prepareGraphicsPipeline()) return false;
	if (!prepareComputePipeline()) return false;

	if( !prepareResourcesForDescriptorSets() )
	return true;
}

bool Xavier::XSampleSponza::buildPreZCommandBuffer(VkCommandBuffer & cmdBuffer, VkFramebuffer & currentFrameBuffer)
{
	// pre z
	VkCommandBufferBeginInfo cmdBI = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	cmdBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmdBuffer, &cmdBI);

	std::vector<float> colorClearValue = { 0.2f, 0.2f, 0.2f, 0.0f };
	std::vector<float> depthClearValue = { 1.0f , 0 };
	VkClearValue clearColor[2] = {};
	memcpy(&clearColor[0].color.float32[0], colorClearValue.data(), sizeof(float) * colorClearValue.size());
	memcpy(&clearColor[1].depthStencil.depth, depthClearValue.data(), sizeof(float) * depthClearValue.size());

	VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassBeginInfo.pNext = nullptr;
	renderPassBeginInfo.renderPass = xSponzaParams.trivialRenderPass;
	renderPassBeginInfo.framebuffer = currentFrameBuffer;
	renderPassBeginInfo.renderArea = { { 0, 0 }, { xParams.xSwapchain.extent.width, xParams.xSwapchain.extent.height } };
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = &clearColor[0];
	vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, xSponzaParams.pipelines.preZGraphicsPipeline.pipeline);
	return true;
}
 
bool Xavier::XSampleSponza::prepareResourcesForDescriptorSets()
{
	// Depth image for storage imageview
	std::map<VkFormat, VkFormat> depthView = {
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_R32_SFLOAT },

	};
	VkImageViewCreateInfo depthImageViewCI = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	depthImageViewCI.image = xParams.;
	depthImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthImageViewCI.format = depthView[xContextParams.depthStencilFormat];
	depthImageViewCI.components = {};
	depthImageViewCI.subresourceRange = {};

	return true;
}

//bool Xavier::XSampleSponza::enablePhysicalDeviceFeatures()
//{
//	VkPhysicalDeviceFeatures features;
//	vkGetPhysicalDeviceFeatures(xParams.xPhysicalDevice, &features);
//	// enable SSBO suport
//	features.shaderStorageImageMultisample;
//	
//	return false;
//}

bool Xavier::XSampleSponza::prepareCameraAndLights()
{
	struct common {
		glm::mat4 MVP = {};
		glm::vec4 lightPos;
		glm::vec4 view;
	} cameraAndLights;
	// calculate the data;
	glm::mat4 id;
	glm::mat4 model = glm::translate(id, glm::vec3{ 0.0f, 3.5f, 0.0f }) * glm::scale(id, glm::vec3{ 0.1f, 0.1f, 0.1f });
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

	void* mappedAddr;
	ZV_VK_CHECK(vkMapMemory(xParams.xDevice, staging.memory, 0, staging.size, 0, &mappedAddr));

	memcpy(mappedAddr, &cameraAndLights, staging.size);

	VkMappedMemoryRange mappedRange = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
	mappedRange.pNext = nullptr;
	mappedRange.memory = staging.memory;
	mappedRange.offset = 0;
	mappedRange.size = staging.size;
	ZV_VK_CHECK(vkFlushMappedMemoryRanges(xParams.xDevice, 1, &mappedRange));

	// Create common uniform buffer.
	xSponzaParams.commonUniform.size = sizeof(cameraAndLights);
	VkBufferCreateInfo commonUniformBufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	commonUniformBufCreateInfo.pNext = nullptr;
	commonUniformBufCreateInfo.flags = 0;
	commonUniformBufCreateInfo.size = xSponzaParams.commonUniform.size;
	commonUniformBufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	commonUniformBufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	commonUniformBufCreateInfo.queueFamilyIndexCount = 0;
	commonUniformBufCreateInfo.pQueueFamilyIndices = nullptr;

	ZV_VK_CHECK(vkCreateBuffer(xParams.xDevice, &commonUniformBufCreateInfo, nullptr, &xSponzaParams.commonUniform.handle));

	ZV_VK_VALIDATE(allocateBufferMemory(xSponzaParams.commonUniform.handle, &xSponzaParams.commonUniform.memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), "ERROR !");

	ZV_VK_CHECK(vkBindBufferMemory(xParams.xDevice, xSponzaParams.commonUniform.handle, xSponzaParams.commonUniform.memory, 0));

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
	bufCopy.size = xSponzaParams.commonUniform.size;
	vkCmdCopyBuffer(xParams.xCopyCmd, staging.handle, xSponzaParams.commonUniform.handle, 1, &bufCopy);

	VkBufferMemoryBarrier barrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	barrier.pNext = nullptr;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.buffer = xSponzaParams.commonUniform.handle;
	barrier.offset = 0;
	barrier.size = xSponzaParams.commonUniform.size;

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
	ZV_VK_CHECK(vkCreateDescriptorPool(xParams.xDevice, &commonPoolCreateInfo, nullptr, &xSponzaParams.commonPool));

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
	ZV_VK_CHECK(vkCreateDescriptorSetLayout(xParams.xDevice, &commonLayoutCreateInfo, nullptr, &xSponzaParams.commonSetLayout));

	VkDescriptorSetAllocateInfo setAllocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	setAllocateInfo.pNext = nullptr;
	setAllocateInfo.descriptorPool = xSponzaParams.commonPool;
	setAllocateInfo.descriptorSetCount = 1;
	setAllocateInfo.pSetLayouts = &xSponzaParams.commonSetLayout;
	ZV_VK_CHECK(vkAllocateDescriptorSets(xParams.xDevice, &setAllocateInfo, &xSponzaParams.commonSet));

	VkDescriptorBufferInfo descBufferInfo = {};
	descBufferInfo.buffer = xSponzaParams.commonUniform.handle;
	descBufferInfo.offset = 0;
	descBufferInfo.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet commonSetWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	commonSetWrite.pNext = nullptr;
	commonSetWrite.dstSet = xSponzaParams.commonSet;
	commonSetWrite.dstBinding = 0;
	commonSetWrite.dstArrayElement = 0;
	commonSetWrite.descriptorCount = 1;
	commonSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	commonSetWrite.pImageInfo = nullptr;
	commonSetWrite.pBufferInfo = &descBufferInfo;
	commonSetWrite.pTexelBufferView = nullptr;
	vkUpdateDescriptorSets(xParams.xDevice, 1, &commonSetWrite, 0, nullptr);

	ZV_VK_CHECK(vkQueueWaitIdle(xParams.xGraphicQueue));

	vkDestroyBuffer(xParams.xDevice, staging.handle, nullptr);
	vkFreeMemory(xParams.xDevice, staging.memory, nullptr);
	

	return true;
}

bool Xavier::XSampleSponza::prepareVulkan()
{
    Xavier::XRender::prepareVulkan();

    // Getting Compute Queue
    ZV_VK_VALIDATE(xParams.xPhysicalDevice != VK_NULL_HANDLE, "Physical Device null !");
    uint32_t computeQueueFamilyIndex;
    if (!checkQueueFamilySupportForCompute(xParams.xPhysicalDevice, computeQueueFamilyIndex))
    {
        std::cout << "Cannot find compute queue family" << std::endl;
        return false;
    }
    
    vkGetDeviceQueue(xParams.xDevice, xParams.xComputeFamilyQueueIndex, 1, &xSponzaParams.xComputeQueue);
}

bool Xavier::XSampleSponza::loadAssets(const std::string &objRelPath)
{
	Assimp::Importer sceneImporter;
	unsigned int flag = aiProcess_PreTransformVertices | aiProcess_Triangulate | aiProcess_GenNormals;

	// Import the obj file.
	sceneImporter.ReadFile(m_getAssetPath() + objRelPath, flag);

	// Get the Scene.
	const aiScene* scene = sceneImporter.GetScene();
	if (scene)
	{
		aiNode* rootNode = scene->mRootNode;
		uint32_t indexBase = 0;
		for (unsigned int i = 0; i < rootNode->mNumMeshes; ++i)
		{
			// Since we import the file with "aiProcess_PreTransformVertices" flag, all meshes will correspond to only one childNode.
			unsigned int meshIndex = rootNode->mMeshes[i];
			aiMesh* mesh = scene->mMeshes[meshIndex];
			bool rest = loadMesh(mesh, indexBase);
		}

		// Create VertexBuffer
		xParams.xVertexBuffer.size = xSponzaParams.vertices.size() * sizeof(XVertex);
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
		void* mapAddress;
		ZV_VK_CHECK(vkMapMemory(xParams.xDevice, vertexStageBuffer.memory, 0, vertexStageBuffer.size, 0, &mapAddress));

		memcpy(mapAddress, xSponzaParams.vertices.data(), vertexStageBuffer.size);

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
		xParams.xIndexBuffer.size = xSponzaParams.indices.size() * sizeof(uint32_t);
		VkBufferCreateInfo indexBufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };;
		indexBufCreateInfo.flags = 0;
		indexBufCreateInfo.pNext = nullptr;
		indexBufCreateInfo.queueFamilyIndexCount = 0;
		indexBufCreateInfo.pQueueFamilyIndices = nullptr;
		indexBufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		indexBufCreateInfo.size = xParams.xIndexBuffer.size;
		indexBufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ZV_VK_CHECK(vkCreateBuffer(xParams.xDevice, &indexBufCreateInfo, nullptr, &xParams.xIndexBuffer.handle));

		ZV_VK_VALIDATE(allocateBufferMemory(xParams.xIndexBuffer.handle, &xParams.xIndexBuffer.memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), "CREATION FOR INDEX BUFFER FAILED !");

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

		memcpy(mapAddress, xSponzaParams.indices.data(), indexStageBuffer.size);

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
		
		vkDestroyBuffer(xParams.xDevice, vertexStageBuffer.handle, nullptr);
		vkFreeMemory(xParams.xDevice, vertexStageBuffer.memory, nullptr);
	
		vkDestroyBuffer(xParams.xDevice, indexStageBuffer.handle, nullptr);
		vkFreeMemory(xParams.xDevice, indexStageBuffer.memory, nullptr);
	
		
	


	//	// Load Texture Materials
	//	for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
	//	{
	//		aiMaterial* material = scene->mMaterials[i];
	//		XMaterialSponza xMat = {};
	//		std::cout << material->GetName().C_Str() << std::endl;

	//		// Load Diffuse Texture.
	//		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
	//		{
	//			aiString texFileName;
	//			if (aiReturn_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texFileName))
	//			{
	//				XTexture* xTex;
	//				if (loadTextureFromFile(texFileName.C_Str(), m_getTexturePath(), xTex))
	//				{
	//					xMat.texDiffuse = xTex;
	//				}

	//				else {
	//					// Load dummy texture
	//					// However we dont have dummy texture to load 
	//					// [TODO]
	//					std::cout << "Texture loading failed, I dont know what to do next !" << std::endl;
	//					return false;
	//				}
	//			}
	//		}
	//		else {
	//			xMat.texDiffuse = nullptr;
	//			std::cout << "The material does not contain diffuse texture !" << std::endl;
	//		}

	//		// Load Material Properties.
	//		aiColor4D kADSE[4];
	//		material->Get(AI_MATKEY_COLOR_AMBIENT, kADSE[0]);
	//		material->Get(AI_MATKEY_COLOR_DIFFUSE, kADSE[1]);
	//		material->Get(AI_MATKEY_COLOR_SPECULAR, kADSE[2]);
	//		material->Get(AI_MATKEY_COLOR_EMISSIVE, kADSE[3]);

	//		xMat.matProps.kAmbient = glm::make_vec4(&kADSE[0].r);
	//		xMat.matProps.kDiffuse = glm::make_vec4(&kADSE[1].r);
	//		xMat.matProps.kSpecular = glm::make_vec4(&kADSE[2].r);
	//		xMat.matProps.kEmissive = glm::make_vec4(&kADSE[3].r);

	//		// Create uniform buffer
	//		xMat.propUniformBuffer.size = sizeof(xMat.matProps);

	//		VkBufferCreateInfo uniformBufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	//		uniformBufferCreateInfo.pNext = nullptr;
	//		uniformBufferCreateInfo.flags = 0;
	//		uniformBufferCreateInfo.size = xMat.propUniformBuffer.size;
	//		uniformBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	//		uniformBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	//		uniformBufferCreateInfo.queueFamilyIndexCount = 0;
	//		uniformBufferCreateInfo.pQueueFamilyIndices = nullptr;
	//		ZV_VK_CHECK(vkCreateBuffer(xParams.xDevice, &uniformBufferCreateInfo, nullptr, &xMat.propUniformBuffer.handle));
	//		ZV_VK_VALIDATE(allocateBufferMemory(xMat.propUniformBuffer.handle, &xMat.propUniformBuffer.memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), "MEMORY ALLOCATION FOR UNIFORM BUFFER FAILED !");
	//		ZV_VK_CHECK(vkBindBufferMemory(xParams.xDevice, xMat.propUniformBuffer.handle, xMat.propUniformBuffer.memory, 0));

	//		// Create stageBuffer and write uniform data to it.
	//		BufferParameters stageBuffer;

	//		stageBuffer.size = xMat.propUniformBuffer.size;
	//		VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	//		bufCreateInfo.pNext = nullptr;
	//		bufCreateInfo.flags = 0;
	//		bufCreateInfo.size = stageBuffer.size;
	//		bufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	//		bufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	//		bufCreateInfo.queueFamilyIndexCount = 0;
	//		bufCreateInfo.pQueueFamilyIndices = nullptr;

	//		ZV_VK_CHECK(vkCreateBuffer(xParams.xDevice, &bufCreateInfo, nullptr, &stageBuffer.handle));
	//		ZV_VK_VALIDATE(allocateBufferMemory(stageBuffer.handle, &stageBuffer.memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), "Cannot allocate memory for staging buffer !");
	//		ZV_VK_CHECK(vkBindBufferMemory(xParams.xDevice, stageBuffer.handle, stageBuffer.memory, 0));

	//		void* mapAddress;
	//		ZV_VK_CHECK(vkMapMemory(xParams.xDevice, stageBuffer.memory, 0, stageBuffer.size, 0, &mapAddress));

	//		memcpy(mapAddress, &xMat.matProps, stageBuffer.size);

	//		VkMappedMemoryRange memRange = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
	//		memRange.memory = stageBuffer.memory;
	//		memRange.pNext = nullptr;
	//		memRange.offset = 0;
	//		memRange.size = stageBuffer.size;
	//		ZV_VK_CHECK(vkFlushMappedMemoryRanges(xParams.xDevice, 1, &memRange));

	//		// create command for transfer uniform data from staging buffer to uniform buffer.
	//		VkCommandBuffer copyCmd;
	//		VkCommandBufferAllocateInfo cmdbufAllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	//		cmdbufAllocInfo.pNext = nullptr;
	//		cmdbufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	//		cmdbufAllocInfo.commandPool = xParams.xRenderCmdPool;
	//		cmdbufAllocInfo.commandBufferCount = 1;
	//		ZV_VK_CHECK(vkAllocateCommandBuffers(xParams.xDevice, &cmdbufAllocInfo, &copyCmd));

	//		VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	//		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	//		beginInfo.pInheritanceInfo = nullptr;
	//		ZV_VK_CHECK(vkBeginCommandBuffer(copyCmd, &beginInfo));

	//		VkBufferCopy bufCpy = {};
	//		bufCpy.srcOffset = 0;
	//		bufCpy.dstOffset = 0;
	//		bufCpy.size = stageBuffer.size;

	//		vkCmdCopyBuffer(copyCmd, stageBuffer.handle, xMat.propUniformBuffer.handle, 1, &bufCpy);

	//		VkBufferMemoryBarrier bufferMemoryBarrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	//		bufferMemoryBarrier.pNext = nullptr;
	//		bufferMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	//		bufferMemoryBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
	//		bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//		bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//		bufferMemoryBarrier.buffer = xMat.propUniformBuffer.handle;
	//		bufferMemoryBarrier.offset = 0;
	//		bufferMemoryBarrier.size = VK_WHOLE_SIZE;
	//		vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	//			VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr);

	//		ZV_VK_CHECK(vkEndCommandBuffer(copyCmd));

	//		// submit the copy command to the graphic queue.
	//		VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	//		submit.pNext = nullptr;
	//		submit.waitSemaphoreCount = 0;
	//		submit.pWaitSemaphores = nullptr;
	//		submit.pWaitDstStageMask = nullptr;
	//		submit.commandBufferCount = 1;
	//		submit.pCommandBuffers = &copyCmd;
	//		submit.signalSemaphoreCount = 0;
	//		submit.pSignalSemaphores = nullptr;

	//		VkFence fence;
	//		VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	//		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	//		fenceCreateInfo.pNext = nullptr;
	//		ZV_VK_CHECK(vkCreateFence(xParams.xDevice, &fenceCreateInfo, nullptr, &fence));
	//		ZV_VK_CHECK(vkResetFences(xParams.xDevice, 1, &fence));
	//		ZV_VK_CHECK(vkQueueSubmit(xParams.xGraphicQueue, 1, &submit, fence));
	//		ZV_VK_CHECK(vkWaitForFences(xParams.xDevice, 1, &fence, VK_TRUE, 1000000000ull));
	//		ZV_VK_CHECK(vkQueueWaitIdle(xParams.xGraphicQueue));
	//		vkDestroyFence(xParams.xDevice, fence, nullptr);
	//		vkFreeMemory(xParams.xDevice, stageBuffer.memory, nullptr);
	//		vkFreeCommandBuffers(xParams.xDevice, xParams.xRenderCmdPool, 1, &copyCmd);
	//		vkDestroyBuffer(xParams.xDevice, stageBuffer.handle, nullptr);

	//		xSponzaParams.materials.push_back(xMat);

	//	}

	//	// Create and update descriptors for all materials.
	//	VkDescriptorPoolSize poolSize[2] = {};
	//	// Uniform descriptor size.
	//	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	//	poolSize[0].descriptorCount = xSponzaParams.materials.size();
	//	// Sampled-Image descriptor size.
	//	poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	//	poolSize[1].descriptorCount = xSponzaParams.materials.size();

	//	// write the texture to the descriptor.
	//	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	//	descriptorPoolCreateInfo.flags = 0;
	//	descriptorPoolCreateInfo.pNext = nullptr;
	//	descriptorPoolCreateInfo.maxSets = xSponzaParams.materials.size();
	//	descriptorPoolCreateInfo.poolSizeCount = 2;
	//	descriptorPoolCreateInfo.pPoolSizes = &poolSize[0];

	//	ZV_VK_CHECK(vkCreateDescriptorPool(xParams.xDevice, &descriptorPoolCreateInfo, nullptr, &xParams.xDescriptorPool));

	//	for (size_t i = 0; i < xSponzaParams.materials.size(); ++i)
	//	{
	//		XMaterialSponza& xMat = xSponzaParams.materials[i];
	//		VkDescriptorSetAllocateInfo descSetAllocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	//		descSetAllocInfo.pNext = nullptr;
	//		descSetAllocInfo.descriptorSetCount = 1;
	//		descSetAllocInfo.pSetLayouts = (xMat.texDiffuse ? &xSponzaParams.xTexDescSetLayout : &xParamsTeapot.xBoldDescSetLayout);
	//		descSetAllocInfo.descriptorPool = xParams.xDescriptorPool;
	//		ZV_VK_CHECK(vkAllocateDescriptorSets(xParams.xDevice, &descSetAllocInfo, &xMat.descriptorSet));

	//		std::vector<VkWriteDescriptorSet> writeDescSets;
	//		// Update descriptor for uniform buffer. 
	//		VkDescriptorBufferInfo descBufferInfo = {};
	//		descBufferInfo.buffer = xMat.propUniformBuffer.handle;
	//		descBufferInfo.offset = 0;
	//		descBufferInfo.range = VK_WHOLE_SIZE;

	//		VkWriteDescriptorSet writeDescSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	//		writeDescSet.pNext = nullptr;
	//		writeDescSet.dstSet = xMat.descriptorSet;
	//		writeDescSet.dstBinding = 0;
	//		writeDescSet.dstArrayElement = 0;
	//		writeDescSet.descriptorCount = 1;
	//		writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	//		writeDescSet.pImageInfo = nullptr;
	//		writeDescSet.pBufferInfo = &descBufferInfo;
	//		writeDescSet.pTexelBufferView = nullptr;
	//		writeDescSets.push_back(writeDescSet);

	//		// Update descriptor for sampled-image.
	//		VkDescriptorImageInfo textureImageInfo = { };
	//		if (xMat.texDiffuse)
	//		{
	//			textureImageInfo.imageView = xMat.texDiffuse->view;
	//			textureImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//			textureImageInfo.sampler = xMat.texDiffuse->sampler;

	//			VkWriteDescriptorSet writeDescSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	//			writeDescSet.pNext = nullptr;
	//			writeDescSet.dstSet = xMat.descriptorSet;
	//			writeDescSet.dstBinding = 1;
	//			writeDescSet.dstArrayElement = 0;
	//			writeDescSet.descriptorCount = 1;
	//			writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	//			writeDescSet.pImageInfo = &textureImageInfo;
	//			writeDescSet.pBufferInfo = nullptr;
	//			writeDescSet.pTexelBufferView = nullptr;
	//			writeDescSets.push_back(std::move(writeDescSet));
	//		}
	//		vkUpdateDescriptorSets(xParams.xDevice, xMat.texDiffuse ? 2 : 1, writeDescSets.data(), 0, nullptr);
	//	}
	}
	else {
		return false;
	}

	return true;
}

bool Xavier::XSampleSponza::loadMesh(aiMesh* mesh, uint32_t& indexBase)
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
		aiFace* face = &mesh->mFaces[i];
		if (face->mNumIndices != 3)
		{
			std::cout << "Face: " << i << "Not a triangle !" << std::endl;
			continue;
		}

		xSponzaParams.indices.push_back(face->mIndices[0]);
		xSponzaParams.indices.push_back(face->mIndices[1]);
		xSponzaParams.indices.push_back(face->mIndices[2]);
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
		vert.pos.g = -mesh->mVertices[i].y;
		vert.pos.b = mesh->mVertices[i].z;

		vert.uv.r = mesh->mTextureCoords[0][i].x;
		vert.uv.g = 1.0f - mesh->mTextureCoords[0][i].y;

		vert.norm.r = mesh->mNormals[i].x;
		vert.norm.g = -mesh->mNormals[i].y;
		vert.norm.b = mesh->mNormals[i].z;

		xSponzaParams.vertices.push_back(std::move(vert));
	}


	// update indexBase in Vertex Buffer
	xMesh.vertexCount = numVertex;
	xMesh.indexBase = indexBase;
	xMesh.indexCount = 3 * numFace;
	indexBase += xMesh.indexCount;

	xMesh.materialIndex = mesh->mMaterialIndex;
	xSponzaParams.Meshes.push_back(xMesh);

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

bool Xavier::XSampleSponza::prepareDescriptorSetLayout()
{
    // Shader Stage Bits Combination
    VkShaderStageFlags comp = VK_SHADER_STAGE_COMPUTE_BIT;
    VkShaderStageFlags vert = VK_SHADER_STAGE_VERTEX_BIT;
    VkShaderStageFlags comp_frag = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    VkShaderStageFlags vert_frag = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    VkShaderStageFlags comp_vert_frag = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    // Descriptor Bindings
    VkDescriptorSetLayoutBinding globalUnifrm =
    { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, comp_vert_frag };
	VkDescriptorSetLayoutBinding depthStorage =
	{ 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, comp };
    VkDescriptorSetLayoutBinding uniqueClustersBuffer =
    { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, comp };
    VkDescriptorSetLayoutBinding clusterIndexStorageImage =
    { 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, comp_frag };
    VkDescriptorSetLayoutBinding allLightsBuffer =
    { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, comp_frag };
    VkDescriptorSetLayoutBinding lightGridBuffer =
    { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, comp_frag };
    VkDescriptorSetLayoutBinding cluseterLightBuffer =
    { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, comp_frag };
    
    std::vector<VkDescriptorSetLayoutBinding> bindingGroups;
    VkDescriptorSetLayoutCreateInfo setlayoutCI = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    // global uniform set layout 
    bindingGroups.push_back(globalUnifrm);
    setlayoutCI.bindingCount = bindingGroups.size();
    setlayoutCI.pBindings = bindingGroups.data();
    ZV_VK_CHECK(vkCreateDescriptorSetLayout(xParams.xDevice, &setlayoutCI, nullptr, &xSponzaParams.descSetsLayouts.globalUniform));
    bindingGroups.clear();

    // find unique clusters pipleine specialized set layout
	depthStorage.binding = 0;
    clusterIndexStorageImage.binding = 1;
    uniqueClustersBuffer.binding = 2;
    bindingGroups = { depthStorage, uniqueClustersBuffer, clusterIndexStorageImage };
    setlayoutCI.bindingCount = bindingGroups.size();
    setlayoutCI.pBindings = bindingGroups.data();
    ZV_VK_CHECK(vkCreateDescriptorSetLayout(xParams.xDevice, &setlayoutCI, nullptr, &xSponzaParams.descSetsLayouts.findUniqueCluster));
    bindingGroups.clear();

	/* To Modified */
    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 5 },
    };

    VkDescriptorPoolCreateInfo descPoolCI = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    descPoolCI.maxSets = 2;
    descPoolCI.poolSizeCount = poolSizes.size();
    descPoolCI.pPoolSizes = poolSizes.data();
    ZV_VK_CHECK(vkCreateDescriptorPool(xParams.xDevice, &descPoolCI, nullptr, &xParams.xDescriptorPool));

    // Allocation for Descriptor Set
    VkDescriptorSetAllocateInfo descSetAllocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, 0, xParams.xDescriptorPool };
	// Global Uniform 
    descSetAllocInfo.descriptorSetCount = 1;
    descSetAllocInfo.pSetLayouts = &xSponzaParams.descSetsLayouts.globalUniform;
    ZV_VK_CHECK(vkAllocateDescriptorSets(xParams.xDevice, &descSetAllocInfo, &xSponzaParams.descSets.globalUniform));

	// FindUniqueCLust3r spec desc set
	descSetAllocInfo.descriptorSetCount = 1;
	descSetAllocInfo.pSetLayouts = &xSponzaParams.descSetsLayouts.findUniqueCluster;
	ZV_VK_CHECK(vkAllocateDescriptorSets(xParams.xDevice, &descSetAllocInfo, &xSponzaParams.descSets.findUniqueCluster));
	
}

bool Xavier::XSampleSponza::preparePipelineLayout()
{
	// pre z 
	std::vector<VkDescriptorSetLayout> setlayouts;
	setlayouts.push_back(xSponzaParams.descSetsLayouts.globalUniform);
	VkPipelineLayoutCreateInfo pipelineLayoutCI = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutCI.setLayoutCount = setlayouts.size();
	pipelineLayoutCI.pSetLayouts = setlayouts.data();
	ZV_VK_CHECK(vkCreatePipelineLayout(xParams.xDevice, &pipelineLayoutCI, nullptr, &xSponzaParams.pipelines.preZGraphicsPipeline.pipeLayout));
	setlayouts.clear();

	// find unique clusters
	setlayouts.push_back(xSponzaParams.descSetsLayouts.globalUniform);
	setlayouts.push_back(xSponzaParams.descSetsLayouts.findUniqueCluster);
	pipelineLayoutCI.setLayoutCount = setlayouts.size();
	pipelineLayoutCI.pSetLayouts = setlayouts.data();
	ZV_VK_CHECK(vkCreatePipelineLayout(xParams.xDevice, &pipelineLayoutCI, nullptr, &xSponzaParams.pipelines.uniqueClustersFindingCompPipe.pipeLayout));
	setlayouts.clear();
	return true;
}

bool Xavier::XSampleSponza::prepareShaders()
{
	if (XShader::xParams == nullptr)
	{
		XShader::xParams = &xParams;
		xSponzaParams.shaders.prezVS.loadShader("Data/" SAMPLE_NUMBER "/trivial.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		xSponzaParams.shaders.prezFS.loadShader("Data/" SAMPLE_NUMBER "/trivial.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		xSponzaParams.shaders.findUniqueClusterCP.loadShader("Data/" SAMPLE_NUMBER "/findUniqueClusters.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
	}
	else
	{
		std::cout << "shader reloading" << std::endl;
	}
	return true;
}

bool Xavier::XSampleSponza::prepareRenderPasses()
{
	// trivial pass 
	VkRenderPassCreateInfo RenderPassCI = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };

	VkAttachmentDescription attach[2] = {};
	// color
	attach[0].format = xParams.xSwapchain.format;
	attach[0].samples = xContextParams.frameBufferColorAttachImageMaxSampleNumber;
	attach[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attach[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attach[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attach[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attach[0].initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attach[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// depth
	attach[1].format = xContextParams.depthStencilFormat;
	attach[1].samples = xContextParams.frameBufferDepthStencilAttachImageMaxSampleNumber;
	attach[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attach[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attach[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attach[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attach[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attach[1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// Reference(sub pass 0)
	// color
	VkAttachmentReference attachRef[2] = {};
	attachRef[0].attachment = 0;
	attachRef[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// depth
	attachRef[1].attachment = 1;
	attachRef[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDesc[1] = {};
	subpassDesc[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc[0].colorAttachmentCount = 1;
	subpassDesc[0].pColorAttachments = &attachRef[0];
	subpassDesc[0].pDepthStencilAttachment = &attachRef[1];
	
	RenderPassCI.attachmentCount = 2;
	RenderPassCI.pAttachments = &attach[0];
	RenderPassCI.subpassCount = 1;
	RenderPassCI.pSubpasses = &subpassDesc[0];

	ZV_VK_CHECK(vkCreateRenderPass(xParams.xDevice, &RenderPassCI, nullptr, &xSponzaParams.trivialRenderPass));

	return true;
}

bool Xavier::XSampleSponza::prepareGraphicsPipeline()
{
	// Pipeline Pre Z Pass 
	VkGraphicsPipelineCreateInfo graphicsPipelineCI = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	// pre Z pipeline 

	VkPipelineShaderStageCreateInfo shaderstage[2] = 
	{
		xSponzaParams.shaders.prezVS.getCreateInfo(),
		xSponzaParams.shaders.prezFS.getCreateInfo() 
	};
	graphicsPipelineCI.stageCount = 2;
	graphicsPipelineCI.pStages = &shaderstage[0];

	VkPipelineVertexInputStateCreateInfo vertInput = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	VkVertexInputBindingDescription inputBinding = { 0, sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX };
	vertInput.vertexBindingDescriptionCount = 1;
	vertInput.pVertexBindingDescriptions = &inputBinding;

	VkVertexInputAttributeDescription attrDescrtion = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 };
	vertInput.vertexAttributeDescriptionCount = 1;
	vertInput.pVertexAttributeDescriptions = &attrDescrtion;
	graphicsPipelineCI.pVertexInputState = &vertInput;

	VkPipelineInputAssemblyStateCreateInfo assemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	assemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	graphicsPipelineCI.pInputAssemblyState = &assemblyState;

	graphicsPipelineCI.pTessellationState = nullptr;
	
	VkPipelineViewportStateCreateInfo viewport = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewport.viewportCount = 1;
	viewport.scissorCount = 1;
	graphicsPipelineCI.pViewportState = &viewport;
	
	VkPipelineRasterizationStateCreateInfo rasterStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterStateInfo.lineWidth = 1.0f;
	graphicsPipelineCI.pRasterizationState = &rasterStateInfo;

	VkPipelineMultisampleStateCreateInfo multisampleSateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleSateInfo.rasterizationSamples = xContextParams.frameBufferColorAttachImageMaxSampleNumber;
	graphicsPipelineCI.pMultisampleState = &multisampleSateInfo;

	VkPipelineDepthStencilStateCreateInfo depthStencilStageInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthStencilStageInfo.depthWriteEnable = VK_TRUE;
	depthStencilStageInfo.depthTestEnable = VK_TRUE;
	depthStencilStageInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	graphicsPipelineCI.pDepthStencilState = &depthStencilStageInfo;

	VkPipelineColorBlendStateCreateInfo colorBlendStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	VkPipelineColorBlendAttachmentState blendAttachState = { VK_FALSE };
	colorBlendStateInfo.attachmentCount = 1;
	colorBlendStateInfo.pAttachments = &blendAttachState;
	graphicsPipelineCI.pColorBlendState = &colorBlendStateInfo;

	VkPipelineDynamicStateCreateInfo dynamicCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	dynamicCreateInfo.dynamicStateCount = dynamicStates.size();
	dynamicCreateInfo.pDynamicStates = dynamicStates.data();
	graphicsPipelineCI.pDynamicState = &dynamicCreateInfo;
	graphicsPipelineCI.layout = xSponzaParams.pipelines.preZGraphicsPipeline.pipeLayout;
	graphicsPipelineCI.renderPass = xSponzaParams.trivialRenderPass;
	graphicsPipelineCI.subpass = 0;
	graphicsPipelineCI.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCI.basePipelineIndex = -1;

	ZV_VK_CHECK(vkCreateGraphicsPipelines(xParams.xDevice, nullptr, 1, &graphicsPipelineCI, nullptr, &xSponzaParams.pipelines.preZGraphicsPipeline.pipeline));
	return true;
}

bool Xavier::XSampleSponza::prepareComputePipeline()
{
    /* create pipeline for stage 0 */
    VkComputePipelineCreateInfo ComputePipelineCI = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    ComputePipelineCI.layout = xSponzaParams.pipelines.uniqueClustersFindingCompPipe.pipeLayout;
    ComputePipelineCI.stage = xSponzaParams.shaders.findUniqueClusterCP.getCreateInfo();
    ComputePipelineCI.basePipelineHandle = VK_NULL_HANDLE;
    ComputePipelineCI.basePipelineIndex = -1;
	ZV_VK_CHECK(vkCreateComputePipelines(xParams.xDevice, VK_NULL_HANDLE, 1, &ComputePipelineCI, nullptr, &xSponzaParams.pipelines.uniqueClustersFindingCompPipe.pipeline));

    return true;
}
