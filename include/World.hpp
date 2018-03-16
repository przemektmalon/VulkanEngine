#pragma once
#include "PCH.hpp"
#include "Model.hpp"

class World
{
public:

	void addModelInstance(ModelInstance& model, std::string instanceName = "");
	void addModelInstance(std::string modelName, std::string instanceName = "");

	std::map<std::string, ModelInstance*> modelMap; /// TODO: hashing for big worlds

	std::vector<ModelInstance> models;
};