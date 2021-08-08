/* //// Notes ////
Matrices can only hold floats
Matrices are in row-major format and all the functionality follows that format
Non-matN vs matN interactions should be in Math.h (or a header file dedicated to those interactions)

//// Creating Matrices ////
Dimensions-Only Constructor:            matN(rows, columns)
You can create a zero-filled matrix by using the constructor and only providing the dimensions.
eg: matN(1,4); This will create a 1x4 matrix filled with zeros

Dimensions and Elements Constructor:    matN(rows, columns, {...})
You can create a matrix of given dimensions with any number of provided elements.
Non-provided elements will be defaulted to zero.                        |1.6, -2.0|
eg: matN(2,2, {1.6f, -2, 1.5});  This will create a 2x2 matrix:    |1.5,  0.0|

Copy Constructor:                        matN(otherMatrix)
You can create a copy of a matrix using this constructor.
The previous matrix will note recieve updates to this matrix.
eg: matN(otherMatrix);

Vector Constructors:                    matN(vector) or matN(vector, w)
You can create a matrix from a vec3 object with an optional fourth element.
eg: matN(vec3(1,2,3)); This will create a 1x3 matrixs: |1.0, 2.0, 3.0|
eg: matN(vec3(1,2,3), 1); This will create a 1x4 matrix: |1.0, 2.0, 3.0, 1.0|

//// Accessing matN Values ////
You can access the values of a matrix using the () operator.
Acessing matrix values starts at zero for both the row and column: 0...n-1 not 1...n
eg: matrix(0,3); This will return the float on the first row and fourth column
eg: matrix(1,1); This will return the float on the second row and second column

Alternatively, you can access the elements directly by their index in a one-dimensional array.
This avoids doing one multiplication and one addition but might be confusing to readers of your code.
eg: matrix.data[3]; This will return the float on the first row and fourth column
eg: matN(4,4).data[10]; This will return the 0 on the third row and third column

//// Visualizing Matrices ////                                                                    2x2 matN:
You can turn matrices into formatted strings using the str() method.                        |+1.000000, -2.000000|
eg: matN(2, 2, {1, -2}).str(); This will return a string formatted like this:        |-0.000000, +0.000000|

Alternatively, you can round the decimals to two places using the str2F() method.        1x1 matN:
Note, zeros can have a + or - in front of them for same size formatting.                |-0.00|
eg: matN(2, 1).str2F(); This will return a string formatted like this:            |+0.00|
eg: matN(3, 4, {1,2,3,4,5,6,7,8,9,10,11,12}).str2F(); This will return a string formatted like the below
3x4 matN:
|+1.00, +2.00, +3.00, +4.00|
|+5.00, +6.00, +7.00, +8.00|
|+9.00, +10.00, +11.00, +12.00|

//// matN Math ////
You can do matrix-scalar multiplication with the * or / operators.
eg: matN(1, 2, {-5, .5f}) * 10; This will return a 1x2 matrix: |-50.0, 5.0|

You can do matrix-matrix addition with the + or - operators.
Note, the matrices must have the same dimensions.
eg: matN(1, 2, {-5, .5f}) - matN(1, 2, {1,1}); This will return a 1x2 matrix: |-6.0, -0.5|

You can do matrix-matrix multiplication with the * operator.
Note, the number of first matrix columns must equal the number of second matrix rows
eg: matN(1, 2, {-5, .5f}) * matN(2, 1, {1,1}); This will return a 1x1 matrix: |-4.5|

You can do matrix-matrix element-wise multiplication with the ^ or % operators.
Element-wise multiplication means multiplying each element with the element in the same location in the other matrix.
Note, the matrices must have the same dimensions. Element-wise division is done with the % operator.
eg: matN(1, 2, {-5, .5f}) ^ matN(1, 2, {2, .5f}); This will return a 1x2 matrix: |-10.0, 0.25|
eg: matN(1, 2, {-5, .5f}) % matN(1, 2, {2, .5f}); This will return a 1x2 matrix: |-2.5, 1.0|

//// Transformation matN ////                                                        |scaleX * rot,    rot,            rot,          translationX|
You can create a transformation matrix by providing the translation, rotation,        |rot,            scaleY * rot,    rot,          translationY|
and scale to the TransformationMatrix() method.                                        |rot,            rot,            scaleZ * rot, translationZ|
The transformation matrix will follow the format to the right:                        |0,                0,                0,              1              |
*/

#pragma once
#ifndef DESHI_matN_H
#define DESHI_matN_H

#include "vector.h"

#include <vector>
#include <string>

struct matN {
    u32 rows = 0;
    u32 cols = 0;
    u32 elementCount = 0;
    std::vector<float> data;
    
    matN() {}
    matN(u32 inRows, u32 inCols);
    matN(u32 inRows, u32 inCols, std::vector<float> list);
    matN(const matN& m);
    
    float&  operator () (u32 row, u32 col);
    float   operator () (u32 row, u32 col) const;
    void    operator =    (const matN& rhs);
    matN operator *  (const float& rhs) const;
    void    operator *= (const float& rhs);
    matN operator /  (const float& rhs) const;
    void    operator /= (const float& rhs);
    matN operator +  (const matN& rhs) const;
    void    operator += (const matN& rhs);
    matN operator -  (const matN& rhs) const;
    void    operator -= (const matN& rhs);
    matN operator *  (const matN& rhs) const;
    void    operator *= (const matN& rhs);
    matN operator ^  (const matN& rhs) const;
    void    operator ^= (const matN& rhs);
    matN operator %  (const matN& rhs) const; 
    void    operator %= (const matN& rhs);
    bool    operator == (const matN& rhs) const;
    bool    operator != (const matN& rhs) const;
    friend matN operator * (const float& lhs, const matN& rhs) { return rhs * lhs; }
    
    const std::string str() const;
    const std::string str2f() const;
    matN Transpose() const;
    matN Submatrix(std::vector<u32> inRows, std::vector<u32> inCols) const;
    float   Minor(int row, int col) const;
    float   Cofactor(int row, int col) const;
    matN Adjoint() const;
    float   Determinant() const;
    matN Inverse() const;
    
    static matN Identity(u32 rows, u32 cols);
    static matN M3x3To4x4(const matN& m);
    static matN RotationMatrix(vec3 rotation, bool _4x4 = true);
    static matN RotationMatrixX(float degrees, bool _4x4 = true);
    static matN RotationMatrixY(float degrees, bool _4x4 = true);
    static matN RotationMatrixZ(float degrees, bool _4x4 = true);
    static matN TranslationMatrix(vec3 translation);
    static matN ScaleMatrix(vec3 scale, bool _4x4 = true);
    static matN TransformationMatrix(vec3 translation, vec3 rotation, vec3 scale);
    
    //Non-matN vs matN interactions
    matN(vec3 v);
    matN(vec3 v, float w);
    
};

//// Constructors ////

inline matN::matN(u32 inRows, u32 inCols) : rows(inRows), cols(inCols) {
    Assert(inRows != 0 && inCols != 0, "matN constructor was given zero size");
    this->elementCount = inRows * inCols;
    this->data = std::vector<float>(elementCount);
}

inline matN::matN(u32 inRows, u32 inCols, std::vector<float> list) : rows(inRows), cols(inCols) {
    Assert(inRows != 0 && inCols != 0, "matN constructor was given zero size");
    this->elementCount = inRows * inCols;
    u32 inCount = list.size();
    Assert(inCount <= elementCount, "matN constructor was given too many elements for given dimensions");
    this->data = std::vector<float>(elementCount);
    for (int i = 0; i < list.size(); ++i) {
        this->data[i] = list[i];
    }
}

inline matN::matN(const matN& m) : rows(m.rows), cols(m.cols), elementCount(m.elementCount) {
    this->data = m.data;
}



//// Operators ////

//element accessor: matrix(row,col)
inline float& matN::operator () (u32 row, u32 col) {
    Assert(row < rows && col < cols, "matN subscript out of bounds");
    return data[(size_t)cols*row + col];
}

inline float  matN::operator () (u32 row, u32 col) const {
    Assert(row < rows && col < cols, "matN subscript out of bounds");
    return data[(size_t)cols * row + col];
}

//deletes current data, copies properties from rhs, creates a new copy of the data from rhs
inline void       matN::operator =  (const matN& rhs) {
    if (&this->data != &rhs.data) {
        this->rows = rhs.rows;
        this->cols = rhs.cols;
        this->elementCount = rhs.elementCount;
        this->data = rhs.data;
    }
}

//scalar multiplication
inline matN  matN::operator *  (const float& rhs) const {
    matN newMatrix(*this);
    for (int i = 0; i < newMatrix.elementCount; ++i) {
        newMatrix.data[i] *= rhs;
    }
    return newMatrix;
}

//scalar multiplication and assignment
inline void    matN::operator *= (const float& rhs) {
    for (int i = 0; i < elementCount; ++i) {
        this->data[i] *= rhs;
    }
}

//scalar division
inline matN  matN::operator /  (const float& rhs) const {
    Assert(rhs != 0, "matN elements cant be divided by zero");
    matN newMatrix(*this);
    for (int i = 0; i < newMatrix.elementCount; ++i) {
        newMatrix.data[i] /= rhs;
    }
    return newMatrix;
}

//scalar division and assignment
inline void    matN::operator /= (const float& rhs){
    Assert(rhs != 0, "matN elements cant be divided by zero");
    for (int i = 0; i < elementCount; ++i) {
        this->data[i] /= rhs;
    }
}

//element-wise addition
inline matN  matN::operator +  (const matN& rhs) const{
    Assert(rows == rhs.rows && cols == rhs.cols, "matN addition requires the same dimensions");
    matN newMatrix(*this);
    for (int i = 0; i < newMatrix.elementCount; ++i) {
        newMatrix.data[i] += rhs.data[i];
    }
    return newMatrix;
}

//element-wise addition and assignment
inline void    matN::operator += (const matN& rhs){
    Assert(rows == rhs.rows && cols == rhs.cols, "matN addition requires the same dimensions");
    for (int i = 0; i < elementCount; ++i) {
        this->data[i] += rhs.data[i];
    }
}

//element-wise substraction
inline matN  matN::operator -  (const matN& rhs) const{
    Assert(rows == rhs.rows && cols == rhs.cols, "matN subtraction requires the same dimensions");
    matN newMatrix(*this);
    for (int i = 0; i < newMatrix.elementCount; ++i) {
        newMatrix.data[i] -= rhs.data[i];
    }
    return newMatrix;
}

//element-wise substraction and assignment
inline void    matN::operator -= (const matN& rhs){
    Assert(rows == rhs.rows && cols == rhs.cols, "matN subtraction requires the same dimensions");
    for (int i = 0; i < elementCount; ++i) {
        this->data[i] -= rhs.data[i];
    }
}

//TODO(delle,Op) look into optimizing this by transposing to remove a loop, see Unreal implementation
inline matN  matN::operator *  (const matN& rhs) const{
    Assert(cols == rhs.rows, "matN multiplication requires the columns of the left matrix to equal the rows of the right matrix");
    matN newMatrix(rows, rhs.cols);
    for (int i = 0; i < this->rows; ++i) { //i=m
        for (int j = 0; j < rhs.cols; ++j) { //j=p
            for (int k = 0; k < rhs.rows; ++k) { //k=n
                newMatrix.data[(size_t)rhs.cols * i + j] += this->data[(size_t)this->cols * i + k] * rhs.data[(size_t)rhs.cols * k + j];
            }
        }
    }
    return newMatrix;
}

inline void    matN::operator *= (const matN& rhs){
    Assert(cols == rhs.rows, "matN multiplication requires the columns of the left matrix to equal the rows of the right matrix");
    matN newMatrix(rows, rhs.cols);
    for (int i = 0; i < this->rows; ++i) { //i=m
        for (int j = 0; j < rhs.cols; ++j) { //j=p
            for (int k = 0; k < rhs.rows; ++k) { //k=n
                newMatrix.data[(size_t)rhs.cols * i + j] += this->data[(size_t)this->cols * i + k] * rhs.data[(size_t)rhs.cols * k + j];
            }
        }
    }
    *this = newMatrix;
}

//element-wise multiplication
inline matN  matN::operator ^  (const matN& rhs) const{
    Assert(rows == rhs.rows && cols == rhs.cols, "matN element-wise multiplication requires the same dimensions");
    matN newMatrix(*this);
    for (int i = 0; i < newMatrix.elementCount; ++i) {
        newMatrix.data[i] *= rhs.data[i];
    }
    return newMatrix;
} 

//element-wise multiplication and assignment
inline void    matN::operator ^= (const matN& rhs){
    Assert(rows == rhs.rows && cols == rhs.cols, "matN element-wise multiplication requires the same dimensions");
    for (int i = 0; i < elementCount; ++i) {
        this->data[i] *= rhs.data[i];
    }
}

//element-wise division
inline matN  matN::operator %  (const matN& rhs) const{
    Assert(rows == rhs.rows && cols == rhs.cols, "matN element-wise division requires the same dimensions");
    matN newMatrix(*this);
    for (int i = 0; i < newMatrix.elementCount; ++i) {
        Assert(rhs.data[i] != 0, "matN element-wise division doesnt allow zeros in the right matrix");
        newMatrix.data[i] /= rhs.data[i];
    }
    return newMatrix;
} 

//element-wise division and assignment
inline void    matN::operator %= (const matN& rhs){
    Assert(rows == rhs.rows && cols == rhs.cols, "matN element-wise division requires the same dimensions");
    for (int i = 0; i < elementCount; ++i) {
        Assert(rhs.data[i] != 0, "matN element-wise division doesnt allow zeros in the right matrix");
        this->data[i] /= rhs.data[i];
    }
}

inline bool       matN::operator    == (const matN& rhs) const { 
    if (this->rows != rhs.rows || this->cols != rhs.cols || this->elementCount != rhs.elementCount) {
        return false;
    }
    for (int i = 0; i < elementCount; ++i) {
        if (this->data[i] != rhs.data[i]) {
            return false;
        }
    }
    return true;
}

inline bool    matN::operator    != (const matN& rhs) const { 
    return !(*this == rhs); 
}



//// Functions ////

//TODO(delle,ClMa) clean up matN.str() and matN.str2F()
inline const std::string matN::str() const {
    if (rows == 0 || cols == 0) {
        return "|Zero dimension matrix|";
    }
    
    std::string str = std::to_string(rows) + "x" + std::to_string(cols) + " matN:\n|";
    if (rows == 1) {
        for (int i = 0; i < cols-1; ++i) {
            char buffer[15];
            std::snprintf(buffer, 15, "%+.6f", data[i]);
            str += std::string(buffer) + ", ";
        }
        char buffer[15];
        std::snprintf(buffer, 15, "%+.6f", data[elementCount - 1]);
        str += std::string(buffer) + "|";
        return str;
    }
    
    for (int i = 0; i < elementCount-1; ++i) {
        char buffer[15];
        std::snprintf(buffer, 15, "%+.6f", data[i]);
        str += std::string(buffer);
        if ((i+1) % cols != 0) {
            str += ", ";
        } else {
            str += "|\n|";
        }
    }
    char buffer[15];
    std::snprintf(buffer, 15, "%+.6f", data[elementCount - 1]);
    str += std::string(buffer) + "|";
    return str;
};

inline const std::string matN::str2f() const {
    if (rows == 0 || cols == 0) {
        return "|Zero dimension matrix|";
    }
    
    std::string str = std::to_string(rows) + "x" + std::to_string(cols) + " matN:\n|";
    if (rows == 1) {
        for (int i = 0; i < cols - 1; ++i) {
            char buffer[15];
            std::snprintf(buffer, 15, "%+.2f", data[i]);
            str += std::string(buffer) + ", ";
        }
        char buffer[15];
        std::snprintf(buffer, 15, "%+.2f", data[elementCount - 1]);
        str += std::string(buffer) + "|";
        return str;
    }
    
    for (int i = 0; i < elementCount - 1; ++i) {
        char buffer[15];
        std::snprintf(buffer, 15, "%+.2f", data[i]);
        str += std::string(buffer);
        if ((i + 1) % cols != 0) {
            str += ", ";
        } else {
            str += "|\n|";
        }
    }
    char buffer[15];
    std::snprintf(buffer, 15, "%+.2f", data[elementCount - 1]);
    str += std::string(buffer) + "|";
    return str;
};

//converts the rows into columns and vice-versa
inline matN matN::Transpose() const{
    matN newMatrix(cols, rows);
    for (int i = 0; i < elementCount; ++i) {
        newMatrix.data[i] = data[(size_t)cols * (i%rows) + (i/rows)];
    }
    return newMatrix;
}

//returns a matrix only with the specified rows and cols
//NOTE 0...n-1 not 1...n
inline matN matN::Submatrix(std::vector<u32> inRows, std::vector<u32> inCols) const{
    Assert(inRows.size() != 0 && inCols.size() > 0, "matN submatrix cant be performed with zero dimensions");
    matN newMatrix(inRows.size(), inCols.size());
    for (int i = 0; i < inRows.size(); ++i) {
        for (int j = 0; j < inCols.size(); ++j) {
            newMatrix.data[(size_t)newMatrix.cols * i + j] = data[(size_t)cols * inRows[i] + inCols[j]];
        }
    }
    return newMatrix;
}

//returns the determinant of this matrix without the specified row and column
inline float matN::Minor(int row, int col) const {
    Assert(rows == cols, "matN minor can only be take of a square matrix");
    Assert(elementCount > 1, "matN minor cant be take of one-dimensional matrix");
    matN newMatrix(rows - 1, cols - 1);
    int index = 0;
    for (int i = 0; i < rows; ++i) {
        if (i == row) continue;
        for (int j = 0; j < cols; ++j) {
            if (j == col) continue;
            newMatrix.data[index++] = data[(size_t)cols * i + j];
        }
    }
    return newMatrix.Determinant();
}

//returns the cofactor (minor with adjusted sign based on location in matrix) at given row and column
inline float matN::Cofactor(int row, int col) const{
    if ((row + col) % 2) {
        return -Minor(row, col);
    } else {
        return Minor(row, col);
    }
}

//returns the determinant of the matrix
inline float matN::Determinant() const{
    Assert(rows == cols, "matN determinant can only be found for square matrices");
    switch (rows) {
        case(1): { //a
            return data[0];
        }
        case(2): { //ad - bc
            return (data[0] * data[3]) - (data[1] * data[2]);
        }
        case(3): { //aei + bfg + cdh - ceg - bdi - afh
            return  (data[0] * data[4] * data[8]) +        //aei
			(data[1] * data[5] * data[6]) +        //bfg
			(data[2] * data[3] * data[7]) -        //cdh
			(data[2] * data[4] * data[6]) -        //ceg
			(data[1] * data[3] * data[8]) -        //bdi
			(data[0] * data[5] * data[7]);        //afh
        }
        case(4): { //not writing this out in letters
            return  data[ 0] * (data[ 5] * (data[10] * data[15] - data[11] * data[14]) -
                                data[ 9] * (data[ 6] * data[15] - data[ 7] * data[14]) + 
                                data[13] * (data[ 6] * data[11] - data[ 7] * data[10]))
                -
                data[ 4] * (data[ 1] * (data[10] * data[15] - data[11] * data[14]) -
                            data[ 9] * (data[ 2] * data[15] - data[ 3] * data[14]) +
                            data[13] * (data[ 2] * data[11] - data[ 3] * data[10]))
                +
                data[ 8] * (data[ 1] * (data[ 6] * data[15] - data[ 7] * data[14]) -
                            data[ 5] * (data[ 2] * data[15] - data[ 3] * data[14]) +
                            data[13] * (data[ 2] * data[ 7] - data[ 3] * data[ 6]))
                -
                data[12] * (data[ 1] * (data[ 6] * data[11] - data[ 7] * data[10]) -
                            data[ 5] * (data[ 2] * data[11] - data[ 3] * data[10]) +
                            data[ 9] * (data[ 2] * data[ 7] - data[ 3] * data[ 6]));
        }
        default: {
            float result = 0;
            for (int i = 0; i < cols; ++i) {
                result += data[i] * this->Cofactor(0, i);
            }
            return result;
        }
    }
}

//returns the transposed matrix of cofactors of this matrix
inline matN matN::Adjoint() const {
    Assert(rows == cols, "matN adjoint can only be found for square matrices");
    matN newMatrix(rows, cols);
    int index = 0;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            newMatrix.data[index++] = this->Cofactor(i, j);
        }
    }
    return newMatrix.Transpose();
}

//returns the adjoint divided by the determinant
inline matN matN::Inverse() const {
    float determinant = this->Determinant();
    Assert(determinant, "matN inverse does not exist if determinant is zero");
    if (elementCount > 1) {
        return this->Adjoint() / determinant;
    }
    return matN(1, 1, {1.f / determinant});
}

//returns an identity matrix with the given dimensions
inline matN matN::Identity(u32 rows, u32 cols) {
    matN newMatrix(rows, cols);
    int index = 0;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (i == j) {
                newMatrix.data[index++] = 1;
            } else {
                newMatrix.data[index++] = 0;
            }
        }
    }
    return newMatrix;
}

//returns a 4x4 matrix with the last element 1 from the provided 3x3 matrix
inline matN matN::M3x3To4x4(const matN& m) {
    Assert(m.rows == 3 && m.cols == 3, "Cant convert 3x3 matrix to 4x4 if the matrix isnt 3x3");
    return matN(4, 4,{m(0,0), m(0,1), m(0,2), 0,
					m(1,0), m(1,1), m(1,2), 0,
					m(2,0), m(2,1), m(2,2), 0,
					0,      0,      0, 1});
}

//returns a 4x4 or 3x3 rotation transformation matrix depending on boolean argument
//the input rotation is in degrees
inline matN matN::RotationMatrix(vec3 rotation, bool _4x4) {
    rotation = RADIANS(rotation);
    float cX = cosf(rotation.x); float sX = sinf(rotation.x);
    float cY = cosf(rotation.y); float sY = sinf(rotation.y);
    float cZ = cosf(rotation.z); float sZ = sinf(rotation.z);
    float r00 = cZ*cY;            float r01 = cY*sZ;            float r02 = -sY;
    float r10 = cZ*sX*sY - cX*sZ; float r11 = cZ*cX + sX*sY*sZ; float r12 = sX*cY;
    float r20 = cZ*cX*sY + sX*sZ; float r21 = cX*sY*sZ - cZ*sX; float r22 = cX*cY;
    if (_4x4) {
        return matN(4, 4, {r00, r01, r02, 0,
						r10, r11, r12, 0,
						r20, r21, r22, 0,
						0,   0,   0,   1});
    } else {
        return matN(3, 3, {r00, r01, r02,
						r10, r11, r12,
						r20, r21, r22});
    }
}

//returns a 4x4 or 3x3 rotation transformation matrix depending on boolean argument
//the input rotation is in degrees
inline matN matN::RotationMatrixX(float degrees, bool _4x4) {
    float r = degrees * (3.14159265359f / 180.f);
    float c = cosf(r);  float s = sinf(r);
    matN newMatrix(3, 3, {1,  0, 0,
					   0,  c, s,
					   0, -s, c});
    if (_4x4) {
        return matN::M3x3To4x4(newMatrix);
    } else {
        return newMatrix;
    }
}

//returns a 4x4 or 3x3 rotation transformation matrix depending on boolean argument
//the input rotation is in degrees
inline matN matN::RotationMatrixY(float degrees, bool _4x4) {
    float r = degrees * (3.14159265359f / 180.f);
    float c = cosf(r); float s = sinf(r);
    matN newMatrix(3, 3, {c, 0, -s,
					   0, 1,  0,
					   s, 0,  c});
    if (_4x4) {
        return matN::M3x3To4x4(newMatrix);
    } else {
        return newMatrix;
    }
}

//returns a 4x4 or 3x3 rotation transformation matrix depending on boolean argument
//the input rotation is in degrees
inline matN matN::RotationMatrixZ(float degrees, bool _4x4) {
    float r = degrees * (3.14159265359f / 180.f);
    float c = cosf(r); float s = sinf(r);
    matN newMatrix(3, 3, { c, s, 0,
					   -s, c, 0,
					   0, 0, 1});
    if (_4x4) {
        return matN::M3x3To4x4(newMatrix);
    } else {
        return newMatrix;
    }
}

//returns a 4x4 translation transformation matrix
inline matN matN::TranslationMatrix(vec3 translation) {
    matN newMatrix = Identity(4,4);
    newMatrix(0,3) = translation.x;
    newMatrix(1,3) = translation.y;
    newMatrix(2,3) = translation.z;
    return newMatrix;
}

inline matN matN::ScaleMatrix(vec3 scale, bool _4x4) {
    matN newMatrix = Identity(3,3);
    newMatrix(0,0) = scale.x;
    newMatrix(1,1) = scale.y;
    newMatrix(2,2) = scale.z;
    if(_4x4) {
        return matN::M3x3To4x4(newMatrix);
    } else {
        return newMatrix;
    }
}

inline matN matN::TransformationMatrix(vec3 tr, vec3 rot, vec3 scale) {
    rot = RADIANS(rot);
    float cX = cosf(rot.x); float sX = sinf(rot.x);
    float cY = cosf(rot.y); float sY = sinf(rot.y);
    float cZ = cosf(rot.z); float sZ = sinf(rot.z);
    float r00 = cZ*cY;            float r01 = cY*sZ;            float r02 = -sY;
    float r10 = cZ*sX*sY - cX*sZ; float r11 = cZ*cX + sX*sY*sZ; float r12 = sX*cY;
    float r20 = cZ*cX*sY + sX*sZ; float r21 = cX*sY*sZ - cZ*sX; float r22 = cX*cY;
    return matN(4, 4, {scale.x*r00, scale.x*r01, scale.x*r02, 0,
					scale.y*r10, scale.y*r11, scale.y*r12, 0,
					scale.z*r20, scale.z*r21, scale.z*r22, 0,
					tr.x,        tr.y,        tr.z, 1});
}

//// Non-Vector vs Vector Interactions ////

inline matN vec3::ToM1x3() const {
    return matN(1, 3, {x, y, z});
}

inline matN vec3::ToM1x4(float w) const {
    return matN(1, 4, { x, y, z, w });
}

//// Non-matN vs matN Interactions ////

//Creates a 1x3 matrix
inline matN::matN(vec3 v) {
    this->rows = 1; this->cols = 3; this->elementCount = 3;
    this->data = {v.x, v.y, v.z};
}

//Creates a 1x4 matrix
inline matN::matN(vec3 v, float w) {
    this->rows = 1; this->cols = 4; this->elementCount = 4;
    this->data = {v.x, v.y, v.z, w};
}

#endif //DESHI_matN_H