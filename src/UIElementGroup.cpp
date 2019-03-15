#include "PCH.hpp"
#include "Engine.hpp"
#include "Threading.hpp"
#include "UIElementGroup.hpp"

UIElement * UIElementGroup::getElement(const std::string& elementName)
{
	auto find = elementLabels.find(elementName);
	if (find == elementLabels.end())
	{
		DBG_WARNING("Layer element not found: " << elementName);
		return nullptr;
	}
	return find->second;
}

UIElement * UIElementGroup::takeElement(const std::string & elementName)
{
	return nullptr;
}

void UIElementGroup::removeElement(UIElement * el)
{
	Engine::threading->layersMutex.lock();
	elementsToRemove.push_back(el);
	el->setDoDraw(false);
	Engine::threading->layersMutex.unlock();
}

void UIElementGroup::garbageCollect()
{
	Engine::threading->layersMutex.lock();

	for (auto elToRemove : elementsToRemove) // For each element to remove
	{
		// Find the iterator of the element to remove
		auto findElement = std::find(elements.begin(), elements.end(), elToRemove);

		// If the element was found
		if (findElement != elements.end())
		{
			// Get pointer before iterator becamos invalid, so we can call the destructor
			auto ptr = *findElement;
			ptr->cleanup();
			elementLabels.erase((*findElement)->getName());
			elements.erase(findElement);
			delete ptr;
		}
	}

	elementsToRemove.clear();

	Engine::threading->layersMutex.unlock();
}

void UIElementGroup::setPosition(glm::ivec2 pPos)
{
	position = pPos;
}

void UIElementGroup::setResolution(glm::ivec2 pRes)
{
	resolution = pRes;
}

void UIElementGroup::insertElement(UIElement * element)
{
	Engine::threading->layersMutex.lock();

	auto find = std::find(elements.begin(), elements.end(), element);
	if (find == elements.end())
	{
		// No element with required shader exists, add vector for this type of shader
		elements.push_back(element);
	}
	else
	{
		// Element already exists
		/// TODO: log error ?
	}

	auto nameFind = elementLabels.find(element->getName());
	if (nameFind == elementLabels.end())
		elementLabels.insert(std::make_pair(element->getName(), element));
	else
	{
		std::string newName = element->getName();
		newName += std::to_string(Engine::clock.now());
		nameFind = elementLabels.find(newName);
		u64 i = 1;
		while (nameFind != elementLabels.end())
		{
			newName = element->getName() + std::to_string(Engine::clock.now() + i);
			nameFind = elementLabels.find(newName);
		}
		elementLabels.insert(std::make_pair(newName, element));
	}

	Engine::threading->layersMutex.unlock();
}
