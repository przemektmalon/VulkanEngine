#include "PCH.hpp"
#include "Model.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "rapidxml.hpp"
#include "File.hpp"

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
	availability |= ON_GPU;
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

void ModelInstance::makePhysicsObject(btCollisionShape * collisionShape, float mass)
{
	physicsObject = new PhysicsObject();
	physicsObject->instance = this;
	physicsObject->create(transform.getTranslation(), transform.getQuat(), collisionShape, mass);
	Engine::physicsWorld.addRigidBody(physicsObject);
}

void ModelInstance::makePhysicsObject()
{
	btCollisionShape* colShape = nullptr;
	float mass = 10.0, friction = 0.5, restitution = 0.2, linearDamping = 0.05, angularDamping = 0.05;

	auto nodeToFloat = [](rapidxml::xml_node<>* node) -> float {
		if (node)
			return std::stof(std::string(node->value()));
		else
			return 0.f; /// TODO: more appropriate default values and logging missing values
	};

	auto generateCollisionShape = [&nodeToFloat, this](rapidxml::xml_node<>* shapeNode) -> btCollisionShape* {
		std::string shapeType = shapeNode->first_attribute("type")->value();

		glm::fvec3 scale = this->transform.getScale();

		if (shapeType == "sphere")
		{
			float radius = nodeToFloat(shapeNode->first_node("radius")) * this->transform.getScale().x;
			return new btSphereShape(radius);
		}
		else if (shapeType == "box")
		{
			float halfx = nodeToFloat(shapeNode->first_node("halfx"));
			float halfy = nodeToFloat(shapeNode->first_node("halfy"));
			float halfz = nodeToFloat(shapeNode->first_node("halfz"));
			return new btBoxShape(btVector3(halfx * scale.x, halfy * scale.y, halfz * scale.z));
		}
		else if (shapeType == "plane")
		{
			float nx = nodeToFloat(shapeNode->first_node("nx"));
			float ny = nodeToFloat(shapeNode->first_node("ny"));
			float nz = nodeToFloat(shapeNode->first_node("nz"));
			float k = nodeToFloat(shapeNode->first_node("k"));
			auto trans = btTransform(btMatrix3x3(1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f));

			return new btStaticPlaneShape(btVector3(nx, ny, nz), k);
		}
		else
		{
			/// TODO: more collision shapes
		}
	};

	if (model->physicsInfoFilePath.length())
	{
		File physicsInfoFile;
		physicsInfoFile.open(model->physicsInfoFilePath, File::in);

		std::string xmlString;

		physicsInfoFile.fstream().seekg(0, std::ios::end);
		xmlString.reserve(physicsInfoFile.fstream().tellg());
		physicsInfoFile.fstream().seekg(0, std::ios::beg);

		xmlString.assign((std::istreambuf_iterator<char>(physicsInfoFile.fstream())), std::istreambuf_iterator<char>());

		physicsInfoFile.close();

		rapidxml::xml_document<> doc;
		doc.parse<0>(const_cast<char*>(xmlString.c_str()));

		rapidxml::xml_node<>* root = doc.first_node("physics");

		mass = nodeToFloat(root->first_node("mass"));
		friction = nodeToFloat(root->first_node("friction"));
		restitution = nodeToFloat(root->first_node("restitution"));
		linearDamping = nodeToFloat(root->first_node("lineardamping"));
		angularDamping = nodeToFloat(root->first_node("angulardamping"));

		std::string compound = std::string(root->first_node("compound")->value());
		if (compound == "true")
		{
			colShape = new btCompoundShape();
			btCompoundShape* compound = (btCompoundShape*)colShape;
			rapidxml::xml_node<>* shapeNode = root->first_node("shape");
			while (shapeNode)
			{
				auto shape = generateCollisionShape(shapeNode);

				btQuaternion rotation(0, 0, 0);
				btVector3 offset(0, 0, 0);

				if (shapeNode->first_node("posx"))
				{
					float posx, posy, posz;
					auto scale = transform.getScale();
					posx = nodeToFloat(shapeNode->first_node("posx"));
					posy = nodeToFloat(shapeNode->first_node("posy"));
					posz = nodeToFloat(shapeNode->first_node("posz"));
					offset = btVector3(posx * scale.x, posy * scale.y, posz * scale.z);
				}

				if (shapeNode->first_node("rotx"))
				{
					/// TODO: get rotation values
				}

				btTransform transform(rotation, offset);

				compound->addChildShape(transform, shape);
				shapeNode = shapeNode->next_sibling();
			}
		}
		else
		{
			colShape = generateCollisionShape(root->first_node("shape"));
		}
	}
	else
	{
		colShape = new btBoxShape(btVector3(10, 10, 10));
	}

	physicsObject = new PhysicsObject();
	physicsObject->instance = this;
	physicsObject->create(transform.getTranslation(), transform.getQuat(), colShape, mass);
	physicsObject->setDamping(linearDamping, angularDamping);
	physicsObject->setFriction(friction);
	physicsObject->setRestitution(restitution);
	physicsObject->rigidBody->forceActivationState(DISABLE_DEACTIVATION);
	Engine::physicsWorld.addRigidBody(physicsObject);
}