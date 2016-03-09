if(NOT GLFW_FIND_VERSION)
	set(GLFW_FIND_VERSION 3)
endif()

find_path(GLFW_INCLUDE_DIR GLFW/glfw${GLFW_FIND_VERSION}.h GLFW/glfw.h)
find_library(GLFW_LIBRARY NAMES GLFW glfw${GLFW_FIND_VERSION} glfw PATH_SUFFIXES lib64)

set(GLFW_INCLUDE_DIRS ${GLFW_INCLUDE_DIR})
set(GLFW_LIBRARIES ${GLFW_LIBRARY})

if(NOT WIN32)
	include(FindX11)
	set(GLFW_LIBRARIES ${GLFW_LIBRARIES} ${X11_X11_LIB} ${X11_Xrandr_LIB} ${X11_Xinerama_LIB} ${X11_Xxf86vm_LIB} ${X11_Xcursor_LIB})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLFW
				REQUIRED_VARS GLFW_INCLUDE_DIR GLFW_LIBRARY)

mark_as_advanced(GLFW_INCLUDE_DIR GLFW_LIBRARY)
