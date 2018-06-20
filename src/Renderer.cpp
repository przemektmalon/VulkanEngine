#include "PCH.hpp"
#include "Renderer.hpp"
#include "Window.hpp"
#include "VulkanWrappers.hpp"
#include "File.hpp"
#include "Image.hpp"
#include "Threading.hpp"
#include "Profiler.hpp"

thread_local vdu::CommandPool Renderer::commandPool;

void Renderer::initialiseDevice()
{
	createLogicalDevice();
}

/*
@brief	Initialise renderer and Vulkan objects
*/
void Renderer::initialise()
{
	// Memory pools, samplers, and semaphores
	createDescriptorPool();
	createCommandPool();
	createQueryPool();
	createTextureSampler();
	createSemaphores();

	lightManager.init();

	overlayShader.create(&logicalDevice);
	overlayShader.compile();

	combineOverlaysShader.create(&logicalDevice);
	combineOverlaysShader.compile();

	// Swap chain
	createScreenSwapChain();

	// Pipeline attachments
	createGBufferAttachments();
	createPBRAttachments();
	createScreenAttachments();

	// Pipeline render passes
	createGBufferRenderPass();
	createShadowRenderPass();
	createScreenRenderPass();

	// Pipeline descriptor set layouts
	createGBufferDescriptorSetLayouts();
	createShadowDescriptorSetLayouts();
	createPBRDescriptorSetLayouts();
	createScreenDescriptorSetLayouts();

	// Pipeline objects
	createGBufferPipeline();
	createShadowPipeline();
	createPBRPipeline();
	createScreenPipeline();

	// Pipeline framebuffers
	createGBufferFramebuffers();
	createScreenFramebuffers();

	// Buffers for models, screen quad, uniforms
	createDataBuffers();

	// Pipeline descriptor sets
	createGBufferDescriptorSets();
	createShadowDescriptorSets();
	createPBRDescriptorSets();
	createScreenDescriptorSets();

	// Pipeline commands
	createGBufferCommands();
	createShadowCommands();
	createPBRCommands();
	createScreenCommands();

	overlayRenderer.createOverlayRenderPass();
	overlayRenderer.createOverlayDescriptorSetLayouts();
	overlayRenderer.createOverlayDescriptorSets();
	overlayRenderer.createOverlayPipeline();
	overlayRenderer.createOverlayAttachmentsFramebuffers();
	overlayRenderer.createOverlayCommands();

	for (int i = 0; i < 10; ++i)
	{
		auto& pl = lightManager.addPointLight();
		auto& r = Engine::rand;
		int s = 300;
		int sh = s / 2;
		pl.setPosition(glm::fvec3(s64(r() % s) - sh, s64(r() % 150) + 50, s64(r() % s) - sh));
		glm::fvec3 col(1.0, 0.8, 1.0);
		switch (i % 7)
		{
		case 0: {
			col = glm::fvec3(1, 0.5, 0.5);
			break; }
		case 1: {
			col = glm::fvec3(0.5, 1, 0.5);
			break; }
		case 2: {
			col = glm::fvec3(0.5, 0.5, 1);
			break; }
		case 3: {
			col = glm::fvec3(1, 1, 0.5);
			break; }
		case 4: {
			col = glm::fvec3(0.5, 1, 1);
			break; }
		case 5: {
			col = glm::fvec3(1, 0.5, 1);
			break; }
		case 6: {
			col = glm::fvec3(1, 1, 1);
			break; }
		}
		pl.setColour(col * glm::fvec3(2.3));
		pl.setLinear(0.0001);
		pl.setQuadratic(0.0001);
		pl.setFadeStart(10000);
		pl.setFadeLength(500);
	}

	lightManager.updateLightCounts();
	lightManager.updateAllPointLights();
	lightManager.updateAllSpotLights();

	lightManager.sunLight.setDirection(glm::normalize(glm::fvec3(1, -1, 0)));
	lightManager.sunLight.setColour(glm::fvec3(2, 2, 2));
	lightManager.sunLight.initTexture(glm::ivec2(1280, 720));
	lightManager.sunLight.calcProjs();

	lightManager.updateSunLight();
}

void Renderer::reInitialise()
{
	VK_CHECK_RESULT(vkDeviceWaitIdle(device));

	cleanupForReInit();

	// Swap chain
	createScreenSwapChain();

	// Pipeline attachments
	createGBufferAttachments();
	createPBRAttachments();
	createScreenAttachments();

	// Pipeline render passes
	createGBufferRenderPass();
	createScreenRenderPass();
	overlayRenderer.createOverlayRenderPass();
	overlayRenderer.createOverlayAttachmentsFramebuffers();

	// Pipeline objects
	createGBufferPipeline();
	createPBRPipeline();
	createScreenPipeline();
	overlayRenderer.createOverlayPipeline();

	// Pipeline framebuffers
	createGBufferFramebuffers();
	createScreenFramebuffers();

	// Pipeline commands
	createGBufferCommands();
	createPBRCommands();
	createScreenCommands();
	overlayRenderer.createOverlayCommands();

	// Pipeline descriptor sets
	updateGBufferDescriptorSets();
	updatePBRDescriptorSets();
	updateScreenDescriptorSets();

	// Pipeline commands

	gBufferCmdsNeedUpdate = true;

	updateGBufferCommands();
	updatePBRCommands();
	updateScreenCommands();
	overlayRenderer.updateOverlayCommands();
}

/*
@brief	Cleanup Vulkan objects
*/
void Renderer::cleanup()
{
	// GBuffer pipeline
	{
		destroyGBufferAttachments();
		destroyGBufferRenderPass();
		destroyGBufferDescriptorSetLayouts();
		destroyGBufferPipeline();
		destroyGBufferFramebuffers();
		//destroyGBufferDescriptorSets();
		//destroyGBufferCommands();
	}

	// Shadow pipelins
	{
		destroyShadowRenderPass();
		destroyShadowDescriptorSetLayouts();
		destroyShadowPipeline();
		//destroyShadowCommands();
	}

	// PBR pipeline
	{
		destroyPBRAttachments();
		destroyPBRDescriptorSetLayouts();
		destroyPBRPipeline();
		//destroyPBRDescriptorSets();
		//destroyPBRCommands();
	}

	// Screen pipeline
	{
		destroyScreenAttachments();
		destroyScreenRenderPass();
		destroyScreenDescriptorSetLayouts();
		destroyScreenPipeline();
		destroyScreenFramebuffers();
		//destroyScreenDescriptorSets();
		//destroyScreenCommands();
		destroyScreenSwapChain();
	}

	overlayRenderer.cleanup();

	descriptorPool.destroy();
	freeableDescriptorPool.destroy();
	commandPool.destroy();
	VK_VALIDATE(vkDestroyQueryPool(device, queryPool, 0));

	VK_VALIDATE(vkDestroySampler(device, textureSampler, nullptr));
	VK_VALIDATE(vkDestroySampler(device, skySampler, nullptr));
	VK_VALIDATE(vkDestroySampler(device, shadowSampler, nullptr));

	VK_VALIDATE(vkDestroySemaphore(device, renderFinishedSemaphore, 0));
	VK_VALIDATE(vkDestroySemaphore(device, imageAvailableSemaphore, 0));
	VK_VALIDATE(vkDestroySemaphore(device, pbrFinishedSemaphore, 0));
	VK_VALIDATE(vkDestroySemaphore(device, screenFinishedSemaphore, 0));
	VK_VALIDATE(vkDestroySemaphore(device, shadowFinishedSemaphore, 0));
	VK_VALIDATE(vkDestroySemaphore(device, overlayFinishedSemaphore, 0));
	VK_VALIDATE(vkDestroySemaphore(device, overlayCombineFinishedSemaphore, 0));

	cameraUBO.destroy();
	transformUBO.destroy();
	vertexIndexBuffer.destroy();
	drawCmdBuffer.destroy();
	screenQuadBuffer.destroy();

	lightManager.cleanup();

	combineOverlaysShader.destroy();
	overlayShader.destroy();

	logicalDevice.destroy();
}

void Renderer::cleanupForReInit()
{
	destroyGBufferAttachments();
	destroyGBufferRenderPass();
	destroyGBufferPipeline();
	destroyGBufferFramebuffers();
	destroyGBufferCommands();

	destroyPBRAttachments();
	destroyPBRPipeline();
	destroyPBRCommands();

	destroyScreenAttachments();
	destroyScreenRenderPass();
	destroyScreenPipeline();
	destroyScreenFramebuffers();
	destroyScreenCommands();
	destroyScreenSwapChain();

	overlayRenderer.cleanupForReInit();
}

/*
@brief	Render
*/
void Renderer::render()
{
	//PROFILE_START("qwaitidle");

	VK_CHECK_RESULT(vkQueueWaitIdle(presentQueue));

	VK_CHECK_RESULT(vkGetQueryPoolResults(device, queryPool, 0, NUM_GPU_TIMESTAMPS, sizeof(u64) * NUM_GPU_TIMESTAMPS, Engine::gpuTimeStamps, sizeof(u64), VK_QUERY_RESULT_64_BIT));

	PROFILE_RESET("gbuffer");
	PROFILE_RESET("shadow");
	PROFILE_RESET("pbr");
	PROFILE_RESET("overlay");
	PROFILE_RESET("overlaycombine");
	PROFILE_RESET("screen");

	PROFILE_GPU_ADD_TIME("gbuffer", Engine::gpuTimeStamps[Renderer::BEGIN_GBUFFER], Engine::gpuTimeStamps[Renderer::END_GBUFFER]);
	PROFILE_GPU_ADD_TIME("shadow", Engine::gpuTimeStamps[Renderer::BEGIN_SHADOW], Engine::gpuTimeStamps[Renderer::END_SHADOW]);
	PROFILE_GPU_ADD_TIME("pbr", Engine::gpuTimeStamps[Renderer::BEGIN_PBR], Engine::gpuTimeStamps[Renderer::END_PBR]);
	PROFILE_GPU_ADD_TIME("overlay", Engine::gpuTimeStamps[Renderer::BEGIN_OVERLAY], Engine::gpuTimeStamps[Renderer::END_OVERLAY]);
	PROFILE_GPU_ADD_TIME("overlaycombine", Engine::gpuTimeStamps[Renderer::BEGIN_OVERLAY_COMBINE], Engine::gpuTimeStamps[Renderer::END_OVERLAY_COMBINE]);
	PROFILE_GPU_ADD_TIME("screen", Engine::gpuTimeStamps[Renderer::BEGIN_SCREEN], Engine::gpuTimeStamps[Renderer::END_SCREEN]);

	//PROFILE_END("qwaitidle");



	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;


	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitDstStageMask = 0;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &gBufferCommands.getHandle();

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

	VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
	// From now on we cant update the gBuffer command buffer until the fence is signalled at some point in the future
	// GBuffer command update will block
	// Later we want to implement a double(or triple) buffering for command buffers and fences, so we can start updating next frames command buffer without blocking

	VkPipelineStageFlags waitStages1[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &renderFinishedSemaphore;
	submitInfo.pWaitDstStageMask = waitStages1;
	submitInfo.pCommandBuffers = &shadowCommandBuffer.getHandle();
	submitInfo.pSignalSemaphores = &shadowFinishedSemaphore;

	VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));


	submitInfo.pWaitSemaphores = &shadowFinishedSemaphore;
	submitInfo.pWaitDstStageMask = waitStages1;
	submitInfo.pCommandBuffers = &pbrCommandBuffer.getHandle();;
	submitInfo.pSignalSemaphores = &pbrFinishedSemaphore;

	VK_CHECK_RESULT(vkQueueSubmit(computeQueue, 1, &submitInfo, VK_NULL_HANDLE));


	VkPipelineStageFlags waitStages2[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitDstStageMask = waitStages2;
	submitInfo.pCommandBuffers = &overlayRenderer.elementCommandBuffer.getHandle();
	submitInfo.pSignalSemaphores = &overlayFinishedSemaphore;

	VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));


	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &overlayFinishedSemaphore;
	submitInfo.pWaitDstStageMask = waitStages2;
	submitInfo.pCommandBuffers = &overlayRenderer.combineCommandBuffer.getHandle();
	submitInfo.pSignalSemaphores = &overlayCombineFinishedSemaphore;

	VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));



	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result != VK_SUCCESS)
	{
		DBG_SEVERE("Could not acquire next image");
		//cleanupVulkanSwapChain();
		//recreateVulkanSwapChain();
		return;
	}

	VkPipelineStageFlags waitStages3[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore waitSems[] = { imageAvailableSemaphore, pbrFinishedSemaphore, overlayCombineFinishedSemaphore };
	submitInfo.pWaitSemaphores = waitSems;
	submitInfo.waitSemaphoreCount = 3;
	submitInfo.pWaitDstStageMask = waitStages3;
	submitInfo.pCommandBuffers = &screenCommandBuffers.getHandle(imageIndex);
	submitInfo.pSignalSemaphores = &screenFinishedSemaphore;

	VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));


	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &screenFinishedSemaphore;

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	PROFILE_START("submitrender");

	VK_CHECK_RESULT(vkQueuePresentKHR(presentQueue, &presentInfo));

	PROFILE_END("submitrender");
}

void Renderer::reloadShaders()
{
	VK_CHECK_RESULT(vkDeviceWaitIdle(device));

	destroyGBufferPipeline();
	destroyPBRPipeline();
	destroyScreenPipeline();

	destroyGBufferCommands();
	destroyPBRCommands();
	destroyScreenCommands();

	gBufferShader.reload();
	pbrShader.reload();
	screenShader.reload();

	createGBufferPipeline();
	createPBRPipeline();
	createScreenPipeline();

	createGBufferCommands();
	createPBRCommands();
	createScreenCommands();

	updateGBufferCommands();
	updatePBRCommands();
	updateScreenCommands();
}

void Renderer::populateDrawCmdBuffer()
{
	//VkDrawIndexedIndirectCommand* cmd = new VkDrawIndexedIndirectCommand[Engine::world.instancesToDraw.size()];

	VkDrawIndexedIndirectCommand* cmd = (VkDrawIndexedIndirectCommand*)drawCmdBuffer.getMemory()->map();

	int i = 0;

	auto tIndex = ModelInstance::toGPUTransformIndex;

	for (auto& m : Engine::world.instancesToDraw)
	{
		glm::fvec3 modelPos = m->transform[tIndex].getTranslation();
		float distanceToCam = glm::length(Engine::camera.getPosition() - modelPos);

		int lodIndex = 0;
		for (auto lim : m->model->lodLimits) /// TODO: monitor for LOD/culling/world changes, dont re-populate this when not needed
		{
			if (distanceToCam >= lim)
				break;
			++lodIndex;
		}

		auto& lodMesh = m->model->modelLODs[lodIndex];

		cmd[i].firstIndex = lodMesh.firstIndex;
		cmd[i].indexCount = lodMesh.indexDataLength;
		cmd[i].vertexOffset = lodMesh.firstVertex;
		cmd[i].firstInstance = (m->material->gpuIndexBase << 20) | (m->transformIndex);
		cmd[i].instanceCount = 1; /// TODO: do we want/need a different class for real instanced drawing ?
		++i;
	}

	drawCmdBuffer.getMemory()->unmap();

	//drawCmdBuffer.setMem(cmd, Engine::world.modelNames.size() * sizeof(VkDrawIndexedIndirectCommand), 0);

	//delete[] cmd;
}

void Renderer::createVertexIndexBuffers()
{
	VkDeviceSize bufferSize = VERTEX_BUFFER_SIZE + INDEX_BUFFER_SIZE;
	vertexIndexBuffer.setMemoryProperty(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vertexIndexBuffer.setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	vertexIndexBuffer.create(&logicalDevice, bufferSize);
}

void Renderer::pushModelDataToGPU(Model & model)
{
	for (auto& lodLevel : model.modelLODs)
	{
		// Copy vertices through staging buffer

		vertexIndexBuffer.createStaging(stagingBuffer);
		memcpy(stagingBuffer.getMemory()->map(), lodLevel.vertexData, lodLevel.vertexDataLength * sizeof(Vertex));
		stagingBuffer.getMemory()->unmap();

		auto cmd = beginSingleTimeCommands();
		stagingBuffer.cmdCopyTo(cmd, &vertexIndexBuffer, lodLevel.vertexDataLength * sizeof(Vertex), 0, vertexInputByteOffset);
		endSingleTimeCommands(cmd);

		stagingBuffer.destroy();

		//vertexIndexBuffer.setMem(lodLevel.vertexData, lodLevel.vertexDataLength * sizeof(Vertex), vertexInputByteOffset);

		lodLevel.firstVertex = (s32)(vertexInputByteOffset / sizeof(Vertex));
		vertexInputByteOffset += lodLevel.vertexDataLength * sizeof(Vertex);


		// Copy indices through staging buffer

		vertexIndexBuffer.createStaging(stagingBuffer);
		memcpy(stagingBuffer.getMemory()->map(), lodLevel.indexData, lodLevel.indexDataLength * sizeof(u32));
		stagingBuffer.getMemory()->unmap();

		cmd = beginSingleTimeCommands();
		stagingBuffer.cmdCopyTo(cmd, &vertexIndexBuffer, lodLevel.indexDataLength * sizeof(u32), 0, INDEX_BUFFER_BASE + indexInputByteOffset);
		endSingleTimeCommands(cmd);

		stagingBuffer.destroy();

		//vertexIndexBuffer.setMem(lodLevel.indexData, lodLevel.indexDataLength * sizeof(u32), INDEX_BUFFER_BASE + indexInputByteOffset);

		lodLevel.firstIndex = (u32)(indexInputByteOffset / sizeof(u32));
		indexInputByteOffset += lodLevel.indexDataLength * sizeof(u32);
	}
}

void Renderer::createDataBuffers()
{
	/// TODO: set appropriate size

	drawCmdBuffer.setMemoryProperty(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	drawCmdBuffer.setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
	drawCmdBuffer.create(&logicalDevice, sizeof(VkDrawIndexedIndirectCommand) * 1000);
	
	createVertexIndexBuffers();

	std::vector<Vertex2D> quad;
	quad.push_back({ { -1,-1 },{ 0,0 } });
	quad.push_back({ { 1,1 },{ 1,1 } });
	quad.push_back({ { -1,1 },{ 0,1 } });
	quad.push_back({ { -1,-1 },{ 0,0 } });
	quad.push_back({ { 1,-1 },{ 1,0 } });
	quad.push_back({ { 1,1 },{ 1,1 } });

	screenQuadBuffer.setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	screenQuadBuffer.setMemoryProperty(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	screenQuadBuffer.create(&logicalDevice, quad.size() * sizeof(Vertex2D));

	screenQuadBuffer.createStaging(stagingBuffer);

	auto data = stagingBuffer.getMemory()->map();
	memcpy(data, quad.data(), quad.size() * sizeof(Vertex2D));
	stagingBuffer.getMemory()->unmap();

	auto cmd = beginSingleTimeCommands();
	stagingBuffer.cmdCopyTo(cmd, &screenQuadBuffer);
	endSingleTimeCommands(cmd);

	stagingBuffer.destroy();

	createUBOs();
}

/*
@brief	Creates a Vulkan logical device (from the optimal physical device) with specific features and queues
*/
void Renderer::createLogicalDevice()
{
	DBG_INFO("Creating Vulkan logical device");

	VkPhysicalDeviceFeatures pdf = {};
	pdf.samplerAnisotropy = VK_TRUE;
	pdf.multiDrawIndirect = VK_TRUE;
	pdf.shaderStorageImageExtendedFormats = VK_TRUE;
	pdf.geometryShader = VK_TRUE;

	lGraphicsQueue.prepare(0, 1.f);
	lTransferQueue.prepare(0, 1.f);

	logicalDevice.addExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	logicalDevice.addLayer("VK_LAYER_LUNARG_standard_validation");
	logicalDevice.addQueue(&lGraphicsQueue);
	logicalDevice.addQueue(&lTransferQueue);

	logicalDevice.setEnabledDeviceFeatures(pdf);

	logicalDevice.create(Engine::physicalDevice);

	graphicsQueue = lGraphicsQueue.getHandle();
	presentQueue = lGraphicsQueue.getHandle();
	computeQueue = lGraphicsQueue.getHandle();
	transferQueue = lTransferQueue.getHandle();

	device = logicalDevice.getHandle();
}

/*
@brief	Create Vulkan command pool for submitting commands to a particular queue
*/
void Renderer::createCommandPool()
{
	commandPool.setFlags(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	commandPool.setQueueFamily(&Engine::physicalDevice->getQueueFamilies().front());
	commandPool.create(&logicalDevice);

	//VkFenceCreateInfo fci = {};
	//fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	//fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	//fci.pNext = 0;

	//VK_CHECK_RESULT(vkCreateFence(device, &fci, nullptr, &gBufferCommands.fence));
	//VK_CHECK_RESULT(vkCreateFence(device, &fci, nullptr, &shadowFence));
}

void Renderer::createPerThreadCommandPools()
{
	commandPool.setFlags(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	commandPool.setQueueFamily(&Engine::physicalDevice->getQueueFamilies().front());
	commandPool.create(&Engine::renderer->logicalDevice);
}

void Renderer::createQueryPool()
{
	VkQueryPoolCreateInfo ci = {};
	ci.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	ci.queryType = VK_QUERY_TYPE_TIMESTAMP;
	ci.queryCount = NUM_GPU_TIMESTAMPS;

	VK_CHECK_RESULT(vkCreateQueryPool(device, &ci, 0, &queryPool));
}

void Renderer::createTextureSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = -1.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 11.0f;

	VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler));

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.f;

	VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &skySampler));

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo.addressModeV = samplerInfo.addressModeU;
	samplerInfo.addressModeW = samplerInfo.addressModeU;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 0;
	//samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;

	VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &shadowSampler));
}

/*
@brief	Create vulkan uniform buffer for MVP matrices
*/
void Renderer::createUBOs()
{
	cameraUBO.setMemoryProperty(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	cameraUBO.setUsage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	cameraUBO.create(&logicalDevice, sizeof(CameraUBOData));

	// 8 MB of transforms can support around 125k model instances
	transformUBO.setMemoryProperty(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	transformUBO.setUsage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	transformUBO.create(&logicalDevice, 8 * 1024 * 1024);
}

/*
@brief	Create Vulkan descriptor pool
*/
void Renderer::createDescriptorPool()
{
	descriptorPool.addPoolCount(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 9);
	descriptorPool.addPoolCount(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1430);
	descriptorPool.addPoolCount(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 12);
	descriptorPool.addSetCount(15);

	descriptorPool.create(&logicalDevice);

	freeableDescriptorPool.addPoolCount(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 200);
	freeableDescriptorPool.addSetCount(200);
	freeableDescriptorPool.setFreeable(true);

	freeableDescriptorPool.create(&logicalDevice);
}

/*
@brief	Create Vulkan semaphores
*/
void Renderer::createSemaphores()
{
	DBG_INFO("Creating Vulkan semaphores");
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore));
	VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore));
	VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &pbrFinishedSemaphore));
	VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &screenFinishedSemaphore));
	VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &shadowFinishedSemaphore));
	VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &overlayFinishedSemaphore));
	VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &overlayCombineFinishedSemaphore));
}

VkCommandBuffer Renderer::beginTransferCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool.getHandle();
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	return commandBuffer;
}

VkSubmitInfo Renderer::endTransferCommands(VkCommandBuffer commandBuffer)
{
	VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	return submitInfo;
}

void Renderer::submitTransferCommands(VkSubmitInfo submitInfo, VkFence fence)
{
	auto submitJobFunc = [&]() -> void
	{
		VK_CHECK_RESULT(vkQueueSubmit(transferQueue, 1, &submitInfo, fence));
		VK_CHECK_RESULT(vkQueueWaitIdle(transferQueue)); /// TODO: non blocking queue submissions !
		VK_VALIDATE(vkFreeCommandBuffers(device, commandPool.getHandle(), 1, submitInfo.pCommandBuffers));
		// To free command buffer for non blocking jobs we will need to know when the gpu finished it !
	};
	auto submitJob = new Job<>(submitJobFunc, defaultTransferJobDoneFunc);
	Engine::threading->addGPUTransferJob(submitJob, 0); /// TODO: make asynchronous
}

VkCommandBuffer Renderer::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool.getHandle();
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	return commandBuffer;
}

void Renderer::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkFence fence) {
	VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	Engine::threading->physToGPUMutex.lock();

	VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence));
	VK_CHECK_RESULT(vkQueueWaitIdle(graphicsQueue)); /// TODO: non blocking queue submissions !

	Engine::threading->physToGPUMutex.unlock();

	VK_VALIDATE(vkFreeCommandBuffers(device, commandPool.getHandle(), 1, &commandBuffer));
}

/*
@brief	Update camera buffer
*/
void Renderer::updateCameraBuffer()
{
	cameraUBOData.view = Engine::camera.getView();
	cameraUBOData.proj = Engine::camera.getProj();
	cameraUBOData.pos = Engine::camera.getPosition();
	cameraUBOData.viewRays = Engine::camera.getViewRays();

	void* data = cameraUBO.getMemory()->map();
	memcpy(data, &cameraUBOData, sizeof(cameraUBOData));
	cameraUBO.getMemory()->unmap();
}

/*
@brief	Update transforms buffer
*/
void Renderer::updateTransformBuffer()
{
	//PROFILE_MUTEX("transformmutex", Engine::threading->instanceTransformMutex.lock());

	//VK_CHECK_RESULT(vkWaitForFences(device, 1, &gBufferCommands.fence, 1, std::numeric_limits<u64>::max()));

	auto tIndex = ModelInstance::toGPUTransformIndex;

	glm::fmat4* transform = (glm::fmat4*)transformUBO.getMemory()->map();

	int i = 0;
	for (auto& m : Engine::world.instancesToDraw)
	{
		transform[m->transformIndex] = m->transform[tIndex].getTransformMat();
		++i;
	}

	transformUBO.getMemory()->unmap();

	//VK_CHECK_RESULT(vkResetFences(device, 1, &gBufferCommands.fence));

	//ModelInstance::toGPUTransformIndex = tIndex == 0 ? 1 : 0;

	//Engine::threading->instanceTransformMutex.unlock();
}

void Renderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, int mipLevels, int layerCount, VkImageAspectFlags aspect)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspect;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = layerCount;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == newLayout)
		return;

	if ((oldLayout == VK_IMAGE_LAYOUT_UNDEFINED || oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED) && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_UNDEFINED || oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED) && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_UNDEFINED || oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED) && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_UNDEFINED || oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED) && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_UNDEFINED || oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED) && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL || newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
	}
	else {
		DBG_SEVERE("unsupported layout transition!");
	}

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VK_VALIDATE(vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	));

	endSingleTimeCommands(commandBuffer);
}

void Renderer::setImageLayout(VkCommandBuffer cmdbuffer, Texture& tex, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
{
	// Create an image barrier object
	VkImageMemoryBarrier imageBarrier = {};
	imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageBarrier.oldLayout = oldImageLayout;
	imageBarrier.newLayout = newImageLayout;
	imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.image = tex.getHandle();

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = tex.getAspectFlags();
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = tex.getMaxMipLevel() + 1;
	subresourceRange.layerCount = 1;
	imageBarrier.subresourceRange = subresourceRange;

	// Source layouts (old)
	// Source access mask controls actions that have to be finished on the old layout
	// before it will be transitioned to the new layout
	switch (oldImageLayout)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		// Image layout is undefined (or does not matter)
		// Only valid as initial layout
		// No flags required, listed only for completeness
		imageBarrier.srcAccessMask = 0;
		break;

	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		// Image is preinitialized
		// Only valid as initial layout for linear images, preserves memory contents
		// Make sure host writes have been finished
		imageBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_GENERAL:
		// Image layout is undefined (or does not matter)
		// Only valid as initial layout
		// No flags required, listed only for completeness
		imageBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image is a color attachment
		// Make sure any writes to the color buffer have been finished
		imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image is a depth/stencil attachment
		// Make sure any writes to the depth/stencil buffer have been finished
		imageBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image is a transfer source 
		// Make sure any reads from the image have been finished
		imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image is a transfer destination
		// Make sure any writes to the image have been finished
		imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image is read by a shader
		// Make sure any shader reads from the image have been finished
		imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		// Other source layouts aren't handled (yet)
		break;
	}

	// Target layouts (new)
	// Destination access mask controls the dependency for the new image layout
	switch (newImageLayout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image will be used as a transfer destination
		// Make sure any writes to the image have been finished
		imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_GENERAL:
		// Image will be used as a transfer destination
		// Make sure any writes to the image have been finished
		imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image will be used as a transfer source
		// Make sure any reads from the image have been finished
		imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image will be used as a color attachment
		// Make sure any writes to the color buffer have been finished
		imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image layout will be used as a depth/stencil attachment
		// Make sure any writes to depth/stencil buffer have been finished
		imageBarrier.dstAccessMask = imageBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image will be read in a shader (sampler, input attachment)
		// Make sure any writes to the image have been finished
		if (imageBarrier.srcAccessMask == 0)
		{
			imageBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		// Other source layouts aren't handled (yet)
		break;
	}

	// Put barrier inside setup command buffer
	VK_VALIDATE(vkCmdPipelineBarrier(
		cmdbuffer,
		srcStageMask,
		dstStageMask,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageBarrier));
}