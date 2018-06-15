#include "PCH.hpp"
#include "Renderer.hpp"

void Renderer::createPBRAttachments()
{
	TextureCreateInfo tci;
	tci.width = renderResolution.width;
	tci.height = renderResolution.height;
	tci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	tci.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	tci.layout = VK_IMAGE_LAYOUT_GENERAL;
	tci.usageFlags = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	pbrOutput.loadToGPU(&tci);
}

void Renderer::createPBRDescriptorSetLayouts()
{
	auto& dsl = pbrDescriptorSetLayout; // for brevity and readability

	dsl.addBinding("output", vdu::DescriptorType::StorageImage, 0, 1, vdu::ShaderStage::Compute);
	dsl.addBinding("albedo", vdu::DescriptorType::StorageImage, 1, 1, vdu::ShaderStage::Compute);
	dsl.addBinding("normal", vdu::DescriptorType::StorageImage, 2, 1, vdu::ShaderStage::Compute);
	dsl.addBinding("pbr", vdu::DescriptorType::StorageImage, 3, 1, vdu::ShaderStage::Compute);
	dsl.addBinding("depth", vdu::DescriptorType::CombinedImageSampler, 4, 1, vdu::ShaderStage::Compute);
	dsl.addBinding("skybox", vdu::DescriptorType::CombinedImageSampler, 5, 1, vdu::ShaderStage::Compute);

	dsl.addBinding("camera", vdu::DescriptorType::UniformBuffer, 9, 1, vdu::ShaderStage::Compute);
	dsl.addBinding("point_lights", vdu::DescriptorType::UniformBuffer, 10, 1, vdu::ShaderStage::Compute);
	dsl.addBinding("spot_lights", vdu::DescriptorType::UniformBuffer, 11, 1, vdu::ShaderStage::Compute);
	dsl.addBinding("light_counts", vdu::DescriptorType::UniformBuffer, 12, 1, vdu::ShaderStage::Compute);

	dsl.addBinding("point_shadows", vdu::DescriptorType::CombinedImageSampler, 13, 150, vdu::ShaderStage::Compute);
	dsl.addBinding("spot_shadows", vdu::DescriptorType::CombinedImageSampler, 14, 150, vdu::ShaderStage::Compute);
	dsl.addBinding("sun_shadow", vdu::DescriptorType::CombinedImageSampler, 15, 3, vdu::ShaderStage::Compute);

	dsl.addBinding("sun_light", vdu::DescriptorType::UniformBuffer, 16, 1, vdu::ShaderStage::Compute);

	dsl.create(&logicalDevice);
}

void Renderer::createPBRPipeline()
{
	// Compile GLSL code to SPIR-V

	pbrShader.create(&logicalDevice);
	pbrShader.compile();

	// Pipeline layout for specifying descriptor sets (shaders use to access buffer and image resources indirectly)
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &pbrDescriptorSetLayout.getHandle();
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pbrPipelineLayout));

	// Collate all the data necessary to create pipeline
	VkComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = *pbrShader.getShaderStageCreateInfos();
	pipelineInfo.layout = pbrPipelineLayout;

	VK_CHECK_RESULT(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pbrPipeline));
}

void Renderer::createPBRDescriptorSets()
{
	pbrDescriptorSet.create(&logicalDevice, &pbrDescriptorSetLayout, &descriptorPool);
}

void Renderer::updatePBRDescriptorSets()
{
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

	auto skyboxUpdater = updater->addImageUpdate("skybox");
	*skyboxUpdater = { skySampler, Engine::assets.getTexture("skybox")->getView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

	auto lightCountsUpdater = updater->addBufferUpdate("light_counts");
	*lightCountsUpdater = { lightManager.lightCountsBuffer.getHandle(), 0, 2 * sizeof(u32) };

	auto cameraUpdater = updater->addBufferUpdate("camera");
	*cameraUpdater = { cameraUBO.getHandle(), 0, sizeof(CameraUBOData) };

	if (lightManager.pointLights.size())
	{
		auto pointLightsUpdater = updater->addBufferUpdate("point_lights");
		*pointLightsUpdater = { lightManager.pointLightsBuffer.getHandle(), 0, lightManager.pointLights.size() * sizeof(PointLight::GPUData) };

		auto pointShadowsUpdater = updater->addImageUpdate("point_shadows", 0, lightManager.pointLights.size());
		int i = 0;
		for (auto& s : lightManager.pointLights)
		{
			pointShadowsUpdater[i] = { shadowSampler, s.getShadowTexture()->getView(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
			++i;
		}
	}

	if (lightManager.spotLights.size())
	{
		auto spotLightsUpdater = updater->addBufferUpdate("spot_lights");
		*spotLightsUpdater = { lightManager.spotLightsBuffer.getHandle(), 0, lightManager.spotLights.size() * sizeof(SpotLight::GPUData) };

		auto spotShadowsUpdater = updater->addImageUpdate("spot_shadows", 0, lightManager.spotLights.size());
		int i = 0;
		for (auto& s : lightManager.spotLights)
		{
			spotShadowsUpdater[i] = { shadowSampler, s.getShadowTexture()->getView(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
			++i;
		}
	}

	if (lightManager.sunLight.shadowTex)
	{
		auto sunLightUpdater = updater->addBufferUpdate("sun_light");
		*sunLightUpdater = { lightManager.sunLightBuffer.getHandle(), 0, sizeof(SunLight::GPUData) };

		auto sunShadowUpdater = updater->addImageUpdate("sun_shadow", 0, 3);
		for (int i = 0; i < 3; ++i)
		{
			sunShadowUpdater[i] = { shadowSampler, lightManager.sunLight.getShadowTexture()[i].getView(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
		}
	}
	
	pbrDescriptorSet.submitUpdater(updater);
	pbrDescriptorSet.destroyUpdater(updater);

}

void Renderer::createPBRCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool.getHandle();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, &pbrCommandBuffer));
}

void Renderer::updatePBRCommands()
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VK_CHECK_RESULT(vkBeginCommandBuffer(pbrCommandBuffer, &beginInfo));

	VK_VALIDATE(vkCmdWriteTimestamp(pbrCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, queryPool, BEGIN_PBR));

	VK_VALIDATE(vkCmdBindPipeline(pbrCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pbrPipeline));
	VK_VALIDATE(vkCmdBindDescriptorSets(pbrCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pbrPipelineLayout, 0, 1, &pbrDescriptorSet.getHandle(), 0, 0));

	VK_VALIDATE(vkCmdDispatch(pbrCommandBuffer, renderResolution.width / 16, renderResolution.height / 16, 1));

	setImageLayout(pbrCommandBuffer, gBufferColourAttachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	setImageLayout(pbrCommandBuffer, gBufferNormalAttachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	setImageLayout(pbrCommandBuffer, gBufferPBRAttachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	setImageLayout(pbrCommandBuffer, gBufferDepthAttachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);

	setImageLayout(pbrCommandBuffer, pbrOutput, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	VK_VALIDATE(vkCmdWriteTimestamp(pbrCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, END_PBR));

	VK_CHECK_RESULT(vkEndCommandBuffer(pbrCommandBuffer));
}

void Renderer::destroyPBRAttachments()
{
	pbrOutput.destroy();
}

void Renderer::destroyPBRDescriptorSetLayouts()
{
	pbrDescriptorSetLayout.destroy();
}

void Renderer::destroyPBRPipeline()
{
	VK_VALIDATE(vkDestroyPipelineLayout(device, pbrPipelineLayout, 0));
	VK_VALIDATE(vkDestroyPipeline(device, pbrPipeline, 0));
	pbrShader.destroy();
}

void Renderer::destroyPBRDescriptorSets()
{
	pbrDescriptorSet.destroy();
}

void Renderer::destroyPBRCommands()
{
	VK_VALIDATE(vkFreeCommandBuffers(device, commandPool.getHandle(), 1, &pbrCommandBuffer));
}