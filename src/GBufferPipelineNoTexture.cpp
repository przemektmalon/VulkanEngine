#include "PCH.hpp"
#include "Renderer.hpp"
#include "Profiler.hpp"

void Renderer::createGBufferNoTexAttachments()
{

	// We need FOUR output textures:
		// 0) RGB colour (format = R8_G8_B8)
		// 1) Normal	 (format = R32_G32)
		// 2) PBR data	 (format = R8_G8)
		// 4) Depth		 (format = D32 or optimal provided by device)

	// Create these texture attachments so the shader can write to them

	/// Attachment (1) - RGB colour
	TextureCreateInfo tci;
	tci.width = renderResolution.width;
	tci.height = renderResolution.height;
	tci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	tci.format = VK_FORMAT_R8G8B8A8_UNORM;
	tci.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	tci.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	gBufferNoTexColourAttachment.loadToGPU(&tci);

	/// Attachment (2) - Normals
	tci.format = VK_FORMAT_R32G32_SFLOAT;
	gBufferNoTexNormalAttachment.loadToGPU(&tci);

	/// Attachment (3) - PBR data
	tci.format = VK_FORMAT_R8G8_UNORM;
	gBufferNoTexPBRAttachment.loadToGPU(&tci);

	/// Attachment (4) - Depth
	tci.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	tci.format = Engine::physicalDevice->findOptimalDepthFormat();
	tci.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	tci.usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	gBufferNoTexDepthAttachment.loadToGPU(&tci);
}

void Renderer::createGBufferNoTexRenderPass()
{
	auto colourInfo = gBufferNoTexRenderPass.addColourAttachment(&gBufferNoTexColourAttachment, "colour");
	colourInfo->setInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED);
	colourInfo->setFinalLayout(VK_IMAGE_LAYOUT_GENERAL);
	colourInfo->setUsageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	auto normalInfo = gBufferNoTexRenderPass.addColourAttachment(&gBufferNoTexNormalAttachment, "normal");
	normalInfo->setInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED);
	normalInfo->setFinalLayout(VK_IMAGE_LAYOUT_GENERAL);
	normalInfo->setUsageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	auto pbrInfo = gBufferNoTexRenderPass.addColourAttachment(&gBufferNoTexPBRAttachment, "pbr");
	pbrInfo->setInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED);
	pbrInfo->setFinalLayout(VK_IMAGE_LAYOUT_GENERAL);
	pbrInfo->setUsageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	auto depthInfo = gBufferNoTexRenderPass.setDepthAttachment(&gBufferNoTexDepthAttachment);
	depthInfo->setInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED);
	//depthInfo->setFinalLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
	depthInfo->setFinalLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	depthInfo->setUsageLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	gBufferNoTexRenderPass.create(&logicalDevice);
}

void Renderer::createGBufferNoTexDescriptorSetLayouts()
{
	auto& dsl = gBufferNoTexDescriptorSetLayout;

	dsl.addBinding("camera", vdu::DescriptorType::UniformBuffer, 0, 1, vdu::ShaderStage::Vertex | vdu::ShaderStage::Fragment);
	dsl.addBinding("transforms", vdu::DescriptorType::UniformBuffer, 1, 1, vdu::ShaderStage::Vertex);
	dsl.addBinding("pbrdata", vdu::DescriptorType::UniformBuffer, 2, 100, vdu::ShaderStage::Fragment);

	dsl.create(&logicalDevice);
}

void Renderer::createGBufferNoTexPipeline()
{
	gBufferNoTexPipelineLayout.addDescriptorSetLayout(&gBufferNoTexDescriptorSetLayout);
	gBufferNoTexPipelineLayout.create(&logicalDevice);

	gBufferNoTexPipeline.addViewport({ 0.f, 0.f, (float)renderResolution.width, (float)renderResolution.height, 0.f, 1.f }, { 0, 0, renderResolution.width, renderResolution.height });
	gBufferNoTexPipeline.setVertexInputState(&defaultVertexInputState);
	gBufferNoTexPipeline.setShaderProgram(&gBufferNoTexShader);
	gBufferNoTexPipeline.setPipelineLayout(&gBufferNoTexPipelineLayout);
	gBufferNoTexPipeline.setRenderPass(&gBufferNoTexRenderPass);
	gBufferNoTexPipeline.setMaxDepthBounds(Engine::maxDepth);
	gBufferNoTexPipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
	gBufferNoTexPipeline.create(&logicalDevice);
}

void Renderer::createGBufferNoTexFramebuffers()
{
	gBufferNoTexFramebuffer.addAttachment(&gBufferNoTexColourAttachment, "gBufferNoTex");
	gBufferNoTexFramebuffer.addAttachment(&gBufferNoTexNormalAttachment, "normal");
	gBufferNoTexFramebuffer.addAttachment(&gBufferNoTexPBRAttachment, "pbr");
	gBufferNoTexFramebuffer.addAttachment(&gBufferNoTexDepthAttachment, "depth");

	gBufferNoTexFramebuffer.create(&logicalDevice, &gBufferNoTexRenderPass);
}

void Renderer::createGBufferNoTexDescriptorSets()
{
	gBufferNoTexDescriptorSet.allocate(&logicalDevice, &gBufferNoTexDescriptorSetLayout, &descriptorPool);
}

void Renderer::updateGBufferNoTexDescriptorSets()
{
	if (!gBufferNoTexDescriptorSetNeedsUpdate)
		return;

	gBufferNoTexDescriptorSetNeedsUpdate = false;
	gBufferNoTexCmdsNeedUpdate = true;

	Engine::console->postMessage("Updating gBufferNoTex descriptor set!", glm::fvec3(0.1, 0.1, 0.9));

	auto updater = gBufferNoTexDescriptorSet.makeUpdater();

	auto cameraUpdate = updater->addBufferUpdate("camera");

	cameraUpdate->buffer = cameraUBO.getHandle();
	cameraUpdate->offset = 0;
	cameraUpdate->range = VK_WHOLE_SIZE;

	auto transformsUpdate = updater->addBufferUpdate("transforms");

	transformsUpdate->buffer = transformUBO.getHandle();
	transformsUpdate->offset = 0;
	transformsUpdate->range = 60000;


	updateFlatMaterialBuffer();

	auto pbrUpdate = updater->addBufferUpdate("pbrdata", 0);
	*pbrUpdate = { flatPBRUBO.getHandle(), 0, sizeof(glm::fvec4) * 2 * 100 };

	gBufferNoTexDescriptorSet.submitUpdater(updater);
	gBufferNoTexDescriptorSet.destroyUpdater(updater);
}

void Renderer::createGBufferNoTexCommands()
{
	gBufferNoTexCommandBuffer.allocate(&logicalDevice, &commandPool);
}

void Renderer::updateGBufferNoTexCommands()
{
	// We need to update gBufferNoTex commands when:
	//	A new model instance was added
	//	A gBufferNoTex descriptor set was updated

	if (!gBufferNoTexCmdsNeedUpdate)
		return;

	gBufferNoTexCmdsNeedUpdate = false;

	//bufferFreeMutex.lock();
	//gBufferNoTexCommandBuffer.reset();
	//bufferFreeMutex.unlock();

	//createGBufferNoTexCommands();

	auto cmd = gBufferNoTexCommandBuffer.getHandle();

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &beginInfo));

	queryPool.cmdReset(cmd);
	queryPool.cmdTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, BEGIN_GBUFFER);

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = gBufferNoTexRenderPass.getHandle();
	renderPassInfo.framebuffer = gBufferNoTexFramebuffer.getHandle();
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

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gBufferNoTexPipeline.getHandle());

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gBufferNoTexPipelineLayout.getHandle(), 0, 1, &gBufferNoTexDescriptorSet.getHandle(), 0, nullptr);

	VkBuffer vertexBuffers[] = { vertexIndexBuffer.getHandle() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(cmd, vertexIndexBuffer.getHandle(), INDEX_BUFFER_BASE, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexedIndirect(cmd, drawCmdBuffer.getHandle(), 0, Engine::world.instancesToDraw.size(), sizeof(VkDrawIndexedIndirectCommand));

	vkCmdEndRenderPass(cmd);

	queryPool.cmdTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, END_GBUFFER);

	VK_CHECK_RESULT(vkEndCommandBuffer(cmd));
}

void Renderer::destroyGBufferNoTexAttachments()
{
	gBufferNoTexColourAttachment.destroy();
	gBufferNoTexNormalAttachment.destroy();
	gBufferNoTexPBRAttachment.destroy();
	gBufferNoTexDepthAttachment.destroy();
}

void Renderer::destroyGBufferNoTexRenderPass()
{
	gBufferNoTexRenderPass.destroy();
}

void Renderer::destroyGBufferNoTexDescriptorSetLayouts()
{
	gBufferNoTexDescriptorSetLayout.destroy();
}

void Renderer::destroyGBufferNoTexPipeline()
{
	gBufferNoTexPipelineLayout.destroy();
	gBufferNoTexPipeline.destroy();
}

void Renderer::destroyGBufferNoTexFramebuffers()
{
	gBufferNoTexFramebuffer.destroy();
}

void Renderer::destroyGBufferNoTexDescriptorSets()
{
	gBufferNoTexDescriptorSet.free();
}

void Renderer::destroyGBufferNoTexCommands()
{
	gBufferNoTexCommandBuffer.free();
}
