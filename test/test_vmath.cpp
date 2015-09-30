#include <gtest/gtest.h>
#include <cstdlib>
#include <cmath>
#include <ctime>
// #define NO_SIMD_INTRINSICS
#include <vmath.h>
#include <iostream>

namespace {

	TEST(Vector, Equal_SameComponents_True) {
		EXPECT_TRUE(VectorEqual(VectorSet(0, 0, 0, 0), VectorSet(0, 0, 0, 0)) == 0xF) << "The two same vectors are reported as different.";
	}

	TEST(Vector, Equal_DifferentComponents_False) {
		EXPECT_FALSE(VectorEqual(VectorSet(0, 0, 0, 0), VectorSet(0, 1, 1, 1)) == 0xF) << "The two different vectors are reported as same.";
	}

	TEST(Vector, Replicate_simpleValue_Equals) {
		EXPECT_TRUE(VectorEqual(VectorReplicate(1), VectorSet(1, 1, 1, 1)) == 0xF);
	}

	TEST(Vector, Load_FloatArray_Equals) {
		ALIGN(16) float v[] = { 1, 2, 3, 4 };
		EXPECT_TRUE(VectorEqual(VectorLoad(v), VectorSet(1, 2, 3, 4)) == 0xF);
	}

	TEST(Vector, Arithmetic) {
		{
			VECTOR B = VectorSet(1 + 4, 2 + 3, 3 + 2, 4 + 1);
			EXPECT_TRUE(VectorEqual(B, VectorAdd(VectorSet(1, 2, 3, 4), VectorSet(4, 3, 2, 1))) == 0xF) << "The two added vectors does not equal the result.";
		}
		{
			VECTOR B = VectorSet(1 - 4, 2 - 3, 3 - 2, 4 - 1);
			EXPECT_TRUE(VectorEqual(B, VectorSubtract(VectorSet(1, 2, 3, 4), VectorSet(4, 3, 2, 1))) == 0xF) << "The two subtracted vectors does not equal the result.";
		}
		{
			VECTOR B = VectorSet(1 * 4, 2 * 3, 3 * 2, 4 * 1);
			EXPECT_TRUE(VectorEqual(B, VectorMultiply(VectorSet(1, 2, 3, 4), VectorSet(4, 3, 2, 1))) == 0xF) << "The two multiplied vectors does not equal the result.";
		}
		{
			VECTOR B = VectorSet(1.F / 4, 2.F / 3, 3.F / 2, 4.F / 1);
			EXPECT_TRUE(VectorEqual(B, VectorDivide(VectorSet(1, 2, 3, 4), VectorSet(4, 3, 2, 1))) == 0xF) << "The two divided vectors does not equal the result.";
		}
	}

	TEST(Vector, Length_simpleValues_Near) {
		float x = 1, y = 2, z = 3, w = 4;
		EXPECT_NEAR(sqrt(x * x + y * y + z * z), VectorLength(VectorSet(x, y, z, w)), 1.f) << "The length of the vector [" << x << ", " << y << ", " << z << ", " << w << "] isn't close to correct.";
	}

	TEST(Vector, Dot_simpleValues_Near) {
		float x1 = 1, y1 = 2, z1 = 3, w1 = 4, x2 = 5, y2 = 6, z2 = 7, w2 = 8;
		VECTOR v1 = VectorSet(x1, y1, z1, w1), v2 = VectorSet(x2, y2, z2, w2);
		float dot = x1 * x2 + y1 * y2 + z1 * z2;
		EXPECT_NEAR(dot, VectorDot(v1, v2), 1.f) << "The dot product of the two vectors [" << x1 << ", " << y1 << ", " << z1 << "] and [" << x2 << ", " << y2 << ", " << z2 << "] doesn't equal " << dot;
	}

	TEST(Matrix, Equal_SameComponents_True) {
		MATRIX m1 = MatrixSet(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), m2 = MatrixSet(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
		EXPECT_TRUE(MatrixEqual(&m1, &m2) == 0xFFFF);
	}

	TEST(Matrix, Equal_DifferentComponents_False) {
		MATRIX m1 = MatrixSet(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), m2 = MatrixSet(16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1);
		EXPECT_FALSE(MatrixEqual(&m1, &m2) == 0xFFFF);
	}

	TEST(Matrix, Arithmetic) {
		MATRIX m1 = MatrixSet(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), m2 = MatrixSet(16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1);
		{
			MATRIX A = MatrixMultiply(&m1, &m2), B = MatrixSet(80, 70, 60, 50, 240, 214, 188, 162, 400, 358, 316, 274, 560, 502, 444, 386);
			EXPECT_TRUE(MatrixEqual(&A, &B) == 0xFFFF);
		}
	}

	/*TEST(Matrix, Inverse_simpleValues_Equal) {
		MATRIX a = MatrixSet(4, 0, 0, 0, 0, 0, 2, 0, 0, 1, 2, 0, 1, 0, 0, 1), b = MatrixSet(.25, 0, 0, 0, 0, -1, 1, 0, 0, .5, 0, 0, -.25, 0, 0, 1),
			inverse = MatrixInverse(&a);
		EXPECT_TRUE(MatrixEqual(&inverse, &b) == 0xFFFF);
	}*/


}
