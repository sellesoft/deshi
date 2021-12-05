/* 
//// Notes ////
Matrices can only hold floats
Matrices are in row-major format and all the functionality follows that format
Matrices are Left-Handed meaning that multiplication travels right and rotation is clockwise 

//// Accessing Matrix Values ////
You can access the values of a matrix using the () operator.
Acessing matrix values starts at zero for both the row and column: 0...n-1 not 1...n
eg: matrix(0,3); This will return the float on the first row and fourth column
eg: matrix(1,1); This will return the float on the second row and second column

Alternatively, you can access the elements directly by their index in a one-dimensional array
This avoids doing one multiplication and one addition but might be confusing to readers of your code
eg: matrix.data[3]; This will return the float on the first row and fourth column

//// Transformation Matrix ////												
You can create a transformation matrix by providing the translation, rotation,
	and scale to the TransformationMatrix() method.
The transformation matrix will follow the format to the below:
|scaleX * rot, rot,          rot,          0|
|rot,          scaleY * rot, rot,          0|
|rot,          rot,          scaleZ * rot, 0|
|translationX, translationY, translationZ, 1|
*/

#pragma once
#ifndef DESHI_MATRIX_H
#define DESHI_MATRIX_H
#include "math_utils.h"
#include "vector.h"

struct vec2;
struct vec3;
struct vec4;
template<u32 n = 0, u32 m = 0>
struct matN;
struct mat3;
struct mat4;
struct quat;

//////////////////////
//// declarations ////
//////////////////////

struct mat3 {
	union{
		float arr[9] = {};
		struct{
			vec3 row0;
			vec3 row1;
			vec3 row2;
		};
	};
	
	mat3(){};
	mat3(float all);
	mat3(float _00, float _01, float _02,
		 float _10, float _11, float _12,
		 float _20, float _21, float _22);
	mat3(const mat3& m);
	mat3(float* data);
	
	static const mat3 IDENTITY;
	
	float& operator()(u32 row, u32 col);
	float  operator()(u32 row, u32 col) const;
	void   operator= (const mat3& rhs);
	mat3   operator* (const float& rhs) const;
	void   operator*=(const float& rhs);
	mat3   operator/ (const float& rhs) const;
	void   operator/=(const float& rhs);
	mat3   operator+ (const mat3& rhs) const;
	void   operator+=(const mat3& rhs);
	mat3   operator- (const mat3& rhs) const;
	void   operator-=(const mat3& rhs);
	mat3   operator^ (const mat3& rhs) const;
	void   operator^=(const mat3& rhs);
	mat3   operator% (const mat3& rhs) const; 
	void   operator%=(const mat3& rhs);
	mat3   operator* (const mat3& rhs) const;
	void   operator*=(const mat3& rhs);
	bool   operator==(const mat3& rhs) const;
	bool   operator!=(const mat3& rhs) const;
	friend mat3 operator* (const float& lhs, const mat3& rhs){ return rhs * lhs; }
	
	mat3  Transpose() const;
	float Determinant() const;
	float Minor(int row, int col) const;
	float Cofactor(int row, int col) const;
	mat3  Adjoint() const;
	mat3  Inverse() const;
	
	static mat3 RotationMatrixX(float degrees);
	static mat3 RotationMatrixY(float degrees);
	static mat3 RotationMatrixZ(float degrees);
	static mat3 RotationMatrix(float x, float y, float z);
	
	//matrix interactions
	mat3(const mat4& m);
	mat4 To4x4() const;
	
	//vector interactions
	vec3 row(u32 row);
	vec3 col(u32 col);
	static mat3 RotationMatrix(vec3 rotation);
	static mat3 ScaleMatrix(vec3 scale);
};
#include "mat3.inl"

struct mat4 {
	union{
		float arr[16] = {};
		struct{
			vec4 row0;
			vec4 row1;
			vec4 row2;
			vec4 row3;
		};
#if DESHI_USE_SSE
		struct{
			__m128 sse_row0;
			__m128 sse_row1;
			__m128 sse_row2;
			__m128 sse_row3;
		};
#endif
	};
	
	mat4(){};
	mat4(float all);
	mat4(float _00, float _01, float _02, float _03,
		 float _10, float _11, float _12, float _13,
		 float _20, float _21, float _22, float _23,
		 float _30, float _31, float _32, float _33);
	mat4(const mat4& m);
	mat4(float* data);
	
	static const mat4 IDENTITY;
	
	float& operator()(u32 row, u32 col);
	float  operator()(u32 row, u32 col) const;
	void   operator= (const mat4& rhs);
	mat4   operator* (const float& rhs) const;
	void   operator*=(const float& rhs);
	mat4   operator/ (const float& rhs) const;
	void   operator/=(const float& rhs);
	mat4   operator+ (const mat4& rhs) const;
	void   operator+=(const mat4& rhs);
	mat4   operator- (const mat4& rhs) const;
	void   operator-=(const mat4& rhs);
	mat4   operator^ (const mat4& rhs) const;
	void   operator^=(const mat4& rhs);
	mat4   operator% (const mat4& rhs) const; 
	void   operator%=(const mat4& rhs);
	mat4   operator* (const mat4& rhs) const;
	void   operator*=(const mat4& rhs);
	bool   operator==(const mat4& rhs) const;
	bool   operator!=(const mat4& rhs) const;
	friend mat4 operator* (const float& lhs, const mat4& rhs){ return rhs * lhs; }
	
	mat4  Transpose() const;
	float Determinant() const;
	float Minor(int row, int col) const;
	float Cofactor(int row, int col) const;
	mat4  Adjoint() const;
	mat4  Inverse() const;
	
	static mat4 RotationMatrixX(float degrees);
	static mat4 RotationMatrixY(float degrees);
	static mat4 RotationMatrixZ(float degrees);
	static mat4 RotationMatrix(float x, float y, float z);
	static mat4 TranslationMatrix(float x, float y, float z);
	static mat4 ScaleMatrix(float x, float y, float z);
	
	//matrix interactions
	mat4(const mat3& m);
	mat3 To3x3() const;
	
	//vector interactions
	vec4 row(u32 row);
	vec4 col(u32 col);
	vec3 Translation();
	vec3 Rotation();
	vec3 Scale();
	static mat4 RotationMatrix(vec3 rotation);
	static mat4 AxisAngleRotationMatrix(float angle, vec4 axis);
	static mat4 RotationMatrixAroundPoint(vec3 pivot, vec3 rotation);
	static mat4 TranslationMatrix(vec3 translation);
	static mat4 ScaleMatrix(vec3 scale);
	static mat4 TransformationMatrix(vec3 translation, vec3 rotation, vec3 scale);
	
};
#include "mat4.inl"

//////////////////////
//// interactions ////
//////////////////////

//// mat3 ////

inline mat3::
mat3(const mat4& m){
	arr[0] = m.arr[0]; arr[1] = m.arr[1]; arr[2] = m.arr[2];
	arr[3] = m.arr[4]; arr[4] = m.arr[5]; arr[5] = m.arr[6];
	arr[6] = m.arr[8]; arr[7] = m.arr[9]; arr[8] = m.arr[10];
}

inline mat4 mat3::
To4x4() const{
	return mat4(arr[0], arr[1], arr[2], 0,
				arr[3], arr[4], arr[5], 0,
				arr[6], arr[7], arr[8], 0,
				0,       0,       0,    1);
}

//// mat4 ////

inline mat4::
mat4(const mat3& m){
	arr[ 0] = m.arr[0]; arr[ 1] = m.arr[1]; arr[ 2] = m.arr[2]; arr[ 3] = 0;
	arr[ 4] = m.arr[3]; arr[ 5] = m.arr[4]; arr[ 6] = m.arr[5]; arr[ 7] = 0;
	arr[ 8] = m.arr[6]; arr[ 9] = m.arr[7]; arr[10] = m.arr[8]; arr[11] = 0;
	arr[12] = 0;        arr[13] = 0;        arr[14] = 0;        arr[15] = 1;
}

inline mat3 mat4::
To3x3() const{
	return mat3(arr[0], arr[1], arr[2],
				arr[4], arr[5], arr[6],
				arr[8], arr[9], arr[10]);
}

#endif //DESHI_MATRIX_H
