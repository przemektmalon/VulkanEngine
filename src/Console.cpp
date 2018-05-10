#pragma once
#include "PCH.hpp"
#include "Console.hpp"
#include "Engine.hpp"
#include "Window.hpp"

void Console::create()
{
	layer = new OLayer();
	layer->create(glm::ivec2(Engine::window->resX, 276));

	input = new Text();

	input->setName("constext");
	input->setFont(Engine::assets.getFont("consola"));
	input->setColour(glm::fvec4(0.1, 0.9, 0.1, 1.0));
	input->setCharSize(20);
	input->setString("");
	input->setPosition(glm::fvec2(10, 253));
	
	input->setDepth(2);

	layer->addElement(input);

	backs[0] = new UIPolygon();
	backs[1] = new UIPolygon();

	backs[0]->setTexture(Engine::assets.getTexture("blank"));
	backs[1]->setTexture(Engine::assets.getTexture("blank"));

	backs[0]->setColour(glm::fvec4(0.1, 0.1, 0.1, 0.99f));
	backs[1]->setColour(glm::fvec4(0.05, 0.05, 0.05, 0.99f));

	backs[0]->setDepth(1);
	backs[1]->setDepth(1);

	layer->addElement(backs[0]);
	layer->addElement(backs[1]);

	layer->setDoDraw(active);
	
	updateBacks();
}

void Console::inputChar(char c)
{
	if (!isActive())
		return;

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
		input->setPosition(glm::fvec2(10, 253));
		input->setDepth(2);

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
	int y = 251 - input->getGlyphs()->getHeight();
	for (auto line : output)
	{
		line->setPosition(glm::fvec2(10, y));
		y -= input->getGlyphs()->getHeight() + 2;
	}
}

void Console::updateBacks()
{
	int w = Engine::window->resX, h = 250;
	std::vector<Vertex2D> verts;

	verts.push_back({ { 0, 0 },{ 0, 0 } });
	verts.push_back({ { 0, h },{ 0, 1 } });
	verts.push_back({ { w, 0 },{ 1, 0 } });
	verts.push_back({ { 0, h },{ 0, 1 } });
	verts.push_back({ { w, h },{ 1, 1 } });
	verts.push_back({ { w, 0 },{ 1, 0 } });
	backs[0]->setVerts(verts);

	verts.clear();

	verts.push_back({ { 0, h },{ 0, 0 } });
	verts.push_back({ { 0, h+26 },{ 0, 1 } });
	verts.push_back({ { w, h },{ 1, 0 } });
	verts.push_back({ { 0, h+26 },{ 0, 1 } });
	verts.push_back({ { w, h+26 },{ 1, 1 } });
	verts.push_back({ { w, h },{ 1, 0 } });
	backs[1]->setVerts(verts);

	verts.clear();
}
