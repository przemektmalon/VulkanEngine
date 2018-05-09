#pragma once
#include "PCH.hpp"
#include "Shader.hpp"
#include "OverlayElement.hpp"
#include "Texture.hpp"
#include "Buffer.hpp"
#include "Model.hpp"

struct PipelineRequirements
{
	int width, height;
};

class OLayer
{
	friend class OverlayRenderer;
public:

	void create(glm::ivec2 resolution);
	void cleanup();

	void addElement(OverlayElement* el);
	OverlayElement* getElement(std::string pName)
	{
		auto find = elementLabels.find(pName);
		if (find == elementLabels.end())
		{
			DBG_WARNING("Layer element not found: " << pName);
			return 0;
		}
		return find->second;
	}
	void removeElement(OverlayElement* el)
	{
		for (auto itr = elements.begin(); itr != elements.end(); ++itr)
		{
			if (*itr == el)
				itr = elements.erase(itr);
		}
		elementLabels.erase(el->getName());
	}

	Texture* getColour() { return &colAttachment; }
	Texture* getDepth() { return &depAttachment; }
	VkFramebuffer getFramebuffer() { return framebuffer; }

	void setPosition(glm::ivec2 pPos)
	{
		position = pPos;
		updateVerts();
	}

	void updateVerts()
	{
		quadVerts.clear();

		quadVerts.push_back(VertexNoNormal({ glm::fvec3(position,depth), glm::fvec2(0,0) }));
		quadVerts.push_back(VertexNoNormal({ glm::fvec3(position,depth) + glm::fvec3(0, resolution.y, 0), glm::fvec2(0,1) }));
		quadVerts.push_back(VertexNoNormal({ glm::fvec3(position,depth) + glm::fvec3(resolution.x, 0, 0), glm::fvec2(1,0) }));

		quadVerts.push_back(VertexNoNormal({ glm::fvec3(position,depth) + glm::fvec3(resolution.x, 0, 0), glm::fvec2(1,0) }));
		quadVerts.push_back(VertexNoNormal({ glm::fvec3(position,depth) + glm::fvec3(0, resolution.y, 0), glm::fvec2(0,1) }));
		quadVerts.push_back(VertexNoNormal({ glm::fvec3(position,depth) + glm::fvec3(resolution, 0), glm::fvec2(1,1) }));

		auto data = quadBuffer.map();
		memcpy(data, quadVerts.data(), sizeof(VertexNoNormal) * 6);
		quadBuffer.unmap();
	}

	bool needsDrawUpdate()
	{
		bool ret = false;
		bool sortDepth = false;
		for (auto& el : elements)
		{
			sortDepth = el->needsDepthUpdate();
			ret |= el->needsDrawUpdate() | sortDepth;
		}
		if (sortDepth)
			sortByDepths();
		return ret;
	}

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
	glm::fvec2 resolution;
	glm::fvec2 position;
	float depth;
	
	VkDescriptorSet imageDescriptor;

	Buffer quadBuffer; // For the to-screen pass
	std::vector<VertexNoNormal> quadVerts;
};

struct Pipeline
{
public:

	void create(PipelineRequirements pipelineReqs);
	void destroy();

	PipelineRequirements requirements;
	VkPipelineLayout layout;
	VkPipeline pipeline;
};

class OverlayRenderer
{
	friend class Renderer;
public:

	void createOverlayRenderPass();
	void createOverlayPipeline();
	void createOverlayAttachmentsFramebuffers();
	void createOverlayDescriptorSetLayouts();
	void createOverlayDescriptorSets();
	void createOverlayCommands();

	void updateOverlayDescriptorSets();
	void updateOverlayCommands();
	
	void destroyOverlayRenderPass();
	void destroyOverlayPipeline();
	void destroyOverlayAttachmentsFramebuffers();
	void destroyOverlayDescriptorSetLayouts();
	void destroyOverlayDescriptorSets();
	void destroyOverlayCommands();

	void cleanup()
	{
		cleanupForReInit();
		destroyOverlayDescriptorSetLayouts();
		projUBO.destroy();

		for (auto l : layers)
			for (auto p : l.second)
				p.first->destroy();

		for (auto layer : layersSet)
			layer->cleanup();

		layers.empty();
		layersSet.empty();
	}

	void cleanupForReInit()
	{
		destroyOverlayRenderPass();
		destroyOverlayPipeline();
		destroyOverlayAttachmentsFramebuffers();
		//destroyOverlayDescriptorSets();
		destroyOverlayCommands();
	}

	void addLayer(OLayer* layer)
	{
		auto find = layers.find(layer);
		std::unordered_map<Pipeline*,std::vector<OverlayElement*>*>* pipeMap;
		if (find == layers.end())
		{
			// Layer not found
			// Insert layer and map of pipelines and corresponding element lists
			pipeMap = &layers.insert(std::make_pair(layer, std::unordered_map<Pipeline*, std::vector<OverlayElement*>*>())).first->second;

			// For our layer add required pipelines and their corresponding element lists
			auto pipeFound = pipeMap->end();
			for (auto pipe = pipeMap->begin(); pipe != pipeMap->end(); ++pipe)
			{
				auto pipeReqs = pipe->first->requirements;
				if (pipeReqs.width == layer->resolution.x &&
					pipeReqs.height == layer->resolution.y)
				{
					pipeFound = pipe;
					break;
				}
			}
			if (pipeFound != pipeMap->end())
			{
				/// TODO: Pipe exists, so elements added, we shouldnt get here, do we need this check or go straight to 'else'?
			}
			else
			{
				auto& insertPipe = pipeMap->insert(std::make_pair(new Pipeline(), &layer->elements));
				PipelineRequirements reqs;
				reqs.width = layer->resolution.x;
				reqs.height = layer->resolution.y;
				insertPipe.first->first->create(reqs);
			}
			layersSet.insert(layer);
		}
	}

	void removeLayer(OLayer* layer)
	{
		layers.erase(layer);
		layersSet.erase(layer);
	}

	VkRenderPass getRenderPass() { return overlayRenderPass; }
	VkDescriptorSetLayout getDescriptorSetLayout() { return overlayDescriptorSetLayout; }

private:

	VkPipeline overlayPipeline;
	VkPipelineLayout overlayPipelineLayout;

	VkDescriptorSetLayout overlayCombineDescriptorSetLayout;
	VkDescriptorSetLayout overlayCombineDescriptorSetProjLayout;
	VkDescriptorSet overlayCombineDescriptorSet;
	VkCommandBuffer overlayCombineCommandBuffer;

	VkFramebuffer framebuffer;
	Texture combinedLayers;
	Texture combinedLayersDepth;

	Buffer projUBO;

	std::unordered_map<OLayer*, std::unordered_map<Pipeline*,std::vector<OverlayElement*>*>> layers;
	std::set<OLayer*> layersSet;
	
	VkDescriptorSetLayout overlayDescriptorSetLayout;
	VkRenderPass overlayRenderPass;
	VkCommandBuffer overlayCommandBuffer;
};