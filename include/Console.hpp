#pragma once
#include "PCH.hpp"
#include "OverlayRenderer.hpp"
#include "Text.hpp"
#include "Event.hpp"

class Console
{
public:
	Console() : active(false), text(0), layer(0) {}

	void create();

	void input(char input);
	void toggle() { active = !active; }
	bool isActive() { return active; }
	OLayer* getLayer() { return layer; }

private:

	OLayer* layer;
	Text* text;
	bool active;

};