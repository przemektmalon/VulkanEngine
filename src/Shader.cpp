#include "PCH.hpp"
#include "Shader.hpp"
#include "File.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

ShaderModule::ShaderModule(Stage pStage) : stage(pStage)
{
	setIntStage(); // Set the (internal) shader stage type used by the compiler
}

ShaderModule::ShaderModule(Stage pStage, std::string pPath) : ShaderModule(pStage)
{
	load(pPath);
}

void ShaderModule::load(std::string pPath)
{
	path = pPath;
	determineLanguage();
	File file;
	if (!file.open(path))
		DBG_WARNING("Failed to open GLSL shader file: " << path);

	glslSource.clear();
	spirvSource.clear();
	if (language == GLSL)
	{
		glslSource.resize(file.getSize());
		file.readFile(&glslSource[0]);
	}
	else if (language == SPV)
	{
		spirvSource.resize(file.getSize());
		file.readFile(&spirvSource[0]);
	}
}

void ShaderModule::reload()
{
	load(path);
}

void ShaderModule::compile()
{
	if (language == GLSL)
	{
		shaderc::Compiler c;
		shaderc::CompileOptions o;
		o.SetAutoBindUniforms(true);
		o.AddMacroDefinition(stageMacro);
		o.SetLimit(shaderc_limit::shaderc_limit_max_combined_texture_image_units, 1024);
		auto res = c.CompileGlslToSpv(glslSource, intStage, path.c_str(), o);
		if (res.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			DBG_SEVERE("\n" << res.GetErrorMessage() << "\n");
		}
		spirvSource.assign(res.begin(), res.end());
	}
	if (spirvSource.size() == 0) {
		DBG_SEVERE("SPIRV source missing, cannot compile shader: " << path);
		return;
	}
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = spirvSource.size() * sizeof(int);
	createInfo.pCode = spirvSource.data();

	VK_CHECK_RESULT(vkCreateShaderModule(Engine::renderer->device, &createInfo, nullptr, &vkModule));
}

void ShaderModule::destroy()
{
	VK_VALIDATE(vkDestroyShaderModule(Engine::renderer->device, vkModule, 0));
}

void ShaderModule::setIntStage()
{
	switch (stage)
	{
	case Vertex:
		intStage = shaderc_shader_kind::shaderc_glsl_vertex_shader; stageMacro = "VERTEX"; return;
	case Fragment:
		intStage = shaderc_shader_kind::shaderc_glsl_fragment_shader; stageMacro = "FRAGMENT"; return;
	case Compute:
		intStage = shaderc_shader_kind::shaderc_glsl_compute_shader; stageMacro = "COMPUTE"; return;
	case Geometry:
		intStage = shaderc_shader_kind::shaderc_glsl_geometry_shader; stageMacro = "GEOMETRY"; return;
	case TessControl:
		intStage = shaderc_shader_kind::shaderc_glsl_tess_control_shader; stageMacro = "TESSCONTROL"; return;
	case TessEval:
		intStage = shaderc_shader_kind::shaderc_glsl_tess_evaluation_shader; stageMacro = "TESSEVAL"; return;
	}
}

void ShaderModule::determineLanguage()
{
	int i = 0;
	while (path[i] != '.')
	{
		++i;
		if (i > path.length())
			DBG_WARNING("Bad shader file name format: " << path);
	}
	std::string extension;
	extension.assign(&path[i + 1]);
	if (extension == "glsl" || extension == "GLSL")
		language = GLSL;
	else if (extension == "spv" || extension == "SPV")
		language = SPV;
	else
	{
		DBG_WARNING("Bad shader file extension: " << path << " - '.glsl' and 'spv' supported");
		language = UNKNOWN;
	}
}