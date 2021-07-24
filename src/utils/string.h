#pragma once
#include <stdlib.h>
#include <iostream>

#include "../defines.h"

struct string {
	char* str;
	int size = 0;

	string(){};

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

	void clear() {
		memset(str, 0, size + 1);
		str = (char*)realloc(str, 1);
		str[0] = '\0';
		size = 0;
	}


	//https://cp-algorithms.com/string/string-hashing.html
	u64 hash(string str) {
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
};


inline std::ostream& operator<<(std::ostream& os, string& m) {
	return os << m.str;
}

string operator + (const char* c, string s) {
	if (s.size == 0) {
		string why_do_i_have_to_do_this(c);
		return why_do_i_have_to_do_this;
	}
	string st(c);
	return st + s;
}


