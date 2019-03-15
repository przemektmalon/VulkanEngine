#pragma once
#include "PCH.hpp"
#include "UIElement.hpp"
#include "Texture.hpp"
#include "Model.hpp"
#include "UIElementGroup.hpp"

class UIRenderer
{
	friend class Renderer;
	friend class Console;
public:

	void createOverlayRenderPass();
	void createOverlayPipeline();
	void createOverlayAttachments();
	void createOverlayFramebuffer();
	void createOverlayDescriptorSetLayouts();
	void createOverlayCommands();

	void updateOverlayCommands();
	
	void destroyOverlayRenderPass();
	void destroyOverlayPipeline();
	void destroyOverlayAttachmentsFramebuffers();
	void destroyOverlayDescriptorSetLayouts();
	void destroyOverlayCommands();

	void cleanup();
	void cleanupForReInit();

	void garbageCollect();

	UIElementGroup* createUIGroup(const std::string& groupName);
	UIElementGroup* getUIGroup(const std::string& groupName);
	void deleteUIGroup(UIElementGroup* group);
	void deleteUIGroup(const std::string& groupName);

	const vdu::RenderPass& getRenderPass() { return overlayRenderPass; }
	vdu::DescriptorSetLayout& getDescriptorSetLayout() { return overlayDescriptorSetLayout; }

private:

	std::vector<UIElementGroup*> uiGroups;

	vdu::RenderPass overlayRenderPass;

	vdu::GraphicsPipeline pipeline;
	vdu::PipelineLayout pipelineLayout;
	vdu::CommandBuffer commandBuffer;

	vdu::VertexInputState vertexInputState;

	vdu::Framebuffer framebuffer;
	Texture uiTexture;
	Texture uiDepthTexture;

	vdu::DescriptorSetLayout overlayDescriptorSetLayout;
};