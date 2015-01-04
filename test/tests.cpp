#include <gtest/gtest.h>
#include <iostream>

using namespace std;

	TEST(SampleTest, AssertionTrue) {
		cout << "test" << endl;
		ASSERT_EQ(1, 1);
	}
	
	int main(int argc, char **argv) {
		cout << "main" << endl;
		::testing::InitGoogleTest(&argc, argv);
		return RUN_ALL_TESTS();
	}