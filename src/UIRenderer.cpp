#include "PCH.hpp"
#include "UIRenderer.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "Window.hpp"
#include "Threading.hpp"

void UIRenderer::cleanup()
{
	destroyOverlayRenderPass();
	destroyOverlayPipeline();
	destroyOverlayAttachmentsFramebuffers();
	destroyOverlayDescriptorSetLayouts();

	/*for (auto layer : layers)
		layer->cleanup();

	layers.empty();*/

	//for (auto group : uiGroups)
		//group->cleanupElements

}

void UIRenderer::cleanupLayerElements()
{
	//for (auto layer : layers)
	//{
	//	layer->cleanupElements();
	//}
}

void UIRenderer::cleanupForReInit()
{
	destroyOverlayRenderPass();
	destroyOverlayPipeline();
	destroyOverlayAttachmentsFramebuffers();
	destroyOverlayCommands();
}

void UIRenderer::addUIGroup(UIElementGroup * group)
{
	Engine::threading->layersMutex.lock();
	uiGroups.insert(group);
	Engine::threading->layersMutex.unlock();
}

void UIRenderer::removeUIGroup(UIElementGroup * group)
{
	Engine::threading->layersMutex.lock();
	uiGroups.erase(group);
	Engine::threading->layersMutex.unlock();
}

void UIRenderer::createOverlayAttachments()
{
	TextureCreateInfo tci;
	tci.width = Engine::renderer->renderResolution.width;
	tci.height = Engine::renderer->renderResolution.height;
	tci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	tci.format = VK_FORMAT_R8G8B8A8_UNORM;
	tci.layout = VK_IMAGE_LAYOUT_GENERAL;
	tci.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	uiTexture.loadToGPU(&tci);

	tci.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	tci.format = Engine::physicalDevice->findOptimalDepthFormat();
	tci.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	tci.usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	uiDepthTexture.loadToGPU(&tci);
}

void UIRenderer::createOverlayFramebuffer()
{
	framebuffer.addAttachment(&uiTexture, "ui");
	framebuffer.addAttachment(&uiDepthTexture, "uidepth");
	framebuffer.create(&Engine::renderer->logicalDevice, &overlayRenderPass);
}

void UIRenderer::createOverlayRenderPass()
{
	auto combinedInfo = overlayRenderPass.addColourAttachment(&uiTexture, "ui");
	combinedInfo->setInitialLayout(VK_IMAGE_LAYOUT_GENERAL);
	combinedInfo->setFinalLayout(VK_IMAGE_LAYOUT_GENERAL);
	combinedInfo->setUsageLayout(VK_IMAGE_LAYOUT_GENERAL);

	auto depthInfo = overlayRenderPass.setDepthAttachment(&uiDepthTexture);
	depthInfo->setInitialLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	depthInfo->setFinalLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	depthInfo->setUsageLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	overlayRenderPass.create(&Engine::renderer->logicalDevice);
}

void UIRenderer::createOverlayPipeline()
{
	pipelineLayout.addPushConstantRange({ VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::fmat4) + (2*sizeof(glm::fvec4)) });
	pipelineLayout.addDescriptorSetLayout(&overlayDescriptorSetLayout);
	pipelineLayout.create(&Engine::renderer->logicalDevice);

	VkPipelineColorBlendAttachmentState colourBlendAttachment = {};
	colourBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colourBlendAttachment.blendEnable = VK_TRUE;
	colourBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colourBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colourBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colourBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colourBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colourBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	pipeline.setAttachmentColorBlendState("ui", colourBlendAttachment);

	pipeline.addViewport({ 0.f, 0.f, (float)Engine::renderer->renderResolution.width, (float)Engine::renderer->renderResolution.height, 0.f, 1.f }, { 0, 0, Engine::renderer->renderResolution.width, Engine::renderer->renderResolution.height });
	pipeline.setVertexInputState(&Engine::renderer->screenVertexInputState);
	pipeline.setShaderProgram(&Engine::renderer->overlayShader);
	pipeline.setPipelineLayout(&pipelineLayout);
	pipeline.setRenderPass(&overlayRenderPass);
	pipeline.setDepthTest(VK_FALSE);
	pipeline.setCullMode(VK_CULL_MODE_NONE);
	pipeline.setMaxDepthBounds(10.f);
	pipeline.addDynamicState(VK_DYNAMIC_STATE_SCISSOR);
	pipeline.create(&Engine::renderer->logicalDevice);
}

void UIRenderer::createOverlayDescriptorSetLayouts()
{
	overlayDescriptorSetLayout.addBinding("texture", vdu::DescriptorType::CombinedImageSampler, 0, 1, vdu::ShaderStage::Fragment);
	overlayDescriptorSetLayout.create(&Engine::renderer->logicalDevice);
}

void UIRenderer::createOverlayCommands()
{
	commandBuffer.allocate(&Engine::renderer->logicalDevice, &Engine::renderer->commandPool);
	commandBuffer.allocate(&Engine::renderer->logicalDevice, &Engine::renderer->commandPool);
}

void UIRenderer::updateOverlayCommands()
{
	/// TODO: if we have a layer that updates every frame, we re-write all commands
	/// Can we avoid this by using secondary command buffers for layers

	Engine::threading->layersMutex.lock();

	/// TODO:
	///		- Sort by depth
	///		- 

	/*bool update = false;
	for (auto l : layers)
	{
		update |= l->needsDrawUpdate();
		l->setUpdated();
	}

	if (!update)
	{
		Engine::threading->layersMutex.unlock();
		return;
	}*/

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	auto cmd = commandBuffer.getHandle();

	VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &beginInfo));

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

	Engine::renderer->queryPool.cmdTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, Renderer::BEGIN_OVERLAY);

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getHandle());

	for (auto group : uiGroups)
	{
		if (!group->doDraw())
			continue;

		auto& elements = group->elements;

		VkRect2D scissor = { 0, 0, group->resolution.x, group->resolution.y };
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		glm::fmat4 proj = glm::ortho<float>(0, Engine::window->resX, Engine::window->resY, 0, -10, 10);
		glm::mat4 clip(1.0f, 0.0f, 0.0f, 0.0f,
			+0.0f, -1.0f, 0.0f, 0.0f,
			+0.0f, 0.0f, 0.5f, 0.0f,
			+0.0f, 0.0f, 0.5f, 1.0f);
		proj = clip * proj;
		vkCmdPushConstants(cmd, pipelineLayout.getHandle(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::fmat4), &proj);

		for (auto element : elements)
		{
			if (!element->doDraw())
				continue;

			glm::fvec4 colour = element->getColour();
			vkCmdPushConstants(cmd, pipelineLayout.getHandle(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 64, sizeof(glm::fvec4), &colour);

			float depth = element->getDepth();
			vkCmdPushConstants(cmd, pipelineLayout.getHandle(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 80, sizeof(float), &depth);

			int type = element->getType();
			vkCmdPushConstants(cmd, pipelineLayout.getHandle(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 84, sizeof(int), &type);

			VkDescriptorSet descSet = element->getDescriptorSet().getHandle();
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout.getHandle(), 0, 1, &descSet, 0, nullptr);

			element->render(commandBuffer);
		}
	}

	vkCmdEndRenderPass(cmd);

	Engine::renderer->queryPool.cmdTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, Renderer::END_OVERLAY);

	VK_CHECK_RESULT(vkEndCommandBuffer(cmd));

	Engine::threading->layersMutex.unlock();
}

void UIRenderer::destroyOverlayCommands()
{
	commandBuffer.free();
}

void UIRenderer::destroyOverlayDescriptorSetLayouts()
{
	overlayDescriptorSetLayout.destroy();
}

void UIRenderer::destroyOverlayRenderPass()
{
	overlayRenderPass.destroy();
}

void UIRenderer::destroyOverlayPipeline()
{
	pipelineLayout.destroy();
	pipeline.destroy();
}

void UIRenderer::destroyOverlayAttachmentsFramebuffers()
{
	uiTexture.destroy();
	uiDepthTexture.destroy();
	framebuffer.destroy();
}
