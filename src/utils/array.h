#pragma once
#ifndef DESHI_ARRAY_H
#define DESHI_ARRAY_H

#include <initializer_list>
#include "../defines.h"

template<class T>
struct array {
	T* items;
	u32 space = 0; //total space array has allocated
	u32 count = 0;
	
	T* first = nullptr;
	T* last  = nullptr;
	T* max   = nullptr;
	T* iter  = nullptr;
	
	array() {
		space = 4;
		items = (T*)calloc(1, sizeof(T));
		first = items;
		iter = first;
		last = 0;
		max = items;
		count = 0;
	}
	
	array(u32 size) {
		space = RoundUpTo(size, 4);
		items = (T*)calloc(size, sizeof(T));
		first = items;
		iter = first;
		last = 0; //could break things but it makes add work 
		max = items + size - 1;
		count = size;
	}
	
	array(std::initializer_list<T> l) {
		space = RoundUpTo(l.size(), 4);
		count = l.size();
		
		items = (T*)calloc(space, sizeof(T));
		
		first = items;
		iter = first;
		
		u32 index = 0;
		for (auto& v : l) {
			u32 he = index * sizeof(T);
			T* nu = new(items + index) T(v);
			index++;
		}
		
		last = &items[count - 1];
		max = last;
	}
	
	//TODO this can probably be much better
	//its necessary so when we return objs the entire array copies properly
	//so we have to make sure everything in the array gets recreated
	array(const array<T>& array) {
		space = array.space;
		items = (T*)calloc(array.space, sizeof(T));
		count = array.count;
		
		first = items;
		iter = first;
		
		last = (array.last == 0) ? 0 : items + array.count - 1;
		max = items + space - 1;
		
		//if last is 0 then the array is empty
		if (array.last != 0) {
			u32 i = 0;
			for (T item : array) {
				new(items + i) T(item);
				i++;
			}
		}
	}
	
	array(T* _data, u32 _count) {
		space = RoundUpTo(_count, 4);
		items = (T*)calloc(space, sizeof(T));
		memcpy(items, _data, _count*sizeof(T));
		first = items;
		iter  = first;
		last  = items+(_count-1);
		max   = items+(space-1);
		count = _count;
	}
	
	virtual ~array() {
		if (last != 0) {
			for (T* i = first; i <= last; i++) {
				i->~T();
			}
		}
		free(items);
	}
	
	size_t size() const {
		return count;
	}
	
	void operator = (array<T>& array) {
		space = array.space;
		items = (T*)calloc(array.space, sizeof(T));
		count = array.count;
		
		first = items;
		iter = first;
		
		last = (array.last == 0) ? 0 : items + array.count - 1;
		max = items + space - 1;
		
		//if last is 0 then the array is empty
		if (array.last != 0) {
			u32 i = 0;
			for (T item : array) {
				new(items + i) T(item);
				i++;
			}
		}
	}
	
	void add(T t) {
		//if array is full, realloc the memory and extend it to accomodate the new item
		if (max - last == 0) {
			u32 iteroffset = iter - first;
			u32 osize = count;
			space *= 2;
			items = (T*)realloc(items, space*sizeof(T));
			Assert(items, "realloc failed and returned nullptr. maybe we ran out of memory?");
			max = items + space - 1;
			
			first = items;
			iter = first + iteroffset;
			last = first + osize;
			for (T* i = last + 1; i <= max; i++) {
				memset(i, 'A', sizeof(T));
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
		count++;
	}
	
	void add(array<T> t) {
		for (T item : t) {
			this->add(item);
		}
	}
	
	//removes last element
	void pop() {
		Assert(count > 0, "attempt to pop with nothing in array");
		memset(last, 0, sizeof(T));
		last--;
		count--;
	}	
	
	//TODO(delle,Op) maybe no need to zerofill in Release since it'll be overwritten anyways
	void pop(u32 _count) {
		Assert(count >= _count, "attempt to pop more than array size");
		memset(last-_count, 0, sizeof(T)*_count);
		last -= _count;
		count -= _count;
	}	
	
	void remove(u32 i) {
		Assert(count > 0, "can't remove element from empty vector");
		Assert(i < count, "index is out of bounds");
		memset(items + i, 0, sizeof(T));
		for (u32 o = i; o < size(); o++) {
			items[o] = items[o + 1];
		}
		memset(last, 0, sizeof(T));
		last--;
		count--;
	}
	
	//removes all elements
	void clear(){
		last = 0;
		count = 0;
	}
	
	//allocates space for count elements and zero-inits any new elements
	void resize(u32 _count){
		if(_count > space){
			space = _count;
			u32 osize = count;
			u32 iteroffset = iter - first;
			items = (T*)realloc(items, space*sizeof(T));
			memset(items+osize+1, 0, (_count-osize)*sizeof(T));
			first = items;
			iter  = first + iteroffset;
			last  = items + space - 1;
			max   = items + space;
		}else if(_count < space){
			space = _count;
			u32 osize = count;
			u32 iteroffset = iter - first;
			items = (T*)realloc(items, space*sizeof(T));
			first = items;
			iter  = first + iteroffset;
			last  = items + space - 1;
			max   = items + space;
		}
		count = _count;
	}
	
	void reserve(u32 nuspace) {
		if (nuspace > space) {
			space = RoundUpTo(nuspace, 4);
			u32 osize = count;
			u32 iteroffset = iter - first;
			items = (T*)realloc(items, space * sizeof(T));
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
	T& at(u32 i) {
		Assert(i < count);
		return items[i];
	}
	
	T& operator[](u32 i) {
		Assert(i < count);
		return items[i];
	}
	
	/*//failed attempt at compile-time reverse accessor
	template<int I>
		T& operator[](int cpp_forces_a_parameter_here_kaksoispiste_ddddd) {
		if constexpr (I < 0){
			return items[count+I];
		}else{
			return items[I];
		}
	}
*/
	
	void BubbleSort(){
		T temp;
		bool swapped;
		forX(i,count-1){
			swapped = false;
			forX(j,count-i-1){
				if(items[j]>items[j+1]){
					temp = items[j];
					items[j] = items[j+1];
					items[j+1] = temp;
					swapped = true;
				}
			}
			if(!swapped) break;
		}
	}
	
	//begin/end functions for for each loops
	T* begin() { return &items[0]; }
	T* end()   { return &items[count]; }
	const T* begin() const { return &items[0]; }
	const T* end()   const { return &items[count]; }
};

#endif