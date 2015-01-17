cmake_minimum_required (VERSION 2.8)
project(game C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#i f(${_MACHINE_ARCH_FLAG} MATCHES X86)
#SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
#SET (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
#SET (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")

set(SOURCES
	src/glh.c
	src/objloader.c
	src/vbo.cpp

	include/glh.h
	include/vmath.h
	include/objloader.h
	include/vbo.h)

set(DEMO_SOURCES
	demo/main.cpp
	demo/shaders.h
	demo/GLDebugDrawer.cpp
	demo/GLDebugDrawer.h)

if(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

add_library (f1 STATIC ${SOURCES})
add_executable (game ${DEMO_SOURCES})

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
# find_package(OpenCL REQUIRED) ${OPENCL_INCLUDE_DIRS} ${OPENCL_LIBRARIES}
find_package(GLEW REQUIRED)
find_package(SOIL REQUIRED)
find_package(Bullet REQUIRED)
find_package(Assimp REQUIRED)

include_directories(include ${OPENGL_INCLUDE_DIR} ${SDL2_INCLUDE_DIR} ${GLEW_INCLUDE_DIR} ${SOIL_INCLUDE_DIRS} ${BULLET_INCLUDE_DIRS} ${ASSIMP_INCLUDE_DIR})
set(LIBRARIES f1 ${SDL2_LIBRARY} ${GLEW_LIBRARY} ${SOIL_LIBRARIES} ${BULLET_LIBRARIES} ${ASSIMP_LIBRARIES} ${OPENGL_LIBRARIES})
target_link_libraries(game ${LIBRARIES})

add_definitions(-DRESOURCE_DIR="../resource/")

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D__SSE2__ -D__SSE3__ -D__SSE4_1__ -D__SSE4_2__)
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

# if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
# 	# list of files for which we add a copy rule
# 	file(GLOB_RECURSE RESOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/resource resource/*)
#
# 	foreach(item IN LISTS RESOURCES)
# 		# message(STATUS ${item})
# 		add_custom_command(
# 			TARGET game POST_BUILD
# 			COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/resource/${item}" "${CMAKE_CURRENT_BINARY_DIR}/${item}")
# 	endforeach()
# endif()

option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
	# This must be set in the top-level CMakeLists.txt for the tests to be run by 'make test' or ctest.
	# enable_testing()
	# add_custom_target(check COMMAND ctest -V)

	find_package(GTest REQUIRED)
	include_directories(${GTEST_INCLUDE_DIRS})

	add_executable(runTests test/test_vector.cpp test/test_graphics.cpp test/test_objloader.cpp)
	target_link_libraries(runTests ${GTEST_BOTH_LIBRARIES} pthread ${LIBRARIES})
	# add_test(NAME runTests COMMAND runTests)
endif()


# cmake -G "Visual Studio 12 2013" ..
