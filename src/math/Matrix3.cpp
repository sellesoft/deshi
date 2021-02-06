#include "dsh_Matrix3.h"

//// Constructors ////

Matrix3::Matrix3(float _00, float _01, float _02,
				float _10, float _11, float _12,
				float _20, float _21, float _22) {
	data[0] = _00; data[1] = _01; data[2] = _02;
	data[3] = _10; data[4] = _11; data[5] = _12;
	data[6] = _20; data[7] = _21; data[8] = _22;
}

Matrix3::Matrix3(const Matrix3& m) {
	memcpy(&data, &m.data, 9*sizeof(float));
}




//// Static Constants ////

const Matrix3 Matrix3::IDENTITY = Matrix3(1,0,0,0,1,0,0,0,1);
