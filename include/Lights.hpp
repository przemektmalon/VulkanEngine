#pragma once
#include "PCH.hpp"
#include "Texture.hpp"
#include "vdu/DeviceMemory.hpp"

class Light
{
	friend class LightManager;
public:
	Texture* shadowTex;
	VkFramebuffer* shadowFBO;

	virtual Texture* getShadowTexture() = 0;
};

class LightGPUDataBase
{
	friend class LightManager;
public:
	union PositionRadius
	{
		PositionRadius() : all(0, 0, 0, 0) {}
		struct Separate
		{
			glm::fvec3 position;
			float radius;
		} separate;
		glm::fvec4 all;
	} positionRadius;

	union ColourQuadratic
	{
		ColourQuadratic() : all(0, 0, 0, 0) {}
		struct Separate
		{
			glm::fvec3 colour;
			float quadratic;
		} separate;
		glm::fvec4 all;
	} colourQuadratic;

	union LinearFadesTexture
	{
		LinearFadesTexture() { separate.linear = 0.001; separate.fadeStart = 500; separate.fadeLength = 100; separate.shadowIndex = -1; }
		struct Separate
		{
			float linear;
			unsigned short fadeStart;
			unsigned short fadeLength;
			s64 shadowIndex;
		} separate;
		glm::fvec4 all;
	} linearFadesTexture;

	float getRadius() { 
		return positionRadius.separate.radius; 
	}
	void setRadius(float set) {
		positionRadius.separate.radius = set;
	}
	glm::fvec3 getPosition() { return positionRadius.separate.position; }
	void setPosition(glm::fvec3 set) {
		positionRadius.separate.position = set;
	}
	glm::fvec3 getColour() { return colourQuadratic.separate.colour; }
	void setColour(glm::fvec3 set) {
		colourQuadratic.separate.colour = set;
	}
	float getQuadratic() { return colourQuadratic.separate.quadratic; }
	void setQuadratic(float set) {
		colourQuadratic.separate.quadratic = set;
	}
	float getLinear() { return linearFadesTexture.separate.linear; }
	void setLinear(float set) {
		linearFadesTexture.separate.linear = set;
	}
	float getFadeStart() { return linearFadesTexture.separate.fadeStart; }
	void setFadeStart(float set) {
		linearFadesTexture.separate.fadeStart = set;
	}
	float getFadeLength() { return linearFadesTexture.separate.fadeLength; }
	void setFadeLength(float set) {
		linearFadesTexture.separate.fadeLength = set;
	}
	s64 getShadowIndex() { return linearFadesTexture.separate.shadowIndex; }
	void setShadowIndex(s64 set) {
		linearFadesTexture.separate.shadowIndex = set;
	}
};

class SunLight : public Light
{
	friend class LightManager;
public:
	struct GPUData;

private:
	GPUData* gpuData;

public:

	Texture* getShadowTexture() { return shadowTex; }

	struct GPUData
	{
		friend struct SunLight;
		friend class LightManager;
	private:
		union Colour
		{
			Colour() : all(100, 100, 100, 1) {}
			struct Separate
			{
				glm::fvec3 colour;
			} separate;
			glm::fvec4 all;
		} colour;

		union Direction
		{
			Direction() : all(-1, -1, -1, 0) 
			{
				separate.direction = glm::normalize(glm::fvec3(0.1, -1.0, 0.2));
			}
			struct Separate
			{
				glm::fvec3 direction;
			} separate;
			glm::fvec4 all;
		} direction;

	public:
		glm::fmat4 projView[3];
		float cascadeEnds[4];
	};

	SunLight()
	{
		/// TODO: create shadow texture
		gpuData = new GPUData();
		gpuData->cascadeEnds[0] = 0.1;
		gpuData->cascadeEnds[1] = 100;
		gpuData->cascadeEnds[2] = 500;
		gpuData->cascadeEnds[3] = 1000;
	}

	void destroy();

	void initTexture(glm::ivec2 resolution);
	void calcProjs();

	glm::fvec3 getDirection() { return gpuData->direction.separate.direction; }
	void setDirection(glm::fvec3 d) { gpuData->direction.separate.direction = d; }

	glm::fvec3 getColour() { return gpuData->colour.separate.colour; }
	void setColour(glm::fvec3 c) { gpuData->colour.separate.colour = c; }

	float* getCascadeEnds() { return gpuData->cascadeEnds; }

	glm::fmat4* getProjView() { return gpuData->projView; }
	void setProjView(int index, glm::fmat4& projView) { gpuData->projView[index] = projView; }
	GPUData* getData() { return gpuData; }
};

class PointLight : public Light
{
	friend class LightManager;
public:
	struct GPUData;

private:
	GPUData* gpuData;

	glm::fmat4 proj;
	bool matNeedsUpdate;

public:
	PointLight() : matNeedsUpdate(true), gpuData(nullptr)
	{
		initTexture();
	}

	struct GPUData : public LightGPUDataBase
	{
		friend struct PointLight;
		friend class LightManager;
	private:

		glm::fmat4 projView[6];

	protected:
		
		glm::fmat4* getProjView() { return projView; }
		void updateProjView(glm::fmat4& proj) ///TODO: dont need lookAt, just construct manually
		{
			projView[0] = proj * (glm::lookAt(getPosition(), getPosition() + glm::fvec3(1.0, 0.0, 0.0), glm::fvec3(0.0, -1.0, 0.0)));
			projView[1] = proj * (glm::lookAt(getPosition(), getPosition() + glm::fvec3(-1.0, 0.0, 0.0), glm::fvec3(0.0, -1.0, 0.0)));
			projView[2] = proj * (glm::lookAt(getPosition(), getPosition() + glm::fvec3(0.0, 1.0, 0.0), glm::fvec3(0.0, 0.0, 1.0)));
			projView[3] = proj * (glm::lookAt(getPosition(), getPosition() + glm::fvec3(0.0, -1.0, 0.0), glm::fvec3(0.0, 0.0, -1.0)));
			projView[4] = proj * (glm::lookAt(getPosition(), getPosition() + glm::fvec3(0.0, 0.0, 1.0), glm::fvec3(0.0, -1.0, 0.0)));
			projView[5] = proj * (glm::lookAt(getPosition(), getPosition() + glm::fvec3(0.0, 0.0, -1.0), glm::fvec3(0.0, -1.0, 0.0)));
		}
	};

	float getRadius() { return gpuData->getRadius(); }
	glm::fvec3 getPosition() { return gpuData->getPosition(); }
	glm::fvec3 getColour() { return gpuData->getColour(); }
	float getQuadratic() { return gpuData->getQuadratic(); }
	float getLinear() { return gpuData->getLinear(); }
	float getFadeStart() { return gpuData->getFadeStart(); }
	float getFadeLength() { return gpuData->getFadeLength(); }
	s64 getHandle() { return gpuData->getShadowIndex(); }
	Texture* getShadowTexture() { return shadowTex; }
	glm::fmat4* getProjView() { return gpuData->projView; }

	void setPosition(glm::fvec3 set)
	{
		gpuData->setPosition(set);
		matNeedsUpdate = true;
	}

	void setColour(glm::fvec3 set)
	{
		gpuData->setColour(set);
	}

	void setQuadratic(float set)
	{
		gpuData->setQuadratic(set);
		updateRadius();
		matNeedsUpdate = true;
	}

	void setLinear(float set)
	{
		gpuData->setLinear(set);
		updateRadius();
		matNeedsUpdate = true;
	}

	void setFadeStart(float set)
	{
		gpuData->setFadeStart(set);
	}

	void setFadeLength(float set)
	{
		gpuData->setFadeLength(set);
	}

	void update()
	{
		updateRadius();
		if (matNeedsUpdate)
			gpuData->updateProjView(proj);
	}

	void initTexture(int resolution = 1024);

	void destroy();

	void updateRadius();
	inline static float calculateRadius(float linear, float quad);

};

class SpotLight : public Light
{
	friend class LightManager;
public:
	struct GPUData;
	
private:
	bool matNeedsUpdate;
	GPUData* gpuData;
	vdu::Buffer* drawCommandsBuffer;

public:
	SpotLight() : matNeedsUpdate(true), gpuData(nullptr), drawCommandsBuffer(nullptr)
	{
		initTexture();
	}
	~SpotLight() {}

	struct GPUData : public LightGPUDataBase
	{
		friend struct SpotLight;
		friend class LightManager;
	private:

		union DirectionInner
		{
			DirectionInner() : all(0, 0, 0, 0) {}
			struct Separate
			{
				glm::fvec3 direction;
				float inner;
			} separate;
			glm::fvec4 all;
		} directionInner;

		union Outer
		{
			Outer() : all(0, 0, 0, 0) {}
			struct Separate
			{
				float outer;
			} separate;
			glm::fvec4 all;
		} outer;

		glm::fmat4 projView;

	protected:

		glm::fvec3 getDirection() { return directionInner.separate.direction; }
		void setDirection(glm::fvec3 set) {
			directionInner.separate.direction = set;
		}
		float getInner() { return directionInner.separate.inner; }
		bool setInner(float set) {
			directionInner.separate.inner = set;
			if (getInner() < getOuter()) {
				outer.separate.outer = set; return true;
			}
			return false;
		}
		float getOuter() { return outer.separate.outer; }
		bool setOuter(float set) {
			outer.separate.outer = set;
			if (getOuter() > getInner()) {
				directionInner.separate.inner = set; return true;
			}
			return false;
		}

		glm::fmat4& getProjView() { return projView; }
		void updateProjView() ///TODO: dont need lookAt, just construct manually
		{
			float out = getOuter();
			float rad = getRadius();
			float rads = glm::acos(out);
			auto proj = glm::perspective<float>(rads*2.f, 1.f, 1.f, rad * 2.f);
			glm::mat4 clip(1.0f, 0.0f, 0.0f, 0.0f,
				+0.0f, -1.0f, 0.0f, 0.0f,
				+0.0f, 0.0f, 0.5f, 0.0f,
				+0.0f, 0.0f, 0.5f, 1.0f);
			proj = clip * proj;
			auto pos = getPosition();
			auto dir = getDirection();
			glm::fmat4 view = glm::lookAt(pos, pos + dir, glm::fvec3(0, 1, 0));
			projView = proj * view;
		}
	};

	float getRadius() { return gpuData->getRadius(); }
	glm::fvec3 getPosition() { return gpuData->getPosition(); }
	glm::fvec3 getDirection() { return gpuData->getDirection(); }
	glm::fvec3 getColour() { return gpuData->getColour(); }
	float getInner() { return gpuData->getInner(); }
	float getOuter() { return gpuData->getOuter(); }
	float getQuadratic() { return gpuData->getQuadratic(); }
	float getLinear() { return gpuData->getLinear(); }
	float getFadeStart() { return gpuData->getFadeStart(); }
	float getFadeLength() { return gpuData->getFadeLength(); }
	s64 getShadowIndex() { return gpuData->getShadowIndex(); }
	Texture* getShadowTexture() { return shadowTex; }
	glm::fmat4& getProjView() { return gpuData->projView; }
	vdu::Buffer* getDrawCommandsBuffer() { return drawCommandsBuffer; }

	void setPosition(glm::fvec3 set)
	{
		gpuData->setPosition(set);
		matNeedsUpdate = true;
	}

	void setDirection(glm::fvec3 set)
	{
		gpuData->setDirection(set);
		matNeedsUpdate = true;
	}

	void setInnerSpread(float set)
	{
		if (gpuData->setInner(glm::cos(glm::radians(set))))
			matNeedsUpdate = true;
	}

	void setOuterSpread(float set)
	{
		gpuData->setOuter(glm::cos(glm::radians(set)));
		matNeedsUpdate = true;
	}

	void setColour(glm::fvec3 set)
	{
		gpuData->setColour(set);
	}

	void setQuadratic(float set)
	{
		gpuData->setQuadratic(set);
		updateRadius();
		matNeedsUpdate = true;
	}

	void setLinear(float set)
	{
		gpuData->setLinear(set);
		updateRadius();
		matNeedsUpdate = true;
	}

	void setFadeStart(float set)
	{
		gpuData->setFadeStart(set);
	}

	void setFadeLength(float set)
	{
		gpuData->setFadeLength(set);
	}

	void destroy();

	void update()
	{
		updateRadius();
		if (matNeedsUpdate)
			gpuData->updateProjView();
	}

	void initTexture(int resolution = 512);
	void updateDrawCommands(u32 gpuIndex);

	void updateRadius();
	inline static float calculateRadius(float linear, float quad);
};

class LightManager
{
public: ///TODO: Max light count is 150, add setters etc
	LightManager() {}
	~LightManager() {}

	void init();

	void cleanup()
	{
		lightCountsBuffer.destroy();
		spotLightsBuffer.destroy();
		pointLightsBuffer.destroy();
		sunLightBuffer.destroy();
		for (auto& l : pointLights)
			l.destroy();
		for (auto& l : spotLights)
			l.destroy();
		sunLight.destroy();
	}

	PointLight& addPointLight()
	{
		auto data = PointLight::GPUData();
		return addPointLight(data);
	}

	PointLight& addPointLight(PointLight::GPUData& data);

	PointLight* getPointLight(u32 pIndex)
	{
		return pointLights.data() + pIndex;
	}

	void updateLightCounts()
	{
		u32* lc = (u32*)lightCountsBuffer.getMemory()->map();
		lc[0] = pointLights.size();
		lc[1] = spotLights.size();
		lightCountsBuffer.getMemory()->unmap();
	}

	void updateAllPointLights()
	{
		for (auto& l : pointLights)
			l.update();
		PointLight::GPUData* d = (PointLight::GPUData*)pointLightsBuffer.getMemory()->map();
		for (int i = 0; i < pointLights.size(); ++i)
		{
			d[i].colourQuadratic = pointLightsGPUData[i].colourQuadratic;
			d[i].positionRadius = pointLightsGPUData[i].positionRadius;
			d[i].linearFadesTexture = pointLightsGPUData[i].linearFadesTexture;
			for (int j = 0; j < 5; ++j)
				d[i].projView[j] = pointLightsGPUData[i].projView[j];
		}
		pointLightsBuffer.getMemory()->unmap();
	}

	SpotLight& addSpotLight()
	{
		auto data = SpotLight::GPUData();
		return addSpotLight(data);
	}

	SpotLight& addSpotLight(SpotLight::GPUData& data);

	SpotLight* getSpotLight(u32 pIndex)
	{
		return spotLights.data() + pIndex;
	}

	void updateAllSpotLights()
	{
		for (auto& l : spotLights)
			l.update();
		SpotLight::GPUData* d = (SpotLight::GPUData*)spotLightsBuffer.getMemory()->map();
		for (int i = 0; i < spotLights.size(); ++i)
		{
			d[i].colourQuadratic = spotLightsGPUData[i].colourQuadratic;
			d[i].positionRadius = spotLightsGPUData[i].positionRadius;
			d[i].linearFadesTexture = spotLightsGPUData[i].linearFadesTexture;
			d[i].directionInner = spotLightsGPUData[i].directionInner;
			d[i].outer = spotLightsGPUData[i].outer;
			d[i].projView = spotLightsGPUData[i].projView;
		}
		spotLightsBuffer.getMemory()->unmap();
	}

	void updateSunLight();

	void updateShadowDrawCommands()
	{
		int i = 0;
		for (auto& sl : spotLights)
		{
			sl.updateDrawCommands(i);
			++i;
		}
	}

	vdu::Buffer lightCountsBuffer;

	std::vector<PointLight> pointLights;
	std::vector<PointLight::GPUData> pointLightsGPUData;
	vdu::Buffer pointLightsBuffer;

	std::vector<SpotLight> spotLights;
	std::vector<SpotLight::GPUData> spotLightsGPUData;
	vdu::Buffer spotLightsBuffer;

	SunLight sunLight;
	vdu::Buffer sunLightBuffer;

	std::vector<vdu::Buffer> spotShadowDrawCommandsBuffers;

	//std::vector<SpotLight::GPUData> staticSpotLights;
	//std::vector<PointLight::GPUData> staticPointLightsGPUData;
	//std::vector<GLTextureCube> staticPointLightsShadowTex;
};