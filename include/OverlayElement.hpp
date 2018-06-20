#pragma once
#include "PCH.hpp"
#include "vdu/Shaders.hpp"
#include "vdu/Descriptors.hpp"

class OverlayElement
{
public:
	enum Type { Text, Poly };

	OverlayElement(Type pType);
	~OverlayElement();
	void setName(std::string pName) { name = pName; }
	std::string getName() { return name; }
	virtual void render(VkCommandBuffer cmd) {}
	virtual void cleanup() {}
	void* getPushConstData() { return pushConstData; }
	int getPushConstSize() { return pushConstSize; }
	VkDescriptorSet getDescriptorSet() { return descSet.getHandle(); }
	vdu::ShaderProgram* getShader() { return shader; }
	void setDepth(float pDepth) { depth = pDepth; depthUpdate = true; }
	float getDepth() { return depth; }
	Type getType() { return type; }

	bool needsDepthUpdate() { return depthUpdate; }
	bool needsDrawUpdate() { return drawUpdate; }
	void setUpdated() { depthUpdate = false; drawUpdate = false; }

	void setDoDraw(bool pDoDraw) { draw = pDoDraw; drawUpdate = true; }
	bool doDraw() { return draw; }
	void toggleDoDraw() { setDoDraw(!doDraw()); }

protected:
	void* pushConstData; /// TODO: free in derived
	int pushConstSize;

	std::string name;
	Type type;

	vdu::DescriptorSet descSet;

	vdu::ShaderProgram* shader;

	float depth;
	bool depthUpdate;
	bool drawUpdate;
	bool drawable;
	bool draw;
};

inline bool compareOverlayElements(OverlayElement* lhs, OverlayElement* rhs)
{
	return lhs->getDepth() < rhs->getDepth();
}