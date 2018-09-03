#include "PCH.hpp"
#include "Renderer.hpp"
#include "Profiler.hpp"

void Renderer::createShadowRenderPass()
{
	auto shadowInfo = shadowRenderPass.setDepthAttachment(Engine::physicalDevice->findOptimalDepthFormat());
	shadowInfo->setInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED);
	shadowInfo->setFinalLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
	shadowInfo->setUsageLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	shadowRenderPass.create(&logicalDevice);
}

void Renderer::createShadowDescriptorSetLayouts()
{
	auto& dsl = shadowDescriptorSetLayout;

	dsl.addBinding("transforms", vdu::DescriptorType::UniformBuffer, 0, 1, vdu::ShaderStage::Vertex);
	dsl.create(&logicalDevice);
}

void Renderer::createShadowPipeline()
{
	pointShadowPipelineLayout.addPushConstantRange({ VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::fvec4) });
	pointShadowPipelineLayout.addDescriptorSetLayout(&shadowDescriptorSetLayout);
	pointShadowPipelineLayout.create(&logicalDevice);
	pointShadowPipeline.setShaderProgram(&pointShadowShader);
	pointShadowPipeline.setPipelineLayout(&pointShadowPipelineLayout);
	pointShadowPipeline.setVertexInputState(&defaultVertexInputState);
	pointShadowPipeline.addViewport({ 0.f, 0.f, 1024.f, 1024.f, 0.f, 1.f }, { 0, 0, 1024, 1024 });
	pointShadowPipeline.setMaxDepthBounds(Engine::maxDepth);
	pointShadowPipeline.setRenderPass(&shadowRenderPass);
	pointShadowPipeline.create(&logicalDevice);

	spotShadowPipelineLayout.addPushConstantRange({ VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::fmat4) + sizeof(glm::fvec4) * 2 });
	spotShadowPipelineLayout.addDescriptorSetLayout(&shadowDescriptorSetLayout);
	spotShadowPipelineLayout.create(&logicalDevice);
	spotShadowPipeline.setShaderProgram(&spotShadowShader);
	spotShadowPipeline.setPipelineLayout(&spotShadowPipelineLayout);
	spotShadowPipeline.setVertexInputState(&defaultVertexInputState);
	spotShadowPipeline.addViewport({ 0.f, 0.f, 512.f, 512.f, 0.f, 1.f }, { 0, 0, 512, 512 });
	spotShadowPipeline.setMaxDepthBounds(Engine::maxDepth);
	spotShadowPipeline.setRenderPass(&shadowRenderPass);
	spotShadowPipeline.create(&logicalDevice);

	sunShadowPipelineLayout.addPushConstantRange({ VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::fmat4) });
	sunShadowPipelineLayout.addDescriptorSetLayout(&shadowDescriptorSetLayout);
	sunShadowPipelineLayout.create(&logicalDevice);
	sunShadowPipeline.setShaderProgram(&sunShadowShader);
	sunShadowPipeline.setPipelineLayout(&sunShadowPipelineLayout);
	sunShadowPipeline.setVertexInputState(&defaultVertexInputState);
	sunShadowPipeline.addViewport({ 0.f, 0.f, 1280.f, 720.f, 0.f, 1.f }, { 0, 0, 1280, 720 });
	sunShadowPipeline.setMaxDepthBounds(Engine::maxDepth);
	sunShadowPipeline.setRenderPass(&shadowRenderPass);
	sunShadowPipeline.create(&logicalDevice);
}

void Renderer::createShadowDescriptorSets()
{
	shadowDescriptorSet.allocate(&logicalDevice, &shadowDescriptorSetLayout, &descriptorPool);
}

void Renderer::updateShadowDescriptorSets()
{
	auto updater = shadowDescriptorSet.makeUpdater();

	auto transformsUpdate = updater->addBufferUpdate("transforms");

	transformsUpdate->buffer = transformUBO.getHandle();
	transformsUpdate->offset = 0;
	transformsUpdate->range = sizeof(glm::fmat4) * 1000;

	shadowDescriptorSet.submitUpdater(updater);
	shadowDescriptorSet.destroyUpdater(updater);
}

void Renderer::createShadowCommands()
{
	shadowCommandBuffer.allocate(&logicalDevice, &commandPool);
}

void Renderer::updateShadowCommands()
{	
	//PROFILE_START("shadowfence");
	//vkWaitForFences(device, 1, &shadowFence, true, std::numeric_limits<u64>::max());
	//PROFILE_END("shadowfence");

	bufferFreeMutex.lock();
	shadowCommandBuffer.free();
	bufferFreeMutex.unlock();
	
	createShadowCommands();
	
	auto cmd = shadowCommandBuffer.getHandle();

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &beginInfo));
	queryPool.cmdTimestamp(shadowCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, BEGIN_SHADOW);

	for (auto& l : lightManager.pointLights)
	{
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = shadowRenderPass.getHandle();
		renderPassInfo.framebuffer = *l.shadowFBO;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent.width = 1024;
		renderPassInfo.renderArea.extent.height = 1024;

		std::array<VkClearValue, 1> clearValues = {};
		clearValues[0].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		VK_VALIDATE(vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE));

		VK_VALIDATE(vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pointShadowPipeline.getHandle()));

		VK_VALIDATE(vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pointShadowPipelineLayout.getHandle(), 0, 1, &shadowDescriptorSet.getHandle(), 0, nullptr));

		VkBuffer vertexBuffers[] = { vertexIndexBuffer.getHandle() };
		VkDeviceSize offsets[] = { 0 };
		VK_VALIDATE(vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets));
		VK_VALIDATE(vkCmdBindIndexBuffer(cmd, vertexIndexBuffer.getHandle(), INDEX_BUFFER_BASE, VK_INDEX_TYPE_UINT32));

		auto pos = l.getPosition();
		glm::fvec4 push(pos.x, pos.y, pos.z, l.getRadius());

		VK_VALIDATE(vkCmdPushConstants(cmd, pointShadowPipelineLayout.getHandle(), VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::fvec4), &push));
		VK_VALIDATE(vkCmdDrawIndexedIndirect(cmd, drawCmdBuffer.getHandle(), 0, Engine::world.instancesToDraw.size(), sizeof(VkDrawIndexedIndirectCommand)));

		VK_VALIDATE(vkCmdEndRenderPass(cmd));
	}

	for (auto& l : lightManager.spotLights)
	{
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = shadowRenderPass.getHandle();
		renderPassInfo.framebuffer = *l.shadowFBO;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent.width = 512;
		renderPassInfo.renderArea.extent.height = 512;

		std::array<VkClearValue, 1> clearValues = {};
		clearValues[0].depthStencil = { 1.f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		VK_VALIDATE(vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE));

		VK_VALIDATE(vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, spotShadowPipeline.getHandle()));

		VK_VALIDATE(vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, spotShadowPipelineLayout.getHandle(), 0, 1, &shadowDescriptorSet.getHandle(), 0, nullptr));

		VkBuffer vertexBuffers[] = { vertexIndexBuffer.getHandle() };
		VkDeviceSize offsets[] = { 0 };
		VK_VALIDATE(vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets));
		VK_VALIDATE(vkCmdBindIndexBuffer(cmd, vertexIndexBuffer.getHandle(), INDEX_BUFFER_BASE, VK_INDEX_TYPE_UINT32));

		float push[(4*4)+4+1];
		glm::fmat4 pv = l.getProjView();
		memcpy(push, &pv, sizeof(glm::fmat4));
		auto p = l.getPosition();
		push[16] = p.x;
		push[17] = p.y;
		push[18] = p.z;
		push[19] = 0;
		push[20] = l.getRadius() * 2.f;

		VK_VALIDATE(vkCmdPushConstants(cmd, spotShadowPipelineLayout.getHandle(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::fmat4) + sizeof(float) + sizeof(glm::fvec3), &push));
		VK_VALIDATE(vkCmdDrawIndexedIndirect(cmd, drawCmdBuffer.getHandle(), 0, Engine::world.instancesToDraw.size(), sizeof(VkDrawIndexedIndirectCommand)));

		VK_VALIDATE(vkCmdEndRenderPass(cmd));
	}

	{
		auto l = lightManager.sunLight;

		for (auto cascadeIndex = 0; cascadeIndex < 3; ++cascadeIndex)
		{
			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = shadowRenderPass.getHandle();
			renderPassInfo.framebuffer = l.shadowFBO[cascadeIndex];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent.width = 1280;
			renderPassInfo.renderArea.extent.height = 720;

			std::array<VkClearValue, 1> clearValues = {};
			clearValues[0].depthStencil = { 1.f, 0 };
			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			VK_VALIDATE(vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE));

			VK_VALIDATE(vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sunShadowPipeline.getHandle()));

			VK_VALIDATE(vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sunShadowPipelineLayout.getHandle(), 0, 1, &shadowDescriptorSet.getHandle(), 0, nullptr));

			VkBuffer vertexBuffers[] = { vertexIndexBuffer.getHandle() };
			VkDeviceSize offsets[] = { 0 };
			VK_VALIDATE(vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets));
			VK_VALIDATE(vkCmdBindIndexBuffer(cmd, vertexIndexBuffer.getHandle(), INDEX_BUFFER_BASE, VK_INDEX_TYPE_UINT32));

			float push[(4 * 4)];
			glm::fmat4 pv = *l.getProjView();
			memcpy(push, &pv, sizeof(glm::fmat4));

			VK_VALIDATE(vkCmdPushConstants(cmd, sunShadowPipelineLayout.getHandle(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::fmat4), &push));
			VK_VALIDATE(vkCmdDrawIndexedIndirect(cmd, drawCmdBuffer.getHandle(), 0, Engine::world.instancesToDraw.size(), sizeof(VkDrawIndexedIndirectCommand)));

			VK_VALIDATE(vkCmdEndRenderPass(cmd));
		}
	}

	queryPool.cmdTimestamp(shadowCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, END_SHADOW);

	VK_CHECK_RESULT(vkEndCommandBuffer(cmd));
}

void Renderer::destroyShadowRenderPass()
{
	shadowRenderPass.destroy();
}

void Renderer::destroyShadowDescriptorSetLayouts()
{
	shadowDescriptorSetLayout.destroy();
}

void Renderer::destroyShadowPipeline()
{
	pointShadowPipelineLayout.destroy();
	spotShadowPipelineLayout.destroy();
	sunShadowPipelineLayout.destroy();

	pointShadowPipeline.destroy();
	spotShadowPipeline.destroy();
	sunShadowPipeline.destroy();

	
}

void Renderer::destroyShadowDescriptorSets()
{
	shadowDescriptorSet.free();
}

void Renderer::destroyShadowCommands()
{
	shadowCommandBuffer.free();
}
