#include "PCH.hpp"
#include "Renderer.hpp"
#include "Engine.hpp"
#include "Window.hpp"

void Renderer::createScreenSwapchain()
{
	screenSwapchain.create(&logicalDevice, Engine::window->vkSurface);
	renderResolution = screenSwapchain.getExtent();
}

void Renderer::createScreenDescriptorSetLayouts()
{
	auto& dsl = screenDescriptorSetLayout;

	dsl.addBinding("scene", vdu::DescriptorType::CombinedImageSampler, 0, 1, vdu::ShaderStage::Fragment);
	dsl.addBinding("overlay", vdu::DescriptorType::CombinedImageSampler, 1, 1, vdu::ShaderStage::Fragment);

	dsl.create(&logicalDevice);
}

void Renderer::createScreenPipeline()
{
	screenPipelineLayout.addDescriptorSetLayout(&screenDescriptorSetLayout);
	screenPipelineLayout.create(&logicalDevice);

	screenPipeline.setShaderProgram(&screenShader);
	screenPipeline.setVertexInputState(&screenVertexInputState);
	screenPipeline.addViewport({ 0.f, 0.f, (float)renderResolution.width, (float)renderResolution.height, 0.f, 1.f }, { 0, 0, renderResolution.width, renderResolution.height });
	screenPipeline.setPipelineLayout(&screenPipelineLayout);
	screenPipeline.setSwapchain(&screenSwapchain);
	screenPipeline.create(&logicalDevice);
}

void Renderer::createScreenDescriptorSets()
{
	screenDescriptorSet.allocate(&logicalDevice, &screenDescriptorSetLayout, &descriptorPool);
}

void Renderer::updateScreenDescriptorSets()
{
	auto updater = screenDescriptorSet.makeUpdater();

	auto sceneUpdate = updater->addImageUpdate("scene");

	sceneUpdate->imageView = pbrOutput.getView();
	sceneUpdate->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	sceneUpdate->sampler = textureSampler;

	auto overlayUpdate = updater->addImageUpdate("overlay");

	overlayUpdate->imageView = uiRenderer.uiTexture.getView();
	overlayUpdate->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	overlayUpdate->sampler = textureSampler;

	screenDescriptorSet.submitUpdater(updater);
	screenDescriptorSet.destroyUpdater(updater);
}

void Renderer::createScreenCommands()
{
	// We need a command buffer for each framebuffer
	screenCommandBuffers.allocate(&logicalDevice, &commandPool, screenSwapchain.getImageCount());
	screenCommandBuffersForConsole.allocate(&logicalDevice, &commandPool, screenSwapchain.getImageCount());
}

void Renderer::updateScreenCommands()
{
	for (size_t i = 0; i < screenCommandBuffers.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		VK_CHECK_RESULT(vkBeginCommandBuffer(screenCommandBuffers.getHandle(i), &beginInfo));

		queryPool.cmdTimestamp(screenCommandBuffers.getHandle(i), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, BEGIN_SCREEN);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = screenSwapchain.getRenderPass().getHandle();
		renderPassInfo.framebuffer = screenSwapchain.getFramebuffers()[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = renderResolution;

		std::array<VkClearValue, 1> clearValues = {};
		clearValues[0].color = { 1.f, 0.1f, 0.1f, 1.0f };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(screenCommandBuffers.getHandle(i), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(screenCommandBuffers.getHandle(i), VK_PIPELINE_BIND_POINT_GRAPHICS, screenPipeline.getHandle());

		vkCmdBindDescriptorSets(screenCommandBuffers.getHandle(i), VK_PIPELINE_BIND_POINT_GRAPHICS, screenPipelineLayout.getHandle(), 0, 1, &screenDescriptorSet.getHandle(), 0, nullptr);

		VkBuffer vertexBuffers[] = { screenQuadBuffer.getHandle() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(screenCommandBuffers.getHandle(i), 0, 1, vertexBuffers, offsets);

		vkCmdDraw(screenCommandBuffers.getHandle(i), 6, 1, 0, 0);

		vkCmdEndRenderPass(screenCommandBuffers.getHandle(i));

		//setImageLayout(screenCommandBuffers.getHandle(i), pbrOutput, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

		queryPool.cmdTimestamp(screenCommandBuffers.getHandle(i), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, END_SCREEN);

		VK_CHECK_RESULT(vkEndCommandBuffer(screenCommandBuffers.getHandle(i)));
	}
}

void Renderer::updateScreenCommandsForConsole()
{
	for (size_t i = 0; i < screenCommandBuffersForConsole.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		VK_CHECK_RESULT(vkBeginCommandBuffer(screenCommandBuffersForConsole.getHandle(i), &beginInfo));

		queryPool.cmdTimestamp(screenCommandBuffersForConsole.getHandle(i), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, BEGIN_SCREEN);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = screenSwapchain.getRenderPass().getHandle();
		renderPassInfo.framebuffer = screenSwapchain.getFramebuffers()[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = renderResolution;

		std::array<VkClearValue, 1> clearValues = {};
		clearValues[0].color = { 1.f, 0.1f, 0.1f, 1.0f };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(screenCommandBuffersForConsole.getHandle(i), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(screenCommandBuffersForConsole.getHandle(i), VK_PIPELINE_BIND_POINT_GRAPHICS, screenPipeline.getHandle());

		vkCmdBindDescriptorSets(screenCommandBuffersForConsole.getHandle(i), VK_PIPELINE_BIND_POINT_GRAPHICS, screenPipelineLayout.getHandle(), 0, 1, &screenDescriptorSet.getHandle(), 0, nullptr);

		VkBuffer vertexBuffers[] = { screenQuadBuffer.getHandle() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(screenCommandBuffersForConsole.getHandle(i), 0, 1, vertexBuffers, offsets);

		vkCmdDraw(screenCommandBuffersForConsole.getHandle(i), 6, 1, 0, 0);

		vkCmdEndRenderPass(screenCommandBuffersForConsole.getHandle(i));

		queryPool.cmdTimestamp(screenCommandBuffersForConsole.getHandle(i), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, END_SCREEN);

		VK_CHECK_RESULT(vkEndCommandBuffer(screenCommandBuffersForConsole.getHandle(i)));
	}
}

void Renderer::destroyScreenSwapchain()
{
	screenSwapchain.destroy();
}

void Renderer::destroyScreenDescriptorSetLayouts()
{
	screenDescriptorSetLayout.destroy();
}

void Renderer::destroyScreenPipeline()
{
	screenPipelineLayout.destroy();
	screenPipeline.destroy();
}

void Renderer::destroyScreenDescriptorSets()
{
	screenDescriptorSet.free();
}

void Renderer::destroyScreenCommands()
{
	screenCommandBuffers.free();
}
