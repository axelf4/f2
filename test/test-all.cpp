#include <gtest/gtest.h>
#include <iostream>

#include "test-vector.cpp"

using namespace std;

int main(int argc, char **argv) {
	cout << "main" << endl;
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}