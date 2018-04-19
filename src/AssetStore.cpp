#include "PCH.hpp"
#include "AssetStore.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

void AssetStore::cleanup()
{
	for (auto t = textures.begin(); t != textures.end(); ++t) {
		t->second.destroy();
	}
}

void AssetStore::loadAssets(std::string assetListFilePath) /// TODO: use xml for this instead (after xml wrappers are done)
{
	std::ifstream file;
	file.open(assetListFilePath.c_str());

	std::string section;

	while (!file.eof())
	{
		std::string line;
		while (std::getline(file, line))
		{
			if (line.length() < 3)
				continue;

			std::string key;
			std::string value;

			auto getUntil = [](std::string& src, char delim)-> std::string {
				auto ret = src.substr(0, src.find(delim) == -1 ? 0 : src.find(delim));
				src.erase(0, src.find(delim) + 1);
				return ret;
			};


			if (line == "[Texture]")
			{
				std::string name, path, mip;
				std::getline(file, key, '=');
				std::getline(file, value);
				path = value;
				std::getline(file, key, '=');
				std::getline(file, value);
				name = value;

				auto find = textures.find(name);
				if (find != textures.end())
				{
					DBG_WARNING("Texture " << name << " already loaded");
					continue;
				}

				auto& tex = textures.insert(std::make_pair(name, Texture())).first->second;
				TextureCreateInfo ci;
				ci.pPaths = &path;
				ci.name = name;
				ci.genMipMaps = true;
				ci.components = 4;
				tex.create(&ci);
			}
			else if (line == "[Model]")
			{
				std::string line;
				std::getline(file, line);
				std::string name, path, limit, albedoSpec, normalRough, material, physics;
				std::vector<std::string> paths;
				std::vector<u32> limits;

				auto place = [&](std::string key, std::string value) -> bool {
					if (key == "path")
					{
						path = value;
						return true;
					}
					else if (key == "name")
					{
						name = value;
						return true;
					}
					else if (key == "limit")
					{
						limit = value;
						return true;
					}
					else if (key == "albedoSpec")
					{
						albedoSpec = value;
						return true;
					}
					else if (key == "normalRough")
					{
						normalRough = value;
						return true;
					}
					else if (key == "material")
					{
						material = value;
						return true;
					}
					else if (key == "physics")
					{
						physics = value;
						return true;
					}
					return false;
				};

				key = getUntil(line, '=');

				int lod = 0;

				while (key != "")
				{
					value = line;
					place(key, value);
					std::getline(file, line);

					if (key == "path")
						paths.push_back(path);
					else if (key == "limit")
						limits.push_back(std::stoi(limit));

					key = getUntil(line, '=');
				}

				auto find = models.find(name);
				if (find != models.end())
				{
					DBG_WARNING("Model " << name << " already loaded");
					continue;
				}

				auto& model = models.insert(std::make_pair(name, Model())).first->second;
				model.physicsInfoFilePath = physics;
				model.lodPaths = paths;
				model.lodLimits = limits;
				model.name = name;
				model.load(path);
				for (auto& triMesh : model.triMeshes)
				{
					for (auto &lodLevel : triMesh)
					{
						if (material.length() != 0)
							lodLevel.material = getMaterial(material);
						else
						{
							lodLevel.material->albedoSpec = getTexture(albedoSpec);
							lodLevel.material->normalRough = getTexture(normalRough);
						}
					}
				}
				Engine::renderer->pushModelDataToGPU(model);
			}
			else if (line == "[Material]")
			{
				std::string line;
				std::getline(file, line);
				std::string name, albedoSpec, normalRough;

				auto place = [&](std::string key, std::string value) -> bool {
					if (key == "name")
					{
						name = value;
						return true;
					}
					else if (key == "albedoSpec")
					{
						albedoSpec = value;
						return true;
					}
					else if (key == "normalRough")
					{
						normalRough = value;
						return true;
					}
					return false;
				};

				key = getUntil(line, '=');

				while (key != "")
				{
					value = line;
					place(key, value);
					std::getline(file, line);

					key = getUntil(line, '=');
				}

				addMaterial(name, albedoSpec, normalRough);
			}
		}
	}
	file.close();
}

void AssetStore::addMaterial(std::string name, std::string albedoSpec, std::string normalRough)
{
	auto find = materialIndices.find(name);
	if (find != materialIndices.end())
		return;

	u32 matIndex = materials.size();
	materialIndices[name] = matIndex;


	materials[matIndex].albedoSpec = getTexture(albedoSpec);
	materials[matIndex].normalRough = getTexture(normalRough);
	materials[matIndex].gpuIndexBase = (materials.size() - 1) * 2;
}