#pragma once
#include "PCH.hpp"
#include "Model.hpp"

class World
{
public:

	void addModelInstance(std::string modelName, std::string instanceName);
	void removeModelInstance(std::string instanceName);
	ModelInstance* getModelInstance(std::string instanceName);

	std::map<std::string, ModelInstance*> modelMap; /// TODO: better hashing for big worlds

	std::list<ModelInstance> models;
};