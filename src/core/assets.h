#pragma once
#ifndef DESHI_ASSETS_H
#define DESHI_ASSETS_H

#include "../utils/defines.h"

#include <string>
#include <vector>
#include <map>

struct Console;

enum AssetTypeBits : u32{
	AssetType_NONE, 
	AssetType_Data, AssetType_Entity, AssetType_Model,  AssetType_Texture, 
	AssetType_Save, AssetType_Sound,  AssetType_Shader, AssetType_Config, 
	AssetType_LAST
}; typedef u32 AssetType;

namespace deshi{
	
	inline static std::string dirData()    { return "data/"; }
	inline static std::string dirConfig()  { return dirData() + "cfg/"; }
	inline static std::string dirEntities(){ return dirData() + "entities/"; }
	inline static std::string dirLogs()    { return dirData() + "logs/"; }
	inline static std::string dirModels()  { return dirData() + "models/"; }
	inline static std::string dirSaves()   { return dirData() + "saves/"; }
	inline static std::string dirShaders() { return dirData() + "shaders/"; }
	inline static std::string dirSounds()  { return dirData() + "sounds/"; }
	inline static std::string dirTextures(){ return dirData() + "textures/"; }
	
	//returns "" if file not found, and logs error to console
	std::string assetPath(const char* filename, AssetType type, b32 logError = true);
	
	//reads a files contents in ASCII and returns it as a char vector
	std::vector<char> readFile(const std::string& filepath, u32 chars = 0);
	
	//reads a files contents in binary and returns it as a char vector
	std::vector<char> readFileBinary(const std::string& filepath, u32 bytes = 0);
	
	//truncates and writes a char vector to a file in ASCII format
	void writeFile(const std::string& filepath, std::vector<char>& data, u32 chars = 0);
	
	//truncates and writes a char array to a file in ASCII format
	void writeFile(const std::string& filepath, const char* data, u32 chars);
	
	//appends and writes a char array to a file in ASCII format
	void appendFile(const std::string& filepath, const char* data, u32 chars);
	
	//appends and writes a char vector array to a file in ASCII format
	void appendFile(const std::string& filepath, std::vector<char>& data, u32 chars);
	
	//truncates and writes a char vector to a file in binary
	void writeFileBinary(const std::string& filepath, std::vector<char>& data, u32 bytes = 0);
	
	//truncates and writes a char array to a file in binary
	void writeFileBinary(const std::string& filepath, const char* data, u32 bytes);
	
	//appends and writes a char array to a file in binary
	void appendFileBinary(const std::string& filepath, const char* data, u32 bytes);
	
	//iterates a config file and returns a map of keys and values (see keybinds.cfg)
	std::map<std::string, std::string> extractConfig(const std::string& filepath);
	
	//iterates directory and returns a list of files in it
	//probably return something other than a vector of strings but thts how it is for now
	std::vector<std::string> iterateDirectory(const std::string& filepath);
	
	//creates base deshi directories if they dont already exist
	void enforceDirectories();
	
} //namespace deshi

#endif //DESHI_ASSETS_H