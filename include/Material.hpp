#pragma once
#include "PCH.hpp"
#include "Texture.hpp"

struct Material
{
	bool splatted;
	Texture *albedo, *normal, *specularMetallic, *roughness, *ao, *height;

	Material() : splatted(false) {}

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