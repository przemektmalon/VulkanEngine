#include "PCH.hpp"
#include "Renderer.hpp"
#include "Engine.hpp"
#include "Window.hpp"
#include "VulkanWrappers.hpp"

/*
@brief	Create the optimal vulkan swapchain
*/
void Renderer::createScreenSwapChain()
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
	// Compile GLSL code to SPIR-V

	screenShader.create(&logicalDevice);
	screenShader.compile();

	// Get the vertex layout format
	auto bindingDescription = Vertex2D::getBindingDescription();
	auto attributeDescriptions = Vertex2D::getAttributeDescriptions();

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
	viewport.width = (float)renderResolution.width;
	viewport.height = (float)renderResolution.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// Scissor
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = renderResolution;

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
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	// Multisampling (doesnt work well with deffered renderer without convoluted methods)
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Color blending info (for transparency)
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	//Depth
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;


	// Pipeline layout for specifying descriptor sets (shaders use to access buffer and image resources indirectly)
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &screenDescriptorSetLayout.getHandle();
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &screenPipelineLayout));

	// Collate all the data necessary to create pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = screenShader.getShaderStageCreateInfos();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.layout = screenPipelineLayout;
	pipelineInfo.renderPass = screenSwapchain.getRenderPass().getHandle();
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.flags = 0;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &screenPipeline));
}

void Renderer::createScreenDescriptorSets()
{
	screenDescriptorSet.create(&logicalDevice, &screenDescriptorSetLayout, &descriptorPool);
}

void Renderer::updateScreenDescriptorSets()
{
	auto updater = screenDescriptorSet.makeUpdater();

	auto sceneUpdate = updater->addImageUpdate("scene");

	sceneUpdate->imageView = pbrOutput.getView();
	sceneUpdate->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	sceneUpdate->sampler = textureSampler;

	auto overlayUpdate = updater->addImageUpdate("overlay");

	overlayUpdate->imageView = overlayRenderer.combinedLayers.getView();
	overlayUpdate->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	overlayUpdate->sampler = textureSampler;

	screenDescriptorSet.submitUpdater(updater);
	screenDescriptorSet.destroyUpdater(updater);
}

void Renderer::createScreenCommands()
{
	// We need a command buffer for each framebuffer
	screenCommandBuffers.allocate(&logicalDevice, &commandPool, screenSwapchain.getImageCount());
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

		VK_VALIDATE(vkCmdBeginRenderPass(screenCommandBuffers.getHandle(i), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE));

		VK_VALIDATE(vkCmdBindPipeline(screenCommandBuffers.getHandle(i), VK_PIPELINE_BIND_POINT_GRAPHICS, screenPipeline));

		VK_VALIDATE(vkCmdBindDescriptorSets(screenCommandBuffers.getHandle(i), VK_PIPELINE_BIND_POINT_GRAPHICS, screenPipelineLayout, 0, 1, &screenDescriptorSet.getHandle(), 0, nullptr));

		VkBuffer vertexBuffers[] = { screenQuadBuffer.getHandle() };
		VkDeviceSize offsets[] = { 0 };
		VK_VALIDATE(vkCmdBindVertexBuffers(screenCommandBuffers.getHandle(i), 0, 1, vertexBuffers, offsets));

		VK_VALIDATE(vkCmdDraw(screenCommandBuffers.getHandle(i), 6, 1, 0, 0));

		VK_VALIDATE(vkCmdEndRenderPass(screenCommandBuffers.getHandle(i)));

		setImageLayout(screenCommandBuffers.getHandle(i), pbrOutput, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

		queryPool.cmdTimestamp(screenCommandBuffers.getHandle(i), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, END_SCREEN);

		VK_CHECK_RESULT(vkEndCommandBuffer(screenCommandBuffers.getHandle(i)));
	}
}

void Renderer::updateScreenCommandsForConsole()
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

		VK_VALIDATE(vkCmdBeginRenderPass(screenCommandBuffers.getHandle(i), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE));

		VK_VALIDATE(vkCmdBindPipeline(screenCommandBuffers.getHandle(i), VK_PIPELINE_BIND_POINT_GRAPHICS, screenPipeline));

		VK_VALIDATE(vkCmdBindDescriptorSets(screenCommandBuffers.getHandle(i), VK_PIPELINE_BIND_POINT_GRAPHICS, screenPipelineLayout, 0, 1, &screenDescriptorSet.getHandle(), 0, nullptr));

		VkBuffer vertexBuffers[] = { screenQuadBuffer.getHandle() };
		VkDeviceSize offsets[] = { 0 };
		VK_VALIDATE(vkCmdBindVertexBuffers(screenCommandBuffers.getHandle(i), 0, 1, vertexBuffers, offsets));

		VK_VALIDATE(vkCmdDraw(screenCommandBuffers.getHandle(i), 6, 1, 0, 0));

		VK_VALIDATE(vkCmdEndRenderPass(screenCommandBuffers.getHandle(i)));

		queryPool.cmdTimestamp(screenCommandBuffers.getHandle(i), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, END_SCREEN);

		VK_CHECK_RESULT(vkEndCommandBuffer(screenCommandBuffers.getHandle(i)));
	}
}

void Renderer::destroyScreenSwapChain()
{
	screenSwapchain.destroy();
}

void Renderer::destroyScreenDescriptorSetLayouts()
{
	screenDescriptorSetLayout.destroy();
}

void Renderer::destroyScreenPipeline()
{
	VK_VALIDATE(vkDestroyPipelineLayout(device, screenPipelineLayout, 0));
	VK_VALIDATE(vkDestroyPipeline(device, screenPipeline, 0));
	screenShader.destroy();
}

void Renderer::destroyScreenDescriptorSets()
{
	//VK_CHECK_RESULT(vkFreeDescriptorSets(device, descriptorPool.getHandle(), 1, &screenDescriptorSet));
	screenDescriptorSet.destroy();
}

void Renderer::destroyScreenCommands()
{
	screenCommandBuffers.free();
}
