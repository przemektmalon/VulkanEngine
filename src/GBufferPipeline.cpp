#include "PCH.hpp"
#include "Renderer.hpp"

void Renderer::createGBufferAttachments()
{
	TextureCreateInfo tci;
	tci.width = renderResolution.width;
	tci.height = renderResolution.height;
	tci.bpp = 32;
	tci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	tci.format = VK_FORMAT_B8G8R8A8_UNORM;
	tci.layout = VK_IMAGE_LAYOUT_GENERAL;
	tci.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

	gBufferColourAttachment.loadStream(&tci);

	tci.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	tci.bpp = 32 * 4;

	gBufferNormalAttachment.loadStream(&tci);

	tci.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	tci.format = findDepthFormat();
	tci.layout = VK_IMAGE_LAYOUT_UNDEFINED;
	tci.usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	gBufferDepthAttachment.loadStream(&tci);
}

void Renderer::createGBufferRenderPass()
{
	DBG_INFO("Creating Vulkan render pass");
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription normalAttachment = {};
	normalAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	normalAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	normalAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	normalAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	normalAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	normalAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	normalAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	normalAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkAttachmentReference normalAttachmentRef = {};
	normalAttachmentRef.attachment = 1;
	normalAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 2;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference refs[] = { colorAttachmentRef, normalAttachmentRef };
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 2;
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
	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, normalAttachment, depthAttachment };
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &gBufferRenderPass) != VK_SUCCESS) {
		DBG_SEVERE("Failed to create render pass");
	}
}

void Renderer::createGBufferDescriptorSetLayouts()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding transLayoutBinding = {};
	transLayoutBinding.binding = 1;
	transLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	transLayoutBinding.descriptorCount = 1;
	transLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 2;
	samplerLayoutBinding.descriptorCount = 1000;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 3> bindings = { uboLayoutBinding, transLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &gBufferDescriptorSetLayout) != VK_SUCCESS) {
		DBG_SEVERE("Failed to create Vulkan descriptor set layout");
	}
}

void Renderer::createGBufferPipeline()
{
	// Compile GLSL code to SPIR-V

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

	VkPipelineColorBlendAttachmentState blendAtts[] = { colorBlendAttachment, normalBlendAttachment };

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 2;
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
	depthStencil.maxDepthBounds = 1.0f;


	// Pipeline layout for specifying descriptor sets (shaders use to access buffer and image resources indirectly)
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &gBufferDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &gBufferPipelineLayout) != VK_SUCCESS) {
		DBG_SEVERE("Failed to create Vulkan pipeline layout");
	}

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

	VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &gBufferPipeline);

	if (result != VK_SUCCESS) {
		DBG_SEVERE("Failed to create Vulkan graphics pipeline");
	}
}

void Renderer::createGBufferFramebuffers()
{
	std::array<VkImageView, 3> attachments = {
		gBufferColourAttachment.getImageViewHandle(),
		gBufferNormalAttachment.getImageViewHandle(),
		gBufferDepthAttachment.getImageViewHandle()
	};

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = gBufferRenderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = renderResolution.width;
	framebufferInfo.height = renderResolution.height;
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &gBufferFramebuffer) != VK_SUCCESS) {
		DBG_SEVERE("Failed to create Vulkan framebuffers");
	}
}

void Renderer::createGBufferDescriptorSets()
{
	VkDescriptorSetLayout layouts[] = { gBufferDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocInfo, &gBufferDescriptorSet) != VK_SUCCESS) {
		DBG_SEVERE("Failed to allocate descriptor set");
	}
}

void Renderer::updateGBufferDescriptorSets()
{
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	VkDescriptorBufferInfo bufferInfo2 = {};
	bufferInfo2.buffer = transformBuffer;
	bufferInfo2.offset = 0;
	bufferInfo2.range = sizeof(glm::fmat4) * 3;

	VkDescriptorImageInfo	imageInfo[1000];
	for (u32 i = 0; i < 1000; ++i)
	{
		imageInfo[i].sampler = textureSampler;
		imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo[i].imageView = Engine::assets.materials.begin()->second.albedo->getImageViewHandle();
	}

	u32 i = 0;
	for (auto& material : Engine::assets.materials)
	{
		imageInfo[i].imageView = material.second.albedo->getImageViewHandle();

		if (material.second.normal)
			imageInfo[i + 1].imageView = material.second.normal->getImageViewHandle();
		else
			imageInfo[i + 1].imageView = material.second.albedo->getImageViewHandle(); /// TODO: some null texture

		if (material.second.specularMetallic)
			imageInfo[i + 2].imageView = material.second.specularMetallic->getImageViewHandle();
		else
			imageInfo[i + 2].imageView = material.second.albedo->getImageViewHandle();

		if (material.second.roughness)
			imageInfo[i + 3].imageView = material.second.roughness->getImageViewHandle();
		else
			imageInfo[i + 3].imageView = material.second.albedo->getImageViewHandle();

		if (material.second.ao)
			imageInfo[i + 4].imageView = material.second.ao->getImageViewHandle();
		else
			imageInfo[i + 4].imageView = material.second.albedo->getImageViewHandle();

		if (material.second.height)
			imageInfo[i + 5].imageView = material.second.height->getImageViewHandle();
		else
			imageInfo[i + 5].imageView = material.second.albedo->getImageViewHandle();
		i += 6;
	}

	std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = gBufferDescriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = gBufferDescriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &bufferInfo2;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = gBufferDescriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[2].descriptorCount = 1000;
	descriptorWrites[2].pImageInfo = imageInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Renderer::createGBufferCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &gBufferCommandBuffer) != VK_SUCCESS) {
		DBG_SEVERE("Failed to allocate Vulkan command buffers");
	}
}

void Renderer::updateGBufferCommands()
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	vkBeginCommandBuffer(gBufferCommandBuffer, &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = gBufferRenderPass;
	renderPassInfo.framebuffer = gBufferFramebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = renderResolution;

	std::array<VkClearValue, 3> clearValues = {};
	clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
	clearValues[1].color = { 0.1f, 0.1f, 0.1f, 1.0f };
	clearValues[2].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(gBufferCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(gBufferCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gBufferPipeline);

	vkCmdBindDescriptorSets(gBufferCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gBufferPipelineLayout, 0, 1, &gBufferDescriptorSet, 0, nullptr);

	VkBuffer vertexBuffers[] = { vertexIndexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(gBufferCommandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(gBufferCommandBuffer, vertexIndexBuffer, INDEX_BUFFER_BASE, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexedIndirect(gBufferCommandBuffer, drawCmdBuffer, 0, Engine::world.models.size(), sizeof(VkDrawIndexedIndirectCommand));

	vkCmdEndRenderPass(gBufferCommandBuffer);

	auto result = vkEndCommandBuffer(gBufferCommandBuffer);
	if (result != VK_SUCCESS) {
		DBG_SEVERE("Failed to record Vulkan command buffer. Error: " << result);
	}
}

void Renderer::destroyGBufferAttachments()
{
	gBufferColourAttachment.destroy();
	gBufferNormalAttachment.destroy();
	gBufferDepthAttachment.destroy();
}

void Renderer::destroyGBufferRenderPass()
{
	vkDestroyRenderPass(device, gBufferRenderPass, 0);
}

void Renderer::destroyGBufferDescriptorSetLayouts()
{
	vkDestroyDescriptorSetLayout(device, gBufferDescriptorSetLayout, 0);
}

void Renderer::destroyGBufferPipeline()
{
	vkDestroyPipelineLayout(device, gBufferPipelineLayout, 0);
	vkDestroyPipeline(device, gBufferPipeline, 0);
}

void Renderer::destroyGBufferFramebuffers()
{
	vkDestroyFramebuffer(device, gBufferFramebuffer, 0);
}

void Renderer::destroyGBufferDescriptorSets()
{
	vkFreeDescriptorSets(device, descriptorPool, 1, &gBufferDescriptorSet);
}

void Renderer::destroyGBufferCommands()
{
	vkFreeCommandBuffers(device, commandPool, 1, &gBufferCommandBuffer);
}