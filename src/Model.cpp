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

	triMeshes.push_back(std::vector<TriangleMesh>()); triMeshes[0].push_back(TriangleMesh());

	TriangleMesh& triList = triMeshes[0][0];
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
}