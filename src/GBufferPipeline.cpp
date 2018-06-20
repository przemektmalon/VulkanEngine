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
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = gBufferDepthAttachment.getFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 3;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription normalAttachment = {};
	normalAttachment.format = VK_FORMAT_R32G32_SFLOAT;
	normalAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	normalAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	normalAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	normalAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	normalAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	normalAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	normalAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference normalAttachmentRef = {};
	normalAttachmentRef.attachment = 1;
	normalAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription pbrAttachment = {};
	pbrAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
	pbrAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	pbrAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	pbrAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	pbrAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	pbrAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	pbrAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	pbrAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference pbrAttachmentRef = {};
	pbrAttachmentRef.attachment = 2;
	pbrAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference refs[] = { colorAttachmentRef, normalAttachmentRef, pbrAttachmentRef};
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 3;
	subpass.pColorAttachments = refs;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	std::array<VkAttachmentDescription, 4> attachments = { colorAttachment, normalAttachment, pbrAttachment, depthAttachment };
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &gBufferRenderPass));
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
	// Compile GLSL code to SPIR-V

	gBufferShader.create(&logicalDevice);
	gBufferShader.compile();

	// Get the vertex layout format
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

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
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
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

	VkPipelineColorBlendAttachmentState normalBlendAttachment = {};
	normalBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	normalBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState pbrBlendAttachment = {};
	pbrBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	pbrBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState blendAtts[] = { colorBlendAttachment, normalBlendAttachment, pbrBlendAttachment};

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 3;
	colorBlending.pAttachments = blendAtts;
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


	// Pipeline layout for specifying descriptor sets (shaders use to access buffer and image resources indirectly)
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &gBufferDescriptorSetLayout.getHandle();
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &gBufferPipelineLayout));

	// Collate all the data necessary to create pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = gBufferShader.getShaderStageCreateInfos();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.layout = gBufferPipelineLayout;
	pipelineInfo.renderPass = gBufferRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &gBufferPipeline));
}

void Renderer::createGBufferFramebuffers()
{
	std::array<VkImageView, 4> attachments = {
		gBufferColourAttachment.getView(),
		gBufferNormalAttachment.getView(),
		gBufferPBRAttachment.getView(),
		gBufferDepthAttachment.getView()
	};

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = gBufferRenderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = renderResolution.width;
	framebufferInfo.height = renderResolution.height;
	framebufferInfo.layers = 1;

	VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &gBufferFramebuffer));
}

void Renderer::createGBufferDescriptorSets()
{
	gBufferDescriptorSet.create(&logicalDevice, &gBufferDescriptorSetLayout, &descriptorPool);
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

	VK_VALIDATE(vkCmdResetQueryPool(cmd, queryPool, 0, NUM_GPU_TIMESTAMPS));
	VK_VALIDATE(vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool, BEGIN_GBUFFER));

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = gBufferRenderPass;
	renderPassInfo.framebuffer = gBufferFramebuffer;
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

	VK_VALIDATE(vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gBufferPipeline));

	VK_VALIDATE(vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gBufferPipelineLayout, 0, 1, &gBufferDescriptorSet.getHandle(), 0, nullptr));

	VkBuffer vertexBuffers[] = { vertexIndexBuffer.getHandle() };
	VkDeviceSize offsets[] = { 0 };
	VK_VALIDATE(vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets));
	VK_VALIDATE(vkCmdBindIndexBuffer(cmd, vertexIndexBuffer.getHandle(), INDEX_BUFFER_BASE, VK_INDEX_TYPE_UINT32));

	VK_VALIDATE(vkCmdDrawIndexedIndirect(cmd, drawCmdBuffer.getHandle(), 0, Engine::world.instancesToDraw.size(), sizeof(VkDrawIndexedIndirectCommand)));

	VK_VALIDATE(vkCmdEndRenderPass(cmd));

	VK_VALIDATE(vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, END_GBUFFER));

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
	VK_VALIDATE(vkDestroyRenderPass(device, gBufferRenderPass, 0));
}

void Renderer::destroyGBufferDescriptorSetLayouts()
{
	gBufferDescriptorSetLayout.destroy();
}

void Renderer::destroyGBufferPipeline()
{
	VK_VALIDATE(vkDestroyPipelineLayout(device, gBufferPipelineLayout, 0));
	VK_VALIDATE(vkDestroyPipeline(device, gBufferPipeline, 0));
	gBufferShader.destroy();
}

void Renderer::destroyGBufferFramebuffers()
{
	VK_VALIDATE(vkDestroyFramebuffer(device, gBufferFramebuffer, 0));
}

void Renderer::destroyGBufferDescriptorSets()
{
	VK_CHECK_RESULT(vkFreeDescriptorSets(device, descriptorPool.getHandle(), 1, &gBufferDescriptorSet.getHandle()));
}

void Renderer::destroyGBufferCommands()
{
	gBufferCommands.free();
}
