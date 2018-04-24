#include "PCH.hpp"
#include "Texture.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

void Texture::loadToRAM(void * pCreateStruct, AllocFunc alloc)
{
	auto ci = (TextureCreateInfo*)pCreateStruct;

	mipped = ci->genMipMaps;
	numLayers = ci->numLayers;

	vkLayout = ci->layout;
	vkFormat = ci->format;
	vkAspect = ci->aspectFlags;
	vkUsage = ci->usageFlags;
	components = ci->components;

	if (isAvailable(ON_DISK))
	{
		// Asset has been prepared by asset store
		// Name and size filled in

		img = new Image[numLayers];

		for (int i = 0; i < numLayers; ++i)
		{
			img[i].load(diskPaths[i], components);
			if (img[i].data.size() == 0)
			{
				DBG_WARNING("Failed to load image: " << diskPaths[i]);
				return;
			}
			if (i > 0 && img->components != components && img->bpp != components)
				DBG_WARNING("Components of all cube images must be equal");
			components = img->components;
			bpp = img->bpp;
		}

		int w = img[0].width, h = img[0].height;
		for (int i = 1; i < numLayers; ++i)
		{
			if (img[i].width != w || img[i].height != h)
			{
				DBG_WARNING("Cube texture layer dimensions must be equal");
				return;
			}
			w = img[i].width; h = img[i].height;
		}

		switch (img->components) {
		case 4:
			vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
			break;
		case 3:
			vkFormat = VK_FORMAT_R8G8B8_UNORM;
			break;
		case 1:
			vkFormat = VK_FORMAT_R8_UNORM;
			break;
		}

		width = img->width;
		height = img->height;
		size = img->data.size() * numLayers;
		vkAspect = VK_IMAGE_ASPECT_COLOR_BIT;

		availability |= ON_RAM;
		ramPointer = img->data.data();
	}
	else
	{
		// Asset is being create from stream by the app
		// The create struct should contain a name a other data to calculate size etc

		name = ci->name;
		width = ci->width;
		height = ci->height;
		components = ci->components;
		bpp = ci->bpp;
		size = width * height * components * numLayers;
		
		if (ci->pData)
		{
			img = new Image;
			img->setSize(width, height, components);
			memcpy(img->data.data(), ci->pData, size);

			availability |= ON_RAM;
			ramPointer = img->data.data();
		}
	}
}

void Texture::loadToGPU(void * pCreateStruct)
{
	auto ci = (TextureCreateInfo*)pCreateStruct;

	if (ci)
	{
		vkLayout = ci->layout;
		vkFormat = ci->format;
		vkAspect = ci->aspectFlags;
		vkUsage = ci->usageFlags;
		mipped = ci->genMipMaps;
	}

	const auto r = Engine::renderer;

	if (isAvailable(ON_RAM))
	{
		// Asset in RAM, check if create struct is non-zero

		if (mipped)
			maxMipLevel = glm::floor(glm::log2<float>(glm::max(width, height)));
		else
			maxMipLevel = 0;

		vkUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		createImage();
		r->transitionImageLayout(vkImage, vkFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, maxMipLevel + 1, numLayers, vkAspect);

		for (int i = 0; i < numLayers; ++i)
		{
			VkDeviceSize layerSize = size / numLayers;
			r->createStagingBuffer(layerSize);
			r->copyToStagingBuffer(img[i].data.data(), (size_t)layerSize);
			r->stagingBuffer.copyTo(this, 0, 0, i, 1);
			r->destroyStagingBuffer();
		}

		if (mipped)
			generateMipMaps();
		else
			r->transitionImageLayout(vkImage, vkFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vkLayout, maxMipLevel + 1, numLayers, vkAspect);
	}
	else
	{
		// Asset not in RAM, check if we have data to upload to GPU
		// We need to fill in meta data here

		if (!ci)
		{
			DBG_SEVERE("Attempting to load texture to GPU with no create info when it's required");
		}

		name = ci->name;
		width = ci->width;
		height = ci->height;
		components = ci->components;
		bpp = ci->bpp;
		size = width * height * bpp;
		numLayers = ci->numLayers;

		if (mipped)
			maxMipLevel = glm::floor(glm::log2<float>(glm::max(width, height)));
		else
			maxMipLevel = 0;

		if (ci->pData)
		{
			r->createStagingBuffer(size);
			r->copyToStagingBuffer(ci->pData, (size_t)size);
			vkUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			createImage();
			r->transitionImageLayout(vkImage, vkFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, maxMipLevel + 1, numLayers, vkAspect);
			r->stagingBuffer.copyTo(this, 0, 0, 0, 1);
			r->destroyStagingBuffer();

			if (mipped)
				generateMipMaps();
			else
				r->transitionImageLayout(vkImage, vkFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vkLayout, maxMipLevel + 1, numLayers, vkAspect);
		}
		else
		{
			createImage();
			r->transitionImageLayout(vkImage, vkFormat, VK_IMAGE_LAYOUT_UNDEFINED, vkLayout, maxMipLevel + 1, numLayers, vkAspect);
		}
	}

	createImageView();
}

void Texture::generateMipMaps()
{
	std::vector<std::vector<Image>> mips;
	mips.resize(numLayers);
	for (auto& layer : mips)
		layer.resize(maxMipLevel);

	const auto r = Engine::renderer;

	VkDeviceSize allMipsSize = (img[0].data.size() * 1.335) * numLayers;
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
			r->copyToStagingBuffer(&level.data[0], level.data.size(), offset);
			
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

			offset += level.data.size();
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

void Texture::createImage()
{
	createImage(width, height, vkFormat, VK_IMAGE_TILING_OPTIMAL, vkUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkImage, vkMemory, maxMipLevel + 1, numLayers);
}

void Texture::createImageView()
{
	vkImageView = createImageView(vkImage, vkFormat, vkAspect, maxMipLevel + 1, numLayers);
}

VkImageView Texture::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, int mipLevels, int numLayers)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = numLayers;
	if (numLayers == 6)
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	else
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

	VkImageView ret;

	VK_CHECK_RESULT(vkCreateImageView(Engine::renderer->device, &viewInfo, nullptr, &ret));

	return ret;
}

void Texture::createImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage & image, VkDeviceMemory & imageMemory, int mipLevels, int numLayers)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = numLayers;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (numLayers == 6)
		imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	auto const r = Engine::renderer;

	VK_CHECK_RESULT(vkCreateImage(r->device, &imageInfo, nullptr, &image));

	VkMemoryRequirements memRequirements;
	VK_VALIDATE(vkGetImageMemoryRequirements(r->device, image, &memRequirements));

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = Engine::getPhysicalDeviceDetails().getMemoryType(memRequirements.memoryTypeBits, properties);

	VK_CHECK_RESULT(vkAllocateMemory(r->device, &allocInfo, nullptr, &imageMemory));

	VK_CHECK_RESULT(vkBindImageMemory(r->device, image, imageMemory, 0));
}
