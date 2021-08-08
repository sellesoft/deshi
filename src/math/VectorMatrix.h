#pragma once
#ifndef DESHI_VECTORMATRIX_H
#define DESHI_VECTORMATRIX_H

#include "vector.h"
#include "matrix.h"

//////////////////////
//// interactions ////
//////////////////////

//// vec3 ////

inline vec3 vec3::
operator *  (const mat3& rhs) const {
    return vec3(x*rhs.data[0] + y*rhs.data[3] + z*rhs.data[6], 
				x*rhs.data[1] + y*rhs.data[4] + z*rhs.data[7], 
				x*rhs.data[2] + y*rhs.data[5] + z*rhs.data[8]);
}

inline void vec3::
operator *= (const mat3& rhs) {
    *this = vec3(x*rhs.data[0] + y*rhs.data[3] + z*rhs.data[6],
				 x*rhs.data[1] + y*rhs.data[4] + z*rhs.data[7],
				 x*rhs.data[2] + y*rhs.data[5] + z*rhs.data[8]);
}

inline vec3 vec3::
operator *  (const mat4& rhs) const {
    return vec3(x*rhs.data[0] + y*rhs.data[4] + z*rhs.data[8]  + rhs.data[12],
				x*rhs.data[1] + y*rhs.data[5] + z*rhs.data[9]  + rhs.data[13],
				x*rhs.data[2] + y*rhs.data[6] + z*rhs.data[10] + rhs.data[14]);
}

inline void vec3::
operator *= (const mat4& rhs) {
    *this = vec3(x*rhs.data[0] + y*rhs.data[4] + z*rhs.data[8]  + rhs.data[12],
				 x*rhs.data[1] + y*rhs.data[5] + z*rhs.data[9]  + rhs.data[13],
				 x*rhs.data[2] + y*rhs.data[6] + z*rhs.data[10] + rhs.data[14]);
}

//// vec4 ////

inline vec4 vec4::
operator *  (const mat4& rhs) const {
    return vec4(x*rhs.data[0] + y*rhs.data[4] + z*rhs.data[8]  + w*rhs.data[12],
				x*rhs.data[1] + y*rhs.data[5] + z*rhs.data[9]  + w*rhs.data[13],
				x*rhs.data[2] + y*rhs.data[6] + z*rhs.data[10] + w*rhs.data[14],
				x*rhs.data[3] + y*rhs.data[7] + z*rhs.data[11] + w*rhs.data[15]);
}

inline void vec4::
operator *= (const mat4& rhs) {
    *this = vec4(x*rhs.data[0] + y*rhs.data[4] + z*rhs.data[8]  + w*rhs.data[12],
				 x*rhs.data[1] + y*rhs.data[5] + z*rhs.data[9]  + w*rhs.data[13],
				 x*rhs.data[2] + y*rhs.data[6] + z*rhs.data[10] + w*rhs.data[14],
				 x*rhs.data[3] + y*rhs.data[7] + z*rhs.data[11] + w*rhs.data[15]);
}

/////////////////////////////
//// affine interactions ////
/////////////////////////////

//// mat3 ////

inline vec3 mat3::
row(u32 row){
    Assert(row < 3, "mat3 subscript out of bounds");
    return vec3(&data[4*row]);
}

inline vec3 mat3::
col(u32 col){
    Assert(col < 3, "mat3 subscript out of bounds");
    return vec3(data[col], data[4+col], data[8+col]);
}

//returns a pre-multiplied X->Y->Z LH rotation transformation matrix based on input in degrees
inline mat3 mat3::
RotationMatrix(vec3 rotation) {
    rotation = RADIANS(rotation);
    float cX = cosf(rotation.x); float sX = sinf(rotation.x);
    float cY = cosf(rotation.y); float sY = sinf(rotation.y);
    float cZ = cosf(rotation.z); float sZ = sinf(rotation.z);
    float r00 = cZ*cY;            float r01 = cY*sZ;            float r02 = -sY;
    float r10 = cZ*sX*sY - cX*sZ; float r11 = cZ*cX + sX*sY*sZ; float r12 = sX*cY;
    float r20 = cZ*cX*sY + sX*sZ; float r21 = cX*sY*sZ - cZ*sX; float r22 = cX*cY;
    return mat3(r00, r01, r02,
				r10, r11, r12,
				r20, r21, r22);
}

//returns a scale matrix where (0,0) = scale.x, (1,1) = scale.y, (2,2) = scale.z
inline mat3 mat3::
ScaleMatrix(vec3 scale) {
    return mat3(scale.x, 0, 0,
				0, scale.y, 0,
				0, 0, scale.z);
}

//// mat4 ////

inline vec4 mat4::
row(u32 row){
    Assert(row < 4, "mat4 subscript out of bounds");
    return vec4(&data[4*row]);
}

inline vec4 mat4::
col(u32 col){
    Assert(col < 4, "mat4 subscript out of bounds");
    return vec4(data[col], data[4+col], data[8+col], data[12+col]);
}

inline vec3 mat4::
Translation(){
    return vec3(data[12], data[13], data[14]);
}

//returns a pre-multiplied X->Y->Z LH rotation transformation matrix based on input in degrees
inline mat4 mat4::
RotationMatrix(vec3 rotation) {
    rotation = RADIANS(rotation);
    float cX = cosf(rotation.x); float sX = sinf(rotation.x);
    float cY = cosf(rotation.y); float sY = sinf(rotation.y);
    float cZ = cosf(rotation.z); float sZ = sinf(rotation.z);
    float r00 = cZ*cY;            float r01 = cY*sZ;            float r02 = -sY;
    float r10 = cZ*sX*sY - cX*sZ; float r11 = cZ*cX + sX*sY*sZ; float r12 = sX*cY;
    float r20 = cZ*cX*sY + sX*sZ; float r21 = cX*sY*sZ - cZ*sX; float r22 = cX*cY;
    return mat4(r00, r01, r02, 0,
				r10, r11, r12, 0,
				r20, r21, r22, 0,
				0,   0,   0,   1);
}

//https://github.com/microsoft/DirectXMath/blob/7c30ba5932e081ca4d64ba4abb8a8986a7444ec9/Inc/DirectXMathMatrix.inl
//line 1675 i do not know how this works but it does so :D
//return a matrix to rotate a vector around an arbitrary axis by some angle
//TODO(sushi, Ma) redo this function, I think its completely wrong around any axis thats not a world axis
inline mat4 mat4::
AxisAngleRotationMatrix(float angle, vec4 axis) {
    angle = RADIANS(angle); 
    float mag = axis.mag();
    axis = vec4(axis.x / mag, axis.y / mag, axis.z / mag, axis.w / mag);
    //axis.normalize();
    float c = cosf(angle); float s = sinf(angle); 
    
    vec4 A = vec4(s, c, 1 - c, 0);
    
    vec4 C2 = vec4(A.z, A.z, A.z, A.z);
    vec4 C1 = vec4(A.y, A.y, A.y, A.y);
    vec4 C0 = vec4(A.x, A.x, A.x, A.x);
    
    vec4 N0 = vec4(axis.y, axis.z, axis.x, axis.w);
    vec4 N1 = vec4(axis.z, axis.x, axis.y, axis.w);
    
    vec4 V0 = C2 * N0;
    V0 *= N1;
    
    vec4 R0 = C2 * axis;
    R0 = R0 * axis + C1;
    
    vec4 R1 = C0 * axis + V0;
    vec4 R2 = (V0 - C0) * axis;
    
    V0 = vec4(R0.x, R0.y, R0.z, A.w);
    vec4 V1 = vec4(R1.z, R2.y, R2.z, R1.x);
    vec4 V2 = vec4(R1.y, R2.x, R1.y, R2.x);
    
    return mat4(V0.x, V1.x, V1.y, V0.w,
				V1.z, V0.y, V1.w, V0.w,
				V2.x, V2.y, V0.z, V0.w,
				0,    0,    0,    1);
}

//returns a transformation matrix of the combined translation, rotation, and scale matrices from input vectors
//rotates over the Y, then Z then X, ref: https://www.euclideanspace.com/maths/geometry/affine/aroundPoint/index.htm
inline mat4 mat4::RotationMatrixAroundPoint(vec3 pivot, vec3 rotation) {
    //pivot = -pivot; //gotta negate this for some reason :)
    rotation = RADIANS(rotation);
    float cX = cosf(rotation.x); float sX = sinf(rotation.x);
    float cY = cosf(rotation.y); float sY = sinf(rotation.y);
    float cZ = cosf(rotation.z); float sZ = sinf(rotation.z);
    float r00 = cZ*cY;            float r01 = cY*sZ;            float r02 = -sY;
    float r10 = cZ*sX*sY - cX*sZ; float r11 = cZ*cX + sX*sY*sZ; float r12 = sX*cY;
    float r20 = cZ*cX*sY + sX*sZ; float r21 = cX*sY*sZ - cZ*sX; float r22 = cX*cY;
    
    return mat4(r00, r01, r02, 0,
                r10, r11, r12, 0,
                r20, r21, r22, 0,
                pivot.x - r00*pivot.x - r01*pivot.y - r02*pivot.z, pivot.y - r10*pivot.x - r11*pivot.y - r12*pivot.z, pivot.z - r20*pivot.x - r21*pivot.y - r22*pivot.z, 1);
}

//returns a translation matrix where (0,3) = translation.x, (1,3) = translation.y, (2,3) = translation.z
inline mat4 mat4::
TranslationMatrix(vec3 translation) {
    return mat4::TranslationMatrix(translation.x, translation.y, translation.z);
}

//returns a scale matrix where (0,0) = scale.x, (1,1) = scale.y, (2,2) = scale.z
inline mat4 mat4::
ScaleMatrix(vec3 scale) {
    return mat4::ScaleMatrix(scale.x, scale.y, scale.z);
}

//returns a transformation matrix of the combined translation, rotation, and scale matrices from input vectors
inline mat4 mat4::
TransformationMatrix(vec3 tr, vec3 rot, vec3 scale) {
    rot = RADIANS(rot);
    float cX = cosf(rot.x); float sX = sinf(rot.x);
    float cY = cosf(rot.y); float sY = sinf(rot.y);
    float cZ = cosf(rot.z); float sZ = sinf(rot.z);
    float r00 = cZ*cY;            float r01 = cY*sZ;            float r02 = -sY;
    float r10 = cZ*sX*sY - cX*sZ; float r11 = cZ*cX + sX*sY*sZ; float r12 = sX*cY;
    float r20 = cZ*cX*sY + sX*sZ; float r21 = cX*sY*sZ - cZ*sX; float r22 = cX*cY;
    return mat4(scale.x*r00, scale.x*r01, scale.x*r02, 0,
                scale.y*r10, scale.y*r11, scale.y*r12, 0,
                scale.z*r20, scale.z*r21, scale.z*r22, 0,
				tr.x,        tr.y,        tr.z, 1);
}

//returns euler angles from a rotation matrix
//TODO(sushi, Ma) confirm that this works at some point
inline vec3 mat4::
Rotation() {
    if ((*this)(0, 2) < 1) {
        if ((*this)(0, 2) > -1) {
            return -vec3(DEGREES(atan2(-(*this)(1, 2), (*this)(2, 2))), DEGREES(asin((*this)(0, 2))), DEGREES(atan2(-(*this)(0,1), (*this)(0,0))));
        } else {
            return -vec3(DEGREES(-atan2((*this)(1, 0), (*this)(1, 1))), DEGREES(-M_PI / 2), 0);
        }
    } else {
        return -vec3(DEGREES(atan2((*this)(1, 0), (*this)(1, 1))),  DEGREES(M_PI / 2) , 0);
    }
    
    
}
#endif //DESHI_VECTORMATRIX_H
