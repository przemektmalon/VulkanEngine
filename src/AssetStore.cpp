#include "AssetStore.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

void AssetStore::cleanup()
{
	for (auto t = textures.begin(); t != textures.end(); ++t) {
		t->second.destroy();
	}
}

void AssetStore::loadAssets(std::string assetListFilePath)
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

				auto& tex = textures.insert(std::make_pair(name, Texture(path))).first->second;
				tex.setName(name);
				DBG_INFO("Loaded texture: " << path);
			}
			else if (line == "[Model]")
			{
				std::string line;
				std::getline(file, line);
				std::string name, path, limit, albedo, normal, specular, metallic, roughness, material, physics;
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
					else if (key == "albedo")
					{
						albedo = value;
						return true;
					}
					else if (key == "normal")
					{
						normal = value;
						return true;
					}
					else if (key == "specular")
					{
						specular = value;
						return true;
					}
					else if (key == "metallic")
					{
						metallic = value;
						return true;
					}
					else if (key == "roughness")
					{
						roughness = value;
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
				model.load(path);
				for (auto& triMesh : model.triMeshes)
				{
					for (auto &lodLevel : triMesh)
					{
						if (material.length() != 0)
							lodLevel.material = *getMaterial(material);
						else
						{
							lodLevel.material.albedo = getTexture(albedo);
							lodLevel.material.normal = getTexture(normal);
							if (specular.length() != 0)
								lodLevel.material.specularMetallic = getTexture(specular);
							else
								lodLevel.material.specularMetallic = getTexture(metallic);

							lodLevel.material.roughness = getTexture(roughness);
						}

						/// TODO: AO texture
					}
				}
				Engine::renderer->pushModelDataToGPU(model);
			}
			else if (line == "[Material]")
			{
				std::string line;
				std::getline(file, line);
				std::string name, albedo, normal, specMetal, roughness, ao, height;

				auto place = [&](std::string key, std::string value) -> bool {
					if (key == "name")
					{
						name = value;
						return true;
					}
					else if (key == "albedo")
					{
						albedo = value;
						return true;
					}
					else if (key == "normal")
					{
						normal = value;
						return true;
					}
					else if (key == "specular")
					{
						specMetal = value;
						return true;
					}
					else if (key == "metallic")
					{
						specMetal = value;
						return true;
					}
					else if (key == "roughness")
					{
						roughness = value;
						return true;
					}
					else if (key == "ao")
					{
						ao = value;
						return true;
					}
					else if (key == "height")
					{
						height = value;
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

				addMaterial(name, albedo, normal, specMetal, roughness, ao, height);
			}
		}
	}
	file.close();
}

void AssetStore::addMaterial(std::string name, std::string albedo, std::string normal, std::string specMetal, std::string roughness, std::string ao, std::string height)
{
	auto find = materials.find(name);
	if (find != materials.end())
		return;

	materials[name].albedo = getTexture(albedo);
	materials[name].normal = getTexture(normal);
	materials[name].specularMetallic = getTexture(specMetal);
	materials[name].roughness = getTexture(roughness);
	materials[name].ao = getTexture(ao);
	materials[name].height = getTexture(height);
}