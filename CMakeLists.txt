cmake_minimum_required (VERSION 2.8)
project(f2 C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(SOURCES
	include/vmath.h
	include/glh.h
	src/glh.c
	include/net.h
	src/net.c
	include/objloader.h
	src/objloader.c
	include/glTFloader.h
	src/glTFloader.cpp
	include/timer.h
	src/timer.c
	include/filesystemwatch.h
	src/filesystemwatch.c)

find_package(OpenGL REQUIRED)
# find_package(OpenCL REQUIRED) ${OPENCL_INCLUDE_DIRS} ${OPENCL_LIBRARIES}
find_package(GLEW REQUIRED)

add_library (f2 STATIC ${SOURCES})
include_directories(include ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D__SSE__ -D__SSE2__ -D__SSE3__ -D__SSE4_1__ -D__SSE4_2__)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:SSE2")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
else()
	if(CMAKE_COMPILER_IS_GNUCC OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -msse -msse2 -msse3 -msse4.1 -msse4.2")
	endif()
	if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -msse -msse2 -msse3 -msse4.1 -msse4.2")
	endif()
endif()

option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
	if(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_COMPILER_IS_GNUCC)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
	endif()

	# This must be set in the top-level CMakeLists.txt for the tests to be run by 'make test' or ctest.
	# enable_testing()
	# add_custom_target(check COMMAND ctest -V)

	set(gtest_force_shared_crt ON)
	set(GTEST_ROOT "${CMAKE_CURRENT_LIST_DIR}/lib/gtest")
	add_subdirectory(${GTEST_ROOT})
	# find_package(GTest REQUIRED)
	# include_directories(${GTEST_INCLUDE_DIRS})
	include_directories(${gtest_SOURCE_DIR}/include)

	add_executable(runTests test/test_vec4.cpp test/test_graphics.cpp test/test_objloader.cpp)
	# target_link_libraries(runTests ${GTEST_BOTH_LIBRARIES} pthread ${LIBRARIES})
	target_link_libraries(runTests gtest gtest_main ${OPENGL_LIBRARIES} ${GLEW_LIBRARY})
	# add_test(NAME runTests COMMAND runTests)
endif()
