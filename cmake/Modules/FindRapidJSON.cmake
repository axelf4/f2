# - Find rapidjson headers.
# This module defines RAPIDJSON_INCLUDE_DIRS, the directory containing headers

find_path(RAPIDJSON_INCLUDE_DIRS rapidjson/rapidjson.h
	HINTS ${CMAKE_SOURCE_DIR}/lib/rapidjson/include/)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RapidJSON DEFAULT_MSG RAPIDJSON_INCLUDE_DIRS)

if (RAPIDJSON_INCLUDE_DIRS)
  set(RAPIDJSON_FOUND TRUE)
else ()
  set(RAPIDJSON_FOUND FALSE)
endif ()