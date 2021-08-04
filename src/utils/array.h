#pragma once
#ifndef DESHI_ARRAY_H
#define DESHI_ARRAY_H

#include <initializer_list>
#include "../defines.h"

template<class T>
struct array {
	T* data;
	u32 space = 0; //total space array has allocated
	u32 count = 0;
	
	T* first = nullptr;
	T* last  = nullptr;
	T* max   = nullptr;
	T* iter  = nullptr;
	
	array() {
		space = 4;
		count = 0;
		data  = (T*)calloc(space, sizeof(T));
		first = data;
		iter  = first;
		last  = 0;
		max   = data+(space-1);
	}
	
	array(u32 _count) {
		space = RoundUpTo(_count, 4);
		count = _count;
		data  = (T*)calloc(_count, sizeof(T));
		first = data;
		iter  = first;
		last  = 0; //could break things but it makes add work 
		max   = data+(space-1);
	}
	
	array(std::initializer_list<T> l) {
		space = RoundUpTo(l.size(), 4);
		count = l.size();
		data = (T*)calloc(space, sizeof(T));

		u32 index = 0;
		for (T item : l) {
			u32 he = index * sizeof(T);
			T* nu = new(data + index) T(item);
			index++;
		}
		
		first = data;
		iter  = data;
		last  = &data[count - 1];
		max   = data+(space-1);
	}
	
	//TODO this can probably be much better
	//its necessary so when we return objs the entire array copies properly
	//so we have to make sure everything in the array gets recreated
	array(const array<T>& array) {
		space = array.space;
		count = array.count;
		data = (T*)calloc(space, sizeof(T));
		
		//if last is 0 then the array is empty
		if (array.last != 0) {
			u32 i = 0;
			for (T item : array) {
				new(data + i) T(item);
				i++;
			}
		}

		first = data;
		iter  = first;
		last = (array.last == 0) ? 0 : data+(array.count-1);
		max  = data+(space-1);
	}
	
	array(T* _data, u32 _count) {
		space = RoundUpTo(_count, 4);
		count = _count;
		data = (T*)calloc(space, sizeof(T));
		memcpy(data, _data, _count*sizeof(T));
		first = data;
		iter  = first;
		last  = data+(_count-1);
		max   = data+(space-1);
	}
	
	virtual ~array() {
		if (last != 0) {
			for (T* i = first; i <= last; i++) {
				i->~T();
			}
		}
		free(data); data = 0;
	}
	
	size_t size() const {
		return count;
	}
	
	void operator = (const array<T>& array) {
		this->~array();

		space = array.space;
		count = array.count;
		data = (T*)calloc(space, sizeof(T));
		
		//if last is 0 then the array is empty
		if (array.last != 0) {
			u32 i = 0;
			for (T item : array) {
				new(data + i) T(item);
				i++;
			}
		}

		first = data;
		iter  = first;
		last  = (array.last == 0) ? 0 : data+(array.count-1);
		max   = data+(space-1);
	}
	
	void add(T t) {
		//if array is full, realloc the memory and extend it to accomodate the new item
		if (max - last == 0) {
			u32 iteroffset = iter - first;
			u32 osize = count;
			space *= 2;
			data = (T*)realloc(data, space*sizeof(T));
			Assert(data, "realloc failed and returned nullptr. maybe we ran out of memory?");
			
			first = data;
			iter  = first + iteroffset;
			last  = first + osize;
			max   = data+(space-1);

			new(last) T(t);
		}
		else {
			if (last == 0) {
				new(data) T(t);
				last = data;
			}
			else {
				last++;
				new(last) T(t);
			}
		}
		count++;
	}
	
	void add(array<T> t) {
		for (T item : t) {
			this->add(item);
		}
	}

	//for taking in something without copying it
	void emplace(const T& t) {
		//if array is full, realloc the memory and extend it to accomodate the new item
		if (max - last == 0) {
			int iteroffset = iter - first;
			int osize = count;
			space *= 2;
			data = (T*)realloc(data, (space)*sizeof(T));
			Assert(data, "realloc failed and returned nullptr. maybe we ran out of memory?");

			first = data;
			iter  = first + iteroffset;
			last  = first + osize;
			max   = data+(space-1);

			new(last) T(t);
		}
		else {
			if (last == 0) {
				new(data) T(t);
				last = data;
			}
			else {
				last++;
				new(last) T(t);
			}
		}
		count++;
	}

	//removes last element
	void pop() {
		Assert(count > 0, "attempt to pop with nothing in array");
		last->~T();
		//memset(last, 0, sizeof(T));
		last--;
		count--;
	}	
	
	//TODO(delle,Op) maybe no need to zerofill in Release since it'll be overwritten anyways
	void pop(u32 _count) {
		Assert(count >= _count, "attempt to pop more than array size");
		forI(_count){
			last->~T();
			last--;
			count--;
		}
	}	
	
	void remove(u32 i) {
		Assert(count > 0, "can't remove element from empty vector");
		Assert(i < count, "index is out of bounds");
		data[i].~T();
		for (u32 o = i; o < size(); o++) {
			data[o] = data[o+1];
		}
		memset(last, 0, sizeof(T));
		last--;
		count--;
	}
	
	//removes all elements
	void clear(){
		this->~array();
		last  = 0;
		count = 0;
	}
	
	//allocates space for count elements and zero-inits any new elements
	void resize(u32 _count){
		if(_count > space){
			space = _count;
			u32 osize = count;
			u32 iteroffset = iter - first;
			data = (T*)realloc(data, space*sizeof(T));
			memset(data+osize+1, 0, (_count-osize)*sizeof(T));
			first = data;
			data  = first + iteroffset;
			last  = data+(space-1);
			max   = data+(space-1);
		}else if(_count < space){
			space = _count;
			u32 osize = count;
			u32 iteroffset = iter - first;
			data = (T*)realloc(data, space*sizeof(T));
			first = data;
			iter  = first + iteroffset;
			last  = data+(space-1);
			max   = data+(space-1);
		}
		count = _count;
	}
	
	void reserve(u32 nuspace) {
		if (nuspace > space) {
			space = RoundUpTo(nuspace, 4);
			u32 osize = count;
			u32 iteroffset = iter - first;
			data = (T*)realloc(data, space * sizeof(T));
			first = data;
			iter  = first + iteroffset;
			last  = data + osize;
			max   = data+(space-1);
		}
	}

	//swaps two elements in the array
	void swap(u32 idx1, u32 idx2) {
		Assert(idx1 < count && idx2 < count, "index out of bounds");
		Assert(idx1 != idx2, "can't swap an element with itself");
		T save = data[idx1];
		data[idx1] = data[idx2];
		data[idx2] = save;
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
	T& at(u32 i) {
		Assert(i < count);
		return data[i];
	}
	
	T& operator[](u32 i) {
		Assert(i < count);
		return data[i];
	}
	
	/*//failed attempt at compile-time reverse accessor
	template<int I>
		T& operator[](int cpp_forces_a_parameter_here_kaksoispiste_ddddd) {
		if constexpr (I < 0){
			return items[count+I];
		}else{
			return items[I];
		}
	}*/
	
	void BubbleSort(){
		T temp;
		bool swapped;
		forX(i,count-1){
			swapped = false;
			forX(j,count-i-1){
				if(data[j]>data[j+1]){
					temp = data[j];
					data[j] = data[j+1];
					data[j+1] = temp;
					swapped = true;
				}
			}
			if(!swapped) break;
		}
	}
	
	//begin/end functions for for each loops
	T* begin() { return &data[0]; }
	T* end()   { return &data[count]; }
	const T* begin() const { return &data[0]; }
	const T* end()   const { return &data[count]; }
};

#endif