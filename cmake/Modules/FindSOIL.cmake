# Find SOIL
# Find the SOIL includes and library
#
#  SOIL_INCLUDE_DIRS - where to find SOIL.h, etc.
#  SOIL_LIBRARIES    - List of libraries when using SOIL.
#  SOIL_FOUND        - True if SOIL found.
#

include( FindPackageHandleStandardArgs )
find_path( SOIL_INCLUDE_DIRS "SOIL.h"
	${SOIL_ROOT_DIR}/src )
	# PATH_SUFFIXES include/SOIL include )
find_library( SOIL_LIBRARIES
	NAMES SOIL
	HINTS ${SOIL_ROOT_DIR}/lib )
	#SOIL )
find_package_handle_standard_args(SOIL DEFAULT_MSG SOIL_LIBRARIES SOIL_INCLUDE_DIRS )
if ( SOIL_INCLUDE_DIRS AND SOIL_LIBRARIES )
	SET( SOIL_FOUND TRUE )
else ()
	SET( SOIL_FOUND FALSE )
endif ()