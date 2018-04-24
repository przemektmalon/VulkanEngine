#include "PCH.hpp"
#include "Model.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "Engine.hpp"
#include "Renderer.hpp"

void Model::loadToRAM(void * pCreateStruct, AllocFunc alloc)
{
	for (auto& path : diskPaths)
	{
		Assimp::Importer importer;
		char buff[FILENAME_MAX];
		GetCurrentDir(buff, FILENAME_MAX);
		std::string current_working_dir(buff);
		const aiScene *scene = importer.ReadFile(current_working_dir + "/" + path, aiProcessPreset_TargetRealtime_MaxQuality);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			DBG_SEVERE("ERROR::ASSIMP::" << importer.GetErrorString());
			return;
		}

		aiMesh *mesh = scene->mMeshes[0];

		modelLODs.push_back(TriangleMesh());

		TriangleMesh& triList = modelLODs.back();
		triList.vertexDataLength = mesh->mNumVertices;
		triList.vertexData = new Vertex[triList.vertexDataLength];
		triList.indexDataLength = mesh->mNumFaces * 3;
		triList.indexData = new u32[triList.indexDataLength];

		aiVector3D* pos = mesh->mVertices;
		aiVector3D* nor = mesh->mNormals;
		aiVector3D* uv = mesh->mTextureCoords[0];

		for (u32 k = 0; k < mesh->mNumVertices; ++k)
		{
			aiVector3D p = pos[k];
			aiVector3D n = nor[k];

			triList.vertexData[k].pos = { p.x, p.y, p.z };
			triList.vertexData[k].nor = { n.x, n.y, n.z };
			if (!uv) {
				triList.vertexData[k].texCoord = { 0.f, 0.f };
			}
			else {
				triList.vertexData[k].texCoord = { uv[k].x, 1.0f - uv[k].y };
			}
		}

		u32 curVertex = 0;
		for (u32 i = 0; i < mesh->mNumFaces; ++i)
		{
			aiFace f = mesh->mFaces[i]; //Face

			for (u32 j = 0; j < f.mNumIndices; ++j)
			{
				u32 vi = f.mIndices[j]; //Vertex Index

				triList.indexData[curVertex] = vi;

				++curVertex;
			}
		}
	}

	availability |= ON_RAM;

	DBG_INFO("Model loaded: " << diskPaths[0]);
}

void Model::loadToGPU(void * pCreateStruct)
{
	Engine::renderer->pushModelDataToGPU(*this);
}

void ModelInstance::setModel(Model * m)
{
	material = m->material;
	model = m;
}

void ModelInstance::setMaterial(Material* pMaterial)
{
	material = pMaterial;
}