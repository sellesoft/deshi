#pragma once
#ifndef DESHI_OPTIONAL_H
#define DESHI_OPTIONAL_H

#include <cstring>

template <typename T>
struct Optional {
	
	T a;
	Optional() {
		memset(&a, 0xFFFFFFFF, sizeof(T));
	};
	
	Optional(T a) {
		this->a = a;
	}
	
	bool test() {
		T b;
		memset(&b, 0xFFFFFFFF, sizeof(T));
		if (memcmp(&a, &b, sizeof(T)) {
			return(true);
		}
		else {
			return(false);
		}
	}
};

#endif //DESHI_OPTIONAL_H