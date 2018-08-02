#include "PCH.hpp"
#include "Pipeline.hpp"

void Pipeline::setDescriptorSet(vdu::DescriptorSet * descriptor)
{
	m_descriptorSet = descriptor;
}

void Pipeline::addTextureAttachment(vdu::Texture * texture)
{
	m_attachments.push_back(texture);
}

void Pipeline::setShaderProgram(vdu::ShaderProgram * shader)
{
	m_shaderProgram = shader;
}

void GraphicsPipeline::create(vdu::LogicalDevice * device)
{
	m_logicalDevice = device;
	createRenderPass();
	createFramebuffer();
}

void GraphicsPipeline::createFramebuffer()
{
}

void GraphicsPipeline::createRenderPass()
{
}

void ComputePipeline::create(vdu::LogicalDevice * device)
{
	m_logicalDevice = device;
}

void PresentPipeline::createSwapChain()
{
}

void PresentPipeline::createFramebuffer()
{
}

void PresentPipeline::createRenderPass()
{
}

void PresentPipeline::create(vdu::LogicalDevice * device)
{
	m_logicalDevice = device;
	createRenderPass();
	createFramebuffer();
}
