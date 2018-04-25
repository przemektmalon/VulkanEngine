#pragma once
#include "PCH.hpp"
#include "Texture.hpp"
#include "ft2build.h"

#include FT_FREETYPE_H

#define NO_PRINTABLE_CHARS 95

class GlyphContainer
{
public:
	GlyphContainer() { }
	GlyphContainer(u16 pCharSize, FT_Face pFace) { load(pCharSize, pFace); }
	~GlyphContainer() { }

	Texture* getTexture() { return glyphs; }

	glm::ivec2 getSize(unsigned char pAscSymbol)
	{
		return sizes[pAscSymbol - 32];
	}

	u16 getHeight()
	{
		return height;
	}

	u16 getAscender()
	{
		return ascender;
	}

	glm::ivec2 getPosition(unsigned char pAscSymbol)
	{
		return positions[pAscSymbol - 32];
	}

	glm::ivec2 getAdvance(unsigned char pAscSymbol)
	{
		return advances[pAscSymbol - 32];
	}

	glm::fvec2 getCoords(u8 pAscSymbol)
	{
		return glm::fvec2(coords[pAscSymbol - 32]) / glm::fvec2(glyphs->getWidth(), glyphs->getHeight());
	}

	void load(u16 pCharSize, FT_Face pFace);

	void release() { if (glyphs) glyphs->destroy(); }

private:

	u16 charSize;
	u16 height;
	u16 ascender;
	Texture* glyphs;
	glm::ivec2 sizes[NO_PRINTABLE_CHARS];
	glm::ivec2 positions[NO_PRINTABLE_CHARS];
	glm::ivec2 advances[NO_PRINTABLE_CHARS];
	glm::ivec2 coords[NO_PRINTABLE_CHARS];

};

class Text;

class Font : public Asset
{
	friend class AssetManager;
public:
	Font() : ftFace(0) {}
	~Font() {}

	GlyphContainer* requestGlyphs(u16 pCharSize, Text* pUser);
	void releaseGlyphs(u16 pCharSize, Text* pUser);

	void loadToRAM(void* pCreateStruct = 0, AllocFunc alloc = malloc);
	void cleanupGPU();

private:

	GlyphContainer * loadGlyphs(u16 pCharSize);

	FT_Face ftFace;
	std::map<u16, std::set<Text*>> useTrack;
	std::map<u16, GlyphContainer> glyphContainers;
	GlyphContainer gl;
};
