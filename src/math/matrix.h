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

#include "../defines.h"
#include <string>

struct vec2;
struct vec3;
struct vec4;
struct matN;
struct mat3;
struct mat4;
struct quat;

//////////////////////
//// declarations ////
//////////////////////

static constexpr float MAT_EPSILON = 0.00001f;

struct mat3 {
    float data[9]{};
    
    mat3(){};
    mat3(float _00, float _01, float _02,
		 float _10, float _11, float _12,
		 float _20, float _21, float _22);
    mat3(const mat3& m);
    
    static const mat3 IDENTITY;
    
    float&  operator () (u32 row, u32 col);
    float   operator () (u32 row, u32 col) const;
    void	operator =  (const mat3& rhs);
    mat3    operator *  (const float& rhs) const;
    void	operator *= (const float& rhs);
    mat3    operator /  (const float& rhs) const;
    void	operator /= (const float& rhs);
    mat3    operator +  (const mat3& rhs) const;
    void	operator += (const mat3& rhs);
    mat3    operator -  (const mat3& rhs) const;
    void	operator -= (const mat3& rhs);
    mat3    operator *  (const mat3& rhs) const;
    void	operator *= (const mat3& rhs);
    mat3    operator ^  (const mat3& rhs) const;
    void	operator ^= (const mat3& rhs);
    mat3    operator %  (const mat3& rhs) const; 
    void	operator %= (const mat3& rhs);
    bool	operator == (const mat3& rhs) const;
    bool	operator != (const mat3& rhs) const;
    friend mat3 operator * (const float& lhs, const mat3& rhs) { return rhs * lhs; }
    
    const std::string str() const;
    const std::string str2f() const;
    mat3 Transpose() const;
    float   Determinant() const;
    float   Minor(int row, int col) const;
    float   Cofactor(int row, int col) const;
    mat3 Adjoint() const;
    mat3 Inverse() const;
    
    static mat3 RotationMatrixX(float degrees);
    static mat3 RotationMatrixY(float degrees);
    static mat3 RotationMatrixZ(float degrees);
    static mat3 RotationMatrix(float x, float y, float z);
    
    //matrix interactions
    mat3(const mat4& m);
    mat4 To4x4();
    
    //vector interactions
    vec3 row(u32 row);
    vec3 col(u32 col);
    static mat3 RotationMatrix(vec3 rotation);
    static mat3 ScaleMatrix(vec3 scale);
};
#include "mat3.inl"

struct mat4 {
    float data[16]{};
    
    mat4(){};
    mat4(float all);
    mat4(float _00, float _01, float _02, float _03,
		 float _10, float _11, float _12, float _13,
		 float _20, float _21, float _22, float _23,
		 float _30, float _31, float _32, float _33);
    mat4(const mat4& m);
    mat4(float* data);
	
    static const mat4 IDENTITY;
    
    float&  operator () (u32 row, u32 col);
    float   operator () (u32 row, u32 col) const;
    void	operator =  (const mat4& rhs);
    mat4    operator *  (const float& rhs) const;
    void    operator *= (const float& rhs);
    mat4    operator /  (const float& rhs) const;
    void    operator /= (const float& rhs);
    mat4    operator +  (const mat4& rhs) const;
    void    operator += (const mat4& rhs);
    mat4    operator -  (const mat4& rhs) const;
    void    operator -= (const mat4& rhs);
    mat4    operator *  (const mat4& rhs) const;
    void    operator *= (const mat4& rhs);
    mat4    operator ^  (const mat4& rhs) const;
    void    operator ^= (const mat4& rhs);
    mat4    operator %  (const mat4& rhs) const; 
    void	operator %= (const mat4& rhs);
    bool	operator == (const mat4& rhs) const;
    bool	operator != (const mat4& rhs) const;
    friend mat4 operator * (const float& lhs, const mat4& rhs) { return rhs * lhs; }
    
    mat4 Transpose() const;
    float   Determinant() const;
    float   Minor(int row, int col) const;
    float   Cofactor(int row, int col) const;
    mat4 Adjoint() const;
    mat4 Inverse() const;
    const std::string str() const;
    const std::string str2f() const;
    
    static mat4 RotationMatrixX(float degrees);
    static mat4 RotationMatrixY(float degrees);
    static mat4 RotationMatrixZ(float degrees);
    static mat4 RotationMatrix(float x, float y, float z);
    static mat4 TranslationMatrix(float x, float y, float z);
    static mat4 ScaleMatrix(float x, float y, float z);
    
    //matrix interactions
    mat4(const mat3& m);
    mat3 To3x3();
    
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
    data[0] = m.data[0]; data[1] = m.data[1]; data[2] = m.data[2];
    data[3] = m.data[4]; data[4] = m.data[5]; data[5] = m.data[6];
    data[6] = m.data[8]; data[7] = m.data[9]; data[8] = m.data[10];
}

inline mat4 mat3::
To4x4() {
    return mat4(data[0], data[1], data[2], 0,
				data[3], data[4], data[5], 0,
				data[6], data[7], data[8], 0,
				0,       0,       0,       1);
}

//// mat4 ////

inline mat4::
mat4(const mat3& m){
    data[ 0] = m.data[0]; data[ 1] = m.data[1]; data[ 2] = m.data[2]; data[ 3] = 0;
    data[ 4] = m.data[3]; data[ 5] = m.data[4]; data[ 6] = m.data[5]; data[ 7] = 0;
    data[ 8] = m.data[6]; data[ 9] = m.data[7]; data[10] = m.data[8]; data[11] = 0;
    data[12] = 0;         data[13] = 0;         data[14] = 0;         data[15] = 1;
}

inline mat3 mat4::
To3x3() {
    return mat3(data[0], data[1], data[2],
				data[4], data[5], data[6],
				data[8], data[9], data[10]);
}

#endif //DESHI_MATRIX_H
