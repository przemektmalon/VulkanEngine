#include "PCH.hpp"
#include "Text.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

Text::Text() : OverlayElement(OverlayElement::Text)
{
	pushConstData = new glm::fvec4;
	pushConstSize = sizeof(glm::fvec4);
	drawUpdate = true;
}

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

void Text::Style::setColour(glm::fvec4 pColour)
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
	
	verts.clear();
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
				botLeft.x,  botLeft.y,  		glyphCoords.x,				glyphCoords.y + cOffset.y,	// Bot-left
				botRight.x, botRight.y, 		glyphCoords.x + cOffset.x,	glyphCoords.y + cOffset.y,	// Bot-right
				topRight.x, topRight.y, 		glyphCoords.x + cOffset.x,	glyphCoords.y,				// Top-right

				topRight.x, topRight.y, 		glyphCoords.x + cOffset.x,	glyphCoords.y,				// Top-right
				topLeft.x,  topLeft.y,  		glyphCoords.x,				glyphCoords.y,				// Top-left
				botLeft.x,  botLeft.y,  		glyphCoords.x,				glyphCoords.y + cOffset.y,	// Bot-left
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

	memcpy(pushConstData, &style.colour, sizeof(style.colour));

	drawUpdate = true;

	static bool descMade = false;

	if (descMade)
	{
		char* data = (char*)vertsBuffer.map();
		memcpy(data, verts.data(), verts.size() * sizeof(Vertex2D));
		vertsBuffer.unmap();
		return;
	}
	descMade = true;

	/// TODO: reuse buffer if possible
	if (vertsBuffer.getBuffer())
		vertsBuffer.destroy();
	/// TODO: Were guessing upper bound. Implement a more sophistocated approach
	vertsBuffer.create(2000 * 6 * sizeof(Vertex2D), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
	char* data = (char*)vertsBuffer.map();
	memcpy(data, verts.data(), verts.size() * sizeof(Vertex2D));
	vertsBuffer.unmap();



	VkDescriptorImageInfo fontInfo = {};
	fontInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	fontInfo.imageView = glyphs->getTexture()->getImageViewHandle();
	fontInfo.sampler = Engine::renderer->textureSampler;

	std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &fontInfo;

	VK_VALIDATE(vkUpdateDescriptorSets(Engine::renderer->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr));
}

void Text::draw(VkCommandBuffer cmd)
{
	VkDeviceSize offsets[] = { 0 };
	VkBuffer buffer[] = { vertsBuffer.getBuffer() };
	VK_VALIDATE(vkCmdBindVertexBuffers(cmd, 0, 1, buffer, offsets));
	VK_VALIDATE(vkCmdDraw(cmd, verts.size(), 1, 0, 0));
}

void Text::cleanup()
{
	vertsBuffer.destroy();
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

void Text::setColour(glm::fvec4 pColour)
{
	style.setColour(pColour);
}