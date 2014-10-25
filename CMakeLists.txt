cmake_minimum_required (VERSION 2.8)
project (game)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(SOURCES
	src/sqr.cpp
	include/sqr.h
	)

set(TEST_SOURCES
	src/main.cpp
	)

add_library (f1 STATIC ${SOURCES})
add_executable (game ${TEST_SOURCES})

find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(SOIL REQUIRED)

include_directories(include ${SDL2_INCLUDE_DIR} ${GLEW_INCLUDE_DIR} ${SOIL_INCLUDE_DIRS})
target_link_libraries(game f1 ${SDL2_LIBRARY} ${GLEW_LIBRARY} ${SOIL_LIBRARIES})

# cmake -G "Visual Studio 12 2013" ..