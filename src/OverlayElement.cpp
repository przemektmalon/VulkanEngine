#include "PCH.hpp"
#include "OverlayElement.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

OverlayElement::OverlayElement(Type pType) : type(pType), draw(true)
{
	VkDescriptorSetLayout layouts[] = { Engine::renderer->overlayRenderer.getDescriptorSetLayout() };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = Engine::renderer->freeableDescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	shader = &Engine::renderer->overlayShader;
		
	depth = 0;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(Engine::renderer->device, &allocInfo, &descSet));
}

OverlayElement::~OverlayElement()
{
	VK_CHECK_RESULT(vkFreeDescriptorSets(Engine::renderer->device, Engine::renderer->freeableDescriptorPool, 1, &descSet));
}
