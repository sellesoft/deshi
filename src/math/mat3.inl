#pragma once
#ifndef DESHI_mat3_INL
#define DESHI_mat3_INL

//////////////////////
//// constructors ////
//////////////////////

inline mat3::
mat3(float _00, float _01, float _02,
        float _10, float _11, float _12,
        float _20, float _21, float _22) {
    data[0] = _00; data[1] = _01; data[2] = _02;
    data[3] = _10; data[4] = _11; data[5] = _12;
    data[6] = _20; data[7] = _21; data[8] = _22;
}

inline mat3::
mat3(const mat3& m) {
    memcpy(this->data, &m.data, 9*sizeof(float));
}

///////////////////
//// constants ////
///////////////////

inline const mat3 mat3::IDENTITY = mat3(1,0,0,
                                        0,1,0,
                                        0,0,1);

///////////////////
//// operators ////
///////////////////

//element accessor: matrix(row,col)
inline float& mat3::
operator () (u32 row, u32 col) {
    Assert(row < 3 && col < 3, "mat3 subscript out of bounds");
    return data[3*row + col];
}

//element accessor [read-only]: matrix(row,col)
inline float mat3::
operator () (u32 row, u32 col) const {
    Assert(row < 3 && col < 3, "mat3 subscript out of bounds");
    return data[3 * row + col];
}

//copies the data from rhs
inline void mat3::
operator =  (const mat3& rhs) {
    memcpy(this->data, &rhs.data, 9*sizeof(float));
}

//scalar multiplication
inline mat3 mat3::
operator *  (const float& rhs) const {
    mat3 newMatrix(*this);
    for (int i = 0; i < 9; ++i) {
        newMatrix.data[i] *= rhs;
    }
    return newMatrix;
}

//scalar multiplication and assignment
inline void mat3::
operator *= (const float& rhs) {
    for (int i = 0; i < 9; ++i) {
        data[i] *= rhs;
    }
}

//scalar division
inline mat3 mat3::
operator /  (const float& rhs) const {
    Assert(rhs != 0, "mat3 elements cant be divided by zero");
    mat3 newMatrix(*this);
    for (int i = 0; i < 9; ++i) {
        newMatrix.data[i] /= rhs;
    }
    return newMatrix;
}

//scalar division and assignment
inline void mat3::
operator /= (const float& rhs){
    Assert(rhs != 0, "mat3 elements cant be divided by zero");
    for (int i = 0; i < 9; ++i) {
        data[i] /= rhs;
    }
}

//element-wise addition
inline mat3 mat3::
operator +  (const mat3& rhs) const{
    mat3 newMatrix(*this);
    for (int i = 0; i < 9; ++i) {
        newMatrix.data[i] += rhs.data[i];
    }
    return newMatrix;
}

//element-wise addition and assignment
inline void mat3::
operator += (const mat3& rhs){
    for (int i = 0; i < 9; ++i) {
        this->data[i] += rhs.data[i];
    }
}

//element-wise substraction
inline mat3 mat3::
operator -  (const mat3& rhs) const{
    mat3 newMatrix(*this);
    for (int i = 0; i < 9; ++i) {
        newMatrix.data[i] -= rhs.data[i];
    }
    return newMatrix;
}

//element-wise substraction and assignment
inline void mat3::
operator -= (const mat3& rhs){
    for (int i = 0; i < 9; ++i) {
        this->data[i] -= rhs.data[i];
    }
}

//TODO(delle,Op) look into optimizing this by transposing to remove a loop, see Unreal Matrix.h
inline mat3 mat3::
operator *  (const mat3& rhs) const{
    mat3 newMatrix;
    for (int i = 0; i < 3; ++i) { //i=m
        for (int j = 0; j < 3; ++j) { //j=p
            for (int k = 0; k < 3; ++k) { //k=n
                newMatrix.data[3 * i + j] += this->data[3 * i + k] * rhs.data[3 * k + j];
            }
        }
    }
    return newMatrix;
}

inline void mat3::
operator *= (const mat3& rhs){
    mat3 newMatrix;
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
inline mat3 mat3::
operator ^  (const mat3& rhs) const{
    mat3 newMatrix(*this);
    for (int i = 0; i < 9; ++i) {
        newMatrix.data[i] *= rhs.data[i];
    }
    return newMatrix;
} 

//element-wise multiplication and assignment
inline void mat3::
operator ^= (const mat3& rhs){
    for (int i = 0; i < 9; ++i) {
        this->data[i] *= rhs.data[i];
    }
}

//element-wise division
inline mat3 mat3::
operator %  (const mat3& rhs) const{
    mat3 newMatrix(*this);
    for (int i = 0; i < 9; ++i) {
        Assert(rhs.data[i] != 0, "mat3 element-wise division doesnt allow zeros in the right matrix");
        newMatrix.data[i] /= rhs.data[i];
    }
    return newMatrix;
} 

//element-wise division and assignment
inline void mat3::
operator %= (const mat3& rhs){
    for (int i = 0; i < 9; ++i) {
        Assert(rhs.data[i] != 0, "mat3 element-wise division doesnt allow zeros in the right matrix");
        this->data[i] /= rhs.data[i];
    }
}

inline bool mat3::
operator == (const mat3& rhs) const { 
    return memcmp(this->data, rhs.data, sizeof(f32)*9) == 0;
}

inline bool mat3::
operator != (const mat3& rhs) const { 
    return !(*this == rhs); 
}


///////////////////
//// functions ////
///////////////////

//TODO(delle,ClMa) clean up mat3.str() and mat3.str2F()
inline const std::string mat3::
str() const {
    std::string str = "mat3:\n|";
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

inline const std::string mat3::
str2f() const {
    std::string str = "mat3:\n|";
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
inline mat3 mat3::
Transpose() const{
    mat3 newMatrix;
    for (int i = 0; i < 9; ++i) {
        newMatrix.data[i] = data[3 * (i%3) + (i/3)];
    }
    return newMatrix;
}

//returns the determinant of the matrix
inline float mat3::
Determinant() const{
    //aei + bfg + cdh - ceg - bdi - afh
    return  (data[0] * data[4] * data[8]) +		//aei
	(data[1] * data[5] * data[6]) +		//bfg
	(data[2] * data[3] * data[7]) -		//cdh
	(data[2] * data[4] * data[6]) -		//ceg
	(data[1] * data[3] * data[8]) -		//bdi
	(data[0] * data[5] * data[7]);		//afh
}

//returns the determinant of this matrix without the specified row and column
inline float mat3::
Minor(int row, int col) const {
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
inline float mat3::
Cofactor(int row, int col) const{
    if ((row + col) % 2) {
        return -Minor(row, col);
    } else {
        return Minor(row, col);
    }
}

//returns the transposed matrix of cofactors of this matrix
inline mat3 mat3::
Adjoint() const {
    mat3 newMatrix = mat3();
    int index = 0;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            newMatrix.data[index++] = this->Cofactor(i, j);
        }
    }
    return newMatrix.Transpose();
}

//returns the adjoint divided by the determinant
inline mat3 mat3::
Inverse() const {
    float det = this->Determinant();
    Assert(det, "mat3 inverse does not exist if determinant is zero");
    return this->Adjoint() / det;
}

//returns a LH rotation transformation matrix based on input in degrees
inline mat3 mat3::
RotationMatrixX(float angle) {
    angle = RADIANS(angle);
    float c = cosf(angle); float s = sinf(angle);
    return mat3(1,  0, 0,
                   0,  c, s,
                   0, -s, c);
}

//returns a LH rotation transformation matrix based on input in degrees
inline mat3 mat3::
RotationMatrixY(float angle) {
    angle = RADIANS(angle);
    float c = cosf(angle); float s = sinf(angle);
    return mat3(c, 0, -s,
                   0, 1,  0,
                   s, 0,  c);
}

//returns a LH rotation transformation matrix based on input in degrees
inline mat3 mat3::
RotationMatrixZ(float angle) {
    angle = RADIANS(angle);
    float c = cosf(angle); float s = sinf(angle);
    return mat3(c,  s, 0,
                   -s, c, 0,
                   0,  0, 1);
}

//returns a pre-multiplied X->Y->Z LH rotation transformation matrix based on input in degrees
inline mat3 mat3::
RotationMatrix(float x, float y, float z) {
    x = RADIANS(x); y = RADIANS(y); z = RADIANS(z);
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

#endif //DESHI_MATRIX3_INL
