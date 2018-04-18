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

	r->copyBufferToImage(r->stagingBuffer.getBuffer(), vkImage, u32(pImage->width), u32(pImage->height), 0);
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

		r->copyBufferToImage(r->stagingBuffer.getBuffer(), vkImage, u32(mip[currentMip].width), u32(mip[currentMip].height), 1);
		r->destroyStagingBuffer();

		currentMip = 1;
		for (int i = 2; i < maxMipLevel; ++i)
		{
			mip[previousMip].generateMipMap(mip[currentMip]);

			textureSize = mip[currentMip].data.size() * sizeof(Pixel);

			r->createStagingBuffer(textureSize);
			r->copyToStagingBuffer(&(mip[currentMip].data[0]), textureSize, 0);

			r->copyBufferToImage(r->stagingBuffer.getBuffer(), vkImage, u32(mip[currentMip].width), u32(mip[currentMip].height), i);
			r->destroyStagingBuffer();

			std::swap(previousMip, currentMip);
		}
	}
	r->transitionImageLayout(vkImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, maxMipLevel);

	vkImageView = r->createImageView(vkImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, maxMipLevel);
}

void Texture::loadCube(std::string pPaths[6], bool genMipMaps)
{
	Image img[6];
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

	if (genMipMaps)
		maxMipLevel = glm::floor(glm::log2<float>(glm::max(width, height)));
	else
		maxMipLevel = 0;

	const auto r = Engine::renderer;

	
	r->createCubeImage(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkImage, vkMemory, maxMipLevel);

	for (int l = 0; l < 6; ++l)
	{
		VkDeviceSize textureSize = img[0].data.size() * sizeof(Pixel);
		r->createStagingBuffer(textureSize);
		r->copyToStagingBuffer(&(img[l].data[0]), (size_t)textureSize);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = vkImage;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = maxMipLevel;
		barrier.subresourceRange.baseArrayLayer = l;
		barrier.subresourceRange.layerCount = 1;

		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		VkCommandBuffer commandBuffer = r->beginSingleTimeCommands();

		VK_VALIDATE(vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier));

		r->endSingleTimeCommands(commandBuffer);

		commandBuffer = r->beginSingleTimeCommands();

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = l;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			(u32)width,
			(u32)height,
			(u32)1
		};

		VK_VALIDATE(vkCmdCopyBufferToImage(commandBuffer, r->stagingBuffer.getBuffer(), vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));

		r->endSingleTimeCommands(commandBuffer);
		r->destroyStagingBuffer();

		if (genMipMaps)
		{
			Image mip[2];
			int currentMip = 0;
			int previousMip = 0;
			img[l].generateMipMap(mip[currentMip]);

			textureSize = mip[currentMip].data.size() * sizeof(Pixel);
			r->createStagingBuffer(textureSize);
			r->copyToStagingBuffer(&(mip[currentMip].data[0]), textureSize, 0);

			commandBuffer = r->beginSingleTimeCommands();

			VkBufferImageCopy region = {};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 1;
			region.imageSubresource.baseArrayLayer = l;
			region.imageSubresource.layerCount = 1;
			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = {
				(u32)mip[currentMip].width,
				(u32)mip[currentMip].height,
				(u32)1
			};

			VK_VALIDATE(vkCmdCopyBufferToImage(commandBuffer, r->stagingBuffer.getBuffer(), vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));

			r->endSingleTimeCommands(commandBuffer);

			r->destroyStagingBuffer();

			currentMip = 1;
			for (int i = 2; i < maxMipLevel; ++i)
			{
				mip[previousMip].generateMipMap(mip[currentMip]);

				textureSize = mip[currentMip].data.size() * sizeof(Pixel);

				r->createStagingBuffer(textureSize);
				r->copyToStagingBuffer(&(mip[currentMip].data[0]), textureSize, 0);

				commandBuffer = r->beginSingleTimeCommands();

				VkBufferImageCopy region = {};
				region.bufferOffset = 0;
				region.bufferRowLength = 0;
				region.bufferImageHeight = 0;
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.mipLevel = i;
				region.imageSubresource.baseArrayLayer = l;
				region.imageSubresource.layerCount = 1;
				region.imageOffset = { 0, 0, 0 };
				region.imageExtent = {
					(u32)mip[currentMip].width,
					(u32)mip[currentMip].height,
					(u32)1
				};

				VK_VALIDATE(vkCmdCopyBufferToImage(commandBuffer, r->stagingBuffer.getBuffer(), vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));

				r->endSingleTimeCommands(commandBuffer);
				r->destroyStagingBuffer();

				std::swap(previousMip, currentMip);
			}
		}
	}

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = vkImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = maxMipLevel;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 6;

	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	auto commandBuffer = r->beginSingleTimeCommands();

	VK_VALIDATE(vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier));

	r->endSingleTimeCommands(commandBuffer);

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
	}

	if (ci->pData)
	{
		r->createImage(width, height, vkFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | vkUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkImage, vkMemory, maxMipLevel + 1);
		r->transitionImageLayout(vkImage, vkFormat, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, maxMipLevel + 1);
	}
	else
	{
		r->createImage(width, height, vkFormat, VK_IMAGE_TILING_OPTIMAL, vkUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkImage, vkMemory, maxMipLevel + 1);
		r->transitionImageLayout(vkImage, vkFormat, VK_IMAGE_LAYOUT_PREINITIALIZED, vkLayout, maxMipLevel + 1);
	}

	if (ci->pData)
	{
		r->copyBufferToImage(r->stagingBuffer.getBuffer(), vkImage, u32(width), u32(height), 0);
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
	if (vkLayout)
		if (ci->pData)
			r->transitionImageLayout(vkImage, vkFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vkLayout, maxMipLevel + 1);

	vkImageView = r->createImageView(vkImage, vkFormat, vkAspect, maxMipLevel + 1);
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
