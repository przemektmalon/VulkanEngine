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

		auto cmd = new vdu::CommandBuffer;
		r->beginTransferCommands(*cmd);
		r->setImageLayout(cmd->getHandle(), *this, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		vdu::Buffer* stagingBuffers = new vdu::Buffer[m_layers];

		for (int i = 0; i < m_layers; ++i)
		{
			VkDeviceSize layerSize = size / m_layers;
			stagingBuffers[i].createStaging(&r->logicalDevice, layerSize);
			memcpy(stagingBuffers[i].getMemory()->map(), img[i].data.data(), (size_t)layerSize);
			stagingBuffers[i].getMemory()->unmap();
			stagingBuffers[i].cmdCopyTo(cmd, this, 0, 0, i, 1);
		}

		if (isMipped)
		{
			cmdGenerateMipMaps(cmd);
		}
		else
		{
			r->setImageLayout(cmd->getHandle(), *this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_layout, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		vdu::QueueSubmission submission;
		r->endTransferCommands(*cmd, submission);

		auto fe = new vdu::Fence(&r->logicalDevice);
		VK_CHECK_RESULT(r->lTransferQueue.submit(submission, *fe));

		auto delayedBufferDestructionFunc = std::bind([](vdu::Fence* fe, vdu::Buffer* buff, u32 layers, vdu::CommandBuffer* cmd) -> void {
			for (u32 i = 0; i < layers; ++i) {
				buff[i].destroy();
			}
			fe->destroy();
			cmd->free();
			delete[] buff;
			delete fe;
			delete cmd;
		}, fe, stagingBuffers, m_layers, cmd);

		r->addFenceDelayedAction(fe, delayedBufferDestructionFunc);
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
			auto cmd = new vdu::CommandBuffer;
			r->beginTransferCommands(*cmd);

			auto stagingBuffer = new vdu::Buffer;
			stagingBuffer->createStaging(&r->logicalDevice, size);
			memcpy(stagingBuffer->getMemory()->map(), ci->pData, (size_t)size);
			stagingBuffer->getMemory()->unmap();
			m_usageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			r->setImageLayout(cmd, *this, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
			stagingBuffer->cmdCopyTo(cmd, this);

			if (isMipped)
			{
				cmdGenerateMipMaps(cmd);
			}
			else
			{
				r->setImageLayout(cmd, *this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_layout, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			}

			vdu::QueueSubmission submission;
			r->endTransferCommands(*cmd, submission);

			auto fe = new vdu::Fence(&r->logicalDevice);
			VK_CHECK_RESULT(r->lTransferQueue.submit(submission, *fe));

			auto delayedBufferDestructionFunc = std::bind([](vdu::Fence* fe, vdu::Buffer* buff, vdu::CommandBuffer* cmd) -> void {
				buff->destroy();
				fe->destroy();
				cmd->free();
				delete buff;
				delete fe;
				delete cmd;
			}, fe, stagingBuffer, cmd);

			r->addFenceDelayedAction(fe, delayedBufferDestructionFunc);
		}
		else
		{
			auto cmd = new vdu::CommandBuffer;
			r->beginTransferCommands(*cmd);

			VkPipelineStageFlagBits dstStage;

			switch (m_layout)
			{
			case VK_IMAGE_LAYOUT_GENERAL:
				dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; 
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; 
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				break;

			default:
				dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				break;
			}

			r->setImageLayout(cmd, *this, VK_IMAGE_LAYOUT_UNDEFINED, m_layout, VK_PIPELINE_STAGE_TRANSFER_BIT, dstStage);

			vdu::QueueSubmission submission;
			r->endTransferCommands(*cmd, submission);
			auto fe = new vdu::Fence(&r->logicalDevice);
			VK_CHECK_RESULT(r->lTransferQueue.submit(submission, *fe));

			auto delayedBufferDestructionFunc = std::bind([](vdu::Fence* fe, vdu::CommandBuffer* cmd) -> void {
				cmd->free();
				fe->destroy();
				delete cmd;
				delete fe;
			}, fe, cmd);

			r->addFenceDelayedAction(fe, delayedBufferDestructionFunc);
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