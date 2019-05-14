#include "PCH.hpp"
#include "Model.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "rapidxml.hpp"
#include "File.hpp"
#include "Threading.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

void Model::loadToRAM(void * pCreateStruct, AllocFunc alloc)
{
	availability |= LOADING_TO_RAM;

	//Engine::console->postMessage("Loading model: " + diskPaths[0], glm::fvec3(0.8, 0.8, 0.3));

	/// TODO:			since textures are stored in binary glb files, having separate model LOD files will use more space than necessary
	/// Possible fix:	have multiple meshes in the glb file, each LOD, then textures will not be stored more than once
	/// ISSUE:			how do we define LOD distance limits ? Can we store meta-data in glb files ?
	/// ISSUE:			if we have different models that use the same textures, we don't want to store the textures in all the glb files
	/// Possible fix:	for textures used in different models, don't store texture in glb, but somehow reference it. Needs more though!
	/// ISSUE:			how do we define physics data ? again, does glTF/glb provide meta-data ? 
	///					or do we have to manually appoint a physics model/primitive since we know what model we're loading ?

	// For each LOD level of a model
	for (auto& path : diskPaths)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(Engine::workingDirectory + "/" + path, aiProcessPreset_TargetRealtime_MaxQuality);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			DBG_SEVERE("ERROR::ASSIMP::" << importer.GetErrorString());
			return;
		}

		auto numTextures = scene->mNumTextures;

		// Only load textures if the model file has them embedded
		if (numTextures != 0) {

			// The order of textures for model import seems to be:
			//	[0] = Albedo
			//	[1] = PBR
			//	[2] = Normal

			//	All of these have 3 channels RGB

			// A Material, however, expects TWO textures :
			//												1) vec4(albedo,metal)
			//												2) vec4(normal,rough)

			// So we need to combine the three given textures into two (4 channel textures), to create materials


			// First load and decompress all the embedded textures
			/// TODO: validate all images are same size ? With the current setup this has to be the case
			Image* texImage = new Image[numTextures];

			for (int i = 0; i < numTextures; ++i) {
				auto tex = scene->mTextures[i];
				if (tex->mHeight == 0) {
					auto data = tex->pcData;
					texImage[i].load(data, tex->mWidth);
					if (texImage[i].components != 3) {
						DBG_SEVERE("For now only 3 channel textures are supported as inputs in model file embeddings");
					}
				}
				else {
					DBG_SEVERE("Embedded model textures should be PNG compressed");
				}
			}

			// Now that we have three 3-component images, combine them into two 4-component images

			Image* albedoMetal = new Image, *normalRough = new Image;

			u32 texWidth = texImage[0].width, texHeight = texImage[0].height;

			albedoMetal->setSize(texWidth, texHeight, 4);
			normalRough->setSize(texWidth, texHeight, 4);

			glm::u8vec3* albedoPixels = (glm::u8vec3*)texImage[0].data.data();
			glm::u8vec3* pbrPixels = (glm::u8vec3*)texImage[1].data.data();
			glm::u8vec3* normalPixels = (glm::u8vec3*)texImage[2].data.data();
			

			int sourceIndex = 0, destIndex = 0;

			/// TODO: performance improvement -> parallelise ??

			for (destIndex; destIndex < albedoMetal->data.size(); destIndex += 4)
			{
				albedoMetal->data[destIndex] = albedoPixels[sourceIndex / 3].r;
				albedoMetal->data[destIndex + 1] = albedoPixels[sourceIndex / 3].g;
				albedoMetal->data[destIndex + 2] = albedoPixels[sourceIndex / 3].b;
				albedoMetal->data[destIndex + 3] = pbrPixels[sourceIndex / 3].b;

				normalRough->data[destIndex] = normalPixels[sourceIndex / 3].r;
				normalRough->data[destIndex + 1] = normalPixels[sourceIndex / 3].g;
				normalRough->data[destIndex + 2] = normalPixels[sourceIndex / 3].b;
				normalRough->data[destIndex + 3] = pbrPixels[sourceIndex / 3].g;

				sourceIndex += 3;
			}

			// Now that we have two Images we can create two Textures

			auto& albedoMetalTexture = Engine::assets.textures.try_emplace(name + "_albedo_metal").first->second;
			auto& normalRoughTexture = Engine::assets.textures.try_emplace(name + "_normal_rough").first->second;

			TextureCreateInfo tci = TextureCreateInfo(albedoMetal);

			tci.format = VK_FORMAT_R8G8B8A8_UNORM;
			tci.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			tci.usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			tci.genMipMaps = true;

			albedoMetalTexture.loadToRAM(&tci);

			tci.image = normalRough;

			normalRoughTexture.loadToRAM(&tci);

			// Now that we have two Textures loaded in RAM we can create a material

			Engine::assets.addMaterial(name + "_material", &albedoMetalTexture, &normalRoughTexture);

			material = Engine::assets.getMaterial(name + "_material");

		}
		modelLODs.push_back(TriangleMesh());

		aiMesh* mesh = scene->mMeshes[0];
		auto rootNode = scene->mRootNode;
		auto meshTransform = rootNode->mTransformation;
		glm::fmat4 glmMeshTransform;
		memcpy(&glmMeshTransform, &meshTransform, sizeof(glm::fmat4));

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

		/// TODO: performance improvement -> parallelise ??

		for (u32 k = 0; k < mesh->mNumVertices; ++k)
		{
			triList.vertexData[k].pos = glm::fvec3(glmMeshTransform * glm::fvec4(triList.vertexData[k].pos, 1));
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
	availability &= ~LOADING_TO_RAM;

	material->loadToRAM();
}

void Model::loadToGPU(void * pCreateStruct)
{
	availability |= LOADING_TO_GPU;
	Engine::threading->pushingModelToGPUMutex.lock();
	Engine::renderer->pushModelDataToGPU(*this);
	availability |= ON_GPU;
	availability &= ~LOADING_TO_GPU;
	Engine::threading->pushingModelToGPUMutex.unlock();

	material->loadToGPU();
}

void ModelInstance::setModel(Model * m)
{
	material = m->material;
	model = m;
}

void ModelInstance::setMaterial(Material* pMaterial)
{
	material = pMaterial;

	auto loadJobFunc = std::bind([](Material* m) -> void {
		m->loadToRAM();
	}, material);

	auto toGPUFunc = std::bind([](Material* m) -> void {
		m->loadToGPU();
	}, material);

	auto job = new Job<decltype(loadJobFunc)>(loadJobFunc);
	job->setChild(new Job<decltype(toGPUFunc)>(toGPUFunc, Engine::threading->m_gpuWorker));
	Engine::threading->addDiskIOJob(job);
}

/*void ModelInstance::makePhysicsObject(btCollisionShape * collisionShape, float mass)
{
	physicsObject = new PhysicsObject();
	physicsObject->instance = this;
	physicsObject->create(transform.getTranslation(), transform.getQuat(), collisionShape, mass);
	Engine::physicsWorld.addRigidBody(physicsObject);
}*/

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

		glm::fvec3 scale = this->transform[0].getScale();

		if (shapeType == "sphere")
		{
			float radius = nodeToFloat(shapeNode->first_node("radius")) * this->transform[0].getScale().x;
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
					auto scale = transform[0].getScale();
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
	physicsObject->create(transform[0].getTranslation(), transform[0].getQuat(), colShape, mass);
	physicsObject->setDamping(linearDamping, angularDamping);
	physicsObject->setFriction(friction);
	physicsObject->setRestitution(restitution);
	Engine::physicsWorld.addRigidBody(physicsObject);
}

u32 ModelInstance::toEngineTransformIndex = 0;
u32 ModelInstance::toGPUTransformIndex = 0;