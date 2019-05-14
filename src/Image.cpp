#include "PCH.hpp"
#include "Image.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "Engine.hpp"
#include "Console.hpp"

void Image::setSize(int width, int height, int channels)
{
	m_width = width; m_height = height; m_channels = channels;
	m_data.resize(m_width * m_height * m_channels);
}

void Image::load(std::string path, int requestedChannels)
{
	path = Engine::workingDirectory + path;
	u8* loadedData = stbi_load(path.c_str(), &m_width, &m_height, &m_channels, requestedChannels);
	internalLoadImage(loadedData, requestedChannels);
}

void Image::load(void* const memory, int length, int requestedChannels)
{
	u8* loadedData = stbi_load_from_memory(static_cast<stbi_uc const *>(memory), length, &m_width, &m_height, &m_channels, requestedChannels);
	internalLoadImage(loadedData, requestedChannels);
}

void Image::save(std::string path)
{
	int result = stbi_write_png(path.c_str(), m_width, m_height, m_channels, &m_data[0], 0);
}

void Image::internalLoadImage(u8* loadedData, int requestedChannels)
{
	if (!loadedData) {
		DBG_WARNING("Failed to load image");
		return;
	}

	if (requestedChannels != 0) {
		m_channels = requestedChannels;
	}

	m_bitsPerPixel = 8 * m_channels;
	m_data.resize(m_width * m_height * m_channels);
	memcpy(&m_data[0], loadedData, m_width * m_height * m_channels);
	stbi_image_free(loadedData);
}

