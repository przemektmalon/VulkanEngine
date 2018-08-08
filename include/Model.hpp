#pragma once
#include "PCH.hpp"
#include "Material.hpp"
#include "Asset.hpp"
#include "PhysicsObject.hpp"
#include "Transform.hpp"

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

struct VertexNoNormal
{
	glm::fvec3 pos;
	glm::fvec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bd = {};
		bd.binding = 0;
		bd.stride = sizeof(VertexNoNormal);
		bd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bd;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(VertexNoNormal, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(VertexNoNormal, texCoord);

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

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};
		attributeDescriptions.resize(2);

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
	};

	std::vector<TriangleMesh> modelLODs;
	std::vector<std::string> lodPaths;
	std::vector<u32> lodLimits;
	std::string physicsInfoFilePath;

	::Material* material;

	void loadToRAM(void* pCreateStruct = 0, AllocFunc alloc = malloc);
	void loadToGPU(void* pCreateStruct = 0);
};

class ModelInstance
{
public:
	void setTransform(Transform& t)
	{
		transform[0] = t;
		transform[1] = t;
	}

	std::string name;
	Transform transform[2];

	static u32 toEngineTransformIndex;
	static u32 toGPUTransformIndex;

	u32 transformIndex; // Index into gpu transform buffer
	Model * model;
	Material* material;
	PhysicsObject* physicsObject;

	void setModel(Model* m);
	void setMaterial(Material* pMaterial);
	Transform& getTransform() { return transform[0]; }

	//void makePhysicsObject(btCollisionShape* collisionShape, float mass);
	void makePhysicsObject();
};
