#include "PCH.hpp"
#include "Material.hpp"

void Material::loadToRAM(void * pCreateStruct, AllocFunc alloc)
{
	if (!albedoSpec->checkAvailability(ON_RAM) && !albedoSpec->checkAvailability(LOADING_TO_RAM))
	{
		albedoSpec->getAvailability() |= LOADING_TO_RAM;

		TextureCreateInfo ci;
		ci.genMipMaps = true;
		ci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		ci.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ci.layers = 1;
		ci.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		ci.format = VK_FORMAT_R8G8B8A8_UNORM;

		albedoSpec->loadToRAM(&ci);
	}

	if (!normalRough->checkAvailability(ON_RAM) && !normalRough->checkAvailability(LOADING_TO_RAM))
	{
		normalRough->getAvailability() |= LOADING_TO_RAM;

		TextureCreateInfo ci;
		ci.genMipMaps = true;
		ci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		ci.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ci.layers = 1;
		ci.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		ci.format = VK_FORMAT_R8G8B8A8_UNORM;

		normalRough->loadToRAM(&ci);
	}
}

void Material::loadToGPU(void * pCreateStruct)
{
	if (!albedoSpec->checkAvailability(ON_GPU) && !albedoSpec->checkAvailability(LOADING_TO_GPU))
	{
		albedoSpec->getAvailability() |= LOADING_TO_GPU;
		albedoSpec->loadToGPU();
		availability |= AWAITING_DESCRIPTOR_UPDATE;
	}

	if (!normalRough->checkAvailability(ON_GPU) && !normalRough->checkAvailability(LOADING_TO_GPU))
	{
		normalRough->getAvailability() |= LOADING_TO_GPU;
		normalRough->loadToGPU();
		availability |= AWAITING_DESCRIPTOR_UPDATE;
	}
}
