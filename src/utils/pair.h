#pragma once
#ifndef DESHI_PAIR_H
#define DESHI_PAIR_H

template<typename First, typename Second>
struct pair {
	First a;
	Second b;

	pair(First a, Second b) {
		this->a = a;
		this->b = b;
	}
};



#endif