#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <boost/optional.hpp>

//TODO(c, sushi) reroute global includes probably by making GLOBAL the bridge between anything we want to be global and everything else instead of Debug doing that

//debug defines
#define PRINT(x) std::cout << x << std::endl;
#ifndef NDEBUG
#   define ASSERTWARN(condition, message) \
do { \
if (! (condition)) { \
std::string file = __FILENAME__; \
std::cout << "Warning '" #condition "' failed in " + file + " line " + std::to_string(__LINE__) + ": \n" #message << std::endl; \
} \
} while (false)
#   define ASSERT(condition, message) \
do { \
if (! (condition)) { \
std::string file = __FILENAME__; \
std::cout << "Assertion '" #condition "' failed in " + file + " line " + std::to_string(__LINE__) + ": \n" #message << std::endl;  \
std::terminate(); \
} \
} while (false)
#else
#   define ASSERT(condition, message) condition;
#endif

#define __FILENAME__ (std::strrchr(__FILE__, '\\') ? std::strrchr(__FILE__, '\\') + 1 : __FILE__)

//this is meant to be a wrapper around vector.
//the idea is that you can have a vector whose elements stay in a position regardless of
//if something before it is deleted.
//when something is removed and it's not already at the end of the vector, it's position
//is set to empty. if it's position is at the end then it simply removes that last position
//when placing an item it looks for the first empty spot and places it there

template<class T>
class ContainerManager {
	public:
	//std::vector<std::pair<std::optional<T>, int>> container;
	
	std::vector<boost::optional<T>> container;
	std::vector<int> empties;
	
	int real_size = 0;
	
	ContainerManager() {}
	
	//T t& operator [](int i) { return container[i]; }
	//void operator = (ContainerManager<T> c) { this->copy(c); }
	
	//add element to container and return its position in it
	int add(T t) {
		//if container is totally full add it to the end
		if (empties.size() == 0) {
			container.push_back(t);
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
	
	//attempt to add element to specific index
	//resize if index is over current size
	void add_to_index(T t, int index) {
		ASSERT(allocate_space(index), "Container was unable to allocate space at specified index");
		if (!container[index]) { real_size++; }
		container[index] = t;
		
	}
	
	//attempt to remove element at index 
	void remove_from(int index) {
		ASSERT(container[index], "Container at index " + std::to_string(index) + " is already empty.");
		ASSERT(index < container.size(), "Trying to access container at an index that doesn't exist.");
		
		if (index == container.size() - 1) {
			container.pop_back();
		}
		else {
			real_size--;
			container[index].reset();
			empties.push_back(index);
		}
	}
	
	bool allocate_space(int index) {
		ASSERT(index >= 0, "Attempted to pass negative index.");
		ASSERT(index >= container.size(), "Attempted to allocate space at an index larger than container size.");
		
		if (index < container.size()) {
			//space already exists, there is no check to actually see if
			//it's already used by something else yet though
			return true;
		}
		else {
			boost::optional<T> o;
			for (int i = container.size(); i < index; i++) {
				container.push_back(o);
			}
			
			return true;
		}
		return false;
	}
	
	int size() {
		return container.size();
	}
	
	void copy(ContainerManager<T> cm) {
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
		real_size = 0;
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