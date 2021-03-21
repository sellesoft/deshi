#pragma once
#ifndef DESHI_ASSETS_H
#define DESHI_ASSETS_H

#include "../utils/defines.h"


#include <iostream>
#include <fstream>
#include <vector>

#include <filesystem>

namespace deshi{
	
	//TODO(,delle) check which dir we are in to get the correct path
	inline static std::string getDataPath()    { return "data/"; }
	inline static std::string getModelsPath()  { return getDataPath() + "models/"; }
	inline static std::string getTexturesPath(){ return getDataPath() + "textures/"; }
	inline static std::string getSoundsPath()  { return getDataPath() + "sounds/"; }
	inline static std::string getShadersPath() { return "shaders/"; }
	inline static std::string getConfigsPath() { return "cfg/"; }
	
	//reads a files contents in binary and returns it as a char vector
	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if(!file.is_open()){
			PRINT("[ERROR] failed to open file: " << filename);
			return std::vector<char>();
		};
		
		size_t fileSize = (size_t) file.tellg();
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();
		
		return buffer;
	}

	//creates base deshi directories if they dont already exist
	static void enforceDirectories() {
		
		using namespace std::filesystem;
		if (!is_directory("data")) {
			create_directory("data");
			create_directory("data/models");
			create_directory("data/scenes");
			create_directory("data/sounds");
			create_directory("data/textures");
		}
		else {
			if (!is_directory("data/models")) { 
				create_directory("data/models");
			}
			if (!is_directory("data/scenes")) { 
				create_directory("data/scenes"); 
			}
			if (!is_directory("data/sounds")) {
				create_directory("data/sounds");
			}
			if (!is_directory("data/textures")) {
				create_directory("data/textures");
			}
		}


		if (!is_directory("shaders")) {
			create_directory("shaders");
		}

		if (!is_directory("cfg")) {
			create_directory("cfg");
		}

	}
	
} //namespace deshi

#endif //DESHI_ASSETS_H