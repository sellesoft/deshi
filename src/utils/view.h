#pragma once
#ifndef DESHI_VIEW_H
#define DESHI_VIEW_H

#include "../defines.h"

//view is a non-owning 'view' over a set of data
template<typename T>
struct view{
	T*  data;
	upt count;
	
	FORCE_INLINE T& operator[](upt index){ return data[index]; }
	FORCE_INLINE T* at(size_t index){ return &data[index]; }
	FORCE_INLINE T* begin(){ return &data[0]; }
	FORCE_INLINE T* end()  { return &data[count]; }
	FORCE_INLINE const T* begin()const{ return &data[0]; }
	FORCE_INLINE const T* end()  const{ return &data[count]; }
};

#endif //DESHI_VIEW_H
