#include "PCH.hpp"
#include "AssetStore.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "XMLParser.hpp"
#include "Threading.hpp"

void AssetStore::cleanup()
{
	for (auto t = textures.begin(); t != textures.end(); ++t) {
		t->second.destroy();
	}
	for (auto& f : fonts)
		f.second.cleanupGPU();
}

void AssetStore::loadAssets(std::string assetListFilePath)
{
	XML xml;
	xml.parseFileFast(assetListFilePath);

	auto root = xml.doc.first_node("resources");
	auto resFolder = xml.getString(xml.firstNode(root, "resfolder"));
	auto texFolder = resFolder + xml.getString(xml.firstNode(root, "texfolder"));
	auto modelFolder = resFolder + xml.getString(xml.firstNode(root, "modelfolder"));
	auto fontFolder = resFolder + xml.getString(xml.firstNode(root, "fontfolder"));

	// Textures ---------------------------------------------------------------------
	
	auto texNode = xml.firstNode(root, "texture");
	while (texNode)
	{
		std::vector<std::string> paths;
		auto name = xml.getString(xml.firstNode(texNode, "name"));
		bool mip = true;

		auto pathNode = xml.firstNode(texNode, "path");
		while (pathNode)
		{
			auto path = xml.getString(pathNode);
			if (path[0] == '_')
				path = texFolder + (path.c_str() + 1);

			paths.push_back(path);

			pathNode = pathNode->next_sibling("path");
		}

		auto mipNode = xml.firstNode(texNode, "mip");
		if (mipNode) {
			if (xml.getString(mipNode) == "false") {
				mip = false;
			}
		}

		texNode = texNode->next_sibling("texture");

		auto find = textures.find(name);
		if (find != textures.end())
		{
			DBG_WARNING("Texture named \"" << name << "\" already loaded (duplicate)");
			continue;
		}

		auto& tex = textures.try_emplace(name).first->second;
		TextureCreateInfo ci;
		ci.genMipMaps = mip;
		ci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		ci.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ci.layers = paths.size();
		ci.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		ci.format = VK_FORMAT_R8G8B8A8_UNORM;

		auto texLoadFunc = std::bind([](Texture* tex, TextureCreateInfo ci) -> void {
			tex->loadToRAM(&ci);
		},&tex,ci);

		auto texToGPUFunc = std::bind([](Texture* tex) -> void {
			tex->loadToGPU();
		}, &tex);

		tex.prepare(paths, name);
		auto job = new Job<decltype(texLoadFunc)>(texLoadFunc, defaultJobDoneFunc);
		job->setChild(new Job<decltype(texToGPUFunc)>(texToGPUFunc, defaultTransferJobDoneFunc, JobBase::Type::Transfer));
		Engine::threading->addJob(job);
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
		if (physics.length() != 0)
			if (physics[0] == '_')
				physics = modelFolder + (physics.c_str() + 1);

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

		auto& model = models.try_emplace(name).first->second;
		model.physicsInfoFilePath = physics;
		model.lodPaths = lodPaths;
		model.lodLimits = lodLimits;

		model.prepare(lodPaths, name);
		model.material = getMaterial(material);
	}

	// Models -----------------------------------------------------------------------

	// Fonts ------------------------------------------------------------------------

	FT_Init_FreeType(&freetype);

	auto fontNode = xml.firstNode(root, "font");
	while (fontNode)
	{
		auto name = xml.getString(xml.firstNode(fontNode, "name"));
		auto path = xml.getString(xml.firstNode(fontNode, "path"));

		if (path[0] == '_')
			path = fontFolder + (path.c_str() + 1);

		std::vector<std::string> paths; paths.push_back(path);

		auto& font = fonts.try_emplace(name).first->second;
		font.prepare(paths, name);
		font.loadToRAM();

		fontNode = fontNode->next_sibling("font");
	}

	// Fonts ------------------------------------------------------------------------
}

void AssetStore::loadDefaultAssets()
{
	{
		auto& tex = textures.try_emplace("blank").first->second;
		TextureCreateInfo ci;
		ci.genMipMaps = false;
		ci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		ci.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ci.layers = 1;
		ci.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		ci.width = 2;
		ci.height = 2;
		int blank[4]; memset(blank, 0xFFFFFFFF, 16);
		ci.pData = blank;
		ci.format = VK_FORMAT_R8G8B8A8_UNORM;

		tex.loadToGPU(&ci);
	}

	{
		auto& tex = textures.try_emplace("black").first->second;
		TextureCreateInfo ci;
		ci.genMipMaps = false;
		ci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		ci.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ci.layers = 1;
		ci.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		ci.width = 2;
		ci.height = 2;
		int blank[4]; memset(blank, 0x00000000, 16);
		ci.pData = blank;
		ci.format = VK_FORMAT_R8G8B8A8_UNORM;

		tex.loadToGPU(&ci);
	}
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