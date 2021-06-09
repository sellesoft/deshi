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

std::string deshi::
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

std::vector<char> deshi::
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

std::vector<char> deshi::
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

char* deshi::
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

char* deshi::
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

void deshi::
writeFile(const std::string& filepath, std::vector<char>& data, u32 chars){
	std::ofstream file(filepath, std::ios::out | std::ios::trunc);
	if(!file.is_open()){ ERROR("Failed to open file: ", filepath); return; }
	defer{ file.close(); };
	
	if(chars == 0){ chars = data.size(); }
	file.write(reinterpret_cast<const char*>(data.data()), chars);
}

void deshi::
writeFile(const std::string& filepath, const char* data, u32 chars){
	std::ofstream file(filepath, std::ios::out | std::ios::trunc);
	if(!file.is_open()){ ERROR("Failed to open file: ", filepath); return; }
	defer{ file.close(); };
	
	file.write(data, chars);
}

void deshi::
appendFile(const std::string& filepath, std::vector<char>& data, u32 chars){
	std::ofstream file(filepath, std::ios::out | std::ios::app);
	if(!file.is_open()){ ERROR("Failed to open file: ", filepath); return; }
	defer{ file.close(); };
	
	if(chars == 0){ chars = data.size(); }
	file.write(reinterpret_cast<const char*>(data.data()), chars);
}

void deshi::
appendFile(const std::string& filepath, const char* data, u32 chars){
	std::ofstream file(filepath, std::ios::out | std::ios::app);
	if(!file.is_open()){ ERROR("Failed to open file: ", filepath); return; }
	defer{ file.close(); };
	
	file.write(data, chars);
}

void deshi::
writeFileBinary(const std::string& filepath, std::vector<char>& data, u32 bytes){
	std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
	if(!file.is_open()){ ERROR("Failed to open file: ", filepath); return; }
	defer{ file.close(); };
	
	if(bytes == 0){ bytes = data.size(); }
	file.write(reinterpret_cast<const char*>(data.data()), bytes);
}

void deshi::
writeFileBinary(const std::string& filepath, const char* data, u32 bytes){
	std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
	if(!file.is_open()){ ERROR("Failed to open file: ", filepath); return; }
	defer{ file.close(); };
	
	file.write(data, bytes);
}

void deshi::
appendFileBinary(const std::string& filepath, const char* data, u32 bytes){
	std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::app);
	if(!file.is_open()){ ERROR("Failed to open file: ", filepath); return; }
	defer{ file.close(); };
	
	file.write(data, bytes);
}

std::vector<std::string> deshi::
iterateDirectory(const std::string& filepath) {
	using namespace std::filesystem;
	std::vector<std::string> files;
	for (auto& p : directory_iterator(filepath)) {
		files.push_back(p.path().filename().string());
	}
	return files;
}

void deshi::
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

std::string deshi::
eat_spaces_leading(std::string str){
	size_t idx = str.find_first_not_of(' ');
	return (idx != -1) ? str.substr(idx) : "";
}

std::string deshi::
eat_spaces_trailing(std::string str){
	size_t idx = str.find_last_not_of(' ');
	return (idx != -1) ? str.substr(0, idx+1) : "";
}

std::string deshi::
eat_comments(std::string str){
	size_t idx = str.find_first_of('#');
	return (idx != -1) ? str.substr(0, idx) : str;
}

std::vector<std::string> deshi::
character_delimit(std::string str, char character){
	std::vector<std::string> out;
	
	int prev = 0;
	for(int i=0; i < str.size(); ++i){
		if(str[i] == character){
			out.push_back(str.substr(prev, i-prev));
    		prev = i+1;
		}
	}
	out.push_back(str.substr(prev, -1));
	
	return out;
}

std::vector<std::string> deshi::
character_delimit_ignore_repeat(std::string str, char character){
	std::vector<std::string> out;
	
	int prev = 0;
	for(int i=0; i < str.size(); ++i){
		if(str[i] == character){
			out.push_back(str.substr(prev, i-prev));
			while(str[i+1] == ' ') ++i;
    		prev = i+1;
		}
	}
	out.push_back(str.substr(prev, -1));
	
	return out;
}

std::vector<std::string> deshi::
space_delimit(std::string str){
	std::vector<std::string> out;
	str = eat_spaces_leading(str);
	str = eat_spaces_trailing(str);
	
	int prev = 0;
	for(int i=0; i < str.size(); ++i){
		if(str[i] == ' '){
			out.push_back(str.substr(prev, i-prev));
			while(str[i+1] == ' ') ++i;
    		prev = i+1;
		}
	}
	out.push_back(str.substr(prev, -1));
	
	return out;
}

std::vector<std::string> deshi::
space_delimit_ignore_strings(std::string str){
	std::vector<std::string> out;
	str = eat_spaces_leading(str);
	str = eat_spaces_trailing(str);
	
	size_t prev = 0, end_quote = 0;
	for_n(i, str.size()){
		if(str[i] == ' '){
			out.push_back(str.substr(prev, i-prev));
			while(str[i+1] == ' ') ++i;
			prev = i+1;
    		while(str[prev] == '\"'){
    		    end_quote = str.find_first_of('\"', prev+1);
    		    if(end_quote != -1){
    		        out.push_back(str.substr(prev+1, end_quote-prev-1));
    		        i = end_quote+1;
					if(i >= str.size()) return out;
    		        prev = i+1;
    		    }else{
    		        ERROR_LOC("Opening quote did not have a closing quote in string:\n\t", str);
    		        return std::vector<std::string>();
    		    }
    		}
		}
	}
	out.push_back(str.substr(prev, -1));
	
	return out;
}

pair<std::string, std::string> deshi::
split_keyValue(std::string str){
	size_t idx = str.find_first_of(' ');
	if (idx == -1) return make_pair<std::string, std::string>(str, std::string(""));
	
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
	
	return make_pair<std::string, std::string>(key, val);
}

std::map<std::string, std::string> deshi::
extractConfig(const std::string& filepath) {
	std::map<std::string, std::string> out;
	
	std::fstream in(deshi::dirConfig() + filepath, std::fstream::in);
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

b32 deshi::
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