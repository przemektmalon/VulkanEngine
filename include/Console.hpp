#pragma once
#include "PCH.hpp"
#include "OverlayRenderer.hpp"
#include "Text.hpp"
#include "Event.hpp"

class Console
{
public:
	Console() : active(false), layer(0) {}

	void create();

	void inputChar(char c);

	void toggle() { active = !active; }
	bool isActive() { return active; }
	OLayer* getLayer() { return layer; }

private:

	void updatePositions();

	OLayer* layer;

	Text* input;
	std::list<Text*> output;
	std::list<std::string> history;

	bool active;
	const int historyLimit = 10;
};