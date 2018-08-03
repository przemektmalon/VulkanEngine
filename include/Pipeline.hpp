#pragma once
#include "PCH.hpp"
#include "vdu/VDU.hpp"

namespace vdu
{
	class Pipeline
	{
	public:

		void setDescriptorSetLayout(vdu::DescriptorSetLayout* descriptorLayout);
		void setDescriptorSet(vdu::DescriptorSet* descriptor);
		void setShaderProgram(vdu::ShaderProgram* shader);
		void setFramebuffer(vdu::Framebuffer* framebuffer);

		virtual void create(vdu::LogicalDevice* device) = 0;

	protected:

		VkPipeline m_pipeline;
		VkPipelineLayout m_layout;

		vdu::DescriptorSet* m_descriptorSet;

		vdu::ShaderProgram* m_shaderProgram;

		vdu::Framebuffer* m_framebuffer;

		vdu::LogicalDevice* m_logicalDevice;
	};

	class GraphicsPipeline : public Pipeline
	{
	public:

		void create(vdu::LogicalDevice* device);

	private:

		void createRenderPass();

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
		void createRenderPass();

		VkRenderPass m_renderPass;

		std::vector<VkFramebuffer> screenFramebuffers;
		std::vector<VkImage> swapChainImages;
		std::vector<VkImageView> swapChainImageViews;
	};
}