#include "PCH.hpp"
#include "AssetStore.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "XMLParser.hpp"
#include "Threading.hpp"
#include "Filesystem.hpp"

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
		/*TextureCreateInfo ci;
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

		auto job = new Job<decltype(texLoadFunc)>(texLoadFunc, JobBase::CPUTransfer);
		job->setChild(new Job<decltype(texToGPUFunc)>(texToGPUFunc, JobBase::GPUTransfer));
		Engine::threading->addDiskIOJob(job);*/
		tex.prepare(paths, name);
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

	// Fonts -------------------------------------------------------------------------
}

void AssetStore::loadDefaultAssets()
{

	/// TODO: Default assets should have READ_ONLY_OPTIMAL layout!

	{
		auto& tex = textures.try_emplace("blank").first->second;
		TextureCreateInfo ci;
		ci.genMipMaps = false;
		ci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		ci.layout = VK_IMAGE_LAYOUT_GENERAL;
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
		auto& tex = textures.try_emplace("blankCube").first->second;
		TextureCreateInfo ci;
		ci.genMipMaps = false;
		ci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		ci.layout = VK_IMAGE_LAYOUT_GENERAL;
		ci.layers = 6;
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
		ci.layout = VK_IMAGE_LAYOUT_GENERAL;
		ci.layers = 1;
		ci.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		ci.width = 2;
		ci.height = 2;
		int blank[4]; memset(blank, 0x00000000, 16);
		ci.pData = blank;
		ci.format = VK_FORMAT_R8G8B8A8_UNORM;

		tex.loadToGPU(&ci);
	}

	{
		addMaterial("null", "blank", "blank");
	}

	{
		auto& model = models.try_emplace("nullModel").first->second;
		model.lodPaths = { "/res/models/nullmodel.obj" };
		model.lodLimits = { 0 };

		model.prepare(model.lodPaths, "nullModel");
		model.material = getMaterial("null");
		model.loadToRAM();
		model.loadToGPU();
	}
}

void AssetStore::gatherAvailableAssets()
{
	std::vector<std::string> assetDirectories = { "res/models_test/", "res/fonts/", "res/textures/", "res/shaders/" };

	typedef std::unordered_map<std::string, AssetInfo> AssetMap;

	auto storeAssetsInfos = [this](AssetMap* destMap, const std::string & directorySource) {
		std::vector<std::string> paths;
		Engine::fs.getFilesInDirectory(directorySource, paths);

		for (auto& path : paths) {
			AssetInfo assetInfo;
			getAssetInfo(path, assetInfo);
			(*destMap)[assetInfo.name] = assetInfo;
		}
	};

	storeAssetsInfos(&modelsOnDisk, "res/models_test/");
	storeAssetsInfos(&texturesOnDisk, "res/textures/");
	storeAssetsInfos(&fontsOnDisk, "res/fonts/");
	storeAssetsInfos(&shadersOnDisk, "res/shaders/");
}

void AssetStore::addMaterial(std::string name, std::string albedoSpec, std::string normalRough)
{
	Engine::threading->addMaterialMutex.lock();

	auto find = materialIndices.find(name);
	if (find != materialIndices.end()) {
		Engine::threading->addMaterialMutex.unlock();
		return;
	}

	u32 matIndex = materials.size();
	materialIndices[name] = matIndex;

	materials.try_emplace(matIndex, name);
	materials[matIndex].data.textures.albedoSpec = getTexture(albedoSpec);
	materials[matIndex].data.textures.normalRough = getTexture(normalRough);
	materials[matIndex].gpuIndexBase = (materials.size() - 1) * 2;

	Engine::threading->addMaterialMutex.unlock();
}

void AssetStore::addMaterial(std::string name, Texture* albedoMetal, Texture* normalRough)
{
	Engine::threading->addMaterialMutex.lock();

	auto find = materialIndices.find(name);
	if (find != materialIndices.end()) {
		Engine::threading->addMaterialMutex.unlock();
		return;
	}

	u32 matIndex = materials.size();
	materialIndices[name] = matIndex;

	materials.try_emplace(matIndex, name);
	materials[matIndex].data.textures.albedoSpec = albedoMetal;
	materials[matIndex].data.textures.normalRough = normalRough;
	materials[matIndex].gpuIndexBase = (materials.size() - 1) * 2;

	Engine::threading->addMaterialMutex.unlock();
}

void AssetStore::addMaterial(std::string name, glm::fvec3 albedo, float roughness, float metal)
{
	Engine::threading->addMaterialMutex.lock();

	auto find = materialIndices.find(name);
	if (find != materialIndices.end()) {
		Engine::threading->addMaterialMutex.unlock();
		return;
	}

	u32 matIndex = materials.size();
	materialIndices[name] = matIndex;

	auto genTexName = name + "_generated_COL=(" + std::to_string(albedo.x) + "," + std::to_string(albedo.y) + "," + std::to_string(albedo.z) + ",R=" + std::to_string(roughness) + "),M=" + std::to_string(metal);

	{
		auto genTexNameAlbedoSpec = genTexName + "_AS";
		auto& tex = textures.try_emplace(genTexNameAlbedoSpec).first->second;
		TextureCreateInfo ci;
		ci.genMipMaps = false;
		ci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		ci.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ci.layers = 1;
		ci.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		ci.width = 1;
		ci.height = 1;
		u8 data[4];
		data[0] = static_cast<u8>(albedo.x * 255.f);
		data[1] = static_cast<u8>(albedo.y * 255.f);
		data[2] = static_cast<u8>(albedo.z * 255.f);
		data[3] = static_cast<u8>(metal * 255.f);
		ci.pData = data;
		ci.format = VK_FORMAT_R8G8B8A8_UNORM;

		tex.loadToGPU(&ci);

		materials[matIndex].data.textures.albedoSpec = &tex;
	}
	{
		auto genTexNameNormalRough = genTexName + "_NR";
		auto& tex = textures.try_emplace(genTexNameNormalRough).first->second;
		TextureCreateInfo ci;
		ci.genMipMaps = false;
		ci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		ci.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ci.layers = 1;
		ci.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		ci.width = 1;
		ci.height = 1;
		u8 data[4];
		data[0] = 128;
		data[1] = 128;
		data[2] = 255;
		data[3] = static_cast<u8>(roughness * 255.f);
		ci.pData = data;
		ci.format = VK_FORMAT_R8G8B8A8_UNORM;

		tex.loadToGPU(&ci);

		materials[matIndex].data.textures.normalRough = &tex;
	}

	materials[matIndex].gpuIndexBase = (materials.size() - 1) * 2;

	Engine::threading->addMaterialMutex.unlock();
}

void AssetStore::getAssetInfo(const std::string& assetPath, AssetInfo& asset)
{
	std::string fileExtension, fileName;
	std::uintmax_t fileSizeInBytes;

	auto itr = assetPath.rbegin();

	for (itr; itr != assetPath.rend(); ++itr) {
		if (*itr == '.') {
			break;
		}
		else {
			fileExtension += *itr;
		}
	}

	std::reverse(fileExtension.begin(), fileExtension.end());

	for (++itr; itr != assetPath.rend(); ++itr) {
		if (*itr == '/' || *itr == '\\') {
			break;
		}
		else {
			fileName += *itr;
		}
	}

	std::reverse(fileName.begin(), fileName.end());

	asset.name = fileName;
	asset.extension = fileExtension;
	asset.sizeInBytes = Engine::fs.getFileSize(assetPath);
	asset.fullPath = Engine::workingDirectory + assetPath;
}
