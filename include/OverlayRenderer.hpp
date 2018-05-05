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
	ShaderProgram* shader;
};

class OLayer
{
	friend class OverlayRenderer;
public:

	void create(glm::ivec2 resolution);
	void cleanup();

	void addElement(OverlayElement* el);
	void removeElement(OverlayElement* el)
	{
		/// TODO: implement
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
		for (auto& map : elements)
			for (auto& el : map.second)
				ret |= el->needsDrawUpdate();
		return ret;
	}

private:

	std::unordered_map<ShaderProgram*, std::vector<OverlayElement*>> elements;

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
		destroyOverlayRenderPass();
		destroyOverlayPipeline();
		destroyOverlayAttachmentsFramebuffers();
		destroyOverlayDescriptorSetLayouts();
		//destroyOverlayDescriptorSets();
		destroyOverlayCommands();

		projUBO.destroy();

		for (auto pipe : pipelines)
			pipe.first->destroy();

		for (auto layer : layers)
			layer->cleanup();
	}

	void addLayer(OLayer* layer)
	{
		for (auto& shader : layer->elements)
		{
			auto pipeFound = pipelines.end();
			for (auto pipe = pipelines.begin(); pipe != pipelines.end(); ++pipe)
			{
				auto pipeReqs = pipe->first->requirements;
				if (pipeReqs.shader == shader.first &&
					pipeReqs.width == layer->resolution.x &&
					pipeReqs.height == layer->resolution.y)
				{
					pipeFound = pipe;
					break;
				}
			}
			if (pipeFound != pipelines.end())
			{
				pipeFound->second.insert(std::make_pair(layer,&shader.second));
			}
			else
			{
				auto& insertPipe = pipelines.insert(std::make_pair(new Pipeline(), std::unordered_map<OLayer*, std::vector<OverlayElement*>*>()));
				insertPipe.first->second.insert(std::make_pair(layer, &shader.second));
				PipelineRequirements reqs;
				reqs.width = layer->resolution.x;
				reqs.height = layer->resolution.y;
				reqs.shader = shader.first;
				insertPipe.first->first->create(reqs);
			}
			layers.insert(layer);
		}
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

	std::unordered_map<Pipeline*, std::unordered_map<OLayer*,std::vector<OverlayElement*>*>> pipelines;
	std::set<OLayer*> layers;
	
	VkDescriptorSetLayout overlayDescriptorSetLayout;
	VkRenderPass overlayRenderPass;
	VkCommandBuffer overlayCommandBuffer;
};