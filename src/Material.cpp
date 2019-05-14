#include "PCH.hpp"
#include "Material.hpp"

void Material::loadToRAM(void * pCreateStruct, AllocFunc alloc)
{
	if (!data.textures.albedoSpec->checkAvailability(ON_RAM) && !data.textures.albedoSpec->checkAvailability(LOADING_TO_RAM))
	{
		data.textures.albedoSpec->getAvailability() |= LOADING_TO_RAM;

		TextureCreateInfo ci;
		ci.genMipMaps = true;
		ci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		ci.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ci.layers = 1;
		ci.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		ci.format = VK_FORMAT_R8G8B8A8_UNORM;

		data.textures.albedoSpec->loadToRAM(&ci);
	}

	if (!data.textures.normalRough->checkAvailability(ON_RAM) && !data.textures.normalRough->checkAvailability(LOADING_TO_RAM))
	{
		data.textures.normalRough->getAvailability() |= LOADING_TO_RAM;

		TextureCreateInfo ci;
		ci.genMipMaps = true;
		ci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		ci.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ci.layers = 1;
		ci.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		ci.format = VK_FORMAT_R8G8B8A8_UNORM;

		data.textures.normalRough->loadToRAM(&ci);
	}
}

void Material::loadToGPU(void * pCreateStruct)
{
	if (!data.textures.albedoSpec->checkAvailability(ON_GPU) && !data.textures.albedoSpec->checkAvailability(LOADING_TO_GPU))
	{
		data.textures.albedoSpec->getAvailability() |= LOADING_TO_GPU;
		data.textures.albedoSpec->loadToGPU();
		availability |= AWAITING_DESCRIPTOR_UPDATE;
	}

	if (!data.textures.normalRough->checkAvailability(ON_GPU) && !data.textures.normalRough->checkAvailability(LOADING_TO_GPU))
	{
		data.textures.normalRough->getAvailability() |= LOADING_TO_GPU;
		data.textures.normalRough->loadToGPU();
		availability |= AWAITING_DESCRIPTOR_UPDATE;
	}
}
