#include "PCH.hpp"
#include "Text.hpp"

void Text::Style::setFont(Font * pFont)
{
	font = pFont;
}

void Text::Style::setCharSize(u16 pCharSize)
{
	charSize = pCharSize;
}

void Text::Style::setOrigin(Origin pOrigin)
{
	originPreDef = pOrigin;
}

void Text::Style::setOrigin(glm::ivec2 pOrigin)
{
	origin = pOrigin;
}

void Text::Style::setColour(glm::fvec3 pColour)
{
	colour = pColour;
}

void Text::update()
{
	bounds.zero();
	if (style.charSize == 0) { return; }
	if (string.length() == 0) { return; }
	if (style.font == nullptr) { return; }

	int numVerts = string.length() * 6;
	
	verts.resize(numVerts);

	glyphs = style.font->requestGlyphs(style.charSize, this);
	auto glyphsTex = glyphs->getTexture();

	int index = 0;
	auto p = string.c_str();
	auto tpos = position - glm::fvec2(style.origin);

	glm::ivec2 vertPos(tpos);

	bounds.left = vertPos.x;
	bounds.top = vertPos.y;

	float ascender = glyphs->getAscender();

	glm::ivec2 glyphPos = glyphs->getPosition(*p);
	glm::ivec2 glyphSize = glyphs->getSize(*p);

	glm::fvec2 botLeft(vertPos.x + glyphPos.x, vertPos.y + ascender + (glyphSize.y - glyphPos.y));
	glm::fvec2 topRight(vertPos.x + glyphPos.x + glyphSize.x, vertPos.y + ascender - glyphPos.y);

	s32 minX, maxX, minY, maxY;
	minX = botLeft.x; maxX = topRight.x;
	minY = topRight.y; maxY = botLeft.y;

	while (*p != 0)
	{
		if (*p == '\n')
		{
			vertPos.x = tpos.x;
			vertPos.y += style.charSize;
		}
		else
		{
			glm::ivec2 glyphPos = glyphs->getPosition(*p);
			glm::ivec2 glyphSize = glyphs->getSize(*p);
			glm::fvec2 glyphCoords = glyphs->getCoords(*p);

			auto cOffset = glm::fvec2(glyphSize) / glm::fvec2(glyphsTex->getWidth(), glyphsTex->getHeight());

			glm::fvec2 botLeft(vertPos.x + glyphPos.x, vertPos.y + ascender + (glyphSize.y - glyphPos.y));
			glm::fvec2 botRight(vertPos.x + glyphPos.x + glyphSize.x, vertPos.y + ascender + (glyphSize.y - glyphPos.y));
			glm::fvec2 topRight(vertPos.x + glyphPos.x + glyphSize.x, vertPos.y + ascender - glyphPos.y);
			glm::fvec2 topLeft(vertPos.x + glyphPos.x, vertPos.y + ascender - glyphPos.y);

			float vertices[] = {
				//Position						Texcoords
				botLeft.x,  botLeft.y,  0,		glyphCoords.x,				glyphCoords.y + cOffset.y,	// Bot-left
				botRight.x, botRight.y, 0,		glyphCoords.x + cOffset.x,	glyphCoords.y + cOffset.y,	// Bot-right
				topRight.x, topRight.y, 0,		glyphCoords.x + cOffset.x,	glyphCoords.y,				// Top-right
				topLeft.x,  topLeft.y,  0,		glyphCoords.x,				glyphCoords.y				// Top-left
			};
			memcpy((char*)(verts.data())+(sizeof(vertices) * index), vertices, sizeof(vertices));

			if (botLeft.x < minX)
				minX = botLeft.x;
			if (botLeft.y > maxY)
				maxY = botLeft.y;

			if (topRight.x > maxX)
				maxX = topRight.x;
			if (topRight.y < minY)
				minY = topRight.y;

			vertPos.x += glyphs->getAdvance(*p).x;
			vertPos.y += glyphs->getAdvance(*p).y;
		}
		++p;
		++index;
	}

	bounds.left = minX;
	bounds.top = minY;
	bounds.width = maxX - minX;
	bounds.height = maxY - minY;
}

void Text::draw(VkCommandBuffer cmd)
{

}

void Text::setFont(Font * pFont)
{
	Font* prevFont = style.font;

	if (prevFont)
		prevFont->releaseGlyphs(style.charSize, this);

	style.setFont(pFont);
	update();
}

void Text::setCharSize(u16 pCharSize)
{
	style.font->releaseGlyphs(style.charSize, this);
	style.setCharSize(pCharSize);
	style.font->requestGlyphs(pCharSize, this);
	update();
}

void Text::setString(std::string pStr)
{
	string = pStr;
	update();
}

void Text::setPosition(glm::fvec2 pPos)
{
	position = pPos;
}

void Text::setOrigin(Origin pOrigin)
{
	style.setOrigin(pOrigin);
	update();
}

void Text::setOrigin(glm::ivec2 pOrigin)
{
	style.setOrigin(pOrigin);
	update();
}

void Text::setColour(glm::fvec3 pColour)
{
	style.setColour(pColour);
}
