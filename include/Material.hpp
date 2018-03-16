#pragma once
#include "PCH.hpp"
#include "Texture.hpp"

struct Material
{
	u32 gpuIndexBase;
	Texture *albedo, *normal, *specularMetallic, *roughness, *ao, *height;

	Material& operator=(Material& rhs)
	{
		albedo = rhs.albedo;
		normal = rhs.normal;
		specularMetallic = rhs.specularMetallic;
		roughness = rhs.roughness;
		ao = rhs.ao;
		height = rhs.height;

		return *this;
	}
};