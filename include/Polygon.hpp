#pragma once
#include "PCH.hpp"
#include "OverlayElement.hpp"
#include "Model.hpp"

class UIPolygon : public OverlayElement
{
public:
	UIPolygon() : OverlayElement(Poly)
	{
		pushConstData = new glm::fvec4(1,1,1,1);
		pushConstSize = sizeof(glm::fvec4);
		drawable = false;
	}

	void reserveBuffer(int numVerts);

	void render(VkCommandBuffer cmd);

	void cleanup();

	void setTexture(Texture* tex);
	void setVerts(std::vector<Vertex2D>& v);
	void setColour(glm::fvec4 colour);

private:

	void updateDescriptorSet();

	std::vector<Vertex2D> verts;
	vdu::Buffer vertsBuffer;
	Texture* texture;
	glm::fvec4 colour;
};