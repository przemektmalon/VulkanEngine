#include "PCH.hpp"
#include "Texture.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

void Texture::loadToRAM(void * pCreateStruct, AllocFunc alloc)
{
	auto ci = (TextureCreateInfo*)pCreateStruct;

	isMipped = ci->genMipMaps;

	setProperties(*ci);

	int components = getNumComponents();

	if (checkAvailability(ON_DISK))
	{
		// Asset has been prepared by asset store
		// Name and size filled in

		img = new Image[m_layers];

		for (int i = 0; i < m_layers; ++i)
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
		}

		int w = img[0].width, h = img[0].height;
		for (int i = 1; i < m_layers; ++i)
		{
			if (img[i].width != w || img[i].height != h)
			{
				DBG_WARNING("Cube texture layer dimensions must be equal");
				return;
			}
			w = img[i].width; h = img[i].height;
		}

		if (getNumComponents() != components || getNumComponents() == 0)
		{
			DBG_SEVERE("Texture format component mismatch");
		}

		m_width = img->width;
		m_height = img->height;
		size = img->data.size() * m_layers;

		availability |= ON_RAM;
		ramPointer = img->data.data();
	}
	else
	{
		// Asset is being create from stream by the app
		// The create struct should contain a name and other data to calculate size etc

		name = ci->name;
		size = m_width * m_height * components * m_layers;
		
		if (ci->pData)
		{
			img = new Image;
			img->setSize(m_width, m_height, components);
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
		setProperties(*ci);
		isMipped = ci->genMipMaps;
	}

	const auto r = Engine::renderer;

	if (checkAvailability(ON_RAM))
	{
		// Asset in RAM, check if create struct is non-zero

		if (isMipped)
			m_maxMipLevel = glm::floor(glm::log2<float>(glm::max(m_width, m_height)));
		else
			m_maxMipLevel = 0;

		m_numMipLevels = m_maxMipLevel + 1;

		m_usageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		create(&r->logicalDevice);
		r->transitionImageLayout(m_image, m_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_numMipLevels, m_layers, m_aspectFlags);

		for (int i = 0; i < m_layers; ++i)
		{
			VkDeviceSize layerSize = size / m_layers;
			Buffer stagingBuffer;
			r->createStagingBuffer(stagingBuffer, layerSize);
			r->copyToStagingBuffer(stagingBuffer, img[i].data.data(), (size_t)layerSize);
			stagingBuffer.copyTo(this, 0, 0, i, 1);
			r->destroyStagingBuffer(stagingBuffer);
		}

		if (isMipped)
		{
			auto cmd = r->beginSingleTimeCommands();
			cmdGenerateMipMaps(cmd);
			r->endSingleTimeCommands(cmd);
		}
		else
		{
			r->transitionImageLayout(m_image, m_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_layout, m_numMipLevels, m_layers, m_aspectFlags);
		}
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
		size = m_width * m_height * getBytesPerPixel();

		if (isMipped)
			m_maxMipLevel = glm::floor(glm::log2<float>(glm::max(m_width, m_height)));
		else
			m_maxMipLevel = 0;

		m_numMipLevels = m_maxMipLevel + 1;

		if (ci->pData)
			m_usageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		create(&r->logicalDevice);

		if (ci->pData)
		{
			Buffer stagingBuffer;
			r->createStagingBuffer(stagingBuffer, size);
			r->copyToStagingBuffer(stagingBuffer, ci->pData, (size_t)size);
			m_usageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			r->transitionImageLayout(m_image, m_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_numMipLevels, m_layers, m_aspectFlags);
			stagingBuffer.copyTo(this, 0, 0, 0, 1);
			r->destroyStagingBuffer(stagingBuffer);

			if (isMipped)
			{
				auto cmd = r->beginSingleTimeCommands();
				cmdGenerateMipMaps(cmd);
				r->endSingleTimeCommands(cmd);
			}
			else
			{
				r->transitionImageLayout(m_image, m_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_layout, m_numMipLevels, m_layers, m_aspectFlags);
			}
		}
		else
		{
			r->transitionImageLayout(m_image, m_format, VK_IMAGE_LAYOUT_UNDEFINED, m_layout, m_numMipLevels, m_layers, m_aspectFlags);
		}
	}

	Engine::renderer->gBufferDescriptorSetNeedsUpdate = true;
}

void Texture::cleanupRAM(FreeFunc fr)
{
}

void Texture::cleanupGPU()
{
}

/*void Texture::generateMipMaps()
{
	std::vector<std::vector<Image>> mips;
	mips.resize(m_layers);
	for (auto& layer : mips)
		layer.resize(maxMipLevel);

	const auto r = Engine::renderer;

	VkDeviceSize allMipsSize = (img[0].data.size() * 1.335) * m_layers;
	Buffer stagingBuffer;
	r->createStagingBuffer(stagingBuffer, allMipsSize);

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
			r->copyToStagingBuffer(stagingBuffer, &level.data[0], level.data.size(), offset);
			
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

	vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.getBuffer(), vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copyRegions.size(), copyRegions.data());

	r->setImageLayout(commandBuffer, *this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vkLayout, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	r->endSingleTimeCommands(commandBuffer);

	r->destroyStagingBuffer(stagingBuffer);
}*/