cmake_minimum_required (VERSION 2.8)
project(f2 C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(SOURCES
	include/glh.h src/glh.c
	include/net.h src/net.c
	include/bitmap_dds.h src/bitmap_dds.c
	include/timer.h src/timer.c
	include/FileSystemWatcher.h src/FileSystemWatcher.c
	include/gridlayout.h src/gridlayout.c
	include/bmfont.h src/bmfont.c)

find_package(OpenGL REQUIRED)
# find_package(OpenCL REQUIRED) ${OPENCL_INCLUDE_DIRS} ${OPENCL_LIBRARIES}
find_package(GLEW REQUIRED)

add_library (f2 STATIC ${SOURCES})
include_directories(include ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})
