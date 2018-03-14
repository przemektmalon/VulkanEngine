#pragma once
#include "PCH.hpp"
#include "Material.hpp"

struct Vertex
{
	glm::fvec3 pos;
	glm::fvec3 col;
	glm::fvec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bd = {};
		bd.binding = 0;
		bd.stride = sizeof(Vertex);
		bd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bd;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, col);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}
};

class Model
{
public:
	Model() {}
	Model(std::string path) { load(path); }

	struct TriangleMesh
	{
		Vertex* vertexData;
		u32 vertexDataLength;
		
		u32* indexData;
		u32 indexDataLength;

		s32 firstVertex; // In GPU buffer
		u32 firstIndex; // In GPU buffer

		Material material;
	};

	std::vector<std::vector<TriangleMesh>> triMeshes;
	std::vector<std::string> lodPaths;
	std::vector<u32> lodLimits;
	std::string physicsInfoFilePath;

	void load(std::string path);
};

class ModelInstance
{
public:
	Model * model;
	glm::fmat4 transform;
	Material material;

	std::vector<std::vector<Material>> materialOverwrite;

	void setModel(Model* m);

	void setMaterial(int lodLevel, int meshIndex, Material& material);
};
