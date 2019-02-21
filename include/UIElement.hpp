#pragma once
#include "PCH.hpp"

class UIElement
{
public:
	enum Type { Text, Poly };

	UIElement(Type pType);
	~UIElement();
	void setName(std::string pName) { name = pName; }
	std::string getName() { return name; }
	virtual void render(vdu::CommandBuffer& cmd) {}
	virtual void cleanup() {}
	vdu::DescriptorSet& getDescriptorSet() { return descSet; }
	vdu::ShaderProgram* getShader() { return shader; }
	glm::fvec4 getColour() { return colour; }
	void setDepth(float pDepth) { depth = pDepth; depthUpdate = true; }
	float getDepth() { return depth; }
	Type getType() { return type; }

	bool needsDepthUpdate() { return depthUpdate; }
	bool needsDrawUpdate() { return drawUpdate; }
	void setUpdated() { depthUpdate = false; drawUpdate = false; }

	void setColour(glm::fvec4 pColour) { colour = pColour; }
	void setDoDraw(bool pDoDraw) { draw = pDoDraw; drawUpdate = true; }
	bool doDraw() { return draw; }
	void toggleDoDraw() { setDoDraw(!doDraw()); }

protected:
	std::string name;
	Type type;

	glm::fvec4 colour;

	vdu::DescriptorSet descSet;

	vdu::ShaderProgram* shader;

	float depth;
	bool depthUpdate;
	bool drawUpdate;
	bool drawable;
	bool draw;
};

inline bool compareOverlayElements(UIElement* lhs, UIElement* rhs)
{
	return lhs->getDepth() < rhs->getDepth();
}