#include "PCH.hpp"
#include "World.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "Threading.hpp"
#include "Profiler.hpp"

// list of vectors of instanaces
// vector.size = 100
// To insert:
//	Find first vector in list with size < 100 and push_back()

ModelInstance* World::addModelInstance(std::string modelName, std::string instanceName)
{
	PROFILE_MUTEX("modeladdmutex", Engine::threading->addingModelInstanceMutex.lock());
	if (modelNames.find(instanceName) != modelNames.end())
	{
		Engine::threading->addingModelInstanceMutex.unlock();
		return nullptr; // Instance exists
	}

	int transformIndex = allInstances.size() * 100; // Dependant on list<vector<>> the instance is in

	ModelInstance* insertPosition = nullptr;
	for (auto listInsert = allInstances.begin(); listInsert != allInstances.end(); ++listInsert)
	{
		for (auto itr = listInsert->begin(); itr != listInsert->end(); ++itr)
		{
			transformIndex++;
			if (itr->name.length() == 0)
			{
				insertPosition = &(*itr);
				break;
			}
		}
		if (insertPosition)
			break;
	}

	if (!insertPosition)
	{
		allInstances.push_front(std::vector<ModelInstance>(100)); /// TODO: variable size of vector ?
		insertPosition = &allInstances.front().front();
	}

	auto m = Engine::assets.getModel(modelName); /// TODO: error if model doesnt exist !
	insertPosition->name = instanceName;
	insertPosition->setModel(m);
	insertPosition->transformIndex = transformIndex;
	modelNames.insert(std::make_pair(instanceName, insertPosition));

	Engine::renderer->gBufferCmdsNeedUpdate = true;
	Engine::renderer->gBufferNoTexCmdsNeedUpdate = true;

	Engine::threading->addingModelInstanceMutex.unlock();

	if (!m->checkAvailability(Asset::ON_RAM) && !m->checkAvailability(Asset::LOADING_TO_RAM))
	{
		if (!m->checkAvailability(Asset::ON_GPU) && !m->checkAvailability(Asset::LOADING_TO_GPU))
		{
			/// TODO: prevent loading the same model more than once
			auto loadJobFunc = std::bind([](Model* m, ModelInstance* instance) -> void {
				m->loadToRAM();
				instance->setMaterial(m->material);
			}, m, insertPosition);

			auto modelToGPUFunc = std::bind([](Model* m) -> void {
				m->loadToGPU();
			}, m);

			m->getAvailability() |= Asset::LOADING_TO_RAM;
			m->getAvailability() |= Asset::LOADING_TO_GPU;

			auto job = new Job<decltype(loadJobFunc)>(loadJobFunc);
			job->setChild(new Job<decltype(modelToGPUFunc)>(modelToGPUFunc, Engine::threading->m_gpuWorker));
			Engine::threading->addDiskIOJob(job);
		}
		else
		{
			// For now models that are in RAM must be on GPU as well
			DBG_SEVERE("Not supported yet"); /// TODO: this
		}
	}
	return insertPosition;
}

void World::removeModelInstance(std::string instanceName)
{
	/*auto find = modelNames.find(instanceName);
	if (find == modelNames.end())
		return;
	modelNames.erase(find);
	for (auto itr = models.begin(); itr != models.end(); ++itr)
	{
		if (itr->name == instanceName)
		{
			models.erase(itr);
			break;
		}
	}

	Engine::renderer->gBufferCmdsNeedUpdate = true;*/
}

ModelInstance * World::getModelInstance(std::string instanceName)
{
	auto find = modelNames.find(instanceName);
	if (find == modelNames.end())
		return 0;
	else
		return find->second;
}

void World::setSkybox(const std::string & skyboxName)
{
	skybox = Engine::assets.getTexture(skyboxName);

	TextureCreateInfo tci;
	tci.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	tci.format = VK_FORMAT_R8G8B8A8_UNORM;
	tci.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	tci.name = "skybox";
	tci.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	tci.layers = 6;
	tci.genMipMaps = false;

	auto texToRAMFunc = std::bind([](Texture* t, TextureCreateInfo tci) -> void {
		t->loadToRAM(&tci);
	}, skybox, tci);

	auto texToGPUFunc = std::bind([](Texture* t) -> void {
		t->loadToGPU();
		t->getAvailability() |= Asset::AWAITING_DESCRIPTOR_UPDATE;
	}, skybox);

	skybox->getAvailability() |= Asset::LOADING_TO_RAM;
	skybox->getAvailability() |= Asset::LOADING_TO_GPU;

	auto job = new Job<decltype(texToRAMFunc)>(texToRAMFunc);
	job->setChild(new Job<decltype(texToGPUFunc)>(texToGPUFunc, Engine::threading->m_gpuWorker));
	Engine::threading->addDiskIOJob(job);
}

void World::frustumCulling(Camera * cam)
{
	/// TODO: actual culling, for now we draw everything thats available on the GPU
	/// It would also be good to only perform culling when changes in the scene cross the frustum, if that is more perfomant ?

	instancesToDraw.clear();

	for (auto& vecs : allInstances)
	{
		for (auto& instance : vecs)
		{
			if (!instance.model)
				continue;
			if (!instance.model->checkAvailability(Asset::ON_GPU))
				continue;
			instancesToDraw.push_back(&instance);
		}
	}
}
