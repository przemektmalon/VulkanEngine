#pragma once
#include "PCH.hpp"
#include "Asset.hpp"

class ShaderProgram : public vdu::ShaderProgram
{
public:
	ShaderProgram() : vdu::ShaderProgram() {}

	void prepare(std::vector<std::string>& pDiskPaths, std::string pName);

	void loadToRAM(void* pCreateStruct = 0, AllocFunc alloc = malloc);
	void loadToGPU(void* pCreateStruct = 0);

	void cleanupRAM(FreeFunc fr = free);
	void cleanupGPU();
};
