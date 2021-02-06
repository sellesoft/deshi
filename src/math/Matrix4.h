#pragma once
#include "../utils/dsh_Debug.h"

struct Vector3;
struct Matrix3;

struct Matrix4 {
	float data[16]{};

	Matrix4() {}
	Matrix4(float _00, float _01, float _02, float _03,
			float _10, float _11, float _12, float _13,
			float _20, float _21, float _22, float _23,
			float _30, float _31, float _32, float _33);
	Matrix4(const Matrix4& m);

	static const Matrix4 IDENTITY;

	float&	operator () (uint32 row, uint32 col);
	float   operator () (uint32 row, uint32 col) const;
	void	operator =	(const Matrix4& rhs);
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

	const std::string str() const;
	const std::string str2f() const;
	Matrix4 Transpose() const;
	float Determinant() const;
	float Minor(int row, int col) const;
	float Cofactor(int row, int col) const;
	Matrix4 Adjoint() const;
	Matrix4 Inverse() const;

	static Matrix4 RotationMatrixX(float degrees);
	static Matrix4 RotationMatrixY(float degrees);
	static Matrix4 RotationMatrixZ(float degrees);

	//Non-Matrix4 vs Matrix4 interactions defined in Matrix.h/Math.h
	Matrix3 To3x3();
	static Matrix4 RotationMatrix(Vector3 rotation);
	static Matrix4 TranslationMatrix(Vector3 translation);
	static Matrix4 ScaleMatrix(Vector3 scale);
	static Matrix4 TransformationMatrix(Vector3 translation, Vector3 rotation, Vector3 scale);
	static Matrix4 RotationMatrixAroundPoint(Vector3 pivot, Vector3 rotation);
};



//// Operators ////

//element accessor: matrix(row,col)
inline float& Matrix4::operator () (uint32 row, uint32 col) {
	ASSERT(row < 4 && col < 4, "Matrix4 subscript out of bounds");
	return data[4*row + col];
}

inline float Matrix4::operator () (uint32 row, uint32 col) const {
	ASSERT(row < 4 && col < 4, "Matrix4 subscript out of bounds");
	return data[4 * row + col];
}

//creates the data from rhs
inline void	   Matrix4::operator =  (const Matrix4& rhs) {
	memcpy(&data, &rhs.data, 16*sizeof(float));
}

//scalar multiplication
inline Matrix4  Matrix4::operator *  (const float& rhs) const {
	Matrix4 newMatrix(*this);
	for (int i = 0; i < 16; ++i) {
		newMatrix.data[i] *= rhs;
	}
	return newMatrix;
}

//scalar multiplication and assignment
inline void    Matrix4::operator *= (const float& rhs) {
	for (int i = 0; i < 16; ++i) {
		data[i] *= rhs;
	}
}

//scalar division
inline Matrix4  Matrix4::operator /  (const float& rhs) const {
	ASSERT(rhs != 0, "Matrix4 elements cant be divided by zero");
	Matrix4 newMatrix(*this);
	for (int i = 0; i < 16; ++i) {
		newMatrix.data[i] /= rhs;
	}
	return newMatrix;
}

//scalar division and assignment
inline void    Matrix4::operator /= (const float& rhs){
	ASSERT(rhs != 0, "Matrix4 elements cant be divided by zero");
	for (int i = 0; i < 16; ++i) {
		data[i] /= rhs;
	}
}

//element-wise addition
inline Matrix4  Matrix4::operator +  (const Matrix4& rhs) const{
	Matrix4 newMatrix(*this);
	for (int i = 0; i < 16; ++i) {
		newMatrix.data[i] += rhs.data[i];
	}
	return newMatrix;
}

//element-wise addition and assignment
inline void    Matrix4::operator += (const Matrix4& rhs){
	for (int i = 0; i < 16; ++i) {
		this->data[i] += rhs.data[i];
	}
}

//element-wise substraction
inline Matrix4  Matrix4::operator -  (const Matrix4& rhs) const{
	Matrix4 newMatrix(*this);
	for (int i = 0; i < 16; ++i) {
		newMatrix.data[i] -= rhs.data[i];
	}
	return newMatrix;
}

//element-wise substraction and assignment
inline void    Matrix4::operator -= (const Matrix4& rhs){
	for (int i = 0; i < 16; ++i) {
		this->data[i] -= rhs.data[i];
	}
}

//TODO(o,delle) look into optimizing this by transposing to remove a loop, see Unreal Matrix.h
inline Matrix4  Matrix4::operator *  (const Matrix4& rhs) const{
	Matrix4 newMatrix;
	for (int i = 0; i < 4; ++i) { //i=m
		for (int j = 0; j < 4; ++j) { //j=p
			for (int k = 0; k < 4; ++k) { //k=n
				newMatrix.data[4 * i + j] += this->data[4 * i + k] * rhs.data[4 * k + j];
			}
		}
	}
	return newMatrix;
}

inline void    Matrix4::operator *= (const Matrix4& rhs){
	Matrix4 newMatrix;
	for (int i = 0; i < 4; ++i) { //i=m
		for (int j = 0; j < 4; ++j) { //j=p
			for (int k = 0; k < 4; ++k) { //k=n
				newMatrix.data[4 * i + j] += this->data[4 * i + k] * rhs.data[4 * k + j];
			}
		}
	}
	*this = newMatrix;
}

//element-wise multiplication
inline Matrix4  Matrix4::operator ^  (const Matrix4& rhs) const{
	Matrix4 newMatrix(*this);
	for (int i = 0; i < 16; ++i) {
		newMatrix.data[i] *= rhs.data[i];
	}
	return newMatrix;
} 

//element-wise multiplication and assignment
inline void    Matrix4::operator ^= (const Matrix4& rhs){
	for (int i = 0; i < 16; ++i) {
		this->data[i] *= rhs.data[i];
	}
}

//element-wise division
inline Matrix4  Matrix4::operator %  (const Matrix4& rhs) const{
	Matrix4 newMatrix(*this);
	for (int i = 0; i < 16; ++i) {
		ASSERT(rhs.data[i] != 0, "Matrix4 element-wise division doesnt allow zeros in the right matrix");
		newMatrix.data[i] /= rhs.data[i];
	}
	return newMatrix;
} 

//element-wise division and assignment
inline void    Matrix4::operator %= (const Matrix4& rhs){
	for (int i = 0; i < 16; ++i) {
		ASSERT(rhs.data[i] != 0, "Matrix4 element-wise division doesnt allow zeros in the right matrix");
		this->data[i] /= rhs.data[i];
	}
}

inline bool	   Matrix4::operator	== (const Matrix4& rhs) const { 
	for (int i = 0; i < 16; ++i) {
		if (this->data[i] != rhs.data[i]) {
			return false;
		}
	}
	return true;
}

inline bool    Matrix4::operator	!= (const Matrix4& rhs) const { 
	return !(*this == rhs); 
}



//// Functions ////

//TODO(c,delle) clean up Matrix4.str() and Matrix4.str2F()
inline const std::string Matrix4::str() const {
	std::string str = "Matrix4:\n|";
	for (int i = 0; i < 15; ++i) {
		char buffer[15];
		std::snprintf(buffer, 15, "%+.6f", data[i]);
		str += std::string(buffer);
		if ((i+1) % 4 != 0) {
			str += ", ";
		} else {
			str += "|\n|";
		}
	}
	char buffer[15];
	std::snprintf(buffer, 15, "%+.6f", data[15]);
	str += std::string(buffer) + "|";
	return str;
};

inline const std::string Matrix4::str2f() const {
	std::string str = "Matrix4:\n|";
	for (int i = 0; i < 15; ++i) {
		char buffer[15];
		std::snprintf(buffer, 15, "%+.2f", data[i]);
		str += std::string(buffer);
		if ((i+1) % 4 != 0) {
			str += ", ";
		} else {
			str += "|\n|";
		}
	}
	char buffer[15];
	std::snprintf(buffer, 15, "%+.2f", data[15]);
	str += std::string(buffer) + "|";
	return str;
};

//converts the rows into columns and vice-versa
inline Matrix4 Matrix4::Transpose() const{
	Matrix4 newMatrix;
	for (int i = 0; i < 16; ++i) {
		newMatrix.data[i] = data[4 * (i%4) + (i/4)];
	}
	return newMatrix;
}

//returns the determinant of the matrix
inline float Matrix4::Determinant() const{
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

//returns the determinant of this matrix without the specified row and column
inline float Matrix4::Minor(int row, int col) const {
	float arr[9];
	int index = 0;
	for (int i = 0; i < 4; ++i) {
		if (i == row) continue;
		for (int j = 0; j < 4; ++j) {
			if (j == col) continue;
			arr[index++] = data[4 * i + j];
		}
	}

	//3x3 determinant
	return (arr[0] * arr[4] * arr[8]) + (arr[1] * arr[5] * arr[6]) + (arr[2] * arr[3] * arr[7]) -	
			(arr[2] * arr[4] * arr[6]) - (arr[1] * arr[3] * arr[8]) - (arr[0] * arr[5] * arr[7]);
}

//returns the cofactor (minor with adjusted sign based on location in matrix) at given row and column
inline float Matrix4::Cofactor(int row, int col) const{
	if ((row + col) % 2) {
		return -Minor(row, col);
	} else {
		return Minor(row, col);
	}
}

//returns the transposed matrix of cofactors of this matrix
inline Matrix4 Matrix4::Adjoint() const {
	Matrix4 newMatrix = Matrix4();
	int index = 0;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			newMatrix.data[index++] = this->Cofactor(i, j);
		}
	}
	return newMatrix.Transpose();
}

//returns the adjoint divided by the determinant
inline Matrix4 Matrix4::Inverse() const {
	ASSERT(this->Determinant(), "Matrix4 inverse does not exist if determinant is zero");
	return this->Adjoint() / this->Determinant();
}

//returns a rotation transformation matrix based on input in degrees
inline Matrix4 Matrix4::RotationMatrixX(float degrees) {
	degrees *= TO_RADIANS;
	float c = cosf(degrees);
	float s = sinf(degrees);
	return Matrix4(
		1,	0,	0,	0,
		0,	c,	-s,	0,
		0,	s,	c,	0,
		0,	0,	0,	1
	);
}

//returns a rotation transformation matrix based on input in degrees
inline Matrix4 Matrix4::RotationMatrixY(float degrees) {
	degrees *= TO_RADIANS;
	float c = cosf(degrees);
	float s = sinf(degrees);
	return Matrix4(
		c,	0,	s,	0,
		0,	1,	0,	0,
		-s,	0,	c,	0,
		0,	0,	0,	1
	);
}

//returns a rotation transformation matrix based on input in degrees
inline Matrix4 Matrix4::RotationMatrixZ(float degrees) {
	degrees *= TO_RADIANS;
	float c = cosf(degrees);
	float s = sinf(degrees);
	return Matrix4(
		c,	-s,	0,	0,
		s,	c,	0,	0,
		0,	0,	1,	0,
		0,	0,	0,	1
	);
}
