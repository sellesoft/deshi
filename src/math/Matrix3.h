#pragma once
#include "../utils/Debug.h"

struct Vector3;
struct Matrix4;

struct Matrix3 {
	float data[9]{};
	
	Matrix3() {}
	Matrix3(float _00, float _01, float _02,
			float _10, float _11, float _12,
			float _20, float _21, float _22);
	Matrix3(const Matrix3& m);
	
	static const Matrix3 IDENTITY;
	
	float&	operator () (u32 row, u32 col);
	float   operator () (u32 row, u32 col) const;
	void	operator =	(const Matrix3& rhs);
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
	float Determinant() const;
	float Minor(int row, int col) const;
	float Cofactor(int row, int col) const;
	Matrix3 Adjoint() const;
	Matrix3 Inverse() const;
	
	static Matrix3 RotationMatrixX(float degrees);
	static Matrix3 RotationMatrixY(float degrees);
	static Matrix3 RotationMatrixZ(float degrees);
	
	//Non-Matrix3 vs Matrix3 interactions defined in Matrix.h/Math.h
	Matrix4 To4x4();
	static Matrix3 RotationMatrix(Vector3 rotation);
	static Matrix3 ScaleMatrix(Vector3 scale);
};



//// Operators ////

//element accessor: matrix(row,col)
inline float& Matrix3::operator () (u32 row, u32 col) {
	ASSERT(row < 3 && col < 3, "Matrix3 subscript out of bounds");
	return data[3*row + col];
}

//element accessor [read-only]: matrix(row,col)
inline float Matrix3::operator () (u32 row, u32 col) const {
	ASSERT(row < 3 && col < 3, "Matrix3 subscript out of bounds");
	return data[3 * row + col];
}

//copies the data from rhs
inline void	   Matrix3::operator =  (const Matrix3& rhs) {
	memcpy(&data, &rhs.data, 9*sizeof(float));
}

//scalar multiplication
inline Matrix3  Matrix3::operator *  (const float& rhs) const {
	Matrix3 newMatrix(*this);
	for (int i = 0; i < 9; ++i) {
		newMatrix.data[i] *= rhs;
	}
	return newMatrix;
}

//scalar multiplication and assignment
inline void    Matrix3::operator *= (const float& rhs) {
	for (int i = 0; i < 9; ++i) {
		data[i] *= rhs;
	}
}

//scalar division
inline Matrix3  Matrix3::operator /  (const float& rhs) const {
	ASSERT(rhs != 0, "Matrix3 elements cant be divided by zero");
	Matrix3 newMatrix(*this);
	for (int i = 0; i < 9; ++i) {
		newMatrix.data[i] /= rhs;
	}
	return newMatrix;
}

//scalar division and assignment
inline void    Matrix3::operator /= (const float& rhs){
	ASSERT(rhs != 0, "Matrix3 elements cant be divided by zero");
	for (int i = 0; i < 9; ++i) {
		data[i] /= rhs;
	}
}

//element-wise addition
inline Matrix3  Matrix3::operator +  (const Matrix3& rhs) const{
	Matrix3 newMatrix(*this);
	for (int i = 0; i < 9; ++i) {
		newMatrix.data[i] += rhs.data[i];
	}
	return newMatrix;
}

//element-wise addition and assignment
inline void    Matrix3::operator += (const Matrix3& rhs){
	for (int i = 0; i < 9; ++i) {
		this->data[i] += rhs.data[i];
	}
}

//element-wise substraction
inline Matrix3  Matrix3::operator -  (const Matrix3& rhs) const{
	Matrix3 newMatrix(*this);
	for (int i = 0; i < 9; ++i) {
		newMatrix.data[i] -= rhs.data[i];
	}
	return newMatrix;
}

//element-wise substraction and assignment
inline void    Matrix3::operator -= (const Matrix3& rhs){
	for (int i = 0; i < 9; ++i) {
		this->data[i] -= rhs.data[i];
	}
}

//TODO(Op,delle) look into optimizing this by transposing to remove a loop, see Unreal Matrix.h
inline Matrix3  Matrix3::operator *  (const Matrix3& rhs) const{
	Matrix3 newMatrix;
	for (int i = 0; i < 3; ++i) { //i=m
		for (int j = 0; j < 3; ++j) { //j=p
			for (int k = 0; k < 3; ++k) { //k=n
				newMatrix.data[3 * i + j] += this->data[3 * i + k] * rhs.data[3 * k + j];
			}
		}
	}
	return newMatrix;
}

inline void    Matrix3::operator *= (const Matrix3& rhs){
	Matrix3 newMatrix;
	for (int i = 0; i < 3; ++i) { //i=m
		for (int j = 0; j < 3; ++j) { //j=p
			for (int k = 0; k < 3; ++k) { //k=n
				newMatrix.data[3 * i + j] += this->data[3 * i + k] * rhs.data[3 * k + j];
			}
		}
	}
	*this = newMatrix;
}

//element-wise multiplication
inline Matrix3  Matrix3::operator ^  (const Matrix3& rhs) const{
	Matrix3 newMatrix(*this);
	for (int i = 0; i < 9; ++i) {
		newMatrix.data[i] *= rhs.data[i];
	}
	return newMatrix;
} 

//element-wise multiplication and assignment
inline void    Matrix3::operator ^= (const Matrix3& rhs){
	for (int i = 0; i < 9; ++i) {
		this->data[i] *= rhs.data[i];
	}
}

//element-wise division
inline Matrix3  Matrix3::operator %  (const Matrix3& rhs) const{
	Matrix3 newMatrix(*this);
	for (int i = 0; i < 9; ++i) {
		ASSERT(rhs.data[i] != 0, "Matrix3 element-wise division doesnt allow zeros in the right matrix");
		newMatrix.data[i] /= rhs.data[i];
	}
	return newMatrix;
} 

//element-wise division and assignment
inline void    Matrix3::operator %= (const Matrix3& rhs){
	for (int i = 0; i < 9; ++i) {
		ASSERT(rhs.data[i] != 0, "Matrix3 element-wise division doesnt allow zeros in the right matrix");
		this->data[i] /= rhs.data[i];
	}
}

inline bool	   Matrix3::operator	== (const Matrix3& rhs) const { 
	for (int i = 0; i < 9; ++i) {
		if (this->data[i] != rhs.data[i]) {
			return false;
		}
	}
	return true;
}

inline bool    Matrix3::operator	!= (const Matrix3& rhs) const { 
	return !(*this == rhs); 
}


//// Functions ////

//TODO(ClMa,delle) clean up Matrix3.str() and Matrix3.str2F()
inline const std::string Matrix3::str() const {
	std::string str = "Matrix3:\n|";
	for (int i = 0; i < 8; ++i) {
		char buffer[15];
		std::snprintf(buffer, 15, "%+.6f", data[i]);
		str += std::string(buffer);
		if ((i+1) % 3 != 0) {
			str += ", ";
		} else {
			str += "|\n|";
		}
	}
	char buffer[15];
	std::snprintf(buffer, 15, "%+.6f", data[8]);
	str += std::string(buffer) + "|";
	return str;
};

inline const std::string Matrix3::str2f() const {
	std::string str = "Matrix3:\n|";
	for (int i = 0; i < 8; ++i) {
		char buffer[15];
		std::snprintf(buffer, 15, "%+.2f", data[i]);
		str += std::string(buffer);
		if ((i + 1) % 3 != 0) {
			str += ", ";
		} else {
			str += "|\n|";
		}
	}
	char buffer[15];
	std::snprintf(buffer, 15, "%+.2f", data[8]);
	str += std::string(buffer) + "|";
	return str;
};

//converts the rows into columns and vice-versa
inline Matrix3 Matrix3::Transpose() const{
	Matrix3 newMatrix;
	for (int i = 0; i < 9; ++i) {
		newMatrix.data[i] = data[3 * (i%3) + (i/3)];
	}
	return newMatrix;
}

//returns the determinant of the matrix
inline float Matrix3::Determinant() const{
	//aei + bfg + cdh - ceg - bdi - afh
	return  (data[0] * data[4] * data[8]) +		//aei
		(data[1] * data[5] * data[6]) +		//bfg
		(data[2] * data[3] * data[7]) -		//cdh
		(data[2] * data[4] * data[6]) -		//ceg
		(data[1] * data[3] * data[8]) -		//bdi
		(data[0] * data[5] * data[7]);		//afh
}

//returns the determinant of this matrix without the specified row and column
inline float Matrix3::Minor(int row, int col) const {
	float arr[4];
	int index = 0;
	for (int i = 0; i < 3; ++i) {
		if (i == row) continue;
		for (int j = 0; j < 3; ++j) {
			if (j == col) continue;
			arr[index++] = data[3 * i + j];
		}
	}
	
	//2x2 determinant
	return (data[0] * data[3]) - (data[1] * data[2]);
}

//returns the cofactor (minor with adjusted sign based on location in matrix) at given row and column
inline float Matrix3::Cofactor(int row, int col) const{
	if ((row + col) % 2) {
		return -Minor(row, col);
	} else {
		return Minor(row, col);
	}
}

//returns the transposed matrix of cofactors of this matrix
inline Matrix3 Matrix3::Adjoint() const {
	Matrix3 newMatrix = Matrix3();
	int index = 0;
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			newMatrix.data[index++] = this->Cofactor(i, j);
		}
	}
	return newMatrix.Transpose();
}

//returns the adjoint divided by the determinant
inline Matrix3 Matrix3::Inverse() const {
	ASSERT(this->Determinant(), "Matrix3 inverse does not exist if determinant is zero");
	return this->Adjoint() / this->Determinant();
}

//returns a LH rotation transformation matrix based on input in degrees
inline Matrix3 Matrix3::RotationMatrixX(float angle) {
	angle = RADIANS(angle);
	float c = cosf(angle); float s = sinf(angle);
	return Matrix3(1,  0, 0,
				   0,  c, s,
				   0, -s, c);
}

//returns a LH rotation transformation matrix based on input in degrees
inline Matrix3 Matrix3::RotationMatrixY(float angle) {
	angle = RADIANS(angle);
	float c = cosf(angle); float s = sinf(angle);
	return Matrix3(c, 0, -s,
				   0, 1,  0,
				   s, 0,  c);
}

//returns a LH rotation transformation matrix based on input in degrees
inline Matrix3 Matrix3::RotationMatrixZ(float angle) {
	angle = RADIANS(angle);
	float c = cosf(angle); float s = sinf(angle);
	return Matrix3(c,  s, 0,
				   -s, c, 0,
				   0,  0, 1);
}
