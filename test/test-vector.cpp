#include <gtest/gtest.h>
#include <stdlib.h>
#include <cmath>
#include <vmath.h>

namespace {

	void vectorCompare(VEC vector, float x, float y, float z, float w) {
		float values[4];
		VectorGet(values, vector);
		EXPECT_FLOAT_EQ(x, values[0]);
		EXPECT_FLOAT_EQ(y, values[1]);
		EXPECT_FLOAT_EQ(z, values[2]);
		EXPECT_FLOAT_EQ(w, values[3]);
	}

	TEST(Vector, SettingAndGetting) {
		float x = rand(), y = rand(), z = rand(), w = rand();
		VEC vector = VectorSet(x, y, z, w);
		vectorCompare(vector, x, y, z, w);
	}
	
	TEST(Vector, Replicating) {
		float f = rand();
		VEC vector = VectorReplicate(f);
		vectorCompare(vector, f, f, f, f);
	}
	
	TEST(Vector, Loading) {
		float v[] = { rand(), rand(), rand(), rand() };
		VEC vector = VectorLoad(v);
		vectorCompare(vector, v[0], v[1], v[2], v[3]);
	}
	
	TEST(Vector, Arithmetic) {
		float x1, y1, z1, w1, x2, y2, z2, w2;
		x1 = rand();
		y1 = rand();
		z1 = rand();
		w1 = rand();
		x2 = rand();
		y2 = rand();
		z2 = rand();
		w2 = rand();
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
	
	TEST(Vector, Comparison) {
		float x = rand(), y = rand(), z = rand(), w = rand();
		VEC v1 = VectorSet(x, y, z, w), v2 = VectorSet(x, y, z, w), v3 = VectorSet(-x, -y, -z, -w);
		EXPECT_NE(0, VectorEqual(v1, v2)) << "Vectors [" << x << ", " << y << ", " << z << ", " << w << "] and [" << x << ", " << y << ", " << z << ", " << w << "] differ";
		EXPECT_EQ(0, VectorEqual(v1, v3)) << "Vectors [" << x << ", " << y << ", " << z << ", " << w << "] and [" << -x << ", " << -y << ", " << -z << ", " << -w << "] do not differ";
	}

}