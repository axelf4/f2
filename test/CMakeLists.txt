message("test cmake lists")

add_executable(runUnitTests tests.cpp)
add_test(runUnitTests runUnitTests)

add_subdirectory(gtest-1.7.0)
# Include the gtest library. gtest_SOURCE_DIR is available due to 'project(gtest)' above.
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

target_link_libraries(runUnitTests gtest gtest_main ${LIBRARIES})