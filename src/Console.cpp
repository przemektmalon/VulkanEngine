#pragma once
#include "PCH.hpp"
#include "Console.hpp"
#include "Engine.hpp"
#include "Window.hpp"

void Console::create()
{
	layer = new OLayer();
	text = new Text();

	layer->create(glm::ivec2(Engine::window->resX, 600));

	text->setFont(Engine::assets.getFont("consola"));
	text->setColour(glm::fvec4(0.1, 0.9, 0.1, 1.0));
	text->setCharSize(20);
	text->setString("CONSOLE");
	text->setPosition(glm::fvec2(100, 100));
	
	layer->addElement(text);
}

void Console::input(char input)
{
	char c = input;
	std::string s; s = text->getString();
	
	if (c == 8)
	{
		if (s.length() > 0)
			s.erase(s.size() - 1, 1);
	}
	else
	{
		s.append(&c, 1);
	}

	text->setString(s);
}
