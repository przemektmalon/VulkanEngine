#include "PCH.hpp"
#include "World.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

void World::addModelInstance(std::string modelName, std::string instanceName)
{
	if (modelMap.find(instanceName) != modelMap.end())
		return;
	ModelInstance ins;
	auto m = Engine::assets.getModel(modelName);
	if (!m->isAvailable(Asset::ON_RAM))
		m->loadToRAM();
	if (!m->isAvailable(Asset::ON_GPU))
		m->loadToGPU();
	ins.setModel(m);
	if (instanceName.length() == 0)
		instanceName = modelName;
	ins.name = instanceName;
	ins.transformIndex = models.size();
	models.push_back(ins);
	modelMap.insert(std::make_pair(ins.name, &models.back()));
	Engine::renderer->gBufferCmdsNeedUpdate = true;
}

void World::removeModelInstance(std::string instanceName)
{
	auto find = modelMap.find(instanceName);
	if (find == modelMap.end())
		return;
	modelMap.erase(find);
	for (auto itr = models.begin(); itr != models.end(); ++itr)
	{
		if (itr->name == instanceName)
		{
			models.erase(itr);
			break;
		}
	}

	Engine::renderer->gBufferCmdsNeedUpdate = true;
}

ModelInstance * World::getModelInstance(std::string instanceName)
{
	auto find = modelMap.find(instanceName);
	if (find == modelMap.end())
		return 0;
	else
		return find->second;
}
