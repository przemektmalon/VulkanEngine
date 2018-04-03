#include "PCH.hpp"
#include "Renderer.hpp"

void Renderer::createPBRAttachments()
{
	TextureCreateInfo tci;
	tci.width = renderResolution.width;
	tci.height = renderResolution.height;
	tci.bpp = 32;
	tci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	tci.format = VK_FORMAT_B8G8R8A8_UNORM;
	tci.layout = VK_IMAGE_LAYOUT_UNDEFINED;
	tci.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	pbrOutput.loadStream(&tci);
}

void Renderer::createPBRDescriptorSetLayouts()
{
	VkDescriptorSetLayoutBinding colLayoutBinding = {};
	colLayoutBinding.binding = 0;
	colLayoutBinding.descriptorCount = 1;
	colLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	colLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding norLayoutBinding = {};
	norLayoutBinding.binding = 1;
	norLayoutBinding.descriptorCount = 1;
	norLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	norLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	/// TODO: other PBR stuff

	VkDescriptorSetLayoutBinding bindings[] = { colLayoutBinding, norLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 2;
	layoutInfo.pBindings = bindings;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &screenDescriptorSetLayout) != VK_SUCCESS) {
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
	pipelineLayoutInfo.pSetLayouts = &gBufferDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &gBufferPipelineLayout) != VK_SUCCESS) {
		DBG_SEVERE("Failed to create Vulkan pipeline layout");
	}

	// Collate all the data necessary to create pipeline
	VkComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = *pbrShader.getShaderStageCreateInfos();
	pipelineInfo.layout = gBufferPipelineLayout;

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

	VkDescriptorImageInfo colInfoPBR;
	colInfoPBR.sampler = textureSampler;
	colInfoPBR.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	colInfoPBR.imageView = pbrOutput.getImageViewHandle();

	std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = pbrDescriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &colInfoPBR;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}