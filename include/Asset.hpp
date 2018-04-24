#pragma once
#include "PCH.hpp"

typedef std::function<void*(size_t)> AllocFunc;
typedef std::function<void(void*)> FreeFunc;

typedef u32 AvailabilityFlags;

class Asset
{
public:
	enum Type { Texture, Material, Model, Font, Script, Undefined };
	enum AvailabilityFlagBits {
		ON_DISK = 1,
		ON_RAM = 2,
		ON_GPU = 4
	};

	Asset() : type(Undefined), availability(0), ramPointer(0), gpuMemory(0), size(0) {}

	void prepare(std::string pDiskPath, std::string pName);

	virtual void loadToRAM(void* pCreateStruct = 0, AllocFunc = malloc);
	virtual void loadToGPU(void* pCreateStruct = 0);

	virtual void cleanupRAM(FreeFunc = free);
	virtual void cleanupGPU();

	bool isAvailable(AvailabilityFlags flags);

	std::string getName() { return name; }
	Type getType() { return type; }
	std::string getDiskPath() { return diskPath; }
	void* getRAMPointer() { return ramPointer; }
	VkDeviceMemory getGPUMemory() { return gpuMemory; }
	u64 getSize() { return size; }

protected:

	std::string name;

	Type type;
	AvailabilityFlags availability;
	std::string diskPath;
	void* ramPointer;
	VkDeviceMemory gpuMemory;

	u64 size;
};