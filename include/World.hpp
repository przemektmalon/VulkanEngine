#pragma once
#include "PCH.hpp"
#include "Model.hpp"

class World
{
public:

	void addModelInstance(ModelInstance& model, std::string instanceName = "");
	void addModelInstance(std::string modelName, std::string instanceName = "");

	void removeModelInstance(std::string instanceName);

	std::map<std::string, ModelInstance*> modelMap; /// TODO: better hashing for big worlds

	std::list<ModelInstance> models;
};