# Include script helpers
include(FindLua51)
include(FindOpenGL)
include(FindGLEW)
include(FindPNG)
include(${PROJECT_SOURCE_DIR}/cmake/FindGLFW.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/LinkerFlags.cmake)
find_package(Boost 1.54.0 COMPONENTS filesystem regex)
if(NOT WIN32)
	include(${PROJECT_SOURCE_DIR}/cmake/FindFontConfig.cmake)
endif()
if(NOT CMAKE_CROSSCOMPILING)
	include(${PROJECT_SOURCE_DIR}/cmake/CheckSIMD.cmake)
endif()

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
option(BUILD_FORCE_SINGLE_THREAD "Force Lua script to run in same thread?" ON)
option(BUILD_WITH_AVX "Try auto-vectorization with AVX?" ${AVX_SWITCH})
option(BUILD_WITH_SSE2 "Try auto-vectorization with SSE2?" ${SSE2_SWITCH})
option(BUILD_WITH_MMX "Try auto-vectorization with MMX?" ${MMX_SWITCH})
set(DEPEND_LUA_INC "${LUA_INCLUDE_DIR}" CACHE PATH "Lua headers directory path")
set(DEPEND_LUA_LIB "${LUA_LIBRARIES}" CACHE FILEPATH "Lua library path")
set(DEPEND_OGL_LIB "${OPENGL_gl_LIBRARY}" CACHE FILEPATH "OpenGL libraries path")
set(DEPEND_GLEW_INC "${GLEW_INCLUDE_DIRS}" CACHE PATH "GLEW headers directory path")
set(DEPEND_GLEW_LIB "${GLEW_LIBRARIES}" CACHE FILEPATH "GLEW library path")
set(DEPEND_GLFW_INC "${GLFW_INCLUDE_DIR}" CACHE PATH "GLFW headers directory path")
set(DEPEND_GLFW_LIB "${GLFW_LIBRARIES}" CACHE FILEPATH "GLFW libraries path")
set(DEPEND_PNG_INC "${PNG_INCLUDE_DIRS}" CACHE PATH "Libpng headers directory path")
set(DEPEND_PNG_LIB "${PNG_LIBRARIES}" CACHE FILEPATH "Libpng libraries path")
set(DEPEND_BOOST_INC ${Boost_INCLUDE_DIRS} CACHE PATH "Boost headers directory path")
set(DEPEND_BOOST_LIB ${Boost_LIBRARIES} CACHE FILEPATH "Boost filesystem & regular expressions libraries path")
if(NOT WIN32)
	set(DEPEND_FC_INC "${FONTCONFIG_INCLUDE_DIRS}" CACHE PATH "FontConfig headers directory path")
	set(DEPEND_FC_LIB "${FONTCONFIG_LIBRARIES}" CACHE FILEPATH "FontConfig library path")
endif()
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
set(COMPILE_DEFINITIONS ${PNG_DEFINITIONS})
if(WIN32)
	set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} -DWIN_LEAN_AND_MEAN)
endif()
if(${BUILD_FORCE_SINGLE_THREAD})
	set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} -DFLUAG_FORCE_SINGLE_THREAD)
endif()
add_definitions(${COMPILE_DEFINITIONS})
message(STATUS "Additional compile definitions: ${COMPILE_DEFINITIONS}")

# Add compiler features (C++11, vectorization)
if(CMAKE_COMPILER_IS_GNUCC)
	set(COMPILE_FEATURES -std=c++11)
endif()
if(BUILD_WITH_AVX)
	set(COMPILE_FEATURES ${COMPILE_FEATURES} ${AVX_FLAG})
elseif(BUILD_WITH_SSE2)
	set(COMPILE_FEATURES ${COMPILE_FEATURES} ${SSE2_FLAG})
elseif(BUILD_WITH_MMX)
	set(COMPILE_FEATURES ${COMPILE_FEATURES} ${MMX_FLAG})
endif()
add_compile_options(${COMPILE_FEATURES})
message(STATUS "Additional compiler features: ${COMPILE_FEATURES}")

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
file(GLOB_RECURSE LIBRARY_SOURCES *.cpp *.hpp *.c *.h *.rc)
if(NOT WIN32)
	list(REMOVE_ITEM LIBRARY_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/interfaces/avisynth.cpp")
endif()
add_library(fluag SHARED ${LIBRARY_SOURCES})
target_include_directories(fluag PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${DEPEND_LUA_INC} ${DEPEND_GLEW_INC} ${DEPEND_GLFW_INC} ${DEPEND_PNG_INC} ${DEPEND_BOOST_INC} ${DEPEND_FC_INC})
target_link_libraries(fluag ${DEPEND_LUA_LIB} ${DEPEND_GLEW_LIB} ${DEPEND_GLFW_LIB} ${DEPEND_OGL_LIB} ${DEPEND_PNG_LIB} ${DEPEND_BOOST_LIB} ${DEPEND_FC_LIB})
if(NOT MSVC)
	target_link_libraries(fluag -pthread)
endif()

# Setup library installation
install(TARGETS fluag	# Runtime
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
message(STATUS "Binaries installation path: ${CMAKE_INSTALL_PREFIX}/bin")
message(STATUS "Library installation path: ${CMAKE_INSTALL_PREFIX}/lib")
install(FILES interfaces/public.h DESTINATION include RENAME fluag.h)	# Public header
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
	# Build & run hello test
	add_executable(hello_test_exe ${PROJECT_SOURCE_DIR}/tests/hello.c)
	if(CMAKE_COMPILER_IS_GNUCC)
		target_link_libraries(hello_test_exe dl "-Wl,-rpath=.")
	endif()
	add_test(hello_test hello_test_exe "${CMAKE_SHARED_LIBRARY_PREFIX}fluag${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()
