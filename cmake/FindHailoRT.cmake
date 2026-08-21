# FindHailoRT.cmake — locate the HailoRT runtime.
#
# HailoRT does not ship a guaranteed official CMake package config across
# versions, so this module locates the header and library by hand. It is used
# only on the target for the perception service's Hailo path (currently blocked
# pending on-target model post-process). Off-target builds do not require it.
#
# Sets: HailoRT_FOUND, HailoRT_INCLUDE_DIRS, HailoRT_LIBRARIES
# Imported target (if found): HailoRT::HailoRT

find_path(HailoRT_INCLUDE_DIR
  NAMES hailo/hailort.h
  PATHS /usr/include /usr/local/include
)

find_library(HailoRT_LIBRARY
  NAMES hailort
  PATHS /usr/lib /usr/local/lib /usr/lib/aarch64-linux-gnu
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HailoRT
  REQUIRED_VARS HailoRT_LIBRARY HailoRT_INCLUDE_DIR
)

if(HailoRT_FOUND)
  set(HailoRT_INCLUDE_DIRS ${HailoRT_INCLUDE_DIR})
  set(HailoRT_LIBRARIES ${HailoRT_LIBRARY})
  if(NOT TARGET HailoRT::HailoRT)
    add_library(HailoRT::HailoRT UNKNOWN IMPORTED)
    set_target_properties(HailoRT::HailoRT PROPERTIES
      IMPORTED_LOCATION "${HailoRT_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${HailoRT_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(HailoRT_INCLUDE_DIR HailoRT_LIBRARY)
