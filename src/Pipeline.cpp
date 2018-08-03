#include "PCH.hpp"
#include "Pipeline.hpp"

void vdu::Pipeline::setDescriptorSetLayout(vdu::DescriptorSetLayout * descriptorLayout)
{
}

void vdu::Pipeline::setDescriptorSet(vdu::DescriptorSet * descriptor)
{
	m_descriptorSet = descriptor;
}

void vdu::Pipeline::setShaderProgram(vdu::ShaderProgram * shader)
{
	m_shaderProgram = shader;
}

void vdu::Pipeline::setFramebuffer(vdu::Framebuffer * framebuffer)
{
	m_framebuffer = framebuffer;
}

void vdu::GraphicsPipeline::create(vdu::LogicalDevice * device)
{
	m_logicalDevice = device;
	createRenderPass();
}

void vdu::GraphicsPipeline::createRenderPass()
{
}

void vdu::ComputePipeline::create(vdu::LogicalDevice * device)
{
	m_logicalDevice = device;
}

void vdu::PresentPipeline::createSwapChain()
{
}

void vdu::PresentPipeline::createRenderPass()
{
}

void vdu::PresentPipeline::create(vdu::LogicalDevice * device)
{
	m_logicalDevice = device;
	createRenderPass();
}
