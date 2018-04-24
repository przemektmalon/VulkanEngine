#pragma once
#include "PCH.hpp"
#include "Material.hpp"
#include "Asset.hpp"

struct Vertex
{
	glm::fvec3 pos;
	glm::fvec3 nor;
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
		attributeDescriptions[1].offset = offsetof(Vertex, nor);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

	static std::array<VkVertexInputAttributeDescription, 1> getShadowAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions = {};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		return attributeDescriptions;
	}
};

struct Vertex2D
{
	glm::fvec2 pos;
	glm::fvec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bd = {};
		bd.binding = 0;
		bd.stride = sizeof(Vertex2D);
		bd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bd;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex2D, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex2D, texCoord);

		return attributeDescriptions;
	}
};

struct ModelCreateInfo
{
	std::string name;
	std::vector<std::string> lodPaths;
	std::vector<u32> lodLimits;
	std::string physicsInfoFilePath;
};

class Model : public Asset
{
public:

	struct TriangleMesh
	{
		Vertex* vertexData;
		u32 vertexDataLength;
		
		u32* indexData;
		u32 indexDataLength;

		s32 firstVertex; // In GPU buffer
		u32 firstIndex; // In GPU buffer

		::Material* material;
	};

	std::string name;

	std::vector<std::vector<TriangleMesh>> triMeshes;
	std::vector<std::string> lodPaths;
	std::vector<u32> lodLimits;
	std::string physicsInfoFilePath;

	void loadToRAM(void* pCreateStruct = 0, AllocFunc alloc = malloc);
	void loadToGPU(void* pCreateStruct = 0);
};

class ModelInstance
{
public:
	std::string name;
	glm::fmat4 transform;
	Model * model;
	u32 transformIndex; // Index into gpu transform buffer

	std::vector<std::vector<Material*>> material;

	void setModel(Model* m);

	void setMaterial(int lodLevel, int meshIndex, Material* pMaterial);
};
