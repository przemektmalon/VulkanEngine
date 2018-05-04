#include "PCH.hpp"
#include "OverlayElement.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

OverlayElement::OverlayElement(Type pType) : type(pType)
{
	VkDescriptorSetLayout layouts[] = { Engine::renderer->overlayRenderer.getDescriptorSetLayout() };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = Engine::renderer->descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (type == Text)
		shader = &Engine::renderer->textShader;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(Engine::renderer->device, &allocInfo, &descSet));
}