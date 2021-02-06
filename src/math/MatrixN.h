#pragma once
#include "../utils/dsh_Debug.h"

struct Vector3;

/*
//// Notes ////
Matrices can only hold floats
Matrices are in row-major format and all the functionality follows that format
Non-MatrixN vs MatrixN interactions should be in Math.h (or a header file dedicated to those interactions)

//// Creating Matrices ////
Dimensions-Only Constructor:			MatrixN(rows, columns)
You can create a zero-filled matrix by using the constructor and only providing the dimensions.
eg: MatrixN(1,4); This will create a 1x4 matrix filled with zeros

Dimensions and Elements Constructor:	MatrixN(rows, columns, {...})
You can create a matrix of given dimensions with any number of provided elements.
Non-provided elements will be defaulted to zero.						|1.6, -2.0|
eg: MatrixN(2,2, {1.6f, -2, 1.5});  This will create a 2x2 matrix:	|1.5,  0.0|

Copy Constructor:						MatrixN(otherMatrix)
You can create a copy of a matrix using this constructor.
The previous matrix will note recieve updates to this matrix.
eg: MatrixN(otherMatrix);

Vector Constructors:					MatrixN(vector) or MatrixN(vector, w)
You can create a matrix from a Vector3 object with an optional fourth element.
eg: MatrixN(Vector3(1,2,3)); This will create a 1x3 matrixs: |1.0, 2.0, 3.0|
eg: MatrixN(Vector3(1,2,3), 1); This will create a 1x4 matrix: |1.0, 2.0, 3.0, 1.0|

//// Accessing MatrixN Values ////
You can access the values of a matrix using the () operator.
Acessing matrix values starts at zero for both the row and column: 0...n-1 not 1...n
eg: matrix(0,3); This will return the float on the first row and fourth column
eg: matrix(1,1); This will return the float on the second row and second column

Alternatively, you can access the elements directly by their index in a one-dimensional array.
This avoids doing one multiplication and one addition but might be confusing to readers of your code.
eg: matrix.data[3]; This will return the float on the first row and fourth column
eg: MatrixN(4,4).data[10]; This will return the 0 on the third row and third column

//// Visualizing Matrices ////																	2x2 MatrixN:
You can turn matrices into formatted strings using the str() method.						|+1.000000, -2.000000|
eg: MatrixN(2, 2, {1, -2}).str(); This will return a string formatted like this:		|-0.000000, +0.000000|

Alternatively, you can round the decimals to two places using the str2F() method.		1x1 MatrixN:
Note, zeros can have a + or - in front of them for same size formatting.				|-0.00|
eg: MatrixN(2, 1).str2F(); This will return a string formatted like this:			|+0.00|
eg: MatrixN(3, 4, {1,2,3,4,5,6,7,8,9,10,11,12}).str2F(); This will return a string formatted like the below
3x4 MatrixN:
|+1.00, +2.00, +3.00, +4.00|
|+5.00, +6.00, +7.00, +8.00|
|+9.00, +10.00, +11.00, +12.00|

//// MatrixN Math ////
You can do matrix-scalar multiplication with the * or / operators.
eg: MatrixN(1, 2, {-5, .5f}) * 10; This will return a 1x2 matrix: |-50.0, 5.0|

You can do matrix-matrix addition with the + or - operators.
Note, the matrices must have the same dimensions.
eg: MatrixN(1, 2, {-5, .5f}) - MatrixN(1, 2, {1,1}); This will return a 1x2 matrix: |-6.0, -0.5|

You can do matrix-matrix multiplication with the * operator.
Note, the number of first matrix columns must equal the number of second matrix rows
eg: MatrixN(1, 2, {-5, .5f}) * MatrixN(2, 1, {1,1}); This will return a 1x1 matrix: |-4.5|

You can do matrix-matrix element-wise multiplication with the ^ or % operators.
Element-wise multiplication means multiplying each element with the element in the same location in the other matrix.
Note, the matrices must have the same dimensions. Element-wise division is done with the % operator.
eg: MatrixN(1, 2, {-5, .5f}) ^ MatrixN(1, 2, {2, .5f}); This will return a 1x2 matrix: |-10.0, 0.25|
eg: MatrixN(1, 2, {-5, .5f}) % MatrixN(1, 2, {2, .5f}); This will return a 1x2 matrix: |-2.5, 1.0|

//// Transformation MatrixN ////														|scaleX * rot,	rot,			rot,		  translationX|
You can create a transformation matrix by providing the translation, rotation,		|rot,			scaleY * rot,	rot,		  translationY|
and scale to the TransformationMatrix() method.										|rot,			rot,			scaleZ * rot, translationZ|
The transformation matrix will follow the format to the right:						|0,				0,				0,			  1			  |

*/

struct MatrixN {
	uint32 rows = 0;
	uint32 cols = 0;
	uint32 elementCount = 0;
	std::vector<float> data;

	MatrixN() {}
	MatrixN(uint32 inRows, uint32 inCols);
	MatrixN(uint32 inRows, uint32 inCols, std::vector<float> list);
	MatrixN(const MatrixN& m);

	float&	operator () (uint32 row, uint32 col);
	float   operator () (uint32 row, uint32 col) const;
	void	operator =	(const MatrixN& rhs);
	MatrixN operator *  (const float& rhs) const;
	void	operator *= (const float& rhs);
	MatrixN operator /  (const float& rhs) const;
	void	operator /= (const float& rhs);
	MatrixN operator +  (const MatrixN& rhs) const;
	void	operator += (const MatrixN& rhs);
	MatrixN operator -  (const MatrixN& rhs) const;
	void	operator -= (const MatrixN& rhs);
	MatrixN operator *  (const MatrixN& rhs) const;
	void	operator *= (const MatrixN& rhs);
	MatrixN operator ^  (const MatrixN& rhs) const;
	void	operator ^= (const MatrixN& rhs);
	MatrixN operator %  (const MatrixN& rhs) const; 
	void	operator %= (const MatrixN& rhs);
	bool	operator == (const MatrixN& rhs) const;
	bool	operator != (const MatrixN& rhs) const;
	friend MatrixN operator * (const float& lhs, const MatrixN& rhs) { return rhs * lhs; }

	const std::string str() const;
	const std::string str2f() const;
	MatrixN Transpose() const;
	MatrixN Submatrix(std::vector<uint32> inRows, std::vector<uint32> inCols) const;
	float Minor(int row, int col) const;
	float Cofactor(int row, int col) const;
	MatrixN Adjoint() const;
	float Determinant() const;
	MatrixN Inverse() const;

	static MatrixN Identity(uint32 rows, uint32 cols);
	static MatrixN M3x3To4x4(const MatrixN& m);
	static MatrixN RotationMatrix(Vector3 rotation, bool _4x4 = true);
	static MatrixN RotationMatrixX(float degrees, bool _4x4 = true);
	static MatrixN RotationMatrixY(float degrees, bool _4x4 = true);
	static MatrixN RotationMatrixZ(float degrees, bool _4x4 = true);
	static MatrixN TranslationMatrix(Vector3 translation);
	static MatrixN ScaleMatrix(Vector3 scale, bool _4x4 = true);
	static MatrixN TransformationMatrix(Vector3 translation, Vector3 rotation, Vector3 scale);

	//Non-MatrixN vs MatrixN interactions
	MatrixN(Vector3 v);
	MatrixN(Vector3 v, float w);

};



//// Constructors ////

inline MatrixN::MatrixN(uint32 inRows, uint32 inCols) : rows(inRows), cols(inCols) {
	ASSERT(inRows != 0 && inCols != 0, "MatrixN constructor was given zero size");
	this->elementCount = inRows * inCols;
	this->data = std::vector<float>(elementCount);
}

inline MatrixN::MatrixN(uint32 inRows, uint32 inCols, std::vector<float> list) : rows(inRows), cols(inCols) {
	ASSERT(inRows != 0 && inCols != 0, "MatrixN constructor was given zero size");
	this->elementCount = inRows * inCols;
	uint32 inCount = list.size();
	ASSERT(inCount <= elementCount, "MatrixN constructor was given too many elements for given dimensions");
	this->data = std::vector<float>(elementCount);
	for (int i = 0; i < list.size(); ++i) {
		this->data[i] = list[i];
	}
}

inline MatrixN::MatrixN(const MatrixN& m) : rows(m.rows), cols(m.cols), elementCount(m.elementCount) {
	this->data = m.data;
}



//// Operators ////

//element accessor: matrix(row,col)
inline float& MatrixN::operator () (uint32 row, uint32 col) {
	ASSERT(row < rows && col < cols, "MatrixN subscript out of bounds");
	return data[(size_t)cols*row + col];
}

inline float  MatrixN::operator () (uint32 row, uint32 col) const {
	ASSERT(row < rows && col < cols, "MatrixN subscript out of bounds");
	return data[(size_t)cols * row + col];
}

//deletes current data, copies properties from rhs, creates a new copy of the data from rhs
inline void	   MatrixN::operator =  (const MatrixN& rhs) {
	if (&this->data != &rhs.data) {
		this->rows = rhs.rows;
		this->cols = rhs.cols;
		this->elementCount = rhs.elementCount;
		this->data = rhs.data;
	}
}

//scalar multiplication
inline MatrixN  MatrixN::operator *  (const float& rhs) const {
	MatrixN newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		newMatrix.data[i] *= rhs;
	}
	return newMatrix;
}

//scalar multiplication and assignment
inline void    MatrixN::operator *= (const float& rhs) {
	for (int i = 0; i < elementCount; ++i) {
		this->data[i] *= rhs;
	}
}

//scalar division
inline MatrixN  MatrixN::operator /  (const float& rhs) const {
	ASSERT(rhs != 0, "MatrixN elements cant be divided by zero");
	MatrixN newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		newMatrix.data[i] /= rhs;
	}
	return newMatrix;
}

//scalar division and assignment
inline void    MatrixN::operator /= (const float& rhs){
	ASSERT(rhs != 0, "MatrixN elements cant be divided by zero");
	for (int i = 0; i < elementCount; ++i) {
		this->data[i] /= rhs;
	}
}

//element-wise addition
inline MatrixN  MatrixN::operator +  (const MatrixN& rhs) const{
	ASSERT(rows == rhs.rows && cols == rhs.cols, "MatrixN addition requires the same dimensions");
	MatrixN newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		newMatrix.data[i] += rhs.data[i];
	}
	return newMatrix;
}

//element-wise addition and assignment
inline void    MatrixN::operator += (const MatrixN& rhs){
	ASSERT(rows == rhs.rows && cols == rhs.cols, "MatrixN addition requires the same dimensions");
	for (int i = 0; i < elementCount; ++i) {
		this->data[i] += rhs.data[i];
	}
}

//element-wise substraction
inline MatrixN  MatrixN::operator -  (const MatrixN& rhs) const{
	ASSERT(rows == rhs.rows && cols == rhs.cols, "MatrixN subtraction requires the same dimensions");
	MatrixN newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		newMatrix.data[i] -= rhs.data[i];
	}
	return newMatrix;
}

//element-wise substraction and assignment
inline void    MatrixN::operator -= (const MatrixN& rhs){
	ASSERT(rows == rhs.rows && cols == rhs.cols, "MatrixN subtraction requires the same dimensions");
	for (int i = 0; i < elementCount; ++i) {
		this->data[i] -= rhs.data[i];
	}
}

//TODO(o,delle) look into optimizing this by transposing to remove a loop, see Unreal implementation
inline MatrixN  MatrixN::operator *  (const MatrixN& rhs) const{
	ASSERT(cols == rhs.rows, "MatrixN multiplication requires the columns of the left matrix to equal the rows of the right matrix");
	MatrixN newMatrix(rows, rhs.cols);
	for (int i = 0; i < this->rows; ++i) { //i=m
		for (int j = 0; j < rhs.cols; ++j) { //j=p
			for (int k = 0; k < rhs.rows; ++k) { //k=n
				newMatrix.data[(size_t)rhs.cols * i + j] += this->data[(size_t)this->cols * i + k] * rhs.data[(size_t)rhs.cols * k + j];
			}
		}
	}
	return newMatrix;
}

inline void    MatrixN::operator *= (const MatrixN& rhs){
	ASSERT(cols == rhs.rows, "MatrixN multiplication requires the columns of the left matrix to equal the rows of the right matrix");
	MatrixN newMatrix(rows, rhs.cols);
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
inline MatrixN  MatrixN::operator ^  (const MatrixN& rhs) const{
	ASSERT(rows == rhs.rows && cols == rhs.cols, "MatrixN element-wise multiplication requires the same dimensions");
	MatrixN newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		newMatrix.data[i] *= rhs.data[i];
	}
	return newMatrix;
} 

//element-wise multiplication and assignment
inline void    MatrixN::operator ^= (const MatrixN& rhs){
	ASSERT(rows == rhs.rows && cols == rhs.cols, "MatrixN element-wise multiplication requires the same dimensions");
	for (int i = 0; i < elementCount; ++i) {
		this->data[i] *= rhs.data[i];
	}
}

//element-wise division
inline MatrixN  MatrixN::operator %  (const MatrixN& rhs) const{
	ASSERT(rows == rhs.rows && cols == rhs.cols, "MatrixN element-wise division requires the same dimensions");
	MatrixN newMatrix(*this);
	for (int i = 0; i < newMatrix.elementCount; ++i) {
		ASSERT(rhs.data[i] != 0, "MatrixN element-wise division doesnt allow zeros in the right matrix");
		newMatrix.data[i] /= rhs.data[i];
	}
	return newMatrix;
} 

//element-wise division and assignment
inline void    MatrixN::operator %= (const MatrixN& rhs){
	ASSERT(rows == rhs.rows && cols == rhs.cols, "MatrixN element-wise division requires the same dimensions");
	for (int i = 0; i < elementCount; ++i) {
		ASSERT(rhs.data[i] != 0, "MatrixN element-wise division doesnt allow zeros in the right matrix");
		this->data[i] /= rhs.data[i];
	}
}

inline bool	   MatrixN::operator	== (const MatrixN& rhs) const { 
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

inline bool    MatrixN::operator	!= (const MatrixN& rhs) const { 
	return !(*this == rhs); 
}



//// Functions ////

//TODO(c,delle) clean up MatrixN.str() and MatrixN.str2F()
inline const std::string MatrixN::str() const {
	if (rows == 0 || cols == 0) {
		return "|Zero dimension matrix|";
	}

	std::string str = std::to_string(rows) + "x" + std::to_string(cols) + " MatrixN:\n|";
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

inline const std::string MatrixN::str2f() const {
	if (rows == 0 || cols == 0) {
		return "|Zero dimension matrix|";
	}

	std::string str = std::to_string(rows) + "x" + std::to_string(cols) + " MatrixN:\n|";
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
inline MatrixN MatrixN::Transpose() const{
	MatrixN newMatrix(cols, rows);
	for (int i = 0; i < elementCount; ++i) {
		newMatrix.data[i] = data[(size_t)cols * (i%rows) + (i/rows)];
	}
	return newMatrix;
}

//returns a matrix only with the specified rows and cols
//NOTE 0...n-1 not 1...n
inline MatrixN MatrixN::Submatrix(std::vector<uint32> inRows, std::vector<uint32> inCols) const{
	ASSERT(inRows.size() != 0 && inCols.size() > 0, "MatrixN submatrix cant be performed with zero dimensions");
	MatrixN newMatrix(inRows.size(), inCols.size());
	for (int i = 0; i < inRows.size(); ++i) {
		for (int j = 0; j < inCols.size(); ++j) {
			newMatrix.data[(size_t)newMatrix.cols * i + j] = data[(size_t)cols * inRows[i] + inCols[j]];
		}
	}
	return newMatrix;
}

//returns the determinant of this matrix without the specified row and column
inline float MatrixN::Minor(int row, int col) const {
	ASSERT(rows == cols, "MatrixN minor can only be take of a square matrix");
	ASSERT(elementCount > 1, "MatrixN minor cant be take of one-dimensional matrix");
	MatrixN newMatrix(rows - 1, cols - 1);
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
inline float MatrixN::Cofactor(int row, int col) const{
	if ((row + col) % 2) {
		return -Minor(row, col);
	} else {
		return Minor(row, col);
	}
}

//returns the determinant of the matrix
inline float MatrixN::Determinant() const{
	ASSERT(rows == cols, "MatrixN determinant can only be found for square matrices");
	switch (rows) {
		case(1): { //a
			return data[0];
		}
		case(2): { //ad - bc
			return (data[0] * data[3]) - (data[1] * data[2]);
		}
		case(3): { //aei + bfg + cdh - ceg - bdi - afh
			return  (data[0] * data[4] * data[8]) +		//aei
				(data[1] * data[5] * data[6]) +		//bfg
				(data[2] * data[3] * data[7]) -		//cdh
				(data[2] * data[4] * data[6]) -		//ceg
				(data[1] * data[3] * data[8]) -		//bdi
				(data[0] * data[5] * data[7]);		//afh
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
inline MatrixN MatrixN::Adjoint() const {
	ASSERT(rows == cols, "MatrixN adjoint can only be found for square matrices");
	MatrixN newMatrix(rows, cols);
	int index = 0;
	for (int i = 0; i < rows; ++i) {
		for (int j = 0; j < cols; ++j) {
			newMatrix.data[index++] = this->Cofactor(i, j);
		}
	}
	return newMatrix.Transpose();
}

//returns the adjoint divided by the determinant
inline MatrixN MatrixN::Inverse() const {
	float determinant = this->Determinant();
	ASSERT(determinant, "MatrixN inverse does not exist if determinant is zero");
	if (elementCount > 1) {
		return this->Adjoint() / determinant;
	}
	return MatrixN(1, 1, {1.f / determinant});
}

//returns an identity matrix with the given dimensions
inline MatrixN MatrixN::Identity(uint32 rows, uint32 cols) {
	MatrixN newMatrix(rows, cols);
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
inline MatrixN MatrixN::M3x3To4x4(const MatrixN& m) {
	ASSERT(m.rows == 3 && m.cols == 3, "Cant convert 3x3 matrix to 4x4 if the matrix isnt 3x3");
	return MatrixN(4, 4,{m(0,0), m(0,1), m(0,2), 0,
		m(1,0), m(1,1), m(1,2), 0,
		m(2,0), m(2,1), m(2,2), 0,
		0,		0,		0,		1});
}

//returns a 4x4 or 3x3 rotation transformation matrix depending on boolean argument
//the input rotation is in degrees
inline MatrixN MatrixN::RotationMatrix(Vector3 rotation, bool _4x4) {
	float cosX = cosf(rotation.x);
	float sinX = sinf(rotation.x);
	float cosY = cosf(rotation.y);
	float sinY = sinf(rotation.y);
	float cosZ = cosf(rotation.z);
	float sinZ = sinf(rotation.z);
	MatrixN newMatrix(3, 3, {cosY,		sinY*sinZ,					cosZ*sinY,
		sinX*sinY,	cosX*cosZ - cosY*sinX*sinZ,	-cosX*sinZ - cosY*cosZ*sinX,
		-cosX*sinY,	cosZ*sinX + cosX*cosY*sinZ, cosX*cosY*cosZ - sinX*sinZ});
	if (_4x4) {
		return MatrixN::M3x3To4x4(newMatrix);
	} else {
		return newMatrix;
	}
}

//returns a 4x4 or 3x3 rotation transformation matrix depending on boolean argument
//the input rotation is in degrees
inline MatrixN MatrixN::RotationMatrixX(float degrees, bool _4x4) {
	float r = degrees * (3.14159265359f / 180.f);
	float c = cosf(r);
	float s = sinf(r);
	MatrixN newMatrix(3, 3, {
		1,	0,	0,
		0,	c,	-s,
		0,	s,	c
		});
	if (_4x4) {
		return MatrixN::M3x3To4x4(newMatrix);
	} else {
		return newMatrix;
	}
}

//returns a 4x4 or 3x3 rotation transformation matrix depending on boolean argument
//the input rotation is in degrees
inline MatrixN MatrixN::RotationMatrixY(float degrees, bool _4x4) {
	float r = degrees * (3.14159265359f / 180.f);
	float c = cosf(r);
	float s = sinf(r);
	MatrixN newMatrix(3, 3, {
		c,	0,	s,
		0,	1,	0,
		-s,	0,	c
		});
	if (_4x4) {
		return MatrixN::M3x3To4x4(newMatrix);
	} else {
		return newMatrix;
	}
}

//returns a 4x4 or 3x3 rotation transformation matrix depending on boolean argument
//the input rotation is in degrees
inline MatrixN MatrixN::RotationMatrixZ(float degrees, bool _4x4) {
	float r = degrees * (3.14159265359f / 180.f);
	float c = cosf(r);
	float s = sinf(r);
	MatrixN newMatrix(3, 3, {
		c,	-s,	0,
		s,	c,	0,
		0,	0,	1
		});
	if (_4x4) {
		return MatrixN::M3x3To4x4(newMatrix);
	} else {
		return newMatrix;
	}
}

//returns a 4x4 translation transformation matrix depending on boolean argument
inline MatrixN MatrixN::TranslationMatrix(Vector3 translation) {
	MatrixN newMatrix = Identity(4,4);
	newMatrix(0,3) = translation.x;
	newMatrix(1,3) = translation.y;
	newMatrix(2,3) = translation.z;
	return newMatrix;
}

inline MatrixN MatrixN::ScaleMatrix(Vector3 scale, bool _4x4) {
	MatrixN newMatrix = Identity(3,3);
	newMatrix(0,0) = scale.x;
	newMatrix(1,1) = scale.y;
	newMatrix(2,2) = scale.z;
	if(_4x4) {
		return MatrixN::M3x3To4x4(newMatrix);
	} else {
		return newMatrix;
	}
}

inline MatrixN MatrixN::TransformationMatrix(Vector3 translation, Vector3 rotation, Vector3 scale) {
	float cosX = cosf(rotation.x);
	float sinX = sinf(rotation.x);
	float cosY = cosf(rotation.y);
	float sinY = sinf(rotation.y);
	float cosZ = cosf(rotation.z);
	float sinZ = sinf(rotation.z);
	return MatrixN(4, 4,{scale.x*(cosY),	sinY*sinZ,								cosZ*sinY,								translation.x,
		sinX*sinY,		scale.y*(cosX*cosZ - cosY*sinX*sinZ),	-cosX*sinZ - cosY*cosZ*sinX,			translation.y,
		-cosX*sinY,		cosZ*sinX + cosX*cosY*sinZ,				scale.z*(cosX*cosY*cosZ - sinX*sinZ),	translation.z,
		0,				0,										0,										1});
}