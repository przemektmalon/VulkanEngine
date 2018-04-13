#pragma once
#include "PCH.hpp"

class Buffer
{
public:
	Buffer() {}
	~Buffer() {}

	void create(VkDeviceSize pSize, VkBufferUsageFlags pUsage, VkMemoryPropertyFlags pMemFlags);
	void destroy();

	void* map();
	void unmap();

	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDeviceSize size;
	VkBufferUsageFlags usage;
	VkMemoryPropertyFlags memFlags;
};