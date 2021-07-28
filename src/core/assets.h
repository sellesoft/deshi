#pragma once
#ifndef DESHI_ASSETS_H
#define DESHI_ASSETS_H

#include "../defines.h"
#include "../utils/tuple.h"

#include <string>
#include <vector>
#include <map>

enum ConfigValueTypeBits{
	ConfigValueType_NONE = 0, //can be used for comments/padding
	ConfigValueType_S32,
	ConfigValueType_Bool,
	ConfigValueType_U32,
	ConfigValueType_U8,
	ConfigValueType_F32, 
	ConfigValueType_F64, 
	ConfigValueType_FV2, 
	ConfigValueType_FV3,
	ConfigValueType_FV4,
	ConfigValueType_CString, //NOTE pointer to 'const char*'; also, this doesnt free allocated memory when set to a new value
	ConfigValueType_StdString, 
	ConfigValueType_Key,
	
	//control enums
	ConfigValueType_PADSECTION,
	ConfigValueType_COUNT,
	
	//enum renames
	ConfigValueType_Int     = ConfigValueType_S32,
	ConfigValueType_Float   = ConfigValueType_F32,
	ConfigValueType_Double  = ConfigValueType_F64,
	ConfigValueType_Vector2 = ConfigValueType_FV2,
	ConfigValueType_Vector3 = ConfigValueType_FV3,
	ConfigValueType_Vector4 = ConfigValueType_FV4,
}; typedef u32 ConfigValueType;

typedef std::vector<pair<const char*,ConfigValueType,void*>> ConfigMap;

namespace Assets{
	
	inline static std::string dirData()    { return "data/"; }
	inline static std::string dirConfig()  { return dirData() + "cfg/"; }
	inline static std::string dirEntities(){ return dirData() + "entities/"; }
	inline static std::string dirFonts()   { return dirData() + "fonts/"; }
	inline static std::string dirLevels()  { return dirData() + "levels/"; }
	inline static std::string dirLogs()    { return dirData() + "logs/"; }
	inline static std::string dirModels()  { return dirData() + "models/"; }
	inline static std::string dirSaves()   { return dirData() + "saves/"; }
	inline static std::string dirShaders() { return dirData() + "shaders/"; }
	inline static std::string dirSounds()  { return dirData() + "sounds/"; }
	inline static std::string dirTemp()    { return dirData() + "temp/"; }
	inline static std::string dirTextures(){ return dirData() + "textures/"; }
	
	//returns true if the file was deleted
	bool deleteFile(std::string& filepath, bool logError = true);
	
	//returns the number of files (and dirs) deleted
	u64 deleteDirectory(std::string& dirpath, bool logError = true);
	
	//reads a files contents in ASCII and returns it as a char vector
	std::vector<char> readFile(const std::string& filepath, u32 chars = 0, bool logError = true);
	
	//reads a files contents in binary and returns it as a char vector
	std::vector<char> readFileBinary(const std::string& filepath, u32 bytes = 0, bool logError = true);
	
	//returns a char array of a file's contents in ASCII, returns 0 if failed
	//NOTE the caller is responsible for freeing the array this allocates
	char* readFileAsciiToArray(std::string filepath, u32 chars = 0, bool logError = true);
	
	//returns a char array of a file's contents in binary, returns 0 if failed
	//NOTE the caller is responsible for freeing the array this allocates
	char* readFileBinaryToArray(std::string filepath, u32 bytes = 0, bool logError = true);
	
	//truncates and writes a char vector to a file in ASCII format
	void writeFile(const std::string& filepath, std::vector<char>& data, u32 chars = 0, bool logError = true);
	
	//truncates and writes a char array to a file in ASCII format
	void writeFile(const std::string& filepath, const char* data, u32 chars, bool logError = true);
	
	//appends and writes a char array to a file in ASCII format
	void appendFile(const std::string& filepath, const char* data, u32 chars, bool logError = true);
	
	//appends and writes a char vector array to a file in ASCII format
	void appendFile(const std::string& filepath, std::vector<char>& data, u32 chars, bool logError = true);
	
	//truncates and writes a char vector to a file in binary
	void writeFileBinary(const std::string& filepath, std::vector<char>& data, u32 bytes = 0, bool logError = true);
	
	//truncates and writes a char array to a file in binary
	void writeFileBinary(const std::string& filepath, const char* data, u32 bytes, bool logError = true);
	
	//appends and writes a char array to a file in binary
	void appendFileBinary(const std::string& filepath, const char* data, u32 bytes, bool logError = true);
	
	//iterates directory and returns a list of files in it
	//probably return something other than a vector of strings but thts how it is for now
	std::vector<std::string> iterateDirectory(const std::string& filepath);
	
	//creates base deshi directories if they dont already exist
	void enforceDirectories();
	
	///////////////////////////
	//// parsing utilities ////
	///////////////////////////
	
	//splits a string by space and does special case checking for comments, strings, and parenthesis
	//for use in deshi text-file parsing, ignores leading and trailing spaces
	pair<std::string, std::string> split_keyValue(std::string str);
	
	//iterates a config file and returns a map of keys and values (see keybinds.cfg)
	std::map<std::string, std::string> extractConfig(const std::string& filepath);
	
	bool parse_bool(std::string& str, const char* filepath = 0, u32 line_number = 0);
	
	void saveConfig(const char* filename, const ConfigMap& configMap);
	
	void loadConfig(const char* filename, ConfigMap configMap);
	
} //namespace Assets

#endif //DESHI_ASSETS_H