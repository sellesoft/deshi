/* //// Notes ////
Matrices can only hold f64s
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
eg: matrix(0,3); This will return the f64 on the first row and fourth column
eg: matrix(1,1); This will return the f64 on the second row and second column

Alternatively, you can access the elements directly by their index in a one-dimensional array.
This avoids doing one multiplication and one addition but might be confusing to readers of your code.
eg: matrix.data[3]; This will return the f64 on the first row and fourth column
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
#include "../utils/array.h"
#include <cstring> //memcpy

struct matN {
	u32 rows;
	u32 cols;
	u32 elementCount;
	array<f64> data;
	
	matN() { rows = 0; cols = 0; elementCount = 0; }
	matN(u32 _rows, u32 _cols) {
		rows = _rows; cols = _cols;
		elementCount = _rows * _cols;
		forI(elementCount) data.add(0);
	};
	matN(u32 _rows, u32 _cols, array<f64> list) {
		Assert(list.count == _rows * _cols);
		rows = _rows; cols = _cols;
		elementCount = _rows * _cols;
		data = list;
	}
	
	f64& operator () (u32 row, u32 col);
	f64  operator () (u32 row, u32 col) const;
	void operator =  (const matN& rhs);
	matN operator *  (const f64& rhs) const;
	void operator *= (const f64& rhs);
	matN operator /  (const f64& rhs) const;
	void operator /= (const f64& rhs);
	matN operator +  (const f64& rhs) const;
	matN operator +  (const matN& rhs) const;
	void operator += (const matN& rhs);
	matN operator -  () const;
	matN operator -  (const matN& rhs) const;
	void operator -= (const matN& rhs);
	matN operator *  (const matN& rhs) const;
	void operator *= (const matN& rhs);
	matN operator ^  (const matN& rhs) const;
	void operator ^= (const matN& rhs);
	matN operator %  (const matN& rhs) const; 
	void operator %= (const matN& rhs);
	bool operator == (const matN& rhs) const;
	bool operator != (const matN& rhs) const;
	friend matN operator * (const f64& lhs, const matN& rhs) { return rhs * lhs; }
	
	matN Transpose() const;
	matN Submatrix(array<u32> inRows, array<u32> inCols) const;
	f64  Minor(int row, int col) const;
	f64  Cofactor(int row, int col) const;
	matN Adjoint() const;
	f64  Determinant() const;
	matN Inverse() const;
	matN Row(u32 row) const;
	void SetRow(u32 row, const matN& rowmat);
	matN Col(u32 col) const;
	void SetCol(u32 col, const matN& colmat);
	void RowSwap(u32 row1, u32 row2);
	void ColSwap(u32 col1, u32 col2);
	
	
	static matN Identity(u32 rows);
	static matN M3x3To4x4(const matN& m);
	static matN RotationMatrix(vec3 rotation);
	static matN RotationMatrixX(f64 degrees);
	static matN RotationMatrixY(f64 degrees);
	static matN RotationMatrixZ(f64 degrees);
	static matN TranslationMatrix(vec3 translation);
	static matN ScaleMatrix(vec3 scale);
	static matN TransformationMatrix(vec3 translation, vec3 rotation, vec3 scale);
	static matN Diag(u32 rows, f64 val, s32 diag_offset = 0);
	static matN Rand(u32 rows, u32 cols, u32 lower, u32 upper);
	static matN Ones(u32 rows, u32 cols);
	static matN TriL(const matN& m);
	static matN TriU(const matN& m);
	static matN Diag(const matN& m);
	
	//Non-matN vs matN interactions
	matN(vec3 v);
	matN(vec3 v, f64 w);
	
};

//// Constructors ////

//matN::matN(u32 _rows, u32 _cols)
//
//matN::matN(u32 _rows, u32 _cols, array<f64> list)


//// Operators ////

//element accessor: matrix(row,col)
inline f64& matN::
operator () (u32 row, u32 col) {
	Assert(row < rows && col < cols, "matN subscript out of bounds");
	return data[(size_t)cols * row + col];
}

inline f64 matN::
operator () (u32 row, u32 col) const {
	Assert(row < rows && col < cols, "matN subscript out of bounds");
	return data[(size_t)cols * row + col];
}

//deletes current data, copies properties from rhs, creates a new copy of the data from rhs
inline void matN::
operator = (const matN& rhs) {
	if (&this->data != &rhs.data) {
		this->rows = rhs.rows;
		this->cols = rhs.cols;
		this->elementCount = rhs.elementCount;
		this->data = rhs.data;
	}
}

//scalar multiplication
inline matN matN::
operator *  (const f64& rhs) const {
	matN newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		newMatrix.data[i] *= rhs;
	}
	return newMatrix;
}

//scalar multiplication and assignment
inline void matN::
operator *= (const f64& rhs) {
	for (int i = 0; i < elementCount; ++i) {
		this->data[i] *= rhs;
	}
}

//scalar division
inline matN matN::
operator /  (const f64& rhs) const {
	Assert(rhs != 0, "matN elements cant be divided by zero");
	matN newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		newMatrix.data[i] /= rhs;
	}
	return newMatrix;
}

//scalar division and assignment
inline void matN::
operator /= (const f64& rhs) {
	Assert(rhs != 0, "matN elements cant be divided by zero");
	for (int i = 0; i < elementCount; ++i) {
		this->data[i] /= rhs;
	}
}

inline matN matN::
operator + (const f64& rhs) const {
	matN newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		newMatrix.data[i] += rhs;
	}
	return newMatrix;
}

//element-wise addition
inline matN matN::
operator +  (const matN& rhs) const {
	Assert(rows == rhs.rows && cols == rhs.cols, "matN addition requires the same dimensions");
	matN newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		newMatrix.data[i] += rhs.data[i];
	}
	return newMatrix;
}

//element-wise addition and assignment
inline void matN::
operator += (const matN& rhs) {
	Assert(rows == rhs.rows && cols == rhs.cols, "matN addition requires the same dimensions");
	for (int i = 0; i < elementCount; ++i) {
		this->data[i] += rhs.data[i];
	}
}

//element-wise substraction
inline matN matN::
operator -  (const matN& rhs) const {
	Assert(rows == rhs.rows && cols == rhs.cols, "matN subtraction requires the same dimensions");
	matN newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		newMatrix.data[i] -= rhs.data[i];
	}
	return newMatrix;
}

//element-wise negation
inline matN matN::
operator -  () const {
	matN newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		newMatrix.data[i] = data[i] * -1;
	}
	return newMatrix;
}

//element-wise substraction and assignment
inline void matN::
operator -= (const matN& rhs) {
	Assert(rows == rhs.rows && cols == rhs.cols, "matN subtraction requires the same dimensions");
	for (int i = 0; i < elementCount; ++i) {
		this->data[i] -= rhs.data[i];
	}
}

//TODO(delle,Op) look into optimizing this by transposing to remove a loop, see Unreal implementation
inline matN matN::
operator * (const matN& rhs) const {
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

inline void    matN::
operator *= (const matN& rhs) {
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
inline matN  matN::
operator ^ (const matN& rhs) const {
	Assert(rows == rhs.rows && cols == rhs.cols, "matN element-wise multiplication requires the same dimensions");
	matN newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		newMatrix.data[i] *= rhs.data[i];
	}
	return newMatrix;
}

//element-wise multiplication and assignment
inline void    matN::
operator ^= (const matN& rhs) {
	Assert(rows == rhs.rows && cols == rhs.cols, "matN element-wise multiplication requires the same dimensions");
	for (int i = 0; i < elementCount; ++i) {
		this->data[i] *= rhs.data[i];
	}
}

//element-wise division
inline matN  matN::
operator % (const matN& rhs) const {
	Assert(rows == rhs.rows && cols == rhs.cols, "matN element-wise division requires the same dimensions");
	matN newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		Assert(rhs.data[i], "matN element-wise division doesnt allow zeros in the right matrix");
		newMatrix.data[i] /= rhs.data[i];
	}
	return newMatrix;
}

//element-wise division and assignment
inline void    matN::
operator %= (const matN& rhs) {
	Assert(rows == rhs.rows && cols == rhs.cols, "matN element-wise division requires the same dimensions");
	for (int i = 0; i < elementCount; ++i) {
		Assert(rhs.data[i] != 0, "matN element-wise division doesnt allow zeros in the right matrix");
		this->data[i] /= rhs.data[i];
	}
}

inline bool matN::
operator == (const matN& rhs) const {
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

inline bool    matN::
operator != (const matN& rhs) const {
	return !(*this == rhs);
}



//// Functions ////



//converts the rows into columns and vice-versa
inline matN matN::
Transpose() const {
	matN newMatrix(rows, cols);
	for (int i = 0; i < elementCount; ++i) {
		newMatrix.data[i] = data[(size_t)cols * (i % rows) + (i / rows)];
	}
	return newMatrix;
}

//returns a matrix only with the specified rows and cols
//NOTE 0...n-1 not 1...n
inline matN matN::
Submatrix(array<u32> inRows, array<u32> inCols) const {
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
inline f64 matN::
Minor(int row, int col) const {
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
inline f64 matN::


Cofactor(int row, int col) const {
	if ((row + col) % 2) {
		return -Minor(row, col);
	}
	else {
		return Minor(row, col);
	}
}

//returns the determinant of the matrix
inline f64 matN::
Determinant() const {
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
			return  data[0] * (data[5] * (data[10] * data[15] - data[11] * data[14]) -
							   data[9] * (data[6] * data[15] - data[7] * data[14]) +
							   data[13] * (data[6] * data[11] - data[7] * data[10]))
				-
				data[4] * (data[1] * (data[10] * data[15] - data[11] * data[14]) -
						   data[9] * (data[2] * data[15] - data[3] * data[14]) +
						   data[13] * (data[2] * data[11] - data[3] * data[10]))
				+
				data[8] * (data[1] * (data[6] * data[15] - data[7] * data[14]) -
						   data[5] * (data[2] * data[15] - data[3] * data[14]) +
						   data[13] * (data[2] * data[7] - data[3] * data[6]))
				-
				data[12] * (data[1] * (data[6] * data[11] - data[7] * data[10]) -
							data[5] * (data[2] * data[11] - data[3] * data[10]) +
							data[9] * (data[2] * data[7] - data[3] * data[6]));
		}
		default: {
			f64 result = 0;
			for (int i = 0; i < cols; ++i) {
				result += data[i] * this->Cofactor(0, i);
			}
			return result;
		}
	}
}

//returns the transposed matrix of cofactors of this matrix
inline matN matN::
Adjoint() const {
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
inline matN matN::
Inverse() const {
	
	//first check if we just have a diagonal matrix, whose inverse is just I / A
	b32 diag = 1;
	matN nu = matN::Identity(rows);
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			
			if (i != j && this->operator()(i, j)) {
				diag = 0;
				goto diagbreak;
			}
			else if(i==j) {
				Assert(this->operator()(i, j), "matN inverse does not exist if determinant is 0");
				nu(i, j) /= data[cols * i + j];
				
			}
		}
	}
	if (diag) return nu;
	diagbreak:
	f64 determinant = this->Determinant();
	Assert(determinant, "matN inverse does not exist if determinant is zero");
	if (elementCount > 1) {
		return this->Adjoint() / determinant;
	}
	return matN(rows, cols, { 1.f / determinant });
	
}

inline matN matN::
Row(u32 row) const{
	Assert(row < rows, "subscript out of range");
	matN ret(1, cols);
	memcpy(ret.data.data, data.data + (u64)cols * row, sizeof(f64) * cols);
	return ret;
}

inline void matN::
SetRow(u32 row, const matN& rowmat){
	Assert(row < rows, "subscript out of range");
	memcpy(&data[cols * row], rowmat.data.data, sizeof(f64) * cols);
}

inline matN matN::
Col(u32 col) const{
	Assert(col < cols, "subscript out of range");
	matN ret(rows, 1);
	forI(rows)
		memcpy(&ret.data[i], data.data + col * rows + i, sizeof(f64));
	return ret;
}

inline void matN::
SetCol(u32 col, const matN& colmat){
	Assert(col < cols, "subscript out of range");
	forI(rows)
		memcpy(&data[col * rows + i], colmat.data.data + i, sizeof(f64));
}

inline void matN::
RowSwap(u32 row1, u32 row2){
	Assert(row1 < rows && row2 < rows, "subscript out of range");
	matN temp = Row(row1);
	SetRow(row1, Row(row2));
	SetRow(row2, temp);
}

inline void matN::
ColSwap(u32 col1, u32 col2){
	Assert(col1 < cols && col2 < cols, "subscript out of range");
	matN temp = Col(col1);
	SetCol(col1, Col(col2));
	SetCol(col2, temp);
}



//returns an identity matrix with the given dimensions
inline matN matN::
Identity(u32 rows) {
	matN newMatrix(rows, rows);
	int index = 0;
	for (int i = 0; i < rows; ++i) {
		for (int j = 0; j < rows; ++j) {
			if (i == j) {
				newMatrix.data[index++] = 1;
			}
			else {
				newMatrix.data[index++] = 0;
			}
		}
	}
	return newMatrix;
}

//returns a 4x4 matrix with the last element 1 from the provided 3x3 matrix
inline matN matN::
M3x3To4x4(const matN& ma) {
	Assert(ma.rows == 3 && ma.cols == 3, "Cant convert 3x3 matrix to 4x4 if the matrix isnt 3x3");
	return matN(4,4,{ma(0,0), ma(0,1), ma(0,2), 0,
					ma(1,0), ma(1,1), ma(1,2), 0,
					ma(2,0), ma(2,1), ma(2,2), 0,
					0,      0,      0,         1 });
}

//returns a 4x4 or 3x3 rotation transformation matrix depending on boolean argument
//the input rotation is in degrees
inline matN matN::
RotationMatrix(vec3 rotation) {
	rotation = Radians(rotation);
	f64 cX = cosf(rotation.x); f64 sX = sinf(rotation.x);
	f64 cY = cosf(rotation.y); f64 sY = sinf(rotation.y);
	f64 cZ = cosf(rotation.z); f64 sZ = sinf(rotation.z);
	f64 r00 = cZ * cY;            f64 r01 = cY * sZ;            f64 r02 = -sY;
	f64 r10 = cZ * sX * sY - cX * sZ; f64 r11 = cZ * cX + sX * sY * sZ; f64 r12 = sX * cY;
	f64 r20 = cZ * cX * sY + sX * sZ; f64 r21 = cX * sY * sZ - cZ * sX; f64 r22 = cX * cY;
	
	return matN(4,4,{ r00, r01, r02, 0,
					r10, r11, r12, 0,
					r20, r21, r22, 0,
					0,   0,   0,   1 });
	
	
}

//returns a 4x4 or 3x3 rotation transformation matrix depending on boolean argument
//the input rotation is in degrees
inline matN matN::
RotationMatrixX(f64 degrees) {
	f64 r = degrees * (3.14159265359f / 180.f);
	f64 c = cosf(r);  f64 s = sinf(r);
	matN newMatrix(4,4, { 1,  0, 0,	0,
					   0,  c, s,	0,
					   0, -s, c, 0,
					   0,  0, 0, 1});
}

//returns a 4x4 or 3x3 rotation transformation matrix depending on boolean argument
//the input rotation is in degrees
inline matN matN::
RotationMatrixY(f64 degrees) {
	f64 r = degrees * (3.14159265359f / 180.f);
	f64 c = cosf(r); f64 s = sinf(r);
	matN newMatrix(4,4, { c, 0, -s, 0,
					   0, 1,  0, 0,
					   s, 0,  c, 0,
					   0, 0,  0, 1});
}

//returns a 4x4 or 3x3 rotation transformation matrix depending on boolean argument
//the input rotation is in degrees
inline matN matN::
RotationMatrixZ(f64 degrees) {
	f64 r = degrees * (3.14159265359f / 180.f);
	f64 c = cosf(r); f64 s = sinf(r);
	matN newMatrix(4,4,{ c, s, 0, 0,
					   -s, c, 0, 0,
					   0, 0, 1, 0,
					   0, 0, 0, 1});
}

//returns a 4x4 translation transformation matrix
inline matN matN::
TranslationMatrix(vec3 translation) {
	matN newMatrix = Identity(4);
	newMatrix(0, 3) = translation.x;
	newMatrix(1, 3) = translation.y;
	newMatrix(2, 3) = translation.z;
	return newMatrix;
}

inline matN matN::
ScaleMatrix(vec3 scale) {
	matN newMatrix = Identity(4);
	newMatrix(0, 0) = scale.x;
	newMatrix(1, 1) = scale.y;
	newMatrix(2, 2) = scale.z;
}

inline matN matN::
TransformationMatrix(vec3 tr, vec3 rot, vec3 scale) {
	rot = Radians(rot);
	f64 cX = cosf(rot.x); f64 sX = sinf(rot.x);
	f64 cY = cosf(rot.y); f64 sY = sinf(rot.y);
	f64 cZ = cosf(rot.z); f64 sZ = sinf(rot.z);
	f64 r00 = cZ * cY;            f64 r01 = cY * sZ;            f64 r02 = -sY;
	f64 r10 = cZ * sX * sY - cX * sZ; f64 r11 = cZ * cX + sX * sY * sZ; f64 r12 = sX * cY;
	f64 r20 = cZ * cX * sY + sX * sZ; f64 r21 = cX * sY * sZ - cZ * sX; f64 r22 = cX * cY;
	return matN(4,4, { scale.x * r00, scale.x * r01, scale.x * r02, 0,
					scale.y * r10, scale.y * r11, scale.y * r12, 0,
					scale.z * r20, scale.z * r21, scale.z * r22, 0,
					tr.x,          tr.y,          tr.z,          1 });
}

//diag_offset > 0 is above the main diagonal
inline matN matN::
Diag(u32 rows, f64 val, s32 diag_offset) {
	matN newMatrix(rows,rows);
	int index = 0;
	for (int i = 0; i < rows; ++i) {
		for (int j = 0; j < rows; ++j) {
			if (i == j-diag_offset) {
				newMatrix.data[index++] = val;
			}
			else {
				newMatrix.data[index++] = 0;
			}
		}
	}
	return newMatrix;
}

//returns a matrix wth random integer values from lower to upper
inline matN matN::
Rand(u32 rows, u32 cols, u32 lower, u32 upper) {
	matN nu(rows, cols);
	forX(i, rows) {
		forX(j, cols) {
			nu(i, j) = f64((rand() % upper) + (u64)lower);
		}
	}
	return nu;
}

inline matN matN::
Ones(u32 rows, u32 cols) {
	matN nu(rows, cols);
	forI(nu.elementCount) nu.data[i] = 1;
	return nu;
}

inline matN matN::TriL(const matN& m){
	matN nu(m.rows, m.cols);
	forX(i, m.rows) {
		for (int j = 0; j <= i; j++) {
			nu(i, j) = m(i, j);
		}
	}
	return nu;
}

inline matN matN::TriU(const matN& m){
	matN nu(m.rows, m.cols);
	forX(i, m.rows) {
		for (int j = i; j < m.cols; j++) {
			nu(i, j) = m(i, j);
		}
	}
	return nu;
}

inline matN matN::Diag(const matN& m){
	matN nu(m.rows, m.cols);
	forI(m.rows) nu(i, i) = m(i, i);
	return nu;
}



//// Non-Vector vs Vector Interactions ////

//inline matNcols> 
//vec3::ToM1x3() const {
//	return matN(1, 3, { x, y, z });
//}
//
//inline matNcols> 
//vec3::ToM1x4(f64 w) const {
//	return matN(1, 4, { x, y, z, w });
//}
//
////// Non-matN vs matN Interactions ////
//
////Creates a 1x3 matrix
//inline matNcols>
//<rows, cols>::matN(vec3 v) {
//	this->rows = 1; this->cols = 3; this->elementCount = 3;
//	this->data = { v.x, v.y, v.z };
//}
//
////Creates a 1x4 matrix
//inline matN
//<rows,cols>::matN(vec3 v, f64 w) {
//	this->rows = 1; this->cols = 4; this->elementCount = 4;
//	this->data = {v.x, v.y, v.z, w};
//}

#endif //DESHI_matN_H
