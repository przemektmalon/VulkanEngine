#pragma once
#include "PCH.hpp"
#include "Font.hpp"
#include "Rect.hpp"
#include "Model.hpp"

class Text
{
public:

	enum Origin { BotLeft, TopLeft, TopRight, BotRight, TopMiddle, BotMiddle, MiddleLeft, MiddleRight, MiddleMiddle, Undefined };

	struct Style
	{
		Style() : font(0), charSize(16), originPreDef(TopLeft), origin(0, 0), colour(glm::fvec3(1, 1, 1)) {}
		Style(Font* pFont, u16 pCharSize = 16, Origin pOriginPreDef = TopLeft, glm::ivec2 pOrigin = glm::ivec2(0, 0), glm::fvec3 pColour = glm::fvec3(1, 1, 1)) :
			font(pFont),
			charSize(pCharSize),
			originPreDef(pOriginPreDef),
			origin(pOrigin),
			colour(pColour) {}
		Font * font;
		u16 charSize;
		Origin originPreDef;
		glm::ivec2 origin;
		glm::fvec3 colour;

		void setFont(Font* pFont);
		void setCharSize(u16 pCharSize);
		void setOrigin(Origin pOrigin);
		void setOrigin(glm::ivec2 pOrigin);
		void setColour(glm::fvec3 pColour);
	};

	Text() {}
	Text(Font* pFont) : style(pFont) {}
	Text(Font* pFont, u16 pCharSize) : style(pFont, pCharSize) {}

	void update();

	void draw(VkCommandBuffer cmd);

	void setFont(Font* pFont);
	void setCharSize(u16 pCharSize);
	void setString(std::string pStr);
	void setPosition(glm::fvec2 pPos);
	void setOrigin(Origin pOrigin);
	void setOrigin(glm::ivec2 pOrigin);
	void setColour(glm::fvec3 pColour);

	Font* getFont() { return style.font; }
	u16 getCharSize() { return style.charSize; }
	std::string getString() { return string; }
	glm::fvec2 getPosition() { return position; }
	frect getBounds() { return bounds; }
	Style getStyle() { return style; }

private:

	Style style;

	std::string string;

	Origin windowOrigin;
	glm::fvec2 position;
	frect bounds;

	GlyphContainer* glyphs;

	std::vector<Vertex2D> verts;
};