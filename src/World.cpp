#include "PCH.hpp"
#include "World.hpp"
#include "Engine.hpp"

void World::addModelInstance(ModelInstance& model, std::string instanceName)
{
	model.transformIndex = models.size();
	models.push_back(model);
	modelMap.insert(std::make_pair(model.name, &models.back()));
}

void World::addModelInstance(std::string modelName, std::string instanceName)
{
	ModelInstance ins;
	ins.setModel(Engine::assets.getModel(modelName));
	if (instanceName.length() == 0)
		instanceName = modelName;
	ins.name = instanceName;
	addModelInstance(ins, instanceName);
}
