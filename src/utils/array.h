#pragma once
#ifndef DESHI_ARRAY_H
#define DESHI_ARRAY_H

#ifndef DESHI_ARRAY_GROWTH_FACTOR
#  define DESHI_ARRAY_GROWTH_FACTOR 2
#endif
#ifndef DESHI_ARRAY_SPACE_ALIGNMENT
#  define DESHI_ARRAY_SPACE_ALIGNMENT 4
#endif

#include "../defines.h"
#include <cstdlib>
#include <initializer_list>

template<typename T, typename Allocator = STLAllocator>
struct array{
	u32 count;
	u32 space; //number of items array can fit
	T*  data;
	T*  first;
	T*  last;
	T*  max;
	T*  iter;
	Allocator* allocator;
	
	array();
	array(u32 _count);
	array(std::initializer_list<T> l);
	array(const array<T,Allocator>& array);
	array(T* _data, u32 _count);
	~array();
	
	array<T,Allocator>& operator= (const array<T,Allocator>& array);
	T& operator[](u32 i);
	T  operator[](u32 i) const;
	
	u32  size();
	void add(const T& t);
	void add(const array<T, Allocator>& t);
	//for taking in something without copying it
	void emplace(const T& t);
	void insert(const T& t, u32 idx);
	//removes _count elements from the end
	void pop(u32 _count = 1);
	//removes element at i and shifts all following elements down one
	void remove(u32 i);
	//removes all elements but DOES NOT affect space
	void clear();
	//resizes space for count elements and zero-inits any new elements
	void resize(u32 _count);
	//allocates space for count elements
	void reserve(u32 nuspace);
	//swaps two elements in the array
	void swap(u32 idx1, u32 idx2);
	bool has(const T& value);
	//this is really only necessary for the copy constructor as far as i know
	T&   at(u32 i);
	
	//TODO add out of bounds checking for these functions
	//returns the value of iter and increments it by one
	T& next();
	//returns the value of iter + some value and doesn't increment it 
	T& peek(int i = 1);
	//returns the value of iter and decrements it by one
	T& prev();
	T& lookback(int i = 1);
	//iterator functions that return pointers if the object isnt already one
	T* nextptr();
	//returns the value of iter + some value and doesn't increment it 
	T* peekptr(int i = 1);
	//returns the value of iter and decrements it by one
	T* prevptr();
	T* lookbackptr(int i = 1);
	
	//begin/end functions for for-each loops
	inline T* begin(){ return &data[0]; }
	inline T* end()  { return &data[count]; }
	inline const T* begin()const{ return &data[0]; }
	inline const T* end()  const{ return &data[count]; }
};

///////////////////////
//// @constructors ////
///////////////////////
template<typename T, typename Allocator> inline array<T, Allocator>::
array(){
	space = 0;
	count = 0;
	data  = 0;
	first = 0;
	iter  = 0;
	last  = 0;
	max   = 0;
}

template<typename T, typename Allocator> inline array<T, Allocator>::
array(u32 _count){
	count = 0;
	space = RoundUpTo(_count, DESHI_ARRAY_SPACE_ALIGNMENT);
	data  = (T*)allocator->reserve(_count*sizeof(T));
	
	first = data;
	iter  = data;
	last  = 0;
	max   = data + (space-1);
}

template<typename T, typename Allocator> inline array<T, Allocator>::
array(std::initializer_list<T> l){
	count = l.size();
	space = RoundUpTo(l.size(), DESHI_ARRAY_SPACE_ALIGNMENT);
	data  = (T*)allocator->reserve(space*sizeof(T));
	allocator->commit(data, count*sizeof(T));
	
	u32 index = 0;
	for(T item : l){
		u32 he = index * sizeof(T);
		T* nu = new(data + index) T(item);
		index++;
	}
	
	first = data;
	iter  = data;
	last  = &data[count - 1];
	max   = data + (space-1);
}

//TODO this can probably be much better
//its necessary so when we return elements the entire array copies properly
//so we have to make sure everything in the array gets recreated
template<typename T, typename Allocator> inline array<T, Allocator>::
array(const array<T, Allocator>& array){
	count = array.count;
	space = array.space;
	data  = (T*)allocator->reserve(space*sizeof(T));
	allocator->commit(data, count*sizeof(T));
	
	//if last is 0 then the array is empty
	if(array.count != 0){
		u32 i = 0;
		for(T item : array){
			new(data + i) T(item);
			i++;
		}
	}
	
	first = data;
	iter  = first;
	last  = (array.last == 0) ? 0 : data+(array.count-1);
	max   = data + (space-1);
}

template<typename T, typename Allocator> inline array<T, Allocator>::
array(T* _data, u32 _count){
	count = _count;
	space = RoundUpTo(_count, DESHI_ARRAY_SPACE_ALIGNMENT);
	data  = (T*)allocator->reserve(space*sizeof(T));
	allocator->commit(data, count*sizeof(T));
	memcpy(data, _data, _count*sizeof(T));
	
	first = data;
	iter  = first;
	last  = data + (_count-1);
	max   = data + (space-1);
}

template<typename T, typename Allocator> inline array<T, Allocator>::
~array(){
	forI(count){ data[i].~T(); }
	allocator->release(data);
}

////////////////////
//// @operators ////
////////////////////
template<typename T,typename Allocator> inline array<T,Allocator>& array<T, Allocator>::
operator= (const array<T, Allocator>& _array){
	forI(count){ data[i].~T(); }
	allocator->release(data);
	
	space = _array.space;
	count = _array.count;
	data  = (T*)allocator->reserve(space*sizeof(T));
	allocator->commit(data, count*sizeof(T));
	
	//if last is 0 then the array is empty
	if(_array.last != 0){
		u32 i = 0;
		for(T item : _array){
			new(data + i) T(item);
			i++;
		}
	}
	
	first = data;
	iter  = first;
	last  = (_array.last == 0) ? 0 : data+(_array.count-1);
	max   = data + (space-1);
	return *this;
}

template<typename T, typename Allocator> inline T& array<T, Allocator>::
operator[](u32 i){
	Assert(i < count);
	return data[i];
}

template<typename T, typename Allocator> inline T array<T, Allocator>::
operator[](u32 i) const {
	Assert(i < count);
	return data[i];
}


////////////////////
//// @functions ////
////////////////////
template<typename T, typename Allocator> inline u32 array<T, Allocator>::
size(){
	return count;
}

template<typename T, typename Allocator> inline void array<T, Allocator>::
add(const T& t){
	if(space == 0){ //if first item, allocate memory
		space = DESHI_ARRAY_SPACE_ALIGNMENT;
		data  = (T*)allocator->reserve(space*sizeof(T));
		allocator->commit(data, 1*sizeof(T));
		
		first = data;
		iter  = data;
		last  = data;
		max   = data + (space-1);
		
		data[0] = t;
		count = 1;
	}else if(count == space){ //if array is full, resize the memory by the growth factor
		space *= DESHI_ARRAY_GROWTH_FACTOR;
		data = (T*)allocator->resize(data, space*sizeof(T));
		memset(data+count, 0, (space-count)*sizeof(T)); //NOTE STL doesnt guarantee memory is zero on realloc
		allocator->commit(data+count, 1*sizeof(T));
		
		iter  = data + (iter - first);
		first = data;
		last  = data + count;
		max   = data + (space-1);
		
		data[count] = t;
		count++;
	}else{
		last++;
		data[count] = t;
		count++;
	}
}

template<typename T, typename Allocator> inline void array<T, Allocator>::
add(const array<T, Allocator>& t){
	for(const T& item : t){
		this->add(item);
	}
}

template<typename T, typename Allocator> inline void array<T, Allocator>::
emplace(const T& t){
	this->add(t); //TODO emplace function signature should mimic the type's constructor
}

template<typename T, typename Allocator> inline void array<T, Allocator>::
insert(const T& t, u32 idx){
	Assert(idx <= count);
	if(space == 0){ //if first item, allocate memory
		space = DESHI_ARRAY_SPACE_ALIGNMENT;
		data  = (T*)allocator->reserve(space*sizeof(T));
		allocator->commit(data, 1*sizeof(T));
		
		first = data;
		iter  = data;
		last  = data;
		max   = data + (space-1);
		
		count = 1;
		data[0] = t;
	}else if(count == space){ //if array is full, resize the memory by the growth factor
		space *= DESHI_ARRAY_GROWTH_FACTOR;
		data = (T*)allocator->resize(data, space*sizeof(T));
		memset(data+count, 0, (space-count)*sizeof(T)); //NOTE STL doesnt guarantee memory is zero on realloc
		allocator->commit(data+count, 1*sizeof(T));
		memmove(data+idx+1, data+idx, (count-idx)*sizeof(T));
		memset(data+idx, 0, 1*sizeof(T));
		
		iter  = data + (iter - first);
		first = data;
		last  = data + count;
		max   = data + (space-1);
		
		data[idx] = t;
		count++;
	}else if(idx == count){
		this->add(t);
	}else{
		memmove(data+idx+1, data+idx, (count-idx)*sizeof(T));
		memset(data+idx, 0, 1*sizeof(T));
		data[idx] = t;
		count++;
	}
}

template<typename T, typename Allocator> inline void array<T, Allocator>::
pop(u32 _count){
	Assert(count >= _count, "attempted to pop more than array size");
	forI(_count){
		last->~T();
		last--;
		count--;
	}
}	

template<typename T, typename Allocator> inline void array<T, Allocator>::
remove(u32 i){
	Assert(count > 0, "can't remove element from empty vector");
	Assert(i < count, "index is out of bounds");
	data[i].~T();
	for(u32 o = i; o < size(); o++){
		data[o] = data[o+1];
	}
	memset(last, 0, sizeof(T));
	last--;
	count--;
}

//TODO(sushi) impl this
//void remove(u32 i1, u32 i2){
//	Assert(count > 0, "can't remove element from empty vector");
//	Assert(i < count, "index is out of bounds");
//	data[i].~T();
//	for(u32 o = i2; o < size(); o++){
//		data[o] = data[o + 1];
//	}
//	memset(last, 0, sizeof(T));
//	last--;
//	count--;
//}

template<typename T, typename Allocator> inline void array<T, Allocator>::
clear(){
	forI(count){ data[i].~T(); }
	
	count = 0;
	first = data;
	iter  = data;
	last  = 0;
	max   = data + (space-1);
}

template<typename T, typename Allocator> inline void array<T, Allocator>::
resize(u32 new_count){
	if(new_count > space){
		space = new_count;
		data = (T*)allocator->resize(data, space*sizeof(T));
		memset(data+count, 0, (new_count-count)*sizeof(T)); //NOTE STL doesnt guarantee memory is zero on realloc
		count = new_count;
		
		iter  = data + (iter - first);
		first = data;
		last  = data + (space-1);
		max   = last;
	}else if(new_count < space){
		for(u32 i = new_count+1; i < count; ++i){ data[i].~T(); }
		
		count = new_count;
		space = new_count;
		data = (T*)allocator->resize(data, space*sizeof(T));
		
		iter  = data + (iter - first);
		first = data;
		last  = data + (space-1);
		max   = last;
	}
}

template<typename T, typename Allocator> inline void array<T, Allocator>::
reserve(u32 new_space){
	if(new_space > space){
		space = RoundUpTo(new_space, DESHI_ARRAY_SPACE_ALIGNMENT);
		data = (T*)allocator->resize(data, space*sizeof(T));
		memset(data+count, 0, (new_space-count)*sizeof(T)); //NOTE STL doesnt guarantee memory is zero on realloc
		
		iter  = data + (iter - first);
		first = data;
		last  = (count) ? data + (count-1) : 0;
		max   = data + (space-1);
	}
}

template<typename T, typename Allocator> inline void array<T, Allocator>::
swap(u32 idx1, u32 idx2){
	Assert(idx1 < count && idx2 < count, "index out of bounds");
	Assert(idx1 != idx2, "can't swap an element with itself");
	T save = data[idx1];
	data[idx1] = data[idx2];
	data[idx2] = save;
}

template<typename T, typename Allocator> inline bool array<T, Allocator>::
has(const T& value){
	for(const T& blahabuasjdas : *this){
		if(blahabuasjdas == value){
			return true;
		}
	}
	return false;
}

template<typename T, typename Allocator> inline T& array<T, Allocator>::
at(u32 i){
	Assert(i < count);
	return data[i];
}

template<typename T, typename Allocator> inline T& array<T, Allocator>::
next(){
	if(last - iter + 1 >= 0) return *++iter;
	return *iter;
}

template<typename T, typename Allocator> inline T& array<T, Allocator>::
peek(int i){
	if(last - iter + 1 >= 0) return *(iter + i);
	return *iter;
}

template<typename T, typename Allocator> inline T& array<T, Allocator>::
prev(){
	if(first - iter + 1 >= 0) return *iter--;
}

template<typename T, typename Allocator> inline T& array<T, Allocator>::
lookback(int i){
	if(first - iter + 1 >= 0) return *(iter - i);
}

template<typename T, typename Allocator> inline T* array<T, Allocator>::
nextptr(){
	if(iter + 1 - last >= 0) return iter++;
	else return nullptr;
}

//TODO come up with a better name for this and the corresponding previous overload
template<typename T, typename Allocator> inline T* array<T, Allocator>::
peekptr(int i){
	if(iter + 1 - last >= 0) return iter + i;
	else return nullptr;
}

template<typename T, typename Allocator> inline T* array<T, Allocator>::
prevptr(){
	if(iter - 1 - first >= 0) return iter--;
	else return nullptr;
}

template<typename T, typename Allocator> inline T* array<T, Allocator>::
lookbackptr(int i){
	if(iter - 1 - first >= 0) return iter - i;
	else return nullptr;
}

#if DESHI_RUN_TESTS
#include <typeinfo>
#include <cstdio>
function void TEST_deshi_utils_array(){
	array<int> array1;
	AssertAlways(array1.count == 0 && array1.space == 0 && array1.data == 0 && array1.first == 0 && array1.last == 0 && array1.max == 0 && array1.iter == 0);
	AssertAlways(typeid(array1.data) == typeid(int*));
	AssertAlways(typeid(array1.allocator) == typeid(STLAllocator*));
	
	printf("[DESHI TEST] PASSED: utils/array");
}
#endif //DESHI_RUN_TESTS

#endif //DESHI_ARRAY_H