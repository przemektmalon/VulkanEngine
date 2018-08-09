#include "PCH.hpp"
#include "Renderer.hpp"
#include "Profiler.hpp"

void Renderer::createGBufferAttachments()
{
	TextureCreateInfo tci;
	tci.width = renderResolution.width;
	tci.height = renderResolution.height;
	tci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	tci.format = VK_FORMAT_R8G8B8A8_UNORM;
	tci.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	tci.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

	gBufferColourAttachment.loadToGPU(&tci);
	gBufferPBRAttachment.loadToGPU(&tci);

	tci.format = VK_FORMAT_R32G32_SFLOAT;

	gBufferNormalAttachment.loadToGPU(&tci);

	tci.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	tci.format = Engine::physicalDevice->findOptimalDepthFormat();
	tci.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	tci.usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	gBufferDepthAttachment.loadToGPU(&tci);
}

void Renderer::createGBufferRenderPass()
{
	auto colourInfo = gBufferRenderPass.addColourAttachment(&gBufferColourAttachment, "colour");
	colourInfo->setInitialLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	colourInfo->setFinalLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	colourInfo->setUsageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	auto normalInfo = gBufferRenderPass.addColourAttachment(&gBufferNormalAttachment, "normal");
	normalInfo->setInitialLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	normalInfo->setFinalLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	normalInfo->setUsageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	auto pbrInfo = gBufferRenderPass.addColourAttachment(&gBufferPBRAttachment, "pbr");
	pbrInfo->setInitialLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	pbrInfo->setFinalLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	pbrInfo->setUsageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	auto depthInfo = gBufferRenderPass.setDepthAttachment(&gBufferDepthAttachment);
	depthInfo->setInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED);
	depthInfo->setFinalLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
	depthInfo->setUsageLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	gBufferRenderPass.create(&logicalDevice);
}

void Renderer::createGBufferDescriptorSetLayouts()
{
	auto& dsl = gBufferDescriptorSetLayout;

	dsl.addBinding("camera", vdu::DescriptorType::UniformBuffer, 0, 1, vdu::ShaderStage::Vertex | vdu::ShaderStage::Fragment);
	dsl.addBinding("transforms", vdu::DescriptorType::UniformBuffer, 1, 1, vdu::ShaderStage::Vertex);
	dsl.addBinding("textures", vdu::DescriptorType::CombinedImageSampler, 2, 1000, vdu::ShaderStage::Fragment);

	dsl.create(&logicalDevice);
}

void Renderer::createGBufferPipeline()
{
	gBufferPipelineLayout.addDescriptorSetLayout(&gBufferDescriptorSetLayout);
	gBufferPipelineLayout.create(&logicalDevice);

	gBufferPipeline.addViewport({ 0.f, 0.f, (float)renderResolution.width, (float)renderResolution.height, 0.f, 1.f }, { 0, 0, renderResolution.width, renderResolution.height });
	gBufferPipeline.setVertexInputState(&defaultVertexInputState);
	gBufferPipeline.setShaderProgram(&gBufferShader);
	gBufferPipeline.setPipelineLayout(&gBufferPipelineLayout);
	gBufferPipeline.setRenderPass(&gBufferRenderPass);
	gBufferPipeline.setMaxDepthBounds(Engine::maxDepth);
	gBufferPipeline.create(&logicalDevice);
}

void Renderer::createGBufferFramebuffers()
{
	gBufferFramebuffer.addAttachment(&gBufferColourAttachment, "gBuffer");
	gBufferFramebuffer.addAttachment(&gBufferNormalAttachment, "normal");
	gBufferFramebuffer.addAttachment(&gBufferPBRAttachment, "pbr");
	gBufferFramebuffer.addAttachment(&gBufferDepthAttachment, "depth");

	gBufferFramebuffer.create(&logicalDevice, &gBufferRenderPass);
}

void Renderer::createGBufferDescriptorSets()
{
	gBufferDescriptorSet.allocate(&logicalDevice, &gBufferDescriptorSetLayout, &descriptorPool);
}

void Renderer::updateGBufferDescriptorSets()
{
	if (!gBufferDescriptorSetNeedsUpdate)
		return;

	gBufferDescriptorSetNeedsUpdate = false;

	auto updater = gBufferDescriptorSet.makeUpdater();

	auto cameraUpdate = updater->addBufferUpdate("camera");

	cameraUpdate->buffer = cameraUBO.getHandle();
	cameraUpdate->offset = 0;
	cameraUpdate->range = VK_WHOLE_SIZE;

	auto transformsUpdate = updater->addBufferUpdate("transforms");

	transformsUpdate->buffer = transformUBO.getHandle();
	transformsUpdate->offset = 0;
	transformsUpdate->range = VK_WHOLE_SIZE;

	auto texturesUpdate = updater->addImageUpdate("textures", 0, 1000);

	for (u32 i = 0; i < 1000; ++i)
	{
		texturesUpdate[i].sampler = textureSampler;
		texturesUpdate[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		texturesUpdate[i].imageView = Engine::assets.materials.begin()->second.albedoSpec->getView();
	}

	u32 i = 0;
	for (auto& material : Engine::assets.materials)
	{
		if (material.second.albedoSpec->getHandle())
			texturesUpdate[i].imageView = material.second.albedoSpec->getView();
		else
			texturesUpdate[i].imageView = Engine::assets.getTexture("blank")->getView();

		if (material.second.normalRough)
		{
			if (material.second.normalRough->getView())
				texturesUpdate[i + 1].imageView = material.second.normalRough->getView();
			else
				texturesUpdate[i + 1].imageView = Engine::assets.getTexture("black")->getView();
		}
		else
			texturesUpdate[i + 1].imageView = Engine::assets.getTexture("black")->getView();
		i += 2;
	}

	gBufferDescriptorSet.submitUpdater(updater);
	gBufferDescriptorSet.destroyUpdater(updater);
}

void Renderer::createGBufferCommands()
{
	gBufferCommands.allocate(&logicalDevice, &commandPool);
}

void Renderer::updateGBufferCommands()
{
	//if (!gBufferCmdsNeedUpdate)
	//	return;

	//gBufferCmdsNeedUpdate = false;

	/// TODO: each thread will have a dynamic number of fences we will have to wait for all of them to signal (from the previous frame)

	//PROFILE_START("gbufferfence");
	//VK_CHECK_RESULT(vkWaitForFences(device, 1, &gBufferCommands.fence, true, std::numeric_limits<u64>::max()));
	//PROFILE_END("gbufferfence");

	bufferFreeMutex.lock();
	gBufferCommands.free();
	bufferFreeMutex.unlock();

	createGBufferCommands();

	auto cmd = gBufferCommands.getHandle();

	// From now on until we reset the gbuffer fence at the end of this function we cant submit a gBuffer command buffer

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &beginInfo));

	queryPool.cmdReset(cmd);
	queryPool.cmdTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, BEGIN_GBUFFER);

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = gBufferRenderPass.getHandle();
	renderPassInfo.framebuffer = gBufferFramebuffer.getHandle();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = renderResolution;

	std::array<VkClearValue, 4> clearValues = {};
	clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
	clearValues[1].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clearValues[2].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clearValues[3].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	VK_VALIDATE(vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE));

	VK_VALIDATE(vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gBufferPipeline.getHandle()));

	VK_VALIDATE(vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gBufferPipelineLayout.getHandle(), 0, 1, &gBufferDescriptorSet.getHandle(), 0, nullptr));

	VkBuffer vertexBuffers[] = { vertexIndexBuffer.getHandle() };
	VkDeviceSize offsets[] = { 0 };
	VK_VALIDATE(vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets));
	VK_VALIDATE(vkCmdBindIndexBuffer(cmd, vertexIndexBuffer.getHandle(), INDEX_BUFFER_BASE, VK_INDEX_TYPE_UINT32));

	VK_VALIDATE(vkCmdDrawIndexedIndirect(cmd, drawCmdBuffer.getHandle(), 0, Engine::world.instancesToDraw.size(), sizeof(VkDrawIndexedIndirectCommand)));

	VK_VALIDATE(vkCmdEndRenderPass(cmd));

	queryPool.cmdTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, END_GBUFFER);

	VK_CHECK_RESULT(vkEndCommandBuffer(cmd));

	//VK_CHECK_RESULT(vkResetFences(device, 1, &gBufferCommands.fence));
}

void Renderer::destroyGBufferAttachments()
{
	gBufferColourAttachment.destroy();
	gBufferNormalAttachment.destroy();
	gBufferPBRAttachment.destroy();
	gBufferDepthAttachment.destroy();
}

void Renderer::destroyGBufferRenderPass()
{
	gBufferRenderPass.destroy();
}

void Renderer::destroyGBufferDescriptorSetLayouts()
{
	gBufferDescriptorSetLayout.destroy();
}

void Renderer::destroyGBufferPipeline()
{
	gBufferPipelineLayout.destroy();
	gBufferPipeline.destroy();
	gBufferShader.destroy();
}

void Renderer::destroyGBufferFramebuffers()
{
	gBufferFramebuffer.destroy();
}

void Renderer::destroyGBufferDescriptorSets()
{
	gBufferDescriptorSet.free();
}

void Renderer::destroyGBufferCommands()
{
	gBufferCommands.free();
}
