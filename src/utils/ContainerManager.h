#pragma once
#ifndef DESHI_CONTAINERMANAGER_H
#define DESHI_CONTAINERMANAGER_H

#include "optional.h"

#include <string>
#include <vector>
#include <iostream>



//this is meant to be a wrapper around vector.
//the idea is that you can have a vector whose elements stay in a position regardless of
//if something before it is deleted.
//when something is removed and it's not already at the end of the vector, it's position
//is set to empty. if it's position is at the end then it simply removes that last position
//when placing an item it looks for the first empty spot and places it there

template<class T>
struct ContainerManager {
	//std::vector<std::pair<std::optional<T>, int>> container;
	std::vector<Optional<T>> container;
	std::vector<int> empties;
	
	int real_size = 0;
	
	ContainerManager() {}
	
	Optional<T>& operator [](int i) { return container[i]; }

	//TODO(sushi) figure this out sometime :)
	//T* operator &() {
	//	return &T;
	//}
	
	//add element to container and return its position in it
	int add(T t) {
		//if container is totally full add it to the end
		if (empties.size() == 0) {
			container.push_back(Optional<T>(t));
			real_size++;
			return container.size() - 1;
		}
		//else place it at the first empty spot and return index
		else {
			container[empties[0]] = t;
			int index = empties[0];
			empties.erase(empties.begin());
			real_size++;
			return index;
		}
	}
	int add(T* t) {
		//if container is totally full add it to the end
		if (empties.size() == 0) {
			container.push_back(Optional<T>(*t));
			real_size++;
			return container.size() - 1;
		}
		//else place it at the first empty spot and return index
		else {
			container[empties[0]] = *t;
			int index = empties[0];
			empties.erase(empties.begin());
			real_size++;
			return index;
		}
	}
	
	//attempt to add element to specific index
	//resize if index is over current size
	void add_to_index(T t, int index) {
		ASSERT(allocate_space(index), "Container was unable to allocate space at specified index");
		if (!container[index]) { real_size++; }
		container[index] = t;
		
	}
	
	//attempt to remove element at index 
	void remove_from(int index) {
		ASSERT(index < container.size(), "Trying to access container at an index that doesn't exist.");
		ASSERT(container[index].test(), "Container at index " + std::to_string(index) + " is already empty.");
		
		if (index == container.size() - 1) {
			container.pop_back();
		}
		else {
			real_size--;
			container[index].reset();
			empties.push_back(index);
			std::sort(empties.begin(), empties.end());
		}
	}
	
	int size() {
		return container.size();
	}
	
	void operator =(ContainerManager<T> cm) {
		container = cm.container;
		empties = cm.empties;
	}
	
	void empty() {
		//this probably isn't what I should do so fix this if it isn't
		empties.clear();
		for (int i = 0; i < container.size(); i++) {
			container[i].reset();
			empties.push_back(i);
		}
		real_size = 0;
	}
	
	void clear() {
		container.clear();
		empties.clear();
		real_size = 0;
	}

	//if reserve gets used when the container isn't empty this will probably fuck up
	//so if you're here because somethings broken thats why
	void reserve(int i) {
		for (int o = 0; o < i; o++) {
			empties.push_back(o);
			Optional<T> op;
			container.push_back(op);

		}
	}

	//typedef typename iterator  iterator;
	//typedef typename iterator std::vector<T>::const_iterator const_iterator;

	using iterator = typename std::vector<Optional<T>>::iterator;
	
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