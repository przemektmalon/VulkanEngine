#include "PCH.hpp"
#include "OverlayRenderer.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "Window.hpp"
#include "Threading.hpp"

void OLayer::create(glm::ivec2 pResolution)
{
	resolution = pResolution;
	position = glm::ivec2(0);
	depth = 0.5;

	auto r = Engine::renderer;

	TextureCreateInfo tci;
	tci.width = resolution.x;
	tci.height = resolution.y;
	tci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	tci.format = VK_FORMAT_R8G8B8A8_UNORM;
	tci.layout = VK_IMAGE_LAYOUT_GENERAL;
	tci.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	colAttachment.loadToGPU(&tci);

	tci.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	tci.format = Engine::physicalDevice->findOptimalDepthFormat();
	tci.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	tci.usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	depAttachment.loadToGPU(&tci);

	framebuffer.addAttachment(&colAttachment, "colour");
	framebuffer.addAttachment(&depAttachment, "depth");
	framebuffer.create(&Engine::renderer->logicalDevice, const_cast<vdu::RenderPass*>(&r->overlayRenderer.getRenderPass()));

	quadBuffer.setMemoryProperty(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	quadBuffer.setUsage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	quadBuffer.create(&r->logicalDevice, sizeof(VertexNoNormal) * 6);

	imageDescriptor.allocate(&r->logicalDevice, &r->overlayRenderer.getDescriptorSetLayout(), &r->descriptorPool);

	auto updater = imageDescriptor.makeUpdater();

	auto texUpdate = updater->addImageUpdate("texture");

	*texUpdate = { r->textureSampler, colAttachment.getView(), VK_IMAGE_LAYOUT_GENERAL };

	imageDescriptor.submitUpdater(updater);

	setPosition(glm::ivec2(0, 0));
}

void OLayer::cleanup()
{
	colAttachment.destroy();
	depAttachment.destroy();
	framebuffer.destroy();
	quadBuffer.destroy();
	for (auto element : elements)
	{
		element->cleanup();
		delete element;
	}
}

void OLayer::addElement(OverlayElement * el)
{
	Engine::threading->layersMutex.lock();

	auto find = std::find(elements.begin(), elements.end(), el);
	if (find == elements.end())
	{
		// No element with required shader exists, add vector for this type of shader
		elements.push_back(el);
	}
	else
	{
		// Elements already exists
		/// TODO: log error ?
	}

	auto nameFind = elementLabels.find(el->getName());
	if (nameFind == elementLabels.end())
		elementLabels.insert(std::make_pair(el->getName(), el));
	else
	{
		std::string newName = el->getName();
		newName += std::to_string(Engine::clock.now());
		nameFind = elementLabels.find(newName);
		u64 i = 1;
		while (nameFind != elementLabels.end())
		{
			newName = el->getName() + std::to_string(Engine::clock.now() + i);
			nameFind = elementLabels.find(newName);
		}
		elementLabels.insert(std::make_pair(newName, el));
	}

	Engine::threading->layersMutex.unlock();
}

OverlayElement * OLayer::getElement(std::string pName)
{
	auto find = elementLabels.find(pName);
	if (find == elementLabels.end())
	{
		DBG_WARNING("Layer element not found: " << pName);
		return 0;
	}
	return find->second;
}

void OLayer::removeElement(OverlayElement * el)
{
	Engine::threading->layersMutex.lock();
	elementsToRemove.push_back(el);
	el->setDoDraw(false);
	Engine::threading->layersMutex.unlock();
}

void OLayer::cleanupElements()
{
	Engine::threading->layersMutex.lock();

	for (auto elToRemove : elementsToRemove) // For each element to remove
	{
		// Find the iterator of the element to remove
		auto findElement = std::find(elements.begin(), elements.end(), elToRemove); 

		// If the element was found
		if (findElement != elements.end())
		{
			// Get pointer before iterator becamos invalid, so we can call the destructor
			auto ptr = *findElement;
			ptr->cleanup();
			elementLabels.erase((*findElement)->getName());
			elements.erase(findElement);
			delete ptr;
		}
	}

	elementsToRemove.clear();

	Engine::threading->layersMutex.unlock();
}

void OLayer::setPosition(glm::ivec2 pPos)
{
	position = pPos;
	updateVerts();
}

void OLayer::setResolution(glm::ivec2 pRes)
{
	colAttachment.destroy();
	depAttachment.destroy();
	framebuffer.destroy();
	quadBuffer.destroy();
	create(pRes);
	updateVerts();
}

void OLayer::updateVerts()
{
	quadVerts.clear();

	quadVerts.push_back(VertexNoNormal({ glm::fvec3(position,depth), glm::fvec2(0,0) }));
	quadVerts.push_back(VertexNoNormal({ glm::fvec3(position,depth) + glm::fvec3(0, resolution.y, 0), glm::fvec2(0,1) }));
	quadVerts.push_back(VertexNoNormal({ glm::fvec3(position,depth) + glm::fvec3(resolution.x, 0, 0), glm::fvec2(1,0) }));

	quadVerts.push_back(VertexNoNormal({ glm::fvec3(position,depth) + glm::fvec3(resolution.x, 0, 0), glm::fvec2(1,0) }));
	quadVerts.push_back(VertexNoNormal({ glm::fvec3(position,depth) + glm::fvec3(0, resolution.y, 0), glm::fvec2(0,1) }));
	quadVerts.push_back(VertexNoNormal({ glm::fvec3(position,depth) + glm::fvec3(resolution, 0), glm::fvec2(1,1) }));

	auto data = quadBuffer.getMemory()->map();
	memcpy(data, quadVerts.data(), sizeof(VertexNoNormal) * 6);
	quadBuffer.getMemory()->unmap();
}

bool OLayer::needsDrawUpdate()
{
	bool ret = needsUpdate;
	bool sortDepth = false;
	for (auto& el : elements)
	{
		sortDepth |= el->needsDepthUpdate();
		ret |= el->needsDrawUpdate() | sortDepth;
	}
	if (sortDepth)
		sortByDepths();
	needsUpdate = false;
	return ret;
}

void OLayer::setUpdated()
{
	for (auto& el : elements)
	{
		el->setUpdated();
	}
}

void OverlayRenderer::cleanup()
{
	destroyOverlayRenderPass();
	destroyOverlayPipeline();
	destroyOverlayAttachmentsFramebuffers();
	destroyOverlayDescriptorSetLayouts();
	combineProjUBO.destroy();

	for (auto layer : layers)
		layer->cleanup();

	layers.empty();
}

void OverlayRenderer::cleanupLayerElements()
{
	for (auto layer : layers)
	{
		layer->cleanupElements();
	}
}

void OverlayRenderer::cleanupForReInit()
{
	destroyOverlayRenderPass();
	destroyOverlayPipeline();
	destroyOverlayAttachmentsFramebuffers();
	//destroyOverlayDescriptorSets();
	destroyOverlayCommands();
}

void OverlayRenderer::addLayer(OLayer * layer)
{
	Engine::threading->layersMutex.lock();
	layers.insert(layer);
	Engine::threading->layersMutex.unlock();
}

void OverlayRenderer::removeLayer(OLayer * layer)
{
	Engine::threading->layersMutex.lock();
	layers.erase(layer);
	Engine::threading->layersMutex.unlock();
}

void OverlayRenderer::createOverlayAttachments()
{
	TextureCreateInfo tci;
	tci.width = Engine::renderer->renderResolution.width;
	tci.height = Engine::renderer->renderResolution.height;
	tci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	tci.format = VK_FORMAT_R8G8B8A8_UNORM;
	tci.layout = VK_IMAGE_LAYOUT_GENERAL;
	tci.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	combinedLayers.loadToGPU(&tci);

	tci.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	tci.format = Engine::physicalDevice->findOptimalDepthFormat();
	tci.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	tci.usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	combinedLayersDepth.loadToGPU(&tci);
}

void OverlayRenderer::createOverlayFramebuffer()
{
	framebuffer.addAttachment(&combinedLayers, "combined");
	framebuffer.addAttachment(&combinedLayersDepth, "combinedDepth");
	framebuffer.create(&Engine::renderer->logicalDevice, &overlayRenderPass);
}

void OverlayRenderer::createOverlayRenderPass()
{
	auto combinedInfo = overlayRenderPass.addColourAttachment(&combinedLayers, "combined");
	combinedInfo->setInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED);
	combinedInfo->setFinalLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	combinedInfo->setUsageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	auto depthInfo = overlayRenderPass.setDepthAttachment(&combinedLayersDepth);
	depthInfo->setInitialLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	depthInfo->setFinalLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	depthInfo->setUsageLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	overlayRenderPass.create(&Engine::renderer->logicalDevice);
}

void OverlayRenderer::createOverlayPipeline()
{
	{
		elementPipelineLayout.addPushConstantRange({ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::fmat4) + sizeof(glm::fvec4) });
		elementPipelineLayout.addPushConstantRange({ VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::fmat4) + sizeof(glm::fvec4), sizeof(glm::fvec4) + sizeof(int) });
		elementPipelineLayout.addDescriptorSetLayout(&overlayDescriptorSetLayout);
		elementPipelineLayout.create(&Engine::renderer->logicalDevice);

		VkPipelineColorBlendAttachmentState colourBlendAttachment = {};
		colourBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colourBlendAttachment.blendEnable = VK_TRUE;
		colourBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colourBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colourBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colourBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colourBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colourBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		elementPipeline.setAttachmentColorBlendState("combined", colourBlendAttachment);

		elementPipeline.addViewport({ 0.f, 0.f, (float)1280, (float)720, 0.f, 1.f }, { 0, 0, 1280, 720 });
		elementPipeline.setVertexInputState(&Engine::renderer->screenVertexInputState);
		elementPipeline.setShaderProgram(&Engine::renderer->overlayShader);
		elementPipeline.setPipelineLayout(&elementPipelineLayout);
		elementPipeline.setRenderPass(&overlayRenderPass);
		elementPipeline.addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
		elementPipeline.addDynamicState(VK_DYNAMIC_STATE_SCISSOR);
		elementPipeline.setDepthTest(VK_FALSE);
		elementPipeline.create(&Engine::renderer->logicalDevice);
	}

	{
		combinePipelineLayout.addPushConstantRange({ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::fmat4) });
		combinePipelineLayout.addDescriptorSetLayout(&overlayDescriptorSetLayout);
		combinePipelineLayout.create(&Engine::renderer->logicalDevice);

		VkPipelineColorBlendAttachmentState colourBlendAttachment = {};
		colourBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colourBlendAttachment.blendEnable = VK_TRUE;
		colourBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colourBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colourBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colourBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colourBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colourBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		combinePipeline.setAttachmentColorBlendState("combined", colourBlendAttachment);

		combinePipeline.addViewport({ 0.f, 0.f, (float)Engine::renderer->renderResolution.width, (float)Engine::renderer->renderResolution.height, 0.f, 1.f }, { 0, 0, Engine::renderer->renderResolution.width, Engine::renderer->renderResolution.height });
		combinePipeline.setVertexInputState(&vertexInputState);
		combinePipeline.setShaderProgram(&Engine::renderer->combineOverlaysShader);
		combinePipeline.setPipelineLayout(&combinePipelineLayout);
		combinePipeline.setRenderPass(&overlayRenderPass);
		combinePipeline.setDepthTest(VK_FALSE);
		combinePipeline.setCullMode(VK_CULL_MODE_NONE);
		combinePipeline.setMaxDepthBounds(10.f);
		combinePipeline.create(&Engine::renderer->logicalDevice);
	}
}

void OverlayRenderer::createOverlayDescriptorSetLayouts()
{
	overlayDescriptorSetLayout.addBinding("texture", vdu::DescriptorType::CombinedImageSampler, 0, 1, vdu::ShaderStage::Fragment);
	overlayDescriptorSetLayout.create(&Engine::renderer->logicalDevice);
}

void OverlayRenderer::createOverlayDescriptorSets()
{
	combineProjUBO.setMemoryProperty(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	combineProjUBO.setUsage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	combineProjUBO.create(&Engine::renderer->logicalDevice, sizeof(glm::fmat4));
	glm::fmat4 p = glm::ortho<float>(0, 1280, 720, 0, -10, 10);
	glm::mat4 clip(1.0f, 0.0f, 0.0f, 0.0f,
		+0.0f, -1.0f, 0.0f, 0.0f,
		+0.0f, 0.0f, 0.5f, 0.0f,
		+0.0f, 0.0f, 0.5f, 1.0f);
	p = clip * p;
	auto data = combineProjUBO.getMemory()->map();
	memcpy(data, &p[0][0], sizeof(glm::fmat4));
	combineProjUBO.getMemory()->unmap();
}

void OverlayRenderer::createOverlayCommands()
{
	elementCommandBuffer.allocate(&Engine::renderer->logicalDevice, &Engine::renderer->commandPool);
	combineCommandBuffer.allocate(&Engine::renderer->logicalDevice, &Engine::renderer->commandPool);
}

void OverlayRenderer::updateOverlayCommands()
{
	/// TODO: if we have a layer that updates every frame, we re-write all commands
	/// Can we avoid this by using secondary command buffers for layers

	Engine::threading->layersMutex.lock();

	bool update = false;
	for (auto l : layers)
	{
		update |= l->needsDrawUpdate();
		l->setUpdated();
	}

	if (!update)
	{
		Engine::threading->layersMutex.unlock();
		return;
	}

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	auto cmd = elementCommandBuffer.getHandle();

	VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &beginInfo));

	Engine::renderer->queryPool.cmdTimestamp(elementCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, Renderer::BEGIN_OVERLAY);

	for (auto layer : layers)
	{
		if (!layer->doDraw())
			continue;

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = overlayRenderPass.getHandle();
		renderPassInfo.framebuffer = layer->framebuffer.getHandle();
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent.width = layer->resolution.x;
		renderPassInfo.renderArea.extent.height = layer->resolution.y;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0, 0, 0, 0 };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, elementPipeline.getHandle());

		auto& elements = layer->elements;

		VkRect2D scissor = { 0, 0, layer->resolution.x, layer->resolution.y };
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)layer->resolution.x;
		viewport.height = (float)layer->resolution.y;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		for (auto element : elements)
		{
			if (!element->doDraw())
				continue;

			float push[20];
			glm::fmat4 proj = glm::ortho<float>(0, layer->resolution.x, 0, layer->resolution.y, -10, 10);
			memcpy(push, &proj[0][0], sizeof(glm::fmat4));
			float depth = element->getDepth();
			memcpy(push + 16, &depth, sizeof(float));
			vkCmdPushConstants(cmd, elementPipelineLayout.getHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::fmat4) + sizeof(glm::fvec4), push);

			float push2[5];
			glm::fvec4 c = *(glm::fvec4*)element->getPushConstData();
			memcpy(push2, &c[0], sizeof(glm::fvec4));
			int t = element->getType();
			memcpy(push2 + 4, &t, sizeof(int));
			vkCmdPushConstants(cmd, elementPipelineLayout.getHandle(), VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::fmat4) + sizeof(glm::fvec4), sizeof(glm::fvec4) + sizeof(int), push2);

			VkDescriptorSet descSet = element->getDescriptorSet();
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, elementPipelineLayout.getHandle(), 0, 1, &descSet, 0, nullptr);

			element->render(cmd);
		}

		vkCmdEndRenderPass(cmd);
	}

	Engine::renderer->queryPool.cmdTimestamp(elementCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, Renderer::END_OVERLAY);

	VK_CHECK_RESULT(vkEndCommandBuffer(cmd));

	cmd = combineCommandBuffer.getHandle();

	VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &beginInfo));

	Engine::renderer->queryPool.cmdTimestamp(combineCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, Renderer::BEGIN_OVERLAY_COMBINE);

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = overlayRenderPass.getHandle();
	renderPassInfo.framebuffer = framebuffer.getHandle();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = Engine::renderer->renderResolution.width;
	renderPassInfo.renderArea.extent.height = Engine::renderer->renderResolution.height;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0, 0, 0, 0 };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, combinePipeline.getHandle());

	glm::fmat4 proj = glm::ortho<float>(0, Engine::window->resX, Engine::window->resY, 0, -10, 10);
	glm::mat4 clip(1.0f, 0.0f, 0.0f, 0.0f,
		+0.0f, -1.0f, 0.0f, 0.0f,
		+0.0f, 0.0f, 0.5f, 0.0f,
		+0.0f, 0.0f, 0.5f, 1.0f);
	proj = clip * proj;
	vkCmdPushConstants(cmd, combinePipelineLayout.getHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::fmat4), &proj);

	for (auto layer : layers)
	{
		if (!layer->doDraw())
			continue;

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, combinePipelineLayout.getHandle(), 0, 1, &layer->imageDescriptor.getHandle(), 0, nullptr);

		VkBuffer vertexBuffers[] = { layer->quadBuffer.getHandle() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

		vkCmdDraw(cmd, 6, 1, 0, 0);
	}

	vkCmdEndRenderPass(cmd);

	Engine::renderer->queryPool.cmdTimestamp(combineCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, Renderer::END_OVERLAY_COMBINE);

	VK_CHECK_RESULT(vkEndCommandBuffer(cmd));

	Engine::threading->layersMutex.unlock();
}

void OverlayRenderer::destroyOverlayCommands()
{
	elementCommandBuffer.free();
	combineCommandBuffer.free();
}

void OverlayRenderer::destroyOverlayDescriptorSetLayouts()
{
	overlayDescriptorSetLayout.destroy();
}

void OverlayRenderer::destroyOverlayRenderPass()
{
	overlayRenderPass.destroy();
}

void OverlayRenderer::destroyOverlayPipeline()
{
	combinePipelineLayout.destroy();
	combinePipeline.destroy();
	
	elementPipelineLayout.destroy();
	elementPipeline.destroy();
}

void OverlayRenderer::destroyOverlayAttachmentsFramebuffers()
{
	combinedLayers.destroy();
	combinedLayersDepth.destroy();
	framebuffer.destroy();
}
