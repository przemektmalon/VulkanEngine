#pragma once
#include "PCH.hpp"
#include "Shader.hpp"

class OverlayElement
{
public:
	enum Type { Text, Poly };

	OverlayElement(Type pType);
	~OverlayElement();
	void setName(std::string pName) { name = pName; }
	std::string getName() { return name; }
	virtual void draw(VkCommandBuffer cmd) {}
	virtual void cleanup() {}
	bool needsDrawUpdate() { return drawUpdate; }
	void* getPushConstData() { return pushConstData; }
	int getPushConstSize() { return pushConstSize; }
	VkDescriptorSet getDescriptorSet() { return descSet; }
	ShaderProgram* getShader() { return shader; }


protected:
	void* pushConstData; /// TODO: free in derived
	int pushConstSize;

	std::string name;
	Type type;

	VkDescriptorSet descSet;

	ShaderProgram* shader;

	bool drawUpdate;
};