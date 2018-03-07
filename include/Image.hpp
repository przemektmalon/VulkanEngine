#pragma once
#include "PCH.hpp"

struct Pixel
{
	char r, g, b, a;
};

class Image
{
public:
	std::vector<Pixel> data;
	int width, height;

	void load(std::string path);
	void save(std::string path);
};