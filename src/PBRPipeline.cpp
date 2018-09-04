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
	//tci.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
	dsl.addBinding("ssao", vdu::DescriptorType::StorageImage, 6, 1, vdu::ShaderStage::Compute);

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
	pbrPipelineLayout.addDescriptorSetLayout(&pbrDescriptorSetLayout);
	pbrPipelineLayout.create(&logicalDevice);

	pbrPipeline.setShaderProgram(&pbrShader);
	pbrPipeline.setPipelineLayout(&pbrPipelineLayout);
	pbrPipeline.create(&logicalDevice);
}

void Renderer::createPBRDescriptorSets()
{
	pbrDescriptorSet.allocate(&logicalDevice, &pbrDescriptorSetLayout, &descriptorPool);
}

void Renderer::updatePBRDescriptorSets()
{
	auto defaultUpdater = pbrDescriptorSet.makeUpdater();

	auto pointLightsUpdater = defaultUpdater->addBufferUpdate("point_lights");
	*pointLightsUpdater = { lightManager.pointLightsBuffer.getHandle(), 0, 150 * sizeof(PointLight::GPUData) };

	auto pointShadowsUpdater = defaultUpdater->addImageUpdate("point_shadows", 0, 150);
	for (int i = 0; i < 150; ++i)
	{
		pointShadowsUpdater[i] = { shadowSampler, Engine::assets.getTexture("blankCube")->getView(), VK_IMAGE_LAYOUT_GENERAL };
	}

	auto spotLightsUpdater = defaultUpdater->addBufferUpdate("spot_lights");
	*spotLightsUpdater = { lightManager.spotLightsBuffer.getHandle(), 0, 150 * sizeof(SpotLight::GPUData) };

	auto spotShadowsUpdater = defaultUpdater->addImageUpdate("spot_shadows", 0, 150);
	for (int i = 0; i < 150; ++i)
	{
		spotShadowsUpdater[i] = { shadowSampler, Engine::assets.getTexture("blank")->getView(), VK_IMAGE_LAYOUT_GENERAL };
	}

	auto sunLightUpdater = defaultUpdater->addBufferUpdate("sun_light");
	*sunLightUpdater = { lightManager.sunLightBuffer.getHandle(), 0, sizeof(SunLight::GPUData) };

	auto sunShadowUpdater = defaultUpdater->addImageUpdate("sun_shadow", 0, 3);
	for (int i = 0; i < 3; ++i)
	{
		sunShadowUpdater[i] = { shadowSampler, Engine::assets.getTexture("blank")->getView(), VK_IMAGE_LAYOUT_GENERAL };
	}

	pbrDescriptorSet.submitUpdater(defaultUpdater);
	pbrDescriptorSet.destroyUpdater(defaultUpdater);

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
	*skyboxUpdater = { skySampler, Engine::assets.getTexture("blankCube")->getView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

	auto ssaoUpdater = updater->addImageUpdate("ssao");
	*ssaoUpdater = { textureSampler, ssaoFinalAttachment.getView(), VK_IMAGE_LAYOUT_GENERAL };

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
	pbrCommandBuffer.allocate(&logicalDevice, &commandPool);
}

void Renderer::updatePBRCommands()
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	auto cmd = pbrCommandBuffer.getHandle();

	VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &beginInfo));

	queryPool.cmdTimestamp(pbrCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, BEGIN_PBR);

	VK_VALIDATE(vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pbrPipeline.getHandle()));
	VK_VALIDATE(vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pbrPipelineLayout.getHandle(), 0, 1, &pbrDescriptorSet.getHandle(), 0, 0));

	VK_VALIDATE(vkCmdDispatch(cmd, renderResolution.width / 16, renderResolution.height / 16, 1));

	setImageLayout(cmd, gBufferColourAttachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	setImageLayout(cmd, gBufferNormalAttachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	setImageLayout(cmd, gBufferPBRAttachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	setImageLayout(cmd, gBufferDepthAttachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);

	setImageLayout(cmd, pbrOutput, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	queryPool.cmdTimestamp(pbrCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, END_PBR);

	VK_CHECK_RESULT(vkEndCommandBuffer(cmd));
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
	pbrPipelineLayout.destroy();
	pbrPipeline.destroy();
}

void Renderer::destroyPBRDescriptorSets()
{
	pbrDescriptorSet.free();
}

void Renderer::destroyPBRCommands()
{
	pbrCommandBuffer.free();
}