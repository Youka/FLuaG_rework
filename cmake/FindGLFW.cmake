find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h)
find_library(GLFW_LIBRARY NAMES GLFW glfw3 glfw PATH_SUFFIXES lib64)

set(GLFW_INCLUDE_DIRS ${GLFW_INCLUDE_DIR})
set(GLFW_LIBRARIES ${GLFW_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLFW
								REQUIRED_VARS GLFW_INCLUDE_DIR GLFW_LIBRARY)

mark_as_advanced(GLFW_INCLUDE_DIR GLFW_LIBRARY)