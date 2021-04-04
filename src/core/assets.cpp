#include "assets.h"
#include "../utils/debug.h"

#include <iostream>
#include <fstream>
#include <filesystem>

////////////////////
//// file paths ////
////////////////////

std::string deshi::
getData(const char* filename){
	std::string file = dirData() + filename;
	if(std::filesystem::exists(std::filesystem::path(file))){
		return file;
	}else{
		PRINT("Failed to find data: "<< file); return "";
	}
}

std::string deshi::
getModel(const char* filename){
	std::string file = dirModels() + filename;
	if(std::filesystem::exists(std::filesystem::path(file))){
		return file;
	}else{
		PRINT("Failed to find model: "<< file); return "";
	}
}

std::string deshi::
getTexture(const char* filename){
	std::string file = dirTextures() + filename;
	if(std::filesystem::exists(std::filesystem::path(file))){
		return file;
	}else{
		PRINT("Failed to find texture: "<< file); return "";
	}
}

std::string deshi::
getSound(const char* filename){
	std::string file = dirSounds() + filename;
	if(std::filesystem::exists(std::filesystem::path(file))){
		return file;
	}else{
		PRINT("Failed to find sound: "<< file); return "";
	}
}

std::string deshi::
getShader(const char* filename){
	std::string file = dirShaders() + filename;
	if(std::filesystem::exists(std::filesystem::path(file))){
		return file;
	}else{
		PRINT("Failed to find shader: "<< file); return "";
	}
}

std::string deshi::
getConfig(const char* filename){
	std::string file = dirConfig() + filename;
	if(std::filesystem::exists(std::filesystem::path(file))){
		return file;
	}else{
		PRINT("Failed to find config: "<< file); return "";
	}
}

/////////////////////////
//// file read-write ////
/////////////////////////

std::vector<char> deshi::
readFile(const std::string& filepath, u32 chars) {
	std::ifstream file(filepath, std::ios::ate);
	if(!file.is_open()){ PRINT("[READ-ERROR] failed to open file: " << filepath); return {}; };
	
	if(chars == 0){ chars = u32(file.tellg()); }
	
	std::vector<char> buffer(chars);
	file.seekg(0);
	file.read(buffer.data(), chars);
	file.close();
	
	return buffer;
}

std::vector<char> deshi::
readFileBinary(const std::string& filepath, u32 bytes) {
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);
	if(!file.is_open()){ PRINT("[READ-ERROR] failed to open file: " << filepath); return {}; };
	
	if(bytes == 0){ bytes = u32(file.tellg()); }
	std::vector<char> buffer(bytes);
	file.seekg(0);
	file.read(buffer.data(), bytes);
	file.close();
	
	return buffer;
}

void deshi::
writeFile(const std::string& filepath, std::vector<char>& data, u32 chars){
	std::ofstream file(filepath, std::ios::out | std::ios::trunc);
	if(!file.is_open()){ PRINT("[WRITE-ERROR] failed to open file: " << filepath); return; }
	
	if(chars == 0){ chars = data.size(); }
	
	file.write(reinterpret_cast<const char*>(data.data()), chars);
	file.close();
}

void deshi::
writeFile(const std::string& filepath, const char* data, u32 chars){
	std::ofstream file(filepath, std::ios::out | std::ios::trunc);
	if(!file.is_open()){ PRINT("[WRITE-ERROR] failed to open file: " << filepath); return; }
	
	file.write(data, chars);
	file.close();
}

void deshi::
writeFileBinary(const std::string& filepath, std::vector<char>& data, u32 bytes){
	std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
	if(!file.is_open()){ PRINT("[WRITE-ERROR] failed to open file: " << filepath); return; }
	
	if(bytes == 0){ bytes = data.size(); }
	
	file.write(reinterpret_cast<const char*>(data.data()), bytes);
	file.close();
}

void deshi::
writeFileBinary(const std::string& filepath, const char* data, u32 bytes){
	std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
	if(!file.is_open()){ PRINT("[WRITE-ERROR] failed to open file: " << filepath); return; }
	
	file.write(data, bytes);
	file.close();
}

std::vector<std::string> deshi::
iterateDirectory(const std::string& filepath) {
	using namespace std::filesystem;
	std::vector<std::string> files;
	for (auto& p : directory_iterator(filepath)) {
		files.push_back(p.path().stem().string());
	}
	return files;
}

void deshi::enforceDirectories() {
	using namespace std::filesystem;
	if (!is_directory("data")) {
		create_directory("data");
		create_directory("data/models");
		create_directory("data/scenes");
		create_directory("data/sounds");
		create_directory("data/textures");
	} else {
		if (!is_directory("data/models")) { create_directory("data/models"); }
		if (!is_directory("data/scenes")) { create_directory("data/scenes"); }
		if (!is_directory("data/sounds")) { create_directory("data/sounds"); }
		if (!is_directory("data/textures")) { create_directory("data/textures"); }
	}
	
	if (!is_directory("shaders")) { create_directory("shaders"); }
	if (!is_directory("cfg")) { create_directory("cfg"); }
	if (!is_directory("logs")) { create_directory("logs"); }
}