#pragma once
#ifndef DESHI_ARRAY_H
#define DESHI_ARRAY_H

#include <initializer_list>
#include "../defines.h"

template<class T>
struct array {
	T* items;
	//int size = 0; //num items in array
	int space = 0; //total space array has allocated
	int itemsize = 0;

	typedef T* iterator;
	typedef const T* const_iterator;

	T* first = nullptr;
	T* last  = nullptr;
	T* max   = nullptr;

	T* iter = nullptr;

	//small helper thing
	int roundUp(int numToRound, int multiple) {
		if (multiple == 0) return numToRound;

		int remainder = numToRound % multiple;
		if (remainder == 0) return numToRound;

		return numToRound + multiple - remainder;
	}

	array() {
		space = 1;
		itemsize = sizeof(T);
		items = (T*)calloc(1, itemsize);
		first = items;
		iter = first;
		last = 0;
		max = items;
	}

	array(int size) {
		space = size;
		itemsize = sizeof(T);
		items = (T*)calloc(size, itemsize);
		first = items;
		iter = first;
		last = 0; //could break things but it makes add work 
		max = items + size - 1;
	}

	array(std::initializer_list<T> l) {
		int size = 0;

		itemsize = sizeof(T);
		for (auto& v : l) size++;
		items = (T*)calloc(size, itemsize);
		space = size;

		first = items;
		iter = first;

		int index = 0;
		for (auto& v : l) {
			int he = index * itemsize;
			T* nu = new(items + index) T(v);
			index++;
		}

		last = &items[size - 1];
		max = last;
	}

	//TODO this can probably be much better
	//its necessary so when we return objs the entire array copies properly
	//so we have to make sure everything in the array gets recreated
	array(const array<T>& array) {
		itemsize = array.itemsize;
		space = array.space;
		items = (T*)calloc(array.space, itemsize);

		first = items;
		iter = first;

		last = (array.last == 0) ? 0 : items + array.size() - 1;
		max = items + space - 1;

		//if last is 0 then the array is empty
		if (array.last != 0) {
			int i = 0;
			for (T item : array) {
				new(items + i) T(item);
				i++;
			}
		}
	}

	~array() {
		if (last != 0) {
			for (T* i = first; i <= last; i++) {
				i->~T();
			}
		}
		free(items);
	}

	int size() const {
		if (last == 0) return 0;
		return (int)(last - first) + 1;
	}

	void operator = (array<T>& array) {
		itemsize = array.itemsize;
		space = array.space;
		items = (T*)calloc(array.space, itemsize);

		first = items;
		iter = first;

		last = (array.last == 0) ? 0 : items + array.size() - 1;
		max = items + space - 1;

		//if last is 0 then the array is empty
		if (array.last != 0) {
			int i = 0;
			for (T item : array) {
				new(items + i) T(item);
				i++;
			}
		}
	}
	
	void add(T t) {
		//if array is full, realloc the memory and extend it to accomodate the new item
		if (max - last == 0) {
			int iteroffset = iter - first;
			int osize = size();
			space += 8;
			items = (T*)realloc(items, (space) * itemsize);
			Assert(items, "realloc failed and returned nullptr. maybe we ran out of memory?");
			max = items + space - 1;
			
			first = items;
			iter = first + iteroffset;
			last = first + osize;
			for (T* i = last + 1; i <= max; i++) {
				memset(i, 'A', itemsize);
			}
			new(last) T(t);
		}
		else {
			if (last == 0) {
				new(items) T(t);
				last = items;
			}
			else {
				last++;
				new(last) T(t);
			}
		}
		return;
	}

	void add(array<T> t) {
		for (T item : t) {
			this->add(item);
		}
	}

	//removes last element
	void pop() {
		Assert(size() > 0, "attempt to pop with nothing in array");
		memset(last, 0, itemsize);
		last--;
	}	

	void remove(int i) {
		Assert(size() > 0, "can't remove element from empty vector");
		Assert(i < size(), "index is out of bounds");
		memset(items + i, 0, itemsize);
		for (int o = i; o < size(); o++) {
			items[o] = items[o + 1];
		}
		memset(last, 0, itemsize);
		last--;
	}

	void reserve(int nuspace) {
		if (nuspace > space) {
			space = nuspace;
			int osize = size() - 1;
			int iteroffset = iter - first;
			items = (T*)realloc(items, space * itemsize);
			first = items;
			iter = first + iteroffset;
			last = items + osize;
			max = items + space;
		}
	}

	//TODO add out of bounds checking for these functions

	//returns the value of iter and increments it by one.
	T& next() {
		if (last - iter + 1 >= 0) return *iter++;
	}

	//returns the value of iter + some value and doesn't increment it 
	//TODO come up with a better name for this and the corresponding previous overload
	T& peek(int i = 1) {
		if (last - iter + 1 >= 0) return *(iter + i);
	}

	//returns the value of iter and decrements it by one.
	T& prev() {
		if(first - iter + 1 >= 0) return *iter--;
	}

	T& lookback(int i = 1) {
		if (first - iter + 1 >= 0) return *(iter - i);
	}

	//iterator functions that return pointers if the object isnt already one
	T* nextptr() {
		if (iter + 1 - last >= 0) return iter++;
		else return nullptr;
	}

	//returns the value of iter + some value and doesn't increment it 
	//TODO come up with a better name for this and the corresponding previous overload
	T* peekptr(int i = 1) {
		if (iter + 1 - last >= 0) return iter + i;
		else return nullptr;
	}

	//returns the value of iter and decrements it by one.
	T* prevptr() {
		if (iter - 1 - first >= 0) return iter--;
		else return nullptr;
	}

	T* lookbackptr(int i = 1) {
		if (iter - 1 - first >= 0) return iter - i;
		else return nullptr;
	}

	//this is really only necessary for the copy constructor as far as i know
	T& at(int i) {
		return items[i];
	}

	T& operator[](int i) {
		return items[i];
	}

	//begin/end functions for for each loops
	iterator begin() { return &items[0]; }
	iterator end()   { return &items[size()]; }
	const_iterator begin() const { return &items[0]; }
	const_iterator end()   const { return &items[size()]; }


};

#endif