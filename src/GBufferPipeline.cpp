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
	
	/*tci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	tci.format = VK_FORMAT_R32_SFLOAT;
	tci.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	tci.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

	gBufferDepthLinearAttachment.loadToGPU(&tci);*/
}

void Renderer::createGBufferRenderPass()
{
	auto colourInfo = gBufferRenderPass.addColourAttachment(&gBufferColourAttachment, "colour");
	colourInfo->setInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED);
	colourInfo->setFinalLayout(VK_IMAGE_LAYOUT_GENERAL);
	colourInfo->setUsageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	auto normalInfo = gBufferRenderPass.addColourAttachment(&gBufferNormalAttachment, "normal");
	normalInfo->setInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED);
	normalInfo->setFinalLayout(VK_IMAGE_LAYOUT_GENERAL);
	normalInfo->setUsageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	auto pbrInfo = gBufferRenderPass.addColourAttachment(&gBufferPBRAttachment, "pbr");
	pbrInfo->setInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED);
	pbrInfo->setFinalLayout(VK_IMAGE_LAYOUT_GENERAL);
	pbrInfo->setUsageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	/*auto depthLinearInfo = gBufferRenderPass.addColourAttachment(&gBufferDepthLinearAttachment, "depthLinear");
	depthLinearInfo->setInitialLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	depthLinearInfo->setFinalLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	depthLinearInfo->setUsageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);*/

	auto depthInfo = gBufferRenderPass.setDepthAttachment(&gBufferDepthAttachment);
	depthInfo->setInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED);
	//depthInfo->setFinalLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
	depthInfo->setFinalLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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
	gBufferPipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
	gBufferPipeline.create(&logicalDevice);
}

void Renderer::createGBufferFramebuffers()
{
	gBufferFramebuffer.addAttachment(&gBufferColourAttachment, "gBuffer");
	gBufferFramebuffer.addAttachment(&gBufferNormalAttachment, "normal");
	gBufferFramebuffer.addAttachment(&gBufferPBRAttachment, "pbr");
	//gBufferFramebuffer.addAttachment(&gBufferDepthLinearAttachment, "depthLinear");
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
	gBufferCmdsNeedUpdate = true;

	Engine::console->postMessage("Updating gBuffer descriptor set!", glm::fvec3(0.1, 0.1, 0.9));

	auto updater = gBufferDescriptorSet.makeUpdater();

	auto cameraUpdate = updater->addBufferUpdate("camera");

	cameraUpdate->buffer = cameraUBO.getHandle();
	cameraUpdate->offset = 0;
	cameraUpdate->range = VK_WHOLE_SIZE;

	auto transformsUpdate = updater->addBufferUpdate("transforms");

	transformsUpdate->buffer = transformUBO.getHandle();
	transformsUpdate->offset = 0;
	transformsUpdate->range = 60000;

	auto texturesUpdate = updater->addImageUpdate("textures", 0, 1000);

	for (u32 i = 0; i < 1000; ++i)
	{
		texturesUpdate[i].sampler = textureSampler;
		texturesUpdate[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		texturesUpdate[i].imageView = Engine::assets.getTexture("blank")->getView();
	}

	gBufferDescriptorSet.submitUpdater(updater);
	gBufferDescriptorSet.destroyUpdater(updater);
}

void Renderer::createGBufferCommands()
{
	gBufferCommandBuffer.allocate(&logicalDevice, &commandPool);
}

void Renderer::updateGBufferCommands()
{
	// We need to update gBuffer commands when:
	//	A new model instance was added
	//	A gBuffer descriptor set was updated

	if (!gBufferCmdsNeedUpdate)
		return;

	gBufferCmdsNeedUpdate = false;

	//bufferFreeMutex.lock();
	//gBufferCommandBuffer.reset();
	//bufferFreeMutex.unlock();

	//createGBufferCommands();

	auto cmd = gBufferCommandBuffer.getHandle();

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
	//clearValues[3].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clearValues[3].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gBufferPipeline.getHandle());

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gBufferPipelineLayout.getHandle(), 0, 1, &gBufferDescriptorSet.getHandle(), 0, nullptr);

	VkBuffer vertexBuffers[] = { vertexIndexBuffer.getHandle() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(cmd, vertexIndexBuffer.getHandle(), INDEX_BUFFER_BASE, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexedIndirect(cmd, drawCmdBuffer.getHandle(), 0, Engine::world.instancesToDraw.size(), sizeof(VkDrawIndexedIndirectCommand));

	vkCmdEndRenderPass(cmd);

	queryPool.cmdTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, END_GBUFFER);

	VK_CHECK_RESULT(vkEndCommandBuffer(cmd));
}

void Renderer::destroyGBufferAttachments()
{
	gBufferColourAttachment.destroy();
	gBufferNormalAttachment.destroy();
	gBufferPBRAttachment.destroy();
	gBufferDepthLinearAttachment.destroy();
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
	gBufferCommandBuffer.free();
}
