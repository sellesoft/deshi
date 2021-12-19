#pragma once
#ifndef DESHI_ARRAY_SORTING_H
#define DESHI_ARRAY_SORTING_H

#include "array.h"

template<typename T> void
bubble_sort_low_to_high(array<T>& arr){
	if(arr.count < 2) return;
	b32 swapped;
	for(int i = 0; i < arr.count-1; ++i){
		swapped = false;
		for(int j = 0; j < arr.count-i-1; ++j){
			if(arr[j] > arr[j+1]){
				Swap(arr[j], arr[j+1]);
				swapped = true;
			}
		}
		if(!swapped) break;
	}
}


template<typename T, class Compare> void
bubble_sort(array<T>& arr, Compare comp) {
	if (arr.count < 2) return;
	b32 swapped;
	for (int i = 0; i < arr.count - 1; ++i) {
		swapped = false;
		for (int j = 0; j < arr.count - i - 1; ++j) {
			if (comp(arr[j], arr[j + 1])) {
				Swap(arr[j], arr[j + 1]);
				swapped = true;
			}
		}
		if (!swapped) break;
	}
}

template<typename T> void
bubble_sort_high_to_low(array<T>& arr){
	if(arr.count < 2) return;
	b32 swapped;
	for(int i = 0; i < arr.count-1; ++i){
		swapped = false;
		for(int j = 0; j < arr.count-i-1; ++j){
			if(arr[j] < arr[j+1]){
				Swap(arr[j], arr[j+1]);
				swapped = true;
			}
		}
		if(!swapped) break;
	}
}



#endif //DESHI_ARRAY_SORTING_H
