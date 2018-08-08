#include "PCH.hpp"
#include "OverlayElement.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

OverlayElement::OverlayElement(Type pType) : type(pType), draw(true)
{
	shader = &Engine::renderer->overlayShader;
		
	depth = 0;

	auto r = Engine::renderer;

	descSet.allocate(&r->logicalDevice, &r->overlayRenderer.getDescriptorSetLayout(), &r->freeableDescriptorPool);
}

OverlayElement::~OverlayElement()
{
	descSet.free();
}
