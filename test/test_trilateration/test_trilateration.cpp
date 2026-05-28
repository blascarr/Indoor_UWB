#include <Arduino.h>
#include <math.h>
#include <unity.h>

#include "../helpers/math_test_utils.h"

static void test_norm3_unit_axes() {
	Vector3D v;
	setVec3(v, 3.f, 4.f, 0.f);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.f, norm3(v));
}

static void test_trilateration_recovers_interior_point() {
	Vector3D P[4];
	setVec3(P[0], 0.f, 0.f, 0.f);
	setVec3(P[1], 5.f, 0.f, 0.f);
	setVec3(P[2], 0.f, 5.f, 0.f);
	setVec3(P[3], 0.f, 0.f, 5.f);

	Vector3D truth;
	setVec3(truth, 2.f, 3.f, 1.f);

	float d[4];
	d[0] = distance3(truth, P[0]);
	d[1] = distance3(truth, P[1]);
	d[2] = distance3(truth, P[2]);
	d[3] = distance3(truth, P[3]);

	Vector3D result = trilateration3D(P, d);

	TEST_ASSERT_FLOAT_WITHIN(0.2f, 2.f, result(0, 0));
	TEST_ASSERT_FLOAT_WITHIN(0.2f, 3.f, result(1, 0));
	TEST_ASSERT_FLOAT_WITHIN(0.2f, 1.f, result(2, 0));
}

static void test_trilateration_on_anchor_plane() {
	Vector3D P[4];
	setVec3(P[0], 0.f, 0.f, 0.f);
	setVec3(P[1], 4.f, 0.f, 0.f);
	setVec3(P[2], 2.f, 4.f, 0.f);
	setVec3(P[3], 2.f, 2.f, 3.f);

	Vector3D truth;
	setVec3(truth, 2.f, 2.f, 0.f);

	float d[4];
	d[0] = distance3(truth, P[0]);
	d[1] = distance3(truth, P[1]);
	d[2] = distance3(truth, P[2]);
	d[3] = distance3(truth, P[3]);

	Vector3D result = trilateration3D(P, d);

	TEST_ASSERT_FLOAT_WITHIN(0.25f, 2.f, result(0, 0));
	TEST_ASSERT_FLOAT_WITHIN(0.25f, 2.f, result(1, 0));
	TEST_ASSERT_FLOAT_WITHIN(0.35f, 0.f, result(2, 0));
}

static void test_distances_are_consistent_with_solution() {
	Vector3D P[4];
	setVec3(P[0], 1.f, 0.f, 0.f);
	setVec3(P[1], 6.f, 0.f, 0.f);
	setVec3(P[2], 1.f, 5.f, 0.f);
	setVec3(P[3], 1.f, 0.f, 4.f);

	Vector3D truth;
	setVec3(truth, 3.f, 2.f, 1.5f);

	float d[4];
	d[0] = distance3(truth, P[0]);
	d[1] = distance3(truth, P[1]);
	d[2] = distance3(truth, P[2]);
	d[3] = distance3(truth, P[3]);

	Vector3D result = trilateration3D(P, d);

	TEST_ASSERT_FLOAT_WITHIN(0.25f, d[0], distance3(result, P[0]));
	TEST_ASSERT_FLOAT_WITHIN(0.25f, d[1], distance3(result, P[1]));
	TEST_ASSERT_FLOAT_WITHIN(0.25f, d[2], distance3(result, P[2]));
	TEST_ASSERT_FLOAT_WITHIN(0.25f, d[3], distance3(result, P[3]));
}

void setup() {
	delay(200);
	UNITY_BEGIN();
	RUN_TEST(test_norm3_unit_axes);
	RUN_TEST(test_trilateration_recovers_interior_point);
	RUN_TEST(test_trilateration_on_anchor_plane);
	RUN_TEST(test_distances_are_consistent_with_solution);
	UNITY_END();
}

void loop() {}
