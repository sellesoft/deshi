#pragma once
#ifndef DESHI_MAT4_INL
#define DESHI_MAT4_INL

//////////////////////
//// constructors ////
//////////////////////

inline mat4::
mat4(f32 x){
#if DESHI_USE_SSE
	sse_row0 = _mm_set1_ps(x);
	sse_row1 = _mm_set1_ps(x);
	sse_row2 = _mm_set1_ps(x);
	sse_row3 = _mm_set1_ps(x);
#else
	arr[ 0] = x; arr[ 1] = x; arr[ 2] = x; arr[ 3] = x;
	arr[ 4] = x; arr[ 5] = x; arr[ 6] = x; arr[ 7] = x;
	arr[ 8] = x; arr[ 9] = x; arr[10] = x; arr[11] = x;
	arr[12] = x; arr[13] = x; arr[14] = x; arr[15] = x;
#endif
}

inline mat4::
mat4(f32 _00, f32 _01, f32 _02, f32 _03,
	 f32 _10, f32 _11, f32 _12, f32 _13,
	 f32 _20, f32 _21, f32 _22, f32 _23,
	 f32 _30, f32 _31, f32 _32, f32 _33){
#if DESHI_USE_SSE
	sse_row0 = _mm_setr_ps(_00, _01, _02, _03);
	sse_row1 = _mm_setr_ps(_10, _11, _12, _13);
	sse_row2 = _mm_setr_ps(_20, _21, _22, _23);
	sse_row3 = _mm_setr_ps(_30, _31, _32, _33);
#else
	arr[ 0] = _00; arr[ 1] = _01; arr[ 2] = _02; arr[ 3] = _03;
	arr[ 4] = _10; arr[ 5] = _11; arr[ 6] = _12; arr[ 7] = _13;
	arr[ 8] = _20; arr[ 9] = _21; arr[10] = _22; arr[11] = _23;
	arr[12] = _30; arr[13] = _31; arr[14] = _32; arr[15] = _33;
#endif
}

inline mat4::
mat4(const mat4& m){
#if DESHI_USE_SSE
	sse_row0 = m.sse_row0;
	sse_row1 = m.sse_row1;
	sse_row2 = m.sse_row2;
	sse_row3 = m.sse_row3;
#else
	arr[ 0] = m.arr[ 0]; arr[ 1] = m.arr[ 1]; arr[ 2] = m.arr[ 2]; arr[ 3] = m.arr[ 3];
	arr[ 4] = m.arr[ 4]; arr[ 5] = m.arr[ 5]; arr[ 6] = m.arr[ 6]; arr[ 7] = m.arr[ 7]; 
	arr[ 8] = m.arr[ 8]; arr[ 9] = m.arr[ 9]; arr[10] = m.arr[10]; arr[11] = m.arr[11];
	arr[12] = m.arr[12]; arr[13] = m.arr[13]; arr[14] = m.arr[14]; arr[15] = m.arr[15];
#endif
}

inline mat4::
mat4(f32* src){
#if DESHI_USE_SSE
	sse_row0 = _mm_load_ps(src);
	sse_row1 = _mm_load_ps(src+4);
	sse_row2 = _mm_load_ps(src+8);
	sse_row3 = _mm_load_ps(src+12);
#else
	arr[ 0] = src[ 0]; arr[ 1] = src[ 1]; arr[ 2] = src[ 2]; arr[ 3] = src[ 3];
	arr[ 4] = src[ 4]; arr[ 5] = src[ 5]; arr[ 6] = src[ 6]; arr[ 7] = src[ 7]; 
	arr[ 8] = src[ 8]; arr[ 9] = src[ 9]; arr[10] = src[10]; arr[11] = src[11];
	arr[12] = src[12]; arr[13] = src[13]; arr[14] = src[14]; arr[15] = src[15];
#endif
}

///////////////////
//// constants ////
///////////////////

inline const mat4 mat4::IDENTITY = mat4(1, 0, 0, 0, 
										0, 1, 0, 0, 
										0, 0, 1, 0, 
										0, 0, 0, 1);

///////////////////
//// operators ////
///////////////////

//element accessor: matrix(row,col)
inline f32& mat4::
operator()(u32 row, u32 col){
	Assert(row < 4 && col < 4, "mat4 subscript out of bounds");
	return arr[4*row + col];
}

inline f32 mat4::
operator()(u32 row, u32 col) const{
	Assert(row < 4 && col < 4, "mat4 subscript out of bounds");
	return arr[4*row + col];
}

//creates the data from rhs
inline void mat4::
operator= (const mat4& rhs){
#if DESHI_USE_SSE
	sse_row0 = rhs.sse_row0;
	sse_row1 = rhs.sse_row1;
	sse_row2 = rhs.sse_row2;
	sse_row3 = rhs.sse_row3;
#else
	arr[ 0] = rhs.arr[ 0]; arr[ 1] = rhs.arr[ 1]; arr[ 2] = rhs.arr[ 2]; arr[ 3] = rhs.arr[ 3];
	arr[ 4] = rhs.arr[ 4]; arr[ 5] = rhs.arr[ 5]; arr[ 6] = rhs.arr[ 6]; arr[ 7] = rhs.arr[ 7]; 
	arr[ 8] = rhs.arr[ 8]; arr[ 9] = rhs.arr[ 9]; arr[10] = rhs.arr[10]; arr[11] = rhs.arr[11];
	arr[12] = rhs.arr[12]; arr[13] = rhs.arr[13]; arr[14] = rhs.arr[14]; arr[15] = rhs.arr[15];
#endif
}

//scalar multiplication
inline mat4 mat4::
operator* (const f32& rhs) const{
	mat4 result;
#if DESHI_USE_SSE
	__m128 scalar = _mm_set1_ps(rhs);
	result.sse_row0 = _mm_mul_ps(sse_row0, scalar);
	result.sse_row1 = _mm_mul_ps(sse_row1, scalar);
	result.sse_row2 = _mm_mul_ps(sse_row2, scalar);
	result.sse_row3 = _mm_mul_ps(sse_row3, scalar);
#else
	result.arr[ 0] = arr[ 0]*rhs; result.arr[ 1] = arr[ 1]*rhs; result.arr[ 2] = arr[ 2]*rhs; result.arr[ 3] = arr[ 3]*rhs;
	result.arr[ 4] = arr[ 4]*rhs; result.arr[ 5] = arr[ 5]*rhs; result.arr[ 6] = arr[ 6]*rhs; result.arr[ 7] = arr[ 7]*rhs;
	result.arr[ 8] = arr[ 8]*rhs; result.arr[ 9] = arr[ 9]*rhs; result.arr[10] = arr[10]*rhs; result.arr[11] = arr[11]*rhs;
	result.arr[12] = arr[12]*rhs; result.arr[13] = arr[13]*rhs; result.arr[14] = arr[14]*rhs; result.arr[15] = arr[15]*rhs;
#endif
	return result;
}

//scalar multiplication and assignment
inline void mat4::
operator*=(const f32& rhs){
#if DESHI_USE_SSE
	__m128 scalar = _mm_set1_ps(rhs);
	sse_row0 = _mm_mul_ps(sse_row0, scalar);
	sse_row1 = _mm_mul_ps(sse_row1, scalar);
	sse_row2 = _mm_mul_ps(sse_row2, scalar);
	sse_row3 = _mm_mul_ps(sse_row3, scalar);
#else
	arr[ 0] *= rhs; arr[ 1] *= rhs; arr[ 2] *= rhs; arr[ 3] *= rhs;
	arr[ 4] *= rhs; arr[ 5] *= rhs; arr[ 6] *= rhs; arr[ 7] *= rhs;
	arr[ 8] *= rhs; arr[ 9] *= rhs; arr[10] *= rhs; arr[11] *= rhs;
	arr[12] *= rhs; arr[13] *= rhs; arr[14] *= rhs; arr[15] *= rhs;
#endif
}

//scalar division
inline mat4 mat4::
operator/ (const f32& rhs) const{
	Assert(rhs != 0, "mat4 elements cant be divided by zero");
	mat4 result;
#if DESHI_USE_SSE
	__m128 scalar = _mm_set1_ps(rhs);
	result.sse_row0 = _mm_div_ps(sse_row0, scalar);
	result.sse_row1 = _mm_div_ps(sse_row1, scalar);
	result.sse_row2 = _mm_div_ps(sse_row2, scalar);
	result.sse_row3 = _mm_div_ps(sse_row3, scalar);
#else
	result.arr[ 0] = arr[ 0]/rhs; result.arr[ 1] = arr[ 1]/rhs; result.arr[ 2] = arr[ 2]/rhs; result.arr[ 3] = arr[ 3]/rhs;
	result.arr[ 4] = arr[ 4]/rhs; result.arr[ 5] = arr[ 5]/rhs; result.arr[ 6] = arr[ 6]/rhs; result.arr[ 7] = arr[ 7]/rhs;
	result.arr[ 8] = arr[ 8]/rhs; result.arr[ 9] = arr[ 9]/rhs; result.arr[10] = arr[10]/rhs; result.arr[11] = arr[11]/rhs;
	result.arr[12] = arr[12]/rhs; result.arr[13] = arr[13]/rhs; result.arr[14] = arr[14]/rhs; result.arr[15] = arr[15]/rhs;
#endif
	return result;
}

//scalar division and assignment
inline void mat4::
operator/=(const f32& rhs){
	Assert(rhs != 0, "mat4 elements cant be divided by zero");
#if DESHI_USE_SSE
	__m128 scalar = _mm_set1_ps(rhs);
	sse_row0 = _mm_div_ps(sse_row0, scalar);
	sse_row1 = _mm_div_ps(sse_row1, scalar);
	sse_row2 = _mm_div_ps(sse_row2, scalar);
	sse_row3 = _mm_div_ps(sse_row3, scalar);
#else
	arr[ 0] /= rhs; arr[ 1] /= rhs; arr[ 2] /= rhs; arr[ 3] /= rhs;
	arr[ 4] /= rhs; arr[ 5] /= rhs; arr[ 6] /= rhs; arr[ 7] /= rhs;
	arr[ 8] /= rhs; arr[ 9] /= rhs; arr[10] /= rhs; arr[11] /= rhs;
	arr[12] /= rhs; arr[13] /= rhs; arr[14] /= rhs; arr[15] /= rhs;
#endif
}

//element-wise addition
inline mat4 mat4::
operator+ (const mat4& rhs) const{
	mat4 result;
#if DESHI_USE_SSE
	result.sse_row0 = _mm_add_ps(sse_row0, rhs.sse_row0);
	result.sse_row1 = _mm_add_ps(sse_row1, rhs.sse_row1);
	result.sse_row2 = _mm_add_ps(sse_row2, rhs.sse_row2);
	result.sse_row3 = _mm_add_ps(sse_row3, rhs.sse_row3);
#else
	result.arr[ 0] = arr[ 0]+rhs.arr[ 0]; result.arr[ 1] = arr[ 1]+rhs.arr[ 1]; result.arr[ 2] = arr[ 2]+rhs.arr[ 2]; result.arr[ 3] = arr[ 3]+rhs.arr[ 3];
	result.arr[ 4] = arr[ 4]+rhs.arr[ 4]; result.arr[ 5] = arr[ 5]+rhs.arr[ 5]; result.arr[ 6] = arr[ 6]+rhs.arr[ 6]; result.arr[ 7] = arr[ 7]+rhs.arr[ 7];
	result.arr[ 8] = arr[ 8]+rhs.arr[ 8]; result.arr[ 9] = arr[ 9]+rhs.arr[ 9]; result.arr[10] = arr[10]+rhs.arr[10]; result.arr[11] = arr[11]+rhs.arr[11];
	result.arr[12] = arr[12]+rhs.arr[12]; result.arr[13] = arr[13]+rhs.arr[13]; result.arr[14] = arr[14]+rhs.arr[14]; result.arr[15] = arr[15]+rhs.arr[15];
#endif
	return result;
}

//element-wise addition and assignment
inline void mat4::
operator+=(const mat4& rhs){
#if DESHI_USE_SSE
	sse_row0 = _mm_add_ps(sse_row0, rhs.sse_row0);
	sse_row1 = _mm_add_ps(sse_row1, rhs.sse_row1);
	sse_row2 = _mm_add_ps(sse_row2, rhs.sse_row2);
	sse_row3 = _mm_add_ps(sse_row3, rhs.sse_row3);
#else
	arr[ 0] += rhs.arr[ 0]; arr[ 1] += rhs.arr[ 1]; arr[ 2] += rhs.arr[ 2]; arr[ 3] += rhs.arr[ 3];
	arr[ 4] += rhs.arr[ 4]; arr[ 5] += rhs.arr[ 5]; arr[ 6] += rhs.arr[ 6]; arr[ 7] += rhs.arr[ 7];
	arr[ 8] += rhs.arr[ 8]; arr[ 9] += rhs.arr[ 9]; arr[10] += rhs.arr[10]; arr[11] += rhs.arr[11];
	arr[12] += rhs.arr[12]; arr[13] += rhs.arr[13]; arr[14] += rhs.arr[14]; arr[15] += rhs.arr[15];
#endif
}

//element-wise substraction
inline mat4 mat4::
operator- (const mat4& rhs) const{
	mat4 result;
#if DESHI_USE_SSE
	result.sse_row0 = _mm_sub_ps(sse_row0, rhs.sse_row0);
	result.sse_row1 = _mm_sub_ps(sse_row1, rhs.sse_row1);
	result.sse_row2 = _mm_sub_ps(sse_row2, rhs.sse_row2);
	result.sse_row3 = _mm_sub_ps(sse_row3, rhs.sse_row3);
#else
	result.arr[ 0] = arr[ 0]-rhs.arr[ 0]; result.arr[ 1] = arr[ 1]-rhs.arr[ 1]; result.arr[ 2] = arr[ 2]-rhs.arr[ 2]; result.arr[ 3] = arr[ 3]-rhs.arr[ 3];
	result.arr[ 4] = arr[ 4]-rhs.arr[ 4]; result.arr[ 5] = arr[ 5]-rhs.arr[ 5]; result.arr[ 6] = arr[ 6]-rhs.arr[ 6]; result.arr[ 7] = arr[ 7]-rhs.arr[ 7];
	result.arr[ 8] = arr[ 8]-rhs.arr[ 8]; result.arr[ 9] = arr[ 9]-rhs.arr[ 9]; result.arr[10] = arr[10]-rhs.arr[10]; result.arr[11] = arr[11]-rhs.arr[11];
	result.arr[12] = arr[12]-rhs.arr[12]; result.arr[13] = arr[13]-rhs.arr[13]; result.arr[14] = arr[14]-rhs.arr[14]; result.arr[15] = arr[15]-rhs.arr[15];
#endif
	return result;
}

//element-wise substraction and assignment
inline void mat4::
operator-=(const mat4& rhs){
#if DESHI_USE_SSE
	sse_row0 = _mm_sub_ps(sse_row0, rhs.sse_row0);
	sse_row1 = _mm_sub_ps(sse_row1, rhs.sse_row1);
	sse_row2 = _mm_sub_ps(sse_row2, rhs.sse_row2);
	sse_row3 = _mm_sub_ps(sse_row3, rhs.sse_row3);
#else
	arr[ 0] -= rhs.arr[ 0]; arr[ 1] -= rhs.arr[ 1]; arr[ 2] -= rhs.arr[ 2]; arr[ 3] -= rhs.arr[ 3];
	arr[ 4] -= rhs.arr[ 4]; arr[ 5] -= rhs.arr[ 5]; arr[ 6] -= rhs.arr[ 6]; arr[ 7] -= rhs.arr[ 7];
	arr[ 8] -= rhs.arr[ 8]; arr[ 9] -= rhs.arr[ 9]; arr[10] -= rhs.arr[10]; arr[11] -= rhs.arr[11];
	arr[12] -= rhs.arr[12]; arr[13] -= rhs.arr[13]; arr[14] -= rhs.arr[14]; arr[15] -= rhs.arr[15];
#endif
}

//element-wise multiplication
inline mat4 mat4::
operator^ (const mat4& rhs) const{
	mat4 result;
#if DESHI_USE_SSE
	result.sse_row0 = _mm_mul_ps(sse_row0, rhs.sse_row0);
	result.sse_row1 = _mm_mul_ps(sse_row1, rhs.sse_row1);
	result.sse_row2 = _mm_mul_ps(sse_row2, rhs.sse_row2);
	result.sse_row3 = _mm_mul_ps(sse_row3, rhs.sse_row3);
#else
	result.arr[ 0] = arr[ 0]*rhs.arr[ 0]; result.arr[ 1] = arr[ 1]*rhs.arr[ 1]; result.arr[ 2] = arr[ 2]*rhs.arr[ 2]; result.arr[ 3] = arr[ 3]*rhs.arr[ 3];
	result.arr[ 4] = arr[ 4]*rhs.arr[ 4]; result.arr[ 5] = arr[ 5]*rhs.arr[ 5]; result.arr[ 6] = arr[ 6]*rhs.arr[ 6]; result.arr[ 7] = arr[ 7]*rhs.arr[ 7];
	result.arr[ 8] = arr[ 8]*rhs.arr[ 8]; result.arr[ 9] = arr[ 9]*rhs.arr[ 9]; result.arr[10] = arr[10]*rhs.arr[10]; result.arr[11] = arr[11]*rhs.arr[11];
	result.arr[12] = arr[12]*rhs.arr[12]; result.arr[13] = arr[13]*rhs.arr[13]; result.arr[14] = arr[14]*rhs.arr[14]; result.arr[15] = arr[15]*rhs.arr[15];
#endif
	return result;
} 

//element-wise multiplication and assignment
inline void mat4::
operator^=(const mat4& rhs){
#if DESHI_USE_SSE
	sse_row0 = _mm_mul_ps(sse_row0, rhs.sse_row0);
	sse_row1 = _mm_mul_ps(sse_row1, rhs.sse_row1);
	sse_row2 = _mm_mul_ps(sse_row2, rhs.sse_row2);
	sse_row3 = _mm_mul_ps(sse_row3, rhs.sse_row3);
#else
	arr[ 0] *= rhs.arr[ 0]; arr[ 1] *= rhs.arr[ 1]; arr[ 2] *= rhs.arr[ 2]; arr[ 3] *= rhs.arr[ 3];
	arr[ 4] *= rhs.arr[ 4]; arr[ 5] *= rhs.arr[ 5]; arr[ 6] *= rhs.arr[ 6]; arr[ 7] *= rhs.arr[ 7];
	arr[ 8] *= rhs.arr[ 8]; arr[ 9] *= rhs.arr[ 9]; arr[10] *= rhs.arr[10]; arr[11] *= rhs.arr[11];
	arr[12] *= rhs.arr[12]; arr[13] *= rhs.arr[13]; arr[14] *= rhs.arr[14]; arr[15] *= rhs.arr[15];
#endif
}

//element-wise division
inline mat4 mat4::
operator% (const mat4& rhs) const{
	mat4 result;
#if DESHI_USE_SSE
	result.sse_row0 = _mm_div_ps(sse_row0, rhs.sse_row0);
	result.sse_row1 = _mm_div_ps(sse_row1, rhs.sse_row1);
	result.sse_row2 = _mm_div_ps(sse_row2, rhs.sse_row2);
	result.sse_row3 = _mm_div_ps(sse_row3, rhs.sse_row3);
#else
	result.arr[ 0] = arr[ 0]/rhs.arr[ 0]; result.arr[ 1] = arr[ 1]/rhs.arr[ 1]; result.arr[ 2] = arr[ 2]/rhs.arr[ 2]; result.arr[ 3] = arr[ 3]/rhs.arr[ 3];
	result.arr[ 4] = arr[ 4]/rhs.arr[ 4]; result.arr[ 5] = arr[ 5]/rhs.arr[ 5]; result.arr[ 6] = arr[ 6]/rhs.arr[ 6]; result.arr[ 7] = arr[ 7]/rhs.arr[ 7];
	result.arr[ 8] = arr[ 8]/rhs.arr[ 8]; result.arr[ 9] = arr[ 9]/rhs.arr[ 9]; result.arr[10] = arr[10]/rhs.arr[10]; result.arr[11] = arr[11]/rhs.arr[11];
	result.arr[12] = arr[12]/rhs.arr[12]; result.arr[13] = arr[13]/rhs.arr[13]; result.arr[14] = arr[14]/rhs.arr[14]; result.arr[15] = arr[15]/rhs.arr[15];
#endif
	return result;
} 

//element-wise division and assignment
inline void mat4::
operator%=(const mat4& rhs){
#if DESHI_USE_SSE
	sse_row0 = _mm_div_ps(sse_row0, rhs.sse_row0);
	sse_row1 = _mm_div_ps(sse_row1, rhs.sse_row1);
	sse_row2 = _mm_div_ps(sse_row2, rhs.sse_row2);
	sse_row3 = _mm_div_ps(sse_row3, rhs.sse_row3);
#else
	arr[ 0] /= rhs.arr[ 0]; arr[ 1] /= rhs.arr[ 1]; arr[ 2] /= rhs.arr[ 2]; arr[ 3] /= rhs.arr[ 3];
	arr[ 4] /= rhs.arr[ 4]; arr[ 5] /= rhs.arr[ 5]; arr[ 6] /= rhs.arr[ 6]; arr[ 7] /= rhs.arr[ 7];
	arr[ 8] /= rhs.arr[ 8]; arr[ 9] /= rhs.arr[ 9]; arr[10] /= rhs.arr[10]; arr[11] /= rhs.arr[11];
	arr[12] /= rhs.arr[12]; arr[13] /= rhs.arr[13]; arr[14] /= rhs.arr[14]; arr[15] /= rhs.arr[15];
#endif
}

inline mat4 mat4::
operator* (const mat4& rhs) const{
	mat4 result;
#if DESHI_USE_SSE
	result.sse_row0 = LinearCombineSSE(sse_row0, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row1 = LinearCombineSSE(sse_row1, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row2 = LinearCombineSSE(sse_row2, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row3 = LinearCombineSSE(sse_row3, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
#else
	//TODO(delle,OpMa) look into optimizing this by transposing to remove a loop, see Unreal Matrix.h
	for(s32 i = 0; i < 4; ++i){ //i=m
		for(s32 j = 0; j < 4; ++j){ //j=p
			for(s32 k = 0; k < 4; ++k){ //k=n
				result.arr[4 * i + j] += this->arr[4 * i + k] * rhs.arr[4 * k + j];
			}
		}
	}
#endif
	return result;
}

inline void mat4::
operator*=(const mat4& rhs){
	mat4 result;
#if DESHI_USE_SSE
	result.sse_row0 = LinearCombineSSE(sse_row0, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row1 = LinearCombineSSE(sse_row1, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row2 = LinearCombineSSE(sse_row2, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
	result.sse_row3 = LinearCombineSSE(sse_row3, rhs.sse_row0, rhs.sse_row1, rhs.sse_row2, rhs.sse_row3);
#else
	//TODO(delle,OpMa) look u32o optimizing this by transposing to remove a loop, see Unreal Matrix.h
	for(s32 i = 0; i < 4; ++i){ //i=m
		for(s32 j = 0; j < 4; ++j){ //j=p
			for(s32 k = 0; k < 4; ++k){ //k=n
				result.arr[4 * i + j] += this->arr[4 * i + k] * rhs.arr[4 * k + j];
			}
		}
	}
#endif
	*this = result;
}

inline b32 mat4::
operator==(const mat4& rhs) const{ 
#if DESHI_USE_SSE
	if(!EpsilonEqualSSE(sse_row0, rhs.sse_row0)) return false;
	if(!EpsilonEqualSSE(sse_row1, rhs.sse_row1)) return false;
	if(!EpsilonEqualSSE(sse_row2, rhs.sse_row2)) return false;
	if(!EpsilonEqualSSE(sse_row3, rhs.sse_row3)) return false;
#else
	for(s32 i = 0; i < 16; ++i){ 
		if(abs(this->arr[i] - rhs.arr[i]) > M_EPSILON) return false; 
	}
#endif
	return true;
}

inline b32 mat4::
operator!=(const mat4& rhs) const{ 
	return !(*this == rhs); 
}

///////////////////
//// functions ////
///////////////////

//converts the rows into columns and vice-versa
inline mat4 mat4::
Transpose() const{
	mat4 result;
#if DESHI_USE_SSE
	result = *this;
	_MM_TRANSPOSE4_PS(result.sse_row0, result.sse_row1, result.sse_row2, result.sse_row3);
#else
	for(s32 i = 0; i < 16; ++i){
		result.arr[i] = arr[4 * (i%4) + (i/4)];
	}
#endif
	return result;
}

//returns the determinant of the matrix
inline f32 mat4::
Determinant() const{
	return 
		arr[ 0] * (arr[ 5] * (arr[10] * arr[15] - arr[11] * arr[14]) -
				   arr[ 9] * (arr[ 6] * arr[15] - arr[ 7] * arr[14]) + 
				   arr[13] * (arr[ 6] * arr[11] - arr[ 7] * arr[10]))
		-
		arr[ 4] * (arr[ 1] * (arr[10] * arr[15] - arr[11] * arr[14]) -
				   arr[ 9] * (arr[ 2] * arr[15] - arr[ 3] * arr[14]) +
				   arr[13] * (arr[ 2] * arr[11] - arr[ 3] * arr[10]))
		+
		arr[ 8] * (arr[ 1] * (arr[ 6] * arr[15] - arr[ 7] * arr[14]) -
				   arr[ 5] * (arr[ 2] * arr[15] - arr[ 3] * arr[14]) +
				   arr[13] * (arr[ 2] * arr[ 7] - arr[ 3] * arr[ 6]))
		-
		arr[12] * (arr[ 1] * (arr[ 6] * arr[11] - arr[ 7] * arr[10]) -
				   arr[ 5] * (arr[ 2] * arr[11] - arr[ 3] * arr[10]) +
				   arr[ 9] * (arr[ 2] * arr[ 7] - arr[ 3] * arr[ 6]));
}

//returns the determinant of this matrix without the specified row and column
inline f32 mat4::
Minor(s32 row, s32 col) const{
	f32 arr[9]{ 0 };
	s32 index = 0;
	for(s32 i = 0; i < 4; ++i){
		if(i == row) continue;
		for(s32 j = 0; j < 4; ++j){
			if(j == col) continue;
			arr[index++] = arr[4 * i + j];
		}
	}
	
	//3x3 determinant
	return (arr[0] * arr[4] * arr[8]) 
		+ (arr[1] * arr[5] * arr[6]) 
		+ (arr[2] * arr[3] * arr[7]) 
		- (arr[2] * arr[4] * arr[6]) 
		- (arr[1] * arr[3] * arr[8]) 
		- (arr[0] * arr[5] * arr[7]);
}

//returns the cofactor (minor with adjusted sign based on location in matrix) at given row and column
inline f32 mat4::
Cofactor(s32 row, s32 col) const{
	if((row + col) % 2){
		return -Minor(row, col);
	} else {
		return Minor(row, col);
	}
}

//returns the transposed matrix of cofactors of this matrix
inline mat4 mat4::
Adjoint() const{
	mat4 result;
	s32 index = 0;
	for(s32 i = 0; i < 4; ++i){
		for(s32 j = 0; j < 4; ++j){
			result.arr[index++] = this->Cofactor(i, j);
		}
	}
	return result.Transpose();
}

//returns the adjoint divided by the determinant
inline mat4 mat4::
Inverse() const{
	mat4 result;
#if DESHI_USE_SSE //NOTE probably right-handed matrix multiplication used internally here
	//!ref https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html
	//2x2 sub matrices (ordered internally tl->tr->bl->br)
	__m128 A = _mm_movelh_ps(sse_row0, sse_row1); //top left
	__m128 B = _mm_movehl_ps(sse_row1, sse_row0); //top right
	__m128 C = _mm_movelh_ps(sse_row2, sse_row3); //bot left
	__m128 D = _mm_movehl_ps(sse_row3, sse_row2); //bot right
	
	//calculate determinants (and broadcast across m128)
#  if 1
	__m128 detA = _mm_set1_ps(arr[ 0] * arr[ 5] - arr[ 1] * arr[ 4]);
	__m128 detB = _mm_set1_ps(arr[ 2] * arr[ 7] - arr[ 3] * arr[ 6]);
	__m128 detC = _mm_set1_ps(arr[ 8] * arr[13] - arr[ 9] * arr[12]);
	__m128 detD = _mm_set1_ps(arr[10] * arr[15] - arr[11] * arr[14]);
#  else //NOTE alternate method with using shuffle instead of float set
	__m128 detSub = _mm_sub_ps(__mm_mul_ps(SSEVecShuffle(sse_row0, sse_row2, 0,2,0,2), SSEVecShuffle(sse_row1, sse_row3, 1,3,1,3)), 
							   __mm_mul_ps(SSEVecShuffle(sse_row0, sse_row2, 1,3,1,3), SSEVecShuffle(sse_row1, sse_row3, 0,2,0,2)));
	__m128 detA = SSEVecSwizzle(detSub, 0,0,0,0);
	__m128 detB = SSEVecSwizzle(detSub, 1,1,1,1);
	__m128 detC = SSEVecSwizzle(detSub, 2,2,2,2);
	__m128 detD = SSEVecSwizzle(detSub, 3,3,3,3);
#  endif
	
	//let inverse M = 1/|M| * |X Y|
	//                        |Z W|
	
	//calculate adjugates and determinant //NOTE A# = adjugate A; |A| = determinant A; Atr = trace A
	__m128 D_C  = SSEMat2AdjMul(D, C); //D#*C
	__m128 A_B  = SSEMat2AdjMul(A, B); //A#*B
	__m128 X_   = _mm_sub_ps(_mm_mul_ps(detD, A), SSEMat2Mul(B, D_C)); //|D|*A - B*(D#*C)
	__m128 W_   = _mm_sub_ps(_mm_mul_ps(detA, D), SSEMat2Mul(C, A_B)); //|A|*D - C*(A#*B)
	__m128 detM = _mm_mul_ps(detA, detD); //|A|*|D| ... (to be continued)
	
	__m128 Y_   = _mm_sub_ps(_mm_mul_ps(detB, C), SSEMat2MulAdj(D, A_B)); //|B|*C - D*((A#*B)#)
	__m128 Z_   = _mm_sub_ps(_mm_mul_ps(detC, B), SSEMat2MulAdj(A, D_C)); //|C|*B - A*((D#*C)#)
	detM        = _mm_add_ps(detM, _mm_mul_ps(detB, detC)); //|A|*|D| + |B|*|C| ... (to be continued)
	
	__m128 tr   = _mm_mul_ps(A_B, SSEVecSwizzle(D_C, 0,2,1,3)); //((A#*B)*(D#*C))tr
	tr          = _mm_hadd_ps(tr, tr);
	tr          = _mm_hadd_ps(tr, tr);
	detM        = _mm_sub_ps(detM, tr); //|A|*|D| + |B|*|C| - ((A#*B)*(D#*C))tr
	
	const __m128 adjSignMask = _mm_setr_ps(1.f, -1.f, -1.f, 1.f);
	__m128 rDetM = _mm_div_ps(adjSignMask, detM); //(1/|M|, -1/|M|, -1/|M|, 1/|M|)
	
	X_ = _mm_mul_ps(X_, rDetM);
	Y_ = _mm_mul_ps(Y_, rDetM);
	Z_ = _mm_mul_ps(Z_, rDetM);
	W_ = _mm_mul_ps(W_, rDetM);
	
	result.sse_row0 = SSEVecShuffle(X_, Y_, 3,1,3,1);
	result.sse_row1 = SSEVecShuffle(X_, Y_, 2,0,2,0);
	result.sse_row2 = SSEVecShuffle(Z_, W_, 3,1,3,1);
	result.sse_row3 = SSEVecShuffle(Z_, W_, 2,0,2,0);
	
#  undef VecSwizzle
#  undef VecShuffle
#else
	f32 det = this->Determinant();
	Assert(det, "mat4 inverse does not exist if determinant is zero");
	result = this->Adjoint() / det;
#endif
	return result;
}

//returns a LH rotation transformation matrix in degrees around the X axis
inline mat4 mat4::
RotationMatrixX(f32 angle){
	angle = Radians(angle);
	f32 c = cosf(angle); f32 s = sinf(angle);
	return mat4(1,  0, 0, 0,
				0,  c, s, 0,
				0, -s, c, 0,
				0,  0, 0, 1);
}

//returns a LH rotation transformation matrix in degrees around the Y axis
inline mat4 mat4::
RotationMatrixY(f32 angle){
	angle = Radians(angle);
	f32 c = cosf(angle); f32 s = sinf(angle);
	return mat4(c, 0, -s, 0,
				0, 1,  0, 0,
				s, 0,  c, 0,
				0, 0,  0, 1);
}

//returns a LH rotation transformation matrix in degrees around the Z axis
inline mat4 mat4::
RotationMatrixZ(f32 angle){
	angle = Radians(angle);
	f32 c = cosf(angle); f32 s = sinf(angle);
	return mat4(c,  s, 0, 0,
				-s, c, 0, 0,
				0,  0, 1, 0,
				0,  0, 0, 1);
}

//returns a pre-multiplied X->Y->Z LH rotation transformation matrix based on input in degrees
inline mat4 mat4::
RotationMatrix(f32 x, f32 y, f32 z){
	x = Radians(x); y = Radians(y); z = Radians(z);
	f32 cX = cosf(x); f32 sX = sinf(x);
	f32 cY = cosf(y); f32 sY = sinf(y);
	f32 cZ = cosf(z); f32 sZ = sinf(z);
	f32 r00 = cZ*cY;            f32 r01 = cY*sZ;            f32 r02 = -sY;
	f32 r10 = cZ*sX*sY - cX*sZ; f32 r11 = cZ*cX + sX*sY*sZ; f32 r12 = sX*cY;
	f32 r20 = cZ*cX*sY + sX*sZ; f32 r21 = cX*sY*sZ - cZ*sX; f32 r22 = cX*cY;
	return mat4(r00, r01, r02, 0,
				r10, r11, r12, 0,
				r20, r21, r22, 0,
				0,   0,   0,   1);
}

//returns a translation matrix where (3,0) = translation.x, (3,1) = translation.y, (3,2) = translation.z
inline mat4 mat4::
TranslationMatrix(f32 x, f32 y, f32 z){
	return mat4(1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				x, y, z, 1);
}

//returns a scale matrix where (0,0) = scale.x, (1,1) = scale.y, (2,2) = scale.z
inline mat4 mat4::
ScaleMatrix(f32 x, f32 y, f32 z){
	return mat4(x, 0, 0, 0,
				0, y, 0, 0,
				0, 0, z, 0,
				0, 0, 0, 1);
}

#endif //DESHI_MAT4_INL