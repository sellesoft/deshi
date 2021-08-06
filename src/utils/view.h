#pragma once
#ifndef DESHI_VIEW_H
#define DESHI_VIEW_H

//view is a non-owning 'view' over a set of data
template<typename T>
struct view{
	T*  data;
	size_t count;
	
	view(){
		data  = 0;
		count = 0;
	}
	
	view(T* data, int count){
		this->data  = data;
		this->count = count;
	}
	
	T& operator[](size_t index){
		return data[index];
	}
	
	T* at(size_t index){
		return &data[index];
	}
	
	T* begin(){ return &data[0]; }
	T* end()  { return &data[count]; }
	const T* begin()const{ return &data[0]; }
	const T* end()  const{ return &data[count]; }
};

#endif //DESHI_VIEW_H
