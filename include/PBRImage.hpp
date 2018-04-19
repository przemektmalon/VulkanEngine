#pragma once
#include "PCH.hpp"
#include "Image.hpp"

class PBRImage
{
public:
	
	void create(Image albedo, Image normal, Image spec, Image rough);

	Image albedoSpec;
	Image normalRough;
	// Ao, height, more ?

};