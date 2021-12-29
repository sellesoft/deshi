#pragma once
#ifndef DESHI_CARRAY_H
#define DESHI_CARRAY_H

#include "../defines.h"
#include <cstring> //memcpy, memset

//NOTE The purposed of these functions is to avoid rewriting their implementations everywhere, not to be fully functional.
//     As such, they do no error checking and do not change external values, so that stuff should be handled externally.
//     Also, I don't guarantee proper moving of complex types using operator=(), I just use memcpy().

//removes the last items
template<typename T> function void
array_pop(T* arr, upt arr_count, upt count = 1){
	forI(count){ arr[arr_count--].~T(); }
	memset(arr+arr_count, 0, count*sizeof(T));
}
template<typename T> FORCE_INLINE void array_pop(carray<T> arr, upt count = 1){ array_pop(arr.data, arr.count, count); }

//swaps the last item with the item at index, then pops one
template<typename T> function void
array_remove_unordered(T* arr, upt arr_count, upt idx){
	arr[idx] = arr[arr_count-1];
	array_pop(arr, arr_count, 1);
}
template<typename T> FORCE_INLINE void array_remove_unordered(carray<T> arr, upt idx){ array_remove_unordered(arr.data, arr.count, idx); }

//shifts all items left of index to the left by one
template<typename T> function void
array_remove_ordered(T* arr, upt arr_count, upt idx){
	arr[idx].~T();
	memmove(arr+idx, arr+idx+1, (arr_count-idx)*sizeof(T));
	memset(arr+arr_count-1, 0, sizeof(T));
}
template<typename T> FORCE_INLINE void array_remove_ordered(carray<T> arr, upt idx){ array_remove_ordered(arr.data, arr.count, idx); }

//shift all items at and to the right of index to the right by one
template<typename T> function void
array_insert(T* arr, upt arr_count, T& item, upt idx){
	memmove(arr+idx+1, arr+idx, (arr_count-idx)*sizeof(T));
	arr[idx] = item;
}
template<typename T> FORCE_INLINE void array_insert(carray<T> arr, T& item, upt idx){ array_insert(arr.data, arr.count, item, idx); }

#endif //DESHI_CARRAY_H