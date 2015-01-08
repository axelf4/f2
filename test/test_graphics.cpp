#include <gtest/gtest.h>
#include <glh.h>

TEST(Graphics, CalculatingStride) {
	struct attrib attributes[] = { { 0, 3, 0 }, { 0, 2, 0} };
	GLsizei stride = calculate_stride(attributes);
	EXPECT_EQ(20, stride);
}