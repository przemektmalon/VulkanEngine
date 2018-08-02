#pragma once
#include "PCH.hpp"
#include "vdu/VDU.hpp"

class Pipeline
{
public:

	void setDescriptorSet(vdu::DescriptorSet* descriptor);
	void addTextureAttachment(vdu::Texture* texture);
	void setShaderProgram(vdu::ShaderProgram* shader);

	virtual void create(vdu::LogicalDevice* device) = 0;

protected:

	VkPipeline m_pipeline;
	VkPipelineLayout m_layout;

	std::vector<vdu::Texture*> m_attachments;

	vdu::DescriptorSet* m_descriptorSet;

	vdu::ShaderProgram* m_shaderProgram;

	vdu::LogicalDevice* m_logicalDevice;
};

class GraphicsPipeline : public Pipeline
{
public:

	void create(vdu::LogicalDevice* device);

private:

	void createFramebuffer();
	void createRenderPass();

	VkFramebuffer m_framebuffer;
	VkRenderPass m_renderPass;
};

class ComputePipeline : public Pipeline
{
public:

	void create(vdu::LogicalDevice* device);

private:

};

class PresentPipeline : public Pipeline
{
public:

	

	void create(vdu::LogicalDevice* device);

private:

	void createSwapChain();
	void createFramebuffer();
	void createRenderPass();

	VkFramebuffer m_framebuffer;
	VkRenderPass m_renderPass;

	std::vector<VkFramebuffer> screenFramebuffers;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
};