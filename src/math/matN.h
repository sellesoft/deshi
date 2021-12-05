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

#include <vector>
#include <string>

template<u32 n, u32 m>
struct matN {
	u32 rows = n;
	u32 cols = m;
	u32 elementCount = n*m;
	f64 data[n * m];
	
	matN() {}
	matN(array<f64> list);
	
	f64&    operator () (u32 row, u32 col);
	f64     operator () (u32 row, u32 col) const;
	void      operator =  (const matN<n,m>& rhs);
	matN<n,m> operator *  (const f64& rhs) const;
	void      operator *= (const f64& rhs);
	matN<n,m> operator /  (const f64& rhs) const;
	void      operator /= (const f64& rhs);
	matN<n,m> operator +  (const matN<n,m>& rhs) const;
	void      operator += (const matN<n,m>& rhs);
	matN<n,m> operator -  (const matN<n,m>& rhs) const;
	void      operator -= (const matN<n,m>& rhs);
	matN<n,m> operator *  (const matN<n,m>& rhs) const;
	void      operator *= (const matN<n,m>& rhs);
	matN<n,m> operator ^  (const matN<n,m>& rhs) const;
	void      operator ^= (const matN<n,m>& rhs);
	matN<n,m> operator %  (const matN<n,m>& rhs) const; 
	void      operator %= (const matN<n,m>& rhs);
	bool      operator == (const matN<n,m>& rhs) const;
	bool      operator != (const matN<n,m>& rhs) const;
	friend matN<n,m> operator * (const f64& lhs, const matN<n,m>& rhs) { return rhs * lhs; }
	
	//special friend operator* overload so we can have compile time matrix mult checking
	template<u32 N>
	friend matN<n, N> operator * (matN<n, m>& lhs, matN<m, N>& rhs) {
		matN<n,N> newMatrix;
		for (int i = 0; i < lhs.rows; ++i) { //i=m
			for (int j = 0; j < rhs.cols; ++j) { //j=p
				for (int k = 0; k < rhs.rows; ++k) { //k=n
					newMatrix.data[rhs.cols * i + j] += lhs.data[lhs.cols * i + k] * rhs.data[rhs.cols * k + j];
				}
			}
		}
		return newMatrix;
	}

	friend matN operator * (matN<0,0>& lhs, matN<n,m>& rhs) {
		Assert(lhs.cols == rhs.rows, "matN multiplication requires the columns of the left matrix to equal the rows of the right matrix");
		matN newMatrix(lhs.rows, rhs.cols);
		for (int i = 0; i < lhs.rows; ++i) { //i=m
			for (int j = 0; j < rhs.cols; ++j) { //j=p
				for (int k = 0; k < rhs.rows; ++k) { //k=n
					newMatrix.data[rhs.cols * i + j] += lhs.data[lhs.cols * i + k] * rhs.data[rhs.cols * k + j];
				}
			}
		}
		return newMatrix;
	}


	const std::string str() const;
	const std::string str2f() const;
	matN<m,n> Transpose() const;
	//TODO reimplement this with templates
	//matN<n,m> Submatrix(array<u32> inRows, array<u32> inCols) const;
	f64     Minor(int row, int col) const;
	f64     Cofactor(int row, int col) const;
	matN<n,m> Adjoint() const;
	f64     Determinant() const;
	matN<n,m> Inverse() const;
	
	static matN<n,m> Identity();
	static matN<4,4> M3x3To4x4(const matN<3,3>& m);
	static matN<4,4> RotationMatrix(vec3 rotation);
	static matN<4,4> RotationMatrixX(f64 degrees);
	static matN<4,4> RotationMatrixY(f64 degrees);
	static matN<4,4> RotationMatrixZ(f64 degrees);
	static matN<4,4> TranslationMatrix(vec3 translation);
	static matN<4,4> ScaleMatrix(vec3 scale);
	static matN<4,4> TransformationMatrix(vec3 translation, vec3 rotation, vec3 scale);
	
	//diag_offset > 0 is above the main diagonal
	static matN<n,m> Diag(f64 val, s32 diag_offset = 0);

	//Non-matN<n,m> vs matN<n,m> interactions
	matN(vec3 v);
	matN(vec3 v, f64 w);
	
};

//// Constructors ////

template<u32 n, u32 m>
matN<n, m>::matN(array<f64> list) {
	Assert(n && m, "can't instantiate matN with no template arguments in list constructor");
	Assert(list.count == elementCount, "given list for matN's size does not equal elementCount");
	memcpy(data, list.data, elementCount * sizeof(f64));
}


//// Operators ////

//element accessor: matrix(row,col)
template<u32 n, u32 m> inline f64& matN<n,m>::
operator () (u32 row, u32 col) {
	Assert(row < rows && col < cols, "matN subscript out of bounds");
	return data[(size_t)cols * row + col];
}

template<u32 n, u32 m> inline f64 matN<n,m>::
operator () (u32 row, u32 col) const {
	Assert(row < rows && col < cols, "matN subscript out of bounds");
	return data[(size_t)cols * row + col];
}

//deletes current data, copies properties from rhs, creates a new copy of the data from rhs
template<u32 n, u32 m> inline void matN<n,m>::
operator = (const matN<n,m>& rhs) {
	if (&this->data != &rhs.data) {
		this->rows = rhs.rows;
		this->cols = rhs.cols;
		this->elementCount = rhs.elementCount;
		this->data = rhs.data;
	}
}

//scalar multiplication
template<u32 n, u32 m> inline matN<n,m> matN<n,m>::
operator *  (const f64& rhs) const {
	matN<n,m> newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		newMatrix.data[i] *= rhs;
	}
	return newMatrix;
}

//scalar multiplication and assignment
template<u32 n, u32 m> inline void matN<n,m>::
operator *= (const f64& rhs) {
	for (int i = 0; i < elementCount; ++i) {
		this->data[i] *= rhs;
	}
}

//scalar division
template<u32 n, u32 m> inline matN<n,m> matN<n,m>::
operator /  (const f64& rhs) const {
	Assert(rhs != 0, "matN elements cant be divided by zero");
	matN<n,m> newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		newMatrix.data[i] /= rhs;
	}
	return newMatrix;
}

//scalar division and assignment
template<u32 n, u32 m> inline void matN<n,m>::
operator /= (const f64& rhs) {
	Assert(rhs != 0, "matN elements cant be divided by zero");
	for (int i = 0; i < elementCount; ++i) {
		this->data[i] /= rhs;
	}
}

//element-wise addition
template<u32 n, u32 m> inline matN<n,m> matN<n,m>::
operator +  (const matN<n,m>& rhs) const {
	Assert(rows == rhs.rows && cols == rhs.cols, "matN addition requires the same dimensions");
	matN<n,m> newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		newMatrix.data[i] += rhs.data[i];
	}
	return newMatrix;
}

//element-wise addition and assignment
template<u32 n, u32 m> inline void matN<n,m>::
operator += (const matN<n,m>& rhs) {
	Assert(rows == rhs.rows && cols == rhs.cols, "matN addition requires the same dimensions");
	for (int i = 0; i < elementCount; ++i) {
		this->data[i] += rhs.data[i];
	}
}

//element-wise substraction
template<u32 n, u32 m> inline matN<n,m> matN<n,m>::
operator -  (const matN<n,m>& rhs) const {
	Assert(rows == rhs.rows && cols == rhs.cols, "matN subtraction requires the same dimensions");
	matN<n,m> newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		newMatrix.data[i] -= rhs.data[i];
	}
	return newMatrix;
}

//element-wise substraction and assignment
template<u32 n, u32 m> inline void    matN<n,m>::
operator -= (const matN<n,m>& rhs) {
	Assert(rows == rhs.rows && cols == rhs.cols, "matN subtraction requires the same dimensions");
	for (int i = 0; i < elementCount; ++i) {
		this->data[i] -= rhs.data[i];
	}
}

//TODO(delle,Op) look into optimizing this by transposing to remove a loop, see Unreal implementation
template<u32 n, u32 m> 
inline matN<n,m> matN<n,m>::
operator * (const matN<n, m>& rhs) const {
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

template<u32 n, u32 m> inline void    matN<n,m>::
operator *= (const matN<n,m>& rhs) {
	Assert(cols == rhs.rows, "matN multiplication requires the columns of the left matrix to equal the rows of the right matrix");
	matN<n,m> newMatrix(rows, rhs.cols);
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
template<u32 n, u32 m> inline matN<n,m>  matN<n,m>::
operator ^ (const matN<n,m>& rhs) const {
	Assert(rows == rhs.rows && cols == rhs.cols, "matN element-wise multiplication requires the same dimensions");
	matN<n,m> newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		newMatrix.data[i] *= rhs.data[i];
	}
	return newMatrix;
}

//element-wise multiplication and assignment
template<u32 n, u32 m> inline void    matN<n,m>::
operator ^= (const matN<n,m>& rhs) {
	Assert(rows == rhs.rows && cols == rhs.cols, "matN element-wise multiplication requires the same dimensions");
	for (int i = 0; i < elementCount; ++i) {
		this->data[i] *= rhs.data[i];
	}
}

//element-wise division
template<u32 n, u32 m> inline matN<n,m>  matN<n,m>::
operator % (const matN<n,m>& rhs) const {
	Assert(rows == rhs.rows && cols == rhs.cols, "matN element-wise division requires the same dimensions");
	matN<n,m> newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		Assert(rhs.data[i] != 0, "matN element-wise division doesnt allow zeros in the right matrix");
		newMatrix.data[i] /= rhs.data[i];
	}
	return newMatrix;
}

//element-wise division and assignment
template<u32 n, u32 m> inline void    matN<n,m>::
operator %= (const matN<n,m>& rhs) {
	Assert(rows == rhs.rows && cols == rhs.cols, "matN element-wise division requires the same dimensions");
	for (int i = 0; i < elementCount; ++i) {
		Assert(rhs.data[i] != 0, "matN element-wise division doesnt allow zeros in the right matrix");
		this->data[i] /= rhs.data[i];
	}
}

template<u32 n, u32 m> inline bool matN<n,m>::
operator == (const matN<n,m>& rhs) const {
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

template<u32 n, u32 m> inline bool    matN<n,m>::
operator != (const matN<n,m>& rhs) const {
	return !(*this == rhs);
}



//// Functions ////

//TODO(delle,ClMa) clean up matN<n,m>.str() and matN<n,m>.str2F(
template<u32 n, u32 m> inline const std::string matN<n,m>::
str() const {
	
};

template<u32 n, u32 m> inline const std::string matN<n,m>::
str2f() const {
	if (rows == 0 || cols == 0) {
		return "|Zero dimension matrix|";
	}

	std::string str = std::to_string(rows) + "x" + std::to_string(cols) + " matN<n,m>:\n|";
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
		}
		else {
			str += "|\n|";
		}
	}
	char buffer[15];
	std::snprintf(buffer, 15, "%+.2f", data[elementCount - 1]);
	str += std::string(buffer) + "|";
	return str;
};

//converts the rows into columns and vice-versa
template<u32 n, u32 m> inline matN<m,n> matN<n,m>::
Transpose() const {
	matN<m,n> newMatrix;
	for (int i = 0; i < elementCount; ++i) {
		newMatrix.data[i] = data[(size_t)cols * (i % rows) + (i / rows)];
	}
	return newMatrix;
}

//returns a matrix only with the specified rows and cols
//NOTE 0...n-1 not 1...n
//template<u32 n, u32 m> inline matN<n,m> //matN<n,m>::
//Submatrix(std::vector<u32> inRows, std::vector<u32> inCols) const {
//	Assert(inRows.size() != 0 && inCols.size() > 0, "matN submatrix cant be performed with zero dimensions");
//	matN<n,m> newMatrix(inRows.size(), inCols.size());
//	for (int i = 0; i < inRows.size(); ++i) {
//		for (int j = 0; j < inCols.size(); ++j) {
//			newMatrix.data[(size_t)newMatrix.cols * i + j] = data[(size_t)cols * inRows[i] + inCols[j]];
//		}
//	}
//	return newMatrix;
//}

//returns the determinant of this matrix without the specified row and column
template<u32 n, u32 m> inline f64 matN<n,m>::
Minor(int row, int col) const {
	Assert(rows == cols, "matN minor can only be take of a square matrix");
	Assert(elementCount > 1, "matN minor cant be take of one-dimensional matrix");
	matN<n,m> newMatrix(rows - 1, cols - 1);
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
template<u32 n, u32 m> inline f64 matN<n, m>::
Cofactor(int row, int col) const {
	if ((row + col) % 2) {
		return -Minor(row, col);
	}
	else {
		return Minor(row, col);
	}
}

//returns the determinant of the matrix
template<u32 n, u32 m> inline f64 matN<n,m>::
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
template<u32 n, u32 m> inline matN<n,m> matN<n,m>::
Adjoint() const {
	Assert(rows == cols, "matN adjoint can only be found for square matrices");
	matN<n,m> newMatrix(rows, cols);
	int index = 0;
	for (int i = 0; i < rows; ++i) {
		for (int j = 0; j < cols; ++j) {
			newMatrix.data[index++] = this->Cofactor(i, j);
		}
	}
	return newMatrix.Transpose();
}

//returns the adjoint divided by the determinant
template<u32 n, u32 m> inline matN<n,m> matN<n,m>::
Inverse() const {
	f64 determinant = this->Determinant();
	Assert(determinant, "matN inverse does not exist if determinant is zero");
	if (elementCount > 1) {
		return this->Adjoint() / determinant;
	}
	return matN<n,m>({ 1.f / determinant });
}

//returns an identity matrix with the given dimensions
template<u32 n, u32 m> inline matN<n,m> matN<n,m>::
Identity() {
	matN<n,m> newMatrix;
	int index = 0;
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < m; ++j) {
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
template<u32 n, u32 m> inline matN<4,4> matN<n,m>::
M3x3To4x4(const matN<3,3>& ma) {
	Assert(ma.rows == 3 && m.acols == 3, "Cant convert 3x3 matrix to 4x4 if the matrix isnt 3x3");
	return matN<4,4>({ma(0,0), ma(0,1), ma(0,2), 0,
					  ma(1,0), ma(1,1), ma(1,2), 0,
					  ma(2,0), ma(2,1), ma(2,2), 0,
					  0,      0,      0,         1 });
}

//returns a 4x4 or 3x3 rotation transformation matrix depending on boolean argument
//the input rotation is in degrees
template<u32 n, u32 m> inline matN<4,4> matN<n,m>::
RotationMatrix(vec3 rotation) {
	rotation = RADIANS(rotation);
	f64 cX = cosf(rotation.x); f64 sX = sinf(rotation.x);
	f64 cY = cosf(rotation.y); f64 sY = sinf(rotation.y);
	f64 cZ = cosf(rotation.z); f64 sZ = sinf(rotation.z);
	f64 r00 = cZ * cY;            f64 r01 = cY * sZ;            f64 r02 = -sY;
	f64 r10 = cZ * sX * sY - cX * sZ; f64 r11 = cZ * cX + sX * sY * sZ; f64 r12 = sX * cY;
	f64 r20 = cZ * cX * sY + sX * sZ; f64 r21 = cX * sY * sZ - cZ * sX; f64 r22 = cX * cY;
	
	return matN<4,4>({ r00, r01, r02, 0,
					   r10, r11, r12, 0,
					   r20, r21, r22, 0,
					   0,   0,   0,   1 });
	
	
}

//returns a 4x4 or 3x3 rotation transformation matrix depending on boolean argument
//the input rotation is in degrees
template<u32 n, u32 m> inline matN<4,4> matN<n,m>::
RotationMatrixX(f64 degrees) {
	f64 r = degrees * (3.14159265359f / 180.f);
	f64 c = cosf(r);  f64 s = sinf(r);
	matN<4,4> newMatrix({ 1,  0, 0,	0,
					      0,  c, s,	0,
					      0, -s, c, 0,
						  0,  0, 0, 1});
}

//returns a 4x4 or 3x3 rotation transformation matrix depending on boolean argument
//the input rotation is in degrees
template<u32 n, u32 m> inline matN<4,4> matN<n,m>::
RotationMatrixY(f64 degrees) {
	f64 r = degrees * (3.14159265359f / 180.f);
	f64 c = cosf(r); f64 s = sinf(r);
	matN<4,4> newMatrix({ c, 0, -s, 0,
					      0, 1,  0, 0,
					      s, 0,  c, 0,
						  0, 0,  0, 1});
}

//returns a 4x4 or 3x3 rotation transformation matrix depending on boolean argument
//the input rotation is in degrees
template<u32 n, u32 m> inline matN<4,4> matN<n,m>::
RotationMatrixZ(f64 degrees) {
	f64 r = degrees * (3.14159265359f / 180.f);
	f64 c = cosf(r); f64 s = sinf(r);
	matN<4,4> newMatrix({ c, s, 0, 0,
					     -s, c, 0, 0,
					      0, 0, 1, 0,
						  0, 0, 0, 1});
}

//returns a 4x4 translation transformation matrix
template<u32 n, u32 m> inline matN<4,4> matN<n,m>::
TranslationMatrix(vec3 translation) {
	matN<4,4> newMatrix = Identity();
	newMatrix(0, 3) = translation.x;
	newMatrix(1, 3) = translation.y;
	newMatrix(2, 3) = translation.z;
	return newMatrix;
}

template<u32 n, u32 m> inline matN<4,4> matN<n,m>::
ScaleMatrix(vec3 scale) {
	matN<4,4> newMatrix = Identity();
	newMatrix(0, 0) = scale.x;
	newMatrix(1, 1) = scale.y;
	newMatrix(2, 2) = scale.z;
}

template<u32 n, u32 m> inline matN<4,4> matN<n,m>::
TransformationMatrix(vec3 tr, vec3 rot, vec3 scale) {
	rot = RADIANS(rot);
	f64 cX = cosf(rot.x); f64 sX = sinf(rot.x);
	f64 cY = cosf(rot.y); f64 sY = sinf(rot.y);
	f64 cZ = cosf(rot.z); f64 sZ = sinf(rot.z);
	f64 r00 = cZ * cY;            f64 r01 = cY * sZ;            f64 r02 = -sY;
	f64 r10 = cZ * sX * sY - cX * sZ; f64 r11 = cZ * cX + sX * sY * sZ; f64 r12 = sX * cY;
	f64 r20 = cZ * cX * sY + sX * sZ; f64 r21 = cX * sY * sZ - cZ * sX; f64 r22 = cX * cY;
	return matN<4,4>({ scale.x * r00, scale.x * r01, scale.x * r02, 0,
					   scale.y * r10, scale.y * r11, scale.y * r12, 0,
					   scale.z * r20, scale.z * r21, scale.z * r22, 0,
					   tr.x,          tr.y,          tr.z,          1 });
}

template<u32 n, u32 m> inline matN<n, m> matN<n, m>::
Diag(f64 val, s32 diag_offset) {
	matN<n, m> newMatrix;
	int index = 0;
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < m; ++j) {
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


//// Non-Vector vs Vector Interactions ////

//template<u32 n, u32 m> inline matN<n,m><rows,cols> 
//vec3::ToM1x3() const {
//	return matN<n,m>(1, 3, { x, y, z });
//}
//
//template<u32 n, u32 m> inline matN<n,m><rows,cols> 
//vec3::ToM1x4(f64 w) const {
//	return matN<n,m>(1, 4, { x, y, z, w });
//}
//
////// Non-matN<n,m> vs matN<n,m> Interactions ////
//
////Creates a 1x3 matrix
//template<u32 n, u32 m> inline matN<n,m><rows,cols>
//<rows, cols>::matN<n,m>(vec3 v) {
//	this->rows = 1; this->cols = 3; this->elementCount = 3;
//	this->data = { v.x, v.y, v.z };
//}
//
////Creates a 1x4 matrix
//template<u32 n, u32 m> inline matN<n,m>
//<rows,cols>::matN<n,m>(vec3 v, f64 w) {
//	this->rows = 1; this->cols = 4; this->elementCount = 4;
//	this->data = {v.x, v.y, v.z, w};
//}

#endif //DESHI_matN_H
