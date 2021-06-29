#pragma once
#ifndef RINGARRAY_H
#define RINGARRAY_H

#include "defines.h"

// RingArray is implemented as a contiguous block of memory that does not automatically
// grow when adding past the initial 'capacity', instead it overwrites old data when full. 
// The 'capacity' can be grown, which will relocate all items to a new location. The 'capacity' 
// can also shrink which will simply reduce the 'capacity' variable but not actually re-allocate
// the original allocation. Lastly, all unused but within 'capacity' items are zero-filled.
// TLDR: back insertion only, front removal only, insertion overwrites old data if array is full

template<typename T>
struct RingArray{
    T* data;      //pointer to the data allocated
    u32 start;    //index of the first item of the ring
    u32 end;      //index of the last item of the ring
    u32 count;    //the number of items in the ring
    u32 capacity; //the maximum number of items in the ring

                  //allocates a contiguous block of 'capacity' zero-filled 'T' structs
    void Init(u32 capacity);

    //deallocates 'data' previously allocated in the constructor
    void Free();

    //adds '_count' of 'items' at the end of the ring, pushing 'end' forward
    //overwrites old data if it goes above 'capacity', pushing 'end' and 'start' forward
    void Add(T item);
    void Add(T* items, u32 _count);
    void Add(const T* items, u32 _count);

    //removes '_count' items at the 'start' in the ring, moves 'start' forward by '_count'
    void Remove(u32 _count);

    //returns a pointer to the item at 'position' in the ring, 0 otherwise
    T* At(u32 position);

    //returns a pointer to the item at 'position' in the ring, asserts false if out of bounds
    T& operator[](u32 position);

    //returns true if the ring array is full
    bool Full();

    //returns true if the ring array is empty
    bool Empty();

    //clears the items in the ring array
    void Clear();

    //grows the 'capacity' to fit the 'new_capacity' if 'new_capacity'
    //does nothing if 'new_capacity' is less than or equal to 'capacity'
    //reorganizes the memory so that 'head' is at position zero
    //NOTE reallocates the entire ring if successful growth
    void Grow(u32 new_capacity);

    //shrinks the 'capacity' to be no more than 'new_capacity'
    //does nothing if 'new_capacity' is more than or equal to 'capacity'
    //reorganizes the memory so that 'head' is at position zero
    //NOTE no reallocation or freeing of allocated memory happens
    void Shrink(u32 new_capacity);
};

template<typename T>
inline void RingArray<T>::Init(u32 new_capacity){
    data = (T*)calloc(new_capacity, sizeof(T));
    start = 0;
    end = -1;
    count = 0;
    capacity = new_capacity;
}

template<typename T>
inline void RingArray<T>::Free(){
    free(data);
}

template<typename T>
inline void RingArray<T>::Add(T item){
    end += 1;
    if(end >= capacity) end = 0;
    data[end] = item;

    if(Full()){
        start += 1;
        if(start >= capacity) start = 0;
    }else{
        count += 1;
    }
}

template<typename T>
inline void RingArray<T>::Add(T* items, u32 _count){
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
inline void RingArray<T>::Add(const T* items, u32 _count){
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
inline void RingArray<T>::Remove(u32 _count){
    if(_count >= count) { Clear(); return; }

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
inline T* RingArray<T>::At(u32 position){
    if(position == -1) position = count-1;

    T* result = 0;
    if(position < count){
        position = start + position;
        if(position > capacity) position = position - capacity;
        result = &data[position];
    }
    return result;
}

template<typename T>
inline T& RingArray<T>::operator[](u32 position){
    Assert(position < count);
    return data[position];
}

template<typename T>
inline bool RingArray<T>::Full(){
    return count == capacity;
}

template<typename T>
inline bool RingArray<T>::Empty(){
    return count == 0;
}

template<typename T>
inline void RingArray<T>::Clear(){
    memset(data, 0, sizeof(T)*capacity);
    count = 0;
    start = 0;
    end = -1;
}

template<typename T>
inline void RingArray<T>::Grow(u32 new_capacity){
    if(new_capacity && new_capacity > capacity){
        T* temp = (T*)calloc(new_capacity, sizeof(T));
        if(count){
            memcpy(temp, data+start, sizeof(T)*(capacity-start));
            memcpy(temp+(capacity-start), data, sizeof(T)*(start));
        }
        free(data);
        data = temp;
        capacity = new_capacity;
        start = 0;
        end = count-1;
    }
}

template<typename T>
inline void RingArray<T>::Shrink(u32 new_capacity){
    if(new_capacity < count){          //new_capacity < count < capacity
        if(start) memcpy(data, data+start, sizeof(T)*new_capacity);
        memset(data+new_capacity, 0, capacity-new_capacity); //@Debug
        count = new_capacity;
        capacity = new_capacity;
        start = 0;
        end = count-1;
    }else if(new_capacity < capacity){ //count < new_capacity < capacity
        if((start + count > new_capacity) && (start + count > capacity)){
            u32 left_of_start = (start + count) - capacity;
            u32 right_of_start = capacity - start;
            void* temp = calloc(left_of_start, sizeof(T));
            defer{ free(temp); };
            memcpy(temp, data, sizeof(T)*(left_of_start));
            memcpy(data, data+start, sizeof(T)*(right_of_start));
            memcpy(data+right_of_start, temp, sizeof(T)*(left_of_start));
        }else{ //if not wrapped or less than new_capacity, just copy items
            memcpy(data, data+start, sizeof(T)*count);
        }
        memset(data+count, 0, new_capacity-count); //@Debug
        capacity = new_capacity;
        start = 0;
        end = count-1;
    }
}

#endif //RINGARRAY_H