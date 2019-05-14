#include "PCH.hpp"
#include "Image.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "Engine.hpp"
#include "Console.hpp"

void Image::setSize(int pWidth, int pHeight, int pComponents)
{
	width = pWidth; height = pHeight; components = pComponents;
	data.resize(width * height * components);
}

void Image::load(std::string path, int pComponents)
{
	/// TODO: use cwd stored in Engine class
	char buff[FILENAME_MAX];
  	GetCurrentDir( buff, FILENAME_MAX );
  	std::string current_working_dir(buff);

	Engine::console->postMessage("Loading image: " + path, glm::fvec3(0.8, 0.8, 0.3));

	if (path[0] == '/')
		path = current_working_dir + path;
	else
		path = current_working_dir + "/" + path;

	
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
	data.resize(width * height * components);
	memcpy(&data[0], loadedData, width * height * components);
	stbi_image_free(loadedData);
}

void Image::load(void* const memory, int length, int requestedChannels)
{
	unsigned char* loadedData = stbi_load_from_memory(static_cast<stbi_uc const *>(memory), length, &width, &height, &components, requestedChannels);
	if (!loadedData) {
		DBG_WARNING("Failed to load image from memory stream");
		return;
	}

	if (requestedChannels != 0) {
		components = requestedChannels;
	}
	bpp = 8 * components;

	data.resize(width * height * components);
	memcpy(&data[0], loadedData, width * height * components);
	stbi_image_free(loadedData);
}

void Image::save(std::string path)
{
	int result = stbi_write_png(path.c_str(), width, height, components, &data[0], 0);
}

