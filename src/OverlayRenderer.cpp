#include "PCH.hpp"
#include "OverlayRenderer.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "Window.hpp"
#include "Threading.hpp"

void OLayer::create(glm::ivec2 pResolution)
{
	resolution = pResolution;
	position = glm::ivec2(0);
	depth = 0.5;

	TextureCreateInfo tci;
	tci.width = resolution.x;
	tci.height = resolution.y;
	tci.bpp = 32;
	tci.components = 4;
	tci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	tci.format = VK_FORMAT_R8G8B8A8_UNORM;
	tci.layout = VK_IMAGE_LAYOUT_GENERAL;
	tci.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	colAttachment.loadToGPU(&tci);

	tci.components = 1;
	tci.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	tci.format = Engine::physicalDevice->findOptimalDepthFormat();
	tci.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	tci.usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	depAttachment.loadToGPU(&tci);

	VkImageView atts[] = { colAttachment.getImageViewHandle(), depAttachment.getImageViewHandle() };
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = Engine::renderer->overlayRenderer.getRenderPass();
	framebufferInfo.attachmentCount = 2;
	framebufferInfo.pAttachments = atts;
	framebufferInfo.width = resolution.x;
	framebufferInfo.height = resolution.y;
	framebufferInfo.layers = 1;

	VK_CHECK_RESULT(vkCreateFramebuffer(Engine::renderer->device, &framebufferInfo, nullptr, &framebuffer));

	quadBuffer.create(sizeof(VertexNoNormal) * 6, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = Engine::renderer->descriptorPool.getHandle();
	allocInfo.descriptorSetCount = 1;
	auto layout = Engine::renderer->overlayRenderer.getDescriptorSetLayout();
	allocInfo.pSetLayouts = &layout;
	
	vkAllocateDescriptorSets(Engine::renderer->device, &allocInfo, &imageDescriptor);

	VkDescriptorImageInfo ii = {};
	ii.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	ii.imageView = colAttachment.getImageViewHandle();
	ii.sampler = Engine::renderer->textureSampler;

	VkWriteDescriptorSet wds = {};
	wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	wds.descriptorCount = 1;
	wds.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	wds.pImageInfo = &ii;
	wds.dstSet = imageDescriptor;

	vkUpdateDescriptorSets(Engine::renderer->device, 1, &wds, 0, 0);

	setPosition(glm::ivec2(0, 0));
}

void OLayer::cleanup()
{
	colAttachment.destroy();
	depAttachment.destroy();
	//VK_VALIDATE(vkFreeDescriptorSets(Engine::renderer->device, Engine::renderer->descriptorPool, 1, &imageDescriptor));
	VK_VALIDATE(vkDestroyFramebuffer(Engine::renderer->device, framebuffer, 0));
	quadBuffer.destroy();
	for (auto element : elements)
	{
		element->cleanup();
		delete element;
	}
}

void OLayer::addElement(OverlayElement * el)
{
	auto find = std::find(elements.begin(), elements.end(), el);
	if (find == elements.end())
	{
		// No element with required shader exists, add vector for this type of shader
		elements.push_back(el);
	}
	else
	{
		// Elements already exists
		/// TODO: log error ?
	}

	auto nameFind = elementLabels.find(el->getName());
	if (nameFind == elementLabels.end())
		elementLabels.insert(std::make_pair(el->getName(), el));
	else
	{
		std::string newName = el->getName();
		newName += std::to_string(Engine::clock.now());
		nameFind = elementLabels.find(newName);
		u64 i = 1;
		while (nameFind != elementLabels.end())
		{
			newName = el->getName() + std::to_string(Engine::clock.now() + i);
			nameFind = elementLabels.find(newName);
		}
		elementLabels.insert(std::make_pair(newName, el));
	}
}

OverlayElement * OLayer::getElement(std::string pName)
{
	auto find = elementLabels.find(pName);
	if (find == elementLabels.end())
	{
		DBG_WARNING("Layer element not found: " << pName);
		return 0;
	}
	return find->second;
}

void OLayer::removeElement(OverlayElement * el)
{
	for (auto itr = elements.begin(); itr != elements.end(); ++itr)
	{
		if (*itr == el)
			itr = elements.erase(itr);
	}
	elementLabels.erase(el->getName());
}

void OLayer::setPosition(glm::ivec2 pPos)
{
	position = pPos;
	updateVerts();
}

void OLayer::updateVerts()
{
	quadVerts.clear();

	quadVerts.push_back(VertexNoNormal({ glm::fvec3(position,depth), glm::fvec2(0,0) }));
	quadVerts.push_back(VertexNoNormal({ glm::fvec3(position,depth) + glm::fvec3(0, resolution.y, 0), glm::fvec2(0,1) }));
	quadVerts.push_back(VertexNoNormal({ glm::fvec3(position,depth) + glm::fvec3(resolution.x, 0, 0), glm::fvec2(1,0) }));

	quadVerts.push_back(VertexNoNormal({ glm::fvec3(position,depth) + glm::fvec3(resolution.x, 0, 0), glm::fvec2(1,0) }));
	quadVerts.push_back(VertexNoNormal({ glm::fvec3(position,depth) + glm::fvec3(0, resolution.y, 0), glm::fvec2(0,1) }));
	quadVerts.push_back(VertexNoNormal({ glm::fvec3(position,depth) + glm::fvec3(resolution, 0), glm::fvec2(1,1) }));

	auto data = quadBuffer.map();
	memcpy(data, quadVerts.data(), sizeof(VertexNoNormal) * 6);
	quadBuffer.unmap();
}

bool OLayer::needsDrawUpdate()
{
	bool ret = needsUpdate;
	bool sortDepth = false;
	for (auto& el : elements)
	{
		sortDepth |= el->needsDepthUpdate();
		ret |= el->needsDrawUpdate() | sortDepth;
	}
	if (sortDepth)
		sortByDepths();
	needsUpdate = false;
	return ret;
}

void OLayer::setUpdated()
{
	for (auto& el : elements)
	{
		el->setUpdated();
	}
}

void OverlayRenderer::cleanup()
{
	cleanupForReInit();
	destroyOverlayDescriptorSetLayouts();
	combineProjUBO.destroy();

	for (auto layer : layers)
		layer->cleanup();

	layers.empty();
}

void OverlayRenderer::cleanupForReInit()
{
	destroyOverlayRenderPass();
	destroyOverlayPipeline();
	destroyOverlayAttachmentsFramebuffers();
	//destroyOverlayDescriptorSets();
	destroyOverlayCommands();
}

void OverlayRenderer::addLayer(OLayer * layer)
{
	Engine::threading->layersMutex.lock();
	layers.insert(layer);
	Engine::threading->layersMutex.unlock();
}

void OverlayRenderer::removeLayer(OLayer * layer)
{
	Engine::threading->layersMutex.lock();
	layers.erase(layer);
	Engine::threading->layersMutex.unlock();
}

void OverlayRenderer::createOverlayAttachmentsFramebuffers()
{
	TextureCreateInfo tci;
	tci.width = Engine::renderer->renderResolution.width;
	tci.height = Engine::renderer->renderResolution.height;
	tci.bpp = 32;
	tci.components = 4;
	tci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	tci.format = VK_FORMAT_R8G8B8A8_UNORM;
	tci.layout = VK_IMAGE_LAYOUT_GENERAL;
	tci.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	combinedLayers.loadToGPU(&tci);

	tci.components = 1;
	tci.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	tci.format = Engine::physicalDevice->findOptimalDepthFormat();
	tci.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	tci.usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	combinedLayersDepth.loadToGPU(&tci);

	VkImageView atts[] = { combinedLayers.getImageViewHandle(), combinedLayersDepth.getImageViewHandle() };
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = Engine::renderer->overlayRenderer.getRenderPass();
	framebufferInfo.attachmentCount = 2;
	framebufferInfo.pAttachments = atts;
	framebufferInfo.width = Engine::renderer->renderResolution.width;
	framebufferInfo.height = Engine::renderer->renderResolution.height;
	framebufferInfo.layers = 1;

	VK_CHECK_RESULT(vkCreateFramebuffer(Engine::renderer->device, &framebufferInfo, nullptr, &framebuffer));
}

void OverlayRenderer::createOverlayRenderPass()
{
	VkAttachmentDescription colAttachment = {};
	colAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
	colAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference colAttachmentRef = {};
	colAttachmentRef.attachment = 0;
	colAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depAttachment = {};
	depAttachment.format = Engine::physicalDevice->findOptimalDepthFormat();
	depAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depAttachmentRef = {};
	depAttachmentRef.attachment = 1;
	depAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colAttachmentRef;
	subpass.pDepthStencilAttachment = &depAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	std::array<VkAttachmentDescription, 2> attachments = { colAttachment, depAttachment };
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VK_CHECK_RESULT(vkCreateRenderPass(Engine::renderer->device, &renderPassInfo, nullptr, &overlayRenderPass));
}

void OverlayRenderer::createOverlayPipeline()
{
	{
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
		viewport.width = (float)1280;
		viewport.height = (float)720;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		// Scissor
		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent.width = 1280;
		scissor.extent.height = 720;

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

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

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
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f;
		depthStencil.maxDepthBounds = 1000.0f; /// WATCH: whether we need to alter this

		VkPushConstantRange pushConstantRanges[2];
		pushConstantRanges[0].offset = 0;
		pushConstantRanges[0].size = sizeof(glm::fmat4) + sizeof(glm::fvec4);
		pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRanges[1].offset = sizeof(glm::fmat4) + sizeof(glm::fvec4);
		pushConstantRanges[1].size = sizeof(glm::fvec4) + sizeof(int);
		pushConstantRanges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


		// Pipeline layout for specifying descriptor sets (shaders use to access buffer and image resources)
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		auto descSetLayout = Engine::renderer->overlayRenderer.getDescriptorSetLayout();
		pipelineLayoutInfo.pSetLayouts = &descSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 2;
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges;

		std::vector<VkDynamicState> dynamicState;
		dynamicState.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamicState.push_back(VK_DYNAMIC_STATE_SCISSOR);

		VkPipelineDynamicStateCreateInfo dsci = {};
		dsci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dsci.dynamicStateCount = dynamicState.size();
		dsci.pDynamicStates = dynamicState.data();

		VK_CHECK_RESULT(vkCreatePipelineLayout(Engine::renderer->device, &pipelineLayoutInfo, nullptr, &elementPipelineLayout));

		// Collate all the data necessary to create pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = Engine::renderer->overlayShader.getShaderStageCreateInfos();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.layout = elementPipelineLayout;
		pipelineInfo.renderPass = Engine::renderer->overlayRenderer.getRenderPass();
		pipelineInfo.subpass = 0;
		pipelineInfo.pDynamicState = &dsci;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(Engine::renderer->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &elementPipeline));
	}

	{
		// Get the vertex layout format
		auto bindingDescription = VertexNoNormal::getBindingDescription();
		auto attributeDescriptions = VertexNoNormal::getAttributeDescriptions();

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
		viewport.width = (float)Engine::renderer->renderResolution.width;
		viewport.height = (float)Engine::renderer->renderResolution.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		// Scissor
		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent.width = Engine::renderer->renderResolution.width;
		scissor.extent.height = Engine::renderer->renderResolution.height;

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
		rasterizer.cullMode = VK_CULL_MODE_NONE;//VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		// Multisampling (doesnt work well with deffered renderer without convoluted methods)
		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

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
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f;
		depthStencil.maxDepthBounds = 10.0f;  /// WATCH: whether we need to alter this

		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(glm::fmat4);
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		// Pipeline layout for specifying descriptor sets (shaders use to access buffer and image resources)
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		VkDescriptorSetLayout descSetLayouts[] = { overlayDescriptorSetLayout };
		pipelineLayoutInfo.pSetLayouts = descSetLayouts;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		VK_CHECK_RESULT(vkCreatePipelineLayout(Engine::renderer->device, &pipelineLayoutInfo, nullptr, &combinePipelineLayout));

		// Collate all the data necessary to create pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = Engine::renderer->combineOverlaysShader.getShaderStageCreateInfos();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.layout = combinePipelineLayout;
		pipelineInfo.renderPass = Engine::renderer->overlayRenderer.getRenderPass();
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(Engine::renderer->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &combinePipeline));
	}
}

void OverlayRenderer::createOverlayDescriptorSetLayouts()
{
	VkDescriptorSetLayoutBinding textDescLayoutBinding = {};
	textDescLayoutBinding.binding = 0;
	textDescLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textDescLayoutBinding.descriptorCount = 1;
	textDescLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 1> bindings = { textDescLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(Engine::renderer->device, &layoutInfo, nullptr, &overlayDescriptorSetLayout));
}

void OverlayRenderer::createOverlayDescriptorSets()
{
	combineProjUBO.create(sizeof(glm::fmat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	glm::fmat4 p = glm::ortho<float>(0, 1280, 720, 0, -10, 10);
	glm::mat4 clip(1.0f, 0.0f, 0.0f, 0.0f,
		+0.0f, -1.0f, 0.0f, 0.0f,
		+0.0f, 0.0f, 0.5f, 0.0f,
		+0.0f, 0.0f, 0.5f, 1.0f);
	p = clip * p;
	auto data = combineProjUBO.map();
	memcpy(data, &p[0][0], sizeof(glm::fmat4));
	combineProjUBO.unmap();
}

void OverlayRenderer::createOverlayCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = Engine::renderer->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VK_CHECK_RESULT(vkAllocateCommandBuffers(Engine::renderer->device, &allocInfo, &elementCommandBuffer));
	VK_CHECK_RESULT(vkAllocateCommandBuffers(Engine::renderer->device, &allocInfo, &combineCommandBuffer));
}

void OverlayRenderer::updateOverlayCommands()
{
	/// TODO: if we have a layer that updates every frame, we re-write all commands
	/// Can we avoid this by using secondary command buffers for layers

	Engine::threading->layersMutex.lock();

	bool update = false;
	for (auto l : layers)
	{
		update |= l->needsDrawUpdate();
		l->setUpdated();
	}

	if (!update)
	{
		Engine::threading->layersMutex.unlock();
		return;
	}

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VK_CHECK_RESULT(vkBeginCommandBuffer(elementCommandBuffer, &beginInfo));

	VK_VALIDATE(vkCmdWriteTimestamp(elementCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, Engine::renderer->queryPool, Renderer::BEGIN_OVERLAY));

	for (auto layer : layers)
	{
		if (!layer->doDraw())
			continue;

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = overlayRenderPass;
		renderPassInfo.framebuffer = layer->framebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent.width = layer->resolution.x;
		renderPassInfo.renderArea.extent.height = layer->resolution.y;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0, 0, 0, 0 };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		VK_VALIDATE(vkCmdBeginRenderPass(elementCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE));

		VK_VALIDATE(vkCmdBindPipeline(elementCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, elementPipeline));

		auto& elements = layer->elements;

		VkRect2D scissor = { 0, 0, layer->resolution.x, layer->resolution.y };
		VK_VALIDATE(vkCmdSetScissor(elementCommandBuffer, 0, 1, &scissor));

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)layer->resolution.x;
		viewport.height = (float)layer->resolution.y;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VK_VALIDATE(vkCmdSetViewport(elementCommandBuffer, 0, 1, &viewport));

		for (auto element : elements)
		{
			if (!element->doDraw())
				continue;

			float push[20];
			glm::fmat4 proj = glm::ortho<float>(0, layer->resolution.x, 0, layer->resolution.y, -10, 10);
			memcpy(push, &proj[0][0], sizeof(glm::fmat4));
			float depth = element->getDepth();
			memcpy(push + 16, &depth, sizeof(float));
			VK_VALIDATE(vkCmdPushConstants(elementCommandBuffer, elementPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::fmat4) + sizeof(glm::fvec4), push));

			float push2[5];
			glm::fvec4 c = *(glm::fvec4*)element->getPushConstData();
			memcpy(push2, &c[0], sizeof(glm::fvec4));
			int t = element->getType();
			memcpy(push2 + 4, &t, sizeof(int));
			VK_VALIDATE(vkCmdPushConstants(elementCommandBuffer, elementPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::fmat4) + sizeof(glm::fvec4), sizeof(glm::fvec4) + sizeof(int), push2));

			VkDescriptorSet descSet = element->getDescriptorSet();
			VK_VALIDATE(vkCmdBindDescriptorSets(elementCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, elementPipelineLayout, 0, 1, &descSet, 0, nullptr));

			element->render(elementCommandBuffer);
		}

		VK_VALIDATE(vkCmdEndRenderPass(elementCommandBuffer));
	}

	VK_VALIDATE(vkCmdWriteTimestamp(elementCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, Engine::renderer->queryPool, Renderer::END_OVERLAY));

	VK_CHECK_RESULT(vkEndCommandBuffer(elementCommandBuffer));


	VK_CHECK_RESULT(vkBeginCommandBuffer(combineCommandBuffer, &beginInfo));

	VK_VALIDATE(vkCmdWriteTimestamp(combineCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, Engine::renderer->queryPool, Renderer::BEGIN_OVERLAY_COMBINE));

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = overlayRenderPass;
	renderPassInfo.framebuffer = framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent.width = Engine::renderer->renderResolution.width;
	renderPassInfo.renderArea.extent.height = Engine::renderer->renderResolution.height;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0, 0, 0, 0 };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	VK_VALIDATE(vkCmdBeginRenderPass(combineCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE));

	VK_VALIDATE(vkCmdBindPipeline(combineCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, combinePipeline));

	glm::fmat4 proj = glm::ortho<float>(0, Engine::window->resX, Engine::window->resY, 0, -10, 10);
	glm::mat4 clip(1.0f, 0.0f, 0.0f, 0.0f,
		+0.0f, -1.0f, 0.0f, 0.0f,
		+0.0f, 0.0f, 0.5f, 0.0f,
		+0.0f, 0.0f, 0.5f, 1.0f);
	proj = clip * proj;
	VK_VALIDATE(vkCmdPushConstants(combineCommandBuffer, combinePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::fmat4), &proj));

	for (auto layer : layers)
	{
		if (!layer->doDraw())
			continue;

		VK_VALIDATE(vkCmdBindDescriptorSets(combineCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, combinePipelineLayout, 0, 1, &layer->imageDescriptor, 0, nullptr));

		VkBuffer vertexBuffers[] = { layer->quadBuffer.getBuffer() };
		VkDeviceSize offsets[] = { 0 };
		VK_VALIDATE(vkCmdBindVertexBuffers(combineCommandBuffer, 0, 1, vertexBuffers, offsets));

		VK_VALIDATE(vkCmdDraw(combineCommandBuffer, 6, 1, 0, 0));
	}

	VK_VALIDATE(vkCmdEndRenderPass(combineCommandBuffer));

	VK_VALIDATE(vkCmdWriteTimestamp(combineCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, Engine::renderer->queryPool, Renderer::END_OVERLAY_COMBINE));

	VK_CHECK_RESULT(vkEndCommandBuffer(combineCommandBuffer));

	Engine::threading->layersMutex.unlock();
}

void OverlayRenderer::destroyOverlayCommands()
{
	VK_VALIDATE(vkFreeCommandBuffers(Engine::renderer->device, Engine::renderer->commandPool, 1, &elementCommandBuffer));
	VK_VALIDATE(vkFreeCommandBuffers(Engine::renderer->device, Engine::renderer->commandPool, 1, &combineCommandBuffer));
}

void OverlayRenderer::destroyOverlayDescriptorSetLayouts()
{
	VK_VALIDATE(vkDestroyDescriptorSetLayout(Engine::renderer->device, overlayDescriptorSetLayout, 0));
}

void OverlayRenderer::destroyOverlayRenderPass()
{
	VK_VALIDATE(vkDestroyRenderPass(Engine::renderer->device, overlayRenderPass, 0));
}

void OverlayRenderer::destroyOverlayPipeline()
{
	VK_VALIDATE(vkDestroyPipelineLayout(Engine::renderer->device, combinePipelineLayout, 0));
	VK_VALIDATE(vkDestroyPipeline(Engine::renderer->device, combinePipeline, 0));

	VK_VALIDATE(vkDestroyPipelineLayout(Engine::renderer->device, elementPipelineLayout, 0));
	VK_VALIDATE(vkDestroyPipeline(Engine::renderer->device, elementPipeline, 0));
}

void OverlayRenderer::destroyOverlayAttachmentsFramebuffers()
{
	combinedLayers.destroy();
	combinedLayersDepth.destroy();
	VK_VALIDATE(vkDestroyFramebuffer(Engine::renderer->device, framebuffer, 0));
}
