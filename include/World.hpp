#pragma once
#include "PCH.hpp"
#include "Model.hpp"
#include "Camera.hpp"

class World
{
public:

	void addModelInstance(std::string modelName, std::string instanceName);
	void removeModelInstance(std::string instanceName);
	ModelInstance* getModelInstance(std::string instanceName);

	void frustumCulling(Camera* cam);

	std::unordered_map<std::string, ModelInstance*> modelNames; /// TODO: better hashing for big worlds

	// Stores all instances in world
	std::list<std::vector<ModelInstance>> allInstances;

	// Culled instances to draw (from allInstances)
	std::vector<ModelInstance*> instancesToDraw;

	// Instances waiting to be added. Might be waiting for load from DISK. Add to allInstances when ready
	//std::vector<ModelInstance> instancesToAdd;
};