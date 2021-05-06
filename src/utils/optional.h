#pragma once
#ifndef DESHI_OPTIONAL_H
#define DESHI_OPTIONAL_H

#include <type_traits>

template <typename T>
struct Optional {
	T value;
	bool has_value;
	
	Optional() {
		this->has_value = false;
	};
	
	Optional(T value) {
		this->value = value;
		this->has_value = true;
	}
	
	inline bool test(){
		return has_value;
	}
	
	inline void reset(){
		has_value = false;
	}
	
	inline bool operator=(Optional& rhs){
		has_value = true;
		return value = rhs.value;
	}
	
	inline bool operator=(T& rhs){
		has_value = true;
		return value = rhs;
	}
	
	inline bool operator==(Optional& rhs){
		return value == rhs.value;
	}
	
	inline bool operator!=(Optional& rhs){
		return value != rhs.value;
	}
	
	inline explicit operator bool(){ return has_value; }
};

#endif //DESHI_OPTIONAL_H