#pragma once
#include "PCH.hpp"

class Image
{
public:
	std::vector<char> data;
	int width, height;
	int bpp;
	int components;
	
	void setSize(int pWidth, int pHeight, int pComponents = 4);

	void load(std::string path, int pComponents = 4);
	void save(std::string path);

	void generateMipMap(Image& mipmapped);
};