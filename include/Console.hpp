#pragma once
#include "PCH.hpp"
#include "OverlayRenderer.hpp"
#include "Text.hpp"
#include "Polygon.hpp"
#include "Event.hpp"

class Console
{
public:
	Console() : active(false), layer(0), timeSinceBlink(0), blinkerPosition(1), oldBlinkerPosition(1) {}

	void create();
	void update();
	void inputChar(char c);
	void moveBlinker(Key k);

	void toggle() { active = !active; layer->setDoDraw(active); }
	bool isActive() { return active; }
	OLayer* getLayer() { return layer; }

private:

	void updatePositions();
	void updateBacks();

	OLayer* layer;

	Text* input;
	std::list<Text*> output;
	std::list<std::string> history;
	std::string oldText;

	UIPolygon* backs[2];
	UIPolygon* blinker;

	int blinkerPosition;
	int oldBlinkerPosition;
	float timeSinceBlink;
	bool active;
	const int historyLimit = 10;
};