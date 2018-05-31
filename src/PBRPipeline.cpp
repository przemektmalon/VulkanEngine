#include "PCH.hpp"
#include "Renderer.hpp"

void Renderer::createPBRAttachments()
{
	TextureCreateInfo tci;
	tci.width = renderResolution.width;
	tci.height = renderResolution.height;
	tci.bpp = 32 * 4;
	tci.components = 4;
	tci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	tci.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	tci.layout = VK_IMAGE_LAYOUT_GENERAL;
	tci.usageFlags = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	pbrOutput.loadToGPU(&tci);
}

void Renderer::createPBRDescriptorSetLayouts()
{
	auto& dsl = pbrDescriptorSetLayout; // for brevity and readability

	dsl.addBinding(vdu::DescriptorType::StorageImage, 0, 1, vdu::ShaderStage::Compute); // out
	dsl.addBinding(vdu::DescriptorType::StorageImage, 1, 1, vdu::ShaderStage::Compute); // colour
	dsl.addBinding(vdu::DescriptorType::StorageImage, 2, 1, vdu::ShaderStage::Compute); // normal
	dsl.addBinding(vdu::DescriptorType::StorageImage, 3, 1, vdu::ShaderStage::Compute); // pbr
	dsl.addBinding(vdu::DescriptorType::CombinedImageSampler, 4, 1, vdu::ShaderStage::Compute); // depth
	dsl.addBinding(vdu::DescriptorType::CombinedImageSampler, 5, 1, vdu::ShaderStage::Compute); // skybox

	dsl.addBinding(vdu::DescriptorType::UniformBuffer, 9, 1, vdu::ShaderStage::Compute); // camera
	dsl.addBinding(vdu::DescriptorType::UniformBuffer, 10, 1, vdu::ShaderStage::Compute); // point lights data
	dsl.addBinding(vdu::DescriptorType::UniformBuffer, 11, 1, vdu::ShaderStage::Compute); // spot lights data
	dsl.addBinding(vdu::DescriptorType::UniformBuffer, 12, 1, vdu::ShaderStage::Compute); // light counts

	dsl.addBinding(vdu::DescriptorType::CombinedImageSampler, 13, 150, vdu::ShaderStage::Compute); // point shadows
	dsl.addBinding(vdu::DescriptorType::CombinedImageSampler, 14, 150, vdu::ShaderStage::Compute); // spot shadows
	dsl.addBinding(vdu::DescriptorType::CombinedImageSampler, 15, 1, vdu::ShaderStage::Compute); // sun shadow

	dsl.addBinding(vdu::DescriptorType::UniformBuffer, 16, 1, vdu::ShaderStage::Compute); // sun light data

	dsl.create(&logicalDevice);
}

void Renderer::createPBRPipeline()
{
	// Compile GLSL code to SPIR-V

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
	VkDescriptorSetLayout layouts[] = { pbrDescriptorSetLayout.getHandle() };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool.getHandle();
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &pbrDescriptorSet));
}

void Renderer::updatePBRDescriptorSets()
{
	VkDescriptorImageInfo outputImageInfo;
	outputImageInfo.sampler = textureSampler;
	outputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	outputImageInfo.imageView = pbrOutput.getImageViewHandle();

	VkDescriptorImageInfo colourImageInfo;
	colourImageInfo.sampler = textureSampler;
	colourImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	colourImageInfo.imageView = gBufferColourAttachment.getImageViewHandle();

	VkDescriptorImageInfo normalImageInfo;
	normalImageInfo.sampler = textureSampler;
	normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	normalImageInfo.imageView = gBufferNormalAttachment.getImageViewHandle();

	VkDescriptorImageInfo pbrImageInfo;
	pbrImageInfo.sampler = textureSampler;
	pbrImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	pbrImageInfo.imageView = gBufferPBRAttachment.getImageViewHandle();

	VkDescriptorImageInfo depthImageInfo;
	depthImageInfo.sampler = textureSampler;
	depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	depthImageInfo.imageView = gBufferDepthAttachment.getImageViewHandle();

	VkDescriptorImageInfo skyImageInfo;
	skyImageInfo.sampler = skySampler;
	skyImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	skyImageInfo.imageView = Engine::assets.getTexture("skybox")->getImageViewHandle();



	VkDescriptorBufferInfo lightCountsInfo;
	lightCountsInfo.buffer = lightManager.lightCountsBuffer.getBuffer();
	lightCountsInfo.offset = 0;
	lightCountsInfo.range = 2 * sizeof(u32);

	VkDescriptorBufferInfo cameraInfo;
	cameraInfo.buffer = cameraUBO.getBuffer();
	cameraInfo.offset = 0;
	cameraInfo.range = sizeof(CameraUBOData);

	std::array<VkWriteDescriptorSet, 8> descriptorWrites = {};

	// We'll need:
	// 3 Light buffers (point, spot, direction)
	// 4 GBuffer images (albedospec, normal, pbr, depth)
	// SSAO image
	// Skybox image
	// Output image
	// Uniforms:
	// 
	

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = pbrDescriptorSet;
	descriptorWrites[0].dstBinding = 1;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &colourImageInfo;
	
	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = pbrDescriptorSet;
	descriptorWrites[1].dstBinding = 2;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &normalImageInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = pbrDescriptorSet;
	descriptorWrites[2].dstBinding = 0;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pImageInfo = &outputImageInfo;

	descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[3].dstSet = pbrDescriptorSet;
	descriptorWrites[3].dstBinding = 3;
	descriptorWrites[3].dstArrayElement = 0;
	descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites[3].descriptorCount = 1;
	descriptorWrites[3].pImageInfo = &pbrImageInfo;

	descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[4].dstSet = pbrDescriptorSet;
	descriptorWrites[4].dstBinding = 4;
	descriptorWrites[4].dstArrayElement = 0;
	descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[4].descriptorCount = 1;
	descriptorWrites[4].pImageInfo = &depthImageInfo;

	descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[5].dstSet = pbrDescriptorSet;
	descriptorWrites[5].dstBinding = 9;
	descriptorWrites[5].dstArrayElement = 0;
	descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[5].descriptorCount = 1;
	descriptorWrites[5].pBufferInfo = &cameraInfo;

	descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[6].dstSet = pbrDescriptorSet;
	descriptorWrites[6].dstBinding = 12;
	descriptorWrites[6].dstArrayElement = 0;
	descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[6].descriptorCount = 1;
	descriptorWrites[6].pBufferInfo = &lightCountsInfo;

	descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[7].dstSet = pbrDescriptorSet;
	descriptorWrites[7].dstBinding = 5;
	descriptorWrites[7].dstArrayElement = 0;
	descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[7].descriptorCount = 1;
	descriptorWrites[7].pImageInfo = &skyImageInfo;

	VK_VALIDATE(vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr));

	std::vector<VkWriteDescriptorSet> descriptorWrites2 = {};
	descriptorWrites2.reserve(4);

	if (lightManager.pointLights.size())
	{
		VkDescriptorImageInfo* pointShadowsInfo = new VkDescriptorImageInfo[lightManager.pointLights.size()]; /// TODO: delete

		int i = 0;
		for (auto& s : lightManager.pointLights)
		{
			pointShadowsInfo[i].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			pointShadowsInfo[i].imageView = s.getShadowTexture()->getImageViewHandle();
			pointShadowsInfo[i].sampler = shadowSampler;
			++i;
		}

		VkDescriptorBufferInfo pointLightsInfo;
		pointLightsInfo.buffer = lightManager.pointLightsBuffer.getBuffer();
		pointLightsInfo.offset = 0;
		pointLightsInfo.range = lightManager.pointLights.size() * sizeof(PointLight::GPUData);

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = pbrDescriptorSet;
		write.dstBinding = 13;
		write.dstArrayElement = 0;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.descriptorCount = lightManager.pointLights.size();
		write.pImageInfo = pointShadowsInfo;

		descriptorWrites2.push_back(write);

		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = pbrDescriptorSet;
		write.dstBinding = 10;
		write.dstArrayElement = 0;
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.descriptorCount = 1;
		write.pBufferInfo = &pointLightsInfo;

		descriptorWrites2.push_back(write);
	}

	if (lightManager.spotLights.size())
	{
		VkDescriptorImageInfo* spotShadowsInfo = new VkDescriptorImageInfo[lightManager.spotLights.size()]; /// TODO: delete

		int i = 0;
		for (auto& s : lightManager.spotLights)
		{
			spotShadowsInfo[i].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			spotShadowsInfo[i].imageView = s.getShadowTexture()->getImageViewHandle();
			spotShadowsInfo[i].sampler = shadowSampler;
			++i;
		}

		VkDescriptorBufferInfo spotLightsInfo;
		spotLightsInfo.buffer = lightManager.spotLightsBuffer.getBuffer();
		spotLightsInfo.offset = 0;
		spotLightsInfo.range = lightManager.spotLights.size() * sizeof(SpotLight::GPUData);

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = pbrDescriptorSet;
		write.dstBinding = 14;
		write.dstArrayElement = 0;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.descriptorCount = lightManager.spotLights.size();
		write.pImageInfo = spotShadowsInfo;

		descriptorWrites2.push_back(write);

		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = pbrDescriptorSet;
		write.dstBinding = 11;
		write.dstArrayElement = 0;
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.descriptorCount = 1;
		write.pBufferInfo = &spotLightsInfo;

		descriptorWrites2.push_back(write);
	}

	if (lightManager.sunLight.shadowTex)
	{
		auto sunShadowInfo = new VkDescriptorImageInfo;

		sunShadowInfo->imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		sunShadowInfo->imageView = lightManager.sunLight.getShadowTexture()->getImageViewHandle();
		sunShadowInfo->sampler = shadowSampler;

		auto sunLightInfo = new VkDescriptorBufferInfo;
		sunLightInfo->buffer = lightManager.sunLightBuffer.getBuffer();
		sunLightInfo->offset = 0;
		sunLightInfo->range = sizeof(SunLight::GPUData);

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = pbrDescriptorSet;
		write.dstBinding = 15;
		write.dstArrayElement = 0;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.descriptorCount = 1;
		write.pImageInfo = sunShadowInfo;

		descriptorWrites2.push_back(write);

		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = pbrDescriptorSet;
		write.dstBinding = 16;
		write.dstArrayElement = 0;
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.descriptorCount = 1;
		write.pBufferInfo = sunLightInfo;

		descriptorWrites2.push_back(write);
	}

	VK_VALIDATE(vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites2.size()), descriptorWrites2.data(), 0, nullptr));
}

void Renderer::createPBRCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
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
	VK_VALIDATE(vkCmdBindDescriptorSets(pbrCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pbrPipelineLayout, 0, 1, &pbrDescriptorSet, 0, 0));

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
	VK_CHECK_RESULT(vkFreeDescriptorSets(device, descriptorPool.getHandle(), 1, &pbrDescriptorSet));
}

void Renderer::destroyPBRCommands()
{
	VK_VALIDATE(vkFreeCommandBuffers(device, commandPool, 1, &pbrCommandBuffer));
}