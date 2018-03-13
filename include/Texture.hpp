#pragma once
#include "PCH.hpp"
#include "Image.hpp"

class Texture
{
public:
	Texture() : width(0), height(0), maxMipLevel(0), vkImage(0), vkMemory(0), vkImageView(0) {}

	VkImage getImageHandle() { return vkImage; }
	VkDeviceMemory getMemoryHandle() { return vkMemory; }
	VkImageView getImageViewHandle() { return vkImageView; }

	void loadFile(std::string pPath, bool genMipMaps = true);
	void loadImage(Image* pImage, bool genMipMaps = true);

	void destroy();

private:

	int width, height;
	int maxMipLevel;
	VkImage vkImage;
	VkDeviceMemory vkMemory;
	VkImageView vkImageView;
};