#ifndef _INDOOR_UWB_TRILATERATION_H
#define _INDOOR_UWB_TRILATERATION_H

#include <BasicLinearAlgebra.h>
#include "../config.h"

using namespace BLA;

typedef Matrix<3, 1> Vector3D;

inline float norm3(const Vector3D &p) {
	return sqrt(pow(p(0, 0), 2) + pow(p(1, 0), 2) + pow(p(2, 0), 2));
}

inline float dot3(const Vector3D &p1, const Vector3D &p2) {
	return (p1(0, 0) * p2(0, 0) + p1(1, 0) * p2(1, 0) + p1(2, 0) * p2(2, 0));
}

inline Vector3D multiply3(Vector3D p, float n) {
	p(0, 0) *= n;
	p(1, 0) *= n;
	p(2, 0) *= n;
	return p;
}

inline Vector3D cross3(const Vector3D &p1, const Vector3D &p2) {
	Vector3D answer;
	answer(0, 0) = p1(1, 0) * p2(2, 0) - p1(2, 0) * p2(1, 0);
	answer(1, 0) = p1(2, 0) * p2(0, 0) - p1(0, 0) * p2(2, 0);
	answer(2, 0) = p1(0, 0) * p2(1, 0) - p1(1, 0) * p2(0, 0);
	return answer;
}

/** Trilateración 2D en plano XY (3 anchors). Z = media de los anchors. */
inline bool trilateration2D(Vector3D P[], float d[], Vector3D &out) {
	const float x1 = P[1](0, 0) - P[0](0, 0);
	const float y1 = P[1](1, 0) - P[0](1, 0);
	const float r1 = sqrt(x1 * x1 + y1 * y1);
	if (r1 < 1e-6f) {
		return false;
	}
	const float ex = x1 / r1;
	const float ey = y1 / r1;
	const float x2 = P[2](0, 0) - P[0](0, 0);
	const float y2 = P[2](1, 0) - P[0](1, 0);
	const float i = ex * x2 + ey * y2;
	const float j = -ey * x2 + ex * y2;
	if (fabs(j) < 1e-6f) {
		return false;
	}
	const float x =
		(d[0] * d[0] - d[1] * d[1] + r1 * r1) / (2.f * r1);
	const float y = (d[0] * d[0] - d[2] * d[2] + i * i + j * j) / (2.f * j) -
					i * x / j;
	out(0, 0) = P[0](0, 0) + ex * x - ey * y;
	out(1, 0) = P[0](1, 0) + ey * x + ex * y;
	out(2, 0) = (P[0](2, 0) + P[1](2, 0) + P[2](2, 0)) / 3.f;
	return true;
}

inline float trilaterationResidual(const Vector3D &pos, Vector3D P[],
								   float d[], uint8_t count) {
	if (count == 0) {
		return -1.f;
	}
	float sum = 0.f;
	for (uint8_t k = 0; k < count; k++) {
		sum += fabs(norm3(pos - P[k]) - d[k]);
	}
	return sum / (float)count;
}

inline Vector3D trilateration3D(Vector3D P[], float d[]) {
	float t = norm3(P[1] - P[0]);
	Vector3D e_x = (P[1] - P[0]) / t;
	float i = dot3(e_x, P[2] - P[0]);
	Vector3D e_yy = P[2] - P[0] - multiply3(e_x, i);
	Vector3D e_y = e_yy / norm3(e_yy);
	Vector3D e_z = cross3(e_x, e_y);
	float j = dot3(e_y, P[2] - P[0]);
	float x = (pow(d[0], 2) - pow(d[1], 2) + pow(t, 2)) / (2 * t);
	float y = (pow(d[0], 2) - pow(d[2], 2) + pow(i, 2) + pow(j, 2)) / (2 * j) -
			  (x * i / j);
	float z1 = sqrt(abs(pow(d[0], 2) - pow(x, 2) - pow(y, 2)));
	float z2 = -z1;
	Vector3D ans1 =
		P[0] + multiply3(e_x, x) + multiply3(e_y, y) + multiply3(e_z, z1);
	Vector3D ans2 =
		P[0] + multiply3(e_x, x) + multiply3(e_y, y) + multiply3(e_z, z2);
	float dist1 = norm3(P[3] - ans1);
	float dist2 = norm3(P[3] - ans2);
	if (abs(d[3] - dist1) < abs(d[3] - dist2)) {
		return ans1;
	}
	return ans2;
}

#endif
