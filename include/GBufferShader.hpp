#pragma once
#include "PCH.hpp"
#include "Shader.hpp"

class GBufferShader
{
public:
	GBufferShader() : vertexModule(ShaderModule::Vertex, "/res/shaders/gbuffer.glsl"), fragmentModule(ShaderModule::Fragment, "/res/shaders/gbuffer.glsl") {}

	void compile()
	{
		vertexModule.compile();
		fragmentModule.compile();

		auto& inf1 = shaderStageCreateInfos[0];
		inf1.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		inf1.stage = VK_SHADER_STAGE_VERTEX_BIT;
		inf1.module = vertexModule.getVkModule();
		inf1.pName = "main";
		inf1.pNext = 0;
		inf1.flags = 0;
		inf1.pSpecializationInfo = 0;

		auto& inf2 = shaderStageCreateInfos[1];
		inf2.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		inf2.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		inf2.module = fragmentModule.getVkModule();
		inf2.pName = "main";
		inf2.pNext = 0;
		inf2.flags = 0;
		inf2.pSpecializationInfo = 0;
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

private:

	ShaderModule vertexModule;
	ShaderModule fragmentModule;

	VkPipelineShaderStageCreateInfo shaderStageCreateInfos[2];
	
};