#pragma once
#ifndef DESHI_VIEW_H
#define DESHI_VIEW_H

//view is a non-owning 'view' over a set of data
template<typename T>
struct view{
	T*  data;
	upt count;
	
	T& operator[](upt index){ return data[index]; }
	T* at(size_t index){ return &data[index]; }
	T* begin(){ return &data[0]; }
	T* end()  { return &data[count]; }
	const T* begin()const{ return &data[0]; }
	const T* end()  const{ return &data[count]; }
};

#endif //DESHI_VIEW_H
