#pragma once
#include "PCH.hpp"
#include "UIElement.hpp"
#include "Texture.hpp"

class UIElementGroup
{
	friend class UIRenderer;
public:
	UIElementGroup() : render(true), needsUpdate(true) {}

	void addElement(UIElement* el);
	UIElement* getElement(std::string pName);
	void removeElement(UIElement* el);
	void cleanupElements();

	void setPosition(glm::ivec2 pPos);
	void setResolution(glm::ivec2 pRes);

	bool doDraw() { return render; }
	void setDoDraw(bool doDraw) {
		render = doDraw;
		needsUpdate = true;
	}

private:

	void sortByDepths()
	{
		std::sort(elements.begin(), elements.end(), compareOverlayElements);
	}

	std::vector<UIElement*> elements;
	std::vector<UIElement*> elementsToRemove;
	std::unordered_map<std::string, UIElement*> elementLabels;

	glm::fvec2 position;
	glm::fvec2 resolution;
	float depth;

	bool render;
	bool needsUpdate;

};