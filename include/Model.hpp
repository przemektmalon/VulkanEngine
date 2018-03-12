#pragma once
#include "PCH.hpp"

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

	struct TriangleList
	{
		Vertex* vertexData;
		u32 vertexDataLength;
		
		u32* indexData;
		u32 indexDataLength;

		VkBuffer vkVertexBuffer;
		VkDeviceMemory vkVertexBufferMemory;

		VkBuffer vkIndexBuffer;
		VkDeviceMemory vkIndexBufferMemory;
	};

	std::vector<std::vector<TriangleList>> triLists;

	void load(std::string path);
};