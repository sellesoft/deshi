#pragma once
#ifndef RINGARRAY_H
#define RINGARRAY_H

// ring_array is implemented as a contiguous block of memory that does not automatically
// grow when adding past the initial 'capacity', instead it overwrites old data when full. 
// The 'capacity' can be grown, which will relocate all items to a new location. The 'capacity' 
// can also shrink which will simply reduce the 'capacity' variable but not actually re-allocate
// the original allocation. Lastly, all unused but within 'capacity' items are zero-filled.
// TLDR: back insertion only, front removal only, insertion overwrites old data if array is full

#include "../defines.h"

template<typename T>
struct ring_array{
	T* data;      //pointer to the data allocated
	u32 start;    //index of the first item of the ring
	u32 end;      //index of the last item of the ring
	u32 count;    //the number of items in the ring
	u32 capacity; //the maximum number of items in the ring
	Allocator* allocator;
	
	//allocates a contiguous block of 'capacity' zero-filled 'T' structs
	void init(u32 capacity, Allocator* a = stl_allocator);
	//deallocates 'data' previously allocated in the constructor
	void free();
	
	//returns a pointer to the item at 'position' in the ring, asserts false if out of bounds
	T& operator[](u32 position);
	T  operator[](u32 position) const;
	
	
	//adds '_count' of 'items' at the end of the ring, pushing 'end' forward
	//overwrites old data if it goes above 'capacity', pushing 'end' and 'start' forward
	void add(T item);
	void add(T* items, u32 _count);
	void add(const T* items, u32 _count);
	
	//removes '_count' items at the 'start' in the ring, moves 'start' forward by '_count'
	void remove(u32 _count);
	
	//returns a pointer to the item at 'position' in the ring, 0 otherwise
	T* at(u32 position);
	
	//returns true if the ring array is full
	bool full() const;
	
	//returns true if the ring array is empty
	bool empty() const;
	
	//clears the items in the ring array
	void clear();
	
	//grows the 'capacity' to fit the 'new_capacity' if 'new_capacity'
	//does nothing if 'new_capacity' is less than or equal to 'capacity'
	//reorganizes the memory so that 'head' is at position zero
	//NOTE reallocates the entire ring if successful growth
	void grow(u32 new_capacity);
	
	//shrinks the 'capacity' to be no more than 'new_capacity'
	//does nothing if 'new_capacity' is more than or equal to 'capacity'
	//reorganizes the memory so that 'head' is at position zero
	//NOTE no reallocation or freeing of allocated memory happens
	void shrink(u32 new_capacity);
};

template<typename T>
inline void ring_array<T>::init(u32 new_capacity, Allocator* a){
	allocator = a;
	data = (T*)allocator->reserve(new_capacity*sizeof(T));
	allocator->commit(data, new_capacity*sizeof(T));
	start = 0;
	end = -1;
	count = 0;
	capacity = new_capacity;
}

template<typename T>
inline void ring_array<T>::free(){
	forI(count){ data[i].~T(); }
	allocator->release(data);
}

template<typename T>
inline void ring_array<T>::add(T item){
	end += 1;
	if(end >= capacity) end = 0;
	data[end] = item;
	
	if(full()){
		start += 1;
		if(start >= capacity) start = 0;
	}else{
		count += 1;
	}
}

template<typename T>
inline void ring_array<T>::add(T* items, u32 _count){
	s32 tail_wrap = (end + 1 + _count) - capacity;
	
	if(tail_wrap > 0){
		memcpy(data+(end+1), items, sizeof(T)*(_count - tail_wrap));
		memcpy(data, items+(_count - tail_wrap), sizeof(T)*(tail_wrap));
		
		count += _count;
		if(count >= capacity){
			count = capacity;
			start = tail_wrap;
		}
		end = tail_wrap - 1;
	}else{
		memcpy(data+(end+1), items, sizeof(T)*(_count)); 
		
		end += _count;
		count += _count;
		if(count >= capacity){
			count = capacity;
			start += _count;
			if(start >= capacity) start = (start - capacity);
		}
	}
}

template<typename T>
inline void ring_array<T>::add(const T* items, u32 _count){
	s32 tail_wrap = (end + 1 + _count) - capacity;
	
	if(tail_wrap > 0){
		memcpy(data+(end+1), items, sizeof(T)*(_count - tail_wrap));
		memcpy(data, items+(_count - tail_wrap), sizeof(T)*(tail_wrap));
		
		count += _count;
		if(count >= capacity){
			count = capacity;
			start = tail_wrap;
		}
		end = tail_wrap - 1;
	}else{
		memcpy(data+(end+1), items, sizeof(T)*(_count)); 
		
		end += _count;
		count += _count;
		if(count >= capacity){
			count = capacity;
			start += _count;
			if(start >= capacity) start = (start - capacity);
		}
	}
}

template<typename T>
inline void ring_array<T>::remove(u32 _count){
	if(_count >= count) { clear(); return; }
	
	s32 remove_wrap_count = (start + _count) - capacity;
	
	if(remove_wrap_count > 0){
		memset(data+start, 0, sizeof(T)*(capacity-start));
		memset(data, 0, sizeof(T)*(remove_wrap_count));
	}else{
		memset(data+start, 0, sizeof(T)*_count);
	}
	count -= _count;
	start += _count;
	if(start >= capacity) start = start - capacity;
}

template<typename T>
inline T* ring_array<T>::at(u32 position){
	T* result = 0;
	if(position < count){
		position = (start + position) % capacity;
		result = &data[position];
	}
	return result;
}

template<typename T>
inline T& ring_array<T>::operator[](u32 position){
	return *at(position);
}

template<typename T>
inline T ring_array<T>::operator[](u32 position) const {
	return *at(position);
}

template<typename T>
inline bool ring_array<T>::full() const{
	return count == capacity;
}

template<typename T>
inline bool ring_array<T>::empty() const{
	return count == 0;
}

template<typename T>
inline void ring_array<T>::clear(){
	forI(count){ data[i].~T(); }
	memset(data, 0, sizeof(T)*capacity);
	count = 0;
	start = 0;
	end = -1;
}

template<typename T>
inline void ring_array<T>::grow(u32 new_capacity){
	if(new_capacity && new_capacity > capacity){
		T* temp = (T*)allocator->reserve(new_capacity*sizeof(T));
		allocator->commit(data, new_capacity*sizeof(T));
		if(count){
			memcpy(temp, data+start, sizeof(T)*(capacity-start));
			memcpy(temp+(capacity-start), data, sizeof(T)*(start));
		}
		allocator->release(data);
		data = temp;
		capacity = new_capacity;
		start = 0;
		end = count-1;
	}
}

template<typename T>
inline void ring_array<T>::shrink(u32 new_capacity){
	if(new_capacity < count){          //new_capacity < count < capacity
		if(start) memcpy(data, data+start, sizeof(T)*new_capacity);
#if DESHI_INTERNAL
		memset(data+new_capacity, 0, capacity-new_capacity);
#endif
		count = new_capacity;
		capacity = new_capacity;
		start = 0;
		end = count-1;
	}else if(new_capacity < capacity){ //count < new_capacity < capacity
		if((start + count > new_capacity) && (start + count > capacity)){
			u32 left_of_start = (start + count) - capacity;
			u32 right_of_start = capacity - start;
			void* temp = allocator->reserve(left_of_start*sizeof(T));
			allocator->commit(left_of_start*sizeof(T));
			defer{ allocator->release(temp); };
			memcpy(temp, data, sizeof(T)*(left_of_start));
			memcpy(data, data+start, sizeof(T)*(right_of_start));
			memcpy(data+right_of_start, temp, sizeof(T)*(left_of_start));
		}else{ //if not wrapped or less than new_capacity, just copy items
			memcpy(data, data+start, sizeof(T)*count);
		}
#if DESHI_INTERNAL
		memset(data+count, 0, new_capacity-count);
#endif
		capacity = new_capacity;
		start = 0;
		end = count-1;
	}
}

#endif //RINGARRAY_H