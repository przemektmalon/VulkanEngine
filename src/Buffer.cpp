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

void Buffer::copyTo(Buffer * dst, VkDeviceSize range, VkDeviceSize srcOffset, VkDeviceSize dstOffset)
{
	VkCommandBuffer commandBuffer = Engine::renderer->beginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = srcOffset;
	copyRegion.dstOffset = dstOffset;
	if (range == 0)
		range = size;
	copyRegion.size = range;
	VK_VALIDATE(vkCmdCopyBuffer(commandBuffer, buffer, dst->getBuffer(), 1, &copyRegion));

	Engine::renderer->endSingleTimeCommands(commandBuffer);
}

void Buffer::copyTo(Texture * dst, VkDeviceSize srcOffset, int mipLevel, int baseLayer, int layerCount, VkOffset3D offset, VkExtent3D extent)
{
	VkCommandBuffer commandBuffer = Engine::renderer->beginSingleTimeCommands();

	if (extent.depth == 0)
		extent.depth = 1;
	if (extent.width == 0)
		extent.width = dst->getWidth();
	if (extent.height == 0)
		extent.height = dst->getHeight();

	VkBufferImageCopy region = {};
	region.bufferOffset = srcOffset;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = mipLevel;
	region.imageSubresource.baseArrayLayer = baseLayer;
	region.imageSubresource.layerCount = layerCount;
	region.imageOffset = offset;
	region.imageExtent = extent;

	VK_VALIDATE(vkCmdCopyBufferToImage(commandBuffer, buffer, dst->getImageHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));

	Engine::renderer->endSingleTimeCommands(commandBuffer);
}

void Buffer::setMem(void * src, VkDeviceSize pSize, VkDeviceSize offset)
{
	if (pSize + offset > size)
		DBG_WARNING("Attempting to write out of buffer bounds");
	if (memFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	{
		const auto r = Engine::renderer;
		r->createStagingBuffer(pSize);
		r->copyToStagingBuffer(src, pSize, 0);
		r->stagingBuffer.copyTo(this , pSize, 0, offset);
		r->destroyStagingBuffer();
	}
	else if ((memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && (memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
	{
		auto d = map(offset, pSize);
		memcpy(d, src, pSize);
		unmap();
	}
}

void Buffer::setMem(int src, VkDeviceSize size, VkDeviceSize offset)
{
	/// TODO: 4 byte alignment assumed (and expected by  vulkan). Check for this ?
	int* mem = new int[size/4];
	memset(mem, src, size);
	setMem(mem, size, offset);
	delete[] mem;
}
