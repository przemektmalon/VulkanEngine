#pragma once
#include "PCH.hpp"

class OverlayElement
{
public:
	OverlayElement();
	virtual void draw(VkCommandBuffer cmd) {}
	void* getPushConstData() { return pushConstData; }
	int getPushConstSize() { return pushConstSize; }
	VkDescriptorSet getDescriptorSet() { return descSet; }

protected:
	void* pushConstData;
	int pushConstSize;

	VkDescriptorSet descSet;
};