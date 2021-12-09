#pragma once
#ifndef DESHI_ARRAY_H
#define DESHI_ARRAY_H

#include "../defines.h"
#include <cstdlib>
#include <initializer_list>

template<typename T, typename Allocator = STLAllocator>
struct array{
	u32 count = 0;
	u32 space = 0; //total space array has allocated
	T* data   = nullptr;
	
	T* first = nullptr;
	T* last  = nullptr;
	T* max   = nullptr;
	T* iter  = nullptr;
	
	Allocator allocator;
	
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
	//removes all elements
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
template<typename T, typename Allocator>
inline array<T, Allocator>::array(){ //TODO(delle) we should not allocate on default init
	space = 4;
	count = 0;
	data  = (T*)allocator.callocate(space, sizeof(T));
	first = data;
	iter  = first;
	last  = 0;
	max   = data+(space-1);
}

template<typename T, typename Allocator>
inline array<T, Allocator>::array(u32 _count){
	space = RoundUpTo(_count, 4);
	count = _count;
	data  = (T*)allocator.callocate(_count, sizeof(T));
	first = data;
	iter  = first;
	last  = &data[count - 1]; 
	max   = data+(space-1);
}

template<typename T, typename Allocator>
inline array<T, Allocator>::array(std::initializer_list<T> l){
	space = RoundUpTo(l.size(), 4);
	count = l.size();
	data  = (T*)allocator.callocate(space, sizeof(T));
	
	u32 index = 0;
	for(T item : l){
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
//its necessary so when we return elements the entire array copies properly
//so we have to make sure everything in the array gets recreated
template<typename T, typename Allocator>
inline array<T, Allocator>::array(const array<T, Allocator>& array){
	space = array.space;
	count = array.count;
	data = (T*)allocator.callocate(space, sizeof(T));
	
	//if last is 0 then the array is empty
	if(array.last != 0){
		u32 i = 0;
		for(T item : array){
			new(data + i) T(item);
			i++;
		}
	}
	
	first = data;
	iter  = first;
	last = (array.last == 0) ? 0 : data+(array.count-1);
	max  = data+(space-1);
}

template<typename T, typename Allocator>
inline array<T, Allocator>::array(T* _data, u32 _count){
	space = RoundUpTo(_count, 4);
	count = _count;
	data = (T*)allocator.callocate(space, sizeof(T));
	memcpy(data, _data, _count*sizeof(T));
	first = data;
	iter  = first;
	last  = data+(_count-1);
	max   = data+(space-1);
}

template<typename T, typename Allocator>
inline array<T, Allocator>::~array(){
	if(last != 0){
		for(T* i = first; i <= last; i++){
			i->~T();
		}
	}
	allocator.deallocate(data); data = 0;
}

////////////////////
//// @operators ////
////////////////////
template<typename T, typename Allocator>
inline array<T, Allocator>& array<T, Allocator>::operator= (const array<T, Allocator>& _array){
	this->~array();
	
	space = _array.space;
	count = _array.count;
	data  = (T*)allocator.callocate(space, sizeof(T));
	
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
	max   = data+(space-1);
	return *this;
}

template<typename T, typename Allocator>
inline T& array<T, Allocator>::operator[](u32 i){
	Assert(i < count);
	return data[i];
}

template<typename T, typename Allocator>
inline T array<T, Allocator>::operator[](u32 i) const {
	Assert(i < count);
	return data[i];
}


////////////////////
//// @functions ////
////////////////////
template<typename T, typename Allocator>
inline u32 array<T, Allocator>::size(){
	return count;
}

template<typename T, typename Allocator>
inline void array<T, Allocator>::add(const T& t){
	//if array is full, realloc the memory and extend it to accomodate the new item
	if(max - last == 0){
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
	}else{
		if(last == 0){
			new(data) T(t);
			last = data;
		}else{
			last++;
			new(last) T(t);
		}
	}
	count++;
}

template<typename T, typename Allocator>
inline void array<T, Allocator>::add(const array<T, Allocator>& t){
	for(const T& item : t){
		this->add(item);
	}
}

template<typename T, typename Allocator>
inline void array<T, Allocator>::emplace(const T& t){
	//if array is full, realloc the memory and extend it to accomodate the new item
	if(max - last == 0){
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
	}else{
		if(last == 0){
			new(data) T(t);
			last = data;
		}else{
			last++;
			new(last) T(t);
		}
	}
	count++;
}

template<typename T, typename Allocator>
inline void array<T, Allocator>::insert(const T& t, u32 idx) {
	Assert(idx <= count);
	count += 1;
	if (space == 0) {
		space = 4;
		Assert(data = (T*)allocator.callocate(space, sizeof(T)));
		data[0] = t;
	}
	else if (space < count + 1) {
		space = RoundUpTo(count + 1, 4);
		Assert(data = (T*)allocator.callocate(space, sizeof(T)));
		memmove(data + idx + 1, data + idx, (count - idx) * sizeof(T));
		data[idx] = t;
	}
	else {
		memmove(data + idx + 1, data + idx, (count - idx) * sizeof(T));
		data[idx] = t;
	}
}

template<typename T, typename Allocator>
inline void array<T, Allocator>::pop(u32 _count){
	Assert(count >= _count, "attempt to pop more than array size");
	forI(_count){
		last->~T();
		last--;
		count--;
	}
}	

template<typename T, typename Allocator>
inline void array<T, Allocator>::remove(u32 i){
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

template<typename T, typename Allocator>
inline void array<T, Allocator>::clear(){
	this->~array();
	space = 4;
	count = 0;
	data = (T*)allocator.callocate(space, sizeof(T));
	first = data;
	iter = first;
	last = 0;
	max = data + (space - 1);
}

template<typename T, typename Allocator>
inline void array<T, Allocator>::resize(u32 _count){
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
		for(u32 i = _count+1; i < count; ++i){ data[i].~T(); }
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

template<typename T, typename Allocator>
inline void array<T, Allocator>::reserve(u32 nuspace){
	if(nuspace > space){
		space = RoundUpTo(nuspace, 4);
		u32 osize = count;
		u32 iteroffset = iter - first;
		data = (T*)realloc(data, space * sizeof(T));
		first = data;
		iter  = first + iteroffset;
		if(osize != 0)
			last  = data + osize;
		max   = data+(space-1);
	}
}

template<typename T, typename Allocator>
inline void array<T, Allocator>::swap(u32 idx1, u32 idx2){
	Assert(idx1 < count && idx2 < count, "index out of bounds");
	Assert(idx1 != idx2, "can't swap an element with itself");
	T save = data[idx1];
	data[idx1] = data[idx2];
	data[idx2] = save;
}

template<typename T, typename Allocator>
inline bool array<T, Allocator>::has(const T& value){
	for(const T& blahabuasjdas : *this) 
		if(blahabuasjdas == value) 
		return true;
	return false;
}

template<typename T, typename Allocator>
inline T& array<T, Allocator>::at(u32 i){
	Assert(i < count);
	return data[i];
}

template<typename T, typename Allocator>
inline T& array<T, Allocator>::next(){
	if(last - iter + 1 >= 0) return *++iter;
	return *iter;
}

template<typename T, typename Allocator>
inline T& array<T, Allocator>::peek(int i){
	if(last - iter + 1 >= 0) return *(iter + i);
	return *iter;
}

template<typename T, typename Allocator>
inline T& array<T, Allocator>::prev(){
	if(first - iter + 1 >= 0) return *iter--;
}

template<typename T, typename Allocator>
inline T& array<T, Allocator>::lookback(int i){
	if(first - iter + 1 >= 0) return *(iter - i);
}

template<typename T, typename Allocator>
inline T* array<T, Allocator>::nextptr(){
	if(iter + 1 - last >= 0) return iter++;
	else return nullptr;
}

//TODO come up with a better name for this and the corresponding previous overload
template<typename T, typename Allocator>
inline T* array<T, Allocator>::peekptr(int i){
	if(iter + 1 - last >= 0) return iter + i;
	else return nullptr;
}

template<typename T, typename Allocator>
inline T* array<T, Allocator>::prevptr(){
	if(iter - 1 - first >= 0) return iter--;
	else return nullptr;
}

template<typename T, typename Allocator>
inline T* array<T, Allocator>::lookbackptr(int i){
	if(iter - 1 - first >= 0) return iter - i;
	else return nullptr;
}

#endif //DESHI_ARRAY_H