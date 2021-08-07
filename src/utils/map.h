#pragma once
#ifndef DESHI_MAP_H
#define DESHI_MAP_H

#include "hash.h"
#include "array.h"

//TODO(delle) make this sorted so its faster
template<typename Key, typename Value, typename HashStruct = hash<Key>>
struct map {
	array<u32>   hashes;
	array<Value> data;
	u32 count = 0;
	
	void clear() {
		hashes.clear();
		data.clear();
		count = 0;
	}
	
	bool has(const Key& key) {
		u32 hashed = HashStruct{}(key);
		forI(hashes.count){ if(hashed == hashes[i]){ return true; } }
		return false;
	}

	Value* at(const Key& key) {
		u32 hashed = HashStruct{}(key);
		forI(hashes.count){ if(hashed == hashes[i]){ return &data[i]; } }
		return 0;
	}

	Value* atIdx(u32 index){
		return &data[index];
	}
	
	//returns index of added or existing key
	u32 add(const Key& key){
		u32 hashed = HashStruct{}(key);
		forI(hashes.count){ if(hashed == hashes[i]){ return i; } }
		hashes.add(hashed);
		data.add(Value{});
		count++;
		return count-1;
	}
	
	u32 add(const Key& key, const Value& value){
		u32 hashed = HashStruct{}(key);
		forI(hashes.count){ if(hashed == hashes[i]){ return i; } }
		hashes.add(hashed);
		data.add(value);
		count++;
		return count-1;
	}

	void swap(u32 idx1, u32 idx2) {
		hashes.swap(idx1, idx2);
		data.swap(idx1, idx2);
	}

	Value* begin() { return data.begin(); }
	Value* end()   { return data.end(); }
	const Value* begin() const { return data.begin(); }
	const Value* end()   const { return data.end(); }
};

template<typename Key, typename HashStruct = hash<Key>>
using set = map<Key,Key,HashStruct>;

#endif