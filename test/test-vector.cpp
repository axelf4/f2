#include <gtest/gtest.h>
#include <stdlib.h>
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

	TEST(Vector, Getting) {
		VEC vector = VectorSet(10, 3, 5, 0);
		vectorCompare(10, 3, 5, 0);
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

}