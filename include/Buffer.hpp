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

	VkBuffer getBuffer() { return buffer; }
	VkDeviceMemory getMemory() { return memory; }
	VkDeviceSize getSize() { return size; }
	VkBufferUsageFlags getUsage() { return usage; }
	VkMemoryPropertyFlags getMemFlags() { return memFlags; }

private:
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDeviceSize size;
	VkBufferUsageFlags usage;
	VkMemoryPropertyFlags memFlags;
};