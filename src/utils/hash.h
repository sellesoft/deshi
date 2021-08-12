#pragma once
#ifndef DESHI_HASH_H
#define DESHI_HASH_H

#include "array.h"
#include "string.h"
#include "../defines.h"
#include "../math/vectormatrix.h"

template<class T>
struct hash {
	
	hash() {};
	
	inline u32 operator()(const T v)const {
		u32 seed = 2166136261;
		size_t data_size = sizeof(T);
		const u8* data = (const u8*)&v;
		while (data_size-- != 0) {
			seed ^= *data++;
			seed *= 16777619;
		}
		return seed;
		
	}
	
	inline u32 operator()(T* v)const {
		u32 seed = 2166136261;
		size_t data_size = sizeof(T);
		const u8* data = (const u8*)v;
		while (data_size-- != 0) {
			seed ^= *data++;
			seed *= 16777619;
		}
		return seed;
		
	}
};

template<> 
struct hash<string> {
	inline u32 operator()(string s) {
		u32 seed = 2166136261;
		u32 size = s.size+1;
		while (size-- != 0) {
			seed ^= s[size];
			seed *= 16777619;
		}
		return seed;
	}
};

template<class T> 
struct hash<array<T>> {
	inline u32 operator()(array<T>* s) {
		u32 seed = 2166136261;
		u32 size = s->size;
		while (size-- != 0) {
			seed ^= s->operator[](size - 1);
			seed *= 16777619;
		}
		return seed;
	}
};

template<> 
struct hash<vec2>{
	inline size_t operator()(vec2 const& v) const{
		size_t seed = 0;
		hash<float> hasher; size_t hash;
		hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		return seed;
	}
};

template<> 
struct hash<vec3>{
	inline size_t operator()(vec3 const& v) const{
		size_t seed = 0;
		hash<float> hasher; size_t hash;
		hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.z); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		return seed;
	}
};


template<> 
struct hash<vec4>{
	inline size_t operator()(vec4 const& v) const{
		size_t seed = 0;
		hash<float> hasher; size_t hash;
		hash = hasher(v.x); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.y); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.z); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		hash = hasher(v.w); hash += 0x9e3779b9 + (seed << 6) + (seed >> 2); seed ^= hash;
		return seed;
	}
};

template<> 
struct hash<mat3>{
	inline size_t operator()(mat3 const& m) const{
		size_t seed = 0;
		hash<float> hasher; size_t hash;
		forI(9){
			hash = hasher(m.data[i]);
			hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= hash;
		}
		return seed;
	}
};

template<> 
struct hash<mat4>{
	inline size_t operator()(mat4 const& m) const{
		size_t seed = 0;
		hash<float> hasher; size_t hash;
		forI(16){
			hash = hasher(m.data[i]);
			hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= hash;
		}
		return seed;
	}
};

#endif