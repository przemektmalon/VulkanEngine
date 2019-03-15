#pragma once
#include "PCH.hpp"
#include "UIElement.hpp"
#include "Texture.hpp"
#include "UIText.hpp"
#include "UIPolygon.hpp"

/*
	@brief Container class and factory for UIElements:
		- Owns and manages the memory of all UIElement objects within 
		- Element memory ownership can be taken by calling 'takeElement', removing it from the group
		- Provides the facility to sort the elements by depth order
*/
class UIElementGroup
{
	friend class UIRenderer;
public:
	UIElementGroup(const std::string& groupName) : name(groupName), render(true), needsUpdate(true) {}
	~UIElementGroup() {
		for (auto element : elements) {
			element->cleanup();
			delete element;
		}
	}

	template<typename UIType>
	UIType* createElement(const std::string& label) {
		static_assert(std::is_base_of<UIElement, UIType>::value, "Cannot create UIElement from non UIElement derived class");
		auto newElement = new UIType();
		newElement->setName(label);
		insertElement(newElement);
		return newElement;
	}

	template<typename UIType>
	UIType* createElement() {
		return createElement<UIType>("");
	}
	
	UIElement* getElement(const std::string& elementName);
	UIElement* takeElement(const std::string& elementName);

	void removeElement(UIElement* el);
	
	void setPosition(glm::ivec2 pPos);
	void setResolution(glm::ivec2 pRes);

	bool doDraw() { return render; }
	void setDoDraw(bool doDraw) {
		render = doDraw;
		needsUpdate = true;
	}

	const std::string& getName() const { return name; }

private:

	void garbageCollect();
	void insertElement(UIElement* element);

	void sortByDepths()
	{
		std::sort(elements.begin(), elements.end(), [](UIElement* lhs, UIElement* rhs) -> bool { return lhs->getDepth() < rhs->getDepth(); });
	}

	std::vector<UIElement*> elements;
	std::vector<UIElement*> elementsToRemove;
	std::unordered_map<std::string, UIElement*> elementLabels;

	glm::fvec2 position;
	glm::fvec2 resolution;
	float depth;

	bool render;
	bool needsUpdate;

	std::string name;
};