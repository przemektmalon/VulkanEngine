#include "PCH.hpp"
#include "UIElement.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

UIElement::UIElement(Type pType) : type(pType), draw(true)
{
	shader = &Engine::renderer->overlayShader;
	
	colour = glm::fvec4(1, 1, 1, 1);
	depth = 0;

	auto r = Engine::renderer;

	descSet.allocate(&r->logicalDevice, &r->overlayRenderer.getDescriptorSetLayout(), &r->freeableDescriptorPool);
}

UIElement::~UIElement()
{
	descSet.free();
}
