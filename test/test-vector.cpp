#include <vmath.h>

namespace {

	TEST(Vector, Getting) {
		VEC vector = VectorSet(10, 3, 5, 0);
		float values[4];
		VectorGet(values, vector);
		EXPECT_FLOAT_EQ(10, values[0]);
		EXPECT_FLOAT_EQ(3, values[1]);
		EXPECT_FLOAT_EQ(5, values[2]);
		EXPECT_FLOAT_EQ(0, values[3]);
	}

}