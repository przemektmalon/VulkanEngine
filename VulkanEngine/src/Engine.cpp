#include "PCH.hpp"
#include "Engine.hpp"
#include "Window.hpp"

/*
	@brief	Initialise enigne and sub-systems
*/
void Engine::start()
{
	DBG_INFO("Launching engine");
#ifdef _WIN32
	win32InstanceHandle = GetModuleHandle(NULL);
#endif

	initVulkanInstance();
	createWindow();
	pickVulkanPhysicalDevice();
	initVulkanLogicalDevice();
	initVulkanSwapChain();
	initVulkanImageViews();
	initVulkanRenderPass();
	initVulkanGraphicsPipeline();
	initVulkanFramebuffers();
	initVulkanCommandPool();
	initVulkanCommandBuffers();
	initVulkanSemaphores();

	while (engineRunning)
	{
		while (window->processMessages()) { /* Invoke timer ? */ }
		
		// Rendering and engine logic

		render();
	}

	quit();
}

void Engine::render()
{
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(vkDevice, vkSwapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result != VK_SUCCESS)
	{
		int x = 10;
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
	@brief	Create a window for drawing to
*/
void Engine::createWindow()
{
	DBG_INFO("Creating window");
	// Window creator requires OS specific handles
	WindowCreateInfo wci;
#ifdef _WIN32
	wci.win32InstanceHandle = win32InstanceHandle;
#endif
	wci.height = 720;
	wci.width = 1280;
	wci.posX = 100;
	wci.posY = 100;
	wci.title = "Vulkan Engine";
	wci.borderless = false;

	window = new Window;
	window->create(&wci);
}

/*
	@brief	Creates vulkan instance, enables extensions
	@note	If ENABLE_VULKAN_VALIDATION is defined, adds lunarg validation layer 
*/
void Engine::initVulkanInstance()
{
	DBG_INFO("Creating vulkan instance");
	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = 0;
	appInfo.apiVersion = VK_API_VERSION_1_0;
	appInfo.pApplicationName = "VulkanEngine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "My Vulkan Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

	std::vector<const char *> enabledExtensions;
	enabledExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#ifdef ENABLE_VULKAN_VALIDATION
	enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

	VkInstanceCreateInfo instInfo;
	instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instInfo.pNext = 0;
	instInfo.pApplicationInfo = &appInfo;
	instInfo.flags = 0;
	instInfo.enabledExtensionCount = enabledExtensions.size();
	instInfo.ppEnabledExtensionNames = &enabledExtensions[0];
#ifdef ENABLE_VULKAN_VALIDATION
	instInfo.enabledLayerCount = 1;
	auto layerName = "VK_LAYER_LUNARG_standard_validation";
	instInfo.ppEnabledLayerNames = &layerName;
#elif
	instInfo.enabledLayerCount = 0;
	instInfo.ppEnabledLayerNames = 0;
#endif

	if (vkCreateInstance(&instInfo, nullptr, &vkInstance) != VK_SUCCESS)
		DBG_SEVERE("Could not create Vulkan instance");

#ifdef ENABLE_VULKAN_VALIDATION
	DBG_INFO("Creating validation layer debug callback");
	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = debugCallbackFunc;

	auto createDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vkInstance, "vkCreateDebugReportCallbackEXT");

	if (createDebugReportCallbackEXT(vkInstance, &createInfo, nullptr, &debugCallbackInfo) != VK_SUCCESS) {
		DBG_WARNING("Failed to create debug callback");
	}
#endif
}

/*
	@brief	Queries vulkan instance for physical device information, picks best one
*/
void Engine::pickVulkanPhysicalDevice()
{
	DBG_INFO("Picking Vulkan device")
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);

	if (deviceCount == 0)
		DBG_SEVERE("No GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

	for (const auto& device : devices) 
	{
		bool deviceSuitable = false;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;

		for (const auto& queueFamily : queueFamilies)
		{
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, window->vkSurface, &presentSupport);
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupport)
			{
				deviceSuitable = true;
				break;
			}
			++i;
		}

		if (deviceSuitable)
		{
			vkPhysicalDevice = device;
			break;
		}
	}

	if (vkPhysicalDevice == VK_NULL_HANDLE)
		DBG_SEVERE("Faled to find a suitable GPU");
}

/*
	@brief	Creates a Vulkan logical device (from the *best* physical device) with specific features and queues
*/
void Engine::initVulkanLogicalDevice()
{
	DBG_INFO("Creating Vulkan logical device");
	VkDeviceQueueCreateInfo qci;
	qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	qci.pNext = 0;
	qci.flags = 0;
	qci.queueFamilyIndex = 0;
	qci.queueCount = 1;
	float priority = 1.f;
	qci.pQueuePriorities = &priority;

	VkPhysicalDeviceFeatures pdf = {};

	VkDeviceCreateInfo dci;
	dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	dci.pNext = 0;
	dci.flags = 0;
	dci.queueCreateInfoCount = 1;
	dci.pQueueCreateInfos = &qci;
#ifdef ENABLE_VULKAN_VALIDATION
	dci.enabledLayerCount = 1;
	auto layerName = "VK_LAYER_LUNARG_standard_validation";
	dci.ppEnabledLayerNames = &layerName;
#elif
	dci.enabledLayerCount = 0;
	dci.ppEnabledLayerNames = 0;
#endif
	dci.enabledExtensionCount = 1;
	auto deviceExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME; /// Todo: check support function
	dci.ppEnabledExtensionNames = &deviceExtension;
	dci.pEnabledFeatures = &pdf;

	if (vkCreateDevice(vkPhysicalDevice, &dci, 0, &vkDevice) != VK_SUCCESS)
		DBG_SEVERE("Could not create Vulkan logical device");

	vkGetDeviceQueue(vkDevice, 0, 0, &vkGraphicsQueue);
	vkGetDeviceQueue(vkDevice, 0, 0, &vkPresentQueue); /// Todo: Support for AMD gpus which have non-universal queues
}

/*
	@brief	Create the optimal vulkan swapchain
*/
void Engine::initVulkanSwapChain()
{
	DBG_INFO("Creating Vulkan swap chain");
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, window->vkSurface, &details.capabilities);

	u32 formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, window->vkSurface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, window->vkSurface, &formatCount, details.formats.data());
	}
	else
	{
		DBG_SEVERE("Device not suitable (swap chain format count is 0)");
	}

	u32 presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, window->vkSurface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, window->vkSurface, &presentModeCount, details.presentModes.data());
	}
	else
	{
		DBG_SEVERE("Device not suitable (swap chain present modes count is 0)");
	}

	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	VkExtent2D extent;

	if (details.formats.size() == 1 && details.formats[0].format == VK_FORMAT_UNDEFINED) {
		surfaceFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}
	else
	{
		bool foundSuitable = false;
		for (const auto& availableFormat : details.formats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				surfaceFormat = availableFormat;
				foundSuitable = true;
			}
		}
		if (!foundSuitable)
			surfaceFormat = details.formats[0];
	}

	for (const auto& availablePresentMode : details.presentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = availablePresentMode;
			break;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			presentMode = availablePresentMode;
		}
	}

	if (details.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		extent = details.capabilities.currentExtent;
	}
	else {
		VkExtent2D actualExtent = { window->resX, window->resY };

		actualExtent.width = std::max(details.capabilities.minImageExtent.width, std::min(details.capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(details.capabilities.minImageExtent.height, std::min(details.capabilities.maxImageExtent.height, actualExtent.height));

		extent = actualExtent;
	}

	u32 imageCount = details.capabilities.minImageCount + 1;
	if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount) {
		imageCount = details.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = window->vkSurface;

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

	createInfo.preTransform = details.capabilities.currentTransform;
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
void Engine::initVulkanImageViews()
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

void Engine::initVulkanRenderPass()
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

void Engine::initVulkanGraphicsPipeline() 
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

void Engine::initVulkanFramebuffers()
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

void Engine::initVulkanCommandPool()
{
	DBG_INFO("Creating Vulkan command pool");

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = 0;

	if (vkCreateCommandPool(vkDevice, &poolInfo, nullptr, &vkCommandPool) != VK_SUCCESS) {
		DBG_SEVERE("Failed to create Vulkan command pool");
	}
}

void Engine::initVulkanCommandBuffers()
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

void Engine::initVulkanSemaphores()
{
	DBG_INFO("Creating Vulkan semaphores");
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {

		DBG_SEVERE("Failed to create Vulkan semaphores");
	}
}

VkShaderModule Engine::createShaderModule(const std::vector<char>& code)
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
	@brief	Cleanup memory on exit
*/
void Engine::quit()
{
	DBG_INFO("Exiting");

	vkDestroyCommandPool(vkDevice, vkCommandPool, nullptr);

	for (auto framebuffer : vkFramebuffers) {
		vkDestroyFramebuffer(vkDevice, framebuffer, nullptr);
	}

	vkDestroyPipeline(vkDevice, vkPipeline, nullptr);
	vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, nullptr);
	vkDestroyRenderPass(vkDevice, vkRenderPass, nullptr);

	for (auto imageView : vkSwapChainImageViews) {
		vkDestroyImageView(vkDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(vkDevice, vkSwapChain, nullptr);
	window->destroy();
	vkDestroyDevice(vkDevice, nullptr);
#ifdef ENABLE_VULKAN_VALIDATION
	PFN_vkDestroyDebugReportCallbackEXT(vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugReportCallbackEXT"))(vkInstance, debugCallbackInfo, 0);
#endif
	vkDestroyInstance(vkInstance, nullptr);
}

#ifdef ENABLE_VULKAN_VALIDATION
VKAPI_ATTR VkBool32 VKAPI_CALL Engine::debugCallbackFunc(VkDebugReportFlagsEXT flags,
													VkDebugReportObjectTypeEXT objType,
													uint64_t obj,
													size_t location,
													int32_t code,
													const char* layerPrefix,
													const char* msg,
													void* userData)
{
	DBG_WARNING(msg);
	return VK_FALSE;
}
VkDebugReportCallbackEXT Engine::debugCallbackInfo;
#endif

#ifdef _WIN32
HINSTANCE Engine::win32InstanceHandle;
#endif
Window* Engine::window;
VkInstance Engine::vkInstance;
VkPhysicalDevice Engine::vkPhysicalDevice = VK_NULL_HANDLE;
VkDevice Engine::vkDevice;
VkQueue Engine::vkGraphicsQueue;
VkQueue Engine::vkPresentQueue;
VkSwapchainKHR Engine::vkSwapChain;
VkPipeline Engine::vkPipeline;
VkPipelineLayout Engine::vkPipelineLayout;
VkRenderPass Engine::vkRenderPass;
VkCommandPool Engine::vkCommandPool;
std::vector<VkCommandBuffer> Engine::vkCommandBuffers;
std::vector<VkFramebuffer> Engine::vkFramebuffers;
std::vector<VkImage> Engine::vkSwapChainImages;
std::vector<VkImageView> Engine::vkSwapChainImageViews;
VkFormat Engine::swapChainImageFormat;
VkExtent2D Engine::swapChainExtent;
VkSemaphore Engine::imageAvailableSemaphore;
VkSemaphore Engine::renderFinishedSemaphore;
bool Engine::engineRunning = true;