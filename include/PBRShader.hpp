#pragma once
#include "PCH.hpp"
#include "Shader.hpp"

class PBRShader : public ShaderProgram
{
public:
	PBRShader() : computeModule(ShaderModule::Compute, "/res/shaders/pbr.glsl") {}

	void compile()
	{
		computeModule.compile();

		auto& inf1 = shaderStageCreateInfos[0];
		inf1.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		inf1.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		inf1.module = computeModule.getVkModule();
		inf1.pName = "main";
		inf1.pNext = 0;
		inf1.flags = 0;
		inf1.pSpecializationInfo = 0;
	}

	void reload()
	{
		computeModule.reload();
	}

	void destroy()
	{
		computeModule.destroy();
	}

	VkPipelineShaderStageCreateInfo* const getShaderStageCreateInfos()
	{
		return shaderStageCreateInfos;
	}

	int getNumStages()
	{
		return 1;
	}

private:

	ShaderModule computeModule;
	
	VkPipelineShaderStageCreateInfo shaderStageCreateInfos[1];

};