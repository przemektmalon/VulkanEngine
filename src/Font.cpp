#include "PCH.hpp"
#include "Font.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

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
			glyphContainers.erase(pCharSize);
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
	auto ins = glyphContainers.insert(std::make_pair(pCharSize, GlyphContainer(pCharSize, ftFace)));
	return &ins.first->second;
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

		chars[i].data.resize(sizeAligned);
		memcpy(chars[i].data.data(), pFace->glyph->bitmap.buffer, size);

		totalGlyphSetTexSize += chars[i].data.size();

		xOffset += sizes[i].x;
	}

	maxYSize += lineYSize;

	TextureCreateInfo ci = {};
	ci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	ci.bpp = 8;
	ci.components = 1;
	ci.format = VK_FORMAT_R8_UNORM;
	ci.genMipMaps = false;
	ci.height = maxYSize;
	ci.width = maxXSize;
	ci.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	ci.numLayers = 1;
	ci.usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	glyphs = new Texture;
	glyphs->loadToGPU(&ci);

	const auto& r = Engine::renderer;

	r->createStagingBuffer(totalGlyphSetTexSize);

	VkDeviceSize copyOffset = 0;
	void* data = r->stagingBuffer.map(0, totalGlyphSetTexSize);
	for (int i = 1; i < NO_PRINTABLE_CHARS; ++i)
	{
		char* dst = (char*)data; dst += copyOffset;
		memcpy(dst, chars[i].data.data(), (size_t)chars[i].data.size());
		copyOffset += chars[i].data.size();
	}

	r->stagingBuffer.unmap();

	copyOffset = 0;
	for (int i = 1; i < NO_PRINTABLE_CHARS; ++i)
	{
		VkOffset3D offset = { coords[i].x, coords[i].y, 0 };
		VkExtent3D extent = { sizes[i].x, sizes[i].y, 1 };
		r->stagingBuffer.copyTo(glyphs, copyOffset, 0, 0, 1, offset, extent);
		copyOffset += chars[i].data.size();
	}

	r->destroyStagingBuffer();

	height = pFace->size->metrics.height >> 6;
	ascender = pFace->size->metrics.ascender >> 6;
}
