#include "PCH.hpp"
#include "Lights.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"

void PointLight::updateRadius()
{
	matNeedsUpdate = true;
	gpuData->setRadius(calculateRadius(gpuData->getLinear(), gpuData->getQuadratic()));
	proj = glm::perspective<float>(glm::radians(90.f), 1.f, 0.01f, gpuData->getRadius());
}

inline float PointLight::calculateRadius(float linear, float quad)
{
	return (0.5 * (std::sqrt(linear*linear + (5000 * quad) - linear))) / quad;
}

void SpotLight::updateRadius()
{
	matNeedsUpdate = true;
	gpuData->setRadius(calculateRadius(gpuData->getLinear(), gpuData->getQuadratic()));
}

inline float SpotLight::calculateRadius(float linear, float quad)
{
	return (0.5 * (std::sqrt(linear*linear + (5000 * quad) - linear))) / (quad * 2.f);
}

PointLight & LightManager::addPointLight(PointLight::GPUData& data)
{
	pointLights.push_back(PointLight());
	auto& light = pointLights.back();
	pointLightsGPUData.push_back(data);
	light.gpuData = &pointLightsGPUData.back();

	return light;
}

SpotLight & LightManager::addSpotLight(SpotLight::GPUData& data)
{
	spotLights.push_back(SpotLight());
	auto& light = spotLights.back();
	spotLightsGPUData.push_back(data);
	light.gpuData = &spotLightsGPUData.back();

	return light;
}