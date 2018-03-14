#pragma once
#include "PCH.hpp"
#include "Model.hpp"
#include "Material.hpp"

class AssetStore
{
public:

	void cleanup();
	void loadAssets(std::string assetListFilePath);

	Material* getMaterial(std::string matName) { auto ret = materials.find(matName); return ret != materials.end() ? &ret->second : nullptr; }
	Texture* getTexture(std::string texName) { auto ret = textures.find(texName); return ret != textures.end() ? &ret->second : nullptr; }
	Model* getModel(std::string modName) { auto ret = models.find(modName); return ret != models.end() ? &ret->second : nullptr; }

	std::unordered_map<std::string, Model> models;
	std::unordered_map<std::string, Material> materials;
	std::unordered_map<std::string, Texture> textures;

	void addMaterial(std::string name, std::string albedo, std::string normal = std::string(""), std::string specMetal = std::string(""), std::string roughness = std::string(""), std::string ao = std::string(""), std::string height = std::string(""));
};