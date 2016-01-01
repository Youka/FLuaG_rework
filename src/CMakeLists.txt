# Include script helpers
include(${PROJECT_SOURCE_DIR}/cmake/LinkerFlags.cmake)

# Check for available compilers (has to happen after project definition for compiler flags)
if(NOT(CMAKE_COMPILER_IS_GNUCC OR MSVC_VERSION GREATER 1600))
	message(FATAL_ERROR "Just GCC & MSVC11+ compilers supported!")
endif()
message(STATUS "Generator: ${CMAKE_GENERATOR}")

# Request build options from user
if(MINGW)
	set(BUILD_STD_STATIC_VALUE ON)
else()
	set(BUILD_STD_STATIC_VALUE OFF)
endif()
option(BUILD_STD_STATIC "Link standard libraries statically?" ${BUILD_STD_STATIC_VALUE})
option(BUILD_TESTS "Build tests?" OFF)
set(DEPEND_LUA_INC "" CACHE PATH "Lua headers directory path")
set(DEPEND_LUA_LIB "lua" CACHE FILEPATH "Lua library path")
set(DEPEND_GLFW_INC "" CACHE PATH "GLFW headers directory path")
set(DEPEND_GLFW_LIB "glfw3" CACHE FILEPATH "GLFW library path")
if(CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

# Set compile warnings
if(MSVC)
	set(COMPILE_WARNINGS /W2)
else()
	set(COMPILE_WARNINGS -Wall -Wextra -pedantic -Wredundant-decls -Wunreachable-code -Wmissing-include-dirs -Wswitch-enum -Wno-missing-field-initializers)
endif()
add_compile_options(${COMPILE_WARNINGS})
message(STATUS "Compile warnings: ${COMPILE_WARNINGS}")

# Add feature definitions
set(COMPILE_DEFINITIONS -D_USE_MATH_DEFINES)
add_definitions(${COMPILE_DEFINITIONS})
message(STATUS "Additional compile definitions: ${COMPILE_DEFINITIONS}")

# Add compiler features (C++, vectorization)
if(CMAKE_COMPILER_IS_GNUCC)
	set(COMPILE_FEATURES -std=c++11 -msse2)
else()
	set(COMPILE_FEATURES /arch:SSE2)
endif()
add_compile_options(${COMPILE_FEATURES})
message(STATUS "Additional compiler features: ${COMPILE_FEATURES}")

# Set link time optimizations
if(MSVC)
	set(LINK_TIME_OPTIMIZATIONS /LTCG)
elseif(NOT(CMAKE_BUILD_TYPE STREQUAL Debug OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo))
	set(LINK_TIME_OPTIMIZATIONS -s)
endif()
add_linker_flags(${LINK_TIME_OPTIMIZATIONS})
message(STATUS "Link time optimizations: ${LINK_TIME_OPTIMIZATIONS}")

# Set standard libraries link type (static vs dynamic)
if(${BUILD_STD_STATIC})
	if(CMAKE_COMPILER_IS_GNUCC)
		add_linker_flags("-static")
	else()	# MSVC
		foreach(build_type
			CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
			CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
			if(${build_type} MATCHES /MD)
				string(REPLACE /MD /MT ${build_type} "${${build_type}}")
			endif()
		endforeach()
	endif()
	message(STATUS "Standard libraries link type: static")
else()
	message(STATUS "Standard libraries link type: dynamic")
endif()

# Generate configuration header
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
	${CMAKE_CURRENT_BINARY_DIR}/config.h
)

# Plan dynamic library compilation
file(GLOB LIBRARY_SOURCES *.cpp *.c *.hpp *.h *.rc)
add_library(fluag SHARED ${LIBRARY_SOURCES})
target_include_directories(fluag PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${DEPEND_LUA_INC} ${DEPEND_GLFW_INC})
target_link_libraries(fluag ${DEPEND_LUA_LIB} ${DEPEND_GLFW_LIB})

# Setup library installation
install(TARGETS fluag	# Runtime
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
message(STATUS "Binaries installation path: ${CMAKE_INSTALL_PREFIX}/bin")
message(STATUS "Library installation path: ${CMAKE_INSTALL_PREFIX}/lib")
install(FILES public.h DESTINATION include RENAME fluag.h)	# Public header
message(STATUS "Headers installation path: ${CMAKE_INSTALL_PREFIX}/include")
configure_file(	# Package config
	${CMAKE_CURRENT_SOURCE_DIR}/fluag.pc.in
	${CMAKE_CURRENT_BINARY_DIR}/fluag.pc
	@ONLY
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fluag.pc DESTINATION share/pkgconfig)
message(STATUS "Package config installation path: ${CMAKE_INSTALL_PREFIX}/share/pkgconfig")

# Add functionality tests
if(${BUILD_TESTS})

	# TODO

endif()