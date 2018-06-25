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
	//Engine::queryVulkanPhysicalDeviceDetails();
	//auto& deviceDets = Engine::getPhysicalDeviceDetails();
	const auto physicalDevice = Engine::physicalDevice;

	auto& surfaceCapabilities = physicalDevice->getSurfaceCapabilities();
	auto& surfaceFormats = physicalDevice->getSurfaceFormats();
	auto& presentModes = physicalDevice->getPresentModes();


	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	VkExtent2D extent;

	// Find a suitable image format for drawing (ie. GL_RGBA8)
	if (surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
		// If the only format returned by query is undefined then we can choose any format we want
		surfaceFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}
	else
	{
		// If the query returns a list of formats choose the one we want (RGBA8)
		bool foundSuitable = false;
		for (const auto& availableFormat : surfaceFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				surfaceFormat = availableFormat;
				foundSuitable = true;
			}
		}
		// If the device doesn't support the one we want choose the first one returned
		if (!foundSuitable)
			surfaceFormat = surfaceFormats[0];
	}

	// Choose a suitable present mode (Mailbox => double or triple buffering
	for (const auto& availablePresentMode : presentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = availablePresentMode;
			break;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			presentMode = availablePresentMode;
		}
	}

	// Choose a resolution for our drawing image
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		// If we have a current extent (ie. the size of our surface as filled in when we queried swap chain capabilities) choose that
		extent = surfaceCapabilities.currentExtent;
	}
	else {
		// If we dont have a current extent then choose the highest one supported
		VkExtent2D actualExtent = { Engine::window->resX, Engine::window->resY };

		actualExtent.width = std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, actualExtent.height));

		extent = actualExtent;
	}

	// Choose the number of images we'll have in the swap chain (2 or 3 == double or triple buffering)
	u32 imageCount = surfaceCapabilities.minImageCount + 1;
	if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
		imageCount = surfaceCapabilities.maxImageCount;
	}

	// Fill in the create struct for our swap chain with the collected information
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = Engine::window->vkSurface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0; // Optional
	createInfo.pQueueFamilyIndices = nullptr; // Optional

											  // If we have several queue families then we will need to look into sharing modes
											  /*
											  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
											  uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

											  if (indices.graphicsFamily != indices.presentFamily) {
											  createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
											  createInfo.queueFamilyIndexCount = 2;
											  createInfo.pQueueFamilyIndices = queueFamilyIndices;
											  }
											  else {
											  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
											  }
											  */

	createInfo.preTransform = surfaceCapabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain));

	VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr));
	swapChainImages.resize(imageCount);
	VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data()));

	swapChainImageFormat = surfaceFormat.format;
	renderResolution = extent;
}

void Renderer::createScreenAttachments()
{
	swapChainImageViews.resize(swapChainImages.size());

	// Views describe how the GPU will write to or sample the image (ie. mip mapping)
	for (uint32_t i = 0; i < swapChainImages.size(); i++) {

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = swapChainImages[i];
		viewInfo.format = swapChainImageFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		
		VK_CHECK_RESULT(vkCreateImageView(logicalDevice.getHandle(), &viewInfo, nullptr, &swapChainImageViews[i]));
	}
}

void Renderer::createScreenRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = 0;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &screenRenderPass));
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
	pipelineInfo.renderPass = screenRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.flags = 0;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &screenPipeline));
}

void Renderer::createScreenFramebuffers()
{
	screenFramebuffers.resize(swapChainImageViews.size());

	// Create a framebuffer for each image in our swap chain (two if double buffering, etc)
	// If we have more attachments for normal and pbr data we will attach more than one image to each framebuffer
	for (size_t i = 0; i < swapChainImageViews.size(); i++) {

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = screenRenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &swapChainImageViews[i];
		framebufferInfo.width = renderResolution.width;
		framebufferInfo.height = renderResolution.height;
		framebufferInfo.layers = 1;

		VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &screenFramebuffers[i]));
	}
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
	screenCommandBuffers.allocate(&logicalDevice, &commandPool, screenFramebuffers.size());
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
		renderPassInfo.renderPass = screenRenderPass;
		renderPassInfo.framebuffer = screenFramebuffers[i];
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
		renderPassInfo.renderPass = screenRenderPass;
		renderPassInfo.framebuffer = screenFramebuffers[i];
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
	VK_VALIDATE(vkDestroySwapchainKHR(device, swapChain, 0));
}

void Renderer::destroyScreenAttachments()
{
	for (int i = 0; i < swapChainImages.size(); ++i)
	{
		// Gives error, maybe because these images are created from the swap chain by KHR extension
		//vkDestroyImage(device, swapChainImages[i], 0);
		VK_VALIDATE(vkDestroyImageView(device, swapChainImageViews[i], 0));
	}
}

void Renderer::destroyScreenRenderPass()
{
	VK_VALIDATE(vkDestroyRenderPass(device, screenRenderPass, 0));
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

void Renderer::destroyScreenFramebuffers()
{
	for (auto fb : screenFramebuffers)
		VK_VALIDATE(vkDestroyFramebuffer(device, fb, 0));
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
