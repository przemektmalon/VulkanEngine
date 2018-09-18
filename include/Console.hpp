#pragma once
#include "PCH.hpp"
#include "OverlayRenderer.hpp"
#include "Text.hpp"
#include "Polygon.hpp"
#include "Event.hpp"

class Console
{
public:
	Console() : active(true), layer(0), timeSinceBlink(0), blinkerPosition(1), oldBlinkerPosition(1), scrollPosition(0) { }

	void create(glm::ivec2 resolution);
	void update();
	void inputChar(char c);
	void moveBlinker(Key k);
	void scroll(s16 wheelDelta);

	void toggle() { active = !active; layer->setDoDraw(active); }
	void setActive(bool set) { active = set; layer->setDoDraw(active); }
	bool isActive() { return active; }
	OLayer* getLayer() { return layer; }

	void postMessage(std::string msg, glm::fvec3 colour);

	void renderAtStartup();

	void setResolution(glm::ivec2 res);

private:

	void updatePositions();
	void updateBacks();

	OLayer* layer;

	Text* input;
	std::list<Text*> output;
	std::list<std::string> history;
	std::string oldText;

	glm::ivec2 resolution;

	UIPolygon* backs[2];
	UIPolygon* blinker;

	std::mutex messagePostMutex;

	float scrollPosition;
	int blinkerPosition;
	int oldBlinkerPosition;
	float timeSinceBlink;
	bool active;
	const int historyLimit = 1000;
};