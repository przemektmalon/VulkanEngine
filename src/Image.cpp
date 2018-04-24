#include "PCH.hpp"
#include "Image.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include "cro_mipmap.h"

void Image::setSize(int pWidth, int pHeight, int pComponents)
{
	width = pWidth; height = pHeight; components = pComponents;
	data.resize(width * height * components);
}

void Image::load(std::string path, int pComponents)
{
	char buff[FILENAME_MAX];
  	GetCurrentDir( buff, FILENAME_MAX );
  	std::string current_working_dir(buff);
	if (path[0] == '/')
		path = current_working_dir + path;
	else
		path = current_working_dir + "/" + path;

	DBG_INFO("Loading image: " << path);
	unsigned char* loadedData = stbi_load(path.c_str(), &width, &height, &components, pComponents);
	if (pComponents != 0)
	{
		components = pComponents;
	}
	bpp = 8 * components;
	if (!loadedData) {
		DBG_WARNING("Failed to load image: " << path);
		return;
	}
	data.resize(width*height*components);
	memcpy(&data[0], loadedData, width * height * components);
	stbi_image_free(loadedData);
}

void Image::save(std::string path)
{
	int result = stbi_write_png(path.c_str(), width, height, components, &data[0], 0);
}

void Image::generateMipMap(Image & mipmapped)
{
	mipmapped.data.clear();
	mipmapped.setSize(width >> 1, height >> 1);
	cro_GenMipMapAvgI((int*)&data[0], width, height, (int*)&mipmapped.data[0]);
}
