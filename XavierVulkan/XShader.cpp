#include "XShader.h"


namespace Xavier
{
	XRenderParas* XShader::xParams = nullptr;

	bool XShader::loadShader(const std::string &codeFilePath, VkShaderStageFlagBits stage)
	{
		clear();

		byteData = utils::m_getFileBinaryContent(codeFilePath);
		stageFlag = stage;
		VkShaderModuleCreateInfo shaderModuleCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		shaderModuleCreateInfo.pCode = (uint32_t*)byteData.data();
		shaderModuleCreateInfo.codeSize = byteData.size();
		if (vkCreateShaderModule(XShader::xParams->xDevice, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			std::cout << "shader load file" << std::endl;
			return false;
		}
		return true;
	}

	VkPipelineShaderStageCreateInfo XShader::getCreateInfo()
	{
		VkPipelineShaderStageCreateInfo ci = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		ci.stage = stageFlag;
		ci.module = shaderModule;
		ci.pName = "main";
		ci.pSpecializationInfo = nullptr;
		return ci;
	}

	void XShader::clear()
	{
		if (shaderModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(XShader::xParams->xDevice, shaderModule, nullptr);
		}
		stageFlag = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		byteData.clear();
	}
}