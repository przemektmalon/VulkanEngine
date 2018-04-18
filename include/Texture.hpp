#pragma once
#include "PCH.hpp"
#include "Image.hpp"

struct TextureCreateInfo
{
	TextureCreateInfo() : width(0), height(0), pData(0), format(VkFormat(0)), bpp(0), genMipMaps(0), aspectFlags(0), usageFlags(0), layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {}
	int width;
	int height;
	int bpp;
	void* pData;
	bool genMipMaps;
	VkFormat format;
	VkImageAspectFlags aspectFlags;
	VkImageUsageFlags usageFlags;
	VkImageLayout layout;
};

class Texture
{
public:
	Texture() : width(0), height(0), maxMipLevel(0), vkImage(0), vkMemory(0), vkImageView(0) {}
	Texture(std::string pPath) { loadFile(pPath); }

	void setName(std::string pName) { name = pName; }
	VkImage getImageHandle() { return vkImage; }
	VkDeviceMemory getMemoryHandle() { return vkMemory; }
	VkImageView getImageViewHandle() { return vkImageView; }
	VkImageLayout getLayout() { return vkLayout; }
	VkImageAspectFlags getAspect() { return vkAspect; }
	int getMaxMipLevel() { return maxMipLevel; }

	void loadFile(std::string pPath, bool genMipMaps = true);
	void loadImage(Image* pImage, bool genMipMaps = true);

	void loadStream(TextureCreateInfo* ci);

	void destroy();

private:

	std::string name;
	int width, height;
	int maxMipLevel;
	VkImage vkImage;
	VkDeviceMemory vkMemory;
	VkImageView vkImageView;
	VkFormat vkFormat;
	VkImageLayout vkLayout;
	VkImageAspectFlags vkAspect;
	VkImageUsageFlags vkUsage;
	u32 gpuIndex;
};
