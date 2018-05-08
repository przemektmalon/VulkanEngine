#pragma once
#include "PCH.hpp"
#include "Console.hpp"
#include "Engine.hpp"
#include "Window.hpp"

void Console::create()
{
	layer = new OLayer();
	layer->create(glm::ivec2(Engine::window->resX, 300));

	input = new Text();

	input->setName("constext");
	input->setFont(Engine::assets.getFont("consola"));
	input->setColour(glm::fvec4(0.1, 0.9, 0.1, 1.0));
	input->setCharSize(20);
	input->setString("");
	input->setPosition(glm::fvec2(10, 250));
	
	layer->addElement(input);
}

void Console::inputChar(char c)
{
	std::string s; s = input->getString();
	
	if (c == 8) // backspace
	{
		if (s.length() > 0)
			s.erase(s.size() - 1, 1);
	}
	else if (c == 13) // enter
	{
		/// TODO: process command
		if (output.size() == historyLimit)
		{
			auto t = output.back();
			t->cleanup();
			layer->removeElement(t);
			delete t;
			output.pop_back();
			history.pop_back();
		}
		input->setColour(glm::fvec4(0.9, 0.9, 0.9, 1.0));
		history.push_front(input->getString());
		output.push_front(input);

		input = new Text();

		input->setFont(Engine::assets.getFont("consola"));
		input->setColour(glm::fvec4(0.1, 0.9, 0.1, 1.0));
		input->setCharSize(20);
		input->setString("");
		input->setPosition(glm::fvec2(10, 250));

		layer->addElement(input);

		updatePositions();

		return;
	}
	else
	{
		s.append(&c, 1);
	}

	input->setString(s);
}

void Console::updatePositions()
{
	int y = 250 - input->getGlyphs()->getHeight();
	for (auto line : output)
	{
		line->setPosition(glm::fvec2(10, y));
		y -= input->getGlyphs()->getHeight();
	}
}
