#pragma once
#include "PCH.hpp"
#include "Image.hpp"
#include "Asset.hpp"
#include "vdu/DeviceMemory.hpp"

struct TextureCreateInfo : vdu::TextureCreateInfo
{
	TextureCreateInfo() : pData(0), genMipMaps(0), name(std::string("Unnamed texture")) {}

	void* pData;
	bool genMipMaps;
	std::vector<std::string> paths;
	std::string name;
};

class Texture : public Asset , public vdu::Texture
{
public:
	Texture();

	void setName(std::string pName) { name = pName; }

	void loadToRAM(void* pCreateStruct = 0, AllocFunc alloc = malloc);
	void loadToGPU(void* pCreateStruct = 0);

	void cleanupRAM(FreeFunc fr = free);
	void cleanupGPU();

private:

	bool isMipped;
	u32 gpuIndex;
	Image* img;
};
