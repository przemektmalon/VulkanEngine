#pragma once
#include "PCH.hpp"
#include "Texture.hpp"

class Buffer
{
public:
	Buffer() {}
	~Buffer() {}

	void create(VkDeviceSize pSize, VkBufferUsageFlags pUsage, VkMemoryPropertyFlags pMemFlags);
	void destroy();

	void* map();
	void* map(VkDeviceSize offset, VkDeviceSize pSize);
	void unmap();

	void copyTo(Buffer* dst, VkDeviceSize range = 0, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);
	void copyTo(Texture* dst, VkDeviceSize srcOffset = 0, int mipLevel = 0, int baseLayer = 0, int layerCount = 1, VkOffset3D offset = { 0,0,0 }, VkExtent3D extent = { 0,0,0 });
	void setMem(void* src, VkDeviceSize size, VkDeviceSize offset = 0);

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