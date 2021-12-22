#pragma once
#ifndef DESHI_MAT3_INL
#define DESHI_MAT3_INL

//////////////////////
//// constructors ////
//////////////////////

inline mat3::
mat3(float x) {
	arr[0] = x; arr[1] = x; arr[2] = x; 
	arr[3] = x; arr[4] = x; arr[5] = x; 
	arr[6] = x; arr[7] = x; arr[8] = x;
}

inline mat3::
mat3(float _00, float _01, float _02,
	 float _10, float _11, float _12,
	 float _20, float _21, float _22){
	arr[0] = _00; arr[1] = _01; arr[2] = _02;
	arr[3] = _10; arr[4] = _11; arr[5] = _12;
	arr[6] = _20; arr[7] = _21; arr[8] = _22;
}

inline mat3::
mat3(const mat3& m){
	arr[0] = m.arr[0]; arr[1] = m.arr[1]; arr[2] = m.arr[2];
	arr[3] = m.arr[3]; arr[4] = m.arr[4]; arr[5] = m.arr[5];
	arr[6] = m.arr[6]; arr[7] = m.arr[7]; arr[8] = m.arr[8];
}

inline mat3::
mat3(float* src){
	arr[0] = src[0]; arr[1] = src[1]; arr[2] = src[2]; 
	arr[3] = src[3]; arr[4] = src[4]; arr[5] = src[5]; 
	arr[6] = src[6]; arr[7] = src[7]; arr[8] = src[8];
}

///////////////////
//// constants ////
///////////////////

inline const mat3 mat3::IDENTITY = mat3(1, 0, 0,
										0, 1, 0,
										0, 0, 1);

///////////////////
//// operators ////
///////////////////

//element accessor: matrix(row,col)
inline float& mat3::
operator()(u32 row, u32 col){
	Assert(row < 3 && col < 3, "mat3 subscript out of bounds");
	return arr[3*row + col];
}

//element accessor [read-only]: matrix(row,col)
inline float mat3::
operator()(u32 row, u32 col) const{
	Assert(row < 3 && col < 3, "mat3 subscript out of bounds");
	return arr[3 * row + col];
}

//copies the arr from rhs
inline void mat3::
operator= (const mat3& rhs){
	arr[0] = rhs.arr[0]; arr[1] = rhs.arr[1]; arr[2] = rhs.arr[2];
	arr[3] = rhs.arr[3]; arr[4] = rhs.arr[4]; arr[5] = rhs.arr[5];
	arr[6] = rhs.arr[6]; arr[7] = rhs.arr[7]; arr[8] = rhs.arr[8];
}

//scalar multiplication
inline mat3 mat3::
operator* (const float& rhs) const{
	mat3 newMatrix(*this);
	for(int i = 0; i < 9; ++i){
		newMatrix.arr[i] *= rhs;
	}
	return newMatrix;
}

//scalar multiplication and assignment
inline void mat3::
operator*=(const float& rhs){
	for(int i = 0; i < 9; ++i){
		arr[i] *= rhs;
	}
}

//scalar division
inline mat3 mat3::
operator/ (const float& rhs) const{
	Assert(rhs != 0, "mat3 elements cant be divided by zero");
	mat3 newMatrix(*this);
	for(int i = 0; i < 9; ++i){
		newMatrix.arr[i] /= rhs;
	}
	return newMatrix;
}

//scalar division and assignment
inline void mat3::
operator/=(const float& rhs){
	Assert(rhs != 0, "mat3 elements cant be divided by zero");
	for(int i = 0; i < 9; ++i){
		arr[i] /= rhs;
	}
}

//element-wise addition
inline mat3 mat3::
operator+ (const mat3& rhs) const{
	mat3 newMatrix(*this);
	for(int i = 0; i < 9; ++i){
		newMatrix.arr[i] += rhs.arr[i];
	}
	return newMatrix;
}

//element-wise addition and assignment
inline void mat3::
operator+=(const mat3& rhs){
	for(int i = 0; i < 9; ++i){
		this->arr[i] += rhs.arr[i];
	}
}

//element-wise substraction
inline mat3 mat3::
operator- (const mat3& rhs) const{
	mat3 newMatrix(*this);
	for(int i = 0; i < 9; ++i){
		newMatrix.arr[i] -= rhs.arr[i];
	}
	return newMatrix;
}

//element-wise substraction and assignment
inline void mat3::
operator-=(const mat3& rhs){
	for(int i = 0; i < 9; ++i){
		this->arr[i] -= rhs.arr[i];
	}
}

//TODO(delle,Op) look into optimizing this by transposing to remove a loop, see Unreal Matrix.h
inline mat3 mat3::
operator* (const mat3& rhs) const{
	mat3 newMatrix;
	for(int i = 0; i < 3; ++i){ //i=m
		for(int j = 0; j < 3; ++j){ //j=p
			for(int k = 0; k < 3; ++k){ //k=n
				newMatrix.arr[3 * i + j] += this->arr[3 * i + k] * rhs.arr[3 * k + j];
			}
		}
	}
	return newMatrix;
}

inline void mat3::
operator*=(const mat3& rhs){
	mat3 newMatrix;
	for(int i = 0; i < 3; ++i){ //i=m
		for(int j = 0; j < 3; ++j){ //j=p
			for(int k = 0; k < 3; ++k){ //k=n
				newMatrix.arr[3 * i + j] += this->arr[3 * i + k] * rhs.arr[3 * k + j];
			}
		}
	}
	*this = newMatrix;
}

//element-wise multiplication
inline mat3 mat3::
operator^ (const mat3& rhs) const{
	mat3 newMatrix(*this);
	for(int i = 0; i < 9; ++i){
		newMatrix.arr[i] *= rhs.arr[i];
	}
	return newMatrix;
} 

//element-wise multiplication and assignment
inline void mat3::
operator^=(const mat3& rhs){
	for(int i = 0; i < 9; ++i){
		this->arr[i] *= rhs.arr[i];
	}
}

//element-wise division
inline mat3 mat3::
operator% (const mat3& rhs) const{
	mat3 newMatrix(*this);
	for(int i = 0; i < 9; ++i){
		Assert(rhs.arr[i] != 0, "mat3 element-wise division doesnt allow zeros in the right matrix");
		newMatrix.arr[i] /= rhs.arr[i];
	}
	return newMatrix;
} 

//element-wise division and assignment
inline void mat3::
operator%=(const mat3& rhs){
	for(int i = 0; i < 9; ++i){
		Assert(rhs.arr[i] != 0, "mat3 element-wise division doesnt allow zeros in the right matrix");
		this->arr[i] /= rhs.arr[i];
	}
}

inline bool mat3::
operator==(const mat3& rhs) const{ 
	for(int i = 0; i < 9; ++i){ if(abs(this->arr[i] - rhs.arr[i]) > M_EPSILON) return false; }
	return true;
}

inline bool mat3::
operator!=(const mat3& rhs) const{ 
	return !(*this == rhs); 
}


///////////////////
//// functions ////
///////////////////

//converts the rows into columns and vice-versa
inline mat3 mat3::
Transpose() const{
	mat3 newMatrix;
	for(int i = 0; i < 9; ++i){
		newMatrix.arr[i] = arr[3 * (i%3) + (i/3)];
	}
	return newMatrix;
}

//returns the determinant of the matrix
inline float mat3::
Determinant() const{
	//aei + bfg + cdh - ceg - bdi - afh
	return 
	(arr[0] * arr[4] * arr[8]) + //aei
	(arr[1] * arr[5] * arr[6]) + //bfg
	(arr[2] * arr[3] * arr[7]) - //cdh
	(arr[2] * arr[4] * arr[6]) - //ceg
	(arr[1] * arr[3] * arr[8]) - //bdi
	(arr[0] * arr[5] * arr[7]);  //afh
}

//returns the determinant of this matrix without the specified row and column
inline float mat3::
Minor(int row, int col) const{
	float arr[4];
	int index = 0;
	for(int i = 0; i < 3; ++i){
		if(i == row) continue;
		for(int j = 0; j < 3; ++j){
			if(j == col) continue;
			arr[index++] = arr[3 * i + j];
		}
	}
	
	//2x2 determinant
	return (arr[0] * arr[3]) - (arr[1] * arr[2]);
}

//returns the cofactor (minor with adjusted sign based on location in matrix) at given row and column
inline float mat3::
Cofactor(int row, int col) const{
	if((row + col) % 2){
		return -Minor(row, col);
	} else {
		return Minor(row, col);
	}
}

//returns the transposed matrix of cofactors of this matrix
inline mat3 mat3::
Adjoint() const{
	mat3 newMatrix = mat3();
	int index = 0;
	for(int i = 0; i < 3; ++i){
		for(int j = 0; j < 3; ++j){
			newMatrix.arr[index++] = this->Cofactor(i, j);
		}
	}
	return newMatrix.Transpose();
}

//returns the adjoint divided by the determinant
inline mat3 mat3::
Inverse() const{
	float det = this->Determinant();
	Assert(det, "mat3 inverse does not exist if determinant is zero");
	return this->Adjoint() / det;
}

//returns a LH rotation transformation matrix based on input in degrees
inline mat3 mat3::
RotationMatrixX(float angle){
	angle = Radians(angle);
	float c = cosf(angle); float s = sinf(angle);
	return mat3(1,  0, 0,
				0,  c, s,
				0, -s, c);
}

//returns a LH rotation transformation matrix based on input in degrees
inline mat3 mat3::
RotationMatrixY(float angle){
	angle = Radians(angle);
	float c = cosf(angle); float s = sinf(angle);
	return mat3(c, 0, -s,
				0, 1,  0,
				s, 0,  c);
}

//returns a LH rotation transformation matrix based on input in degrees
inline mat3 mat3::
RotationMatrixZ(float angle){
	angle = Radians(angle);
	float c = cosf(angle); float s = sinf(angle);
	return mat3(c,  s, 0,
				-s, c, 0,
				0,  0, 1);
}

//returns a pre-multiplied X->Y->Z LH rotation transformation matrix based on input in degrees
inline mat3 mat3::
RotationMatrix(float x, float y, float z){
	x = Radians(x); y = Radians(y); z = Radians(z);
	float cX = cosf(x); float sX = sinf(x);
	float cY = cosf(y); float sY = sinf(y);
	float cZ = cosf(z); float sZ = sinf(z);
	float r00 = cZ*cY;            float r01 = cY*sZ;            float r02 = -sY;
	float r10 = cZ*sX*sY - cX*sZ; float r11 = cZ*cX + sX*sY*sZ; float r12 = sX*cY;
	float r20 = cZ*cX*sY + sX*sZ; float r21 = cX*sY*sZ - cZ*sX; float r22 = cX*cY;
	return mat3(r00, r01, r02,
				r10, r11, r12,
				r20, r21, r22);
}

#endif //DESHI_MAT3_INL
