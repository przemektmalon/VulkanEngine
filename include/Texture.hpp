#pragma once
#include "PCH.hpp"
#include "Image.hpp"
#include "Asset.hpp"

struct TextureCreateInfo
{
	TextureCreateInfo() : width(0), height(0), pData(0), format(VkFormat(0)), bpp(0), components(4), genMipMaps(0), aspectFlags(0), usageFlags(0), layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL), pPaths(0), numLayers(1), name(std::string("Unnamed texture")) {}
	int width;
	int height;
	int bpp;
	int components;
	void* pData;
	bool genMipMaps;
	VkFormat format;
	VkImageAspectFlags aspectFlags;
	VkImageUsageFlags usageFlags;
	VkImageLayout layout;
	std::string* pPaths;
	int numLayers;
	std::string name;
};

class Texture : public Asset
{
public:
	Texture() : width(0), height(0), components(0), numLayers(1), maxMipLevel(1), vkImage(0), vkMemory(0), vkImageView(0) {}
	Texture(std::string pPath) { loadFile(pPath); }

	void setName(std::string pName) { name = pName; }
	VkImage getImageHandle() { return vkImage; }
	VkDeviceMemory getMemoryHandle() { return vkMemory; }
	VkImageView getImageViewHandle() { return vkImageView; }
	VkImageLayout getLayout() { return vkLayout; }
	VkImageAspectFlags getAspect() { return vkAspect; }
	int getMaxMipLevel() { return maxMipLevel; }
	u32 getWidth() { return width; }
	u32 getHeight() { return height; }
	int getNumLayers() { return numLayers; }
	VkFormat getFormat() { return vkFormat; }

	void loadToRAM(void* pCreateStruct = 0, AllocFunc = malloc);
	void loadToGPU(void* pCreateStruct = 0);

	void create(TextureCreateInfo* ci);

	void destroy();

	static void createImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, int mipLevels = 1, int numLayers = 1);
	static VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, int mipLevels, int numLayers = 1);
	
private:

	void loadFile(std::string pPath, bool genMipMaps = true);
	void loadImage(Image* pImage, bool genMipMaps = true);
	void loadCube(std::string pPaths[6], bool genMipMaps = true);
	void loadStream(TextureCreateInfo* ci);

	void createImage();
	void createImageView();
	void generateMipMaps();

	int width, height;
	int bpp, components;
	int maxMipLevel;
	VkImage vkImage;
	VkDeviceMemory vkMemory;
	VkImageView vkImageView;
	VkFormat vkFormat;
	VkImageLayout vkLayout;
	VkImageAspectFlags vkAspect;
	VkImageUsageFlags vkUsage;
	int numLayers;
	bool mipped;
	u32 gpuIndex;
	Image* img;
};
