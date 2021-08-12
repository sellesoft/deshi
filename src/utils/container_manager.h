#pragma once
#ifndef DESHI_CONTAINERMANAGER_H
#define DESHI_CONTAINERMANAGER_H

#include "optional.h"
#include "tuple.h"

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>


//this is meant to be a wrapper around vector.
//the idea is that you can have a vector whose elements stay in a position regardless of
//if something before it is deleted.
//when something is removed and it's not already at the end of the vector, it's position
//is set to empty. if it's position is at the end then it simply removes that last position
//when placing an item it looks for the first empty spot and places it there

//TODO(sushi, Cl) container manager needs some clean up, some redundant functions need removed, and some need fleshed out for stability
//it also needs a better name

template<class T>
struct container_manager {
	//std::vector<pair<std::optional<T>, int>> container;
	std::vector<optional<T>> container;
	std::vector<int> empties;
	
	container_manager() {}
	
	optional<T>& operator [](int i) { return container[i]; }
	
	//TODO(sushi) figure this out sometime :)
	//T* operator &() {
	//	return &T;
	//}
	
	int realSize() {
		return container.size() - empties.size();
	}
	
	//add element to container and return its position in it
	int add(T t) {
		//if container is totally full add it to the end
		if (empties.size() == 0) {
			container.push_back(optional<T>(t));
			return container.size() - 1;
		}
		//else place it at the first empty spot and return index
		else {
			container[empties[0]] = t;
			int index = empties[0];
			empties.erase(empties.begin());
			return index;
		}
	}
	int add(T* t) {
		//if container is totally full add it to the end
		if (empties.size() == 0) {
			container.push_back(optional<T>(*t));
			return container.size() - 1;
		}
		//else place it at the first empty spot and return index
		else {
			container[empties[0]] = *t;
			int index = empties[0];
			empties.erase(empties.begin());
			return index;
		}
	}
	
	//attempt to add element to specific index
	//resize if index is over current size
	void addToIndex(T t, int index) {
		//TODO(sushi) allocate_space was removed?
		//Assert(allocate_space(index), "Container was unable to allocate space at specified index");
		if (!container[index]) { realSize++; }
		container[index] = t;
		
	}
	
	//attempt to remove element at index 
	void removeFrom(int index) {
		Assert(index < container.size(), "Trying to access container at an index that doesn't exist.");
		Assert(container[index].test(), "Container at index " + std::to_string(index) + " is already empty.");
		
		if (index == container.size() - 1) {
			container.pop_back();
		}
		else {
			container[index].reset();
			empties.push_back(index);
			std::sort(empties.begin(), empties.end());
		}
	}
	
	int size() {
		return container.size();
	}
	
	void operator =(container_manager<T> cm) {
		container = cm.container;
		empties = cm.empties;
	}
	
	void empty() {
		empties.clear();
		for (int i = 0; i < container.size(); i++) {
			container[i].reset();
			empties.push_back(i);
		}
	}
	
	void clear() {
		container.clear();
		empties.clear();
	}
	
	//if reserve gets used when the container isn't empty this will probably fuck up
	//so if you're here because somethings broken thats why
	void reserve(int i) {
		for (int o = 0; o < i; o++) {
			empties.push_back(o);
			optional<T> op;
			container.push_back(op);
			
		}
	}
	
	using iterator = typename std::vector<optional<T>>::iterator;
	
	iterator begin() {
		return container.begin();
	}
	
	iterator end() {
		return container.end();
	}
	
	std::string str() {
		std::string s = "";
		int index = 1;
		for (auto t : container) {
			s += t.value()->str + " " + std::to_string(t.value()->index) + " " + std::to_string(index) + "\n";
			index++;
		}
		return s;
	}
};

#endif //DESHI_CONTAINERMANAGER_H