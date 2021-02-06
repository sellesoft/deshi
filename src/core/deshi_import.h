#pragma once
#include "deshi_defines.h"

#include <vector>
#include <iostream>
#include <fstream>

namespace deshi{
	//TODO(,delle) fix error when not finding file
	//reads a files contents in binary and returns it as a char vector
	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		
		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}
		
		size_t fileSize = (size_t) file.tellg();
		//std::cout << filename << ": " << fileSize << std::endl;
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();
		
		return buffer;
	}
}