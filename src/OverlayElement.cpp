#include "PCH.hpp"
#include "OverlayElement.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

OverlayElement::OverlayElement()
{
	VkDescriptorSetLayout layouts[] = { Engine::renderer->overlayDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = Engine::renderer->descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(Engine::renderer->device, &allocInfo, &descSet));
}