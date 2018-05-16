#include "PCH.hpp"
#include "World.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "Threading.hpp"

// list of vectors of instanaces
// vector.size = 100
// To insert:
//	Find first vector in list with size < 100 and push_back()

void World::addModelInstance(std::string modelName, std::string instanceName)
{
	Engine::threading->addingModelInstanceMutex.lock();
	if (modelNames.find(instanceName) != modelNames.end())
		return; // Instance exists

	int transformIndex = 0; // Dependant on list<vector<>> the instance is in

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

	auto m = Engine::assets.getModel(modelName);
	insertPosition->name = instanceName;
	insertPosition->setModel(m);
	insertPosition->transformIndex = transformIndex;
	modelNames.insert(std::make_pair(instanceName, insertPosition));

	Engine::renderer->gBufferCmdsNeedUpdate = true;

	std::cout << "Adding model instance: " << modelName << std::endl;

	Engine::threading->addingModelInstanceMutex.unlock();

	if (!m->checkAvailability(Asset::ON_RAM) && !m->checkAvailability(Asset::LOADING_TO_RAM))
	{
		if (!m->checkAvailability(Asset::ON_GPU) && !m->checkAvailability(Asset::LOADING_TO_GPU))
		{
			/// TODO: prevent loading the same model more than once
			auto loadJobFunc = std::bind([](Model* m, ModelInstance* mi) -> void {
				m->loadToRAM();
				m->loadToGPU(); /// TODO: asset availability might need to be an atomic
				Engine::threading->totalJobsFinished.fetch_add(1);
				std::cout << "Done adding: " << m->getName() << std::endl;
			}, m, insertPosition);

			m->getAvailability() |= Asset::LOADING_TO_RAM;
			m->getAvailability() |= Asset::LOADING_TO_GPU;
			Engine::threading->addJob(new Job<decltype(loadJobFunc), VoidJobType>(loadJobFunc, []()->void {}));
		}
		else
		{
			// For now models that are in RAM must be on GPU as well
			DBG_SEVERE("Not supported yet"); /// TODO: this
		}
	}

	
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

void World::frustumCulling(Camera * cam)
{
	/// TODO: actual culling, for now we draw everything thats available on the GPU
	/// It would also be good to only perform culling when changes in the scene cross the frustum

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
