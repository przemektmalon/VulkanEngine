#pragma once
#include "PCH.hpp"
#include <filesystem>
#include "File.hpp"

namespace fs = std::filesystem;

class Filesystem {

public:

	// NOTE: This function does not clear the previous entries in the 'files' vector, which may come from different directories
	void getFilesInDirectory(const std::string& directory, std::vector<std::string>& files) {
		for (auto& entry : fs::directory_iterator(directory)) {
			if (entry.is_regular_file()) {
				files.push_back(entry.path().generic_string()); // NOTE: does not support >8 bit characters ?
			}
		}
	}

	uintmax_t getFileSize(const std::string& filePath) {
		return fs::file_size(filePath);
	}

};