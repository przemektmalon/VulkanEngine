#pragma once
#include "PCH.hpp"
#include "UIRenderer.hpp"
#include "UIText.hpp"
#include "UIPolygon.hpp"
#include "Event.hpp"

class Console
{
public:
	Console() : active(true), uiGroup(0), timeSinceBlink(0), blinkerPosition(1), oldBlinkerPosition(1), scrollPosition(0) { }

	void create(glm::ivec2 resolution);
	void update();
	void cleanup();
	void inputChar(char c);
	void moveBlinker(Key k);
	void scroll(s16 wheelDelta);

	void toggle() { active = !active; uiGroup->setDoDraw(active); }
	void setActive(bool set) { active = set; uiGroup->setDoDraw(active); }
	bool isActive() { return active; }
	UIElementGroup* getUIGroup() { return uiGroup; }

	void postMessage(std::string msg, glm::fvec3 colour);

	void renderAtStartup();

	void setResolution(glm::ivec2 res);

	const vdu::Fence& getStartupRenderFence() { return finishedStartupRenderFence; }

private:

	void updatePositions();
	void updateBacks();

	UIElementGroup* uiGroup;
	vdu::Fence finishedStartupRenderFence;

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