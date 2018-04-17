#include "PCH.hpp"
#include "Renderer.hpp"

void Renderer::createPBRAttachments()
{
	TextureCreateInfo tci;
	tci.width = renderResolution.width;
	tci.height = renderResolution.height;
	tci.bpp = 32 * 4;
	tci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	tci.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	tci.layout = VK_IMAGE_LAYOUT_GENERAL;
	tci.usageFlags = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	pbrOutput.loadStream(&tci);
}

void Renderer::createPBRDescriptorSetLayouts()
{
	VkDescriptorSetLayoutBinding outLayoutBinding = {};
	outLayoutBinding.binding = 0;
	outLayoutBinding.descriptorCount = 1;
	outLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	outLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding colLayoutBinding = {};
	colLayoutBinding.binding = 1;
	colLayoutBinding.descriptorCount = 1;
	colLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	colLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding norLayoutBinding = {};
	norLayoutBinding.binding = 2;
	norLayoutBinding.descriptorCount = 1;
	norLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	norLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding pbrLayoutBinding = {};
	pbrLayoutBinding.binding = 3;
	pbrLayoutBinding.descriptorCount = 1;
	pbrLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	pbrLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding depthLayoutBinding = {};
	depthLayoutBinding.binding = 4;
	depthLayoutBinding.descriptorCount = 1;
	depthLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	depthLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding pointLightsLayoutBinding = {};
	pointLightsLayoutBinding.binding = 10;
	pointLightsLayoutBinding.descriptorCount = 1;
	pointLightsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pointLightsLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding spotLightsLayoutBinding = {};
	spotLightsLayoutBinding.binding = 11;
	spotLightsLayoutBinding.descriptorCount = 1;
	spotLightsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	spotLightsLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding lightCountsLayoutBinding = {};
	lightCountsLayoutBinding.binding = 12;
	lightCountsLayoutBinding.descriptorCount = 1;
	lightCountsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	lightCountsLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutBinding cameraLayoutBinding = {};
	cameraLayoutBinding.binding = 9;
	cameraLayoutBinding.descriptorCount = 1;
	cameraLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	cameraLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;


	VkDescriptorSetLayoutBinding wsdLayoutBinding = {};
	wsdLayoutBinding.binding = 5;
	wsdLayoutBinding.descriptorCount = 1;
	wsdLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	wsdLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;


	VkDescriptorSetLayoutBinding bindings[] = { outLayoutBinding, colLayoutBinding, norLayoutBinding, pbrLayoutBinding, depthLayoutBinding, pointLightsLayoutBinding, spotLightsLayoutBinding, cameraLayoutBinding, wsdLayoutBinding, lightCountsLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 10;
	layoutInfo.pBindings = bindings;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &pbrDescriptorSetLayout) != VK_SUCCESS) {
		DBG_SEVERE("Failed to create Vulkan descriptor set layout");
	}
}

void Renderer::createPBRPipeline()
{
	// Compile GLSL code to SPIR-V

	pbrShader.compile();

	// Pipeline layout for specifying descriptor sets (shaders use to access buffer and image resources indirectly)
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &pbrDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pbrPipelineLayout) != VK_SUCCESS) {
		DBG_SEVERE("Failed to create Vulkan pipeline layout");
	}

	// Collate all the data necessary to create pipeline
	VkComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = *pbrShader.getShaderStageCreateInfos();
	pipelineInfo.layout = pbrPipelineLayout;

	VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pbrPipeline);

	if (result != VK_SUCCESS) {
		DBG_SEVERE("Failed to create Vulkan graphics pipeline");
	}
}

void Renderer::createPBRDescriptorSets()
{
	VkDescriptorSetLayout layouts[] = { pbrDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocInfo, &pbrDescriptorSet) != VK_SUCCESS) {
		DBG_SEVERE("Failed to allocate descriptor set");
	}
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

	VkDescriptorBufferInfo pointLightsInfo;
	pointLightsInfo.buffer = lightManager.pointLightsBuffer.getBuffer();
	pointLightsInfo.offset = 0;
	pointLightsInfo.range = 150 * sizeof(PointLight::GPUData);
	
	VkDescriptorBufferInfo spotLightsInfo;
	spotLightsInfo.buffer = lightManager.spotLightsBuffer.getBuffer();
	spotLightsInfo.offset = 0;
	spotLightsInfo.range = 150 * sizeof(SpotLight::GPUData);

	VkDescriptorBufferInfo lightCountsInfo;
	lightCountsInfo.buffer = lightManager.lightCountsBuffer.getBuffer();
	lightCountsInfo.offset = 0;
	lightCountsInfo.range = 2 * sizeof(u32);

	VkDescriptorBufferInfo cameraInfo;
	cameraInfo.buffer = cameraUBO.getBuffer();
	cameraInfo.offset = 0;
	cameraInfo.range = sizeof(CameraUBOData);

	std::array<VkWriteDescriptorSet, 9> descriptorWrites = {};

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
	descriptorWrites[5].dstBinding = 10;
	descriptorWrites[5].dstArrayElement = 0;
	descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[5].descriptorCount = 1;
	descriptorWrites[5].pBufferInfo = &pointLightsInfo;

	descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[6].dstSet = pbrDescriptorSet;
	descriptorWrites[6].dstBinding = 11;
	descriptorWrites[6].dstArrayElement = 0;
	descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[6].descriptorCount = 1;
	descriptorWrites[6].pBufferInfo = &spotLightsInfo;

	descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[7].dstSet = pbrDescriptorSet;
	descriptorWrites[7].dstBinding = 9;
	descriptorWrites[7].dstArrayElement = 0;
	descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[7].descriptorCount = 1;
	descriptorWrites[7].pBufferInfo = &cameraInfo;

	descriptorWrites[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[8].dstSet = pbrDescriptorSet;
	descriptorWrites[8].dstBinding = 12;
	descriptorWrites[8].dstArrayElement = 0;
	descriptorWrites[8].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[8].descriptorCount = 1;
	descriptorWrites[8].pBufferInfo = &lightCountsInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Renderer::createPBRCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &pbrCommandBuffer) != VK_SUCCESS) {
		DBG_SEVERE("Failed to allocate Vulkan command buffers");
	}
}

void Renderer::updatePBRCommands()
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	vkBeginCommandBuffer(pbrCommandBuffer, &beginInfo);

	vkCmdBindPipeline(pbrCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pbrPipeline);
	vkCmdBindDescriptorSets(pbrCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pbrPipelineLayout, 0, 1, &pbrDescriptorSet, 0, 0);
	vkCmdDispatch(pbrCommandBuffer, renderResolution.width / 16, renderResolution.height / 16, 1);

	auto result = vkEndCommandBuffer(pbrCommandBuffer);
	if (result != VK_SUCCESS) {
		DBG_SEVERE("Failed to record Vulkan command buffer. Error: " << result);
	}
}

void Renderer::destroyPBRAttachments()
{
	pbrOutput.destroy();
}

void Renderer::destroyPBRDescriptorSetLayouts()
{
	vkDestroyDescriptorSetLayout(device, pbrDescriptorSetLayout, 0);
}

void Renderer::destroyPBRPipeline()
{
	vkDestroyPipelineLayout(device, pbrPipelineLayout, 0);
	vkDestroyPipeline(device, pbrPipeline, 0);
}

void Renderer::destroyPBRDescriptorSets()
{
	vkFreeDescriptorSets(device, descriptorPool, 1, &pbrDescriptorSet);
}

void Renderer::destroyPBRCommands()
{
	vkFreeCommandBuffers(device, commandPool, 1, &pbrCommandBuffer);
}