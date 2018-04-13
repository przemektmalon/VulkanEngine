#include "PCH.hpp"
#include "Buffer.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

void Buffer::create(VkDeviceSize pSize, VkBufferUsageFlags pUsage, VkMemoryPropertyFlags pMemFlags)
{
	size = pSize;
	usage = pUsage;
	memFlags = pMemFlags;

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(Engine::renderer->device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		DBG_SEVERE("Failed to create Vulkan buffer");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(Engine::renderer->device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = Engine::getPhysicalDeviceDetails().getMemoryType(memRequirements.memoryTypeBits, memFlags);

	if (vkAllocateMemory(Engine::renderer->device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
		DBG_SEVERE("Failed to allocate Vulkan buffer memory");
	}

	vkBindBufferMemory(Engine::renderer->device, buffer, memory, 0);
}

void Buffer::destroy()
{
	vkDestroyBuffer(Engine::renderer->device, buffer, 0);
	vkFreeMemory(Engine::renderer->device, memory, 0);
}

void * Buffer::map()
{
	void* data;
	vkMapMemory(Engine::renderer->device, memory, 0, size, 0, &data);
	return data;
}

void Buffer::unmap()
{
	vkUnmapMemory(Engine::renderer->device, memory);
}
