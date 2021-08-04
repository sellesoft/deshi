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
	
	Value& operator[](const Key& key) {
		u32 hashed = HashStruct{}(key);
		forI(hashes.count){ if(hashed == hashes[i]){ return data[i]; } }
		hashes.add(hashed);
		data.add(Value{});
		count++;
		return *data.last;
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
};

template<typename Key, typename HashStruct = hash<Key>>
using Set = map<Key,Key,HashStruct>;

#endif