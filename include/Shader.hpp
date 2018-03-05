#pragma once
#include "PCH.hpp"
#include "shaderc/shaderc.hpp"

class ShaderModule
{
public:
	enum Stage { Vertex, Fragment, Geometry, Compute, TessControl, TessEval };

private:
	Stage stage;
	shaderc_shader_kind intStage;
	std::string stageMacro;

	enum Language { GLSL, SPV, UNKNOWN } language;

	std::string path;
	std::string glslSource;
	std::vector<u32> spirvSource;

	VkShaderModule vkModule;
	
	std::string infoLog;
	std::string debugLog;

public:
	ShaderModule(Stage pStage);
	ShaderModule(Stage pStage, std::string path);

	VkShaderModule getVkModule() { return vkModule; }

	void load(std::string path);
	void compile();
	void createVulkanModule();

private:
	void setIntStage();
	void determineLanguage();
};