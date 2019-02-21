#include "PCH.hpp"
#include "Engine.hpp"
#include "Threading.hpp"
#include "UIElementGroup.hpp"

void UIElementGroup::addElement(UIElement * el)
{
	Engine::threading->layersMutex.lock();

	auto find = std::find(elements.begin(), elements.end(), el);
	if (find == elements.end())
	{
		// No element with required shader exists, add vector for this type of shader
		elements.push_back(el);
	}
	else
	{
		// Elements already exists
		/// TODO: log error ?
	}

	auto nameFind = elementLabels.find(el->getName());
	if (nameFind == elementLabels.end())
		elementLabels.insert(std::make_pair(el->getName(), el));
	else
	{
		std::string newName = el->getName();
		newName += std::to_string(Engine::clock.now());
		nameFind = elementLabels.find(newName);
		u64 i = 1;
		while (nameFind != elementLabels.end())
		{
			newName = el->getName() + std::to_string(Engine::clock.now() + i);
			nameFind = elementLabels.find(newName);
		}
		elementLabels.insert(std::make_pair(newName, el));
	}

	Engine::threading->layersMutex.unlock();
}

UIElement * UIElementGroup::getElement(std::string pName)
{
	auto find = elementLabels.find(pName);
	if (find == elementLabels.end())
	{
		DBG_WARNING("Layer element not found: " << pName);
		return 0;
	}
	return find->second;
}

void UIElementGroup::removeElement(UIElement * el)
{
	Engine::threading->layersMutex.lock();
	elementsToRemove.push_back(el);
	el->setDoDraw(false);
	Engine::threading->layersMutex.unlock();
}

void UIElementGroup::cleanupElements()
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
