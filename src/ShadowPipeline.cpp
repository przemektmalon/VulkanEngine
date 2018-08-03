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
	// Compile GLSL code to SPIR-V

	pointShadowShader.create(&logicalDevice);
	spotShadowShader.create(&logicalDevice);
	sunShadowShader.create(&logicalDevice);

	pointShadowShader.compile();
	spotShadowShader.compile();
	sunShadowShader.compile();

	// Get the vertex layout format
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getShadowAttributeDescriptions();

	// For submitting vertex layout info
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// Assembly info (triangles quads lines strips etc)
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Viewport
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)1024; /// TODO: variable resolutions require multiple pipelines or maybe dynamic state ?
	viewport.height = (float)1024;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// Scissor
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent.width = 1024;
	scissor.extent.height = 1024;

	// Submit info for viewport(s)
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterizer info (culling, polygon fill mode, etc)
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	// Multisampling (doesnt work well with deffered renderer without convoluted methods)
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 0;
	colorBlending.pAttachments = 0;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	//Depth
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = Engine::maxDepth; /// TODO: make variable

	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(glm::fvec4);
	pushConstantRange.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	// Pipeline layout for specifying descriptor sets (shaders use to access buffer and image resources)
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &shadowDescriptorSetLayout.getHandle();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pointShadowPipelineLayout));

	// Collate all the data necessary to create pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 3;
	pipelineInfo.pStages = pointShadowShader.getShaderStageCreateInfos();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.layout = pointShadowPipelineLayout;
	pipelineInfo.renderPass = shadowRenderPass.getHandle();
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pointShadowPipeline));


	viewport.width = (float)512; /// TODO: variable resolutions require multiple pipelines or maybe dynamic state ?
	viewport.height = (float)512;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	scissor.extent.width = 512;
	scissor.extent.height = 512;

	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = Engine::maxDepth; /// TODO: make variable
	pushConstantRange.offset = 0;
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.size = sizeof(glm::fmat4) + sizeof(glm::fvec4) * 2;

	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &shadowDescriptorSetLayout.getHandle();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &spotShadowPipelineLayout));

	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = spotShadowShader.getShaderStageCreateInfos();
	pipelineInfo.layout = spotShadowPipelineLayout;
	pipelineInfo.renderPass = shadowRenderPass.getHandle();

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &spotShadowPipeline));



	viewport.width = (float)1280; /// TODO: variable resolutions require multiple pipelines or maybe dynamic state ?
	viewport.height = (float)720;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	scissor.extent.width = 1280;
	scissor.extent.height = 720;

	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = Engine::maxDepth; /// TODO: make variable
	pushConstantRange.offset = 0;
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.size = sizeof(glm::fmat4);

	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &shadowDescriptorSetLayout.getHandle();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &sunShadowPipelineLayout));

	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = sunShadowShader.getShaderStageCreateInfos();
	pipelineInfo.layout = sunShadowPipelineLayout;
	pipelineInfo.renderPass = shadowRenderPass.getHandle();

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &sunShadowPipeline));
}

void Renderer::createShadowDescriptorSets()
{
	shadowDescriptorSet.create(&logicalDevice, &shadowDescriptorSetLayout, &descriptorPool);
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

		VK_VALIDATE(vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pointShadowPipeline));

		VK_VALIDATE(vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pointShadowPipelineLayout, 0, 1, &shadowDescriptorSet.getHandle(), 0, nullptr));

		VkBuffer vertexBuffers[] = { vertexIndexBuffer.getHandle() };
		VkDeviceSize offsets[] = { 0 };
		VK_VALIDATE(vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets));
		VK_VALIDATE(vkCmdBindIndexBuffer(cmd, vertexIndexBuffer.getHandle(), INDEX_BUFFER_BASE, VK_INDEX_TYPE_UINT32));

		auto pos = l.getPosition();
		glm::fvec4 push(pos.x, pos.y, pos.z, l.getRadius());

		VK_VALIDATE(vkCmdPushConstants(cmd, pointShadowPipelineLayout, VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::fvec4), &push));
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

		VK_VALIDATE(vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, spotShadowPipeline));

		VK_VALIDATE(vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, spotShadowPipelineLayout, 0, 1, &shadowDescriptorSet.getHandle(), 0, nullptr));

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

		VK_VALIDATE(vkCmdPushConstants(cmd, spotShadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::fmat4) + sizeof(float) + sizeof(glm::fvec3), &push));
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

			VK_VALIDATE(vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sunShadowPipeline));

			VK_VALIDATE(vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sunShadowPipelineLayout, 0, 1, &shadowDescriptorSet.getHandle(), 0, nullptr));

			VkBuffer vertexBuffers[] = { vertexIndexBuffer.getHandle() };
			VkDeviceSize offsets[] = { 0 };
			VK_VALIDATE(vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets));
			VK_VALIDATE(vkCmdBindIndexBuffer(cmd, vertexIndexBuffer.getHandle(), INDEX_BUFFER_BASE, VK_INDEX_TYPE_UINT32));

			float push[(4 * 4)];
			glm::fmat4 pv = *l.getProjView();
			memcpy(push, &pv, sizeof(glm::fmat4));

			VK_VALIDATE(vkCmdPushConstants(cmd, sunShadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::fmat4), &push));
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
	VK_VALIDATE(vkDestroyPipelineLayout(device, pointShadowPipelineLayout, 0));
	VK_VALIDATE(vkDestroyPipeline(device, pointShadowPipeline, 0));
	VK_VALIDATE(vkDestroyPipelineLayout(device, spotShadowPipelineLayout, 0));
	VK_VALIDATE(vkDestroyPipeline(device, spotShadowPipeline, 0));
	VK_VALIDATE(vkDestroyPipelineLayout(device, sunShadowPipelineLayout, 0));
	VK_VALIDATE(vkDestroyPipeline(device, sunShadowPipeline, 0));
	pointShadowShader.destroy();
	spotShadowShader.destroy();
	sunShadowShader.destroy();
}

void Renderer::destroyShadowDescriptorSets()
{
	shadowDescriptorSet.destroy();
}

void Renderer::destroyShadowCommands()
{
	shadowCommandBuffer.free();
}
