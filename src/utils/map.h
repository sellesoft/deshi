#pragma once
#ifndef DESHI_MAP_H
#define DESHI_MAP_H

#include "array.h"
#include "tuple.h"
#include "utils.h"
#include "hash.h"

template<class Key, class Value>
struct map {
	array<pair<u32, Value>> data;

	u32 seed = 213;

	bool has(const Key& key) {
		u32 hashed = hash<Key>{}(key);
		for (auto& p : data) {
			if (hashed == p.first) {
				return true;
			}
		}
		return false;
	}

	Value& operator[](const Key& key) {
		u32 hashed = hash<Key>{}(key);
		for (auto& p : data) {
			if (hashed == p.first) {
				return p.second;
			}
		}
		Value v{};
		data.add(make_pair(hashed, v));
		return data.last->second;
	}

	u32 size() {
		return data.size();
	}

	void clear() {
		data.clear();
	}

	void erase(u32 idx) {
		data.remove(idx);
	}

};

#endif