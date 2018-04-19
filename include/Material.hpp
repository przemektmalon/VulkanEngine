#pragma once
#include "PCH.hpp"
#include "Texture.hpp"

struct Material
{
	u32 gpuIndexBase;
	Texture *albedoSpec, *normalRough;

	Material& operator=(Material& rhs)
	{
		albedoSpec = rhs.albedoSpec;
		normalRough = rhs.normalRough;
		
		return *this;
	}
};