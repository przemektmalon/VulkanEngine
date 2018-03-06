#include "PCH.hpp"
#include "Renderer.hpp"
#include "Window.hpp"
#include "VulkanWrappers.hpp"
#include "File.hpp"
#include "Shader.hpp"

/*
	@brief	Initialise renderer and Vulkan objects
*/
void Renderer::initialise()
{
	initVulkanLogicalDevice();
	initVulkanSwapChain();
	initVulkanImageViews();
	initVulkanRenderPass();
	initVulkanDescriptorSetLayout();
	initVulkanGraphicsPipeline();
	initVulkanFramebuffers();
	initVulkanCommandPool();
	initVulkanIndexBuffer();
	initVulkanVertexBuffer();
	initVulkanUniformBuffer();
	initVulkanDescriptorPool();
	initVulkanDescriptorSet();
	initVulkanCommandBuffers();
	initVulkanSemaphores();
}

/*
	@brief	Cleanup Vulkan objects
*/
void Renderer::cleanup()
{
	cleanupVulkanSwapChain();
	vkDestroyDescriptorPool(vkDevice, vkDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, nullptr);
	vkDestroyBuffer(vkDevice, vkUniformBuffer, nullptr);
	vkFreeMemory(vkDevice, vkUniformBufferMemory, nullptr);
	vkDestroyBuffer(vkDevice, vkIndexBuffer, nullptr);
	vkFreeMemory(vkDevice, vkIndexBufferMemory, nullptr);
	vkDestroyBuffer(vkDevice, vkVertexBuffer, nullptr);
	vkFreeMemory(vkDevice, vkVertexBufferMemory, nullptr);
	vkDestroySemaphore(vkDevice, renderFinishedSemaphore, 0);
	vkDestroySemaphore(vkDevice, imageAvailableSemaphore, 0);
	vkDestroyCommandPool(vkDevice, vkCommandPool, 0);
	vkDestroyDevice(vkDevice, 0);
}

/*
	@brief	Render
*/
void Renderer::render()
{
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(vkDevice, vkSwapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result != VK_SUCCESS)
	{
		DBG_SEVERE("Could not acquire next image");
		//cleanupVulkanSwapChain();
		//recreateVulkanSwapChain();
		return;
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vkCommandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		DBG_SEVERE("Failed to submit Vulkan draw command buffer");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { vkSwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(vkPresentQueue, &presentInfo);

	vkQueueWaitIdle(vkPresentQueue);
}

/*
	@brief	Creates a Vulkan logical device (from the optimal physical device) with specific features and queues
*/
void Renderer::initVulkanLogicalDevice()
{
	DBG_INFO("Creating Vulkan logical device");
	
	// Initialise one queue for our device which will be used for everything (rendering presenting transferring operations)
	VkDeviceQueueCreateInfo qci = {};
	qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	qci.pNext = 0;
	qci.flags = 0;
	qci.queueFamilyIndex = 0;
	qci.queueCount = 1;
	float priority = 1.f;
	qci.pQueuePriorities = &priority;

	VkPhysicalDeviceFeatures pdf = {};

	VkDeviceCreateInfo dci = {};
	dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	dci.pNext = 0;
	dci.flags = 0;
	dci.queueCreateInfoCount = 1;
	dci.pQueueCreateInfos = &qci;
	// Enable validation for vulkan calls, for debugging
#ifdef ENABLE_VULKAN_VALIDATION
	dci.enabledLayerCount = 1;
	auto layerName = "VK_LAYER_LUNARG_standard_validation";
	dci.ppEnabledLayerNames = &layerName;
#else
	dci.enabledLayerCount = 0;
	dci.ppEnabledLayerNames = 0;
#endif
	dci.enabledExtensionCount = 1;
	auto deviceExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME; /// Todo: check support function
	dci.ppEnabledExtensionNames = &deviceExtension;
	dci.pEnabledFeatures = &pdf;

	if (vkCreateDevice(Engine::vkPhysicalDevice, &dci, 0, &vkDevice) != VK_SUCCESS)
		DBG_SEVERE("Could not create Vulkan logical device");

	vkGetDeviceQueue(vkDevice, 0, 0, &vkGraphicsQueue);
	vkGetDeviceQueue(vkDevice, 0, 0, &vkPresentQueue); /// Todo: Support for AMD gpus which have non-universal queues
}

/*
	@brief	Create the optimal vulkan swapchain
*/
void Renderer::initVulkanSwapChain()
{
	DBG_INFO("Creating Vulkan swap chain");

	auto& device = Engine::getPhysicalDeviceDetails();

	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	VkExtent2D extent;

	// Find a suitable image format for drawing (ie. GL_RGBA8)
	if (device.swapChainDetails.formats.size() == 1 && device.swapChainDetails.formats[0].format == VK_FORMAT_UNDEFINED) {
		// If the only format returned by query is undefined then we can choose any format we want
		surfaceFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}
	else
	{
		// If the query returns a list of formats choose the one we want (RGBA8)
		bool foundSuitable = false;
		for (const auto& availableFormat : device.swapChainDetails.formats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				surfaceFormat = availableFormat;
				foundSuitable = true;
			}
		}
		// If the device doesn't support the one we want choose the first one returned
		if (!foundSuitable)
			surfaceFormat = device.swapChainDetails.formats[0];
	}

	// Choose a suitable present mode (Mailbox => double or triple buffering
	for (const auto& availablePresentMode : device.swapChainDetails.presentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = availablePresentMode;
			break;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			presentMode = availablePresentMode;
		}
	}

	// Choose a resolution for our drawing image
	if (device.swapChainDetails.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		// If we have a current extent (ie. the size of our surface as filled in when we queried swap chain capabilities) choose that
		extent = device.swapChainDetails.capabilities.currentExtent;
	}
	else {
		// If we dont have a current extent then choose the highest one supported
		VkExtent2D actualExtent = { Engine::window->resX, Engine::window->resY };

		actualExtent.width = std::max(device.swapChainDetails.capabilities.minImageExtent.width, std::min(device.swapChainDetails.capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(device.swapChainDetails.capabilities.minImageExtent.height, std::min(device.swapChainDetails.capabilities.maxImageExtent.height, actualExtent.height));

		extent = actualExtent;
	}

	// Choose the number of images we'll have in the swap chain (2 or 3 == double or triple buffering)
	u32 imageCount = device.swapChainDetails.capabilities.minImageCount + 1;
	if (device.swapChainDetails.capabilities.maxImageCount > 0 && imageCount > device.swapChainDetails.capabilities.maxImageCount) {
		imageCount = device.swapChainDetails.capabilities.maxImageCount;
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

	createInfo.preTransform = device.swapChainDetails.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(vkDevice, &createInfo, nullptr, &vkSwapChain) != VK_SUCCESS) {
		DBG_SEVERE("Failed to create Vulkan swap chain!");
	}

	vkGetSwapchainImagesKHR(vkDevice, vkSwapChain, &imageCount, nullptr);
	vkSwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(vkDevice, vkSwapChain, &imageCount, vkSwapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

/*
	@brief	Create image views for swap chain images
*/
void Renderer::initVulkanImageViews()
{
	DBG_INFO("Creating Vulkan image views");
	vkSwapChainImageViews.resize(vkSwapChainImages.size());

	// Views describe how the GPU will write to or sample the image (ie. mip mapping)
	for (size_t i = 0; i < vkSwapChainImages.size(); i++) {
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = vkSwapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(vkDevice, &createInfo, nullptr, &vkSwapChainImageViews[i]) != VK_SUCCESS) {
			DBG_SEVERE("Failed to create vulkan image views");
		}
	}
}

/*
	@brief	Create a Vulkan render pass
*/
void Renderer::initVulkanRenderPass()
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
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(vkDevice, &renderPassInfo, nullptr, &vkRenderPass) != VK_SUCCESS) {
		DBG_SEVERE("Failed to create render pass");
	}
}

/*
	@brief	Create a Vulkan descriptor set for shaders extracting data from buffers
*/
void Renderer::initVulkanDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &vkDescriptorSetLayout) != VK_SUCCESS) {
		DBG_SEVERE("Failed to create Vulkan descriptor set layout");
	}
}

/*
	@brief	Load shaders and create Vulkan graphics pipeline
*/
void Renderer::initVulkanGraphicsPipeline()
{
	DBG_INFO("Creating vulkan graphics pipeline");

	// Compile GLSL code to SPIR-V

	ShaderModule sh(ShaderModule::Vertex, "/shaders/square.glsl");
	sh.compile();
	sh.createVulkanModule();

	ShaderModule sh2(ShaderModule::Fragment, "/shaders/square.glsl");
	sh2.compile();
	sh2.createVulkanModule();

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = sh.getVkModule();
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = sh2.getVkModule();
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

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
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// Scissor
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

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

	// Pipeline layout for specifying descriptor sets (shaders use to access buffer and image resources indirectly)
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &vkDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &vkPipelineLayout) != VK_SUCCESS) {
		DBG_SEVERE("Failed to create Vulkan pipeline layout");
	}

	// Collate all the data necessary to create pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = vkPipelineLayout;
	pipelineInfo.renderPass = vkRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkResult result = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkPipeline);

	if (result != VK_SUCCESS) {
		DBG_SEVERE("Failed to create Vulkan graphics pipeline");
	}

	// Pipeline saves the shader modules we can delete them
	//vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
	//vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
}

/*
	@brief	Create Vulkan framebuffers
*/
void Renderer::initVulkanFramebuffers()
{
	DBG_INFO("Creating Vulkan framebuffers");

	vkFramebuffers.resize(vkSwapChainImageViews.size());

	// Create a framebuffer for each image in our swap chain (two if double buffering, etc)
	// If we have more attachments for normal and pbr data we will attach more than one image to each framebuffer
	for (size_t i = 0; i < vkSwapChainImageViews.size(); i++) {
		VkImageView attachments[] = {
			vkSwapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = vkRenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(vkDevice, &framebufferInfo, nullptr, &vkFramebuffers[i]) != VK_SUCCESS) {
			DBG_SEVERE("Failed to create Vulkan framebuffers");
		}
	}
}

/*
	@brief	Create Vulkan command pool for submitting commands to a particular queue
*/
void Renderer::initVulkanCommandPool()
{
	DBG_INFO("Creating Vulkan command pool");

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = 0;

	if (vkCreateCommandPool(vkDevice, &poolInfo, nullptr, &vkCommandPool) != VK_SUCCESS) {
		DBG_SEVERE("Failed to create Vulkan command pool");
	}
}

/*
	@brief	Create Vulkan vertex buffer
*/
void Renderer::initVulkanVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	createVulkanBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vkStagingBuffer, vkStagingBufferMemory);

	void* data;
	vkMapMemory(vkDevice, vkStagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(vkDevice, vkStagingBufferMemory);

	createVulkanBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkVertexBuffer, vkVertexBufferMemory);

	copyVulkanBuffer(vkStagingBuffer, vkVertexBuffer, bufferSize);

	vkDestroyBuffer(vkDevice, vkStagingBuffer, 0);
	vkFreeMemory(vkDevice, vkStagingBufferMemory, 0);
}

/*
	@brief	Create Vulkan index buffer
*/
void Renderer::initVulkanIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createVulkanBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(vkDevice, stagingBufferMemory);

	createVulkanBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkIndexBuffer, vkIndexBufferMemory);

	copyVulkanBuffer(stagingBuffer, vkIndexBuffer, bufferSize);

	vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
	vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);
}

/*
	@brief	Create vulkan uniform buffer for MVP matrices
*/
void Renderer::initVulkanUniformBuffer()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	createVulkanBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vkUniformBuffer, vkUniformBufferMemory);
}

/*
	@brief	Create Vulkan descriptor pool
*/
void Renderer::initVulkanDescriptorPool()
{
	VkDescriptorPoolSize poolSize = {};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = 1;

	if (vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &vkDescriptorPool) != VK_SUCCESS) {
		DBG_SEVERE("Failed to create Vulkan descriptor pool");
	}
}

/*
	@brief	Create Vulkan descriptor set
*/
void Renderer::initVulkanDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { vkDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = vkDescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(vkDevice, &allocInfo, &vkDescriptorSet) != VK_SUCCESS) {
		DBG_SEVERE("Failed to allocate descriptor set");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = vkUniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = vkDescriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(vkDevice, 1, &descriptorWrite, 0, nullptr);
}

/*
	@brief	Create Vulkan command buffers
*/
void Renderer::initVulkanCommandBuffers()
{
	DBG_INFO("Creating Vulkan command buffers");

	// We need a command buffer for each framebuffer
	vkCommandBuffers.resize(vkFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = vkCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)vkCommandBuffers.size();

	if (vkAllocateCommandBuffers(vkDevice, &allocInfo, vkCommandBuffers.data()) != VK_SUCCESS) {
		DBG_SEVERE("Failed to allocate Vulkan command buffers");
	}

	for (size_t i = 0; i < vkCommandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		vkBeginCommandBuffer(vkCommandBuffers[i], &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = vkRenderPass;
		renderPassInfo.framebuffer = vkFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(vkCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);

		vkCmdBindDescriptorSets(vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineLayout, 0, 1, &vkDescriptorSet, 0, nullptr);

		VkBuffer vertexBuffers[] = { vkVertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(vkCommandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(vkCommandBuffers[i], vkIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

		vkCmdDrawIndexed(vkCommandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		//vkCmdDraw(vkCommandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);

		vkCmdEndRenderPass(vkCommandBuffers[i]);

		if (vkEndCommandBuffer(vkCommandBuffers[i]) != VK_SUCCESS) {
			DBG_SEVERE("Failed to record Vulkan command buffer");
		}
	}
}

/*
	@brief	Create Vulkan semaphores
*/
void Renderer::initVulkanSemaphores()
{
	DBG_INFO("Creating Vulkan semaphores");
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {

		DBG_SEVERE("Failed to create Vulkan semaphores");
	}
}

/*
	@brief	Create a vulkan buffer with the specified size, usage and properties
*/
void Renderer::createVulkanBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		DBG_SEVERE("Failed to create Vulkan buffer");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(vkDevice, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = Engine::getPhysicalDeviceDetails().getMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		DBG_SEVERE("Failed to allocate Vulkan buffer memory");
	}

	vkBindBufferMemory(vkDevice, buffer, bufferMemory, 0);
}

/*
	@brief	Copy a vulkan buffer
*/
void Renderer::copyVulkanBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = vkCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(vkDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vkGraphicsQueue);

	vkFreeCommandBuffers(vkDevice, vkCommandPool, 1, &commandBuffer);
}

/*
	@brief	Update transforms
*/
void Renderer::updateUniformBuffer()
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(vkDevice, vkUniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(vkDevice, vkUniformBufferMemory);
}

/*
	@brief	Clean up Vulkan swapchain
*/
void Renderer::cleanupVulkanSwapChain()
{
	for (auto framebuffer : vkFramebuffers) {
		vkDestroyFramebuffer(vkDevice, framebuffer, nullptr);
	}

	vkFreeCommandBuffers(vkDevice, vkCommandPool, u32(vkCommandBuffers.size()), vkCommandBuffers.data());
	vkDestroyPipeline(vkDevice, vkPipeline, nullptr);
	vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, nullptr);
	vkDestroyRenderPass(vkDevice, vkRenderPass, nullptr);

	for (auto imageView : vkSwapChainImageViews) {
		vkDestroyImageView(vkDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(vkDevice, vkSwapChain, nullptr);
}

/*
	@brief	Recreate Vulkan swap chain
	@note	Call swapchain cleanup function before doing this
*/
void Renderer::recreateVulkanSwapChain()
{
	vkDeviceWaitIdle(vkDevice);

	initVulkanSwapChain();
	initVulkanImageViews();
	initVulkanRenderPass();
	initVulkanGraphicsPipeline();
	initVulkanFramebuffers();
	initVulkanCommandBuffers();
}
