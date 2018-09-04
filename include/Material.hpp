#pragma once
#include "PCH.hpp"
#include "Texture.hpp"
#include "Asset.hpp"

struct Material : public Asset
{
	Material() {}
	Material(const std::string& name) { std::vector<std::string> paths; prepare(paths, name); }
	u32 gpuIndexBase;
	::Texture *albedoSpec, *normalRough;

	void loadToRAM(void* pCreateStruct = 0, AllocFunc alloc = malloc);
	void loadToGPU(void* pCreateStruct = 0);

	Material& operator=(Material& rhs)
	{
		albedoSpec = rhs.albedoSpec;
		normalRough = rhs.normalRough;
		
		return *this;
	}
};