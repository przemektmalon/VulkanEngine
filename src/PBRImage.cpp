#include "PCH.hpp"
#include "PBRImage.hpp"

void PBRImage::create(Image albedo, Image normal, Image spec, Image rough)
{
	albedoSpec.setSize(albedo.width, albedo.height, 4);
	normalRough.setSize(albedo.width, albedo.height, 4);

	glm::u8vec4* albedoPixels = (glm::u8vec4*)albedo.data.data();
	glm::u8vec4* normalPixels = (glm::u8vec4*)normal.data.data();
	glm::u8vec4* specPixels = (glm::u8vec4*)spec.data.data();
	glm::u8vec4* roughPixels = (glm::u8vec4*)rough.data.data();

	for (int i = 0; i < albedoSpec.data.size(); i+=4)
	{ 
		albedoSpec.data[i] = albedoPixels[i / 4].r;
		albedoSpec.data[i + 1] = albedoPixels[i / 4].g;
		albedoSpec.data[i + 2] = albedoPixels[i / 4].b;
		albedoSpec.data[i + 3] = specPixels[i / 4].r;

		normalRough.data[i] = normalPixels[i / 4].r;
		normalRough.data[i + 1] = normalPixels[i / 4].g;
		normalRough.data[i + 2] = normalPixels[i / 4].b;
		normalRough.data[i + 3] = roughPixels[i / 4].r;
	}
}
