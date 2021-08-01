#pragma once
#include <stdlib.h>
#include <iostream>

#include "../defines.h"

struct string {
	char* str;
	int size = 0;

	string() { str = nullptr; };

	string(const char c) {
		size = 1;
		str = new char[1 + 1];
		str[0] = c;
		str[1] = '\0';
	}

	string(const char* s) {
		size = strlen(s);
		if (size != 0) {
			str = new char[size + 1];
			strcpy(str, s);
		}
		else {
			str = new char[1];
			memset(str, '\0', 1);
		}
	}

	string(const char* s, size_t size) {
		this->size = size;
		if (size != 0) {
			str = new char[size + 1];
			memcpy(str, s, size);
			memset(str + size, '\0', 1);
			//strcpy(str, s);
		}
		else {
			str = new char[1];
			memset(str, '\0', 1);
		}
	}

	string(const string& s) {
		size = s.size;
		if (size != 0) {
			str = new char[size + 1];
			strcpy(str, s.str);
		}
		else {
			str = new char[1];
			memset(str, '\0', 1);
		}
	}

	~string() {
		if (str) free(str);
		str = nullptr;
		size = 0;
	}

	char& operator[](int i) {
		//assert that index is less than str size
		return str[i];
	}

	void operator = (char c) {
		size = 1;
		str = new char[size + 1];
		memset(str, c, 2);
		memset(str + 1, '\0', 1);
	}

	void operator = (string s) {
		size = s.size;
		str = new char[size + 1];
		memcpy(str, s.str, size + 1);
		memset(str + size, '\0', 1);
	}

	void operator = (const char* s) {
		size = strlen(s);
		str = new char[size + 1];
		strcpy(str, s);
	}

	bool operator == (string& s) {
		return !strcmp(str, s.str);
	}

	bool operator == (const char* s) {
		return !strcmp(str, s);
	}

	bool operator == (char c) {
		if (*str == c) return true;
		return false;
	}

	//these could probably be better
	void operator += (char& c) {
		int newsize = size + 1;
		char* old = new char[size];
		memcpy(old, str, size);
		str = new char[newsize + 1];
		memcpy(str, old, size);
		memcpy(str + size, &c, 1);
		size = newsize;
		memset(str + size, '\0', 1);
		delete old;
	}

	//these could probably be better
	void operator += (string s) {
		if (s.size == 0) return;
		int newsize = size + s.size;
		char* old = new char[size];
		memcpy(old, str, size);
		str = new char[newsize + 1];
		memcpy(str, old, size);
		memcpy(str + size, s.str, s.size);
		size = newsize;
		memset(str + size, '\0', 1);
		delete old;
	}

	//these could probably be better
	void operator += (const char* ss) {
		string s(ss); //being lazy
		if (s.size == 0) return;
		int newsize = size + s.size;
		char* old = new char[size];
		memcpy(old, str, size);
		str = new char[newsize + 1];
		memcpy(str, old, size);
		memcpy(str + size, s.str, s.size);
		size = newsize;
		memset(str + size, '\0', 1);
		delete old;
	}

	string operator + (string s) {
		if (s.size == 0) return *this;
		int newsize = size + s.size;
		char* old = new char[size];
		memcpy(old, str, size);
		string nustr;
		nustr.str = new char[newsize + 1];
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

	friend string operator + (const char* c, string s);


	//static helper functions

	string substr(size_t first, size_t second) const {
		Assert(first <= size && second <= size && second >= first, "check first/second variables");
		return string(str + first, second - first + 1);
	}

	string substr(size_t idx) {
		Assert(idx <= size, "idx greater than size");
		return substr(idx, size);
	}

	//returned when something specified is not found in a fucntion
	static const size_t npos = -1;

	size_t find(const string& text) const {
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

	size_t find_first_of(char c) const {
		for (int i = 0; i < size; i++) {
			if (c == str[i]) return i;
		}
		return npos;
	}

	//find first of from offset
	size_t find_first_of(char c, int offset) const {
		Assert(offset < size, "attempt to parse string at offset greater than size");
		for (int i = offset; i < size; i++) {
			if (c == str[i]) return i;
		}
		return npos;
	}

	//find first of from offset backwards
	//TODO(sushi) make this for the other functions
	size_t find_first_of_lookback(char c, int offset) const {
		Assert(offset < size, "attempt to parse string at offset greater than size");
		for (int i = offset; i > 0; i--) {
			if (c == str[i]) return i;
		}
		return npos;
	}

	size_t find_first_not_of(char c) const {
		for (int i = 0; i < size; i++) {
			if (c != str[i]) return i;
		}
		return npos;
	}

	size_t find_last_of(char c) const {
		for (int i = size - 1; i != 0; i--) {
			if (c == str[i]) return i;
		}
		return npos;
	}

	size_t find_last_not_of(char c) const {
		for (int i = size - 1; i != 0; i--) {
			if (c != str[i]) return i;
		}
		return npos;
	}

	inline string substrToChar(char c) const {
		return string(str, find_first_of(c));
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


