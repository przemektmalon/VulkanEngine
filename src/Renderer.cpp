#include "PCH.hpp"
#include "Renderer.hpp"
#include "Window.hpp"
#include "File.hpp"
#include "Image.hpp"
#include "Threading.hpp"
#include "Profiler.hpp"

thread_local vdu::CommandPool Renderer::commandPool;
thread_local std::unordered_map<vdu::Fence*, std::function<void(void)>> Renderer::fenceDelayedActions;

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
	createQueryPool();
	createTextureSampler();
	createSynchroObjects();

	lightManager.init();

	defaultVertexInputState.addBinding(Vertex::getBindingDescription());
	defaultVertexInputState.addAttributes(Vertex::getAttributeDescriptions());

	ssaoVertexInputState.addBinding(Vertex2D::getBindingDescription());
	ssaoVertexInputState.addAttributes(Vertex2D::getAttributeDescriptions());

	screenVertexInputState.addBinding(Vertex2D::getBindingDescription());
	screenVertexInputState.addAttributes(Vertex2D::getAttributeDescriptions());

	uiRenderer.vertexInputState.addBinding(VertexNoNormal::getBindingDescription());
	uiRenderer.vertexInputState.addAttributes(VertexNoNormal::getAttributeDescriptions());

	// Shaders
	createShaders();
	compileShaders();

	// Screen
	{
		createScreenDescriptorSetLayouts();
		createScreenSwapchain();
		createScreenPipeline();
		createScreenDescriptorSets();
		createScreenCommands();
	}

	// PBR
	{
		createPBRAttachments();
		createPBRDescriptorSetLayouts();
		createPBRPipeline();
		createPBRDescriptorSets();
		createPBRCommands();
	}

	// Shadows
	/// TODO: Variable resolution shadows
	{
		createShadowDescriptorSetLayouts();
		createShadowRenderPass();
		createShadowPipeline();
		createShadowDescriptorSets();
		createShadowCommands();
	}

	// SSAO
	{
		createSSAOAttachments();
		createSSAORenderPass();
		createSSAODescriptorSetLayouts();
		createSSAOPipeline();
		createSSAOFramebuffer();
		createSSAODescriptorSets();
		createSSAOCommands();
	}

	// GBuffer
	{
		createGBufferAttachments();
		createGBufferRenderPass();
		createGBufferDescriptorSetLayouts();
		createGBufferPipeline();
		createGBufferFramebuffers();
		createGBufferDescriptorSets();
		createGBufferCommands();

		createGBufferNoTexAttachments();
		createGBufferNoTexRenderPass();
		createGBufferNoTexDescriptorSetLayouts();
		createGBufferNoTexPipeline();
		createGBufferNoTexFramebuffers();
		createGBufferNoTexDescriptorSets();
		createGBufferNoTexCommands();
	}

	// Overlays
	{
		uiRenderer.createOverlayAttachments();
		uiRenderer.createOverlayRenderPass();
		uiRenderer.createOverlayDescriptorSetLayouts();
		uiRenderer.createOverlayPipeline();
		uiRenderer.createOverlayFramebuffer();
		uiRenderer.createOverlayCommands();
	}

	createDataBuffers();

	/*for (int i = 0; i < 1; ++i)
	{
		auto& pl = lightManager.addSpotLight();
		auto& r = Engine::rand;
		int s = 15000;
		int sh = s / 2;
		//pl.setPosition(glm::fvec3(s64(r() % s) - sh, s64(r() % 150) + 5000, s64(r() % s) - sh));
		pl.setPosition(glm::fvec3(0,1000,0));
		pl.setDirection(glm::normalize(-pl.getPosition()));
		pl.setColour(glm::fvec3(2.55, 2.42, 2.26));
		pl.setLinear(0.00000001);
		pl.setQuadratic(0.00000001);
		pl.setFadeStart(1000000000000);
		pl.setFadeLength(500);
		pl.setInnerSpread(45);
		pl.setOuterSpread(45.01);
	}*/

	for (int i = 0; i < 1; ++i)
	{
		auto& pl = lightManager.addPointLight();
		auto& r = Engine::rand;
		int s = 250;
		int sh = s / 2;
		//pl.setPosition(glm::fvec3(s64(r() % s) - sh, s64(r() % 150) + 5000, s64(r() % s) - sh));
		pl.setPosition(glm::fvec3(2, 3, 2));
		pl.setColour(glm::fvec3(2,2,2));
		pl.setLinear(0.0000001);
		pl.setQuadratic(0.0000001);
		pl.setFadeStart(100000000);
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

	initialiseQueryPool();

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
	}

	// GBufferNoTex pipeline
	{
		destroyGBufferNoTexAttachments();
		destroyGBufferNoTexRenderPass();
		destroyGBufferNoTexDescriptorSetLayouts();
		destroyGBufferNoTexPipeline();
		destroyGBufferNoTexFramebuffers();
	}

	// Shadow pipeline
	{
		destroyShadowRenderPass();
		destroyShadowDescriptorSetLayouts();
		destroyShadowPipeline();
	}

	// SSAO pipeline
	{
		destroySSAOAttachments();
		destroySSAORenderPass();
		destroySSAODescriptorSetLayouts();
		destroySSAOPipeline();
		destroySSAOFramebuffer();
	}

	// PBR pipeline
	{
		destroyPBRAttachments();
		destroyPBRDescriptorSetLayouts();
		destroyPBRPipeline();
	}

	// Screen pipeline
	{
		destroyScreenDescriptorSetLayouts();
		destroyScreenPipeline();
		destroyScreenSwapchain();
	}

	uiRenderer.cleanup();

	descriptorPool.destroy();
	freeableDescriptorPool.destroy();
	commandPool.destroy();
	queryPool.destroy();

	vkDestroySampler(device, textureSampler, nullptr);
	vkDestroySampler(device, skySampler, nullptr);
	vkDestroySampler(device, shadowSampler, nullptr);

	gBufferFinishedSemaphore.destroy();
	imageAvailableSemaphore.destroy();
	pbrFinishedSemaphore.destroy();
	screenFinishedSemaphore.destroy();
	shadowFinishedSemaphore.destroy();
	overlayFinishedSemaphore.destroy();
	overlayCombineFinishedSemaphore.destroy();
	ssaoFinishedSemaphore.destroy();

	gBufferGroupFence.destroy();
	pbrGroupFence.destroy();

	cameraUBO.destroy();
	flatPBRUBO.destroy();
	transformUBO.destroy();
	vertexIndexBuffer.destroy();
	drawCmdBuffer.destroy();
	screenQuadBuffer.destroy();
	ssaoConfigBuffer.destroy();

	lightManager.cleanup();

	combineOverlaysShader.destroy();
	overlayShader.destroy();
	ssaoShader.destroy();
	ssaoBlurShader.destroy();
	pointShadowShader.destroy();
	spotShadowShader.destroy();
	sunShadowShader.destroy();
	gBufferShader.destroy();
	gBufferNoTexShader.destroy();
	pbrShader.destroy();
	screenShader.destroy();
	logicalDevice.destroy();
}

/*
@brief	Render
*/
void Renderer::render()
{
	/*
		Query GPU profiling data and add it to our profiler
	*/
	auto timestamps = queryPool.query();
	memcpy(Engine::gpuTimeStamps, timestamps, sizeof(u64) * NUM_GPU_TIMESTAMPS);
	
	PROFILE_GPU_ADD_TIME("gbuffer", Engine::gpuTimeStamps[Renderer::BEGIN_GBUFFER], Engine::gpuTimeStamps[Renderer::END_GBUFFER]);
	PROFILE_GPU_ADD_TIME("shadow", Engine::gpuTimeStamps[Renderer::BEGIN_SHADOW], Engine::gpuTimeStamps[Renderer::END_SHADOW]);
	PROFILE_GPU_ADD_TIME("ssao", Engine::gpuTimeStamps[Renderer::BEGIN_SSAO], Engine::gpuTimeStamps[Renderer::END_SSAO]);
	PROFILE_GPU_ADD_TIME("pbr", Engine::gpuTimeStamps[Renderer::BEGIN_PBR], Engine::gpuTimeStamps[Renderer::END_PBR]);
	PROFILE_GPU_ADD_TIME("overlay", Engine::gpuTimeStamps[Renderer::BEGIN_UI], Engine::gpuTimeStamps[Renderer::END_UI]);
	PROFILE_GPU_ADD_TIME("screen", Engine::gpuTimeStamps[Renderer::BEGIN_SCREEN], Engine::gpuTimeStamps[Renderer::END_SCREEN]);

	PROFILE_START("submitrender");

	/*
		Submit batched vulkan commands
	*/

	std::vector<vdu::QueueSubmission> submissionsGroup1(3);

	/////////////////////////////////////////
	/*
		GBUFFER & SHADOWS & OVERLAYS

		Completing this submission allows:
			Re-recording gBuffers commands
			Re-recording shadow commands
			Re-recording overlay commands
			Updating transforms buffer
			Updating draw commands buffer
			Updating material descriptors
			Updating camera buffer (If PBR is also done)

		We dont need to wait to:
			Perform frustum culling for the next frame

	*/
	/////////////////////////////////////////

	submissionsGroup1[0].addCommands(&gBufferCommandBuffer);
	//submissionsGroup1[0].addCommands(&gBufferNoTexCommandBuffer);
	submissionsGroup1[0].addSignal(gBufferFinishedSemaphore);

	submissionsGroup1[1].addCommands(&shadowCommandBuffer);
	submissionsGroup1[1].addSignal(shadowFinishedSemaphore);

	submissionsGroup1[2].addCommands(&uiRenderer.commandBuffer);
	submissionsGroup1[2].addSignal(overlayFinishedSemaphore);

	gBufferGroupFence.reset();
	VK_CHECK_RESULT(lGraphicsQueue.submit(submissionsGroup1, gBufferGroupFence));

	/////////////////////////////////////////
	/*
		SSAO & PBR

		Completing this submission allows:
			Re-recording pbr commands
			Updating light buffers
			Updating skybox descriptor
			Updating camera buffer (If gBuffer is also done)

	*/
	/////////////////////////////////////////

	std::vector<vdu::QueueSubmission> submissionsGroup2(2);

	submissionsGroup2[0].addWait(gBufferFinishedSemaphore, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
	submissionsGroup2[0].addCommands(&ssaoCommandBuffer);
	submissionsGroup2[0].addSignal(ssaoFinishedSemaphore);
	
	submissionsGroup2[1].addWait(ssaoFinishedSemaphore, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
	submissionsGroup2[1].addWait(shadowFinishedSemaphore, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
	submissionsGroup2[1].addCommands(&pbrCommandBuffer);
	submissionsGroup2[1].addSignal(pbrFinishedSemaphore);

	pbrGroupFence.reset();
	VK_CHECK_RESULT(lGraphicsQueue.submit(submissionsGroup2, pbrGroupFence));

	/////////////////////////////////////////
	/*
		SCREEN
	*/
	/////////////////////////////////////////

	u32 imageIndex;
	auto result = screenSwapchain.acquireNextImage(imageIndex, imageAvailableSemaphore);

	if (result != VK_SUCCESS) {
		DBG_SEVERE("Could not acquire next image");
		return;
	}

	vdu::QueueSubmission screenSubmission;
	screenSubmission.addWait(imageAvailableSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	screenSubmission.addWait(pbrFinishedSemaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	screenSubmission.addWait(overlayFinishedSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	screenSubmission.addCommands(screenCommandBuffers.getHandle(imageIndex));
	screenSubmission.addSignal(screenFinishedSemaphore);

	VK_CHECK_RESULT(lGraphicsQueue.submit(screenSubmission));

	/////////////////////////////////////////
	/*
		PRESENT
	*/
	/////////////////////////////////////////

	vdu::QueuePresentation presentation;
	presentation.addWait(screenFinishedSemaphore);
	presentation.addSwapchain(screenSwapchain, imageIndex);

	VK_CHECK_RESULT(lGraphicsQueue.present(presentation));

	PROFILE_END("submitrender");
}

void Renderer::renderJob()
{
	auto& _this = Engine::renderer;
	auto& assets = Engine::assets;
	auto& threading = Engine::threading;
	auto& world = Engine::world;

	PROFILE_START("commands");
	_this->gBufferGroupFence.wait();
	_this->updateMaterialDescriptors();

	/// TODO: choose between differnt gBuffer shaders
	_this->updateGBufferCommands();
	_this->updateGBufferNoTexCommands();
	/// /////////////////////////////////////////////

	_this->updateShadowCommands(); // Mutex with engine model transform update
	PROFILE_END("commands");

	_this->lightManager.sunLight.calcProjs();
	world.frustumCulling(&Engine::camera);

	PROFILE_START("qwaitidle");
	_this->lGraphicsQueue.waitIdle();
	vkDeviceWaitIdle(_this->logicalDevice.getHandle());
	PROFILE_END("qwaitidle");

	_this->updateSkyboxDescriptor();

	PROFILE_START("cullingdrawbuffer");

	_this->lightManager.updateSunLight();
	_this->uiRenderer.garbageCollect();
	_this->executeFenceDelayedActions();
	_this->populateDrawCmdBuffer(); // Mutex with engine model transform update
	_this->lightManager.updateShadowDrawCommands();
	_this->updateCameraBuffer();

	PROFILE_END("cullingdrawbuffer");

	PROFILE_START("setuprender");

	_this->updateConfigs();
	_this->uiRenderer.updateOverlayCommands(); // Mutex with any overlay additions/removals
	PROFILE_MUTEX("transformmutex", threading->instanceTransformMutex.lock());
	threading->instanceTransformMutex.unlock();
	PROFILE_MUTEX("phystogpumutex", threading->physToGPUMutex.lock());
	_this->render();
	threading->physToGPUMutex.unlock();

	PROFILE_END("setuprender");

	if (Engine::engineRunning)
		threading->addGPUJob(new Job<>(&renderJob));
}

void Renderer::reloadShaders()
{
	VK_CHECK_RESULT(vkDeviceWaitIdle(device));

	gBufferShader.reload();
	screenShader.reload();
	pbrShader.reload();
	overlayShader.reload();
	combineOverlaysShader.reload();
	ssaoShader.reload();
	ssaoBlurShader.reload();
	pointShadowShader.reload();
	spotShadowShader.reload();
	sunShadowShader.reload();

	compileShaders();

	destroyGBufferPipeline();
	destroyGBufferNoTexPipeline();
	destroyPBRPipeline();
	destroyScreenPipeline();
	destroyShadowPipeline();
	destroySSAOPipeline();
	uiRenderer.destroyOverlayPipeline();

	destroyGBufferCommands();
	destroyGBufferNoTexCommands();
	destroyPBRCommands();
	destroyScreenCommands();
	destroyShadowCommands();
	destroySSAOCommands();
	uiRenderer.destroyOverlayCommands();

	createGBufferPipeline();
	createGBufferNoTexPipeline();
	createPBRPipeline();
	createScreenPipeline();
	createShadowPipeline();
	createSSAOPipeline();
	uiRenderer.createOverlayPipeline();

	createGBufferCommands();
	createGBufferNoTexCommands();
	createPBRCommands();
	createScreenCommands();
	createShadowCommands();
	createSSAOCommands();
	uiRenderer.createOverlayCommands();

	updateGBufferCommands();
	updateGBufferNoTexCommands();
	updatePBRCommands();
	updateScreenCommands();
	updateShadowCommands();
	updateSSAOCommands();
	uiRenderer.updateOverlayCommands();
}

void Renderer::updateConfigs()
{
	auto& config = Engine::config;

	for (auto group : config.changedGroups)
	{
		switch (group) {
		case EngineConfig::SSAO_Group:
			updateSSAOConfigBuffer();
			break;
		}
	}

	for (auto special : config.changedSpecials)
	{
		switch (special) {
		case EngineConfig::Render_Resolution:

			// Recreate the screen pipeline and resolution dependant components
			{
				destroyScreenPipeline();
				destroyScreenCommands();
				destroyScreenSwapchain();

				createScreenSwapchain();
				createScreenCommands();
				createScreenPipeline();
			}

			// Recreate the gBuffer pipeline and resoulution dependant components
			{
				destroyGBufferAttachments();
				destroyGBufferRenderPass();
				destroyGBufferFramebuffers();
				destroyGBufferPipeline();
				destroyGBufferCommands();

				createGBufferAttachments();
				createGBufferRenderPass();
				createGBufferFramebuffers();
				createGBufferPipeline();
				createGBufferCommands();

				updateGBufferCommands();
			}
			
			// Recreate the SSAO pipeline and resolution dependant components
			{
				destroySSAOAttachments();
				destroySSAORenderPass();
				destroySSAOFramebuffer();
				destroySSAODescriptorSets();
				destroySSAOPipeline();
				destroySSAOCommands();

				createSSAOAttachments();
				createSSAORenderPass();
				createSSAOFramebuffer();
				createSSAODescriptorSets();
				createSSAOPipeline();
				createSSAOCommands();

				updateSSAODescriptorSets();
				updateSSAOCommands();
			}

			// Recreate the PBR pipeline and resolution dependant components
			{
				destroyPBRAttachments();
				destroyPBRPipeline();
				destroyPBRCommands();

				createPBRAttachments();
				createPBRPipeline();
				createPBRCommands();

				updatePBRDescriptorSets(USED_GBUFFER);
				
				// We dont need to update all descriptors, just resolution dependant ones
				auto updater = pbrDescriptorSet.makeUpdater();

				auto outputUpdater = updater->addImageUpdate("output");
				*outputUpdater = { textureSampler, pbrOutput.getView(), VK_IMAGE_LAYOUT_GENERAL };

				auto albedoUpdater = updater->addImageUpdate("albedo");
				*albedoUpdater = { textureSampler, gBufferColourAttachment.getView(), VK_IMAGE_LAYOUT_GENERAL };

				auto normalUpdater = updater->addImageUpdate("normal");
				*normalUpdater = { textureSampler, gBufferNormalAttachment.getView(), VK_IMAGE_LAYOUT_GENERAL };

				auto pbrUpdater = updater->addImageUpdate("pbr");
				*pbrUpdater = { textureSampler, gBufferPBRAttachment.getView(), VK_IMAGE_LAYOUT_GENERAL };

				auto depthUpdater = updater->addImageUpdate("depth");
				*depthUpdater = { textureSampler, gBufferDepthAttachment.getView(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };

				auto ssaoUpdater = updater->addImageUpdate("ssao");
				*ssaoUpdater = { textureSampler, ssaoFinalAttachment.getView(), VK_IMAGE_LAYOUT_GENERAL };

				pbrDescriptorSet.submitUpdater(updater);
				pbrDescriptorSet.destroyUpdater(updater);

				updatePBRCommands();
			}

			// Recreate the overlay pipeline and resolution dependant components
			{
				uiRenderer.cleanupForReInit();

				uiRenderer.createOverlayRenderPass();
				uiRenderer.createOverlayPipeline();
				uiRenderer.createOverlayAttachments();
				uiRenderer.createOverlayFramebuffer();
				uiRenderer.createOverlayCommands();

				uiRenderer.updateOverlayCommands();
			}

			// Update screen commands and descriptor sets
			{
				updateScreenDescriptorSets();
				updateScreenCommands();
			}

			break;
		}
	}

	config.changedGroups.clear();
	config.changedSpecials.clear();
	
}

void Renderer::createShaders()
{
	gBufferShader.create(&logicalDevice);
	gBufferNoTexShader.create(&logicalDevice);
	screenShader.create(&logicalDevice);
	pbrShader.create(&logicalDevice);
	overlayShader.create(&logicalDevice);
	combineOverlaysShader.create(&logicalDevice);
	ssaoShader.create(&logicalDevice);
	ssaoBlurShader.create(&logicalDevice);
	pointShadowShader.create(&logicalDevice);
	spotShadowShader.create(&logicalDevice);
	sunShadowShader.create(&logicalDevice);
}

void Renderer::compileShaders()
{
	gBufferShader.compile();
	gBufferNoTexShader.compile();
	screenShader.compile();
	pbrShader.compile();
	overlayShader.compile();
	combineOverlaysShader.compile();
	ssaoShader.compile();
	ssaoBlurShader.compile();
	pointShadowShader.compile();
	spotShadowShader.compile();
	sunShadowShader.compile();
}

void Renderer::addFenceDelayedAction(vdu::Fence * fe, std::function<void(void)> action)
{
	fenceDelayedActions[fe] = action;
}

void Renderer::executeFenceDelayedActions()
{
	for (auto itr = fenceDelayedActions.begin(); itr != fenceDelayedActions.end();)
	{
		if (itr->first->isSignalled())
		{
			itr->second();
			itr = fenceDelayedActions.erase(itr);
		}
		else
		{
			++itr;
		}
	}
}

void Renderer::initialiseQueryPool()
{
	vdu::CommandBuffer cmd;
	cmd.allocate(&logicalDevice, &commandPool);
	cmd.begin();
	for (int i = 0; i < NUM_GPU_TIMESTAMPS; ++i)
	{
		queryPool.cmdTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, i);
	}
	cmd.end();

	vdu::QueueSubmission submit;
	submit.addCommands(&cmd);

	VK_CHECK_RESULT(lGraphicsQueue.submit(submit));
	VK_CHECK_RESULT(lGraphicsQueue.waitIdle());
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

void Renderer::pushModelDataToGPU(Model & model)
{
	for (auto& lodLevel : model.modelLODs)
	{
		// Create staging buffers for this LODs vertices and indices
		vdu::Buffer* stagingBufferVertex = new vdu::Buffer(vertexIndexBuffer, static_cast<VkDeviceSize>(lodLevel.vertexDataLength * sizeof(Vertex)));
		memcpy(stagingBufferVertex->getMemory()->map(), lodLevel.vertexData, lodLevel.vertexDataLength * sizeof(Vertex));
		stagingBufferVertex->getMemory()->unmap();

		auto stagingBufferIndex = new vdu::Buffer(vertexIndexBuffer, static_cast<VkDeviceSize>(lodLevel.indexDataLength * sizeof(u32)));
		memcpy(stagingBufferIndex->getMemory()->map(), lodLevel.indexData, lodLevel.indexDataLength * sizeof(u32));
		stagingBufferIndex->getMemory()->unmap();

		// Create the command buffer that will transfer data from the staging buffer to the device local buffer
		auto cmd = new vdu::CommandBuffer;
		beginTransferCommands(*cmd);
		stagingBufferVertex->cmdCopyTo(cmd, &vertexIndexBuffer, lodLevel.vertexDataLength * sizeof(Vertex), 0, vertexInputByteOffset);
		stagingBufferIndex->cmdCopyTo(cmd, &vertexIndexBuffer, lodLevel.indexDataLength * sizeof(u32), 0, INDEX_BUFFER_BASE + indexInputByteOffset);

		// Submit the transfer commands
		vdu::QueueSubmission submission;
		endTransferCommands(*cmd, submission);
		auto fence = new vdu::Fence(&logicalDevice);
		VK_CHECK_RESULT(lTransferQueue.submit(submission, *fence));

		// Delay the fence and buffer destruction until the GPU has finished the transfer operation
		addFenceDelayedAction(fence, std::bind([](vdu::Buffer* vertexBuffer, vdu::Buffer* indexBuffer, vdu::Fence* fence) -> void {
			fence->destroy();
			vertexBuffer->destroy();
			indexBuffer->destroy();
			delete fence;
			delete vertexBuffer;
			delete indexBuffer;
		}, stagingBufferVertex, stagingBufferIndex, fence));

		// Let the model know where its first vertex and index are located on the GPU
		lodLevel.firstVertex = (s32)(vertexInputByteOffset / sizeof(Vertex));
		lodLevel.firstIndex = (u32)(indexInputByteOffset / sizeof(u32));

		// Increment the offset of where the next model will be added
		vertexInputByteOffset += lodLevel.vertexDataLength * sizeof(Vertex);
		indexInputByteOffset += lodLevel.indexDataLength * sizeof(u32);
	}
}

void Renderer::createDataBuffers()
{
	/// TODO: set appropriate size

	drawCmdBuffer.setMemoryProperty(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	drawCmdBuffer.setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
	drawCmdBuffer.create(&logicalDevice, sizeof(VkDrawIndexedIndirectCommand) * 1000);
	
	VkDeviceSize bufferSize = VERTEX_BUFFER_SIZE + INDEX_BUFFER_SIZE;
	vertexIndexBuffer.setMemoryProperty(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vertexIndexBuffer.setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	vertexIndexBuffer.create(&logicalDevice, bufferSize);

	flatPBRUBO.setMemoryProperty(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	flatPBRUBO.setUsage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	flatPBRUBO.create(&logicalDevice, sizeof(glm::fvec4) * 2 * 100);

	updateFlatMaterialBuffer();

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

	auto stagingBuffer = new vdu::Buffer;
	screenQuadBuffer.createStaging(*stagingBuffer);

	auto fe = new vdu::Fence;
	fe->create(&logicalDevice);

	auto data = stagingBuffer->getMemory()->map();
	memcpy(data, quad.data(), quad.size() * sizeof(Vertex2D));
	stagingBuffer->getMemory()->unmap();

	auto cmd = new vdu::CommandBuffer;
	beginTransferCommands(*cmd);
	stagingBuffer->cmdCopyTo(cmd, &screenQuadBuffer);
	vdu::QueueSubmission submission;
	endTransferCommands(*cmd, submission);

	VK_CHECK_RESULT(lGraphicsQueue.submit(submission, *fe));

	auto delayedBufferDestruct = std::bind([](vdu::Buffer* buffer, vdu::Fence* fe) -> void {
		buffer->destroy();
		fe->destroy();
		delete buffer;
		delete fe;
	}, stagingBuffer, fe);

	addFenceDelayedAction(fe, delayedBufferDestruct);

	createUBOs();
}

void Renderer::updateMaterialDescriptors()
{
	gBufferCmdsNeedUpdate = true;
	gBufferNoTexCmdsNeedUpdate = true;
	Engine::threading->addMaterialMutex.lock();

	auto& assets = Engine::assets;

	u32 textureArrayIndex = 0, flatPBRArrayIndex = 0;
	for (auto& material : assets.materials)
	{
		
		if (material.second.data.textures.albedoSpec != nullptr) {

			// This material consists of two PBR textures

			if (!material.second.checkAvailability(Asset::AWAITING_DESCRIPTOR_UPDATE))
			{
				textureArrayIndex += 2;
				continue;
			}

			material.second.getAvailability() &= ~Asset::AWAITING_DESCRIPTOR_UPDATE;

			auto updater = gBufferDescriptorSet.makeUpdater();
			auto texturesUpdate = updater->addImageUpdate("textures", textureArrayIndex, 2);

			texturesUpdate[0].sampler = textureSampler;
			texturesUpdate[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			texturesUpdate[1].sampler = textureSampler;
			texturesUpdate[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			if (material.second.data.textures.albedoSpec->getHandle())
				texturesUpdate[0].imageView = material.second.data.textures.albedoSpec->getView();
			else
				texturesUpdate[0].imageView = assets.getTexture("blank")->getView();

			if (material.second.data.textures.normalRough)
			{
				if (material.second.data.textures.normalRough->getView())
					texturesUpdate[1].imageView = material.second.data.textures.normalRough->getView();
				else
					texturesUpdate[1].imageView = assets.getTexture("black")->getView();
			}
			else
				texturesUpdate[1].imageView = assets.getTexture("black")->getView();

			textureArrayIndex += 2;

			gBufferDescriptorSet.submitUpdater(updater);
			gBufferDescriptorSet.destroyUpdater(updater);
		}
		else {

			// This material consists of flat PBR data

			/*if (!material.second.checkAvailability(Asset::AWAITING_DESCRIPTOR_UPDATE))
			{
				++flatPBRArrayIndex;
				continue;
			}

			material.second.getAvailability() &= ~Asset::AWAITING_DESCRIPTOR_UPDATE;

			auto updater = gBufferNoTexDescriptorSet.makeUpdater();
			auto pbrDataUpdate = updater->addBufferUpdate("pbrdata", flatPBRArrayIndex);

			pbrDataUpdate->buffer = flatPBRUBO.getHandle();
			pbrDataUpdate->offset = 0;
			pbrDataUpdate->range = sizeof(glm::fvec4) * 2;*/

		}

		
	}

	Engine::threading->addMaterialMutex.unlock();
}

void Renderer::updateFlatMaterialBuffer()
{
	struct PBRData {
		glm::fvec3 c;
		float m;
		float r;
		glm::fvec3 PAD;
	};

	glm::fvec3 cs[5] = { 
		{0.9,0.5,0.2},
		{0.5,0.9,0.4},
		{0.3,0.2,0.9},
		{0.6,0.6,0.5},
		{0.2,0.7,0.1},
	};

	float ms[5] = { 0.0, 0.3, 0.5, 0.7, 0.9 };
	float rs[5] = { 0.0, 0.3, 0.5, 0.7, 0.9 };

	PBRData* b = (PBRData*)flatPBRUBO.getMemory()->map();

	for (int i = 0; i < 100; ++i) {
		b[i].c = cs[i % 5];
		b[i].m = ms[i % 5];
		b[i].r = rs[i % 5];
		b[i].PAD = glm::fvec3(2, 3, 4);
	}

	b[0].c = { 1, 0, 1 };

	flatPBRUBO.getMemory()->unmap();
}

void Renderer::updateSkyboxDescriptor()
{
	if (Engine::world.skybox->checkAvailability(Asset::AWAITING_DESCRIPTOR_UPDATE)) {

		auto updater = pbrDescriptorSet.makeUpdater();
		auto skyboxUpdater = updater->addImageUpdate("skybox");
		*skyboxUpdater = { 
			skySampler, 
			Engine::assets.getTexture(Engine::world.skybox->getName())->getView(), 
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL 
		};

		pbrDescriptorSet.submitUpdater(updater);
		pbrDescriptorSet.destroyUpdater(updater);

		Engine::world.skybox->getAvailability() &= ~Asset::LOADING_TO_GPU;

		updatePBRCommands();
	}
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

	auto& dev = Engine::physicalDevice;
	auto qFams = dev->getQueueFamilies();

	lGraphicsQueue = qFams[0].createQueue(1.f);
	lTransferQueue = qFams[0].createQueue(1.f);

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

void Renderer::createPerThreadCommandPools()
{
	commandPool.setFlags(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	commandPool.setQueueFamily(&Engine::physicalDevice->getQueueFamilies().front());
	commandPool.create(&Engine::renderer->logicalDevice);
}

void Renderer::createQueryPool()
{
	queryPool.setQueryCount(NUM_GPU_TIMESTAMPS);
	queryPool.setQueryType(VK_QUERY_TYPE_TIMESTAMP);
	queryPool.create(&logicalDevice);
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
	samplerInfo.mipLodBias = 0.0f;
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

	
	//samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = samplerInfo.addressModeU;
	samplerInfo.addressModeW = samplerInfo.addressModeU;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 0;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	//samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
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

	ssaoConfigBuffer.setMemoryProperty(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	ssaoConfigBuffer.setUsage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	ssaoConfigBuffer.create(&logicalDevice, sizeof(SSAOConfig));
}

/*
@brief	Create Vulkan descriptor pool
*/
void Renderer::createDescriptorPool()
{
	descriptorPool.addPoolCount(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 20);
	descriptorPool.addPoolCount(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1100);
	descriptorPool.addPoolCount(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1430);
	descriptorPool.addPoolCount(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 13);
	descriptorPool.addSetCount(20);

	descriptorPool.create(&logicalDevice);

	freeableDescriptorPool.addPoolCount(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 200);
	freeableDescriptorPool.addPoolCount(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3);
	freeableDescriptorPool.addSetCount(200);
	freeableDescriptorPool.setFreeable(true);

	freeableDescriptorPool.create(&logicalDevice);
}

/*
@brief	Create Vulkan semaphores
*/
void Renderer::createSynchroObjects()
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	gBufferFinishedSemaphore.create(&logicalDevice);
	imageAvailableSemaphore.create(&logicalDevice);
	pbrFinishedSemaphore.create(&logicalDevice);
	screenFinishedSemaphore.create(&logicalDevice);
	shadowFinishedSemaphore.create(&logicalDevice);
	overlayFinishedSemaphore.create(&logicalDevice);
	overlayCombineFinishedSemaphore.create(&logicalDevice);
	ssaoFinishedSemaphore.create(&logicalDevice);

	gBufferGroupFence.create(&logicalDevice, true);
	pbrGroupFence.create(&logicalDevice, true);
}

void Renderer::beginTransferCommands(vdu::CommandBuffer& cmd)
{
	cmd.allocate(&logicalDevice, &commandPool);
	cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

void Renderer::endTransferCommands(vdu::CommandBuffer& cmd, vdu::QueueSubmission& submission)
{
	cmd.end();
	submission.addCommands(&cmd);
}

void Renderer::submitTransferCommands(vdu::QueueSubmission& submission, vdu::Fence fence)
{
	/*auto submitJobFunc = [&]() -> void
	{
		lTransferQueue.submit(submission, fence);
		//vkFreeCommandBuffers(device, commandPool.getHandle(), 1, submitInfo.pCommandBuffers);
	};
	auto submitJob = new Job<>(submitJobFunc);
	Engine::threading->addGPUTransferJob(submitJob);*/

	//lTransferQueue.submit(submission, fence);
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
	//gBufferGroupFence.wait();

	//PROFILE_MUTEX("transformmutex", Engine::threading->instanceTransformMutex.lock());

	auto tIndex = ModelInstance::toGPUTransformIndex;

	glm::fmat4* transform = (glm::fmat4*)transformUBO.getMemory()->map();

	int i = 0;
	for (auto& m : Engine::world.instancesToDraw)
	{
		transform[m->transformIndex] = m->transform[tIndex].getTransformMat();
		++i;
	}

	transformUBO.getMemory()->unmap();

	//Engine::threading->instanceTransformMutex.unlock();

	ModelInstance::toGPUTransformIndex = tIndex == 0 ? 1 : 0;
}

void Renderer::updateSSAOConfigBuffer()
{
	Engine::console->postMessage("Updating SSAO config buffer", glm::fvec3(0.1, 0.1, 0.9));

	const auto& ssao = Engine::config.render.ssao;

	ssaoConfigData.samples = ssao.getSamples();
	ssaoConfigData.spiralTurns = ssao.getSpiralTurns();
	ssaoConfigData.projScale = ssao.getProjScale();
	ssaoConfigData.radius = ssao.getRadius();
	ssaoConfigData.bias = ssao.getBias();
	ssaoConfigData.intensity = ssao.getIntensityInternal();

	auto& p = Engine::camera.getProj();
	auto& r = renderResolution;

	ssaoConfigData.projInfo = glm::fvec4(-2.f / (r.width * p[0][0]),
										-2.f / (r.height*p[1][1]),
										(1.f - p[0][2]) / p[0][0],
										(1.f + p[1][2]) / p[1][1]);

	void* data = ssaoConfigBuffer.getMemory()->map();
	memcpy(data, &ssaoConfigData, sizeof(ssaoConfigData));
	ssaoConfigBuffer.getMemory()->unmap();
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
	vkCmdPipelineBarrier(
		cmdbuffer,
		srcStageMask,
		dstStageMask,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageBarrier);
}

void Renderer::setImageLayout(vdu::CommandBuffer * cmdbuffer, Texture & tex, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
{
	setImageLayout(cmdbuffer->getHandle(), tex, oldImageLayout, newImageLayout, srcStageMask, dstStageMask);
}


