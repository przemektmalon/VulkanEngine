#include "Model.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "Engine.hpp"
#include "Renderer.hpp"

void Model::load(std::string path)
{
	Assimp::Importer importer;
	char buff[FILENAME_MAX];
	GetCurrentDir(buff, FILENAME_MAX);
	std::string current_working_dir(buff);
	const aiScene *scene = importer.ReadFile(current_working_dir + path, 0);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		DBG_SEVERE("ERROR::ASSIMP::" << importer.GetErrorString());
		return;
	}

	aiMesh *mesh = scene->mMeshes[0];

	triLists.push_back(std::vector<TriangleList>()); triLists[0].push_back(TriangleList());

	TriangleList& triList = triLists[0][0];
	triList.vertexDataLength = mesh->mNumVertices;
	triList.vertexData = new Vertex[triList.vertexDataLength];
	triList.indexDataLength = mesh->mNumFaces * 3;
	triList.indexData = new u32[triList.indexDataLength];

	aiVector3D* pos = mesh->mVertices;
	aiVector3D* uv = mesh->mTextureCoords[0];

	u32 curVertex = 0;
	for (u32 i = 0; i < mesh->mNumFaces; ++i)
	{
		aiFace f = mesh->mFaces[i]; //Face

		for (u32 j = 0; j < f.mNumIndices; ++j)
		{
			u32 vi = f.mIndices[j]; //Vertex Index

			triList.vertexData[curVertex].pos = { pos[vi].x, pos[vi].y, pos[vi].z };
			triList.vertexData[curVertex].col = { 0.f,0.f,0.f };

			if (!uv)
			{
				triList.vertexData[curVertex].texCoord = { 0.f, 0.f };
			}
			else
			{
				triList.vertexData[curVertex].texCoord = { uv[vi].x, 1.0f - uv[vi].y };
			}

			triList.indexData[curVertex] = vi;

			++curVertex;
		}
	}

	DBG_INFO("Model loaded: " << path);



	VkDeviceSize bufferSize = triList.vertexDataLength * sizeof(Vertex);

	Engine::renderer->createVulkanBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, Engine::renderer->vkStagingBuffer, Engine::renderer->vkStagingBufferMemory);

	void* data;
	vkMapMemory(Engine::renderer->vkDevice, Engine::renderer->vkStagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, triList.vertexData, (size_t)bufferSize);
	vkUnmapMemory(Engine::renderer->vkDevice, Engine::renderer->vkStagingBufferMemory);

	Engine::renderer->createVulkanBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, triList.vkVertexBuffer, triList.vkVertexBufferMemory);

	Engine::renderer->copyVulkanBuffer(Engine::renderer->vkStagingBuffer, triList.vkVertexBuffer, bufferSize);

	vkDestroyBuffer(Engine::renderer->vkDevice, Engine::renderer->vkStagingBuffer, 0);
	vkFreeMemory(Engine::renderer->vkDevice, Engine::renderer->vkStagingBufferMemory, 0);


	bufferSize = triList.indexDataLength * sizeof(int);

	Engine::renderer->createVulkanBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, Engine::renderer->vkStagingBuffer, Engine::renderer->vkStagingBufferMemory);

	vkMapMemory(Engine::renderer->vkDevice, Engine::renderer->vkStagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, triList.indexData, (size_t)bufferSize);
	vkUnmapMemory(Engine::renderer->vkDevice, Engine::renderer->vkStagingBufferMemory);

	Engine::renderer->createVulkanBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, triList.vkIndexBuffer, triList.vkIndexBufferMemory);

	Engine::renderer->copyVulkanBuffer(Engine::renderer->vkStagingBuffer, triList.vkIndexBuffer, bufferSize);

	vkDestroyBuffer(Engine::renderer->vkDevice, Engine::renderer->vkStagingBuffer, 0);
	vkFreeMemory(Engine::renderer->vkDevice, Engine::renderer->vkStagingBufferMemory, 0);
}