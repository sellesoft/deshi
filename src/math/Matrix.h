/* //// Notes ////
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

#include "../utils/debug.h"

struct Vector2;
struct Vector3;
struct Vector4;
struct MatrixN;
struct Matrix3;
struct Matrix4;
struct Quaternion;

//////////////////////
//// declarations ////
//////////////////////

struct Matrix3 {
	float data[9]{};
	
	Matrix3(){};
	Matrix3(float _00, float _01, float _02,
			float _10, float _11, float _12,
			float _20, float _21, float _22);
	Matrix3(const Matrix3& m);
	
	static const Matrix3 IDENTITY;
	
	float&  operator () (u32 row, u32 col);
	float   operator () (u32 row, u32 col) const;
	void	operator =  (const Matrix3& rhs);
	Matrix3 operator *  (const float& rhs) const;
	void	operator *= (const float& rhs);
	Matrix3 operator /  (const float& rhs) const;
	void	operator /= (const float& rhs);
	Matrix3 operator +  (const Matrix3& rhs) const;
	void	operator += (const Matrix3& rhs);
	Matrix3 operator -  (const Matrix3& rhs) const;
	void	operator -= (const Matrix3& rhs);
	Matrix3 operator *  (const Matrix3& rhs) const;
	void	operator *= (const Matrix3& rhs);
	Matrix3 operator ^  (const Matrix3& rhs) const;
	void	operator ^= (const Matrix3& rhs);
	Matrix3 operator %  (const Matrix3& rhs) const; 
	void	operator %= (const Matrix3& rhs);
	bool	operator == (const Matrix3& rhs) const;
	bool	operator != (const Matrix3& rhs) const;
	friend Matrix3 operator * (const float& lhs, const Matrix3& rhs) { return rhs * lhs; }
	
	const std::string str() const;
	const std::string str2f() const;
	Matrix3 Transpose() const;
	float   Determinant() const;
	float   Minor(int row, int col) const;
	float   Cofactor(int row, int col) const;
	Matrix3 Adjoint() const;
	Matrix3 Inverse() const;
	
	static Matrix3 RotationMatrixX(float degrees);
	static Matrix3 RotationMatrixY(float degrees);
	static Matrix3 RotationMatrixZ(float degrees);
	static Matrix3 RotationMatrix(float x, float y, float z);
	
	//matrix interactions
	Matrix3(const Matrix4& m);
	Matrix4 To4x4();
	
	//vector interactions
	Vector3 r(u32 row);
	Vector3 c(u32 col);
	static Matrix3 RotationMatrix(Vector3 rotation);
	static Matrix3 ScaleMatrix(Vector3 scale);
};
typedef Matrix3 mat3;
#include "Matrix3.inl"

struct Matrix4 {
	float data[16]{};
	
	Matrix4(){};
	Matrix4(float all);
	Matrix4(float _00, float _01, float _02, float _03,
			float _10, float _11, float _12, float _13,
			float _20, float _21, float _22, float _23,
			float _30, float _31, float _32, float _33);
	Matrix4(const Matrix4& m);
	Matrix4(float* data);
	
	static const Matrix4 IDENTITY;
	
	float&  operator () (u32 row, u32 col);
	float   operator () (u32 row, u32 col) const;
	void	operator =  (const Matrix4& rhs);
	Matrix4 operator *  (const float& rhs) const;
	void	operator *= (const float& rhs);
	Matrix4 operator /  (const float& rhs) const;
	void	operator /= (const float& rhs);
	Matrix4 operator +  (const Matrix4& rhs) const;
	void	operator += (const Matrix4& rhs);
	Matrix4 operator -  (const Matrix4& rhs) const;
	void	operator -= (const Matrix4& rhs);
	Matrix4 operator *  (const Matrix4& rhs) const;
	void	operator *= (const Matrix4& rhs);
	Matrix4 operator ^  (const Matrix4& rhs) const;
	void	operator ^= (const Matrix4& rhs);
	Matrix4 operator %  (const Matrix4& rhs) const; 
	void	operator %= (const Matrix4& rhs);
	bool	operator == (const Matrix4& rhs) const;
	bool	operator != (const Matrix4& rhs) const;
	friend Matrix4 operator * (const float& lhs, const Matrix4& rhs) { return rhs * lhs; }
	
	Matrix4 Transpose() const;
	float   Determinant() const;
	float   Minor(int row, int col) const;
	float   Cofactor(int row, int col) const;
	Matrix4 Adjoint() const;
	Matrix4 Inverse() const;
	const std::string str() const;
	const std::string str2f() const;
	
	static Matrix4 RotationMatrixX(float degrees);
	static Matrix4 RotationMatrixY(float degrees);
	static Matrix4 RotationMatrixZ(float degrees);
	static Matrix4 RotationMatrix(float x, float y, float z);
	static Matrix4 TranslationMatrix(float x, float y, float z);
	static Matrix4 ScaleMatrix(float x, float y, float z);
	
	//matrix interactions
	Matrix4(const Matrix3& m);
	Matrix3 To3x3();
	
	//vector interactions
	Vector4 r(u32 row);
	Vector4 c(u32 col);
	Vector3 Translation();
	Vector3 Rotation();
	Vector3 Scale();
	static Matrix4 RotationMatrix(Vector3 rotation);
	static Matrix4 AxisAngleRotationMatrix(float angle, Vector4 axis);
	static Matrix4 RotationMatrixAroundPoint(Vector3 pivot, Vector3 rotation);
	static Matrix4 TranslationMatrix(Vector3 translation);
	static Matrix4 ScaleMatrix(Vector3 scale);
	static Matrix4 TransformationMatrix(Vector3 translation, Vector3 rotation, Vector3 scale);
};
typedef Matrix4 mat4;
#include "Matrix4.inl"

//////////////////////
//// interactions ////
//////////////////////

//// Matrix3 ////

inline Matrix3::
Matrix3(const Matrix4& m){
	data[0] = m.data[0]; data[1] = m.data[1]; data[2] = m.data[2];
	data[3] = m.data[4]; data[4] = m.data[5]; data[5] = m.data[6];
	data[6] = m.data[8]; data[7] = m.data[9]; data[8] = m.data[10];
}

inline Matrix4 Matrix3::
To4x4() {
	return Matrix4(data[0], data[1], data[2], 0,
				   data[3], data[4], data[5], 0,
				   data[6], data[7], data[8], 0,
				   0,       0,       0,       1);
}

//// Matrix4 ////

inline Matrix4::
Matrix4(const Matrix3& m){
	data[ 0] = m.data[0]; data[ 1] = m.data[1]; data[ 2] = m.data[2]; data[ 3] = 0;
	data[ 4] = m.data[3]; data[ 5] = m.data[4]; data[ 6] = m.data[5]; data[ 7] = 0;
	data[ 8] = m.data[6]; data[ 9] = m.data[7]; data[10] = m.data[8]; data[11] = 0;
	data[12] = 0;         data[13] = 0;         data[14] = 0;         data[15] = 1;
}

inline Matrix3 Matrix4::
To3x3() {
	return Matrix3(data[0], data[1], data[2],
				   data[4], data[5], data[6],
				   data[8], data[9], data[10]);
}

#endif //DESHI_MATRIX_H
