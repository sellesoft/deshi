#pragma once
#ifndef DESHI_TUPLE_H
#define DESHI_TUPLE_H

//probably dont use this yet, or ever
//i just had fun making it 

template<class... T>
struct tuple {
	void* mem = 0;
	u32* sizes = 0;
	u32 total = 0;

	tuple(T... args) {
		const int numargs = sizeof...(T);
		const int uintsize = sizeof(0U);

		mem = malloc((sizeof(args) + ...));
		sizes = (u32*)malloc(numargs * uintsize);

		total = (sizeof(args) + ...);

		void* ptrs[numargs] = { (void*)&args... };
		u32 sa[numargs] = { sizeof(args)... };

		for (int i = 0, j = 0; i < numargs; i++, j += sa[i]) {
			memcpy((char*)mem + j, ptrs[i], sa[i]);
			memcpy(sizes + i, &sa[i], uintsize);
		}
	}

	~tuple() {
		free(realloc(mem, total));
		free(realloc(sizes, sizeof...(T) * sizeof(0U)));
	}

	template<class S>
	S* get(int i) {
		for (int o = 0, k = 0; o < i; k += *(sizes + o), o++) {
			if (o == i - 1) return (S*)((char*)mem + k);
		}
	}

};

#endif