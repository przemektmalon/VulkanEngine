#include "PCH.hpp"
#include "Font.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "Threading.hpp"

GlyphContainer* Font::requestGlyphs(u16 pCharSize, Text * pUser)
{
	auto find = useTrack.find(pCharSize);
	if (find == useTrack.end())
	{
		auto ret = loadGlyphs(pCharSize);
		useTrack.insert(std::make_pair(pCharSize, std::set<Text*>({ pUser })));
		return ret;
	}
	else
	{
		if (find->second.size() == 0)
		{
			auto ret = loadGlyphs(pCharSize);
			useTrack.insert(std::make_pair(pCharSize, std::set<Text*>({ pUser })));
			return ret;
		}
		find->second.insert(pUser);
		return &glyphContainers.at(pCharSize);
	}
}

void Font::releaseGlyphs(u16 pCharSize, Text* pUser)
{
	auto find = useTrack.find(pCharSize);
	if (find == useTrack.end())
	{

	}
	else
	{
		find->second.erase(pUser);
		if (find->second.size() == 0)
		{
			auto find = glyphContainers.find(pCharSize);
			find->second.release();
			glyphContainers.erase(find);
		}
	}
}

void Font::loadToRAM(void * pCreateStruct, AllocFunc alloc)
{
	if (ftFace == nullptr)
	{
		auto err = FT_New_Face(Engine::assets.freetype, diskPaths[0].c_str(), 0, &ftFace);
		if (err)
			assert(false);
	}
}

void Font::cleanupGPU()
{
	for (auto& g : glyphContainers)
		g.second.release();
}

GlyphContainer * Font::loadGlyphs(u16 pCharSize)
{
	auto findGlyphs = glyphContainers.find(pCharSize);
	if (findGlyphs == glyphContainers.end()) {
		DBG_INFO("Creating new glyphs for: " << name << ", size " << (int)pCharSize);
		auto ins = glyphContainers.insert(std::make_pair(pCharSize, GlyphContainer(pCharSize, ftFace)));
		return &ins.first->second;
	}
	else {
		return &findGlyphs->second;
	}
	
}

void GlyphContainer::load(u16 pCharSize, FT_Face pFace)
{
	charSize = pCharSize;

	if (pFace == nullptr)
	{
		assert(false);
	}

	FT_Set_Pixel_Sizes(pFace, 0, charSize);

	int maxXSize = 0;
	int maxYSize = 0;
	int lineXSize = 0;
	int lineYSize = 0;

	int xOffset = 0;
	int yOffset = 0;

	Image chars[NO_PRINTABLE_CHARS];

	const int storeResolutionX = 12;

	VkDeviceSize totalGlyphSetTexSize = 0;

	for (char i = 0; i < NO_PRINTABLE_CHARS; ++i)
	{
		int x = i % storeResolutionX;
		int y = floor(i / storeResolutionX);

		if (x == 0)
		{
			xOffset = 0;
			yOffset += lineYSize;
			maxYSize += lineYSize;
			maxXSize = lineXSize > maxXSize ? lineXSize : maxXSize;
			lineXSize = 0;
			lineYSize = 0;
		}

		char ascSymbol = i + (unsigned char)(32);

		FT_Error err = FT_Load_Char(pFace, ascSymbol, FT_LOAD_RENDER);

		if (err)
			assert(0);

		lineXSize += pFace->glyph->bitmap.width;
		lineYSize = pFace->glyph->bitmap.rows > lineYSize ? pFace->glyph->bitmap.rows : lineYSize;

		sizes[i].x = pFace->glyph->bitmap.width;
		sizes[i].y = pFace->glyph->bitmap.rows;

		positions[i].x = pFace->glyph->bitmap_left;
		positions[i].y = pFace->glyph->bitmap_top;

		advances[i].x = pFace->glyph->advance.x >> 6;
		advances[i].y = pFace->glyph->advance.y >> 6;

		coords[i].x = xOffset;
		coords[i].y = yOffset;

		int size = (sizes[i].x * sizes[i].y);
		int sizeAligned = size + 4 - (size % 4);

		chars[i].m_data.resize(sizeAligned);
		memcpy(chars[i].m_data.data(), pFace->glyph->bitmap.buffer, size);

		totalGlyphSetTexSize += chars[i].m_data.size();

		xOffset += sizes[i].x;
	}

	maxYSize += lineYSize;

	TextureCreateInfo ci = {};
	ci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	ci.format = VK_FORMAT_R8_UNORM;
	ci.genMipMaps = false;
	ci.height = maxYSize;
	ci.width = maxXSize;
	ci.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	ci.layers = 1;
	ci.usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	glyphs = new Texture;

	auto loadGlyphsFunc = [&ci, this]() -> void {
		glyphs->loadToGPU(&ci);
	};

	Engine::threading->addGPUJob(new Job<>(loadGlyphsFunc));

	/// TODO: provide mechanism for checking whether specific job finished (+ wait until finish)
	Engine::threading->m_gpuWorker->waitForAllJobsToFinish();

	const auto& r = Engine::renderer;

	auto stagingBuffer = new vdu::Buffer;
	stagingBuffer->createStaging(&r->logicalDevice, totalGlyphSetTexSize);

	VkDeviceSize copyOffset = 0;
	void* data = stagingBuffer->getMemory()->map(0, totalGlyphSetTexSize);
	for (int i = 1; i < NO_PRINTABLE_CHARS; ++i)
	{
		char* dst = (char*)data; dst += copyOffset;
		memcpy(dst, chars[i].m_data.data(), (size_t)chars[i].m_data.size());
		copyOffset += chars[i].m_data.size();
	}

	stagingBuffer->getMemory()->unmap();

	auto cmd = new vdu::CommandBuffer;
	r->beginTransferCommands(*cmd);

	copyOffset = 0;
	for (int i = 1; i < NO_PRINTABLE_CHARS; ++i)
	{
		VkOffset3D offset = { coords[i].x, coords[i].y, 0 };
		VkExtent3D extent = { sizes[i].x, sizes[i].y, 1 };

		VkBufferImageCopy region = {};
		region.bufferOffset = copyOffset;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = offset;
		region.imageExtent = extent;

		vkCmdCopyBufferToImage(cmd->getHandle(), stagingBuffer->getHandle(), glyphs->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		copyOffset += chars[i].m_data.size();
	}

	auto fence = new vdu::Fence();
	fence->create(&r->logicalDevice);

	/// TODO: LAYOUT_GENERAL not necessary, need to properly handle all image layouts and transitions throughout Engine
	r->setImageLayout(cmd, *glyphs, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	vdu::QueueSubmission submission;
	r->endTransferCommands(*cmd, submission);

	VK_CHECK_RESULT(r->lTransferQueue.submit(submission, *fence));

	// Once the all GPU operations are done, the fence is signalled, and we can free the command and data buffers
	r->addFenceDelayedAction(fence, std::bind([](vdu::CommandBuffer* cmd, vdu::Buffer* stagingBuffer, vdu::Fence* fe) -> void {
		cmd->free();
		stagingBuffer->destroy();
		fe->destroy();
		delete cmd;
		delete stagingBuffer;
		delete fe;
	}, cmd, stagingBuffer, fence));

	height = pFace->size->metrics.height >> 6;
	ascender = pFace->size->metrics.ascender >> 6;
}

