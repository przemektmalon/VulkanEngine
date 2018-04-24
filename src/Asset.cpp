#include "PCH.hpp"
#include "Asset.hpp"
#include "File.hpp"

void Asset::prepare(std::vector<std::string>& pDiskPaths, std::string pName)
{
	name = pName;
	diskPaths = pDiskPaths;

	bool pathMissing = false;

	for (auto& diskPath : diskPaths)
	{
		File file;
		file.open(diskPath, File::in);
		if (!file.isOpen()) {
			pathMissing = true;
			DBG_WARNING("Asset \"" << diskPath << "\" not found on disk.");
		}
		size += file.getSize();
		file.close();
	}

	if (!pathMissing)
	{
		availability |= ON_DISK;
	}
}

void Asset::loadToRAM(void* pCreateStruct, AllocFunc)
{
	DBG_WARNING("Attempting to load asset \"" << name << "\" to RAM, but loading function not overriden.")
}

void Asset::loadToGPU(void* pCreateStruct)
{
	DBG_WARNING("Attempting to load asset \"" << name << "\" to GPU, but loading function not overriden.")
}

void Asset::cleanupRAM(FreeFunc)
{
	DBG_WARNING("Attempting to cleanup asset \"" << name << "\" from RAM, but cleanup function not overriden.")
}

void Asset::cleanupGPU()
{
	DBG_WARNING("Attempting to cleanup asset \"" << name << "\" from GPU, but cleanup function not overriden.")
}

bool Asset::isAvailable(AvailabilityFlags flags)
{
	return (availability & flags);
}
