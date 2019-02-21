#pragma once
#include "PCH.hpp"
#include "UIElement.hpp"
#include "Model.hpp"

class UIPolygon : public UIElement
{
public:
	UIPolygon() : UIElement(Poly)
	{
		drawable = false;
	}

	void reserveBuffer(int numVerts);

	void render(vdu::CommandBuffer& cmd);

	void cleanup();

	void setTexture(Texture* tex);
	void setVerts(std::vector<Vertex2D>& v);

private:

	void updateDescriptorSet();

	std::vector<Vertex2D> verts;
	vdu::Buffer vertsBuffer;
	Texture* texture;
};