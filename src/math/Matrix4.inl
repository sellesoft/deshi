#pragma once
#ifndef DESHI_MATRIX4_INL
#define DESHI_MATRIX4_INL

//////////////////////
//// constructors ////
//////////////////////

inline Matrix4::
Matrix4(float x) {
	data[0]  = x; data[1]  = x; data[2]  = x; data[3]  = x;
	data[4]  = x; data[5]  = x; data[6]  = x; data[7]  = x;
	data[8]  = x; data[9]  = x; data[10] = x; data[11] = x;
	data[12] = x; data[13] = x; data[14] = x; data[15] = x;
}

inline Matrix4::
Matrix4(float _00, float _01, float _02, float _03,
		float _10, float _11, float _12, float _13,
		float _20, float _21, float _22, float _23,
		float _30, float _31, float _32, float _33) {
	data[0]  = _00; data[1]  = _01; data[2]  = _02; data[3]  = _03;
	data[4]  = _10; data[5]  = _11; data[6]  = _12; data[7]  = _13;
	data[8]  = _20; data[9]  = _21; data[10] = _22; data[11] = _23;
	data[12] = _30; data[13] = _31; data[14] = _32; data[15] = _33;
}

inline Matrix4::
Matrix4(const Matrix4& m) {
	memcpy(&data, &m.data, 16*sizeof(float));
}

inline Matrix4::
Matrix4(float* data){
	memcpy(this->data, data, 16*sizeof(float));
}

///////////////////
//// constants ////
///////////////////

inline const Matrix4 Matrix4::IDENTITY = Matrix4(1, 0, 0, 0, 
												 0, 1, 0, 0, 
												 0, 0, 1, 0, 
												 0, 0, 0, 1);

///////////////////
//// operators ////
///////////////////

//element accessor: matrix(row,col)
inline float& Matrix4::
operator () (u32 row, u32 col) {
	ASSERT(row < 4 && col < 4, "Matrix4 subscript out of bounds");
	return data[4*row + col];
}

inline float Matrix4::
operator () (u32 row, u32 col) const {
	ASSERT(row < 4 && col < 4, "Matrix4 subscript out of bounds");
	return data[4 * row + col];
}

//creates the data from rhs
inline void Matrix4::
operator =  (const Matrix4& rhs) {
	memcpy(&data, &rhs.data, 16*sizeof(float));
}

//scalar multiplication
inline Matrix4 Matrix4::
operator *  (const float& rhs) const {
	Matrix4 newMatrix(*this);
	for (int i = 0; i < 16; ++i) {
		newMatrix.data[i] *= rhs;
	}
	return newMatrix;
}

//scalar multiplication and assignment
inline void Matrix4::
operator *= (const float& rhs) {
	for (int i = 0; i < 16; ++i) {
		data[i] *= rhs;
	}
}

//scalar division
inline Matrix4 Matrix4::
operator /  (const float& rhs) const {
	ASSERT(rhs != 0, "Matrix4 elements cant be divided by zero");
	Matrix4 newMatrix(*this);
	for (int i = 0; i < 16; ++i) {
		newMatrix.data[i] /= rhs;
	}
	return newMatrix;
}

//scalar division and assignment
inline void Matrix4::
operator /= (const float& rhs){
	ASSERT(rhs != 0, "Matrix4 elements cant be divided by zero");
	for (int i = 0; i < 16; ++i) {
		data[i] /= rhs;
	}
}

//element-wise addition
inline Matrix4 Matrix4::
operator +  (const Matrix4& rhs) const{
	Matrix4 newMatrix(*this);
	for (int i = 0; i < 16; ++i) {
		newMatrix.data[i] += rhs.data[i];
	}
	return newMatrix;
}

//element-wise addition and assignment
inline void Matrix4::
operator += (const Matrix4& rhs){
	for (int i = 0; i < 16; ++i) {
		this->data[i] += rhs.data[i];
	}
}

//element-wise substraction
inline Matrix4 Matrix4::
operator -  (const Matrix4& rhs) const{
	Matrix4 newMatrix(*this);
	for (int i = 0; i < 16; ++i) {
		newMatrix.data[i] -= rhs.data[i];
	}
	return newMatrix;
}

//element-wise substraction and assignment
inline void Matrix4::
operator -= (const Matrix4& rhs){
	for (int i = 0; i < 16; ++i) {
		this->data[i] -= rhs.data[i];
	}
}

//TODO(delle,OpMa) look into optimizing this by transposing to remove a loop, see Unreal Matrix.h
inline Matrix4 Matrix4::
operator *  (const Matrix4& rhs) const{
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

inline void Matrix4::
operator *= (const Matrix4& rhs){
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
inline Matrix4 Matrix4::
operator ^  (const Matrix4& rhs) const{
	Matrix4 newMatrix(*this);
	for (int i = 0; i < 16; ++i) {
		newMatrix.data[i] *= rhs.data[i];
	}
	return newMatrix;
} 

//element-wise multiplication and assignment
inline void Matrix4::
operator ^= (const Matrix4& rhs){
	for (int i = 0; i < 16; ++i) {
		this->data[i] *= rhs.data[i];
	}
}

//element-wise division
inline Matrix4 Matrix4::
operator %  (const Matrix4& rhs) const{
	Matrix4 newMatrix(*this);
	for (int i = 0; i < 16; ++i) {
		ASSERT(rhs.data[i] != 0, "Matrix4 element-wise division doesnt allow zeros in the right matrix");
		newMatrix.data[i] /= rhs.data[i];
	}
	return newMatrix;
} 

//element-wise division and assignment
inline void Matrix4::
operator %= (const Matrix4& rhs){
	for (int i = 0; i < 16; ++i) {
		ASSERT(rhs.data[i] != 0, "Matrix4 element-wise division doesnt allow zeros in the right matrix");
		this->data[i] /= rhs.data[i];
	}
}

inline bool Matrix4::
operator == (const Matrix4& rhs) const { 
	for (int i = 0; i < 16; ++i) {
		if (this->data[i] != rhs.data[i]) {
			return false;
		}
	}
	return true;
}

inline bool Matrix4::
operator != (const Matrix4& rhs) const { 
	return !(*this == rhs); 
}

///////////////////
//// functions ////
///////////////////

//TODO(delle,ClMa) clean up Matrix4.str() and Matrix4.str2F()
inline const std::string Matrix4::
str() const {
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

inline const std::string Matrix4::
str2f() const {
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
inline Matrix4 Matrix4::
Transpose() const{
	Matrix4 newMatrix;
	for (int i = 0; i < 16; ++i) {
		newMatrix.data[i] = data[4 * (i%4) + (i/4)];
	}
	return newMatrix;
}

//returns the determinant of the matrix
inline float Matrix4::
Determinant() const{
	return data[0] * (data[ 5] * (data[10] * data[15] - data[11] * data[14]) -
					  data[ 9] * (data[ 6] * data[15] - data[ 7] * data[14]) + 
					  data[13] * (data[ 6] * data[11] - data[ 7] * data[10]))
		-
		data[4]    * (data[ 1] * (data[10] * data[15] - data[11] * data[14]) -
					  data[ 9] * (data[ 2] * data[15] - data[ 3] * data[14]) +
					  data[13] * (data[ 2] * data[11] - data[ 3] * data[10]))
		+
		data[8]    * (data[ 1] * (data[ 6] * data[15] - data[ 7] * data[14]) -
					  data[ 5] * (data[ 2] * data[15] - data[ 3] * data[14]) +
					  data[13] * (data[ 2] * data[ 7] - data[ 3] * data[ 6]))
		-
		data[12]   * (data[ 1] * (data[ 6] * data[11] - data[ 7] * data[10]) -
					  data[ 5] * (data[ 2] * data[11] - data[ 3] * data[10]) +
					  data[ 9] * (data[ 2] * data[ 7] - data[ 3] * data[ 6]));
}

//returns the determinant of this matrix without the specified row and column
inline float Matrix4::
Minor(int row, int col) const {
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
inline float Matrix4::
Cofactor(int row, int col) const{
	if ((row + col) % 2) {
		return -Minor(row, col);
	} else {
		return Minor(row, col);
	}
}

//returns the transposed matrix of cofactors of this matrix
inline Matrix4 Matrix4::
Adjoint() const {
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
inline Matrix4 Matrix4::
Inverse() const {
	ASSERT(this->Determinant(), "Matrix4 inverse does not exist if determinant is zero");
	return this->Adjoint() / this->Determinant();
}

//returns a LH rotation transformation matrix in degrees around the X axis
inline Matrix4 Matrix4::
RotationMatrixX(float angle) {
	angle = RADIANS(angle);
	float c = cosf(angle); float s = sinf(angle);
	return Matrix4(1,  0, 0, 0,
				   0,  c, s, 0,
				   0, -s, c, 0,
				   0,  0, 0, 1);
}

//returns a LH rotation transformation matrix in degrees around the Y axis
inline Matrix4 Matrix4::
RotationMatrixY(float angle) {
	angle = RADIANS(angle);
	float c = cosf(angle); float s = sinf(angle);
	return Matrix4(c, 0, -s, 0,
				   0, 1,  0, 0,
				   s, 0,  c, 0,
				   0, 0,  0, 1);
}

//returns a LH rotation transformation matrix in degrees around the Z axis
inline Matrix4 Matrix4::
RotationMatrixZ(float angle) {
	angle = RADIANS(angle);
	float c = cosf(angle); float s = sinf(angle);
	return Matrix4(c,  s, 0, 0,
				   -s, c, 0, 0,
				   0,  0, 1, 0,
				   0,  0, 0, 1);
}

//TODO(sushi, Ma) fix the rotation order here since I think its incorrect. It should be Y->Z->X right?
//returns a pre-multiplied X->Y->Z LH rotation transformation matrix based on input in degrees
inline Matrix4 Matrix4::
RotationMatrix(float x, float y, float z) {
	x = RADIANS(x); y = RADIANS(y); z = RADIANS(z);
	float cX = cosf(x); float sX = sinf(x);
	float cY = cosf(y); float sY = sinf(y);
	float cZ = cosf(z); float sZ = sinf(z);
	float r00 = cZ*cY;            float r01 = cY*sZ;            float r02 = -sY;
	float r10 = cZ*sX*sY - cX*sZ; float r11 = cZ*cX + sX*sY*sZ; float r12 = sX*cY;
	float r20 = cZ*cX*sY + sX*sZ; float r21 = cX*sY*sZ - cZ*sX; float r22 = cX*cY;
	return Matrix4(r00, r01, r02, 0,
				   r10, r11, r12, 0,
				   r20, r21, r22, 0,
				   0,   0,   0,   1);
}

//returns a translation matrix where (0,3) = translation.x, (1,3) = translation.y, (2,3) = translation.z
inline Matrix4 Matrix4::
TranslationMatrix(float x, float y, float z) {
	return Matrix4(1, 0, 0, 0,
				   0, 1, 0, 0,
				   0, 0, 1, 0,
				   x, y, z, 1);
}

//returns a scale matrix where (0,0) = scale.x, (1,1) = scale.y, (2,2) = scale.z
inline Matrix4 Matrix4::
ScaleMatrix(float x, float y, float z) {
	Matrix4 newMatrix = Matrix4::IDENTITY;
	newMatrix.data[0] = x;
	newMatrix.data[4] = y;
	newMatrix.data[8] = z;
	return newMatrix;
}

//////////////
//// hash ////
//////////////

namespace std{
	template<> struct hash<Matrix4>{
		inline size_t operator()(Matrix4 const& m) const{
			size_t seed = 0;
			hash<float> hasher; size_t hash;
			for_n(i,16){
				hash = hasher(m.data[i]);
				hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= hash;
			}
			return seed;
		}
	};
};


#endif //DESHI_MATRIX4_INL