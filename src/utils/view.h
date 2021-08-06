#pragma once
#ifndef DESHI_VIEW_H
#define DESHI_VIEW_H

//View is a non-owning 'view' over a set of data
template<typename T>
struct View{
	T*  data;
	size_t count;
	
	View(){
		data  = 0;
		count = 0;
	}
	
	View(T* data, size_t count){
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
