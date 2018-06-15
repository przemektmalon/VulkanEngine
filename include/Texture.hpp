#pragma once
#include "PCH.hpp"
#include "Image.hpp"
#include "Asset.hpp"
#include "VDU.hpp"

struct TextureCreateInfo : vdu::TextureCreateInfo
{
	TextureCreateInfo() : pData(0), /*bpp(0), components(4), */genMipMaps(0), name(std::string("Unnamed texture")) {}

	/*int bpp;
	int components;*/
	void* pData;
	bool genMipMaps;
	std::vector<std::string> paths;
	//std::string* pPaths;
	std::string name;
};

class Texture : public Asset , public vdu::Texture
{
public:
	Texture() : vdu::Texture() {}

	void setName(std::string pName) { name = pName; }
	
	/*
	VkImage getHandle() { return vkImage; }
	VkDeviceMemory getMemoryHandle() { return vkMemory; }
	VkImageView getView() { return vkImageView; }
	VkImageLayout getLayout() { return vkLayout; }
	VkImageAspectFlags getAspect() { return vkAspect; }
	int getMaxMipLevel() { return maxMipLevel; }
	u32 getWidth() { return m_width; }
	u32 getHeight() { return m_height; }
	int getNumLayers() { return m_depth; }
	VkFormat getFormat() { return vkFormat; }
	*/

	void loadToRAM(void* pCreateStruct = 0, AllocFunc alloc = malloc);
	void loadToGPU(void* pCreateStruct = 0);

	void cleanupRAM(FreeFunc fr = free);
	void cleanupGPU();

private:

	//int bpp, components;

	/*int m_width, m_height, m_depth;
	int maxMipLevel;
	VkImage vkImage;
	VkDeviceMemory vkMemory;
	VkImageView vkImageView;
	VkFormat vkFormat;
	VkImageLayout vkLayout;
	VkImageAspectFlags vkAspect;
	VkImageUsageFlags vkUsage;*/

	bool isMipped;
	u32 gpuIndex;
	Image* img;
};
