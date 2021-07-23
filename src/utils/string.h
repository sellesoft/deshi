#pragma once
#include <stdlib.h>
#include <iostream>

#include "../defines.h"

struct string {
	char* str;
	int size = 0;
	bool do_i_own_this = true;

	string() {}
	string(const char c);
	string(const char* s);
	string(const char* s, int size);
	string(const string& s);

	~string();

	char& operator[](int i);
	void operator = (char c);
	void operator = (string s);
	void operator = (const char* s);
	bool operator == (string& s);
	bool operator == (const char* c);
	bool operator == (char c);
	void operator += (const char* c);
	void operator += (string s);
	void operator += (char& c);
	string operator + (string s);
	string operator + (const char* c);
	friend string operator + (const char* c, string s);
	void clear();

	static u64 hash(string str);
	static int stoi(string str) {
		int x;
		sscanf(str.str, "%d", &x);
		return x;
	}

	//void* operator new(size_t size, void* place);
	//void operator delete(void* ptr) noexcept;

};


inline std::ostream& operator<<(std::ostream& os, string& m) {
	return os << m.str;
}

