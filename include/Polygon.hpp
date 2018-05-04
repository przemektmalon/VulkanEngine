#pragma once
#include "PCH.hpp"
#include "OverlayElement.hpp"
#include "Model.hpp"

class Polygon : public OverlayElement
{
public:

	Polygon();

	void draw(VkCommandBuffer cmd);

	

private:

	std::vector<Vertex2D> verts;
};