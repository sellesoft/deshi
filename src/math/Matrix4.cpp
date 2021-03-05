#include "Matrix4.h"

//// Constructors ////

Matrix4::Matrix4(float all) {
	data[0] = all;	data[1] = all;	data[2] = all;	data[3] = all;
	data[4] = all;	data[5] = all;	data[6] = all;	data[7] = all;
	data[8] = all;	data[9] = all;	data[10] = all;   data[11] = all;
	data[12] = all;   data[13] = all;   data[14] = all;   data[15] = all;
}

Matrix4::Matrix4(float _00, float _01, float _02, float _03,
				 float _10, float _11, float _12, float _13,
				 float _20, float _21, float _22, float _23,
				 float _30, float _31, float _32, float _33) {
	data[0] = _00;	data[1] = _01;	data[2] = _02;	data[3] = _03;
	data[4] = _10;	data[5] = _11;	data[6] = _12;	data[7] = _13;
	data[8] = _20;	data[9] = _21;	data[10] = _22; data[11] = _23;
	data[12] = _30; data[13] = _31; data[14] = _32; data[15] = _33;
}

Matrix4::Matrix4(const Matrix4& m) {
	memcpy(&data, &m.data, 16*sizeof(float));
}



//// Static Constants ////

const Matrix4 Matrix4::IDENTITY = Matrix4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
