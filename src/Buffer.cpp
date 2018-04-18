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

	VK_CHECK_RESULT(vkCreateBuffer(Engine::renderer->device, &bufferInfo, nullptr, &buffer));

	VkMemoryRequirements memRequirements;
	VK_VALIDATE(vkGetBufferMemoryRequirements(Engine::renderer->device, buffer, &memRequirements));

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = Engine::getPhysicalDeviceDetails().getMemoryType(memRequirements.memoryTypeBits, memFlags);

	VK_CHECK_RESULT(vkAllocateMemory(Engine::renderer->device, &allocInfo, nullptr, &memory));

	VK_CHECK_RESULT(vkBindBufferMemory(Engine::renderer->device, buffer, memory, 0));
}

void Buffer::destroy()
{
	VK_VALIDATE(vkDestroyBuffer(Engine::renderer->device, buffer, 0));
	VK_VALIDATE(vkFreeMemory(Engine::renderer->device, memory, 0));
}

void * Buffer::map()
{
	void* data;
	VK_CHECK_RESULT(vkMapMemory(Engine::renderer->device, memory, 0, size, 0, &data));
	return data;
}

void * Buffer::map(VkDeviceSize offset, VkDeviceSize pSize)
{
	void* data;
	VK_CHECK_RESULT(vkMapMemory(Engine::renderer->device, memory, offset, pSize, 0, &data));
	return data;
}

void Buffer::unmap()
{
	VK_VALIDATE(vkUnmapMemory(Engine::renderer->device, memory));
}
