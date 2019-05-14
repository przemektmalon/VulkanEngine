#pragma once
#include "PCH.hpp"
#include "Texture.hpp"
#include "Asset.hpp"

struct Material : public Asset
{
	Material() {}
	//Material(const std::string& name) { std::vector<std::string> paths; prepare(paths, name); }
	Material(const std::string& name) { std::vector<std::string> paths; }
	u32 gpuIndexBase = 0;

	union MaterialData {
		MaterialData() {}
		struct Textures {
			::Texture* albedoSpec = nullptr, * normalRough = nullptr;
		} textures;
		struct PBRData {
			glm::fvec3 colour = glm::fvec3(1, 0, 1);
			float metallic = 0.2f;
			float roughness = 0.6f;
			glm::fvec3 _PAD;
		} pbrData;
	} data;


	void loadToRAM(void* pCreateStruct = 0, AllocFunc alloc = malloc);
	void loadToGPU(void* pCreateStruct = 0);

	Material& operator=(Material& rhs)
	{
		data = rhs.data;
		gpuIndexBase = rhs.gpuIndexBase;
		//albedoSpec = rhs.albedoSpec;
		//normalRough = rhs.normalRough;
		
		return *this;
	}
};