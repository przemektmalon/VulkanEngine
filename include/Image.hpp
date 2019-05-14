#pragma once
#include "PCH.hpp"

class Image
{
public:
	Image() {}
	Image(std::string path, int components = 4) { load(path, components); }
	std::vector<char> m_data;
	s32 m_width = 0,
		m_height = 0,
		m_bitsPerPixel = 0,
		m_channels = 0;
	
	void setSize(int width, int height, int channels = 4);

	void load(std::string path, int requestedChannels = 4);
	void load(void* const memory, int length, int requestedChannels = 0);
	void save(std::string path);

private:

	void internalLoadImage(u8* data, int requestedChannels);

};