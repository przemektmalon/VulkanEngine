#pragma once
#include "PCH.hpp"
#include "Shader.hpp"

class OverlayElement
{
public:
	enum Type { Text, Poly };

	OverlayElement(Type pType);
	virtual void draw(VkCommandBuffer cmd) {}
	virtual void cleanup() {}
	void* getPushConstData() { return pushConstData; }
	int getPushConstSize() { return pushConstSize; }
	VkDescriptorSet getDescriptorSet() { return descSet; }
	ShaderProgram* getShader() { return shader; }

protected:
	void* pushConstData; /// TODO: free in derived
	int pushConstSize;

	Type type;

	VkDescriptorSet descSet;

	ShaderProgram* shader;
};