#pragma once
#ifndef DESHI_TYPE_H
#define DESHI_TYPE_H

#include "utils.h"
#include "../defines.h"

//left over idea for trying to store type info for metaprogramming
//i dont know of anyway to actually use this info in any meaningful way,
//even though we can store what type a var is and its offset, theres no way for
//us to actually transform that data using what we have, since we can't cast the data

struct type {
	const type* vars[256];
	const u32 var_count = 1;
	const u32 var_offset = 0;
	const u32 var_size = 0;
};


#define t_u8 (name, sname) new type<u8>{{0}, 1, offsetof(sname, name), u8size}
#define t_u16(name, sname) new type<u16>{{0}, 1, offsetof(sname, name), u16size}
#define t_u32(name, sname) new type<u32>{{0}, 1, offsetof(sname, name), u32size}
#define t_u64(name, sname) new type<u64>{{0}, 1, offsetof(sname, name), u64size}
#define t_s8 (name, sname) new type<s8>{{0}, 1, offsetof(sname, name), s8size}
#define t_s16(name, sname) new type<s16>{{0}, 1, offsetof(sname, name), s16size}
#define t_s32(name, sname) new type<s32>{{0}, 1, offsetof(sname, name), s32size}
#define t_s64(name, sname) new type<s64>{{0}, 1, offsetof(sname, name), s64size}
#define t_spt(name, sname) new type<spt>{{0}, 1, offsetof(sname, name), sptsize}
#define t_upt(name, sname) new type<upt>{{0}, 1, offsetof(sname, name), uptsize}
#define t_f32(name, sname) new type<f32>{{0}, 1, offsetof(sname, name), f32size}
#define t_f64(name, sname) new type<f64>{{0}, 1, offsetof(sname, name), f64size}
#define t_b32(name, sname) new type<b32>{{0}, 1, offsetof(sname, name), b32size}
#define t_uchar(name) new type{"uchar", 1, 0, ucharsize, {0}}

#endif //DESHI_TYPE_H