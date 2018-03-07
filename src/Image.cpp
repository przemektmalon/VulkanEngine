#include "Image.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

void Image::load(std::string path)
{
	char buff[FILENAME_MAX];
  	GetCurrentDir( buff, FILENAME_MAX );
  	std::string current_working_dir(buff);
	path = current_working_dir + "/" + path;

	int bpp;
	DBG_INFO("Attempting to load image: " << path);
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