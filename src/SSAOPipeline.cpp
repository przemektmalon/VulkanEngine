#include "PCH.hpp"
#include "Renderer.hpp"

void Renderer::createSSAOAttachments()
{
	TextureCreateInfo tci;
	tci.width = renderResolution.width;
	tci.height = renderResolution.height;
	tci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	tci.format = VK_FORMAT_R8G8B8A8_UNORM;
	tci.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	tci.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

	ssaoColourAttachment.loadToGPU(&tci);

	tci.width = renderResolution.width;
	tci.height = renderResolution.height;
	tci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	tci.format = VK_FORMAT_R8_UNORM;
	tci.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	tci.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

	ssaoBlurAttachment.loadToGPU(&tci);

	tci.width = renderResolution.width;
	tci.height = renderResolution.height;
	tci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	tci.format = VK_FORMAT_R8_UNORM;
	tci.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	tci.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

	ssaoFinalAttachment.loadToGPU(&tci);
}

void Renderer::createSSAORenderPass()
{
	auto ssaoInfo = ssaoRenderPass.addColourAttachment(&ssaoColourAttachment, "ssao");
	ssaoInfo->setInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED);
	ssaoInfo->setFinalLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	ssaoInfo->setUsageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	ssaoRenderPass.create(&logicalDevice);

	auto ssaoBlurInfo = ssaoBlurRenderPass.addColourAttachment(&ssaoBlurAttachment, "ssao");
	ssaoBlurInfo->setInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED);
	ssaoBlurInfo->setFinalLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	ssaoBlurInfo->setUsageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	ssaoBlurRenderPass.create(&logicalDevice);
}

void Renderer::createSSAODescriptorSetLayouts()
{
	auto& dsl = ssaoDescriptorSetLayout;

	dsl.addBinding("config", vdu::DescriptorType::UniformBuffer, 0, 1, vdu::ShaderStage::Fragment);
	dsl.addBinding("camera", vdu::DescriptorType::UniformBuffer, 1, 1, vdu::ShaderStage::Fragment);
	dsl.addBinding("depth", vdu::DescriptorType::CombinedImageSampler, 2, 1, vdu::ShaderStage::Fragment);
	dsl.create(&logicalDevice);

	auto& dslBlur = ssaoBlurDescriptorSetLayout;

	dslBlur.addBinding("source", vdu::DescriptorType::CombinedImageSampler, 0, 1, vdu::ShaderStage::Fragment);
	dslBlur.create(&logicalDevice);

	auto& dslFinal = ssaoFinalDescriptorSetLayout;

	dslFinal.addBinding("source", vdu::DescriptorType::CombinedImageSampler, 0, 1, vdu::ShaderStage::Fragment);
	dslFinal.create(&logicalDevice);
}

void Renderer::createSSAOPipeline()
{
	ssaoPipelineLayout.addDescriptorSetLayout(&ssaoDescriptorSetLayout);
	ssaoPipelineLayout.create(&logicalDevice);

	ssaoPipeline.addViewport({ 0.f, 0.f, (float)renderResolution.width, (float)renderResolution.height, 0.f, 1.f }, { 0, 0, renderResolution.width, renderResolution.height });
	ssaoPipeline.setVertexInputState(&ssaoVertexInputState);
	ssaoPipeline.setShaderProgram(&ssaoShader);
	ssaoPipeline.setPipelineLayout(&ssaoPipelineLayout);
	ssaoPipeline.setRenderPass(&ssaoRenderPass);
	ssaoPipeline.setMaxDepthBounds(Engine::maxDepth);
	ssaoPipeline.setCullMode(VK_CULL_MODE_FRONT_BIT);
	ssaoPipeline.create(&logicalDevice);

	ssaoBlurPipelineLayout.addDescriptorSetLayout(&ssaoBlurDescriptorSetLayout);
	ssaoBlurPipelineLayout.addPushConstantRange({ VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(glm::ivec2) });
	ssaoBlurPipelineLayout.create(&logicalDevice);

	ssaoBlurPipeline.addViewport({ 0.f, 0.f, (float)renderResolution.width, (float)renderResolution.height, 0.f, 1.f }, { 0, 0, renderResolution.width, renderResolution.height });
	ssaoBlurPipeline.setVertexInputState(&ssaoVertexInputState);
	ssaoBlurPipeline.setShaderProgram(&ssaoBlurShader);
	ssaoBlurPipeline.setPipelineLayout(&ssaoBlurPipelineLayout);
	ssaoBlurPipeline.setRenderPass(&ssaoBlurRenderPass);
	ssaoBlurPipeline.setMaxDepthBounds(Engine::maxDepth);
	ssaoBlurPipeline.setCullMode(VK_CULL_MODE_FRONT_BIT);
	ssaoBlurPipeline.create(&logicalDevice);
}

void Renderer::createSSAOFramebuffer()
{
	ssaoFramebuffer.addAttachment(&ssaoColourAttachment, "ssao");
	ssaoFramebuffer.create(&logicalDevice, &ssaoRenderPass);

	ssaoBlurFramebuffer.addAttachment(&ssaoBlurAttachment, "ssao");
	ssaoBlurFramebuffer.create(&logicalDevice, &ssaoBlurRenderPass);

	ssaoFinalFramebuffer.addAttachment(&ssaoFinalAttachment, "ssao");
	ssaoFinalFramebuffer.create(&logicalDevice, &ssaoBlurRenderPass);
}

void Renderer::createSSAODescriptorSets()
{
	ssaoDescriptorSet.allocate(&logicalDevice, &ssaoDescriptorSetLayout, &freeableDescriptorPool);

	ssaoBlurDescriptorSet.allocate(&logicalDevice, &ssaoBlurDescriptorSetLayout, &freeableDescriptorPool);

	ssaoFinalDescriptorSet.allocate(&logicalDevice, &ssaoFinalDescriptorSetLayout, &freeableDescriptorPool);
}

void Renderer::createSSAOCommands()
{
	ssaoCommandBuffer.allocate(&logicalDevice, &commandPool);
}

void Renderer::updateSSAODescriptorSets()
{
	Engine::console->postMessage("Updating ssao descriptor set!", glm::fvec3(0.1, 0.1, 0.9));

	auto updater = ssaoDescriptorSet.makeUpdater();

	auto configUpdate = updater->addBufferUpdate("config");

	configUpdate->buffer = ssaoConfigBuffer.getHandle();
	configUpdate->offset = 0;
	configUpdate->range = VK_WHOLE_SIZE;

	auto cameraUpdate = updater->addBufferUpdate("camera");

	cameraUpdate->buffer = cameraUBO.getHandle();
	cameraUpdate->offset = 0;
	cameraUpdate->range = VK_WHOLE_SIZE;

	auto depthUpdate = updater->addImageUpdate("depth");
	depthUpdate->imageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
	depthUpdate->imageView = gBufferDepthAttachment.getView();
	depthUpdate->sampler = shadowSampler;

	ssaoDescriptorSet.submitUpdater(updater);
	ssaoDescriptorSet.destroyUpdater(updater);

	updater = ssaoBlurDescriptorSet.makeUpdater();

	auto sourceUpdate = updater->addImageUpdate("source");

	sourceUpdate->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	sourceUpdate->imageView = ssaoColourAttachment.getView();
	sourceUpdate->sampler = textureSampler;

	ssaoBlurDescriptorSet.submitUpdater(updater);
	ssaoBlurDescriptorSet.destroyUpdater(updater);

	updater = ssaoFinalDescriptorSet.makeUpdater();

	sourceUpdate = updater->addImageUpdate("source");

	sourceUpdate->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	sourceUpdate->imageView = ssaoBlurAttachment.getView();
	sourceUpdate->sampler = textureSampler;

	ssaoFinalDescriptorSet.submitUpdater(updater);
	ssaoFinalDescriptorSet.destroyUpdater(updater);
}

void Renderer::updateSSAOCommands()
{
	auto cmd = ssaoCommandBuffer.getHandle();

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &beginInfo));

	queryPool.cmdTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, BEGIN_SSAO);

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = ssaoRenderPass.getHandle();
	renderPassInfo.framebuffer = ssaoFramebuffer.getHandle();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = renderResolution;

	std::array<VkClearValue, 1> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	VK_VALIDATE(vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE));

	VK_VALIDATE(vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ssaoPipeline.getHandle()));

	VK_VALIDATE(vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ssaoPipelineLayout.getHandle(), 0, 1, &ssaoDescriptorSet.getHandle(), 0, nullptr));

	VkBuffer vertexBuffers[] = { screenQuadBuffer.getHandle() };
	VkDeviceSize offsets[] = { 0 };
	VK_VALIDATE(vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets));

	VK_VALIDATE(vkCmdDraw(cmd, 6, 1, 0, 0));

	VK_VALIDATE(vkCmdEndRenderPass(cmd));

	renderPassInfo.renderPass = ssaoBlurRenderPass.getHandle();
	renderPassInfo.framebuffer = ssaoBlurFramebuffer.getHandle();

	VK_VALIDATE(vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE));

	VK_VALIDATE(vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ssaoBlurPipeline.getHandle()));

	VK_VALIDATE(vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ssaoBlurPipelineLayout.getHandle(), 0, 1, &ssaoBlurDescriptorSet.getHandle(), 0, nullptr));

	glm::ivec2 axis = { 1,0 };
	VK_VALIDATE(vkCmdPushConstants(cmd, ssaoBlurPipelineLayout.getHandle(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::ivec2), &axis));

	VK_VALIDATE(vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets));

	VK_VALIDATE(vkCmdDraw(cmd, 6, 1, 0, 0));

	VK_VALIDATE(vkCmdEndRenderPass(cmd));

	renderPassInfo.framebuffer = ssaoFinalFramebuffer.getHandle();

	VK_VALIDATE(vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE));

	VK_VALIDATE(vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ssaoBlurPipeline.getHandle()));

	VK_VALIDATE(vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ssaoBlurPipelineLayout.getHandle(), 0, 1, &ssaoFinalDescriptorSet.getHandle(), 0, nullptr));

	axis = { 0,1 };
	VK_VALIDATE(vkCmdPushConstants(cmd, ssaoBlurPipelineLayout.getHandle(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::ivec2), &axis));

	VK_VALIDATE(vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets));

	VK_VALIDATE(vkCmdDraw(cmd, 6, 1, 0, 0));

	VK_VALIDATE(vkCmdEndRenderPass(cmd));

	queryPool.cmdTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, END_SSAO);

	VK_CHECK_RESULT(vkEndCommandBuffer(cmd));
}

void Renderer::destroySSAOAttachments()
{
	ssaoColourAttachment.destroy();
	ssaoBlurAttachment.destroy();
	ssaoFinalAttachment.destroy();
}

void Renderer::destroySSAORenderPass()
{
	ssaoRenderPass.destroy();
	ssaoBlurRenderPass.destroy();
}

void Renderer::destroySSAODescriptorSetLayouts()
{
	ssaoDescriptorSetLayout.destroy();
	ssaoBlurDescriptorSetLayout.destroy();
	ssaoFinalDescriptorSetLayout.destroy();
}

void Renderer::destroySSAOPipeline()
{
	ssaoPipelineLayout.destroy();
	ssaoPipeline.destroy();

	ssaoBlurPipelineLayout.destroy();
	ssaoBlurPipeline.destroy();
}

void Renderer::destroySSAOFramebuffer()
{
	ssaoFramebuffer.destroy();
	ssaoBlurFramebuffer.destroy();
	ssaoFinalFramebuffer.destroy();
}

void Renderer::destroySSAODescriptorSets()
{
	ssaoDescriptorSet.free();
	ssaoBlurDescriptorSet.free();
	ssaoFinalDescriptorSet.free();
}

void Renderer::destroySSAOCommands()
{
	ssaoCommandBuffer.free();
}
