#pragma once
#include "vulkan/vulkan.h"
#include "XStruct.h"
#include "XUtils.h"
#include <string>
#include <iostream>

namespace Xavier
{

	class XShader {
	public:
		XShader() :
			stageFlag(VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM),
			shaderModule(VK_NULL_HANDLE),
			byteData()
		{ }

		static XRenderParas *xParams;

		bool loadShader(const std::string& codeFilePath, VkShaderStageFlagBits stage);
		
		VkPipelineShaderStageCreateInfo getCreateInfo();

		void clear();
		
	private:
		VkShaderStageFlagBits stageFlag;
		VkShaderModule shaderModule;
		std::vector<char> byteData;
	};
}