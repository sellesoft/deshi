#pragma once
#ifndef DESHI_ASSETS_H
#define DESHI_ASSETS_H

#include "../utils/defines.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

namespace deshi{
	
	inline static std::string getDataPath()    { return "data/"; }
	inline static std::string getModelsPath()  { return getDataPath() + "models/"; }
	inline static std::string getTexturesPath(){ return getDataPath() + "textures/"; }
	inline static std::string getSoundsPath()  { return getDataPath() + "sounds/"; }
	inline static std::string getShadersPath() { return "shaders/"; }
	inline static std::string getConfigsPath() { return "cfg/"; }
	
	//reads a files contents in ASCII and returns it as a char vector
	static std::vector<char> readFile(const std::string& filepath, u32 chars = 0) {
		std::ifstream file(filepath, std::ios::ate);
		if(!file.is_open()){ PRINT("[READ-ERROR] failed to open file: " << filepath); return {}; };
		
		if(chars == 0){ chars = u32(file.tellg()); }
		
		std::vector<char> buffer(chars);
		file.seekg(0);
		file.read(buffer.data(), chars);
		file.close();
		
		return buffer;
	}
	
	//reads a files contents in binary and returns it as a char vector
	static std::vector<char> readFileBinary(const std::string& filepath, u32 bytes = 0) {
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);
		if(!file.is_open()){ PRINT("[READ-ERROR] failed to open file: " << filepath); return {}; };
		
		if(bytes == 0){ bytes = u32(file.tellg()); }
		std::vector<char> buffer(bytes);
		file.seekg(0);
		file.read(buffer.data(), bytes);
		file.close();
		
		return buffer;
	}
	
	//truncates and writes a char vector to a file in ASCII format
	static void writeFile(const std::string& filepath, std::vector<char>& data, u32 chars = 0){
		std::ofstream file(filepath, std::ios::out | std::ios::trunc);
		if(!file.is_open()){ PRINT("[WRITE-ERROR] failed to open file: " << filepath); return; }
		
		if(chars == 0){ chars = data.size(); }
		
		file.write(reinterpret_cast<const char*>(data.data()), chars);
		file.close();
	}
	
	//truncates and writes a char array to a file in ASCII format
	static void writeFile(const std::string& filepath, const char* data, u32 chars){
		std::ofstream file(filepath, std::ios::out | std::ios::trunc);
		if(!file.is_open()){ PRINT("[WRITE-ERROR] failed to open file: " << filepath); return; }
		
		file.write(data, chars);
		file.close();
	}
	
	//truncates and writes a char vector to a file in binary
	static void writeFileBinary(const std::string& filepath, std::vector<char>& data, u32 bytes = 0){
		std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
		if(!file.is_open()){ PRINT("[WRITE-ERROR] failed to open file: " << filepath); return; }
		
		if(bytes == 0){ bytes = data.size(); }
		
		file.write(reinterpret_cast<const char*>(data.data()), bytes);
		file.close();
	}
	
	//truncates and writes a char array to a file in binary
	static void writeFileBinary(const std::string& filepath, const char* data, u32 bytes){
		std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
		if(!file.is_open()){ PRINT("[WRITE-ERROR] failed to open file: " << filepath); return; }
		
		file.write(data, bytes);
		file.close();
	}

	//iterates directory and returns a list of files in it
	//probably return something other than a vector of strings but thts how it is for now
	static std::vector<std::string> iterateDirectory(const std::string& filepath) {
		using namespace std::filesystem;
		for (auto& p : directory_iterator(filepath)) {
			PRINT(p.path());
		}
		return std::vector<std::string>();
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
			if (!is_directory("data/models")) { create_directory("data/models"); }
			if (!is_directory("data/scenes")) { create_directory("data/scenes"); }
			if (!is_directory("data/sounds")) { create_directory("data/sounds"); }
			if (!is_directory("data/textures")) { create_directory("data/textures"); }
		}
		
		if (!is_directory("shaders")) { create_directory("shaders"); }
		if (!is_directory("cfg")) { create_directory("cfg"); }
		if (!is_directory("logs")) { create_directory("logs"); }
	}
	
} //namespace deshi

#endif //DESHI_ASSETS_H