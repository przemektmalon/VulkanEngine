#pragma once
#include "PCH.hpp"
#include "Shader.hpp"

class PointShadowShader
{
public:
	PointShadowShader() : geometryModule(ShaderModule::Geometry, "/res/shaders/pointShadow.glsl"), vertexModule(ShaderModule::Vertex, "/res/shaders/pointShadow.glsl"), fragmentModule(ShaderModule::Fragment, "/res/shaders/pointShadow.glsl") {}

	void compile()
	{
		geometryModule.compile();
		vertexModule.compile();
		fragmentModule.compile();

		auto& inf1 = shaderStageCreateInfos[0];
		inf1.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		inf1.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		inf1.module = geometryModule.getVkModule();
		inf1.pName = "main";
		inf1.pNext = 0;
		inf1.flags = 0;
		inf1.pSpecializationInfo = 0;

		auto& inf2 = shaderStageCreateInfos[1];
		inf2.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		inf2.stage = VK_SHADER_STAGE_VERTEX_BIT;
		inf2.module = vertexModule.getVkModule();
		inf2.pName = "main";
		inf2.pNext = 0;
		inf2.flags = 0;
		inf2.pSpecializationInfo = 0;

		auto& inf3 = shaderStageCreateInfos[2];
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
		geometryModule.reload();
		vertexModule.reload();
		fragmentModule.reload();
	}

	void destroy()
	{
		geometryModule.destroy();
		vertexModule.destroy();
		fragmentModule.destroy();
	}

	VkPipelineShaderStageCreateInfo* const getShaderStageCreateInfos()
	{
		return shaderStageCreateInfos;
	}

private:

	ShaderModule geometryModule;
	ShaderModule vertexModule;
	ShaderModule fragmentModule;

	VkPipelineShaderStageCreateInfo shaderStageCreateInfos[3];

};