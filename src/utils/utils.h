#pragma once
#ifndef UTILS_H
#define UTILS_H

#include "tuple.h"
#include "../defines.h"

#include <string>
#include <vector>
#include <cmath>

namespace Utils{
	

	
	///////////////////////////////
	//// FNV-1a hash functions ////
	///////////////////////////////
	
	u32 dataHash32(void* data, size_t data_size, u32 seed = 2166136261); //32bit FNV_offset_basis
	u64 dataHash64(void* data, size_t data_size, u64 seed = 14695981039346656037); //64bit FNV_offset_basis
	u32 stringHash32(char* data, size_t data_size = 0, u32 seed = 2166136261); //32bit FNV_offset_basis
	u64 stringHash64(char* data, size_t data_size = 0, u64 seed = 14695981039346656037); //64bit FNV_offset_basis
	
	///////////////////////////////
	//// std::string functions ////
	///////////////////////////////
	
	//returns a new string with the leading spaces removed
	std::string eatSpacesLeading(std::string text);
	
	//returns a new string with the trailing spaces removed
	std::string eatSpacesTrailing(std::string text);
	
	//returns a new string with the comments removed
	//NOTE all comment_characters are compared against to start a comment
	std::string eatComments(std::string text, const char* comment_characters);
	
	//separates a string by specified character
	std::vector<std::string> characterDelimit(std::string text, char character);
	
	//separates a string by specified character, ignores sequences of the character
	//eg: 1,,,,2,3 is the same as 1,2,3
	std::vector<std::string> characterDelimitIgnoreRepeat(std::string text, char character);
	
	//separates a string by spaces, ignores leading and trailing spaces
	std::vector<std::string> spaceDelimit(std::string text);
	
	//separates a string by spaces, ignores leading and trailing spaces
	//also ignores spaces between double quotes
	std::vector<std::string> spaceDelimitIgnoreStrings(std::string text);
	
	////////////////////////////
	//// c-string functions ////
	////////////////////////////
	
	//returns the index of the first character that is not a space
	size_t skipSpacesLeading(const char* text, size_t text_size = 0);
	
	//returns the index of the last character that is not a space
	size_t skipSpacesTrailing(const char* text, size_t text_size = 0);
	
	//returns the index of the last character which is not commented out
	size_t skipComments(const char* text, const char* comment_chararacters, size_t text_size = 0);
	
	u32 findCharFromLeft(char* text, char character, u32 offset = 0);
	
	//returns an array of start-stop index pairs to characters separated by the specified character
	//NOTE the caller is responsible for freeing the array this allocates
	pair<size_t,size_t>* characterDelimit(const char* text, char character, size_t text_size = 0);
	
	//returns an array of start-stop index pairs to characters separated by any number of the specified character
	//eg: 1,,,,2,3 is treated the same as 1,2,3
	//NOTE the caller is responsible for freeing the array this allocates
	pair<size_t,size_t>* characterDelimitIgnoreRepeat(const char* text, char character, size_t text_size = 0);
	
	//returns an array of start-stop index pairs to characters separated by spaces
	//ignores leading and trailing spaces
	//NOTE the caller is responsible for freeing the array this allocates
	pair<size_t,size_t>* spaceDelimit(const char* text, size_t text_size = 0);
	
	//returns an array of start-stop index pairs to characters separated by spaces
	//ignores leading, trailing, and double quotes encapsulated spaces 
	//eg: '1 2 "3 4" 5' creates 4 items: ["1", "2", "3 4", "5"]
	//NOTE the caller is responsible for freeing the array this allocates
	pair<size_t,size_t>* spaceDelimitIgnoreStrings(const char* text, size_t text_size = 0);
	
}; //namespace Utils


///////////////////////////////
//// FNV-1a hash functions //// //ref: https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
/////////////////////////////// //ref: imgui.cpp ImHashData https://github.com/ocornut/imgui
inline u32 Utils::
dataHash32(void* _data, size_t data_size, u32 seed){
	const u8* data = (const u8*)_data;
	while(data_size-- != 0){
		seed ^= *data++;
		seed *= 16777619; //32bit FNV_prime
	}
	return seed;
}

inline u64 Utils::
dataHash64(void* _data, size_t data_size, u64 seed){
	const u8* data = (const u8*)_data;
	while(data_size-- != 0){
		seed ^= *data++;
		seed *= 1099511628211; //64bit FNV_prime
	}
	return seed;
}

inline u32 Utils::
stringHash32(char* _data, size_t data_size, u32 seed){
	const u8* data = (const u8*)_data;
	if(data_size){
		while(data_size-- != 0){
			seed ^= *data++;
			seed *= 16777619; //32bit FNV_prime
		}
	}else{
		while(u8 c = *data++){
			seed ^= c;
			seed *= 16777619; //32bit FNV_prime
		}
	}
	return seed;
}

inline u64 Utils::
stringHash64(char* _data, size_t data_size, u64 seed){
	const u8* data = (const u8*)_data;
	if(data_size){
		while(data_size-- != 0){
			seed ^= *data++;
			seed *= 1099511628211; //64bit FNV_prime
		}
	}else{
		while(u8 c = *data++){
			seed ^= c;
			seed *= 1099511628211; //64bit FNV_prime
		}
	}
	return seed;
}


///////////////////////////////
//// std::string functions ////
///////////////////////////////
inline std::string Utils::
eatSpacesLeading(std::string text){
	size_t idx = text.find_last_not_of(' ');
	return (idx != -1) ? text.substr(idx) : "";
}

inline std::string Utils::
eatSpacesTrailing(std::string text){
	size_t idx = text.find_last_not_of(' ');
	return (idx != -1) ? text.substr(0, idx+1) : "";
}

inline std::string Utils::
eatComments(std::string text, const char* comment_characters){
	size_t idx = text.find(comment_characters);
	return (idx != -1) ? text.substr(0, idx) : text;
}

inline std::vector<std::string> Utils::
characterDelimit(std::string text, char character){
	std::vector<std::string> out;
	
	int prev = 0;
	for(int i=0; i < text.size(); ++i){
		if(text[i] == character){
			out.push_back(text.substr(prev, i-prev));
			prev = i+1;
		}
	}
	out.push_back(text.substr(prev, -1));
	
	return out;
}

inline std::vector<std::string> Utils::
characterDelimitIgnoreRepeat(std::string text, char character){
	std::vector<std::string> out;
	
	int prev = 0;
	for(int i=0; i < text.size(); ++i){
		if(text[i] == character){
			out.push_back(text.substr((upt)prev, (upt)i-prev));
			while(text[i+1] == ' ') ++i;
			prev = i+1;
		}
	}
	out.push_back(text.substr(prev, -1));
	
	return out;
}

inline std::vector<std::string> Utils::
spaceDelimit(std::string text){
	std::vector<std::string> out;
	text = eatSpacesLeading(text);
	text = eatSpacesTrailing(text);
	
	int prev = 0;
	for(int i=0; i < text.size(); ++i){
		if(text[i] == ' '){
			out.push_back(text.substr(prev, i-prev));
			while(text[i+1] == ' ') ++i;
			prev = i+1;
		}
	}
	out.push_back(text.substr(prev, -1));
	
	return out;
}

inline std::vector<std::string> Utils::
spaceDelimitIgnoreStrings(std::string text){
	std::vector<std::string> out;
	text = eatSpacesLeading(text);
	text = eatSpacesTrailing(text);
	
	size_t prev = 0, end_quote = 0;
	forI(text.size()){
		if(text[i] == ' '){
			out.push_back(text.substr(prev, i-prev));
			while(text[i+1] == ' ') ++i;
			prev = i+1;
		}
		while(text[prev] == '\"'){
			end_quote = text.find_first_of('\"', prev+1);
			if(end_quote != -1){
				out.push_back(text.substr(prev+1, end_quote-prev-1));
				i = end_quote+1;
				if(i >= text.size()) return out;
				prev = i+1;
			}else{
				Assert(!"Opening quote did not have a closing quote in the text");
				return std::vector<std::string>();
			}
		}
	}
	out.push_back(text.substr(prev, -1));
	
	return out;
}


////////////////////////////
//// c-string functions ////
////////////////////////////
//returns the index of the first character that is not a space
inline size_t Utils::
skipSpacesLeading(const char* text, size_t text_size){
	const char* cursor = text;
	if(text_size){
		while(*cursor == ' '){
			if(text_size-- == 0) break;
			cursor++;
		}
	}else{
		while(*cursor == ' ' && *cursor != '\0'){
			cursor++;
		}
	}
	return cursor-text;
}

//returns the index of the last character that is not a space
inline size_t Utils::
skipSpacesTrailing(const char* text, size_t text_size){
	const char* cursor = text;
	if(text_size){
		cursor += text_size-1;
		while(*cursor == ' '){
			if(cursor == text) return 0;
			cursor--;
		}
		cursor++;
	}else{
		cursor += strlen(text)-1;
		while(*cursor == ' '){
			if(cursor == text) return 0;
			cursor--;
		}
		cursor++;
	}
	return cursor-text;
}

inline size_t Utils::
skipComments(const char* text, const char* comment_characters, size_t text_size){
	const char* cursor = text;
	size_t comment_char_count = strlen(comment_characters);
	size_t stop = 0;
	if(text_size){
		while(strncmp(comment_characters, cursor, comment_char_count) != 0){
			if(text_size-- == 0) return stop;
			cursor++;
			stop++;
		}
	}else{
		while(strncmp(comment_characters, cursor, comment_char_count) != 0){
			if(*cursor == '\0') return stop;
			cursor++;
			stop++;
		}
	}
	return stop;
}

inline u32 Utils::
findCharFromLeft(char* text, char character, u32 offset){
	for(u32 i = offset; ;++i){
		if(text[i] == character) return i;
		if(text[i] == '\0') break;
	}
	return -1;
}

#endif //UTILS_H
