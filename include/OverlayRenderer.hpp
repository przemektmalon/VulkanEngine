#pragma once
#include "PCH.hpp"
#include "OverlayElement.hpp"
#include "Texture.hpp"
#include "vdu/DeviceMemory.hpp"
#include "Model.hpp"
#include "vdu/CommandBuffer.hpp"
#include "vdu/Framebuffer.hpp"
#include "vdu/Pipeline.hpp"

class OLayer
{
	friend class OverlayRenderer;
public:

	OLayer() : render(true), needsUpdate(true) {}

	void create(glm::ivec2 resolution);
	void cleanup();

	void addElement(OverlayElement* el);
	OverlayElement* getElement(std::string pName);
	void removeElement(OverlayElement* el);

	void cleanupElements();

	Texture* getColour() { return &colAttachment; }
	Texture* getDepth() { return &depAttachment; }
	const vdu::Framebuffer& getFramebuffer() { return framebuffer; }

	void setPosition(glm::ivec2 pPos);
	void setResolution(glm::ivec2 pRes);

	void updateVerts();
	bool needsDrawUpdate();
	void setUpdated();

	bool doDraw() { return render; }
	void setDoDraw(bool doDraw) { 
		render = doDraw;
		needsUpdate = true; }

private:

	void sortByDepths()
	{
		std::sort(elements.begin(), elements.end(), compareOverlayElements);
	}

	std::vector<OverlayElement*> elements;
	std::vector<OverlayElement*> elementsToRemove;
	std::unordered_map<std::string, OverlayElement*> elementLabels;

	vdu::Framebuffer framebuffer;
	Texture colAttachment;
	Texture depAttachment;

	glm::fvec2 position;
	glm::fvec2 resolution;
	float depth;
	
	bool render;
	bool needsUpdate;

	vdu::DescriptorSet imageDescriptor;

	vdu::Buffer quadBuffer; // For the to-screen pass
	std::vector<VertexNoNormal> quadVerts;
};

class OverlayRenderer
{
	friend class Renderer;
	friend class Console;
public:

	void createOverlayRenderPass();
	void createOverlayPipeline();
	void createOverlayAttachments();
	void createOverlayFramebuffer();
	void createOverlayDescriptorSetLayouts();
	void createOverlayDescriptorSets();
	void createOverlayCommands();

	void updateOverlayCommands();
	
	void destroyOverlayRenderPass();
	void destroyOverlayPipeline();
	void destroyOverlayAttachmentsFramebuffers();
	void destroyOverlayDescriptorSetLayouts();
	void destroyOverlayCommands();

	void cleanup();
	void cleanupLayerElements();
	void cleanupForReInit();

	void addLayer(OLayer* layer);
	void removeLayer(OLayer* layer);

	const vdu::RenderPass& getRenderPass() { return overlayRenderPass; }
	vdu::DescriptorSetLayout& getDescriptorSetLayout() { return overlayDescriptorSetLayout; }

private:

	std::set<OLayer*> layers;

	// For rendering and combining layers
	vdu::RenderPass overlayRenderPass;

	// Pipeline and command buffer for rendering elements
	vdu::GraphicsPipeline elementPipeline;
	vdu::PipelineLayout elementPipelineLayout;
	vdu::CommandBuffer elementCommandBuffer;

	// Pipeline and command buffer for rendering layers
	vdu::GraphicsPipeline combinePipeline;
	vdu::PipelineLayout combinePipelineLayout;
	vdu::CommandBuffer combineCommandBuffer;

	vdu::VertexInputState vertexInputState;

	// Framebuffer for combined layers
	vdu::Framebuffer framebuffer;
	Texture combinedLayers;
	Texture combinedLayersDepth;

	// Projection matrix for combining layers
	vdu::Buffer combineProjUBO;

	// Layout for rendering and combining elements (one texture sampler)
	vdu::DescriptorSetLayout overlayDescriptorSetLayout;
};