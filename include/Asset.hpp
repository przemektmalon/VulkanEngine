#pragma once
#include "PCH.hpp"

typedef std::function<void*(size_t)> AllocFunc;
typedef std::function<void(void*)> FreeFunc;

class Asset
{
public:
	enum Type { Texture, Material, Model, Font, Script, Undefined };
	enum AvailabilityFlagBits {
		ON_DISK = 1,
		ON_RAM = 2,
		ON_GPU = 4,
		LOADING_TO_RAM = 8,
		LOADING_TO_GPU = 16,
	};

	Asset() : type(Undefined), availability(0), ramPointer(0), gpuMemory(0), size(0) {}

	void prepare(std::vector<std::string>& pDiskPaths, std::string pName);

	virtual void loadToRAM(void* pCreateStruct = 0, AllocFunc alloc = malloc);
	virtual void loadToGPU(void* pCreateStruct = 0);

	virtual void cleanupRAM(FreeFunc fr = free);
	virtual void cleanupGPU();

	bool checkAvailability(int flags);
	std::atomic_char32_t& getAvailability() { return availability; }

	std::string getName() { return name; }
	Type getType() { return type; }
	const std::vector<std::string>& getDiskPath() { return diskPaths; }
	void* getRAMPointer() { return ramPointer; }
	VkDeviceMemory getGPUMemory() { return gpuMemory; }
	u64 getSize() { return size; }

protected:

	std::string name;
	Type type;
	std::atomic_char32_t availability;
	std::vector<std::string> diskPaths;
	void* ramPointer;
	VkDeviceMemory gpuMemory;

	u64 size;
};