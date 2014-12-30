cmake_minimum_required (VERSION 2.8)
project (game)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#i f(${_MACHINE_ARCH_FLAG} MATCHES X86)
#SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
#SET (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
#SET (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")

set(SOURCES
	src/f1.cpp
	src/glh.c
	src/vbo.cpp

	include/f1.h
	include/glh.h
	include/vbo.h
	include/obj_loader.h
	include/vmath.h)

set(TEST_SOURCES
	test/main.cpp
	test/shaders.h
	test/GLDebugDrawer.cpp
	test/GLDebugDrawer.h
	test/EntitySystem.cpp
	test/EntitySystem.h
	test/opencltest.cpp)

add_library (f1 STATIC ${SOURCES})
add_executable (game ${TEST_SOURCES})

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenCL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SOIL REQUIRED)
find_package(Bullet REQUIRED)
find_package(Assimp REQUIRED)

include_directories(include ${SDL2_INCLUDE_DIR} ${OPENCL_INCLUDE_DIRS} ${GLEW_INCLUDE_DIR} ${SOIL_INCLUDE_DIRS} ${BULLET_INCLUDE_DIRS} ${ASSIMP_INCLUDE_DIR})
target_link_libraries(game f1 ${SDL2_LIBRARY} ${OPENGL_LIBRARIES} ${OPENCL_LIBRARIES} ${GLEW_LIBRARY} ${SOIL_LIBRARIES} ${BULLET_LIBRARIES} ${ASSIMP_LIBRARIES})

add_definitions(-D_CRT_SECURE_NO_WARNINGS -D__SSE__ -D__SSE2__ - D__SSE3__ -D__SSE4_1__ -D__SSE4_2__ -DRESOURCE_DIR="../resource/")

if (MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:SSE2")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
else
	if (CMAKE_COMPILER_IS_GNUCC)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse -msse2 -msse3 -msse4.1 -msse4.2")
	endif
	if (CMAKE_COMPILER_IS_GNUCXX)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2 -msse3 -msse4.1 -msse4.2")
	endif
endif

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

# cmake -G "Visual Studio 12 2013" ..
