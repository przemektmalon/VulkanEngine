#include "PCH.hpp"
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

	r->copyBufferToImage(r->stagingBuffer, vkImage, u32(pImage->width), u32(pImage->height), 0);
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

		r->copyBufferToImage(r->stagingBuffer, vkImage, u32(mip[currentMip].width), u32(mip[currentMip].height), 1);
		r->destroyStagingBuffer();

		currentMip = 1;
		for (int i = 2; i < maxMipLevel; ++i)
		{
			mip[previousMip].generateMipMap(mip[currentMip]);

			textureSize = mip[currentMip].data.size() * sizeof(Pixel);

			r->createStagingBuffer(textureSize);
			r->copyToStagingBuffer(&(mip[currentMip].data[0]), textureSize, 0);

			r->copyBufferToImage(r->stagingBuffer, vkImage, u32(mip[currentMip].width), u32(mip[currentMip].height), i);
			r->destroyStagingBuffer();

			std::swap(previousMip, currentMip);
		}
	}
	r->transitionImageLayout(vkImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, maxMipLevel);

	vkImageView = r->createImageView(vkImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, maxMipLevel);
}

void Texture::loadStream(TextureCreateInfo * ci)
{
	width = ci->width; height = ci->height;
	vkFormat = ci->format;
	vkLayout = ci->layout;
	VkDeviceSize textureSize = width * height * ci->bpp;

	if (ci->genMipMaps)
		maxMipLevel = glm::floor(glm::log2<float>(glm::max(width, height)));
	else
		maxMipLevel = 0;

	const auto r = Engine::renderer;

	if (ci->pData)
	{
		r->createStagingBuffer(textureSize);
		r->copyToStagingBuffer(ci->pData, (size_t)textureSize);
	}

	if (ci->pData)
	{
		r->createImage(width, height, vkFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | ci->usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkImage, vkMemory, maxMipLevel + 1);
		r->transitionImageLayout(vkImage, vkFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, maxMipLevel + 1);
	}
	else
	{
		r->createImage(width, height, vkFormat, VK_IMAGE_TILING_OPTIMAL, ci->usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkImage, vkMemory, maxMipLevel + 1);
		r->transitionImageLayout(vkImage, vkFormat, VK_IMAGE_LAYOUT_UNDEFINED, ci->layout, maxMipLevel + 1);
	}

	if (ci->pData)
	{
		r->copyBufferToImage(r->stagingBuffer, vkImage, u32(width), u32(height), 0);
		r->destroyStagingBuffer();
	}

	/// TODO: mip maps for stream loaded textures
	/*if (genMipMaps)
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
	}*/
	if (ci->layout)
		if (ci->pData)
			r->transitionImageLayout(vkImage, vkFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ci->layout, maxMipLevel + 1);

	vkImageView = r->createImageView(vkImage, vkFormat, ci->aspectFlags, maxMipLevel + 1);
}

void Texture::destroy()
{
	const auto r = Engine::renderer;
	DBG_INFO("Destroying: " << name);
	vkDestroyImageView(r->device, vkImageView, nullptr);
	vkDestroyImage(r->device, vkImage, nullptr);
	vkFreeMemory(r->device, vkMemory, nullptr);
}
