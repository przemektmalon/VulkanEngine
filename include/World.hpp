#pragma once
#include "PCH.hpp"
#include "Model.hpp"

class World
{
public:

	void addModelInstance(ModelInstance& model);
	std::vector<ModelInstance> models;
};