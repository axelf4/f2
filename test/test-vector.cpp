#include <gtest/gtest.h>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <vmath.h>

namespace {

	float frand() {
		return static_cast <float> (rand());
	}

	void vectorCompare(VEC vector, float x, float y, float z, float w) {
		float values[4];
		VectorGet(values, vector);
		EXPECT_FLOAT_EQ(x, values[0]);
		EXPECT_FLOAT_EQ(y, values[1]);
		EXPECT_FLOAT_EQ(z, values[2]);
		EXPECT_FLOAT_EQ(w, values[3]);
	}
	
	void vectorCompareNear(VEC vector, float x, float y, float z, float w, float abs_error) {
		float values[4];
		VectorGet(values, vector);
		EXPECT_NEAR(x, values[0], abs_error);
		EXPECT_NEAR(y, values[1], abs_error);
		EXPECT_NEAR(z, values[2], abs_error);
		EXPECT_NEAR(w, values[3], abs_error);
	}

	TEST(Vector, SettingAndGetting) {
		srand (static_cast <unsigned> (time(0)));
		float x = frand(), y = frand(), z = frand(), w = frand();
		VEC vector = VectorSet(x, y, z, w);
		vectorCompare(vector, x, y, z, w);
	}
	
	TEST(Vector, Replicating) {
		float f = frand();
		VEC vector = VectorReplicate(f);
		vectorCompare(vector, f, f, f, f);
	}
	
	TEST(Vector, Loading) {
		float v[] = { frand(), frand(), frand(), frand() };
		VEC vector = VectorLoad(v);
		vectorCompare(vector, v[0], v[1], v[2], v[3]);
	}
	
	TEST(Vector, Arithmetic) {
		float x1 = frand(), y1 = frand(), z1 = frand(), w1 = frand(), x2 = frand(), y2 = frand(), z2 = frand(), w2 = frand();
		VEC v1 = VectorSet(x1, y1, z1, w1), v2 = VectorSet(x2, y2, z2, w2);
		vectorCompare(VectorAdd(v1, v2), x1 + x2, y1 + y2, z1 + z2, w1 + w2);
		vectorCompare(VectorSubtract(v1, v2), x1 - x2, y1 - y2, z1 - z2, w1 - w2);
		vectorCompare(VectorMultiply(v1, v2), x1 * x2, y1 * y2, z1 * z2, w1 * w2);
		vectorCompare(VectorDivide(v1, v2), x1 / x2, y1 / y2, z1 / z2, w1 / w2);
	}
	
	TEST(Vector, Length) {
		VEC vector = VectorSet(3, 4, 6, 0);
		EXPECT_FLOAT_EQ(std::sqrt(61), VectorLength(vector)) << "The vector's length doesn't equal the square root of 61";
	}
	
	TEST(Vector, LengthAndNormalization) {
		float x = frand() + 0.1, y = frand() + 0.1, z = frand() + 0.1, w = frand() + 0.1;
		VEC vector = VectorSet(x, y, z, w);
		float length = (float) sqrt(x * x + y * y + z * z), tmp;
		EXPECT_NEAR(length, tmp = VectorLength(vector), 1.f) << "The length of the vector [" << x1 << ", " << y1 << ", " << z1 << "] isn't " << length;
		length = tmp;
		
		vectorCompareNear(VectorNormalize(vector), x / length, y / length, z / length, w / length, 1.f);
	}
	
	TEST(Vector, DotAndCrossProducts) {
		float x1 = frand(), y1 = frand(), z1 = frand(), w1 = frand(), x2 = frand(), y2 = frand(), z2 = frand(), w2 = frand();
		VEC v1 = VectorSet(x1, y1, z1, w1), v2 = VectorSet(x2, y2, z2, w2);
		float dot = x1 * x2 + y1 * y2 + z1 * z2;
		EXPECT_NEAR(dot, VectorDot(v1, v2), 1.f) << "The dot product of the two vectors [" << x1 << ", " << y1 << ", " << z1 << "] and [" << x2 << ", " << y2 << ", " << z2 << "] doesn't equal " << dot;
		
		vectorCompareNear(VectorCross(v1, v2), y1 * z2 - z1 * y2,
			z1 * x2 - x1 * z2,
			x1 * y2 - y1 * x2,
			0, 1.f);
	}
	
	TEST(Vector, Comparison) {
		float x = frand(), y = frand(), z = frand(), w = frand();
		VEC v1 = VectorSet(x, y, z, w), v2 = VectorSet(x, y, z, w), v3 = VectorSet(-x, -y, -z, -w);
		EXPECT_NE(0, VectorEqual(v1, v2)) << "Vectors [" << x << ", " << y << ", " << z << ", " << w << "] and [" << x << ", " << y << ", " << z << ", " << w << "] differ";
		EXPECT_EQ(0, VectorEqual(v1, v3)) << "Vectors [" << x << ", " << y << ", " << z << ", " << w << "] and [" << -x << ", " << -y << ", " << -z << ", " << -w << "] do not differ";
	}

}