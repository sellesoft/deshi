#pragma once
#ifndef DESHI_OPTIONAL_H
#define DESHI_OPTIONAL_H

template <class T>
struct optional {
	T value;
	bool has_value;
	
	optional() {
		this->has_value = false;
	};
	
	optional(T& value) {
		this->value = value;
		this->has_value = true;
	}
	
	inline bool test(){
		return has_value;
	}
	
	inline void reset(){
		has_value = false;
		//memset(&value, 0xFFFFFFFF, sizeof(T));
	}
	
	inline void operator=(optional& rhs){
		has_value = true;
		value = rhs.value;
	}
	
	inline void operator=(T& rhs){
		has_value = true;
		value = rhs;
	}
	
	inline bool operator==(optional& rhs){
		return value == rhs.value;
	}
	
	inline bool operator!=(optional& rhs){
		return value != rhs.value;
	}


	// :)
	T& operator()() {
		return value;
	}


	T* operator &() {
		return &value;
	}

	//TODO(sushi) figure out a way to allow grabbing the values/functions the templated value has without having to use an operator/function >:)
	inline T* getptr() {
		return &value;
	}
	
	inline explicit operator bool(){ return has_value; }
};

#endif //DESHI_OPTIONAL_H