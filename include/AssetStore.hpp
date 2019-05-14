#pragma once
#include "PCH.hpp"
#include "Model.hpp"
#include "Material.hpp"
#include "Font.hpp"



class AssetStore
{
public:

	////
	bool loadAsset(Asset::Type type, const std::string& name) {
		switch (type) {
		case Asset::Model:
			
			// Check if the model exists on DISK
			auto modelOnDisk = modelsOnDisk.find(name);
			if (modelOnDisk == modelsOnDisk.end())
				return false; /// TODO: log this (model is not on disk)

			// Check if the Model object already has a place in RAM
			// If it does then we dont need to load/allocate it
			auto objectInRAM = models.find(name);
			if (objectInRAM != models.end())
				return false; /// TODO: log this (model already in RAM)

			// If Model is on DISK but not in RAM, then load it to RAM

			// Find AssetInfo from cached asset meta-data

			AssetInfo& ai = modelOnDisk->second;

			Model& model = models.try_emplace(name).first->second;
			std::vector<std::string> path{ "res/models_test/" + modelOnDisk->second.name + "." + modelOnDisk->second.extension };
			
			model.lodPaths = { ai.fullPath };
			model.lodLimits = { 0 };

			model.prepare(path, name);
			model.getAvailability() |= Asset::ON_DISK;
			model.material = getMaterial("null");

			return true;
		}
	}
	void unloadAsset(const std::string name); /// Should be unload assets by their handle (hash) ??
	////

	void cleanup();

	/// DEPRACATED !
	void loadAssets(std::string assetListFilePath);
	////////////////

	void loadDefaultAssets();

	// Scans the filesystem for assets
	// lets the engine know which ones are available on disk
	void gatherAvailableAssets();

	Material* getMaterial(std::string matName) 
	{ 
		auto ind = materialIndices.find(matName);
		if (ind == materialIndices.end())
			return nullptr;
		auto ret = materials.find(ind->second);
		return ret != materials.end() ? &ret->second : nullptr;
	}
	Texture* getTexture(std::string texName) { 
		auto ret = textures.find(texName); 
		return ret != textures.end() ? &ret->second : nullptr; }
	Model* getModel(std::string modName) { auto ret = models.find(modName); return ret != models.end() ? &ret->second : nullptr; }
	Font* getFont(std::string fontName) { auto ret = fonts.find(fontName); return ret != fonts.end() ? &ret->second : nullptr; }

	/// TODO: big problem with this is that when maps grow, any pointers to assets become invalid
	///       we want custom allocators and containers that tackle this problem efficiently
	std::unordered_map<std::string, Model> models;
	std::unordered_map<u32, Material> materials;
	std::unordered_map<std::string, u32> materialIndices;
	std::unordered_map<std::string, Texture> textures;
	std::unordered_map<std::string, Font> fonts;




	void addMaterial(std::string name, std::string albedoSpec, std::string normalRough);
	void addMaterial(std::string name, Texture* albedoMetal, Texture* normalRough);
	void addMaterial(std::string name, glm::fvec3 albedo, float roughness, float metal);

	FT_Library freetype;

private:

	struct AssetInfo {
		std::string name;
		std::string extension;
		std::string fullPath;
		std::uintmax_t sizeInBytes;
	};

	// Returns a struct with information about the asset
	void getAssetInfo(const std::string& assetPath, AssetInfo& asset);


public:

	// Maps -> key = asset_handle (hash of asset_name), value = AssetInfo struct
	// To access asset_name="box", use std::hash<K>("box")




	// Assets available on DISK
	std::unordered_map<std::string, AssetInfo> modelsOnDisk;
	std::unordered_map<std::string, AssetInfo> texturesOnDisk;
	std::unordered_map<std::string, AssetInfo> fontsOnDisk;
	std::unordered_map<std::string, AssetInfo> shadersOnDisk;
	/// TODO: more assets ?



};