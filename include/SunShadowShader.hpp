#pragma once
#include "PCH.hpp"
#include "Shader.hpp"

class SunShadowShader : public ShaderProgram
{
public:
	SunShadowShader() : vertexModule(ShaderModule::Vertex, "/res/shaders/sunShadow.glsl"), fragmentModule(ShaderModule::Fragment, "/res/shaders/sunShadow.glsl") {}

	void compile()
	{
		vertexModule.compile();
		fragmentModule.compile();

		auto& inf2 = shaderStageCreateInfos[0];
		inf2.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		inf2.stage = VK_SHADER_STAGE_VERTEX_BIT;
		inf2.module = vertexModule.getVkModule();
		inf2.pName = "main";
		inf2.pNext = 0;
		inf2.flags = 0;
		inf2.pSpecializationInfo = 0;

		auto& inf3 = shaderStageCreateInfos[1];
		inf3.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		inf3.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		inf3.module = fragmentModule.getVkModule();
		inf3.pName = "main";
		inf3.pNext = 0;
		inf3.flags = 0;
		inf3.pSpecializationInfo = 0;
	}

	void reload()
	{
		vertexModule.reload();
		fragmentModule.reload();
	}

	void destroy()
	{
		vertexModule.destroy();
		fragmentModule.destroy();
	}

	VkPipelineShaderStageCreateInfo* const getShaderStageCreateInfos()
	{
		return shaderStageCreateInfos;
	}

	int getNumStages()
	{
		return 2;
	}

private:

	ShaderModule vertexModule;
	ShaderModule fragmentModule;

	VkPipelineShaderStageCreateInfo shaderStageCreateInfos[2];

};