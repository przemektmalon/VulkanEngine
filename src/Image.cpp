#include "PCH.hpp"
#include "Image.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include "cro_mipmap.h"

void Image::setSize(int pWidth, int pHeight)
{
	width = pWidth; height = pHeight;
	data.resize(width * height);
}

void Image::load(std::string path)
{
	char buff[FILENAME_MAX];
  	GetCurrentDir( buff, FILENAME_MAX );
  	std::string current_working_dir(buff);
	path = current_working_dir + "/" + path;

	int bpp;
	DBG_INFO("Loading image: " << path);
	unsigned char* loadedData = stbi_load(path.c_str(), &width, &height, &bpp, 4);
	if (!loadedData) {
		DBG_WARNING("Failed to load image: " << path);
		return;
	}
	data.resize(width*height);
	memcpy(&data[0], loadedData, width * height * sizeof(Pixel));
	stbi_image_free(loadedData);
}

void Image::save(std::string path)
{
	int result = stbi_write_png(path.c_str(), width, height, 4, &data[0], 0);
}

void Image::generateMipMap(Image & mipmapped)
{
	mipmapped.data.clear();
	mipmapped.setSize(width >> 1, height >> 1);
	cro_GenMipMapAvgI((int*)&data[0], width, height, (int*)&mipmapped.data[0]);
}
