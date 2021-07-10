#include "assets.h"
#include "../utils/debug.h"
#include "console.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <regex>

////////////////////
//// file paths ////
////////////////////

std::string Assets::
assetPath(const char* filename, AssetType type, b32 logError){
	std::string file;
	switch(type){
		case AssetType_Entity:  file = dirEntities() + filename; break;
		case AssetType_Model:   file = dirModels() + filename; break;
		case AssetType_Texture: file = dirTextures() + filename; break;
		case AssetType_Save:    file = dirSaves() + filename; break;
		case AssetType_Sound:   file = dirSounds() + filename; break;
		case AssetType_Shader:  file = dirShaders() + filename; break;
		case AssetType_Config:  file = dirConfig() + filename; break;
		case AssetType_NONE:    file = filename; break;
		default:                file = dirData() + filename; break;
	}
	if(std::filesystem::exists(std::filesystem::path(file))){
		return file;
	}else{
		if(logError) ERROR("Failed to find data: ", file);
		return "";
	}
}

/////////////////////////
//// file read-write ////
/////////////////////////

std::vector<char> Assets::
readFile(const std::string& filepath, u32 chars) {
	std::ifstream file(filepath, std::ios::ate);
	if(!file.is_open()){ ERROR("Failed to open file: ", filepath); return {}; };
	defer{ file.close(); };
	
	if(chars == 0){ chars = (u32)file.tellg(); }
	
	std::vector<char> buffer(chars);
	file.seekg(0);
	file.read(buffer.data(), chars);
	
	return buffer;
}

std::vector<char> Assets::
readFileBinary(const std::string& filepath, u32 bytes) {
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);
	if(!file.is_open()){ ERROR("Failed to open file: ", filepath); return {}; };
	defer{ file.close(); };
	
	if(bytes == 0){ bytes = (u32)file.tellg(); }
	std::vector<char> buffer(bytes);
	file.seekg(0);
	file.read(buffer.data(), bytes);
	
	return buffer;
}

char* Assets::
readFileAsciiToArray(std::string filepath, u32 chars){
	std::ifstream file(filepath, std::ifstream::in);
	if(!file.is_open()) { ERROR("Failed to open file: ", filepath.c_str()); return 0; }
	defer{ file.close(); };
	
	file.seekg(0, file.end);
	if(chars == 0) chars = file.tellg();
	
	char* buffer = new char[chars];
	file.seekg(0, file.beg);
	file.read(buffer,chars);
	return buffer;
}

char* Assets::
readFileBinaryToArray(std::string filepath, u32 bytes){
	std::ifstream file(filepath, std::ifstream::in | std::ios::binary);
	if(!file.is_open()) { ERROR("Failed to open file: ", filepath.c_str()); return 0; }
	defer{ file.close(); };
	
	file.seekg(0, file.end);
	if(bytes == 0) bytes = file.tellg();
	
	char* buffer = new char[bytes];
	file.seekg(0, file.beg);
	file.read(buffer,bytes);
	return buffer;
}

void Assets::
writeFile(const std::string& filepath, std::vector<char>& data, u32 chars){
	std::ofstream file(filepath, std::ios::out | std::ios::trunc);
	if(!file.is_open()){ ERROR("Failed to open file: ", filepath); return; }
	defer{ file.close(); };
	
	if(chars == 0){ chars = data.size(); }
	file.write(reinterpret_cast<const char*>(data.data()), chars);
}

void Assets::
writeFile(const std::string& filepath, const char* data, u32 chars){
	std::ofstream file(filepath, std::ios::out | std::ios::trunc);
	if(!file.is_open()){ ERROR("Failed to open file: ", filepath); return; }
	defer{ file.close(); };
	
	file.write(data, chars);
}

void Assets::
appendFile(const std::string& filepath, std::vector<char>& data, u32 chars){
	std::ofstream file(filepath, std::ios::out | std::ios::app);
	if(!file.is_open()){ ERROR("Failed to open file: ", filepath); return; }
	defer{ file.close(); };
	
	if(chars == 0){ chars = data.size(); }
	file.write(reinterpret_cast<const char*>(data.data()), chars);
}

void Assets::
appendFile(const std::string& filepath, const char* data, u32 chars){
	std::ofstream file(filepath, std::ios::out | std::ios::app);
	if(!file.is_open()){ ERROR("Failed to open file: ", filepath); return; }
	defer{ file.close(); };
	
	file.write(data, chars);
}

void Assets::
writeFileBinary(const std::string& filepath, std::vector<char>& data, u32 bytes){
	std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
	if(!file.is_open()){ ERROR("Failed to open file: ", filepath); return; }
	defer{ file.close(); };
	
	if(bytes == 0){ bytes = data.size(); }
	file.write(reinterpret_cast<const char*>(data.data()), bytes);
}

void Assets::
writeFileBinary(const std::string& filepath, const char* data, u32 bytes){
	std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
	if(!file.is_open()){ ERROR("Failed to open file: ", filepath); return; }
	defer{ file.close(); };
	
	file.write(data, bytes);
}

void Assets::
appendFileBinary(const std::string& filepath, const char* data, u32 bytes){
	std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::app);
	if(!file.is_open()){ ERROR("Failed to open file: ", filepath); return; }
	defer{ file.close(); };
	
	file.write(data, bytes);
}

std::vector<std::string> Assets::
iterateDirectory(const std::string& filepath) {
	using namespace std::filesystem;
	std::vector<std::string> files;
	for (auto& p : directory_iterator(filepath)) {
		files.push_back(p.path().filename().string());
	}
	return files;
}

void Assets::
enforceDirectories() {
	using namespace std::filesystem;
	if (!is_directory(dirData())) {
		create_directory(dirData());
		create_directory(dirConfig());
		create_directory(dirEntities());
		create_directory(dirLogs());
		create_directory(dirModels());
		create_directory(dirSaves());
		create_directory(dirShaders());
		create_directory(dirSounds());
		create_directory(dirTextures());
	} else {
		if (!is_directory(dirConfig()))   create_directory(dirConfig());
		if (!is_directory(dirEntities())) create_directory(dirEntities());
		if (!is_directory(dirLogs()))     create_directory(dirLogs());
		if (!is_directory(dirModels()))   create_directory(dirModels());
		if (!is_directory(dirSaves()))    create_directory(dirSaves());
		if (!is_directory(dirShaders()))  create_directory(dirShaders());
		if (!is_directory(dirSounds()))   create_directory(dirSounds());
		if (!is_directory(dirTextures())) create_directory(dirTextures());
	}
}

///////////////////////////
//// parsing utilities ////
///////////////////////////

pair<std::string, std::string> Assets::
split_keyValue(std::string str){
	size_t idx = str.find_first_of(' ');
	if (idx == -1) return pair<std::string, std::string>(str, std::string(""));
	
	std::string key = str.substr(0, idx);
	std::string val;
	size_t start = str.find_first_not_of(' ', idx);
	size_t end = -1;
	if(str[start] == '\"') {
		end = str.find_first_of('\"', start + 1);
		val = str.substr(start + 1, end - (start + 1));
	}else if(str[start] == '\'') {
		end = str.find_first_of('\'', start + 1);
		val = str.substr(start + 1, end - (start + 1));
	}else if(str[start] == '(') {
		end = str.find_first_of(')', start + 1);
		val = str.substr(start + 1, end - (start + 1));
	}else{
		end = str.find_first_of(" #\n", start + 1);
		val = str.substr(start, end - start);
	}
	
	return pair<std::string, std::string>(key, val);
}

std::map<std::string, std::string> Assets::
extractConfig(const std::string& filepath) {
	std::map<std::string, std::string> out;
	
	std::fstream in(Assets::dirConfig() + filepath, std::fstream::in);
	if(!in.is_open()){ ERROR("Failed to open file: ", filepath); out.emplace("FileNotFound", ""); return out; }
	defer{ in.close(); };
	
	std::regex r = std::regex("([A-Za-z]+) += +(.+)");
	int line = 0;
	char* c = (char*)malloc(255);
	while (!in.eof()) {
		in.getline(c, 255);
		std::string s(c);
		
		if (s[0] == '>') { line++; continue; }
		
		std::smatch m;
		std::regex_match(s, m, r);
		if (m.size() == 1) {
			ERROR_LOC(m[1], "\nConfig regex did not find a match for the string above.");
			line++;
			continue;
		}
		out.emplace(m[1], m[2]);
	}
	return out;
}

b32 Assets::
parse_bool(std::string& str, const char* filepath, u32 line_number){
	if(str == "true" || str == "1"){
		return true;
	}else if(str == "false" || str == "0"){
		return false;
	}else{
		if(filepath && line_number){
			ERROR("Error parsing '",filepath,"' on line '",line_number,"'! Invalid boolean value: ", str);
		}else{
			ERROR("Failed to parse boolean value: ", str);
		}
		return false;
	}
}