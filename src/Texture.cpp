#include "Texture.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

void Texture::loadFile(std::string pPath, bool genMipMaps)
{
	Image img;
	img.load(pPath);
	if (img.data.size() == 0)
	{
		DBG_WARNING("Failed to load image: " << pPath);
		/// TODO: Load a default null texture
		return;
	}
	loadImage(&img, genMipMaps);
}

void Texture::loadImage(Image * pImage, bool genMipMaps)
{
	VkDeviceSize textureSize = pImage->data.size() * sizeof(Pixel);
	width = pImage->width; height = pImage->height;

	if (genMipMaps)
		maxMipLevel = glm::floor(glm::log2<float>(glm::max(width, height)));
	else
		maxMipLevel = 0;

	const auto r = Engine::renderer;

	r->createStagingBuffer(textureSize);
	r->copyToStagingBuffer(&(pImage->data[0]), (size_t)textureSize);

	r->createImage(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkImage, vkMemory, maxMipLevel);
	r->transitionImageLayout(vkImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, maxMipLevel);

	r->copyBufferToImage(r->vkStagingBuffer, vkImage, u32(pImage->width), u32(pImage->height), 0);
	r->destroyStagingBuffer();

	if (genMipMaps)
	{
		Image mip[2];
		int currentMip = 0;
		int previousMip = 0;
		pImage->generateMipMap(mip[currentMip]);

		textureSize = mip[currentMip].data.size() * sizeof(Pixel);
		r->createStagingBuffer(textureSize);
		r->copyToStagingBuffer(&(mip[currentMip].data[0]), textureSize, 0);

		r->copyBufferToImage(r->vkStagingBuffer, vkImage, u32(mip[currentMip].width), u32(mip[currentMip].height), 1);
		r->destroyStagingBuffer();

		currentMip = 1;
		for (int i = 2; i < maxMipLevel; ++i)
		{
			mip[previousMip].generateMipMap(mip[currentMip]);

			textureSize = mip[currentMip].data.size() * sizeof(Pixel);

			r->createStagingBuffer(textureSize);
			r->copyToStagingBuffer(&(mip[currentMip].data[0]), textureSize, 0);

			r->copyBufferToImage(r->vkStagingBuffer, vkImage, u32(mip[currentMip].width), u32(mip[currentMip].height), i);
			r->destroyStagingBuffer();

			std::swap(previousMip, currentMip);
		}
	}
	r->transitionImageLayout(vkImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, maxMipLevel);

	vkImageView = r->createImageView(vkImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, maxMipLevel);
}

void Texture::destroy()
{
	const auto r = Engine::renderer;
	DBG_INFO("Destroying: " << name);
	vkDestroyImageView(r->vkDevice, vkImageView, nullptr);
	vkDestroyImage(r->vkDevice, vkImage, nullptr);
	vkFreeMemory(r->vkDevice, vkMemory, nullptr);
}
