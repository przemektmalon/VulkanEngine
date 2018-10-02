#include "PCH.hpp"
#include "Text.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

Text::Text() : OverlayElement(OverlayElement::Text)
{
	pushConstData = new glm::fvec4;
	pushConstSize = sizeof(glm::fvec4);
	drawUpdate = true;
	drawable = false;
	
	/// TODO: Were guessing upper bound. Implement a more sophistocated approach
	vertsBuffer.setMemoryProperty(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vertsBuffer.setUsage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	vertsBuffer.create(&Engine::renderer->logicalDevice, 2000 * 6 * sizeof(Vertex2D));
	memset(vertsBuffer.getMemory()->map(), 0, 2000 * 6 * sizeof(Vertex2D));
	vertsBuffer.getMemory()->unmap();
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
	drawingMutex.lock();

	glyphs = style.font->requestGlyphs(style.charSize, this); /// TODO: We get device lost sometimes, most likely a threading issue !!
	bounds.zero();

	drawable = false;

	if (style.charSize == 0) { drawingMutex.unlock(); return; }
	if (string.length() == 0) { drawingMutex.unlock(); return; }
	if (style.font == nullptr) { drawingMutex.unlock(); return; }

	drawable = true;

	int numVerts = string.length() * 6;

	verts.clear();
	verts.resize(numVerts);

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
		else if (*p == ' ') // Include trailing spaces in bounds calculation
		{
			int spaceAdvance = glyphs->getAdvance(*p).x;
			float advancedX = vertPos.x + glyphPos.x + spaceAdvance;
			if (advancedX > maxX)
				maxX = advancedX;
			vertPos.x += glyphs->getAdvance(*p).x;
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
			memcpy((char*)(verts.data()) + (sizeof(vertices) * index), vertices, sizeof(vertices));

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

	char* data = (char*)vertsBuffer.getMemory()->map();
	memcpy(data, verts.data(), verts.size() * sizeof(Vertex2D));
	vertsBuffer.getMemory()->unmap();

	drawingMutex.unlock();
}

void Text::render(VkCommandBuffer cmd)
{
	drawingMutex.lock();
	if (drawable)
	{
		VkDeviceSize offsets[] = { 0 };
		VkBuffer buffer[] = { vertsBuffer.getHandle() };
		vkCmdBindVertexBuffers(cmd, 0, 1, buffer, offsets);
		vkCmdDraw(cmd, verts.size(), 1, 0, 0);
	}
	drawingMutex.unlock();
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
	updateDescriptorSet();
}

void Text::setCharSize(u16 pCharSize)
{
	if (style.charSize == pCharSize)
		return;
	style.font->releaseGlyphs(style.charSize, this);
	style.setCharSize(pCharSize);
	style.font->requestGlyphs(pCharSize, this);
	update();
	updateDescriptorSet();
}

void Text::setString(std::string pStr)
{
	string = pStr;
	update();
}

void Text::setPosition(glm::fvec2 pPos)
{
	position = pPos;
	update();
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

glm::fvec2 Text::getCharsPosition(int index)
{
	if (style.charSize == 0) { return glm::fvec2(0,0); }
	if (string.length() == 0) { return glm::fvec2(0, 0); }
	if (style.font == nullptr) { return glm::fvec2(0, 0); }

	auto tpos = position - glm::fvec2(style.origin);
	glm::ivec2 vertPos(tpos);

	float ascender = glyphs->getAscender();

	auto p = string.c_str();

	glm::ivec2 glyphPos = glyphs->getPosition(*p);
	glm::ivec2 glyphSize = glyphs->getSize(*p);

	int i = 0;

	while (*p != 0)
	{
		if (*p == '\n')
		{
			vertPos.x = tpos.x;
			vertPos.y += style.charSize;
		}
		else
		{
			vertPos.x += glyphs->getAdvance(*p).x;
			vertPos.y += glyphs->getAdvance(*p).y;
		}
		if (i == index)
			return vertPos;
		++i;
		++p;
	}
	return vertPos;
}

void Text::updateDescriptorSet()
{
	VkDescriptorImageInfo fontInfo = {};
	fontInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	fontInfo.imageView = glyphs->getTexture()->getView();
	fontInfo.sampler = Engine::renderer->textureSampler;

	std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descSet.getHandle();
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &fontInfo;

	vkUpdateDescriptorSets(Engine::renderer->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}
