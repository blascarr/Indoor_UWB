#ifndef _INDOOR_UWB_MATH_TEST_UTILS_H
#define _INDOOR_UWB_MATH_TEST_UTILS_H

#include "../../src/models/Trilateration.h"

inline void setVec3(Vector3D &v, float x, float y, float z) {
	v(0, 0) = x;
	v(1, 0) = y;
	v(2, 0) = z;
}

inline float distance3(const Vector3D &a, const Vector3D &b) {
	return norm3(a - b);
}

#endif
