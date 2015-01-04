enable_testing()

add_subdirectory(gtest-1.7.0)
# Include the gtest library. gtest_SOURCE_DIR is available due to 'project(gtest)' above.
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(tests tests.cpp)
target_link_libraries(tests gtest gtest_main ${LIBRARIES})

add_test(tests tests)