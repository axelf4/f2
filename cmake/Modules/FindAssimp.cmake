# - Try to find Assimp
# Once done, this will define
#
#  ASSIMP_FOUND - system has Assimp
#  ASSIMP_INCLUDE_DIR - the Assimp include directories
#  ASSIMP_LIBRARIES - link these to use Assimp

include( FindPackageHandleStandardArgs )

FIND_PATH( ASSIMP_INCLUDE_DIR assimp/mesh.h
	${ASSIMP_ROOT_DIR}/include
  /usr/include
  /usr/local/include
  /opt/local/include
)

FIND_LIBRARY( ASSIMP_LIBRARIES assimp
	${ASSIMP_ROOT_DIR}/lib32
  /usr/lib64
  /usr/lib
  /usr/local/lib
  /opt/local/lib
)

find_package_handle_standard_args(ASSIMP DEFAULT_MSG ASSIMP_LIBRARIES ASSIMP_INCLUDE_DIR )
IF(ASSIMP_INCLUDE_DIR AND ASSIMP_LIBRARIES)
  SET( ASSIMP_FOUND TRUE )
ENDIF(ASSIMP_INCLUDE_DIR AND ASSIMP_LIBRARIES)