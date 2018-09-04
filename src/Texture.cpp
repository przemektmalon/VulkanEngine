#include "PCH.hpp"
#include "Texture.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

Texture::Texture() : vdu::Texture() {}

void Texture::loadToRAM(void * pCreateStruct, AllocFunc alloc)
{
	availability |= LOADING_TO_RAM;

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
				availability &= ~LOADING_TO_RAM;
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
				availability &= ~LOADING_TO_RAM;
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
			ramPointer = img->data.data();
		}
	}

	availability |= ON_RAM;
	availability &= ~LOADING_TO_RAM;
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

		auto cmd = r->beginSingleTimeCommands();

		vdu::Buffer* stagingBuffers = new vdu::Buffer[m_layers];
		vdu::Fence* fe = new vdu::Fence;
		fe->create(&r->logicalDevice);

		for (int i = 0; i < m_layers; ++i)
		{
			VkDeviceSize layerSize = size / m_layers;
			stagingBuffers[i].createStaging(&r->logicalDevice, layerSize);
			memcpy(stagingBuffers[i].getMemory()->map(), img[i].data.data(), (size_t)layerSize);
			stagingBuffers[i].getMemory()->unmap();
			stagingBuffers[i].cmdCopyTo(cmd, this, 0, 0, i, 1);
		}

		r->endSingleTimeCommands(cmd,fe->getHandle());

		auto delayedBufferDestructionFunc = std::bind([](vdu::Fence* fe, vdu::Buffer* buff, u32 layers) -> void {
			for (u32 i = 0; i < layers; ++i) {
				buff[i].destroy();
			}
			delete[] buff;
			fe->destroy();
			delete fe;
		}, fe, stagingBuffers, m_layers);

		r->addFenceDelayedAction(fe, delayedBufferDestructionFunc);

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
			vdu::Buffer stagingBuffer;
			
			auto cmd = r->beginSingleTimeCommands();

			stagingBuffer.createStaging(&r->logicalDevice, size);
			memcpy(stagingBuffer.getMemory()->map(), ci->pData, (size_t)size);
			stagingBuffer.getMemory()->unmap();
			m_usageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			r->transitionImageLayout(m_image, m_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_numMipLevels, m_layers, m_aspectFlags);
			stagingBuffer.cmdCopyTo(cmd, this);

			r->endSingleTimeCommands(cmd);

			//stagingBuffer.destroy();

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

	availability |= ON_GPU;
	availability &= ~LOADING_TO_GPU;

	Engine::renderer->gBufferDescriptorSetNeedsUpdate = true;
}

void Texture::cleanupRAM(FreeFunc fr)
{
}

void Texture::cleanupGPU()
{
}