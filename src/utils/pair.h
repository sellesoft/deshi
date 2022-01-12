#pragma once
#ifndef DESHI_PAIR_H
#define DESHI_PAIR_H

//base case, never instantiated
template<typename... Dummy> struct pair;

template<typename T, typename U>
struct pair<T,U> {
	T first;
	U second;
	
	pair(){ first={}; second={}; }
	pair(T first, U second) {
		this->first = first;
		this->second = second;
	}
};

template<typename T, typename U, typename V>
struct pair<T,U,V> {
	T first;
	U second;
	V third;
	
	pair(){ first={}; second={}; third={}; }
	pair(T first, U second, V third) {
		this->first = first;
		this->second = second;
		this->third = third;
	}
};

template<typename T, typename U, typename V, typename W>
struct pair<T,U,V,W> {
	T first;
	U second;
	V third;
	W fourth;
	
	pair(){ first={}; second={}; third={}; fourth={}; }
	pair(T first, U second, V third, W fourth) {
		this->first = first;
		this->second = second;
		this->third = third;
		this->fourth = fourth;
	}
};

template<typename T, typename U, typename V, typename W, typename X>
struct pair<T,U,V,W,X> {
	T first;
	U second;
	V third;
	W fourth;
	X fifth;
	
	pair(){ first={}; second={}; third={}; fourth={}; fifth={}; }
	pair(T first, U second, V third, W fourth, X fifth) {
		this->first = first;
		this->second = second;
		this->third = third;
		this->fourth = fourth;
		this->fifth = fifth;
	}
};

template<typename T, typename U>
static pair<T, U> make_pair(const T& first, const U& second) {
	return pair<T, U>(first, second);
}

#endif //DESHI_PAIR_H