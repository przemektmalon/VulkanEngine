#include "PCH.hpp"
#include "Texture.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

void Texture::loadFile(std::string pPath, bool genMipMaps)
{
	img = new Image;
	img->load(pPath);
	if (img->data.size() == 0)
	{
		DBG_WARNING("Failed to load image: " << pPath);
		/// TODO: Load a default null texture
		return;
	}
	loadImage(img, genMipMaps);
}

void Texture::loadImage(Image * pImage, bool genMipMaps)
{
	VkDeviceSize textureSize = pImage->data.size() * sizeof(Pixel);
	vkAspect = VK_IMAGE_ASPECT_COLOR_BIT;
	width = pImage->width; height = pImage->height;

	if (genMipMaps)
		maxMipLevel = glm::floor(glm::log2<float>(glm::max(width, height)));
	else
		maxMipLevel = 0;

	const auto r = Engine::renderer;

	r->createStagingBuffer(textureSize);
	r->copyToStagingBuffer(&(pImage->data[0]), (size_t)textureSize);

	r->createImage(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkImage, vkMemory, maxMipLevel + 1);
	r->transitionImageLayout(vkImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, maxMipLevel + 1);

	r->copyBufferToImage(r->stagingBuffer.getBuffer(), vkImage, u32(pImage->width), u32(pImage->height), 0);
	r->destroyStagingBuffer();

	if (genMipMaps)
		generateMipMaps();
	else
		r->transitionImageLayout(vkImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, maxMipLevel + 1);

	vkImageView = r->createImageView(vkImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, maxMipLevel + 1);
}

void Texture::loadCube(std::string pPaths[6], bool genMipMaps)
{
	//Image img[6];
	img = new Image[6];
	for (int i = 0; i < 6; ++i)
	{
		img[i].load(pPaths[i]);
		if (img[i].data.size() == 0)
		{
			DBG_WARNING("Failed to load image: " << pPaths[i]);
			return;
		}
	}

	int w = img[0].width, h = img[0].height;
	for (int i = 1; i < 6; ++i)
	{
		if (img[i].width != w || img[i].height != h)
		{
			DBG_WARNING("Cube texture layer dimensions must be equal");
			return;
		}
		w = img[i].width; h = img[i].height;
	}

	width = img[0].width; height = img[0].height;
	vkAspect = VK_IMAGE_ASPECT_COLOR_BIT;

	if (genMipMaps)
		maxMipLevel = glm::floor(glm::log2<float>(glm::max(width, height)));
	else
		maxMipLevel = 0;

	const auto r = Engine::renderer;

	
	r->createCubeImage(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkImage, vkMemory, maxMipLevel + 1);
	r->transitionImageLayout(vkImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, maxMipLevel + 1, numLayers);

	for (int l = 0; l < 6; ++l)
	{
		VkDeviceSize textureSize = img[0].data.size() * sizeof(Pixel);
		r->createStagingBuffer(textureSize);
		r->copyToStagingBuffer(&(img[l].data[0]), (size_t)textureSize);

		r->copyBufferToImage(r->stagingBuffer.getBuffer(), vkImage, width, height, 0, l, 1);

		r->destroyStagingBuffer();
	}

	if (genMipMaps)
		generateMipMaps();

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = vkImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	viewInfo.subresourceRange.aspectMask = vkAspect;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = maxMipLevel;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 6;

	VK_CHECK_RESULT(vkCreateImageView(r->device, &viewInfo, nullptr, &vkImageView));
}

void Texture::loadStream(TextureCreateInfo * ci)
{
	width = ci->width; height = ci->height;
	vkFormat = ci->format;
	vkLayout = ci->layout;
	vkAspect = ci->aspectFlags;
	vkUsage = ci->usageFlags;
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
		r->createImage(width, height, vkFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | vkUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkImage, vkMemory, maxMipLevel + 1);
		r->transitionImageLayout(vkImage, vkFormat, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, maxMipLevel + 1);
		r->copyBufferToImage(r->stagingBuffer.getBuffer(), vkImage, u32(width), u32(height), 0);
		r->destroyStagingBuffer();
	}
	else
	{
		r->createImage(width, height, vkFormat, VK_IMAGE_TILING_OPTIMAL, vkUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkImage, vkMemory, maxMipLevel + 1);
		r->transitionImageLayout(vkImage, vkFormat, VK_IMAGE_LAYOUT_PREINITIALIZED, vkLayout, maxMipLevel + 1);
	}

	if (vkLayout)
		if (ci->pData)
			r->transitionImageLayout(vkImage, vkFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vkLayout, maxMipLevel + 1);

	vkImageView = r->createImageView(vkImage, vkFormat, vkAspect, maxMipLevel + 1);
}

void Texture::create(TextureCreateInfo * ci)
{
	numLayers = ci->numLayers;
	vkLayout = ci->layout;
	if (ci->pData)
		loadStream(ci);
	else if (ci->pPaths)
	{
		if (ci->numLayers == 1)
			loadFile(ci->pPaths[0], ci->genMipMaps);
		else if (ci->numLayers == 6)
		{
			std::string cubePaths[6];
			for (int i = 0; i < 6; ++i)
				cubePaths[i] = ci->pPaths[i];
			loadCube(cubePaths, ci->genMipMaps);
		}
		else
		{
			DBG_WARNING("Only 1 or 6 layer textures supported");
		}
	}
	else
		loadStream(ci);
}

void Texture::generateMipMaps()
{
	std::vector<std::vector<Image>> mips;
	mips.resize(numLayers);
	for (auto& layer : mips)
		layer.resize(maxMipLevel);

	const auto r = Engine::renderer;

	VkDeviceSize allMipsSize = (img[0].data.size() * 1.335) * numLayers * sizeof(Pixel);
	r->createStagingBuffer(allMipsSize);

	VkDeviceSize offset = 0;

	std::vector<VkBufferImageCopy> copyRegions;

	int lay = 0;
	for (auto& layer : mips)
	{
		int lvl = 1;
		Image* prevMip = img + lay;
		for (auto& level : layer)
		{
			prevMip->generateMipMap(level);
			r->copyToStagingBuffer(&level.data[0], level.data.size() * sizeof(Pixel), offset);
			
			VkBufferImageCopy region = {};
			region.imageSubresource.aspectMask = vkAspect;
			region.imageSubresource.mipLevel = lvl;
			region.imageSubresource.baseArrayLayer = lay;
			region.imageSubresource.layerCount = 1;
			region.imageExtent.width = level.width;
			region.imageExtent.height = level.height;
			region.imageExtent.depth = 1;
			region.bufferOffset = offset;

			copyRegions.push_back(region);

			offset += level.data.size() * sizeof(Pixel);
			prevMip = &level;
			++lvl;
		}
		++lay;
	}

	VkCommandBuffer commandBuffer = r->beginSingleTimeCommands();

	vkCmdCopyBufferToImage(commandBuffer, r->stagingBuffer.getBuffer(), vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copyRegions.size(), copyRegions.data());

	r->setImageLayout(commandBuffer, *this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vkLayout, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	r->endSingleTimeCommands(commandBuffer);

	r->destroyStagingBuffer();
}

void Texture::destroy()
{
	const auto r = Engine::renderer;
	if (name.length() == 0) {
		DBG_INFO("Destroying unnamed texture");
	}
	else {
		DBG_INFO("Destroying texture: " << name);
	}
	VK_VALIDATE(vkDestroyImageView(r->device, vkImageView, nullptr));
	VK_VALIDATE(vkDestroyImage(r->device, vkImage, nullptr));
	VK_VALIDATE(vkFreeMemory(r->device, vkMemory, nullptr));
}
