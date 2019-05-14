#pragma once
#include "PCH.hpp"
#include "Image.hpp"
#include "Asset.hpp"

struct TextureCreateInfo : vdu::TextureCreateInfo
{
	TextureCreateInfo() : pData(nullptr), image(nullptr), genMipMaps(0), name(std::string("Unnamed texture")) {}

	TextureCreateInfo(Image* sourceImage) : pData(nullptr), image(sourceImage), genMipMaps(0), name(std::string("Unnamed texture")) {}

	Image* image;
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
