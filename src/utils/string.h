#pragma once
#include "../defines.h"
#include "tuple.h"

#include <stdlib.h>
#include <iostream>
#include <vector> //temp and should be removed
#include <string> //temp and should be removed

#define __FILENAME__ (std::strrchr(__FILE__, '\\') ? std::strrchr(__FILE__, '\\') + 1 : __FILE__)
#define ADDRUPDATE(str, size, create) 0//addrupdate((void*)str, size, __FILENAME__, __func__, create)

global_ std::vector<pair<u32, void*, std::string>> addrs;
global_ bool track = false;
global_ u32 inscount = 0;

global_ void setTrack() { track = true; inscount = 0; }
global_ void endTrack() { track = false; }

global_ void addrupdate(void* addr, u32 size, const char* file, const char* func, bool create = true) {
	if (track) {
		std::cout << "\033c";
		char* str = (char*)malloc(255);
		strncpy(str, (char*)addr, size);
		str[size] = '\0';
		if (create) {
			addrs.push_back(pair<u32, void*, std::string>(inscount, addr, std::string(func)));
			std::cout << "added addr   " << (int)addr << " ~ " << str << std::endl;
		}
		else {
			for (int i = 0; i < addrs.size(); i++) {
				if (addrs[i].second == addr) {
					std::cout << "erased addr   " << (int)addr << " at idx " << addrs[i].first << " ~ " << str <<  std::endl;
					addrs.erase(addrs.begin() + i);
					break;
				}
			}
		}
		
		std::cout << "CURRENT ADDR LIST WITH " << inscount << " INSTRUCTIONS AND " << addrs.size() << " ADDRS:" << std::endl;
		
		for (int i = 0; i < addrs.size(); i++) {
			std::cout << addrs[i].first << ".  " << (int)addrs[i].second << " - " << addrs[i].third << " ~ " << (char*)addrs[i].second << std::endl;
		}
		std::cout << std::endl;
		free(str);
		inscount++;
	}
}

//TODO(delle) add reserved space on string for small strings, offset it to a proper padding
struct string {
	char* str = 0;
	int size = 0;
	
	//returned when something specified is not found in a search fucntion
	static const size_t npos = -1;
	
	string() {
		size = 0;
		ADDRUPDATE(str, 0, 1);
	};
	
	string(const char c) {
		size = 1;
		str = (char*)malloc(1 + 1);
		str[0] = c;
		str[1] = '\0';
		ADDRUPDATE(str, size, 1);
	}
	
	string(const char* s) {
		size = strlen(s);
		if (size != 0) {
			str = (char*)malloc(size + 1);
			strcpy(str, s);
			ADDRUPDATE(str, size, 1);
		}
		else {
			//str = new char[1];
			str = (char*)malloc(1);
			memset(str, '\0', 1);
			ADDRUPDATE(str, size, 1);
		}
	}
	
	string(const char* s, size_t size) {
		this->size = size;
		if (size != 0) {
			str = (char*)malloc(size + 1);
			memcpy(str, s, size);
			memset(str + size, '\0', 1);
			ADDRUPDATE(str, size, 1);
		}
		else {
			str = (char*)malloc(1);
			memset(str, '\0', 1);
			ADDRUPDATE(str, size, 1);
		}
	}
	
	string(const string& s) {
		
		size = s.size;
		if (size != 0) {
			free(str);
			str = (char*)malloc(size + 1);
			strcpy(str, s.str);
			ADDRUPDATE(str, size, 1);
		}
		else {
			free(str);
			str = (char*)malloc(1);
			size = 0;
			memset(str, '\0', 1);
			ADDRUPDATE(str, 0, 1);
		}
	}
	
	//temp
	string(std::string s) {
		free(str);
		size = s.size();
		str = (char*)malloc(size + 1);
		memcpy(str, s.c_str(), size);
		memset(str + size, '\0', 1);
	}
	
	~string() {
		ADDRUPDATE(str, size, 0);
		free(str);
		str = nullptr;
		size = 0;
	}
	
	char& operator[](int i) {
		//assert that index is less than str size
		return str[i];
	}
	
	void operator = (char c) {
		size = 1;
		str = (char*)malloc(size + 1);
		memset(str, c, 2);
		memset(str + 1, '\0', 1);
		ADDRUPDATE(str, size, 1);
	}
	
	void operator = (const string& s) {
		ADDRUPDATE(str, size, 0);
		free(str);
		size = s.size;
		str = (char*)malloc(size + 1);
		memcpy(str, s.str, size + 1);
		memset(str + size, '\0', 1);
		ADDRUPDATE(str, size, 1);
	}
	
	void operator = (const char* s) {
		size = strlen(s);
		str = (char*)malloc(size + 1);
		strcpy(str, s);
		ADDRUPDATE(str, size, 1);
	}

	bool operator == (string s) {
		return !strcmp(str, s.str);
	}
	
	bool operator == (const char* s) {
		return !strcmp(str, s);
	}
	
	bool operator == (char c) {
		if(size == 1 && *str == c) return true;
		return false;
	}
	
	//these could probably be better
	void operator += (char& c) {
		int newsize = size + 1;
		char* old = new char[size];
		memcpy(old, str, size);
		str = (char*)malloc(newsize + 1);
		memcpy(str, old, size);
		memcpy(str + size, &c, 1);
		size = newsize;
		memset(str + size, '\0', 1);
		ADDRUPDATE(str, size, 1);
		delete old;
	}
	
	//these could probably be better
	void operator += (const string& s) {
		if (s.size == 0) return;
		int newsize = size + s.size;
		str = (char*)realloc(str, newsize + 1);
		memcpy(str + size, s.str, s.size);
		size = newsize;
		memset(str + size, '\0', 1);
		ADDRUPDATE(str, size, 1);
	}
	
	//these could probably be better
	void operator += (const char* ss) {
		string s(ss); //being lazy
		if (s.size == 0) return;
		int newsize = size + s.size;
		char* old = new char[size];
		memcpy(old, str, size);
		str = (char*)malloc(newsize + 1);
		memcpy(str, old, size);
		memcpy(str + size, s.str, s.size);
		size = newsize;
		memset(str + size, '\0', 1);
		ADDRUPDATE(str, size, 1);
		delete old;
	}
	
	string operator + (string s) {
		if (s.size == 0) return *this;
		int newsize = size + s.size;
		char* old = new char[size];
		memcpy(old, str, size);
		string nustr;
		nustr.str = (char*)malloc(newsize + 1);
		memcpy(nustr.str, old, size);
		memcpy(nustr.str + size, s.str, s.size);
		nustr.size = newsize;
		memset(nustr.str + nustr.size, '\0', 1);
		delete old;
		return nustr;
	}
	
	string operator + (const char* c) {
		string s(c);
		return this->operator+(s);
	}

	string operator -- (int) {
		if (size != 1) {
			str = (char*)realloc(str, size - 1);
			size--;
			memset(str + size, '\0', 1);
		}
		else {
			str = (char*)realloc(str, 1);
			size = 0;
			memset(str, '\0', 1);
		}
		return *this;

	}	
	
	//const func for getting a char in a string
	char at(u32 idx) const {
		return str[idx];
	}
	
	void clear() {
		memset(str, 0, size + 1);
		str = (char*)realloc(str, 1);
		str[0] = '\0';
		size = 0;
	}
	
	void erase(u32 idx) {
		Assert(idx <= size && idx >= 0 && size != 0);
		if (size - 1 != 0) {
			for (int i = idx; i < size - 1; i++)
				str[i] = str[i + 1];
			size--;
			str = (char*)realloc(str, size + 1);
			memset(str + size, '\0', 1);
		}
		else {
			size = 0;
			str = (char*)realloc(str, 1);
			memset(str, '\0', 1);
		}
	}

	//inserts after specified idx, so an idx of -1 inserts at the begining
	void insert(char c, s32 idx) {
		Assert(idx <= size && idx >= -1);
		
		idx++;
		size++;
		str = (char*)realloc(str, size + 1);
		for (s32 i = size - 1; i >= idx; i--)
			str[i] = str[i - 1];
		str[idx] = c;
	}
	
	
	//https://cp-algorithms.com/string/string-hashing.html
	static u64 hash(string str) {
		const int p = 31;
		const int m = 1e9 + 9;
		u64 hash_value = 0;
		u64 p_pow = 1;
		for (int i = 0; i < str.size; i++) {
			hash_value = (hash_value + (str.str[i] - 'a' + 1) * p_pow) % m;
			p_pow = (p_pow * p) % m;
		}
		return hash_value;
	}
	
	static int stoi(string str) {
		int x;
		sscanf(str.str, "%d", &x);
		return x;
	}
	
	static string toStr(int i) {
		string s;
		s.size = (i == 0) ? 1 : (int)((floor(log10(i)) + 1) * sizeof(char));
		free(s.str);
		s.str = (char*)malloc(s.size + 1);
		sprintf(s.str, "%d", i);
		ADDRUPDATE(s.str, s.size, 1);
		return s;
	}

	static string toStr(u32 i) {
		string s;
		s.size = (i == 0) ? 1 : (int)((floor(log10(i)) + 1) * sizeof(char));
		free(s.str);
		s.str = (char*)malloc(s.size + 1);
		sprintf(s.str, "%d", i);
		ADDRUPDATE(s.str, s.size, 1);
		return s;
	}
	
	static string toStr(float f) {
		string s;
		s.size = snprintf(nullptr, 0, "%f", f);
		s.str = (char*)malloc(s.size + 1);
		snprintf(s.str, s.size + 1, "%f", f);
		ADDRUPDATE(s.str, s.size, 1);
		return s;
	}

	static string toStr(size_t f) {
		string s;
		s.size = snprintf(nullptr, 0, "%zu", f);
		s.str = (char*)malloc(s.size + 1);
		snprintf(s.str, s.size + 1, "%zu", f);
		ADDRUPDATE(s.str, s.size, 1);
		return s;
	}
	
	friend string operator + (const char* c, string s);
	
	inline string substr(size_t first, size_t second = npos) const {
		if(second == npos) second = size;
		Assert(first <= size && second <= size && second >= first, "check first/second variables");
		return string(str + first, second - first + 1);
	}
	
	inline size_t find(const string& text) const {
		for (int i = 0; i < size - (text.size - 1); i++) {
			//dont use strcmp if text.size is only 1
			if (text.size == 1)
				if (str[i] == text.str[0])
				return i;
			
			//early cont if char doesnt match first char of input
			if (str[i] != text.str[0]) continue;
			else if(!strcmp(substr(i, i + text.size - 1).str, text.str)) {
				return i;
			}
		}
		return npos;
	}
	
	inline size_t find_first_of(char c) const {
		for (int i = 0; i < size; i++) {
			if (c == str[i]) return i;
		}
		return npos;
	}
	
	//find first of from offset
	inline size_t find_first_of(char c, int offset) const {
		Assert(offset < size, "attempt to parse string at offset greater than size");
		for (int i = offset; i < size; i++) {
			if (c == str[i]) return i;
		}
		return npos;
	}
	
	//find first of from offset backwards
	//TODO(sushi) make this for the other functions
	inline size_t find_first_of_lookback(char c, int offset = 0) const {
		Assert(offset < size, "attempt to parse string at offset greater than size");
		for (int i = offset; i > 0; i--) {
			if (c == str[i]) return i;
		}
		return npos;
	}
	
	inline size_t find_first_not_of(char c) const {
		for (int i = 0; i < size; i++) {
			if (c != str[i]) return i;
		}
		return npos;
	}
	
	inline size_t find_last_of(char c) const {
		for (int i = size - 1; i != 0; i--) {
			if (c == str[i]) return i;
		}
		return npos;
	}
	
	inline size_t find_last_not_of(char c) const {
		for (int i = size - 1; i != 0; i--) {
			if (c != str[i]) return i;
		}
		return npos;
	}
	
	//counts how many characters are in the string
	inline u32 count(char c) {
		u32 sum = 0;
		for (int i = 0; i < size; i++) if (str[i] == c) sum++;
		return sum;
	}
	
	inline string substrToChar(char c) const {
		size_t first = find_first_of(c);
		if(first == npos){
			return string(str);
		}else{
			return string(str, first);
		}
	}
	
	static inline string eatSpacesLeading(string text) {
		size_t idx = text.find_first_of(' ');
		return (idx != npos) ? text.substr(idx) : "";
	}
	
	static inline string eatSpacesTrailing(string text) {
		size_t idx = text.find_last_not_of(' ');
		return (idx != npos) ? text.substr(0, idx + 1) : "";
	}
};


inline std::ostream& operator<<(std::ostream& os, string& m) {
	return os << m.str;
}

inline string operator + (const char* c, string s) {
	if (s.size == 0) {
		string why_do_i_have_to_do_this(c);
		return why_do_i_have_to_do_this;
	}
	string st(c);
	return st + s;
}


