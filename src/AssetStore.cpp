#include "PCH.hpp"
#include "AssetStore.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "XMLParser.hpp"

void AssetStore::cleanup()
{
	for (auto t = textures.begin(); t != textures.end(); ++t) {
		t->second.destroy();
	}
}

void AssetStore::loadAssets(std::string assetListFilePath)
{
	XML xml;
	xml.parseFileFast(assetListFilePath);

	auto root = xml.doc.first_node("resources");
	auto resFolder = xml.getString(xml.firstNode(root, "resfolder"));
	auto texFolder = resFolder + xml.getString(xml.firstNode(root, "texfolder"));
	auto modelFolder = resFolder + xml.getString(xml.firstNode(root, "modelfolder"));

	// Textures ---------------------------------------------------------------------
	
	auto texNode = xml.firstNode(root, "texture");
	while (texNode)
	{
		auto path = xml.getString(xml.firstNode(texNode, "path"));
		auto name = xml.getString(xml.firstNode(texNode, "name"));

		if (path.length() == 0)
			continue;
		if (path[0] == '_')
			path = texFolder + (path.c_str() + 1);

		texNode = texNode->next_sibling("texture");

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

	// Textures ---------------------------------------------------------------------

	// Materials --------------------------------------------------------------------

	auto matNode = xml.firstNode(root, "material");
	while (matNode)
	{
		auto name = xml.getString(xml.firstNode(matNode, "name"));
		auto albSpec = xml.getString(xml.firstNode(matNode, "albedospec"));
		auto norRough = xml.getString(xml.firstNode(matNode, "normalrough"));

		addMaterial(name, albSpec, norRough);

		matNode = matNode->next_sibling("material");
	}

	// Materials --------------------------------------------------------------------

	// Models -----------------------------------------------------------------------

	auto modelNode = xml.firstNode(root, "model");
	while (modelNode)
	{
		auto name = xml.getString(xml.firstNode(modelNode, "name"));
		auto material = xml.getString(xml.firstNode(modelNode, "material"));
		auto physics = xml.getString(xml.firstNode(modelNode, "physics"));

		std::vector<std::string> lodPaths;
		std::vector<u32> lodLimits;

		auto pathNode = xml.firstNode(modelNode, "path");
		while (pathNode)
		{
			std::string path;
			path = xml.getString(pathNode);
			if (path.length() == 0)
				continue;
			if (path[0] == '_')
				path = modelFolder + (path.c_str() + 1);
			lodPaths.push_back(path);
			pathNode = pathNode->next_sibling("path");
		}

		auto lodNode = xml.firstNode(modelNode, "limit");
		while (lodNode)
		{
			lodLimits.push_back(std::stoi(xml.getString(lodNode)));
			lodNode = lodNode->next_sibling("limit");
		}

		modelNode = modelNode->next_sibling("model");

		auto find = models.find(name);
		if (find != models.end())
		{
			DBG_WARNING("Model " << name << " already loaded");
			continue;
		}

		auto& model = models.insert(std::make_pair(name, Model())).first->second;
		model.physicsInfoFilePath = physics;
		model.lodPaths = lodPaths;
		model.lodLimits = lodLimits;
		model.name = name;
		model.load();
		for (auto& triMesh : model.triMeshes)
		{
			for (auto &lodLevel : triMesh)
			{
				lodLevel.material = getMaterial(material);
			}
		}
		Engine::renderer->pushModelDataToGPU(model); /// TODO: load only whats needed to GPU
	}

	// Models -----------------------------------------------------------------------
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