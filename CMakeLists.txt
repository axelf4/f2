cmake_minimum_required (VERSION 2.8)
project (game)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#i f(${_MACHINE_ARCH_FLAG} MATCHES X86)
#SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
#SET (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
#SET (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")

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
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SOIL REQUIRED)
find_package(GLM REQUIRED)
find_package(Bullet REQUIRED)
find_package(Assimp REQUIRED)

include_directories(include ${SDL2_INCLUDE_DIR} ${GLEW_INCLUDE_DIR} ${SOIL_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS} ${BULLET_INCLUDE_DIRS} ${ASSIMP_INCLUDE_DIR})
target_link_libraries(game f1 ${SDL2_LIBRARY} ${OPENGL_LIBRARIES} ${GLEW_LIBRARY} ${SOIL_LIBRARIES} ${BULLET_LIBRARIES} ${ASSIMP_LIBRARIES})

# cmake -G "Visual Studio 12 2013" ..