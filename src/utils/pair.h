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

//TODO make an index sequence thing of our own
#include <utility>

//from https://ldionne.com/2015/11/29/efficient-parameter-pack-indexing/
template <std::size_t n, typename = std::make_index_sequence<n>>
struct nth_element_impl;

template <std::size_t n, std::size_t ...ignore>
struct nth_element_impl<n, std::index_sequence<ignore...>> {
    template <typename Tn>
    static Tn f(decltype((void*)ignore)..., Tn*, ...);
};

template <typename T>
struct wrapper { using type = T; };

template <std::size_t n, typename ...T>
using nth_element = typename decltype(
    nth_element_impl<n>::f(static_cast<wrapper<T>*>(0)...)
)::type;

template<typename... T>
constexpr u64 Sum(){
	return (sizeof(T) + ...);
}

template<typename T, typename... Args>
constexpr T RunConstructor(Args... args){
	return T(args...);
}

template<typename... T>
struct tuple {
	u8 raw[Sum<T...>()];
	u64 offsets[sizeof...(T)];
	tuple(){
		u64 sum = 0, idx = 0;
		((offsets[idx++] = sum, 
		memcpy(((T*)&raw[sum]), 
		&RunConstructor<T>(), 
		sizeof(T)), sum+=sizeof(T)), ...);
	}

	tuple(T...args){
		u64 sum = 0, idx = 0;
		((offsets[idx++] = sum, memcpy(((T*)&raw[sum]), &args, sizeof(T)), sum+=sizeof(T)), ...);
	}

	template<upt n>
	decltype(auto) get(){
		return *(nth_element<n, T...>*)&raw[offsets[n]];
	} 

	template<upt n>
	decltype(auto) getptr(){
		return (nth_element<n, T...>*)&raw[offsets[n]];
	} 
	
	


};


#endif //DESHI_PAIR_H