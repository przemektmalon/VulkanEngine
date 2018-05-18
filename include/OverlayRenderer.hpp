#pragma once
#include "PCH.hpp"
#include "Shader.hpp"
#include "OverlayElement.hpp"
#include "Texture.hpp"
#include "Buffer.hpp"
#include "Model.hpp"

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

	Texture* getColour() { return &colAttachment; }
	Texture* getDepth() { return &depAttachment; }
	VkFramebuffer getFramebuffer() { return framebuffer; }

	void setPosition(glm::ivec2 pPos);

	void updateVerts();
	bool needsDrawUpdate();
	void setUpdated();

	bool doDraw() { return render; }
	void setDoDraw(bool doDraw) { render = doDraw; needsUpdate = true; }

private:

	void sortByDepths()
	{
		std::sort(elements.begin(), elements.end(), compareOverlayElements);
	}

	std::vector<OverlayElement*> elements;
	std::unordered_map<std::string, OverlayElement*> elementLabels;

	VkFramebuffer framebuffer;
	Texture colAttachment;
	Texture depAttachment;

	glm::fvec2 position;
	glm::fvec2 resolution;
	float depth;
	
	bool render;
	bool needsUpdate;

	VkDescriptorSet imageDescriptor;

	Buffer quadBuffer; // For the to-screen pass
	std::vector<VertexNoNormal> quadVerts;
};

class OverlayRenderer
{
	friend class Renderer;
	friend class Console;
public:

	void createOverlayRenderPass();
	void createOverlayPipeline();
	void createOverlayAttachmentsFramebuffers();
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
	void cleanupForReInit();

	void addLayer(OLayer* layer);
	void removeLayer(OLayer* layer);

	VkRenderPass getRenderPass() { return overlayRenderPass; }
	VkDescriptorSetLayout getDescriptorSetLayout() { return overlayDescriptorSetLayout; }

private:

	std::set<OLayer*> layers;

	// For rendering and combining layers
	VkRenderPass overlayRenderPass;

	// Pipeline and command buffer for rendering elements
	VkPipeline elementPipeline;
	VkPipelineLayout elementPipelineLayout;
	VkCommandBuffer elementCommandBuffer;

	// Pipeline and command buffer for rendering layers
	VkPipeline combinePipeline;
	VkPipelineLayout combinePipelineLayout;
	VkCommandBuffer combineCommandBuffer;

	// Framebuffer for combined layers
	VkFramebuffer framebuffer;
	Texture combinedLayers;
	Texture combinedLayersDepth;

	// Projection matrix for combining layers
	Buffer combineProjUBO;

	// Layout for rendering and combining elements (one texture sampler)
	VkDescriptorSetLayout overlayDescriptorSetLayout;
};