#include "PCH.hpp"
#include "Renderer.hpp"
#include "Window.hpp"
#include "VulkanWrappers.hpp"

/*
	@brief	Initialise renderer and Vulkan objects
*/
void Renderer::initialise()
{
	initVulkanLogicalDevice();
	initVulkanSwapChain();
	initVulkanImageViews();
	initVulkanRenderPass();
	initVulkanGraphicsPipeline();
	initVulkanFramebuffers();
	initVulkanCommandPool();
	initVulkanCommandBuffers();
	initVulkanSemaphores();
}

/*
	@brief	Cleanup Vulkan objects
*/
void Renderer::cleanup()
{
	cleanupVulkanSwapChain();
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

	if (device.swapChainDetails.formats.size() == 1 && device.swapChainDetails.formats[0].format == VK_FORMAT_UNDEFINED) {
		surfaceFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}
	else
	{
		bool foundSuitable = false;
		for (const auto& availableFormat : device.swapChainDetails.formats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				surfaceFormat = availableFormat;
				foundSuitable = true;
			}
		}
		if (!foundSuitable)
			surfaceFormat = device.swapChainDetails.formats[0];
	}

	for (const auto& availablePresentMode : device.swapChainDetails.presentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = availablePresentMode;
			break;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			presentMode = availablePresentMode;
		}
	}

	if (device.swapChainDetails.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		extent = device.swapChainDetails.capabilities.currentExtent;
	}
	else {
		VkExtent2D actualExtent = { Engine::window->resX, Engine::window->resY };

		actualExtent.width = std::max(device.swapChainDetails.capabilities.minImageExtent.width, std::min(device.swapChainDetails.capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(device.swapChainDetails.capabilities.minImageExtent.height, std::min(device.swapChainDetails.capabilities.maxImageExtent.height, actualExtent.height));

		extent = actualExtent;
	}

	u32 imageCount = device.swapChainDetails.capabilities.minImageCount + 1;
	if (device.swapChainDetails.capabilities.maxImageCount > 0 && imageCount > device.swapChainDetails.capabilities.maxImageCount) {
		imageCount = device.swapChainDetails.capabilities.maxImageCount;
	}

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

											  /*QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
											  uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

											  if (indices.graphicsFamily != indices.presentFamily) {
											  createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
											  createInfo.queueFamilyIndexCount = 2;
											  createInfo.pQueueFamilyIndices = queueFamilyIndices;
											  }
											  else {
											  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
											  }*/

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

static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

/*
	@brief	Load shaders and create Vulkan graphics pipeline
*/
void Renderer::initVulkanGraphicsPipeline()
{
	DBG_INFO("Creating vulkan graphics pipeline");

	auto vertShaderCode = readFile("shaders/vert.spv");
	auto fragShaderCode = readFile("shaders/frag.spv");

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

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

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &vkPipelineLayout) != VK_SUCCESS) {
		DBG_SEVERE("Failed to create Vulkan pipeline layout");
	}

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

	vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
}

/*
	@brief	Create Vulkan framebuffers
*/
void Renderer::initVulkanFramebuffers()
{
	DBG_INFO("Creating Vulkan framebuffers");

	vkFramebuffers.resize(vkSwapChainImageViews.size());

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
	@brief	Create Vulkan command pool
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
	@brief	Create Vulkan command buffers
*/
void Renderer::initVulkanCommandBuffers()
{
	DBG_INFO("Creating Vulkan command buffers");

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

		vkCmdDraw(vkCommandBuffers[i], 3, 1, 0, 0);

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
	@brief	Create Vulkan shader module
*/
VkShaderModule Renderer::createShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(vkDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		DBG_SEVERE("Failed to create shader module");
	}

	return shaderModule;
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
