#pragma once
#ifndef DESHI_ARRAY_UTILS_H
#define DESHI_ARRAY_UTILS_H

#include "../defines.h"
#include "array.h"


//////////////////////
//// @bubble sort ////
//////////////////////
template<typename T, class Compare> void
bubble_sort(T* arr, upt count, Compare comp) {
	if(arr == 0) return;
	if(count < 2) return;
	
	b32 swapped;
	for(int i = 0; i < count-1; ++i){
		swapped = false;
		for(int j = 0; j < count-i-1; ++j){
			if(comp(arr[j], arr[j+1])){
				Swap(arr[j], arr[j+1]);
				swapped = true;
			}
		}
		if(!swapped) break;
	}
}
template<typename T, class Compare> FORCE_INLINE void bubble_sort(T* first, T* last, Compare comp){ if(last > first){ bubble_sort(first, last-first, comp); } }
template<typename T, class Compare> FORCE_INLINE void bubble_sort(array<T>& arr, Compare comp){ bubble_sort(arr.data, arr.count, comp); }
template<typename T, class Compare> FORCE_INLINE void bubble_sort(carray<T> arr, Compare comp){ bubble_sort(arr.data, arr.count, comp); }

template<typename T> void
bubble_sort_low_to_high(T* arr, upt count){
	if(arr == 0) return;
	if(count < 2) return;
	
	b32 swapped;
	for(int i = 0; i < count-1; ++i){
		swapped = false;
		for(int j = 0; j < count-i-1; ++j){
			if(arr[j] > arr[j+1]){
				Swap(arr[j], arr[j+1]);
				swapped = true;
			}
		}
		if(!swapped) break;
	}
}
template<typename T> FORCE_INLINE void bubble_sort_low_to_high(T* first, T* last){ if(last > first){ bubble_sort_low_to_high(first, last-first); } }
template<typename T> FORCE_INLINE void bubble_sort_low_to_high(array<T>& arr){ bubble_sort_low_to_high(arr.data, arr.count); }
template<typename T> FORCE_INLINE void bubble_sort_low_to_high(carray<T> arr){ bubble_sort_low_to_high(arr.data, arr.count); }

template<typename T> void
bubble_sort_high_to_low(T* arr, upt count){
	if(arr == 0) return;
	if(count < 2) return;
	
	b32 swapped;
	for(int i = 0; i < count-1; ++i){
		swapped = false;
		for(int j = 0; j < count-i-1; ++j){
			if(arr[j] < arr[j+1]){
				Swap(arr[j], arr[j+1]);
				swapped = true;
			}
		}
		if(!swapped) break;
	}
}
template<typename T> FORCE_INLINE void bubble_sort_high_to_low(T* first, T* last){ if(last > first){ bubble_sort_high_to_low(first, last-first); } }
template<typename T> FORCE_INLINE void bubble_sort_high_to_low(array<T>& arr){ bubble_sort_high_to_low(arr.data, arr.count); }
template<typename T> FORCE_INLINE void bubble_sort_high_to_low(carray<T> arr){ bubble_sort_high_to_low(arr.data, arr.count); }


//////////////////
//// @reverse ////
//////////////////
template<typename T> void
reverse(T* arr, upt count){
	if(arr == 0) return;
	if(count < 2) return;
	
	T* last = arr+count;
	while((arr != last) && (arr != --last)){
		Swap(*arr, *last);
		++arr;
	}
}
template<typename T> FORCE_INLINE void reverse(T* first, T* last){ if(last > first){ reverse(first, last-first); } }
template<typename T> FORCE_INLINE void reverse(array<T>& arr){ reverse(arr.data, arr.count); }
template<typename T> FORCE_INLINE void reverse(carray<T> arr){ reverse(arr.data, arr.count); }


//////////////////////// //returns the index of the item in the low-to-high sorted array
//// @binary search //// //if the item is not in the array, returns -1
//////////////////////// //if the array has non-unique items, there's no guarantee which it will select
template<typename T, typename Compare> upt
binary_search(T* arr, upt count, const T& item, Compare less_than){
	if(arr == 0) return -1;
	if(count == 0) return -1;
	
	spt left  = 0;
	spt right = count-1;
	while(left <= right){
		spt middle = left + ((right - left) / 2);
		if      (less_than(arr[middle], item)){
			left = middle + 1;
		}else if(less_than(item, arr[middle])){
			right = middle - 1;
		}else{
			return middle;
		}
	}
	return -1;
}
template<typename T, typename Compare> FORCE_INLINE upt binary_search(T* first, T* last, const T& item, Compare less_than){ return (last > first) ? binary_search(first, last-first, item, less_than) : -1; }
template<typename T, typename Compare> FORCE_INLINE upt binary_search(array<T>& arr, const T& item, Compare less_than){ return binary_search(arr.data, arr.count, item, less_than); }
template<typename T, typename Compare> FORCE_INLINE upt binary_search(carray<T> arr, const T& item, Compare less_than){ return binary_search(arr.data, arr.count, item, less_than); }

template<typename T> upt
binary_search_low_to_high(T* arr, upt count, const T& item){
	if(arr == 0) return -1;
	if(count == 0) return -1;
	
	spt left  = 0;
	spt right = count-1;
	while(left <= right){
		spt middle = left + ((right - left) / 2);
		if(arr[middle] == item) return middle;
		if(arr[middle] < item){
			left = middle + 1;
		}else{
			right = middle - 1;
		}
	}
	return -1;
}
template<typename T> FORCE_INLINE upt binary_search_low_to_high(T* first, T* last, const T& item){ return (last > first) ? binary_search_low_to_high(first, last-first, item) : -1; }
template<typename T> FORCE_INLINE upt binary_search_low_to_high(array<T>& arr, const T& item){ return binary_search_low_to_high(arr.data, arr.count, item); }
template<typename T> FORCE_INLINE upt binary_search_low_to_high(carray<T> arr, const T& item){ return binary_search_low_to_high(arr.data, arr.count, item); }

#endif //DESHI_ARRAY_UTILS_H
